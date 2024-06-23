#include "Parkour/ParkourComponent.h"

#include "AnimationWarpingLibrary.h"
#include "Chooser.h"
#include "InputActionValue.h"
#include "KismetTraceUtils.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/EnhancedPlayerInputComponent.h"
#include "Logging/StructuredLog.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "Traversables/TraversableActor.h"


UParkourComponent::UParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

//@Z TODO:: This doesn't match the implementation of GetDesiredGait in BP right now.
void UParkourComponent::GetDesiredGait(const FVector StickMovementInput)
{
	if (StickMovementInput.Size2D() >= AnalogWalkRunThreshold)
	{
		CurrentDesiredGait = EMovementGait::Run;
	}
	else
	{
		CurrentDesiredGait = EMovementGait::Walk;
	}
}

void UParkourComponent::UpdateRotation(const bool WantsToStrafe)
{
	MovementComponent->bUseControllerDesiredRotation = WantsToStrafe;
	MovementComponent->bOrientRotationToMovement = !WantsToStrafe;
}

float CalculateAbsoluteDirectionFast(const FVector& CurrentVelocity, const FRotator& Rotation)
{
	if (CurrentVelocity.IsNearlyZero())
	{
		return 0.f;
	}

	const FVector NormalizedVel = CurrentVelocity.GetSafeNormal2D();
	const FVector ForwardVector = Rotation.Vector();
	const FVector RightVector = Rotation.RotateVector(FVector::RightVector);

	const float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVel);
	const float RightDot = FVector::DotProduct(RightVector, NormalizedVel);

	const float AngleRad = FMath::Atan2(RightDot, ForwardDot);
	return FMath::Abs(FMath::RadiansToDegrees(AngleRad));
}

float UParkourComponent::CalculateMaxSpeed() const
{
	const auto CurrentVelocity = MovementComponent->Velocity;
	const auto Rot = ControlledCharacter->GetActorRotation();
	const auto VelocityRelativeDirection = CalculateAbsoluteDirectionFast(CurrentVelocity, Rot);

	const auto DirectionScaledSpeed = StrafeMapSpeedCurve.ExternalCurve->GetFloatValue(VelocityRelativeDirection);

	FVector2d SpeedConstraints;
	switch (CurrentDesiredGait)
	{
	case EMovementGait::Run:
		SpeedConstraints = FVector2d{RunSpeeds.X, RunSpeeds.Y};
		break;
	case EMovementGait::Sprint:
		SpeedConstraints = FVector2d{SprintSpeeds.X, SprintSpeeds.Y};
		break;
	default: SpeedConstraints = FVector2d{WalkSpeeds.X, WalkSpeeds.Y};
	}

	const FVector2d SpeedRange{0.0f, 1.0f};
	return FMath::GetMappedRangeValueClamped(SpeedRange, SpeedConstraints, DirectionScaledSpeed);
}

void UParkourComponent::Move(const FInputActionValue& InputActionValue)
{
	const auto InputAxisVector = InputActionValue.Get<FVector2d>();
	const FRotator Rotation = ControlledCharacter->GetControlRotation();
	const FVector ForwardDirection = Rotation.Vector();
	const FVector RightDirection = Rotation.RotateVector(FVector::RightVector);

	ControlledCharacter->AddMovementInput(ForwardDirection, InputAxisVector.Y);
	ControlledCharacter->AddMovementInput(RightDirection, InputAxisVector.X);
}

void UParkourComponent::Look(const FInputActionValue& InputActionValue)
{
	const auto InputValue = InputActionValue.Get<FVector2d>();
	ControlledCharacter->AddControllerYawInput(InputValue.X);
	ControlledCharacter->AddControllerPitchInput(InputValue.Y);
}

void UParkourComponent::LookGamepad(const FInputActionValue& InputActionValue)
{
	const auto DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const auto InputValue = InputActionValue.Get<FVector2d>();
	ControlledCharacter->AddControllerYawInput(InputValue.X * DeltaSeconds);
	ControlledCharacter->AddControllerPitchInput(InputValue.Y * DeltaSeconds);
}

