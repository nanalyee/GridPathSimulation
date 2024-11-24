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

        // HTTP ��� �߰�
        PublicDependencyModuleNames.Add("HTTP");

        // JSON ���� ��� �߰�
        PublicDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
