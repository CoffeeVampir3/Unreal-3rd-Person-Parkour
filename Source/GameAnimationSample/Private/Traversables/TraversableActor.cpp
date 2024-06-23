// Fill out your copyright notice in the Description page of Project Settings.


#include "Traversables/TraversableActor.h"

USplineComponent* ATraversableActor::FindClosestLedgeToLocation(const FVector& Location)
{
	
	float CurrentDistance = 99999.f;
	USplineComponent* ClosestSpline {nullptr};

	for(const auto SplineItem : LedgeSplines)
	{
		FVector ClosestOnThisSpline = SplineItem->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
		ClosestOnThisSpline += SplineItem->FindUpVectorClosestToWorldLocation(Location, ESplineCoordinateSpace::World) * 10.0f;
		const float DistToSpline = FVector::Distance(ClosestOnThisSpline, Location);
		if(CurrentDistance > DistToSpline)
		{
			ClosestSpline = SplineItem;
			CurrentDistance = DistToSpline;
		}
	}
	return ClosestSpline;
}

FTraversableCheckResult ATraversableActor::GetLedgeTransforms(const FVector HitLocation, const FVector ActorLocation)
{
	const USplineComponent* ClosestLedge = FindClosestLedgeToLocation(ActorLocation);
	constexpr float MinLedgeWidth = 60.0f;
	FTraversableCheckResult CheckResult {};

	const auto SplineLength = ClosestLedge->GetSplineLength();
	if(!ClosestLedge || SplineLength < MinLedgeWidth) return CheckResult;

	const auto ClosestLocation = ClosestLedge->FindLocationClosestToWorldLocation(HitLocation, ESplineCoordinateSpace::Local);
	const auto DistanceAlongClosest = ClosestLedge->GetDistanceAlongSplineAtLocation(ClosestLocation, ESplineCoordinateSpace::Local);

	const auto TransitivePoint = FMath::Clamp(DistanceAlongClosest, MinLedgeWidth / 2.0f, SplineLength - (MinLedgeWidth / 2.0f));

	const auto FrontLedgeCheck = ClosestLedge->GetTransformAtDistanceAlongSpline(TransitivePoint, ESplineCoordinateSpace::World);

	CheckResult.bHasFrontLedge = true;
	CheckResult.FrontLedgeLocation = FrontLedgeCheck.GetLocation();
	CheckResult.FrontLedgeNormal = FrontLedgeCheck.Rotator().Quaternion().GetUpVector();

	const auto OppositeLedge = OppositeLedges[ClosestLedge];
	if(!OppositeLedge) return CheckResult;

	const auto BackLedgeCheck = OppositeLedge->FindTransformClosestToWorldLocation(FrontLedgeCheck.GetLocation(), ESplineCoordinateSpace::World);
	CheckResult.bHasBackLedge = true;
	CheckResult.BackLedgeLocation = BackLedgeCheck.GetLocation();
	CheckResult.BackLedgeNormal = BackLedgeCheck.Rotator().Quaternion().GetUpVector();

	return CheckResult;
}