void UParkourComponent::WalkToggle(const FInputActionValue& InputActionValue)
{
	bWantsToWalk = (!bWantsToSprint) && (!bWantsToWalk);
}

void UParkourComponent::Sprint(const FInputActionValue& InputActionValue)
{
	bWantsToSprint = InputActionValue.Get<bool>();
	bWantsToWalk = false;
}

float UParkourComponent::GetForwardTraversalTraceDistance(const FVector& CurrentVelocity,
                                                          const FRotator& CurrentRotation) const
{
	const float ForwardVelocity = CurrentRotation.UnrotateVector(CurrentVelocity).X;
	return FMath::GetMappedRangeValueClamped(VelocityRange, TraceRange, ForwardVelocity);
}

bool ParkourTrace(
	FHitResult& OutHit,
	const UWorld* World,
	const FCollisionQueryParams& CapsuleTraceParams,
	const FCollisionShape& TraceCapsule,
	const FQuat& CapsuleRotation,
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const bool bDrawDebug,
	const FColor DebugColor,
	const float DebugDuration)
{
	World->SweepSingleByChannel(OutHit,
	                            TraceStart,
	                            TraceEnd,
	                            CapsuleRotation,
	                            ECC_Visibility,
	                            TraceCapsule,
	                            CapsuleTraceParams);

	if (bDrawDebug)
	{
		DrawDebugCapsuleTraceSingle(
			World,
			TraceStart,
			TraceEnd,
			TraceCapsule.GetCapsuleRadius(),
			TraceCapsule.GetCapsuleHalfHeight(),
			EDrawDebugTrace::ForDuration, OutHit.bBlockingHit, FHitResult{}, DebugColor, DebugColor, DebugDuration);
	}

	return OutHit.bBlockingHit;
}

bool GetAnimationDistanceFromFrontLedgeToTargetOrDeleteWarp(
	const UAnimMontage* Anim,
	UMotionWarpingComponent* const MotionWarpingComponent,
	const FName& WarpingTerminalZoneName,
	const FName& DistanceFromLedgeName, float& OutAnimationDistance)
{
	TArray<FMotionWarpingWindowData> BackLedgeMotionWarpingWindows;
	UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(
		Anim, WarpingTerminalZoneName, BackLedgeMotionWarpingWindows);
	if (!BackLedgeMotionWarpingWindows.IsEmpty())
	{
		float AnimationDistanceFromXtoY;
		UAnimationWarpingLibrary::GetCurveValueFromAnimation(Anim, DistanceFromLedgeName,
		                                                     BackLedgeMotionWarpingWindows[0].EndTime,
		                                                     AnimationDistanceFromXtoY);
		OutAnimationDistance = AnimationDistanceFromXtoY;
		return true;
	}

	MotionWarpingComponent->RemoveWarpTarget(WarpingTerminalZoneName);
	return false;
}

bool UParkourComponent::SelectParkourMontage(
	const EParkourActionType ActionType,
	const FTraversableCheckResult& TraversalCheck,
	UAnimMontage*& OutAnim,
	float& OutTime,
	float& OutPlayRate) const
{
	auto ChooserParams = FMovementChooserParams{
		CurrentDesiredGait,
		ActionType,
		TraversalCheck.ObstacleHeight,
		TraversalCheck.ObstacleDepth,
		ControlledCharacter->GetVelocity().Size2D()
	};
	OnTryTraverse.Broadcast(ChooserParams);

	FChooserEvaluationContext EvalCTX{};
	EvalCTX.AddStructParam(ChooserParams);

	TArray<UObject*> Results;
	UChooserTable::EvaluateChooser(EvalCTX, TraversalAnimChooser,
	                               FObjectChooserBase::FObjectChooserIteratorCallback::CreateLambda(
		                               [&Results](UObject* InResult)
		                               {
			                               if (InResult == nullptr)
			                               {
				                               return FObjectChooserBase::EIteratorStatus::Stop;
			                               }
			                               Results.Add(InResult);
			                               return FObjectChooserBase::EIteratorStatus::Continue;
		                               }));

	const auto AnimInstance = ControlledCharacter->GetMesh()->GetAnimInstance();
	FPoseSearchBlueprintResult PoseSearchResult;
	UPoseSearchLibrary::MotionMatch(AnimInstance, Results, FName(TEXT("PoseHistory")), FPoseSearchFutureProperties{},
	                                PoseSearchResult, 69420);

	OutAnim = const_cast<UAnimMontage*>(Cast<UAnimMontage>(PoseSearchResult.SelectedAnimation.Get()));
	if (!OutAnim) return false;

	OutTime = PoseSearchResult.SelectedTime;
	OutPlayRate = PoseSearchResult.WantedPlayRate;

	return true;
}

