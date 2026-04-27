# Flow Event Manager Quick Start

## Install

1. Place the plugin at `Plugins/FlowEventManager` inside an Unreal Engine project.
2. Regenerate project files if required by your workflow.
3. Compile the project.
4. Enable the plugin in Unreal Editor.

## Create a Flow

1. Create a `Flow Event Sequence` asset in the Content Browser.
2. Double-click the asset to open the visual flow editor.
3. Right-click the graph and add flow event nodes.
4. Connect output pins to input pins to define execution order.
5. Select a node and configure target selection, event name, duration, and timing mode.
6. Click `Validate Flow` before saving.
7. Use `Fix Node IDs` if validation reports empty or duplicate ids.
8. Use `Auto Arrange` to clean up graph layout.

## Run a Flow

1. Add `FlowEventManagerComponent` to an Actor.
2. Assign the `Flow Event Sequence` asset to `FlowAsset`.
3. Disable `bUseInlineNodes` if using an asset.
4. Call `StartFlow` or enable `bAutoStart`.

Before starting a flow, call `ValidateConfiguredFlow` if you want to block invalid setup in Blueprint or editor utility scripts.

## Target Events

Each target Actor should expose a Blueprint event or callable function named by the node's `EventName`.

Supported signatures:

```cpp
void EventName();
void EventName(float Value);
void EventName(float Value, float ElapsedTime);
```

For normal timed nodes, `Value` is the node duration. For timeline nodes, `Value` is the evaluated curve value. When present, `ElapsedTime` is the node's elapsed execution time in seconds.

## Validate Before Runtime

Use validation before shipping a map or starting a flow:

- In the visual editor, click `Validate Flow`.
- In Blueprint, call `ValidateFlow` on a `Flow Event Sequence` asset.
- In Blueprint, call `ValidateConfiguredFlow` on `FlowEventManagerComponent`.

Validation reports missing targets, empty event names, duplicate node ids, invalid links, unreachable nodes, and cycles.
