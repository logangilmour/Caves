

#include "CaveGame.h"
#include "Modify.h"

static const FVector BaseOrientation(-1, 0, 0);
static const FVector BaseDirection(0, 0, 1);

UModify::UModify(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

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
	mod.Orientation = BaseOrientation;
	mod.Direction = BaseDirection;
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

FModSet UModify::smooth(FModSet input, float factor, int32 iterations){
	FModSet m = makeFrom(input);

	for (FMod mod : input.Mods){
		TMap<int32, Adjacent> neighbors;
		EdgeLabels edges;
		for (int32 faceIndex : mod.Faces){
			FFace face = m.Mesh->Faces[faceIndex];
			edges.AddFace(face);
			int32 vertexCount = face.Indices.Num();
			for (int32 i = 0; i < vertexCount; i++){
				int32 current = face.Indices[i];
				int32 previous = face.Indices[(i + vertexCount - 1) % vertexCount];
				int32 next = face.Indices[(i + 1) % vertexCount];
				Adjacent adj = neighbors.FindOrAdd(current);
				adj.Neighbors.Add(previous);
				adj.Neighbors.Add(next);

				adj.Position = m.Mesh->Vertices[current].Position;
				neighbors.Add(current, adj);
			}
		}
		TSet<int32> edgeVertices = edges.EdgeVertices();
		TArray<int32> indices;
		neighbors.GetKeys(indices);
		for (int32 i = 0; i < iterations; i++){
			for (int32 vertexIndex : indices){
				if (edgeVertices.Contains(vertexIndex))continue;
				Adjacent adj = neighbors[vertexIndex];
				FVector barycenter = FVector(0, 0, 0);
				float total = 0;
				for (int32 neighborIndex : adj.Neighbors){
					FVector neighbor = m.Mesh->Vertices[neighborIndex].Position;
					FVector delta = neighbor - adj.Position;
					float weight = 1 / FVector::Dist(adj.Position, neighbor);
					barycenter += delta*weight;
					total += weight;
				}
				barycenter = barycenter / total;
				m.Mesh->Vertices[vertexIndex].Position = adj.Position + barycenter * factor;
				
			}
			for (int32 vertexIndex : indices){
				neighbors[vertexIndex].Position = m.Mesh->Vertices[vertexIndex].Position;
			}
		}

	}
	return input;
}

FModSet UModify::transform(FModSet input, FTransform t){
	t.DebugPrint();
	FModSet m = makeFrom(input);
	for (FMod mod : input.Mods){
		FMod transformed;
		FVector bary = barycenter(m.Mesh, mod);
		FQuat dir = FQuat::FindBetween(mod.Direction, BaseDirection);

		FQuat or = FQuat::FindBetween(dir.RotateVector(mod.Orientation), BaseOrientation);

		dir = or*dir;

		for (int32 f : mod.Faces){
			FFace face = m.Mesh->Faces[f];
			for (int32 i = 0; i < face.Indices.Num();i++){
				int32 vindex = face.Indices[i];
				FVector temp = dir.RotateVector((m.Mesh->Vertices[vindex].Position - bary));
				temp = t.TransformPosition(temp);

				m.Mesh->Vertices[vindex].Position = dir.Inverse().RotateVector(temp) + bary;
			}
		}

		FVector direction = dir.Inverse().RotateVector(t.GetRotation().RotateVector(BaseDirection)).SafeNormal();
		FVector orientation = (direction ^ mod.Direction).SafeNormal();

		if (orientation == FVector::ZeroVector){
			orientation = mod.Orientation;
		}

		transformed.Orientation = orientation;
		transformed.Direction = direction;
		transformed.Faces = mod.Faces;
		m.Mods.Add(transformed);

	}
	return m;
}

FVector UModify::barycenter(UGenerator * mesh, FMod mod){
	FVector bary = FVector::ZeroVector;
	int32 count = 0;
	for (int32 f : mod.Faces){
		FFace face = mesh->Faces[f];
		count += face.Indices.Num();
		for (int32 vindex : face.Indices){
			bary += mesh->Vertices[vindex].Position;
		}
	}
	bary /= count;
	return bary;
}

FModSet UModify::subdivide(FModSet input){

	FModSet m = makeFrom(input);
	for (FMod mod : input.Mods){
		FMod subFaces;
		subFaces.Direction = mod.Direction;
		subFaces.Orientation = mod.Orientation;
		EdgeLabels edges;
		for (int32 faceIndex : mod.Faces){
			FFace face = m.Mesh->Faces[faceIndex];
			m.Mesh->Faces[faceIndex].Hide();
			int32 vertexCount = face.Indices.Num();

			TArray<int32> edgeRing;
			FVector facePoint = FVector(0, 0, 0);
			for (int32 i = 0; i < vertexCount; i++){
				int32 current = face.Indices[i];
				int32 next = face.Indices[(i + 1) % vertexCount];
				facePoint += m.Mesh->Vertices[current].Position;
				int32 edgePointIndex;
				if (edges.Contains(current,next)){
					edgePointIndex = edges.GetLabel(current, next);
				}else{
					FDynamicMeshVertex vertex;
					vertex.Position = (m.Mesh->Vertices[current].Position + m.Mesh->Vertices[next].Position) / 2;
					edgePointIndex = m.Mesh->Vertices.Add(vertex);
					edges.Add(current, next, edgePointIndex);
				}
				edgeRing.Add(current);
				edgeRing.Add(edgePointIndex);
			}

			facePoint /= vertexCount;

			FDynamicMeshVertex vertex;
			vertex.Position = facePoint;
			int32 facePointIndex = m.Mesh->Vertices.Add(vertex);

			int32 count = edgeRing.Num();
			for (int32 i = 0; i < count; i += 2){
				FFace face;
				face.Indices.Add(facePointIndex);
				face.Indices.Add(edgeRing[(i + count - 1) % count]);
				face.Indices.Add(edgeRing[i]);
				face.Indices.Add(edgeRing[(i + 1) % count]);
				subFaces.Faces.Add(m.Mesh->Faces.Add(face));
			}
		}

		m.Mods.Add(subFaces);
	}

	return m;
}

FModSet UModify::calculateNormals(FModSet m){

	for (FMod mod : m.Mods){
		TMap<int32, FNormalList> faceLookup;
		for (int32 faceIndex : mod.Faces){
			FFace face = m.Mesh->Faces[faceIndex];
			FVector v0 = m.Mesh->Vertices[face.Indices[0]].Position;
			FVector v1 = m.Mesh->Vertices[face.Indices[1]].Position;
			FVector v2 = m.Mesh->Vertices[face.Indices[2]].Position;

			const FVector Edge01 = (v1 - v0);
			const FVector Edge02 = (v2 - v0);



			const FVector Normal = (Edge02 ^ Edge01);
			for (int32 vertexIndex : face.Indices){
				faceLookup.FindOrAdd(vertexIndex).Normals.Add(Normal);
			}
		}
		TArray<int32> keys;
		faceLookup.GetKeys(keys);

		for (int32 key : keys){

			FVector normal = FVector(0, 0, 0);
			if (faceLookup.Contains(key)){
				for (FVector v : faceLookup[key].Normals){
					normal += v;
				}
			}
			FVector TangentZ = normal.SafeNormal();
			m.Mesh->Vertices[key].SetTangents(TangentZ, TangentZ, TangentZ);
		}
	}
	return m;
}

FModSet UModify::inflate(FModSet m, float distance){
	calculateNormals(m);
	for (FMod mod : m.Mods){
		EdgeLabels edges;
		TSet<int32> vertices;
		for (int32 faceIndex : mod.Faces){
			FFace face = m.Mesh->Faces[faceIndex];
			edges.AddFace(face);
			vertices.Append(face.Indices);
		}
		TSet<int32> edgeSet = edges.EdgeVertices();
		for (int32 vertexIndex: vertices){
			if (!edgeSet.Contains(vertexIndex)){
				m.Mesh->Vertices[vertexIndex].Position += distance*m.Mesh->Vertices[vertexIndex].TangentZ;
			}
		}
	}
	return m;
}

FModSet UModify::selectAllFaces(FModSet input){
	FModSet m = makeFrom(input);

	FMod mod;
	mod.Direction = BaseDirection;
	mod.Orientation = BaseOrientation;
	for (int32 i = 0; i < input.Mesh->Faces.Num(); i++){
		if (input.Mesh->Faces[i].IsVisible()){
			mod.Faces.Add(i);
		}
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
			m.Mesh->Faces[faceIndex].Hide();
			FMod sides;
			FMod capMod;

			FFace cap;
			FVector p0 = m.Mesh->Vertices[face.Indices[0]].Position;
			FVector p1 = m.Mesh->Vertices[face.Indices[1]].Position;
			FVector p2 = m.Mesh->Vertices[face.Indices[2]].Position;
			FVector extrusion = ((p0 - p1) ^ (p2 - p1)).SafeNormal()*distance;

			FVector orientation = (extrusion ^ mod.Direction).SafeNormal();

			if (orientation == FVector::ZeroVector){
				orientation = mod.Orientation;
			}
			sides.Direction = extrusion.SafeNormal();
			sides.Orientation = orientation;
			capMod.Direction = extrusion.SafeNormal();
			capMod.Orientation = orientation;
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

			capMod.Faces.Add(m.Mesh->Faces.Add(cap));
			caps.Mods.Add(capMod);

			m.Mods.Add(sides);
		}
	}
	return m;
}

FModSet UModify::edit(UGenerator * mesh){
	FModSet m;
	m.Mesh = mesh;
	return m;
}



void UModify::finish(FModSet input){
	calculateNormals(selectAllFaces(input));
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