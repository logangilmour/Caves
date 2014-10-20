

#include "CaveGame.h"
#include "Modify.h"

UModify::UModify(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

FModSet UModify::triangle(FModSet input){
	FModSet m;
	m.Mesh = input.Mesh;
	FDynamicMeshVertex vertex;
	vertex.Position = FVector(0, 0, 0);
	m.Mesh->Vertices.Add(vertex);
	vertex.Position = FVector(0, 100, 0);
	m.Mesh->Vertices.Add(vertex);
	vertex.Position = FVector(100, 0, 0);
	m.Mesh->Vertices.Add(vertex);
	FMod mod;
	FVertexSet face;
	face.Indices.Add(0);
	face.Indices.Add(1);
	face.Indices.Add(2);
	mod.VertexSets.Add(face);
	m.Mods.Add(mod);
	return m;
}

FModSet UModify::circle(FModSet input, int32 vertexCount){
	FModSet m;
	m.Mesh = input.Mesh;
	FDynamicMeshVertex vertex;
	float arcLength = PI * 2 / vertexCount;
	FMod mod;
	FVertexSet face;
	for (int32 i = 0; i < vertexCount; i++){
		vertex.Position = FVector(cosf(-i*arcLength)*100, sinf(-i*arcLength)*100, 0);
		face.Indices.Add(m.Mesh->Vertices.Add(vertex));
	}
	mod.VertexSets.Add(face);
	m.Mods.Add(mod);
	return m;
}

FModSet UModify::subset(FModSet input, int32 startIndex, int32 endIndex)
{
	FModSet m;
	m.Mesh = input.Mesh;
	for (FMod mod : input.Mods){
		FMod filtered;
		int32 i = 0;
		int32 startIndexPositive = startIndex;
		if (startIndex < 0){
			startIndexPositive = mod.VertexSets.Num() + startIndex;
		}
		int32 endIndexPositive = endIndex;
		if (endIndex < 0){
			endIndexPositive = mod.VertexSets.Num() + endIndex;
		}
		for (FVertexSet verts : mod.VertexSets){
			if (i >= startIndexPositive && i <= endIndexPositive){
				filtered.VertexSets.Add(verts);
			}
			i++;
		}
		m.Mods.Add(filtered);
	}
	return m;
}

FModSet UModify::extrude(FModSet input, float distance, FModSet& caps)
{
	FModSet m;
	m.Mesh = input.Mesh;
	caps.Mesh = input.Mesh;
	for (FMod mod : input.Mods){
		for (FVertexSet face : mod.VertexSets){
			FMod sides;
			FVertexSet cap;
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

				FVertexSet side;

				side.Indices.Add(previousCapIndex);
				side.Indices.Add(previousBaseIndex);
				side.Indices.Add(currentBaseIndex);
				side.Indices.Add(currentCapIndex);
				sides.VertexSets.Add(side);
				previousBaseIndex = currentBaseIndex;
				previousCapIndex = currentCapIndex;
			}
			FVertexSet last;
			last.Indices.Add(previousCapIndex);
			last.Indices.Add(previousBaseIndex);
			last.Indices.Add(firstBaseIndex);
			last.Indices.Add(firstCapIndex);
			sides.VertexSets.Add(last);

			FMod capMod;
			capMod.VertexSets.Add(cap);
			caps.Mods.Add(capMod);

			m.Mods.Add(sides);
		}
	}
	return m;
}

FModSet UModify::face(FModSet input)
{
	for (FMod mod : input.Mods){
		for (FVertexSet face : mod.VertexSets)
		{
			int32 firstIndex = face.Indices[0];
			for (int32 i = 0; i < face.Indices.Num() - 2; i++)
			{
				input.Mesh->Indices.Add(firstIndex);
				input.Mesh->Indices.Add(face.Indices[i + 1]);
				input.Mesh->Indices.Add(face.Indices[i + 2]);
			}
		}
	}
	return input;
}


FModSet UModify::edit(UGenerator * mesh){
	FModSet m;
	m.Mesh = mesh;
	return m;
}

void UModify::finish(FModSet input){
	input.Mesh->SetGeneratedMeshTriangles(input.Mesh->Vertices,input.Mesh->Indices);
}