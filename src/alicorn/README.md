# Alicorn Snapping checkbox view

This directory contains an isolated Alicorn-backed view for Audacity 3's Snap
checkbox. It does not create a native window, event loop, renderer, or SDL
dependency. The host owns event delivery and drawing.

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

The view uses Alicorn's `runtime_set_viewport` and `paint_display_list` APIs.
The source checkout must provide those public APIs and Alicorn's bundled font
assets at compile time.

## C ABI and ownership

See [`AudacitySnapView.h`](AudacitySnapView.h) for the exported names, fixed
integer constants, and draw-command layout. `AudacityAlicornSnapView` is an
opaque, caller-owned handle. All operations for one handle must be serialized
on the same thread. Destroy it exactly once, after any paint callback returns.

`audacity_alicorn_snap_view_update` takes the current application-owned checked
value. It describes or flushes a retained frame and reports an activation with
`out_changed` and `out_checked`. The host remains authoritative: write the
reported value to `ProjectSnap`, then call update with the resulting state.
Pointer coordinates use Alicorn logical client units. The key vocabulary is
limited to Tab, Shift+Tab, and Space.

`audacity_alicorn_snap_view_paint` consumes the last completed Alicorn display
list synchronously. Its command pointer is stack-backed and valid only during
the callback. `text_utf8` is a borrowed, length-delimited view into runtime
storage, not a NUL-terminated string; copy it if it must outlive the callback.
The callback must not reenter or destroy the view. Surface colors are Alicorn
linear color values; geometry, clip, and translation are logical coordinates.
This adapter emits only the checkbox's surface, check icon, and label text
commands; unsupported paint payloads are skipped.

## Accessibility boundary

The retained Alicorn control is a standard checkbox labeled **Snap**, so its
runtime semantic node has checkbox role, accessible name, and checked state.
This adapter does not project that semantic node into wxWidgets' accessibility
tree. A wx integration still needs an accessibility bridge (or must keep the
existing accessible wx checkbox as the platform-facing accessible object) and
must synchronize its accessible value/focus state with `ProjectSnap` and this
view. Native screen-reader behavior is not validated by this isolated view.
