using System.IO;

namespace UnrealBuildTool.Rules
{
	public class FlowEventManager : ModuleRules
	{
		public FlowEventManager(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			bUseUnity = false;

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine"
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				});

			PublicIncludePaths.Add(ModuleDirectory);
			PublicIncludePaths.AddRange(Directory.GetDirectories(Path.Combine(ModuleDirectory, "Public"), "*", SearchOption.AllDirectories));
			PrivateIncludePaths.AddRange(Directory.GetDirectories(Path.Combine(ModuleDirectory, "Private"), "*", SearchOption.AllDirectories));
		}
	}
}
