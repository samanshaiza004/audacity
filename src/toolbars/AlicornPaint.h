/*  SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <wx/font.h>
#include <wx/graphics.h>

#include "AudacitySnapView.h"

namespace AudacityAlicorn {

struct PaintContext {
   wxGraphicsContext* graphics{};
   wxFont font;
};

void DrawPaintCommand(void* context, const AudacitySnapDrawCommand* command);

} // namespace AudacityAlicorn
