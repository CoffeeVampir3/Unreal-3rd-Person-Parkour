// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "TraversableActor.generated.h"

USTRUCT(BlueprintType)
struct FTraversableCheckResult
{
	GENERATED_BODY()
	float ObstacleHeight {0.0f};
	float ObstacleDepth {0.0f};
	float BackLedgeHeight {0.0f};

	FVector FrontLedgeLocation {0.0f, 0.0f, 0.0f};
	FVector FrontLedgeNormal {0.0f, 0.0f, 0.0f};
	bool bHasFrontLedge {false};

	FVector BackLedgeLocation {0.0f, 0.0f, 0.0f};
	FVector BackLedgeNormal {0.0f, 0.0f, 0.0f};
	bool bHasBackLedge {false};

	FVector BackFloorLocation {0.0f, 0.0f, 0.0f};
	bool bHasBackFloor {false};

	UPROPERTY()
	TObjectPtr<UObject> HitObject;
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> HitComponent;
};

UCLASS()
class GAMEANIMATIONSAMPLE_API ATraversableActor : public AActor
{
	GENERATED_BODY()

public:
	USplineComponent* FindClosestLedgeToLocation(const FVector& Location);
	UFUNCTION(BlueprintCallable)
	FTraversableCheckResult GetLedgeTransforms(FVector HitLocation, FVector ActorLocation);
	
	UPROPERTY(BlueprintReadWrite)
	TArray<USplineComponent*> LedgeSplines;

	UPROPERTY(BlueprintReadWrite)
	TMap<USplineComponent*, USplineComponent*> OppositeLedges;
};
