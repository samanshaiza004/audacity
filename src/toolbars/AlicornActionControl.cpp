/*  SPDX-License-Identifier: GPL-2.0-or-later */

#include "AlicornActionControl.h"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/log.h>

#include "AlicornPaint.h"

#if wxUSE_ACCESSIBILITY
#include "WindowAccessible.h"

namespace {
class ActionControlAccessible final : public WindowAccessible {
public:
   explicit ActionControlAccessible(AlicornActionControl* control)
      : WindowAccessible(control)
   {}

   wxAccStatus GetRole(int childId, wxAccRole* role) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      *role = wxROLE_SYSTEM_PUSHBUTTON;
      return wxACC_OK;
   }

   wxAccStatus GetState(int childId, long* state) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      auto* control = static_cast<AlicornActionControl*>(GetWindow());
      if (!control)
         return wxACC_FAIL;

      *state = wxACC_STATE_SYSTEM_FOCUSABLE;
      if (control->HasFocus())
         *state |= wxACC_STATE_SYSTEM_FOCUSED;
      if (control->IsPressed())
         *state |= wxACC_STATE_SYSTEM_PRESSED;
      if (!control->IsEnabled())
         *state |= wxACC_STATE_SYSTEM_UNAVAILABLE;
      return wxACC_OK;
   }

   wxAccStatus GetDefaultAction(int childId, wxString* actionName) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      *actionName = wxT("Press");
      return wxACC_OK;
   }

   wxAccStatus DoDefaultAction(int childId) override
   {
      if (childId != wxACC_SELF && childId != 0)
         return wxACC_FAIL;
      auto* control = static_cast<AlicornActionControl*>(GetWindow());
      if (!control || !control->IsEnabled())
         return wxACC_FAIL;
      control->Activate();
      return wxACC_OK;
   }
};
}
#endif

namespace {
const wxSize MinimumSize{ 112, 48 };
}

AlicornActionControl::AlicornActionControl(
   wxWindow* parent, wxWindowID id, const wxString& label,
   uint32_t iconId, wxSize size)
   : wxWindow(parent, id, wxDefaultPosition,
      size == wxDefaultSize ? MinimumSize : size, wxBORDER_NONE)
{
   const auto initialSize = size == wxDefaultSize ? MinimumSize : size;
   SetMinSize(initialSize);
   SetBackgroundStyle(wxBG_STYLE_PAINT);
   SetName(label);
   const auto utf8 = label.utf8_str();
   mView = audacity_alicorn_action_view_create(
      static_cast<float>(GetClientSize().x),
      static_cast<float>(GetClientSize().y),
      utf8.data(),
      1,
      0,
      iconId);
   if (!mView)
      wxLogError(wxT("Could not initialize the Alicorn action button"));

#if wxUSE_ACCESSIBILITY
   SetAccessible(safenew ActionControlAccessible(this));
#endif

   Bind(wxEVT_PAINT, &AlicornActionControl::OnPaint, this);
   Bind(wxEVT_SIZE, &AlicornActionControl::OnSize, this);
   Bind(wxEVT_LEFT_DOWN, &AlicornActionControl::OnLeftDown, this);
   Bind(wxEVT_LEFT_UP, &AlicornActionControl::OnLeftUp, this);
   Bind(wxEVT_MOTION, &AlicornActionControl::OnMotion, this);
   Bind(wxEVT_LEAVE_WINDOW, &AlicornActionControl::OnLeave, this);
   Bind(wxEVT_MOUSE_CAPTURE_LOST, &AlicornActionControl::OnCaptureLost, this);
   Bind(wxEVT_KEY_DOWN, &AlicornActionControl::OnKeyDown, this);
   Bind(wxEVT_SET_FOCUS, &AlicornActionControl::OnSetFocus, this);
}

AlicornActionControl::~AlicornActionControl()
{
   if (mView)
      audacity_alicorn_action_view_destroy(mView);
}

void AlicornActionControl::SetPressed(bool pressed)
{
   if (mPressed == pressed)
      return;
   mPressed = pressed;
   UpdateView();
}

void AlicornActionControl::SetAlicornEnabled(bool enabled)
{
   Enable(enabled);
   UpdateView();
}

void AlicornActionControl::Activate(uint32_t key)
{
   if (!mView || !IsEnabled())
      return;
   SetFocus();
   audacity_alicorn_action_view_key(mView, key);
   UpdateView();
}

