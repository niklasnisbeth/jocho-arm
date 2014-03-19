#ifndef UI_H
#define UI_H

enum ui_pages {
  UI_PAGE_OP,
  UI_PAGE_AENV,
  UI_PAGE_PENV
};

struct ui_state_t {
  uint16_t selected_voice;
  uint16_t selected_op;
  enum ui_pages page;
};

void ui_button_press ( ui_state_t *ui, uint16_t button );
void ui_encoder_turn ( ui_state_t *ui, uint16_t encoder );
void ui_update_text ( ui_state_t *ui, char[255] textbuf );

#endif
