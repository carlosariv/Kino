#pragma once

#include "ui.h"

namespace ui {
Box *label(String string);
Signal button(String name);
void line_edit(u8 **buffer, int *pos, int *len, int *capacity, String string);
};
