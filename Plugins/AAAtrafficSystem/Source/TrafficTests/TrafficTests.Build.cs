using UnrealBuildTool;

public class TrafficTests : ModuleRules
{
	public TrafficTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TrafficRuntime",
			"ZoneGraph",
			"UnrealEd",
			"TrafficEditor", // REQUIRED
			"AutomationController"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"FunctionalTesting",
			"DeveloperSettings",
			"Engine",
			"GeometryFramework"
		});
	}
}
