#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "tui.h"

static struct termios setsave;

/* print string with certain encoding */
static int print_char(char *str)
{
    int tty;
    tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    write(tty, "\033(0", 3);
    write(tty, str, strlen(str));
    write(tty, "\033(B", 3);
    close(tty);
    return 0;
}

/* (x1, y1): upper left, (x2, y2): bottom right */
int tui_draw_box(int x1, int y1, int x2, int y2)
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    int i, j;
    for (i = 0; i < x2; i++) {
        tui_goto_pos(x1 + i, y1);
        if (i == 0) {
            print_char("l"); /* upper left corner */
            for (j = 2; j < y2; j++) {
                print_char("q"); /* horizontal */
            }
            print_char("k"); /* top right corner */
        }
        /* if not the beginning and not the end, then the spaces */
        if (i != 0 && i != x2 - 1) {
            print_char("x"); /* left vertical */
            for (j = 2; j < y2; j++) {
                print_char(" ");
            }
            print_char("x"); /* right vertical */
        }
        if (i == x2 - 1) {
            print_char("m"); /* lower left corner */
            for (j = 2; j < y2; j++)
                print_char("q"); /* horizontal */
            print_char("j");     /* bottom right corner */
        }
    }
    close(tty);
    return 0;
}

static int print_big_char(int mas[],
                          int x,
                          int y,
                          tui_color_t fg,
                          tui_color_t bg)
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    tui_set_fgcolor(fg);
    tui_set_bgcolor(bg);
    tui_goto_pos(x, y);

    int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 32; j++) {
            if (j % 8 == 0)
                tui_goto_pos(x++, y);
            print_char((mas[i] >> j) & 0x1 ? "a" : " ");
        }
    }
    close(tty);
    return 0;
}

/* draw the "big" character */
int tui_draw_big_char(int *big,
                      int x,
                      int y,
                      char value,
                      tui_color_t fg,
                      tui_color_t bg)
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    if (tui_goto_pos(x, y) != -1) {
        switch (value) {
        case 'x':
            big[0] = (int) 1013367747;
            big[1] = (int) 3284362812;
            break;
        case 'o':
            big[0] = (int) 3284386620;
            big[1] = (int) 1019462595;
            break;
        case '*':
            big[0] = 0;
            big[1] = 0;
            break;
        }
    }
    print_big_char(big, x, y, fg, bg);
    close(tty);
    return 0;
}

#define SIZE_KEY_SEQUENCES 15
static char *key_sequences[SIZE_KEY_SEQUENCES];

static int is_supported_key(char *sequence, int *key, int len)
{
    if (!sequence || !key)
        return -1;

    key_sequences[K_UNKNOWN] = "";
    key_sequences[K_UP] = "\033[A";
    key_sequences[K_DOWN] = "\033[B";
    key_sequences[K_LEFT] = "\033[D";
    key_sequences[K_RIGHT] = "\033[C";
    key_sequences[K_ESC] = "\033\033";
    key_sequences[K_ENTER] = "\n";

    for (int i = K_UNKNOWN; i < SIZE_KEY_SEQUENCES; i++) {
        if (strlen(key_sequences[i]) < len)
            continue;
        if (strncmp(sequence, key_sequences[i], len) == 0) {
            if (len < strlen(key_sequences[i]))
                return 1;
            *key = i;
            return 0;
        }
    }
    *key = K_UNKNOWN;
    return 0;
}

#define KEY_MAX_LEN 20

int tui_read_key(int *key)
{
    if (!key)
        return -1;

    int len = 1;
    int res;
    char tmp_key[KEY_MAX_LEN];
    while (read(0, &(tmp_key[len - 1]), 1) > 0 && len <= KEY_MAX_LEN) {
        res = is_supported_key(tmp_key, key, len);
        if (res <= 0)
            return res;
        len++;
    }
    return 0;
}

/* save terminal settings */
int tui_save_term()
{
    if (tcgetattr(0, &setsave) == 0)
        return 0;
    return -1;
}

/* restore saved terminal settings */
int tui_restore_term()
{
    if (tcsetattr(0, TCSANOW, &setsave) == 0)
        return 0;
    return -1;
}

int tui_set_term(int canon, int vtime, int vmin, int echo, int sigint)
{
    if ((canon > 1) || (canon < 0))
        return -1;
    if ((echo > 1) || (echo < 0) || (sigint > 1) || (sigint < 0) ||
        (vtime < 0) || (vmin < 0))
        return -1;

    struct termios setting;
    int result = tcgetattr(0, &setting);
    if (result == -1)
        return -1;

    if (canon == 1) {
        setting.c_lflag |= ICANON;
    } else {
        setting.c_lflag &= ~ICANON;
        setting.c_lflag &= ~ECHO;
        setting.c_lflag &= ~ISIG;
        if (echo == 1)
            setting.c_lflag |= ECHO;
        if (sigint == 1)
            setting.c_lflag |= ISIG;
        setting.c_cc[VTIME] = vtime;
        setting.c_cc[VMIN] = vmin;
    }
    result = tcsetattr(0, TCSANOW, &setting);
    if (result == -1)
        return -1;
    return 0;
}

int tui_clear_screen()
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    write(tty, "\033[H\033[J", 6);
    close(tty);
    return 0;
}

int tui_goto_pos(int row, int col)
{
    int len;
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    if ((row > -1 && row <= TERM_ROW) && (col > -1 && col <= TERM_COL)) {
        char str[20];
        len = sprintf(str, "\033[%d;%dH", row, col);
        write(tty, str, len);
        close(tty);
        return 0;
    }

    fprintf(stderr, "tui_goto_pos: Incorrect row or col value\n");
    close(tty);
    return 1;
}

int tui_set_fgcolor(tui_color_t color)
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    char str[20];
    int len = sprintf(str, "\033[3%dm", color);
    write(tty, str, len);
    write(tty, "\033[1m", 4);
    close(tty);
    return 0;
}

int tui_set_bgcolor(tui_color_t color)
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    char str[20];
    int len = sprintf(str, "\033[4%dm", color);
    write(tty, str, len);
    close(tty);
    return 0;
}

int tui_set_stdcolor()
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }

    write(tty, "\033[0m", 5);
    return 0;
}

int tui_set_cursor(bool visible)
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        close(tty);
        return -1;
    }
    char *buf = visible ? "\033[?12;25h" : "\033[?25l";
    if (write(tty, buf, strlen(buf)) != strlen(buf))
        return -1;
    return 0;
}
