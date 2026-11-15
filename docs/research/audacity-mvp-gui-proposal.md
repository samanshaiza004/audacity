# Audacity 3 Alicorn GUI MVP

**Status:** Proposal; no implementation changes are included in this document.

**Target:** Audacity 3.7.9, the newest Audacity 3 release listed by the official release page at the time of research. Audacity 4 is a separate product direction; this proposal targets the Audacity 3 behavior and project format. See the [Audacity releases](https://github.com/audacity/audacity/releases) and [3.7.9 notes](https://github.com/audacity/audacity/releases/tag/Audacity-3.7.9).

## Recommendation

Build an **editing-first, single-track MVP** as an experimental Alicorn workspace inside the Audacity 3 application. Make one end-to-end audio-editing loop reliable before migrating the full multi-track workbench:

> Open one supported audio file → view its waveform on a timeline → select a time range → play/stop → delete the selected range → undo/redo → save and reopen the project → export a WAV.

The MVP supports one mono or stereo audio track. Basic edits may split that track into clip segments and gaps, but the user cannot move, resize, or mix clips. There is no recording, multitrack mixing, effects, or plugin management in this first version.

This is deliberately smaller than Audacity's defining multitrack workflow. That tradeoff is acceptable for a first Alicorn editor because it isolates the hardest new GUI surface—waveform rendering, time selection, transport feedback, and safe project persistence—without first rebuilding every TrackPanel mode. Keep the complete Audacity 3 workspace available as a fallback until the MVP has passed its acceptance gates.

## Product scope

### Included

- A single project window and one audio track, mono or stereo.
- Open/import one audio file through Audacity's existing importers. The MVP does not implement codecs; the importer remains the format authority.
- A waveform overview, seconds ruler, horizontal scroll and zoom, playhead, and visible time selection.
- Pointer drag selection with sample-accurate endpoints, plus keyboard-accessible selection controls and readable start/end values.
- Play, play selection, and stop through Audacity's existing audio engine, using the current default output device.
- Delete the selected range using Audacity's existing edit semantics; undo and redo through the project history.
- Save/reopen as `.aup3` and export the complete one-track project as WAV.
- Basic keyboard operation, focus visibility, and semantic names/values for transport, selection, zoom, and file actions.
- Existing native file dialogs and platform services may remain hosted by wxWidgets during this migration slice.

### Deferred until after the MVP

- Multiple tracks, mixing, mute/solo, gain/pan, sync lock, and track grouping.
- Recording, input-device selection, monitoring, punch-and-roll, and latency controls.
- Direct clip movement/resizing, fades, envelope automation, clipboard cut/copy/paste, and cross-track operations.
- Effects, generators, analyzers, plugin hosting, presets, and realtime effects.
- Label, MIDI/note, time, and spectral tracks or views.
- Macros, scripting, batch workflows, advanced export options, cloud features, and the full preferences surface.

Accessibility is not a deferred feature. The first workspace needs a usable keyboard path and an accessible way to inspect and set the time range. Richer screen-reader interaction can expand alongside the later track and clip features.

## Project compatibility and data safety

Audacity project files are not interchangeable with exported audio. An `.aup3` stores project tracks, clips, labels, envelopes, gain/pan, and other editing data; export produces a playable audio file. The project also carries editing and recovery responsibilities, so the MVP must reuse Audacity's project I/O and recovery path rather than inventing a second project format. See [Managing Audacity Projects](https://manual.audacityteam.org/man/audacity_projects.html).

The MVP may open an `.aup3` only when it can represent and preserve every project object in that file. If a project contains unsupported tracks, effects, or other data, offer to open it in the complete legacy workspace. Never silently flatten, omit, or rewrite unsupported content. Saving from the MVP must preserve the same project data that was opened.

The full wxWidgets workspace remains available during rollout. The Alicorn workspace becomes eligible for default use only after import, edit, save/reopen, export, accessibility, and recovery behavior pass on supported platforms.

## Responsibility boundary

| Area | Owns |
|---|---|
| Audacity application/backend (Odin/Caliber as the rewrite advances) | Project and track data, authoritative time selection, audio transport, edits, undo/redo, file I/O, export, and error/recovery policy |
| Caliber boundary | Versioned, bounded application commands and revision-tagged state exchange; schema and command meaning remain Audacity-owned |
| Alicorn | Timeline layout, waveform presentation, input gestures, focus, semantics, and transient viewport state such as zoom and scroll |
| wxWidgets host during migration | Native window/event-loop integration, OS dialogs, clipboard/device services that have not moved, and platform bridging |

The GUI must not keep a second authoritative copy of the project, sample data, selection, or undo history. Pointer selection sends a value to the application layer; subsequent UI state is derived from the accepted application snapshot.

Use stable project/track identities and monotonically increasing revisions. A waveform response or edit based on an old revision must be discarded or rejected. The waveform request should describe only the visible time window and requested display resolution. Return bounded per-channel min/max summaries (and other display summaries only if the UI needs them), not entire PCM buffers on every frame. The real-time audio path must stay outside Caliber's general command queue.

The current migration should preserve an implementation seam: Alicorn consumes the Audacity application contract, while Odin/Caliber progressively takes ownership of the operations behind it. Do not expose C++ object layouts or private runtime storage across that boundary. If an MVP operation is not yet available in the Odin backend, identify it explicitly as a backend gate; do not implement a second copy of its policy in the UI.

## Implementation sequence

### 0. Freeze the support contract

Document the one-track project eligibility rules and the exact edit semantics. Define the state and commands the workspace needs: open/import, read project summary, request a visible waveform window, set selection, play/stop, delete, undo/redo, save, and export WAV. Keep error, cancellation, progress, and stale-revision behavior explicit.

### 1. Make Alicorn a window-level hosted surface

Resolve the shared host integration before adding more individual controls. The current controls use separate retained runtimes, which is acceptable as a prototype but not as the foundation for a full editor. Use one long-lived Alicorn runtime per project window, hosted by Audacity's event loop, and validate resize, focus, pointer capture, keyboard/IME, accessibility, scheduling, and teardown.

### 2. Deliver the read-only waveform workspace

Replace the central track canvas for eligible projects with one Alicorn timeline: ruler, waveform, selection/playhead display, scroll, and zoom. Reuse Audacity's cached waveform summaries rather than copying or scanning the whole audio buffer during pointer movement. Preserve the existing import and playback engines.

### 3. Complete the edit loop

Connect sample-accurate selection to Audacity's authoritative selection model. Add delete, undo, redo, dirty-state reporting, and playback of the selected range. Keep edit policy and history in the backend.

### 4. Add persistence and export, then validate recovery

Use the existing `.aup3` read/write and recovery machinery for supported projects, and the existing WAV export path. Test save/reopen fidelity, interrupted saves, recovery after an abnormal close, and failure reporting such as a full disk. Do not promote the MVP while those paths are incomplete.

### 5. Roll out behind a feature flag

Keep the complete wxWidgets workspace available for unsupported projects and as a rollback path. Enable the Alicorn workspace by default only after native validation on macOS, Windows, and Linux, including real file I/O and audio-device sessions.

## Existing Alicorn dependencies

These are already tracked by Alicorn and should be advanced instead of duplicated:

- [#196 — Integrate Alicorn with an existing desktop event loop](https://github.com/samanshaiza004/alicorn/issues/196): needed for one host-owned event loop and a shared retained runtime.
- [#162 — Host-owned renderer-neutral presentation boundary](https://github.com/samanshaiza004/alicorn/issues/162): relevant if the desktop host must own presentation independently of an Alicorn-created window.
- [#164 — Custom GPU canvas and waveform dogfood](https://github.com/samanshaiza004/alicorn/issues/164): relevant for a retained waveform surface and native rendering evidence.

The current Audacity control bridge emits surface, icon, and text commands; it skips geometry and image payloads. A waveform implementation therefore needs an explicit supported rendering path through Alicorn's public canvas/surface APIs and the host bridge. Do not create a parallel renderer inside the editor. I found no separate Alicorn issue to file for this proposal because the existing host and custom-canvas issues cover the concrete dependencies.

## Acceptance gates

1. **Import and view:** open a supported mono and stereo file; preserve sample rate/channel data; display the correct duration and waveform at multiple zoom levels.
2. **Selection:** drag in both directions, change zoom/scroll, and verify endpoints map to the expected source sample indices. Keyboard users can inspect and set the same range.
3. **Transport:** play from the cursor and selected range, stop, and verify the playhead follows the backend state without continuous polling while idle.
4. **Editing:** delete a range, verify the exact resulting audio/clip structure, then undo and redo and compare the source data.
5. **Persistence:** save and reopen `.aup3`, verify samples and project state, and exercise recovery/failure paths. Unsupported project content must open in the full workspace without mutation.
6. **Export:** export WAV and compare channel count, sample rate, duration, and decoded samples with the expected result.
7. **Performance:** for a long stereo file, waveform work scales with visible display resolution rather than total project duration. A selection drag must not trigger a full PCM copy, full-project decode, or Alicorn description rebuild for every motion event. Record p50/p95 input-to-visible and frame costs on each native backend before fixing release budgets.
8. **Platform behavior:** native builds and manual interaction pass on macOS, Windows, and Linux; include keyboard focus, screen-reader discoverability, real audio playback, file dialogs, and window close/recovery.

Use `alicorn:testui` for deterministic selection, keyboard, focus, and viewport assertions. It cannot prove actual GPU rendering, native audio behavior, file durability, or OS accessibility, so pair it with native integration checks.

## Expansion path toward Audacity 3 parity

1. **Composition:** multiple audio tracks, simple alignment, track selection, mute/solo, gain/pan, and basic mixing.
2. **Clip editing:** move/resize, split/join, trim, fades, clipboard operations, and undo semantics across tracks.
3. **Recording:** input/device selection, monitoring, latency, punch-and-roll, and recording recovery.
4. **Processing:** destructive and realtime effects, generators, analyzers, plugin formats, and presets.
5. **Additional media and views:** labels, MIDI/note and time tracks, spectral selection, spectrograms, envelopes, and specialized track controls.
6. **Power workflows:** macros/scripting, advanced export, preferences, batch operations, multi-window behavior, and remaining accessibility/platform parity.

Each addition should be an end-to-end vertical slice: backend command and persistence semantics, Alicorn interaction and semantics, native platform validation, and a regression fixture. Keep the old workspace until each supported project feature has a no-data-loss route through the new UI.

## Estimate and current progress

The current Alicorn work covers a handful of toolbar controls; the waveform editor and the editing workflow above have not started. By visible UI surface, this is roughly **5–10% of the GUI migration**, but effectively **0% of the MVP editing loop**. These are planning estimates, not measured code-completion metrics.

If existing Audacity project, audio, import, and export operations can be exposed behind the boundary, estimate **8–14 engineer-weeks** for the hosted waveform editor, core edit loop, persistence, recovery, accessibility, and native validation (about 2–4 calendar months for one experienced engineer). The current Odin/Caliber experiment does not yet own all of project I/O, import, playback, and export, so making the backend rewrite a strict prerequisite adds a separate, not-yet-sized phase. Re-estimate after the window-host spike and the first import→waveform→playback vertical slice.

Full parity is not responsibly dateable from this MVP estimate. It is multiple feature and platform-validation cycles beyond the first release.

## Sources

- [Audacity 3.7.9 release notes](https://github.com/audacity/audacity/releases/tag/Audacity-3.7.9) and [official releases list](https://github.com/audacity/audacity/releases).
- Audacity manual: [Importing Audio](https://manual.audacityteam.org/man/importing_audio.html), [Selecting Audio](https://manual.audacityteam.org/man/audacity_selection.html), [Playback](https://manual.audacityteam.org/man/playback.html), and [Managing Audacity Projects](https://manual.audacityteam.org/man/audacity_projects.html).
- Audacity 3.7.9 source: [`TrackPanel.cpp`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/src/TrackPanel.cpp), [`WaveformView.cpp`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/src/tracks/playabletrack/wavetrack/ui/WaveformView.cpp), [`WaveDataCache.cpp`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/libraries/lib-wave-track-paint/waveform/WaveDataCache.cpp), [`WaveClip.h`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/libraries/lib-wave-track/WaveClip.h), [`SelectedRegion.h`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/libraries/lib-time-frequency-selection/SelectedRegion.h), [`ProjectAudioManager.cpp`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/src/ProjectAudioManager.cpp), and [`EditMenus.cpp`](https://github.com/audacity/audacity/blob/Audacity-3.7.9/src/menus/EditMenus.cpp).
- Current fork integration overview: [`src/alicorn/README.md`](https://github.com/samanshaiza004/audacity/blob/main/src/alicorn/README.md).
