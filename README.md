# Flow Event Manager

Flow Event Manager is an Unreal Engine plugin for authoring and running timed Blueprint event flows. It includes a runtime component for executing serial and parallel event sequences, plus an editor-only visual node editor for `Flow Event Sequence` assets.

## Compatibility

This plugin is maintained as a single multi-version branch for Unreal Engine 5.x.

| Unreal Engine | Status |
| --- | --- |
| 5.3 | Supported |
| 5.4 | Supported |
| 5.5 | Supported |
| 5.6 | Supported |
| 5.7 | Supported |

The `.uplugin` file intentionally does not pin `EngineVersion`, so the same checkout can be used across supported UE 5.x projects. If Epic changes editor APIs in a future engine release, compatibility should be handled with version guards in the editor module instead of splitting the runtime module.

## Installation

1. Copy or clone this repository into your Unreal project under `Plugins/FlowEventManager`.
2. Regenerate project files if your workflow requires it.
3. Build the project from your IDE or from UnrealBuildTool.
4. Enable the plugin in Unreal Editor if it is not enabled automatically.

## Usage

1. Add `FlowEventManagerComponent` to an Actor in the level.
2. Configure `InlineNodes`, or create a `Flow Event Sequence` asset from the Content Browser and assign it to `FlowAsset` with `bUseInlineNodes` disabled.
3. Each target Blueprint should expose a Custom Event or BlueprintCallable function named by `EventName`.
4. The event can accept no parameters, or one `float` duration parameter. The plugin passes `EventDuration` to that parameter.

## Visual Editor

- Double-click a `Flow Event Sequence` asset to open the visual node editor.
- Right-click the graph and choose `Add Flow Event Node` to add steps.
- Connect an output pin to an input pin to set the next node. The graph editor writes back to the asset's `Nodes` array.
- Use `Validate Flow` in the details panel to find missing targets, duplicate node ids, invalid links, unreachable nodes, and cycles before runtime.
- Use `Fix Node IDs` to assign unique ids to empty or duplicate nodes.
- Use `Auto Arrange` to quickly lay out nodes from left to right.
- Existing assets that leave `NextNodeIndex` as `-1` keep the original implicit sequential behavior.

## Node Timing

- `Serial`: the next node starts after the current node's `EventDuration`.
- `Parallel`: the next node starts after `ParallelStartDelay`, while the current node continues until `EventDuration`.

Example: node 1 has `EventDuration = 5`, `NextMode = Parallel`, `ParallelStartDelay = 2`. Node 2 starts two seconds after node 1 starts, and node 1 still finishes at five seconds.

## Runtime Debugging

`FlowEventManagerComponent` exposes Blueprint-callable runtime state helpers:

- `GetActiveNodeCount`
- `GetActiveNodeIndices`
- `GetActiveNodeIds`
- `GetActiveNodeElapsedTime`
- `GetActiveNodeProgress`
- `GetPendingStartCount`

Use these functions to build in-game debug widgets, editor utility tools, or automated tests around active flow execution.

## Validation

`Flow Event Sequence` assets expose `ValidateFlow`, which returns structured validation issues with severity, node index, node id, and message. The editor calls the same API, so validation behavior is consistent between tools and runtime/editor scripting.

For inline component flows, call `ValidateConfiguredFlow` on `FlowEventManagerComponent`. It validates either `InlineNodes` or the assigned `FlowAsset`, depending on the component configuration.

## Development

Generated Unreal folders are intentionally excluded from Git:

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`

Before publishing a release, validate the plugin against each supported engine version. A typical packaged-plugin check is:

```powershell
<UnrealEngineRoot>\Engine\Build\BatchFiles\RunUAT.bat BuildPlugin `
  -Plugin="<ProjectRoot>\Plugins\FlowEventManager\FlowEventManager.uplugin" `
  -Package="<OutputRoot>\FlowEventManager_UE<Version>" `
  -TargetPlatforms=Win64
```

Use paths that match your local Unreal Engine installation and project layout.

Release notes are maintained in `CHANGELOG.md`.

The plugin icon is stored at `Resources/Icon128.png`.
