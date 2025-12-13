using UnrealBuildTool;

public class TrafficEditor : ModuleRules
{
	public TrafficEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"LevelEditor",
			"ToolMenus",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"EditorSubsystem",
			"PropertyEditor",
			"InputCore",
			"TrafficRuntime",
			"ZoneGraph"
		});
	}
}
