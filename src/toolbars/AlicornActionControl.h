/*  SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <wx/window.h>

#include "AudacitySnapView.h"

class AlicornActionControl final : public wxWindow {
public:
   AlicornActionControl(
      wxWindow* parent, wxWindowID id, const wxString& label,
      uint32_t iconId = AUDACITY_ACTION_ICON_NONE,
      wxSize size = wxDefaultSize);
   ~AlicornActionControl() override;

   bool IsPressed() const noexcept { return mPressed; }
   void SetPressed(bool pressed);
   void SetAlicornEnabled(bool enabled);
   bool Enable(bool enabled = true) override;
   void Activate(uint32_t key = AUDACITY_SNAP_KEY_ACTIVATE);

private:
   void OnPaint(wxPaintEvent& event);
   void OnSize(wxSizeEvent& event);
   void OnLeftDown(wxMouseEvent& event);
   void OnLeftUp(wxMouseEvent& event);
   void OnMotion(wxMouseEvent& event);
   void OnLeave(wxMouseEvent& event);
   void OnCaptureLost(wxMouseCaptureLostEvent& event);
   void OnKeyDown(wxKeyEvent& event);
   void OnSetFocus(wxFocusEvent& event);

   void UpdateView();
   int32_t SendPointer(uint32_t kind, float x, float y, uint32_t button);
   void SendActivation();

   AudacityAlicornActionView* mView{};
   bool mPressed{};
   bool mUpdating{};
};
