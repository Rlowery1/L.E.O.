using UnrealBuildTool;

public class TrafficRuntime : ModuleRules
{
	public TrafficRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"ZoneGraph"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"MeshDescription",
			"StaticMeshDescription",
			"AssetRegistry",
			"GeometryFramework"
		});
	}
}
