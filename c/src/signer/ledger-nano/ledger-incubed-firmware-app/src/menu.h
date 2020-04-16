#pragma once

#include "globals.h"

unsigned char display_text_part();

void ui_idle(void);

void ui_text(void);

void ui_approval(void);

unsigned char io_event(unsigned char channel);
