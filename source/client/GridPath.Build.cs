// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GridPath : ModuleRules
{
	public GridPath(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        { "Core", "CoreUObject", "Engine", "InputCore",
                    "NavigationSystem", "AIModule", "Niagara", "EnhancedInput",
                    "UMG", "Slate", "SlateCore" });

        // HTTP 모듈 추가
        PublicDependencyModuleNames.Add("HTTP");

        // JSON 관련 모듈 추가
        PublicDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
