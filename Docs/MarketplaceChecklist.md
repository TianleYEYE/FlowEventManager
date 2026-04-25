# Marketplace Readiness Checklist

## Included

- Runtime flow execution component.
- Visual editor for `Flow Event Sequence` assets.
- Asset validation API and editor validation panel.
- Runtime state query helpers for debugging.
- UE 5.3, 5.4, 5.5, 5.6, and 5.7 BuildPlugin validation on Win64.
- Plugin icon at `Resources/Icon128.png`.
- Generated Unreal folders excluded from source control.

## Recommended Before Public Release

- Add a sample project or sample content pack.
- Add screenshots or a short demo video.
- Add automated tests for validation and runtime execution.
- Verify any additional target platforms before adding them to `PlatformAllowList`.
- Maintain release notes for each published version.

## Build Notes

UE 5.3 and 5.4 may fail before plugin compilation if UnrealBuildTool selects a newer MSVC toolchain than the engine expects. Validate those versions with a compatible Visual Studio 2022 toolchain if you see errors in Unreal Engine shared PCH files before any plugin source file is compiled.
