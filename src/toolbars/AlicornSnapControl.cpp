/*  SPDX-License-Identifier: GPL-2.0-or-later */

#include "AlicornSnapControl.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/bitmap.h>
#include <wx/graphics.h>
#include <wx/log.h>

#include "AColor.h"
#include "AllThemeResources.h"
#include "Theme.h"
#include "AlicornPaint.h"
#include "AudacitySnapView.h"

#if wxUSE_ACCESSIBILITY
#include "WindowAccessible.h"

namespace {
class SnapControlAccessible final : public WindowAccessible {
public:
   explicit SnapControlAccessible(AlicornSnapControl* control)
      : WindowAccessible(control)
   {}

   wxAccStatus GetRole(int childId, wxAccRole* role) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      *role = wxROLE_SYSTEM_CHECKBUTTON;
      return wxACC_OK;
   }

   wxAccStatus GetState(int childId, long* state) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      auto* control = static_cast<AlicornSnapControl*>(GetWindow());
      if (!control)
         return wxACC_FAIL;

      *state = wxACC_STATE_SYSTEM_FOCUSABLE;
      if (control->HasFocus())
         *state |= wxACC_STATE_SYSTEM_FOCUSED;
      if (control->IsChecked())
         *state |= wxACC_STATE_SYSTEM_CHECKED;
      if (!control->IsEnabled())
         *state |= wxACC_STATE_SYSTEM_UNAVAILABLE;
      return wxACC_OK;
   }

   wxAccStatus GetDefaultAction(int childId, wxString* actionName) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      *actionName = wxT("Toggle");
      return wxACC_OK;
   }

   wxAccStatus DoDefaultAction(int childId) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      auto* control = static_cast<AlicornSnapControl*>(GetWindow());
      if (!control || !control->IsEnabled())
         return wxACC_FAIL;
      control->Activate();
      return wxACC_OK;
   }

   wxAccStatus GetValue(int childId, wxString* value) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      auto* control = static_cast<AlicornSnapControl*>(GetWindow());
      if (!control)
         return wxACC_FAIL;
      *value = control->IsChecked() ? wxT("1") : wxT("0");
      return wxACC_OK;
   }
};
}
#endif

namespace {
const wxSize MinimumSize{ 78, 30 };

double LinearToSrgb(float value)
{
   const auto linear = std::clamp(static_cast<double>(value), 0.0, 1.0);
   return linear <= 0.0031308
      ? linear * 12.92
      : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
}

wxColour ToWxColour(AudacitySnapColor color, float opacity)
{
   const auto alpha = std::clamp(color.a * opacity, 0.0f, 1.0f);
   return wxColour(
      static_cast<unsigned char>(std::lround(LinearToSrgb(color.r) * 255.0)),
      static_cast<unsigned char>(std::lround(LinearToSrgb(color.g) * 255.0)),
      static_cast<unsigned char>(std::lround(LinearToSrgb(color.b) * 255.0)),
      static_cast<unsigned char>(std::lround(alpha * 255.0f)));
}
}

