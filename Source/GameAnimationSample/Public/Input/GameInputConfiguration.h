// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "GameInputConfiguration.generated.h"

USTRUCT(BlueprintType)
struct FGameInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	class UInputAction* InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag = FGameplayTag();
};

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSAMPLE_API UGameInputConfiguration : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Input Actions", meta=(ExpandBoolAsExecs="ReturnValue"))
	bool TryGetInputActionForTag(const FGameplayTag& InputTag, UInputAction*& OutInputAction) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameInputAction> AbilityInputActions;
};
