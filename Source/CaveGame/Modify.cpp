

#include "CaveGame.h"
#include "Modify.h"

UModify::UModify(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

FModSet UModify::triangle(FModSet input){
	FModSet m = makeFrom(input);
	FDynamicMeshVertex vertex;
	vertex.Position = FVector(0, 0, 0);
	m.Mesh->Vertices.Add(vertex);
	vertex.Position = FVector(0, 100, 0);
	m.Mesh->Vertices.Add(vertex);
	vertex.Position = FVector(100, 0, 0);
	m.Mesh->Vertices.Add(vertex);
	FMod mod;
	FFace face;
	face.Indices.Add(0);
	face.Indices.Add(1);
	face.Indices.Add(2);
	mod.Faces.Add(input.Mesh->Faces.Add(face));
	m.Mods.Add(mod);
	return m;
}

FModSet UModify::circle(FModSet input, int32 vertexCount){
	FModSet m = makeFrom(input);
	FDynamicMeshVertex vertex;
	float arcLength = PI * 2 / vertexCount;
	FMod mod;
	FFace face;
	for (int32 i = 0; i < vertexCount; i++){
		vertex.Position = FVector(cosf(-i*arcLength)*100, sinf(-i*arcLength)*100, 0);
		face.Indices.Add(m.Mesh->Vertices.Add(vertex));
	}
	mod.Faces.Add(input.Mesh->Faces.Add(face));
	m.Mods.Add(mod);
	return m;
}

FModSet UModify::subset(FModSet input, int32 startIndex, int32 endIndex)
{
	FModSet m = makeFrom(input);
	for (FMod mod : input.Mods){
		FMod filtered;
		int32 i = 0;
		int32 startIndexPositive = startIndex;
		if (startIndex < 0){
			startIndexPositive = mod.Faces.Num() + startIndex;
		}
		int32 endIndexPositive = endIndex;
		if (endIndex < 0){
			endIndexPositive = mod.Faces.Num() + endIndex;
		}
		for (int32 f : mod.Faces){
			if (i >= startIndexPositive && i <= endIndexPositive){
				filtered.Faces.Add(f);
			}
			i++;
		}
		m.Mods.Add(filtered);
	}
	return m;
}

FModSet UModify::smooth(FModSet input, float shrink, float grow, int32 iterations){
	FModSet m = makeFrom(input);

	for (FMod mod : input.Mods){
		TMap<int32, Adjacent> neighbors;
		for (int32 faceIndex : mod.Faces){
			FFace face = m.Mesh->Faces[faceIndex];
			int32 vertexCount = face.Indices.Num();
			for (int32 i = 0; i < vertexCount; i++){
				Adjacent adj = neighbors.FindOrAdd(face.Indices[i]);
				adj.Neighbors.Add(face.Indices[(i + vertexCount - 1) % vertexCount]);
				adj.Neighbors.Add(face.Indices[(i + 1) % vertexCount]);
				adj.Position = m.Mesh->Vertices[face.Indices[i]].Position;
				neighbors.Add(face.Indices[i], adj);
			}
		}
		TArray<int32> indices;
		neighbors.GetKeys(indices);
		for (int32 i = 0; i < iterations * 2; i++){
			float scale = shrink;
			if (i % 2 == 1){
				scale = -grow;
			}
			for (int32 vertexIndex : indices){
				Adjacent adj = neighbors[vertexIndex];
				FVector barycenter = FVector(0, 0, 0);
				float total = 0;
				for (int32 neighborIndex : adj.Neighbors){
					FVector neighbor = m.Mesh->Vertices[neighborIndex].Position;
					FVector delta = neighbor - adj.Position;
					float weight = 1/FVector::Dist(adj.Position, neighbor);
					barycenter += delta*weight;
					total += weight;
				}
				barycenter = barycenter / total;
				m.Mesh->Vertices[vertexIndex].Position = adj.Position + barycenter * scale;
			}
			for (int32 vertexIndex : indices){
				neighbors[vertexIndex].Position = m.Mesh->Vertices[vertexIndex].Position;
			}
		}

	}
	return input;
}

FModSet UModify::selectAllFaces(FModSet input){
	FModSet m = makeFrom(input);

	FMod mod;
	for (int32 i = 0; i < input.Mesh->Faces.Num(); i++){
		mod.Faces.Add(i);
	}
	m.Mods.Add(mod);
	return m;
}

FModSet UModify::extrude(FModSet input, float distance, FModSet& caps)
{
	FModSet m = makeFrom(input);
	copyContext(input, caps);
	for (FMod mod : input.Mods){
		for (int32 faceIndex : mod.Faces){
			FFace face = m.Mesh->Faces[faceIndex];
			face.Hide();
			FMod sides;
			FFace cap;
			FVector p0 = m.Mesh->Vertices[face.Indices[0]].Position;
			FVector p1 = m.Mesh->Vertices[face.Indices[1]].Position;
			FVector p2 = m.Mesh->Vertices[face.Indices[2]].Position;
			FVector extrusion = FVector::CrossProduct(p0 - p1, p2 - p1).SafeNormal()*distance;

			FDynamicMeshVertex vertex;

			int32 firstBaseIndex = face.Indices[0];
			vertex.Position = m.Mesh->Vertices[firstBaseIndex].Position + extrusion;
			int32 firstCapIndex = m.Mesh->Vertices.Add(vertex);
			cap.Indices.Add(firstCapIndex);
			int32 previousBaseIndex = firstBaseIndex;
			int32 previousCapIndex = firstCapIndex;
			for (int32 i = 1; i < face.Indices.Num(); i++){
				int32 currentBaseIndex = face.Indices[i];
				vertex.Position = m.Mesh->Vertices[currentBaseIndex].Position + extrusion;
				int32 currentCapIndex = m.Mesh->Vertices.Add(vertex);
				cap.Indices.Add(currentCapIndex);

				FFace side;

				side.Indices.Add(previousCapIndex);
				side.Indices.Add(previousBaseIndex);
				side.Indices.Add(currentBaseIndex);
				side.Indices.Add(currentCapIndex);
				sides.Faces.Add(m.Mesh->Faces.Add(side));
				previousBaseIndex = currentBaseIndex;
				previousCapIndex = currentCapIndex;
			}
			FFace last;
			last.Indices.Add(previousCapIndex);
			last.Indices.Add(previousBaseIndex);
			last.Indices.Add(firstBaseIndex);
			last.Indices.Add(firstCapIndex);
			sides.Faces.Add(m.Mesh->Faces.Add(last));

			FMod capMod;
			capMod.Faces.Add(m.Mesh->Faces.Add(cap));
			caps.Mods.Add(capMod);

			m.Mods.Add(sides);
		}
	}
	return m;
}

FModSet UModify::face(FModSet input)
{

	return input;
}


FModSet UModify::edit(UGenerator * mesh){
	FModSet m;
	m.Mesh = mesh;
	return m;
}



void UModify::finish(FModSet input){
	for (FFace face : input.Mesh->Faces)
	{
		if (face.IsVisible()){
			int32 firstIndex = face.Indices[0];
			for (int32 i = 0; i < face.Indices.Num() - 2; i++)
			{
				input.Mesh->Indices.Add(firstIndex);
				input.Mesh->Indices.Add(face.Indices[i + 1]);
				input.Mesh->Indices.Add(face.Indices[i + 2]);
			}
		}
	}

	input.Mesh->SetGeneratedMeshTriangles(input.Mesh->Vertices,input.Mesh->Indices);
}

void UModify::copyContext(FModSet& from, FModSet& to){
	to.Mesh = from.Mesh;
	TArray<FMod> mods;
	to.Mods = mods;
}

FModSet UModify::makeFrom(FModSet input){
	FModSet m;
	copyContext(input, m);
	return m;
}