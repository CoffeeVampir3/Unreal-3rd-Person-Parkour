// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class GameAnimationSampleTarget : TargetRules
{
	public GameAnimationSampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		CppStandardEngine = CppStandardVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange( new string[] { "GameAnimationSample" } );
	}
}