void AlicornActionControl::UpdateView()
{
   if (!mView || mUpdating)
      return;

   mUpdating = true;
   uint8_t activated = 0;
   const auto result = audacity_alicorn_action_view_update(
      mView, IsEnabled() ? 1 : 0, mPressed ? 1 : 0, &activated);
   mUpdating = false;
   if (result < 0) {
      wxLogWarning(wxT("Alicorn action button failed to update"));
      return;
   }
   Refresh(false);
   if (activated)
      SendActivation(); // May synchronously close the owning toolbar.
}

int32_t AlicornActionControl::SendPointer(
   uint32_t kind, float x, float y, uint32_t button)
{
   if (!mView)
      return -1;
   const auto result = audacity_alicorn_action_view_pointer(
      mView, kind, x, y, button);
   if (result >= 0)
      UpdateView();
   return result;
}

void AlicornActionControl::SendActivation()
{
   auto* parent = GetParent();
   if (!parent)
      return;
   wxCommandEvent event(wxEVT_BUTTON, GetId());
   event.SetEventObject(this);
   parent->GetEventHandler()->ProcessEvent(event);
}

void AlicornActionControl::OnPaint(wxPaintEvent&)
{
   wxAutoBufferedPaintDC dc(this);
   const auto parent = GetParent();
   dc.SetBackground(wxBrush(parent ? parent->GetBackgroundColour() : GetBackgroundColour()));
   dc.Clear();
   if (!mView)
      return;

   std::unique_ptr<wxGraphicsContext> graphics{ wxGraphicsContext::Create(dc) };
   if (!graphics)
      return;
   AudacityAlicorn::PaintContext context{ graphics.get(), GetFont() };
   if (audacity_alicorn_action_view_paint(
          mView, &AudacityAlicorn::DrawPaintCommand, &context) < 0)
      wxLogWarning(wxT("Alicorn action button could not read its paint list"));
}

void AlicornActionControl::OnSize(wxSizeEvent& event)
{
   if (mView) {
      const auto size = GetClientSize();
      if (audacity_alicorn_action_view_resize(
             mView, static_cast<float>(size.x), static_cast<float>(size.y)) < 0)
         wxLogWarning(wxT("Alicorn action button rejected its new size"));
      UpdateView();
   }
   event.Skip();
}

void AlicornActionControl::OnLeftDown(wxMouseEvent& event)
{
   SetFocus();
   if (!HasCapture())
      CaptureMouse();
   SendPointer(AUDACITY_SNAP_POINTER_DOWN, event.GetX(), event.GetY(),
      AUDACITY_SNAP_BUTTON_PRIMARY);
}

void AlicornActionControl::OnLeftUp(wxMouseEvent& event)
{
   if (!mView)
      return;
   const auto result = audacity_alicorn_action_view_pointer(
      mView, AUDACITY_SNAP_POINTER_UP, event.GetX(), event.GetY(),
      AUDACITY_SNAP_BUTTON_PRIMARY);
   if (HasCapture())
      ReleaseMouse();
   if (result >= 0)
      UpdateView(); // May synchronously open a menu and close this toolbar.
}

void AlicornActionControl::OnMotion(wxMouseEvent& event)
{
   SendPointer(AUDACITY_SNAP_POINTER_MOVE, event.GetX(), event.GetY(),
      AUDACITY_SNAP_BUTTON_PRIMARY);
}

void AlicornActionControl::OnLeave(wxMouseEvent& event)
{
   if (!HasCapture())
      SendPointer(AUDACITY_SNAP_POINTER_MOVE, -1.0f, -1.0f, 0);
   event.Skip();
}

void AlicornActionControl::OnCaptureLost(wxMouseCaptureLostEvent&)
{
   SendPointer(AUDACITY_SNAP_POINTER_CANCEL, -1.0f, -1.0f, 0);
}

void AlicornActionControl::OnKeyDown(wxKeyEvent& event)
{
   if (event.GetKeyCode() == WXK_SPACE) {
      Activate(AUDACITY_SNAP_KEY_SPACE);
      return;
   }
   if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) {
      Activate(AUDACITY_SNAP_KEY_ACTIVATE);
      return;
   }
   if (event.GetKeyCode() == WXK_TAB) {
      event.Skip();
      return;
   }
   event.Skip();
}

void AlicornActionControl::OnSetFocus(wxFocusEvent& event)
{
   if (mView) {
      audacity_alicorn_action_view_key(mView, AUDACITY_SNAP_KEY_TAB);
      UpdateView();
   }
   event.Skip();
}
