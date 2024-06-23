// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/GameInputConfiguration.h"

bool UGameInputConfiguration::TryGetInputActionForTag(const FGameplayTag& InputTag, UInputAction*& OutInputAction) const
{
	for (const auto [InputAction, AbilityInputTag] : AbilityInputActions)
	{
		if (InputAction && InputTag.MatchesTagExact(AbilityInputTag))
		{
			OutInputAction = InputAction;
			return true;
		}
	}
	return false;
}