void UParkourComponent::UpdateMotionWarping(
	const UAnimMontage* Anim,
	const FTraversableCheckResult& TraversalCheck,
	const EParkourActionType ActionType) const
{
	static const auto FrontLedgeName = FName(TEXT("FrontLedge"));
	static const auto BackLedgeName = FName(TEXT("BackLedge"));
	static const auto FloorName = FName(TEXT("BackFloor"));
	static const auto DistanceFromLedgeName = FName(TEXT("Distance_From_Ledge"));

	const auto MotionWarpingComponent = ControlledCharacter->GetComponentByClass<UMotionWarpingComponent>();

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
		FrontLedgeName, TraversalCheck.FrontLedgeLocation,
		(-TraversalCheck.FrontLedgeNormal).ToOrientationRotator());

	float AnimationWarpDistanceFromFrontLedgeToBackLedge{0.0};
	if (not(ActionType == EParkourActionType::Hurdle || ActionType == EParkourActionType::Mantle))
	{
		MotionWarpingComponent->RemoveWarpTarget(BackLedgeName);
	}
	else
	{
		const bool bBackLedgeHadWarpTargets = GetAnimationDistanceFromFrontLedgeToTargetOrDeleteWarp(
			Anim, MotionWarpingComponent, BackLedgeName, DistanceFromLedgeName,
			AnimationWarpDistanceFromFrontLedgeToBackLedge);
		if (bBackLedgeHadWarpTargets)
		{
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
				BackLedgeName, TraversalCheck.BackLedgeLocation, FRotator::ZeroRotator);
		}
	}

	if (ActionType == EParkourActionType::Hurdle)
	{
		float AnimationWarpDistanceFromFrontLedgeToFloor{0.0};
		const bool bFloorHadWarpTargets = GetAnimationDistanceFromFrontLedgeToTargetOrDeleteWarp(
			Anim, MotionWarpingComponent, FloorName, DistanceFromLedgeName,
			AnimationWarpDistanceFromFrontLedgeToFloor);

		if (bFloorHadWarpTargets)
		{
			const auto AbsDistanceBackLedgeToFloor = FMath::Abs(
				AnimationWarpDistanceFromFrontLedgeToBackLedge - AnimationWarpDistanceFromFrontLedgeToFloor);

			const auto BackLedgePlane = TraversalCheck.BackLedgeLocation + (TraversalCheck.BackLedgeNormal *
				AbsDistanceBackLedgeToFloor);
			const FVector MotionWarpingFloorTarget{
				BackLedgePlane.X, BackLedgePlane.Y,
				TraversalCheck.BackFloorLocation.Z
			};

			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
				FloorName, MotionWarpingFloorTarget, FRotator::ZeroRotator);
		}
	}
	else
	{
		MotionWarpingComponent->RemoveWarpTarget(FloorName);
	}
}

