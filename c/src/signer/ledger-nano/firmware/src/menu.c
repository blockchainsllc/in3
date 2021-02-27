#include "menu.h"

#include "operations.h"
#include "ui.h"

unsigned int  current_text_pos; // parsing cursor in the text to display
unsigned int  text_y;
enum UI_STATE uiState;
char          lineBuffer[100];

unsigned char display_text_part() {
  PRINTF("display_text_part: enter\n");
  unsigned int i;

  // WIDE char* text = (char*) G_io_apdu_buffer + 5;
  // PRINTF("display_text_part: text pointer %d %d\n", G_io_apdu_buffer[4], current_text_pos);
  // if (text[current_text_pos] == '\0') {
  //   PRINTF("display_text_part: returning null\n");
  //   return 0;
  // }
  // i = 0;
  // while ((text[current_text_pos] != 0) && (text[current_text_pos] != '\n') &&
  //        (i < MAX_CHARS_PER_LINE)) {
  //   PRINTF("display_text_part: counter %d \n", current_text_pos);
  //   lineBuffer[i++] = text[current_text_pos];
  //   current_text_pos++;
  // }
  // if (text[current_text_pos] == '\n') {
  //   current_text_pos++;
  // }
  tohex(msg_hash, HASH_LEN, lineBuffer, sizeof(lineBuffer));
  current_text_pos             = HASH_LEN * 2;
  lineBuffer[current_text_pos] = NULL;
#ifdef TARGET_BLUE
  os_memset(bagl_ui_text, 0, sizeof(bagl_ui_text));
  bagl_ui_text[0].component.type   = BAGL_LABEL;
  bagl_ui_text[0].component.x      = 4;
  bagl_ui_text[0].component.y      = text_y;
  bagl_ui_text[0].component.width  = 320;
  bagl_ui_text[0].component.height = TEXT_HEIGHT;

  bagl_ui_text[0].component.fgcolor = 0x000000;
  bagl_ui_text[0].component.bgcolor = 0xf9f9f9;
  bagl_ui_text[0].component.font_id = DEFAULT_FONT;
  bagl_ui_text[0].text              = lineBuffer;
  text_y += TEXT_HEIGHT + TEXT_SPACE;
#endif

  PRINTF("display_text_part:exit");
  return 0;
}

void ui_idle(void) {
  uiState = UI_IDLE;
#ifdef TARGET_BLUE
  UX_DISPLAY(bagl_ui_idle_blue, NULL);
#else
  UX_DISPLAY(bagl_ui_idle_nanos, NULL);
#endif
}

void ui_text(void) {
  uiState = UI_TEXT;
#ifdef TARGET_BLUE
  UX_DISPLAY(bagl_ui_text, NULL);
#else
  UX_DISPLAY(bagl_ui_text_review_nanos, NULL);
#endif
}

void ui_approval(void) {
  uiState = UI_APPROVAL;
#ifdef TARGET_BLUE
  UX_DISPLAY(bagl_ui_approval_blue, NULL);
#else
  UX_DISPLAY(bagl_ui_approval_nanos, NULL);
#endif
}

unsigned char io_event(unsigned char channel) {
  // nothing done with the event, throw an error on the transport layer if
  // needed

  // can't have more than one tag in the reply,        //PRINTF ("io_event:enter"); not supported yet.
  switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
      UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
      break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
      UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
      break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
      if ((uiState == UI_TEXT) &&
          (os_seph_features() &
           SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG)) {
        if (!display_text_part()) {
          //PRINTF ("io_event:display_text_part");
          ui_approval();
        }
        else {
          UX_REDISPLAY();
        }
      }
      else {
        UX_DISPLAYED_EVENT();
      }
      break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
#ifdef TARGET_NANOS
      UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
        // defaulty retrig very soon (will be overriden during
        // stepper_prepro)
        UX_CALLBACK_SET_INTERVAL(500);
        UX_REDISPLAY();
      });
#endif
      break;

    // unknown events are acknowledged
    default:
      UX_DEFAULT_EVENT();
      break;
  }

  // close the event if not done previously (by a display or whatever)
  if (!io_seproxyhal_spi_is_status_sent()) {
    io_seproxyhal_general_status();
  }

  // command has been processed, DO NOT reset the current APDU transport
  return 1;
}
