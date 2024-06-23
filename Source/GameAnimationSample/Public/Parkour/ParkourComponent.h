// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "MotionWarpingComponent.h"
#include "Components/ActorComponent.h"
#include "CoroStateMachine/CoroStateMachine.h"
#include "Traversables/TraversableActor.h"
#include "ParkourComponent.generated.h"

class UChooserTable;
class UInputAction;

UENUM(BlueprintType)
enum class EMovementGait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class EParkourActionType : uint8
{
	Hurdle UMETA(DisplayName = "Hurdle"),
	Vault UMETA(DisplayName = "Vault"),
	Mantle UMETA(DisplayName = "Mantle"),
	NoValidAction UMETA(DisplayName = "None"),
};

USTRUCT(BlueprintType)
struct FMovementChooserParams
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	EMovementGait Gait{EMovementGait::Walk};
	UPROPERTY(BlueprintReadWrite)
	EParkourActionType ParkourAction{EParkourActionType::Hurdle};
	UPROPERTY(BlueprintReadWrite)
	float ObstacleHeight{0.0};
	UPROPERTY(BlueprintReadWrite)
	float ObstacleDepth{0.0};
	UPROPERTY(BlueprintReadWrite)
	float Speed{0.0};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSetInteractionTransformDelegate, FTransform, NewTransform);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTraverseLookup, FMovementChooserParams, ChooserParams);

class UCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEANIMATIONSAMPLE_API UParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UParkourComponent();
	UFUNCTION(BlueprintCallable)
	void GetDesiredGait(const FVector StickMovementInput);
	UFUNCTION(BlueprintCallable)
	void UpdateRotation(bool WantsToStrafe);
	UFUNCTION(BlueprintCallable)
	float CalculateMaxSpeed();
	CoroState ParkourStateMachine();
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void LookGamepad(const FInputActionValue& InputActionValue);
	void WalkToggle(const FInputActionValue& InputActionValue);
	void Sprint(const FInputActionValue& InputActionValue);
	float GetForwardTraversalTraceDistance(const FVector& CurrentVelocity, const FRotator& CurrentRotation) const;
	bool SelectParkourMontage(EParkourActionType ActionType, const FTraversableCheckResult& TraversalCheck,
	                          UAnimMontage*& OutAnim, float& OutTime, float& OutPlayRate) const;
	void UpdateMotionWarping(const UAnimMontage* Anim, const FTraversableCheckResult& TraversalCheck,
	                         const EParkourActionType ActionType) const;
	static bool DetermineParkourAction(const FTraversableCheckResult& TraversalCheck,
	                                   const bool bDebugEnabled,
	                                   EParkourActionType& OutParkourActionType);
	bool PerformTraversalCheck(FTraversableCheckResult& OutTraversalCheck, float CapsuleRadius, float CapsuleHalfHeight,
	                           bool bDebugEnabled) const;
	bool TryTraversalAction(FTraversableCheckResult& OutTraversalData, EParkourActionType& OutParkourAction);

	UFUNCTION()
	void OnAnimInstanceMontageEndOrAbort(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnAnimInstanceMontageEndOrAbortForVault(UAnimMontage* Montage, bool bInterrupted);
	void Jump(const FInputActionValue& InputActionValue);
	void StrafeToggle(const FInputActionValue& InputActionValue);
	void Aim(const FInputActionValue& InputActionValue);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FSetInteractionTransformDelegate OnSetInteractionTransform;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FTraverseLookup OnTryTraverse;

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	//Not a uproperty.
	CoroStateMachine StateMachine;

	UPROPERTY()
	TObjectPtr<ACharacter> ControlledCharacter;
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
	UPROPERTY(EditAnywhere)
	FRuntimeFloatCurve StrafeMapSpeedCurve;
	UPROPERTY(EditAnywhere, Category="Parkour")
	FVector WalkSpeeds{200.0, 175.0, 150.0};
	UPROPERTY(EditAnywhere, Category="Parkour")
	FVector RunSpeeds{500.0, 350.0, 300.0};
	UPROPERTY(EditAnywhere, Category="Parkour")
	FVector SprintSpeeds{700.0, 700.0, 700.0};
	UPROPERTY(EditAnywhere, Category="Parkour")
	FVector CrouchSpeeds{200.0, 175.0, 150.0};

	UPROPERTY(EditAnywhere, Category="Parkour")
	FVector2f VelocityRange{0.0, 500.0};
	UPROPERTY(EditAnywhere, Category="Parkour")
	FVector2f TraceRange{75.0, 180.0};

	UPROPERTY(EditAnywhere, Category="Parkour")
	float AnalogWalkRunThreshold{0.7};
	UPROPERTY()
	EMovementGait CurrentDesiredGait{EMovementGait::Walk};

	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UChooserTable> TraversalAnimChooser;

	bool bWantsToStrafe{false};
	bool bWantsToSprint{false};
	bool bWantsToWalk{false};
	bool bWantsToAim{false};
	bool bWantsToJump{false};
	bool bCurrentlyTraversing{false};

	UPROPERTY(BlueprintReadOnly)
	FTransform InteractionTransform;

	/* INPUT */

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveWorldspaceAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookGamepadAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> WalkToggleAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> StrafeAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;
};