bool UParkourComponent::DetermineParkourAction(
	const FTraversableCheckResult& TraversalCheck,
	const bool bDebugEnabled,
	EParkourActionType& OutParkourActionType)
{
	const bool bObstacleHeightInRangeVaultHurdle = TraversalCheck.ObstacleHeight >= 50.0f && TraversalCheck.
		ObstacleHeight <= 125.0f;
	const bool bObstacleSmallerThanPlayer = TraversalCheck.ObstacleDepth < 59.0f;
	const bool bObstacleHeightInRangeMantle = TraversalCheck.ObstacleHeight >= 50.0f && TraversalCheck.ObstacleHeight <=
		275.0f;

	const bool bIsVault = TraversalCheck.bHasFrontLedge &&
		TraversalCheck.bHasBackLedge &&
		(!TraversalCheck.bHasBackFloor) &&
		bObstacleHeightInRangeVaultHurdle &&
		bObstacleSmallerThanPlayer;

	const bool bIsHurdle = TraversalCheck.bHasFrontLedge &&
		TraversalCheck.bHasBackLedge &&
		TraversalCheck.bHasBackFloor &&
		bObstacleHeightInRangeVaultHurdle &&
		bObstacleSmallerThanPlayer &&
		TraversalCheck.BackLedgeHeight > 50.0f;

	const bool bIsMantle = TraversalCheck.bHasFrontLedge &&
		bObstacleHeightInRangeMantle &&
		TraversalCheck.ObstacleDepth > 59.0f;

	if (bDebugEnabled)
	{
		UE_LOGFMT(LogTemp, Warning,
		          "Front: {1} -- Back: {2} -- BackFloor: {3} -- In Range: {4} -- Smaller Obstacle: {5} -- Mantleable: {6}",
		          TraversalCheck.bHasFrontLedge,
		          TraversalCheck.bHasBackLedge,
		          TraversalCheck.bHasBackFloor,
		          bObstacleHeightInRangeVaultHurdle,
		          bObstacleSmallerThanPlayer,
		          bObstacleHeightInRangeMantle);
	}

	OutParkourActionType = bIsVault
		                       ? EParkourActionType::Vault
		                       : bIsHurdle
		                       ? EParkourActionType::Hurdle
		                       : EParkourActionType::Mantle;

	return bIsMantle || bIsHurdle || bIsVault;
}

bool UParkourComponent::PerformTraversalCheck(FTraversableCheckResult& OutTraversalCheck, float CapsuleRadius,
                                              float CapsuleHalfHeight, bool bDebugEnabled) const
{
	const float ForwardTraceDistance = GetForwardTraversalTraceDistance(
		ControlledCharacter->GetVelocity(),
		ControlledCharacter->GetActorRotation());

	const auto ActorLocation = ControlledCharacter->GetActorLocation();
	const auto ActorForward = ControlledCharacter->GetActorForwardVector();
	const auto CapsuleRotation = ControlledCharacter->GetCapsuleComponent()->GetComponentRotation();

	FHitResult HitResult;
	const auto InitialTraceEnd = ActorLocation + ActorForward * ForwardTraceDistance;

	static const FName CapsuleTraceSingleName(TEXT("CapsuleTraceSingleByProfile"));
	const auto CapsuleTraceParams = FCollisionQueryParams{CapsuleTraceSingleName, false, ControlledCharacter};
	const auto TraceCapsule = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

	// Initial trace
	if (!ParkourTrace(HitResult, GetWorld(), CapsuleTraceParams, TraceCapsule, CapsuleRotation.Quaternion(),
	                  ActorLocation, InitialTraceEnd, bDebugEnabled, FColor::Green, 5.0f))
	{
		return false;
	}

	const auto HitTraversable = Cast<ATraversableActor>(HitResult.GetActor());
	if (!HitTraversable) return false;

	OutTraversalCheck = HitTraversable->GetLedgeTransforms(HitResult.ImpactPoint, ActorLocation);
	OutTraversalCheck.HitComponent = HitResult.Component.Get();

	if (!OutTraversalCheck.bHasFrontLedge) return false;

	// Front ledge room check
	const auto FrontLedgeRoomCheck = OutTraversalCheck.FrontLedgeLocation +
		OutTraversalCheck.FrontLedgeNormal * (CapsuleRadius + 2.0f) +
		FVector{0.0f, 0.0f, CapsuleHalfHeight + 2.0f};

	if (ParkourTrace(HitResult, GetWorld(), CapsuleTraceParams, TraceCapsule, CapsuleRotation.Quaternion(),
	                 ActorLocation, FrontLedgeRoomCheck, bDebugEnabled, FColor::Red, 5.0f))
	{
		return false;
	}

	OutTraversalCheck.ObstacleHeight = FMath::Abs(
		(ActorLocation - CapsuleHalfHeight - OutTraversalCheck.FrontLedgeLocation).Z);

	// Back ledge room check
	const auto BackLedgeRoomCheck = OutTraversalCheck.BackLedgeLocation +
		OutTraversalCheck.BackLedgeNormal * (CapsuleRadius + 2.0f) +
		FVector{0.0f, 0.0f, CapsuleHalfHeight + 2.0f};

	ParkourTrace(HitResult, GetWorld(), CapsuleTraceParams, TraceCapsule, CapsuleRotation.Quaternion(),
	             FrontLedgeRoomCheck, BackLedgeRoomCheck, bDebugEnabled, FColor::Yellow, 5.0f);

	if (HitResult.bBlockingHit)
	{
		OutTraversalCheck.ObstacleDepth = (HitResult.ImpactPoint - OutTraversalCheck.FrontLedgeLocation).Size2D();
		OutTraversalCheck.bHasBackLedge = false;
	}
	else
	{
		OutTraversalCheck.ObstacleDepth = (OutTraversalCheck.FrontLedgeLocation - OutTraversalCheck.BackLedgeLocation).
			Size2D();

		const auto FloorCheck = (OutTraversalCheck.BackLedgeLocation +
				OutTraversalCheck.BackLedgeNormal * (CapsuleRadius + 2.0f)) -
			FVector{0.0f, 0.0f, (OutTraversalCheck.ObstacleHeight - CapsuleHalfHeight) + 50.0f};

		ParkourTrace(HitResult, GetWorld(), CapsuleTraceParams, TraceCapsule, CapsuleRotation.Quaternion(),
		             BackLedgeRoomCheck, FloorCheck, bDebugEnabled, FColor::Purple, 5.0f);

		if (HitResult.bBlockingHit)
		{
			OutTraversalCheck.BackFloorLocation = HitResult.ImpactPoint;
			OutTraversalCheck.BackLedgeHeight = FMath::Abs(
				(HitResult.ImpactPoint - OutTraversalCheck.BackLedgeLocation).Z);
			OutTraversalCheck.bHasBackFloor = true;
		}
	}

	return true;
}

