// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class ProjectRL : ModuleRules
{
	public ProjectRL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Sockets", "Networking", "OnlineSubsystemUtils",
        "Json", "JsonUtilities"});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "onnxruntime", "include"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "onnxruntime", "lib", "onnxruntime.lib"));
        RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "onnxruntime", "lib", "onnxruntime.dll"));
    }
}
