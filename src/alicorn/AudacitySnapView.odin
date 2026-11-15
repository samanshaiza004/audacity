package audacity_alicorn_snap_view

import alicorn "alicorn:runtime"
import odin_runtime "base:runtime"

AudacitySnapRect :: struct {
	x, y, width, height: f32,
}

AudacitySnapColor :: struct {
	r, g, b, a: f32,
}

AudacitySnapDrawCommand :: struct {
	kind: u32,
	reserved: u32,
	owner: u64,
	bounds: AudacitySnapRect,
	clip: AudacitySnapRect,
	fill: AudacitySnapColor,
	stroke: AudacitySnapColor,
	text_color: AudacitySnapColor,
	stroke_width: f32,
	opacity: f32,
	translation_x: f32,
	translation_y: f32,
	radius: f32,
	radius_top_left: f32,
	radius_top_right: f32,
	radius_bottom_right: f32,
	radius_bottom_left: f32,
	shape_kind: u32,
	stroke_placement: u32,
	material_id: u32,
	icon_id: u32,
	text_utf8: ^u8,
	text_length: u64,
}

AudacitySnapDrawCallback :: #type proc "c" (
	ctx: rawptr,
	command: ^AudacitySnapDrawCommand,
)

View :: struct {
	runtime: alicorn.Runtime,
	checkbox_id: alicorn.Node_ID,
	last_checked: bool,
	checked_initialized: bool,
	callback_active: bool,
	runtime_initialized: bool,
}

AUDACITY_SNAP_DRAW_SURFACE :: u32(1)
AUDACITY_SNAP_DRAW_ICON :: u32(2)
AUDACITY_SNAP_DRAW_TEXT :: u32(3)

AUDACITY_SNAP_SHAPE_RECTANGLE :: u32(1)
AUDACITY_SNAP_SHAPE_ROUNDED_RECTANGLE :: u32(2)
AUDACITY_SNAP_SHAPE_LINE :: u32(3)
AUDACITY_SNAP_SHAPE_PER_CORNER_ROUNDED_RECTANGLE :: u32(4)
AUDACITY_SNAP_SHAPE_CONNECTED_TAB :: u32(5)

AUDACITY_SNAP_ICON_NONE :: u32(0)
AUDACITY_SNAP_ICON_CHECK :: u32(1)

AUDACITY_SNAP_POINTER_MOVE :: u32(0)
AUDACITY_SNAP_POINTER_DOWN :: u32(1)
AUDACITY_SNAP_POINTER_UP :: u32(2)
AUDACITY_SNAP_POINTER_CANCEL :: u32(3)

AUDACITY_SNAP_KEY_TAB :: u32(0)
AUDACITY_SNAP_KEY_SHIFT_TAB :: u32(1)
AUDACITY_SNAP_KEY_SPACE :: u32(2)

valid_view :: proc(view: ^View) -> bool {
	return view != nil && view.runtime_initialized && !view.callback_active
}

snap_color :: proc(color: alicorn.Color) -> AudacitySnapColor {
	return AudacitySnapColor{color.r, color.g, color.b, color.a}
}

snap_rect :: proc(rect: alicorn.Rect) -> AudacitySnapRect {
	return AudacitySnapRect{rect.x, rect.y, rect.w, rect.h}
}

snap_shape_kind :: proc(kind: alicorn.Surface_Shape_Kind) -> u32 {
	switch kind {
	case .Rectangle: return AUDACITY_SNAP_SHAPE_RECTANGLE
	case .Rounded_Rectangle: return AUDACITY_SNAP_SHAPE_ROUNDED_RECTANGLE
	case .Line: return AUDACITY_SNAP_SHAPE_LINE
	case .Rounded_Rectangle_Per_Corner: return AUDACITY_SNAP_SHAPE_PER_CORNER_ROUNDED_RECTANGLE
	case .Connected_Tab: return AUDACITY_SNAP_SHAPE_CONNECTED_TAB
	}
	return 0
}

snap_view_build_description :: proc(view: ^View, checked: bool) -> (built: bool, effective_checked: bool, changed: bool) {
	ui, should_build := alicorn.begin_frame(&view.runtime)
	if !should_build {
		return false, checked, false
	}

	viewport := alicorn.viewport_bounds(&view.runtime)
	root_style := alicorn.layout_style(
		.Row,
		width=viewport.w,
		height=viewport.h,
		padding=2,
		align=.Start,
	)
	_ = alicorn.container_begin(
		&ui,
		.Root,
		key=alicorn.key_string("audacity-alicorn-snap-root"),
		style=root_style,
	)
	control := alicorn.checkbox(
		&ui,
		"Snap",
		checked,
		key=alicorn.key_string("audacity-alicorn-snap-checkbox"),
	)
	alicorn.container_end(&ui)
	alicorn.end_frame(&ui)

	view.checkbox_id, _ = alicorn.node_by_key(
		&view.runtime,
		alicorn.key_string("audacity-alicorn-snap-checkbox"),
		.Checkbox,
	)
	return true, control.value, control.changed
}

