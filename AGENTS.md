# Audacity Alicorn GUI Migration

## Goal and boundary

Incrementally replace Audacity 3's wxWidgets-authored application UI with Alicorn while preserving Audacity's editing behavior, accessibility, platform integration, and performance. The target is an Alicorn application UI, not a long-term collection of Alicorn-painted wxWidgets controls.

Keep domain state, commands, audio processing, and project policy in Audacity's application/backend layers (including the Odin/Caliber work). Alicorn owns reusable UI description, layout, interaction, semantics, motion, and paint. wxWidgets may remain as a native host/platform adapter while migration proceeds.

## Current state

As of Audacity `main` commit `74a35c7`, an opt-in `AUDACITY_USE_ALICORN_SNAP_UI` path hosts Alicorn-rendered controls for Snap, Audio Setup, Selection Toolbar setup, Edit, and Cut/Copy/Paste. Most of Audacity's UI—including the waveform/track editor, menus, dialogs, and the surrounding workbench—remains wxWidgets. Do not describe the application as migrated or at GUI parity.

The current control adapter creates one retained Alicorn runtime and loads default fonts per hosted control. This is acceptable only as an early prototype; account for this cost before expanding to many more controls. Alicorn issue [#196](https://github.com/samanshaiza004/alicorn/issues/196) is the existing host-integration discussion; check it before proposing duplicate work.

## Working and coordination rules

- Read the relevant source and current GitHub `main` state before editing; conversation summaries are not authoritative.
- Preserve existing user and agent changes. Do not reset, overwrite, or reformat unrelated work.
- Give each parallel agent a disjoint file/surface scope. Before touching shared adapter or build files, tell the orchestrator what is needed and coordinate ownership.
- Notify the orchestrator promptly about verified defects, ABI/build-pin mismatches, blockers, scope changes, and findings that affect another agent's work. At completion, report changed files, evidence, and remaining limitations.
- The orchestrator owns integration and publication to GitHub `main` unless a separate end-to-end commit is explicitly assigned. Check open Alicorn issues before filing; create only concrete, evidence-backed, non-duplicate issues relevant to this migration.
- Keep each change a behavior-preserving, reviewable vertical slice. Prefer existing Alicorn/Audacity dependencies and public APIs; avoid private runtime access and parallel UI/domain frameworks.
- Do not add or run tests unless explicitly requested. Build/compile checks are appropriate when validating an implementation slice.

## Build and pin cautions

The Alicorn UI path is opt-in. Configure with `-Daudacity_use_alicorn_snap_ui=ON` (and the existing Odin/Caliber option when the build requires it), then build the `Audacity` target. Preserve the revision in `cmake-proxies/cmake-modules/AlicornSnapUIPin.cmake` for committed changes. A local Alicorn checkout may be at a different revision; if temporarily changing the pin for a local build, restore it before committing and report which Alicorn revision was actually compiled.

Check `git remote -v` before publishing. In some migration worktrees, `origin` points to a local checkout rather than GitHub; do not assume a local push updates `samanshaiza004/audacity`.
