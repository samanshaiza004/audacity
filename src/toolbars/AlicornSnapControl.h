/*  SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <functional>
#include <stdint.h>
#include <utility>

#include <wx/window.h>

#include "AudacitySnapView.h"

class AlicornSnapControl final : public wxWindow {
public:
   using ChangeHandler = std::function<void(bool)>;

   explicit AlicornSnapControl(wxWindow* parent);
   ~AlicornSnapControl() override;

   bool IsChecked() const noexcept { return mChecked; }
   void SetChecked(bool checked);
   void SetChangeHandler(ChangeHandler handler) { mChangeHandler = std::move(handler); }

   // Used by the platform accessibility action as well as keyboard input.
   void Activate();

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
   void PublishChange(bool checked);

   AudacityAlicornSnapView* mView{};
   bool mChecked{};
   bool mUpdating{};
   ChangeHandler mChangeHandler;
};