snap_view_flush_presentation :: proc(view: ^View) -> bool {
	ui, ready := alicorn.begin_presentation_frame(&view.runtime)
	if !ready { return false }
	alicorn.end_presentation_frame(&ui)
	return true
}

@(export, link_name="audacity_alicorn_snap_view_create")
audacity_alicorn_snap_view_create :: proc "c" (width, height: f32, checked: u8) -> rawptr {
	context = odin_runtime.default_context()
	if !alicorn.number_is_finite(width) || !alicorn.number_is_finite(height) || width < 0 || height < 0 {
		return nil
	}

	view := new(View)
	view.runtime = alicorn.new_runtime(alicorn.Rect{0, 0, width, height})
	view.runtime_initialized = true
	view.last_checked = checked != 0
	view.checked_initialized = true
	if !alicorn.text_engine_load_default_fonts(&view.runtime) {
		alicorn.destroy_runtime(&view.runtime)
		view.runtime_initialized = false
		free(view)
		return nil
	}
	return rawptr(view)
}

@(export, link_name="audacity_alicorn_snap_view_destroy")
audacity_alicorn_snap_view_destroy :: proc "c" (opaque_view: rawptr) {
	if opaque_view == nil { return }
	context = odin_runtime.default_context()
	view := transmute(^View)opaque_view
	// A destroy requested from a draw callback is ignored rather than freeing
	// the storage currently borrowed by that callback. The caller may destroy
	// normally after the callback returns.
	if view.callback_active { return }
	if view.runtime_initialized {
		alicorn.destroy_runtime(&view.runtime)
		view.runtime_initialized = false
	}
	free(view)
}

@(export, link_name="audacity_alicorn_snap_view_resize")
audacity_alicorn_snap_view_resize :: proc "c" (opaque_view: rawptr, width, height: f32) -> i32 {
	context = odin_runtime.default_context()
	view := transmute(^View)opaque_view
	if !valid_view(view) || !alicorn.number_is_finite(width) || !alicorn.number_is_finite(height) || width < 0 || height < 0 {
		return -1
	}
	previous := alicorn.viewport_bounds(&view.runtime)
	if previous.x == 0 && previous.y == 0 && previous.w == width && previous.h == height {
		return 0
	}
	if alicorn.runtime_set_viewport(&view.runtime, alicorn.Rect{0, 0, width, height}) {
		return 1
	}
	return -1
}

@(export, link_name="audacity_alicorn_snap_view_pointer")
audacity_alicorn_snap_view_pointer :: proc "c" (
	opaque_view: rawptr,
	kind: u32,
	x, y: f32,
	button: u32,
) -> i32 {
	context = odin_runtime.default_context()
	view := transmute(^View)opaque_view
	if !valid_view(view) || !alicorn.number_is_finite(x) || !alicorn.number_is_finite(y) {
		return -1
	}
	pointer_kind: alicorn.Pointer_Kind
	switch kind {
	case AUDACITY_SNAP_POINTER_MOVE: pointer_kind = .Move
	case AUDACITY_SNAP_POINTER_DOWN: pointer_kind = .Down
	case AUDACITY_SNAP_POINTER_UP: pointer_kind = .Up
	case AUDACITY_SNAP_POINTER_CANCEL: pointer_kind = .Cancel
	case: return -1
	}
	if kind == AUDACITY_SNAP_POINTER_MOVE || kind == AUDACITY_SNAP_POINTER_CANCEL {
		if button > 3 { return -1 }
	} else if button > 3 {
		return -1
	}
	target := alicorn.process_pointer(&view.runtime, alicorn.Pointer_Event{
		kind=pointer_kind,
		x=x,
		y=y,
		button=int(button),
	})
	if target != 0 && target == view.checkbox_id { return 1 }
	return 0
}

