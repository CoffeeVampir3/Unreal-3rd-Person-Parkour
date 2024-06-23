// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/EnhancedPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Input/EnhancedPlayerInputComponent.h"

AEnhancedPlayerController::AEnhancedPlayerController()
{
	bReplicates = true;
}

void AEnhancedPlayerController::BeginPlay()
{
	Super::BeginPlay();

	check(InputMappingContext);

    //Input Setup -- For the local player.
    if (auto InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
    	InputSubsystem->AddMappingContext(InputMappingContext, 0);
    }
}

void AEnhancedPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	const auto EnhancedInputComponent = CastChecked<UEnhancedPlayerInputComponent>(InputComponent);
    EnhancedInputComponent->BindAbilityActions(
    	InputConfig,
    	this,
    	&ThisClass::AbilityInputTagPressed,
    	&ThisClass::AbilityInputTagReleased,
    	&ThisClass::AbilityInputTagHeld);
}

void AEnhancedPlayerController::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	//AbilitySystemComponent->AbilityInputTagHeld(InputTag);
}

void AEnhancedPlayerController::AbilityInputTagReleased(const FGameplayTag InputTag)
{
	//AbilitySystemComponent->AbilityInputTagHeld(InputTag);
}

void AEnhancedPlayerController::AbilityInputTagHeld(const FGameplayTag InputTag)
{
	//AbilitySystemComponent->AbilityInputTagHeld(InputTag);
}
