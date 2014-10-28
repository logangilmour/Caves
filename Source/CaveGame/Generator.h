// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GeneratedMeshComponent.h"
#include "Generator.generated.h"

/**
* Procedural Generation
*/
class UGenerator;

USTRUCT()
struct FFace
{
	GENERATED_USTRUCT_BODY()



		UPROPERTY()
		TArray<int32> Indices;
	TMap<FName, int32> Attributes;

	void Hide(){
		Attributes.Add(FName(TEXT("visible")), 0);
	}

	int32 IsVisible(){
		return Attributes[FName(TEXT("visible"))];
	}

	FFace(){
		Attributes.Add(FName(TEXT("visible")), 1);
	}
};

USTRUCT(BlueprintType)
struct FMod
{
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY()
	TArray<int32> Faces;

	UPROPERTY()
		FVector Direction;
	UPROPERTY()
		FVector Orientation;

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

UCLASS(editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class UGenerator : public UGeneratedMeshComponent
{
	GENERATED_UCLASS_BODY()

public:
	TArray<FFace> Faces;
};

