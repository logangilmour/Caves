

#pragma once
#include "CaveGame.h"
#include "Generator.h"
#include "Modify.generated.h"


/**
 * A Mesh and a working set of faces on which to perform operations. Should support multiple outputs by passing in references so blueprints are great.
 */

struct Adjacent
{
	TSet<int32> Neighbors;
	FVector Position;
};



struct Edges
{
	TMap<int32,int32> Neighbors;
};

struct EdgeLabels
{
	TMap<int32, Edges> LabelSet;
	void Add(int32 A, int32 B, int32 label){
		if (B > A){
			int32 temp = B;
			B = A;
			A = temp;
		}

		LabelSet.FindOrAdd(A).Neighbors.Add(B, label);
	}

	void Increment(int32 A, int32 B){
		if (B > A){
			int32 temp = B;
			B = A;
			A = temp;
		}
		int32 label = 1;
		if (Contains(A, B)){
			label += GetLabel(A, B);
		}
		Add(A, B, label);
	}

	bool Contains(int32 A, int32 B){
		if (B > A){
			int32 temp = B;
			B = A;
			A = temp;
		}
		return LabelSet.Contains(A) && LabelSet[A].Neighbors.Contains(B);
	}

	int32 GetLabel(int32 A, int32 B){
		if (B > A){
			int32 temp = B;
			B = A;
			A = temp;
		}
		return LabelSet[A].Neighbors[B];
	}

	void AddFace(FFace face){
		int32 vertexCount = face.Indices.Num();
		for (int32 i = 0; i < vertexCount; i++){
			int32 current = face.Indices[i];
			int32 next = face.Indices[(i + 1) % vertexCount];
			Increment(current, next);
		}
	}

	TSet<int32> EdgeVertices(){
		TSet<int32> edgeVerts;
		TArray<int32> AIndexes;
		LabelSet.GetKeys(AIndexes);
		for (int32 i = 0; i < AIndexes.Num(); i++){
			int32 A = AIndexes[i];

			TArray<int32> BIndexes;
			LabelSet[A].Neighbors.GetKeys(BIndexes);
			for (int32 j = 0; j < BIndexes.Num(); j++){
				int32 B = BIndexes[j];

				if (LabelSet[A].Neighbors[B] < 2){
					edgeVerts.Add(A);
					edgeVerts.Add(B);
				}
			}
		}
		return edgeVerts;
	}
};

UCLASS(BlueprintType)
class UModify : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet circle(FModSet input, int32 vertexCount);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet extrude(FModSet input, float distance, FModSet& caps);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet subset(FModSet input, int32 startIndex = 0, int32 endIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet subdivide(FModSet input);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet calculateNormals(FModSet input);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet inflate(FModSet input, float distance);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet selectAllFaces(FModSet input);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet smooth(FModSet input, float factor = 0.5, int32 iterations = 1);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet transform(FModSet input, const FTransform t);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet edit(UGenerator * mesh);

	UFUNCTION(BlueprintCallable, Category = "Procedural")
		static void finish(FModSet input);

	static void copyContext(FModSet& from, FModSet& to);

	static FModSet makeFrom(FModSet input);

	static FVector barycenter(UGenerator * mesh, FMod mod);


};
