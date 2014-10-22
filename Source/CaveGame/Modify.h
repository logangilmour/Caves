

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

UCLASS(BlueprintType)
class UModify : public UObject
{
	GENERATED_UCLASS_BODY()

public:

		UFUNCTION(BlueprintCallable, Category="Procedural")
		static FModSet triangle(FModSet input);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet circle(FModSet input, int32 vertexCount);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet extrude(FModSet input, float distance, FModSet& caps);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet face(FModSet input);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet subset(FModSet input, int32 startIndex = 0, int32 endIndex = -1);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet selectAllFaces(FModSet input);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet smooth(FModSet input,float shrink=0.3, float grow=0.5, int32 iterations=1);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static FModSet edit(UGenerator * mesh);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static void finish(FModSet input);

		static void copyContext(FModSet& from, FModSet& to);

		static FModSet makeFrom(FModSet input);

};
