

#pragma once
#include "CaveGame.h"
#include "Generator.h"
#include "Modify.generated.h"


/**
 * A Mesh and a working set of faces on which to perform operations. Should support multiple outputs by passing in references so blueprints are great.
 */

USTRUCT()
struct FVertexSet
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TArray<int32> Indices;
};

USTRUCT(BlueprintType)
struct FMod
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FVertexSet> VertexSets;
};

USTRUCT(BlueprintType)
struct FModSet
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FMod> Mods;

	UPROPERTY()
	UGenerator * Mesh;
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
		static FModSet edit(UGenerator * mesh);

		UFUNCTION(BlueprintCallable, Category = "Procedural")
		static void finish(FModSet input);
};