@(export, link_name="audacity_alicorn_snap_view_key")
audacity_alicorn_snap_view_key :: proc "c" (opaque_view: rawptr, key: u32) -> i32 {
	context = odin_runtime.default_context()
	view := transmute(^View)opaque_view
	if !valid_view(view) { return -1 }
	switch key {
	case AUDACITY_SNAP_KEY_TAB:
		return 1 if alicorn.application_dispatch_tab_focus(&view.runtime, false) else 0
	case AUDACITY_SNAP_KEY_SHIFT_TAB:
		return 1 if alicorn.application_dispatch_tab_focus(&view.runtime, true) else 0
	case AUDACITY_SNAP_KEY_SPACE:
		return 1 if alicorn.application_route_focused_control_key(&view.runtime, .Toggle) else 0
	case:
		return -1
	}
}

@(export, link_name="audacity_alicorn_snap_view_update")
audacity_alicorn_snap_view_update :: proc "c" (
	opaque_view: rawptr,
	checked: u8,
	out_checked: ^u8,
	out_changed: ^u8,
) -> i32 {
	context = odin_runtime.default_context()
	view := transmute(^View)opaque_view
	if !valid_view(view) { return -1 }
	checked_value := checked != 0
	if out_checked != nil { out_checked^ = checked }
	if out_changed != nil { out_changed^ = 0 }
	if view.checked_initialized && view.last_checked != checked_value {
		alicorn.invalidate_root(&view.runtime, "Audacity Snap state changed")
	}
	view.last_checked = checked_value
	view.checked_initialized = true

	built, effective_checked, changed := snap_view_build_description(view, checked_value)
	if built {
		if changed {
			if out_checked != nil { out_checked^ = 1 if effective_checked else 0 }
			if out_changed != nil { out_changed^ = 1 }
		}
		return 1
	}
	if snap_view_flush_presentation(view) { return 1 }
	return 0
}

snap_view_command_base :: proc(command: alicorn.Paint_Command, kind: u32) -> AudacitySnapDrawCommand {
	return AudacitySnapDrawCommand{
		kind=kind,
		owner=u64(command.owner),
		bounds=snap_rect(command.bounds),
		clip=snap_rect(command.clip),
		opacity=command.opacity,
		translation_x=command.translation[0],
		translation_y=command.translation[1],
		stroke_placement=u32(command.stroke_placement),
	}
}

@(export, link_name="audacity_alicorn_snap_view_paint")
audacity_alicorn_snap_view_paint :: proc "c" (
	opaque_view: rawptr,
	callback: AudacitySnapDrawCallback,
	callback_context: rawptr,
) -> i32 {
	context = odin_runtime.default_context()
	view := transmute(^View)opaque_view
	if !valid_view(view) || callback == nil { return -1 }
	display, ok := alicorn.paint_display_list(&view.runtime)
	if !ok { return -1 }

	view.callback_active = true
	delivered := i32(0)
	for retained in display.commands {
		if retained.owner != view.checkbox_id { continue }
		output := AudacitySnapDrawCommand{}
		switch payload in retained.payload {
		case alicorn.Surface_Paint:
			output = snap_view_command_base(retained, AUDACITY_SNAP_DRAW_SURFACE)
			output.fill = snap_color(payload.fill)
			output.stroke = snap_color(payload.stroke.color)
			output.stroke_width = payload.stroke.width
			output.radius = payload.shape.corner_radius
			output.shape_kind = snap_shape_kind(payload.shape.kind)
			output.material_id = u32(payload.material)
			if shape, shape_ok := alicorn.surface_paint_shape_resolve(&view.runtime, retained.owner, payload.shape); shape_ok {
				radii := alicorn.surface_shape_corner_radii(shape)
				output.radius_top_left = radii.top_left
				output.radius_top_right = radii.top_right
				output.radius_bottom_right = radii.bottom_right
				output.radius_bottom_left = radii.bottom_left
			}
		case alicorn.Icon_Paint:
			if payload.id != .Check { continue }
			output = snap_view_command_base(retained, AUDACITY_SNAP_DRAW_ICON)
			output.icon_id = AUDACITY_SNAP_ICON_CHECK
			output.fill = snap_color(payload.color)
		case alicorn.Text_Paint:
			run, run_ok := alicorn.paint_text_run_resolve(&view.runtime, payload.run)
			if !run_ok { continue }
			output = snap_view_command_base(retained, AUDACITY_SNAP_DRAW_TEXT)
			output.text_color = snap_color(payload.color)
			output.text_length = u64(len(run.value))
			if len(run.value) > 0 { output.text_utf8 = raw_data(run.value) }
		case alicorn.Backdrop_Effect_Paint, alicorn.Image_Paint, alicorn.Geometry_Paint:
			continue
		case:
			continue
		}
		callback(callback_context, &output)
		delivered += 1
	}
	view.callback_active = false
	return delivered
}
