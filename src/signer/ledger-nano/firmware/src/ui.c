#include "ui.h"

#include "menu.h"
#include "operations.h"

unsigned int
bagl_ui_approval_blue_button(unsigned int button_mask,
                             unsigned int button_mask_counter) {
  return 0;
}

unsigned int
bagl_ui_idle_blue_button(unsigned int button_mask,
                         unsigned int button_mask_counter) {
  return 0;
}

unsigned int
bagl_ui_text_button(unsigned int button_mask,
                    unsigned int button_mask_counter) {
  return 0;
}

unsigned int
bagl_ui_idle_nanos_button(unsigned int button_mask,
                          unsigned int button_mask_counter) {
  switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
    case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
      io_seproxyhal_touch_exit(NULL);
      break;
  }

  return 0;
}

unsigned int
bagl_ui_approval_nanos_button(unsigned int button_mask,
                              unsigned int button_mask_counter) {
  switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
      io_seproxyhal_touch_approve(NULL);
      break;

    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
      io_seproxyhal_touch_deny(NULL);
      break;
  }
  return 0;
}

unsigned int
bagl_ui_text_review_nanos_button(unsigned int button_mask,
                                 unsigned int button_mask_counter) {
  switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
      if (!display_text_part()) {
        ui_approval();
      }
      else {
        UX_REDISPLAY();
      }
      break;

    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
      io_seproxyhal_touch_deny(NULL);
      break;
  }
  return 0;
}