bool UParkourComponent::TryTraversalAction(FTraversableCheckResult& OutTraversalData,
                                           EParkourActionType& OutParkourAction)
{
	constexpr bool bDebugEnabled{true};

	const auto CapsuleRadius = ControlledCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const auto CapsuleHalfHeight = ControlledCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FTraversableCheckResult TraversalCheck;
	if (!PerformTraversalCheck(TraversalCheck, CapsuleRadius, CapsuleHalfHeight, bDebugEnabled)) return false;

	EParkourActionType ActionType;
	if (!DetermineParkourAction(TraversalCheck, bDebugEnabled, ActionType)) return false;

	//This seems to actually just be a problem, idk why it exists.
	//ControlledCharacter->GetCapsuleComponent()->IgnoreComponentWhenMoving(TraversalCheck.HitComponent, true);
	OnSetInteractionTransform.Broadcast(FTransform(TraversalCheck.FrontLedgeNormal.Rotation().Quaternion(),
	                                               TraversalCheck.FrontLedgeLocation));

	UE_LOGFMT(LogTemp, Warning, "Action Type: {0}",
	          [](const EParkourActionType Type) -> FString {
	          switch(Type) {
	          case EParkourActionType::Hurdle: return TEXT("Hurdle");
	          case EParkourActionType::Vault: return TEXT("Vault");
	          case EParkourActionType::Mantle: return TEXT("Mantle");
	          default: return TEXT("Unknown");
	          }
	          }(ActionType)
	);

	UAnimMontage* Anim;
	float Time;
	float PlayRate;
	if (!SelectParkourMontage(ActionType, TraversalCheck, Anim, Time, PlayRate))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find montage."))
		return false;
	}

	UpdateMotionWarping(Anim, TraversalCheck, ActionType);

	const auto AnimInstance = ControlledCharacter->GetMesh()->GetAnimInstance();
	MovementComponent->SetMovementMode(MOVE_Flying);
	
	AnimInstance->Montage_Play(Anim, PlayRate, EMontagePlayReturnType::MontageLength, Time);
	
	bCurrentlyTraversing = true;
	if (ActionType == EParkourActionType::Vault)
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &UParkourComponent::OnAnimInstanceMontageEndOrAbortForVault);
	}
	else if (ActionType == EParkourActionType::Hurdle)
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &UParkourComponent::OnAnimInstanceMontageEndOrAbort);
	}
	else
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &UParkourComponent::OnAnimInstanceMontageEndOrAbort);

		//The blend time for climbing is kind of long, this feels better.
		if (TraversalCheck.ObstacleHeight > 150.0)
		{
			AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UParkourComponent::OnAnimInstanceMontageEndOrAbort);
		}
	}

	OutTraversalData = TraversalCheck;
	OutParkourAction = ActionType;
	return true;
}

