# Alicorn-backed Audacity controls

This directory contains small Alicorn-backed controls hosted by Audacity 3.
The current opt-in integration replaces the Snap checkbox, the Audio Setup
toolbar button, the Selection Toolbar mode-setup button, and the Cut/Copy/Paste
toolbar buttons. Audacity still owns the native window, event loop, accessibility
bridge, menus, audio-device state, selection controls, command dispatch, and
drawing surface; Alicorn supplies retained control descriptions and paint
commands. This adapter does not create an SDL window or renderer.

## Build

Compile `AudacitySnapView.odin` as a static library with the external Alicorn
repository root mapped as the collection (the import selects `runtime`), for
example:

```sh
odin build src/alicorn \
  -collection:alicorn=/path/to/alicorn \
  -build-mode:lib -no-entry-point \
  -out:libaudacity_alicorn_snap_view.a
```

The views use Alicorn's `runtime_set_viewport` and `paint_display_list` APIs.
The source checkout must provide those public APIs and Alicorn's bundled font
assets at compile time. Each hosted control currently owns a small retained
runtime; sharing a host session is a separate integration step.

## C ABI and ownership

See [`AudacitySnapView.h`](AudacitySnapView.h) for the exported names, fixed
integer constants, and draw-command layout. The Snap and action views are
opaque, caller-owned handles. All operations for one handle must be serialized
on the same thread. Destroy a handle exactly once, after any paint callback
returns. The action view copies its localized UTF-8 label during creation.

`audacity_alicorn_snap_view_update` takes the current application-owned checked
value. It describes or flushes a retained frame and reports an activation with
`out_changed` and `out_checked`. The host remains authoritative: write the
reported value to `ProjectSnap`, then call update with the resulting state.
Pointer coordinates use Alicorn logical client units. The key vocabulary
includes Tab, Shift+Tab, Space, and an explicit activation request.

`audacity_alicorn_snap_view_paint` consumes the last completed Alicorn display
list synchronously. Its command pointer is stack-backed and valid only during
the callback. `text_utf8` is a borrowed, length-delimited view into runtime
storage, not a NUL-terminated string; copy it if it must outlive the callback.
The callback must not reenter or destroy the view. Surface colors are Alicorn
linear color values; geometry, clip, and translation are logical coordinates.
This adapter emits only the checkbox's surface, check icon, and label text
commands, or the action button's surface and label; unsupported paint payloads
are skipped.

## Accessibility boundary

The retained Alicorn controls provide the visual and input behavior. Audacity's
wx host objects remain the platform-facing accessible objects: Snap exposes a
checkbox role/value, while Audio Setup exposes a push-button role and localized
name. The wx bridge synchronizes their state with `ProjectSnap` and the native
audio menu. Native screen-reader behavior has not yet been manually validated.