namespace AudacityAlicorn {
void DrawPaintCommand(void* rawContext, const AudacitySnapDrawCommand* command)
{
   auto* context = static_cast<PaintContext*>(rawContext);
   if (!context || !context->graphics || !command)
      return;

   auto* graphics = context->graphics;
   graphics->PushState();
   if (command->clip.width >= 0.0f && command->clip.height >= 0.0f)
      graphics->Clip(command->clip.x, command->clip.y,
         command->clip.width, command->clip.height);
   graphics->Translate(command->translation_x, command->translation_y);

   const auto& bounds = command->bounds;
   const auto fill = ToWxColour(command->fill, command->opacity);
   const auto stroke = ToWxColour(command->stroke, command->opacity);
   switch (command->kind) {
   case AUDACITY_SNAP_DRAW_SURFACE: {
      if (command->shape_kind == AUDACITY_SNAP_SHAPE_LINE) {
         graphics->SetPen(wxPen(fill, std::max(1.0f, command->stroke_width)));
         graphics->StrokeLine(bounds.x, bounds.y,
            bounds.x + bounds.width, bounds.y + bounds.height);
         break;
      }

      graphics->SetBrush(wxBrush(fill));
      if (command->stroke_width > 0.0f && stroke.Alpha() != 0)
         graphics->SetPen(wxPen(stroke, command->stroke_width));
      else
         graphics->SetPen(*wxTRANSPARENT_PEN);

      if (command->shape_kind == AUDACITY_SNAP_SHAPE_ROUNDED_RECTANGLE ||
          command->shape_kind == AUDACITY_SNAP_SHAPE_PER_CORNER_ROUNDED_RECTANGLE ||
          command->shape_kind == AUDACITY_SNAP_SHAPE_CONNECTED_TAB) {
         graphics->DrawRoundedRectangle(bounds.x, bounds.y,
            bounds.width, bounds.height,
            std::max(0.0f, command->radius));
      }
      else {
         graphics->DrawRectangle(bounds.x, bounds.y,
            bounds.width, bounds.height);
      }
      break;
   }
   case AUDACITY_SNAP_DRAW_ICON:
      if (command->icon_id == AUDACITY_SNAP_ICON_CHECK) {
         graphics->SetPen(wxPen(fill, std::max(1.5f, bounds.height * 0.11f), wxPENSTYLE_SOLID));
         graphics->SetBrush(*wxTRANSPARENT_BRUSH);
         auto path = graphics->CreatePath();
         path.MoveToPoint(bounds.x + bounds.width * 0.20,
            bounds.y + bounds.height * 0.53);
         path.AddLineToPoint(bounds.x + bounds.width * 0.43,
            bounds.y + bounds.height * 0.76);
         path.AddLineToPoint(bounds.x + bounds.width * 0.82,
            bounds.y + bounds.height * 0.25);
         graphics->StrokePath(path);
      }
      else if (command->icon_id == AUDACITY_SNAP_ICON_SETTINGS) {
         const auto image = theTheme.Image(bmpCogwheel);
         if (image.IsOk()) {
            const wxBitmap bitmap{ image };
            graphics->DrawBitmap(bitmap, bounds.x, bounds.y,
               bounds.width, bounds.height);
         }
      }
      else if (command->icon_id == AUDACITY_SNAP_ICON_AUDIO_SETUP) {
         const auto image = theTheme.Image(bmpSetup);
         if (image.IsOk()) {
            const wxBitmap bitmap{ image };
            const auto width = static_cast<double>(image.GetWidth());
            const auto height = static_cast<double>(image.GetHeight());
            graphics->DrawBitmap(bitmap,
               bounds.x + (bounds.width - width) / 2.0,
               bounds.y + (bounds.height - height) / 2.0,
               width, height);
         }
      }
      break;
   case AUDACITY_SNAP_DRAW_TEXT:
      if (command->text_utf8 && command->text_length > 0) {
         const auto text = wxString::FromUTF8(
            reinterpret_cast<const char*>(command->text_utf8),
            static_cast<size_t>(command->text_length));
         graphics->SetFont(context->font, ToWxColour(command->text_color, command->opacity));
         graphics->DrawText(text, bounds.x, bounds.y);
      }
      break;
   default:
      break;
   }
   graphics->PopState();
}
}

AlicornSnapControl::AlicornSnapControl(wxWindow* parent)
   : wxWindow(parent, wxID_ANY, wxDefaultPosition, MinimumSize, wxBORDER_NONE)
{
   SetMinSize(MinimumSize);
   SetBackgroundStyle(wxBG_STYLE_PAINT);
   SetName(wxT("Snap"));

   const auto size = GetClientSize();
   mView = audacity_alicorn_snap_view_create(
      static_cast<float>(size.x), static_cast<float>(size.y), 0);
   if (!mView)
      wxLogError(wxT("Could not initialize the Alicorn Snap control"));

#if wxUSE_ACCESSIBILITY
   SetAccessible(safenew SnapControlAccessible(this));
#endif

   Bind(wxEVT_PAINT, &AlicornSnapControl::OnPaint, this);
   Bind(wxEVT_SIZE, &AlicornSnapControl::OnSize, this);
   Bind(wxEVT_LEFT_DOWN, &AlicornSnapControl::OnLeftDown, this);
   Bind(wxEVT_LEFT_UP, &AlicornSnapControl::OnLeftUp, this);
   Bind(wxEVT_MOTION, &AlicornSnapControl::OnMotion, this);
   Bind(wxEVT_LEAVE_WINDOW, &AlicornSnapControl::OnLeave, this);
   Bind(wxEVT_MOUSE_CAPTURE_LOST, &AlicornSnapControl::OnCaptureLost, this);
   Bind(wxEVT_KEY_DOWN, &AlicornSnapControl::OnKeyDown, this);
   Bind(wxEVT_SET_FOCUS, &AlicornSnapControl::OnSetFocus, this);
}

AlicornSnapControl::~AlicornSnapControl()
{
   if (mView)
      audacity_alicorn_snap_view_destroy(mView);
}