void UParkourComponent::OnAnimInstanceMontageEndOrAbort(UAnimMontage* Montage, bool bInterrupted)
{
	const auto AnimInstance = ControlledCharacter->GetMesh()->GetAnimInstance();
	MovementComponent->SetMovementMode(MOVE_Walking);
	AnimInstance->OnMontageBlendingOut.Clear();
	AnimInstance->OnMontageEnded.Clear();
	bCurrentlyTraversing = false;
}

void UParkourComponent::OnAnimInstanceMontageEndOrAbortForVault(UAnimMontage* Montage, bool bInterrupted)
{
	const auto AnimInstance = ControlledCharacter->GetMesh()->GetAnimInstance();
	MovementComponent->SetMovementMode(MOVE_Falling);
	AnimInstance->OnMontageEnded.Clear();
	bCurrentlyTraversing = false;
}

void UParkourComponent::Jump(const FInputActionValue& InputActionValue)
{
	if (bCurrentlyTraversing) return;

	//If you want the character to only be able to parkour while grounded.
	//if(!MovementComponent->IsMovingOnGround()) return;
	bWantsToJump = true;
}

void UParkourComponent::StrafeToggle(const FInputActionValue& InputActionValue)
{
	bWantsToStrafe = !bWantsToStrafe;
}

void UParkourComponent::Aim(const FInputActionValue& InputActionValue)
{
	bWantsToAim = InputActionValue.Get<bool>();
	bWantsToStrafe |= bWantsToAim;
}

CoroState UParkourComponent::ParkourStateMachine()
{
	while (true)
	{
		co_await std::suspend_always{};
		const auto MaxSpeed = CalculateMaxSpeed();
		MovementComponent->MaxWalkSpeed = MaxSpeed;
		UpdateRotation(!bWantsToStrafe);

		if (bWantsToJump)
		{
			FTraversableCheckResult TraversalCheck;
			EParkourActionType ActionType;
			bWantsToJump = false;
			if (!TryTraversalAction(TraversalCheck, ActionType))
			{
				ControlledCharacter->Jump();
			} //...
		}
	}
}

void UParkourComponent::BeginPlay()
{
	Super::BeginPlay();

	ControlledCharacter = GetOwner<ACharacter>();
	MovementComponent = Cast<UCharacterMovementComponent>(ControlledCharacter->GetMovementComponent());

	check(ControlledCharacter);
	check(MovementComponent);
	StateMachine.ChangeToState(ParkourStateMachine());

	const auto EnhancedInputComponent = CastChecked<UEnhancedPlayerInputComponent>(ControlledCharacter->InputComponent);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInputComponent->BindAction(LookGamepadAction, ETriggerEvent::Triggered, this, &ThisClass::LookGamepad);
	EnhancedInputComponent->BindAction(WalkToggleAction, ETriggerEvent::Triggered, this, &ThisClass::WalkToggle);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::Sprint);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Triggered, this, &ThisClass::StrafeToggle);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::Aim);
}

void UParkourComponent::BeginDestroy()
{
	StateMachine.Destroy();
	Super::BeginDestroy();
}

void UParkourComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	StateMachine.Run();
}
