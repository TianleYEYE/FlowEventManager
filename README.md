# Flow Event Manager

Runtime Unreal Engine plugin for managing timed Blueprint event flows in a level.

## Usage

1. Copy `FlowEventManager` into a project's `Plugins` folder, or keep it under `F:\UnrealProjects` and mount it from your project.
2. Enable the plugin and compile the project.
3. Add `FlowEventManagerComponent` to an Actor in the level.
4. Configure `InlineNodes`, or create a `Flow Event Sequence` asset from the Content Browser and assign it to `FlowAsset` with `bUseInlineNodes` disabled.
5. Each target Blueprint should expose a Custom Event or BlueprintCallable function named by `EventName`.
6. The event can accept no parameters, or one `float` duration parameter. The plugin passes `EventDuration` to that parameter.

## Visual Editor

- Double-click a `Flow Event Sequence` asset to open the visual node editor.
- Right-click the graph and choose `Add Flow Event Node` to add steps.
- Connect an output pin to an input pin to set the next node. The graph editor writes back to the asset's `Nodes` array.
- Existing assets that leave `NextNodeIndex` as `-1` keep the original implicit sequential behavior.

## Node Timing

- `Serial`: the next node starts after the current node's `EventDuration`.
- `Parallel`: the next node starts after `ParallelStartDelay`, while the current node continues until `EventDuration`.

Example: node 1 has `EventDuration = 5`, `NextMode = Parallel`, `ParallelStartDelay = 2`. Node 2 starts two seconds after node 1 starts, and node 1 still finishes at five seconds.
