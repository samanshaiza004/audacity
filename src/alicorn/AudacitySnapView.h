#ifndef AUDACITY_SNAP_VIEW_H
#define AUDACITY_SNAP_VIEW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AudacityAlicornSnapView AudacityAlicornSnapView;
typedef struct AudacityAlicornActionView AudacityAlicornActionView;

typedef struct AudacitySnapRect {
    float x;
    float y;
    float width;
    float height;
} AudacitySnapRect;

typedef struct AudacitySnapColor {
    float r;
    float g;
    float b;
    float a;
} AudacitySnapColor;

typedef struct AudacitySnapDrawCommand {
    uint32_t kind;
    uint32_t reserved;
    uint64_t owner;
    AudacitySnapRect bounds;
    AudacitySnapRect clip;
    AudacitySnapColor fill;
    AudacitySnapColor stroke;
    AudacitySnapColor text_color;
    float stroke_width;
    float opacity;
    float translation_x;
    float translation_y;
    float radius;
    float radius_top_left;
    float radius_top_right;
    float radius_bottom_right;
    float radius_bottom_left;
    uint32_t shape_kind;
    uint32_t stroke_placement;
    uint32_t material_id;
    uint32_t icon_id;
    const uint8_t *text_utf8;
    uint64_t text_length;
} AudacitySnapDrawCommand;

/* Draw command kinds. Other Alicorn payloads are not emitted by this view. */
enum {
    AUDACITY_SNAP_DRAW_SURFACE = 1,
    AUDACITY_SNAP_DRAW_ICON = 2,
    AUDACITY_SNAP_DRAW_TEXT = 3
};

/* Surface shape values emitted by this ABI. */
enum {
    AUDACITY_SNAP_SHAPE_RECTANGLE = 1,
    AUDACITY_SNAP_SHAPE_ROUNDED_RECTANGLE = 2,
    AUDACITY_SNAP_SHAPE_LINE = 3,
    AUDACITY_SNAP_SHAPE_PER_CORNER_ROUNDED_RECTANGLE = 4,
    AUDACITY_SNAP_SHAPE_CONNECTED_TAB = 5
};

/* Alicorn icon values emitted by this ABI. */
enum {
    AUDACITY_SNAP_ICON_NONE = 0,
    AUDACITY_SNAP_ICON_CHECK = 1
};

/* Pointer kinds; button values use 1=primary, 2=middle, 3=secondary. */
enum {
    AUDACITY_SNAP_POINTER_MOVE = 0,
    AUDACITY_SNAP_POINTER_DOWN = 1,
    AUDACITY_SNAP_POINTER_UP = 2,
    AUDACITY_SNAP_POINTER_CANCEL = 3,
    AUDACITY_SNAP_BUTTON_PRIMARY = 1,
    AUDACITY_SNAP_BUTTON_MIDDLE = 2,
    AUDACITY_SNAP_BUTTON_SECONDARY = 3
};

/* The intentionally small keyboard vocabulary needed by a checkbox. */
enum {
    AUDACITY_SNAP_KEY_TAB = 0,
    AUDACITY_SNAP_KEY_SHIFT_TAB = 1,
    AUDACITY_SNAP_KEY_SPACE = 2,
    AUDACITY_SNAP_KEY_ACTIVATE = 3
};

/*
 * command and text_utf8 are borrowed and valid only until the callback
 * returns. text_utf8 is length-delimited and is not NUL-terminated. Copy any
 * data that must outlive the callback.
 */
typedef void (*AudacitySnapDrawCallback)(
    void *context,
    const AudacitySnapDrawCommand *command);

/* Returns NULL for invalid dimensions or runtime/font initialization failure. */
AudacityAlicornSnapView *audacity_alicorn_snap_view_create(
    float width,
    float height,
    uint8_t checked);

/* The caller owns the opaque view and must destroy it exactly once. */
void audacity_alicorn_snap_view_destroy(AudacityAlicornSnapView *view);

/* resize: 1 changed, 0 unchanged, -1 invalid handle/dimensions or busy view. */
int32_t audacity_alicorn_snap_view_resize(
    AudacityAlicornSnapView *view,
    float width,
    float height);

/*
 * pointer: 1 if the checkbox consumed/hit the event, 0 if not, -1 on invalid
 * input. Coordinates are logical client units. Call update after input.
 */
int32_t audacity_alicorn_snap_view_pointer(
    AudacityAlicornSnapView *view,
    uint32_t kind,
    float x,
    float y,
    uint32_t button);

/* key: 1 if Alicorn handled it, 0 if unhandled, -1 for invalid input. */
int32_t audacity_alicorn_snap_view_key(
    AudacityAlicornSnapView *view,
    uint32_t key);

/*
 * update: 1 if a description/presentation frame was processed, 0 if idle,
 * -1 for invalid arguments or a busy view. checked is application-owned input.
 * out_checked receives the effective value; out_changed is nonzero only when
 * this update consumed a checkbox activation. Either output may be NULL.
 */
int32_t audacity_alicorn_snap_view_update(
    AudacityAlicornSnapView *view,
    uint8_t checked,
    uint8_t *out_checked,
    uint8_t *out_changed);

/*
 * paint synchronously visits the completed Alicorn display list and returns
 * the number of supported commands delivered, or -1 on invalid input/state.
 * The callback must not reenter this view or destroy it.
 */
int32_t audacity_alicorn_snap_view_paint(
    AudacityAlicornSnapView *view,
    AudacitySnapDrawCallback callback,
    void *context);

/* Creates a labeled, controlled Alicorn action button for a native host. */
AudacityAlicornActionView *audacity_alicorn_action_view_create(
    float width,
    float height,
    const char *label_utf8,
    uint8_t enabled,
    uint8_t pressed);

void audacity_alicorn_action_view_destroy(AudacityAlicornActionView *view);
int32_t audacity_alicorn_action_view_resize(
    AudacityAlicornActionView *view,
    float width,
    float height);
int32_t audacity_alicorn_action_view_pointer(
    AudacityAlicornActionView *view,
    uint32_t kind,
    float x,
    float y,
    uint32_t button);
int32_t audacity_alicorn_action_view_key(
    AudacityAlicornActionView *view,
    uint32_t key);
int32_t audacity_alicorn_action_view_update(
    AudacityAlicornActionView *view,
    uint8_t enabled,
    uint8_t pressed,
    uint8_t *out_activated);
int32_t audacity_alicorn_action_view_paint(
    AudacityAlicornActionView *view,
    AudacitySnapDrawCallback callback,
    void *context);

#ifdef __cplusplus
}
#endif

#endif /* AUDACITY_SNAP_VIEW_H */
