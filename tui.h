#ifndef TUI_H
#define TUI_H

#include <stdbool.h>

enum keys {
    K_UNKNOWN,
    K_UP,
    K_DOWN,
    K_LEFT,
    K_RIGHT,
    K_ESC,
    K_ENTER,
    SIZE_KEY_SEQUENCES
};

int tui_read_key(int *key);
int tui_save_term();
int tui_restore_term();
int tui_set_term(int canon, int vtime, int vmin, int echo, int sigint);

/* console size */
#define TERM_ROW 44
#define TERM_COL 168

typedef enum {
    cBlack = 0,
    cRed,
    cGreen,
    cYellow,
    cBlue,
    cPurple,
    cAqua,
    cWhite
} tui_color_t;

int tui_clear_screen();
int tui_goto_pos(int row, int column);
int tui_set_fgcolor(tui_color_t color);
int tui_set_bgcolor(tui_color_t color);
int tui_set_stdcolor();
int tui_set_cursor(bool visible);

/* simple operations */
int tui_draw_box(int x1, int y1, int x2, int y2);
int tui_draw_big_char(int *big,
                      int x,
                      int y,
                      char value,
                      tui_color_t fg,
                      tui_color_t bg);

#endif
