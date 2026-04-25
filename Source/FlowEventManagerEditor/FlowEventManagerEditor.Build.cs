using System.IO;

namespace UnrealBuildTool.Rules
{
	public class FlowEventManagerEditor : ModuleRules
	{
		public FlowEventManagerEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			bUseUnity = false;

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"FlowEventManager"
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"AppFramework",
					"AssetTools",
					"EditorFramework",
					"GraphEditor",
					"InputCore",
					"PropertyEditor",
					"Slate",
					"SlateCore",
					"ToolMenus",
					"UnrealEd"
				});

			PublicIncludePaths.Add(ModuleDirectory);
			PublicIncludePaths.AddRange(Directory.GetDirectories(Path.Combine(ModuleDirectory, "Public"), "*", SearchOption.AllDirectories));
			PrivateIncludePaths.AddRange(Directory.GetDirectories(Path.Combine(ModuleDirectory, "Private"), "*", SearchOption.AllDirectories));
		}
	}
}
