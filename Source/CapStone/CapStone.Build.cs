// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CapStone : ModuleRules
{
	public CapStone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "WebSockets", "Json", "JsonUtilities", "UMG" });

        PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Add the new files here
        PublicIncludePaths.AddRange(new string[] { "CapStone" });
    }
}
