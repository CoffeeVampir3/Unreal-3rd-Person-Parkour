// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedPlayerController.generated.h"

class UGameInputConfiguration;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class GAMEANIMATIONSAMPLE_API AEnhancedPlayerController : public APlayerController
{
	GENERATED_BODY()
	AEnhancedPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	UPROPERTY(EditAnywhere, Category="Input")
    TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UGameInputConfiguration> InputConfig;
};
