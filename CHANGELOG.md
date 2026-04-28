# Changelog

## Unreleased

- Added delay nodes that wait for their duration without requiring a target event.
- Added multi-output parallel branches and branch-aware auto arrangement in the visual editor.
- Made generated next-node indices read-only so graph links are the source of truth.
- Added optional two-parameter event signatures so timeline nodes can output both the evaluated curve value and elapsed execution time.

## 1.1.0

- Added structured flow validation for assets and inline component nodes.
- Added editor validation panel with `Validate Flow`, `Fix Node IDs`, and `Auto Arrange`.
- Added runtime state query helpers for active nodes, progress, elapsed time, and pending starts.
- Added plugin icon at `Resources/Icon128.png`.
- Added quick start and marketplace readiness documentation.
- Updated plugin metadata for public distribution.
- Validated packaged plugin builds from UE 5.3 through UE 5.7 on Win64.

## 1.0.0

- Added runtime flow execution component.
- Added `Flow Event Sequence` asset.
- Added visual node editor for flow assets.
- Added serial and parallel timing modes.
- Added timeline curve support for repeated output values during node execution.
