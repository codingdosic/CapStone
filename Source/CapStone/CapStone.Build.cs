// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CapStone : ModuleRules
{
	public CapStone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "WebSockets", "Json", "JsonUtilities", "EnhancedInput", "Networking", "Sockets", "UMG", "Slate", "SlateCore" });
    }
}