void AlicornSnapControl::SetChecked(bool checked)
{
   if (mChecked == checked)
      return;
   mChecked = checked;
   UpdateView();
}

void AlicornSnapControl::Activate()
{
   if (!mView || !IsEnabled())
      return;
   SetFocus();
   audacity_alicorn_snap_view_key(mView, AUDACITY_SNAP_KEY_SPACE);
   UpdateView();
}

void AlicornSnapControl::PublishChange(bool checked)
{
   const auto handler = mChangeHandler;
   if (handler)
      handler(checked);
}

void AlicornSnapControl::UpdateView()
{
   if (!mView || mUpdating)
      return;

   mUpdating = true;
   uint8_t checked = mChecked ? 1 : 0;
   uint8_t changed = 0;
   const auto result = audacity_alicorn_snap_view_update(
      mView, checked, &checked, &changed);
   if (result < 0) {
      mUpdating = false;
      wxLogWarning(wxT("Alicorn Snap control failed to update"));
      return;
   }
   mChecked = checked != 0;
   mUpdating = false;
   Refresh(false);
   if (changed)
      PublishChange(mChecked);
}

int32_t AlicornSnapControl::SendPointer(
   uint32_t kind, float x, float y, uint32_t button)
{
   if (!mView)
      return -1;
   const auto result = audacity_alicorn_snap_view_pointer(
      mView, kind, x, y, button);
   if (result >= 0)
      UpdateView();
   return result;
}

void AlicornSnapControl::OnPaint(wxPaintEvent&)
{
   wxAutoBufferedPaintDC dc(this);
   dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
   dc.Clear();
   if (!mView)
      return;

   std::unique_ptr<wxGraphicsContext> graphics{ wxGraphicsContext::Create(dc) };
   if (!graphics)
      return;
   AudacityAlicorn::PaintContext context{ graphics.get(), GetFont() };
   if (audacity_alicorn_snap_view_paint(
          mView, &AudacityAlicorn::DrawPaintCommand, &context) < 0)
      wxLogWarning(wxT("Alicorn Snap control could not read its paint list"));
}

void AlicornSnapControl::OnSize(wxSizeEvent& event)
{
   if (mView) {
      const auto size = GetClientSize();
      if (audacity_alicorn_snap_view_resize(
             mView, static_cast<float>(size.x), static_cast<float>(size.y)) < 0)
         wxLogWarning(wxT("Alicorn Snap control rejected its new size"));
      UpdateView();
   }
   event.Skip();
}

void AlicornSnapControl::OnLeftDown(wxMouseEvent& event)
{
   SetFocus();
   if (!HasCapture())
      CaptureMouse();
   SendPointer(AUDACITY_SNAP_POINTER_DOWN, event.GetX(), event.GetY(),
      AUDACITY_SNAP_BUTTON_PRIMARY);
}

void AlicornSnapControl::OnLeftUp(wxMouseEvent& event)
{
   if (!mView)
      return;
   const auto result = audacity_alicorn_snap_view_pointer(
      mView, AUDACITY_SNAP_POINTER_UP, event.GetX(), event.GetY(),
      AUDACITY_SNAP_BUTTON_PRIMARY);
   if (HasCapture())
      ReleaseMouse();
   if (result >= 0)
      UpdateView(); // May synchronously close the owning toolbar.
}

void AlicornSnapControl::OnMotion(wxMouseEvent& event)
{
   SendPointer(AUDACITY_SNAP_POINTER_MOVE, event.GetX(), event.GetY(),
      AUDACITY_SNAP_BUTTON_PRIMARY);
}

void AlicornSnapControl::OnLeave(wxMouseEvent& event)
{
   if (!HasCapture())
      SendPointer(AUDACITY_SNAP_POINTER_MOVE, -1.0f, -1.0f, 0);
   event.Skip();
}

void AlicornSnapControl::OnCaptureLost(wxMouseCaptureLostEvent&)
{
   SendPointer(AUDACITY_SNAP_POINTER_CANCEL, -1.0f, -1.0f, 0);
}

void AlicornSnapControl::OnKeyDown(wxKeyEvent& event)
{
   if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN ||
       event.GetKeyCode() == WXK_NUMPAD_ENTER) {
      Activate();
      return;
   }
   if (event.GetKeyCode() == WXK_TAB) {
      event.Skip();
      return;
   }
   event.Skip();
}

void AlicornSnapControl::OnSetFocus(wxFocusEvent& event)
{
   if (mView) {
      audacity_alicorn_snap_view_key(mView, AUDACITY_SNAP_KEY_TAB);
      UpdateView();
   }
   event.Skip();
}
