using UnrealBuildTool;
using System.Collections.Generic;

public class CardGameEditorTarget : TargetRules
{
	public CardGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CardGame");
	}
}
