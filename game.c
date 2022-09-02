#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "tui.h"

typedef enum { NOUGHT, CROSS, EMPTY, FULL, DRAW } status_t;

static status_t game_field[3][3] = {
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
};

static bool is_finished = false;
static status_t prev_move = EMPTY;

static pthread_mutex_t game_mutex;

static void computer_move(status_t player);

static void *computer_func(void *arg)
{
    while (1) {
        usleep(200 * 1000);
        computer_move(CROSS);
    }
    return 0;
}

static int play(status_t s[3][3])
{
    /*             case7               case8
     *                 \   c4   c5  c6   /
     *                  \  ||   ||  ||  /
     *                   \ \/   \/  \/ /
     *   case 1 ----->    | x | o | x |
     *   case 2 ----->    | o | x | o |
     *   case 3 ----->    | o | x | x |
     */

    /* case 1, 2, 3 */
    if (s[0][0] == NOUGHT && s[0][1] == NOUGHT && s[0][2] == NOUGHT)
        return NOUGHT;
    if (s[1][0] == NOUGHT && s[1][1] == NOUGHT && s[1][2] == NOUGHT)
        return NOUGHT;
    if (s[2][0] == NOUGHT && s[2][1] == NOUGHT && s[2][2] == NOUGHT)
        return NOUGHT;

    /* case 4, 5 , 6 */
    if (s[0][0] == NOUGHT && s[1][0] == NOUGHT && s[2][0] == NOUGHT)
        return NOUGHT;
    if (s[0][1] == NOUGHT && s[1][1] == NOUGHT && s[2][1] == NOUGHT)
        return NOUGHT;
    if (s[0][2] == NOUGHT && s[1][2] == NOUGHT && s[2][2] == NOUGHT)
        return NOUGHT;

    /* case 7, 8 */
    if (s[0][0] == NOUGHT && s[1][1] == NOUGHT && s[2][2] == NOUGHT)
        return NOUGHT;
    if (s[0][2] == NOUGHT && s[1][1] == NOUGHT && s[2][0] == NOUGHT)
        return NOUGHT;

    /* case 1, 2, 3 */
    if (s[0][0] == CROSS && s[0][1] == CROSS && s[0][2] == CROSS)
        return CROSS;
    if (s[1][0] == CROSS && s[1][1] == CROSS && s[1][2] == CROSS)
        return CROSS;
    if (s[2][0] == CROSS && s[2][1] == CROSS && s[2][2] == CROSS)
        return CROSS;

    /* case 4, 5, 6 */
    if (s[0][0] == CROSS && s[1][0] == CROSS && s[2][0] == CROSS)
        return CROSS;
    if (s[0][1] == CROSS && s[1][1] == CROSS && s[2][1] == CROSS)
        return CROSS;
    if (s[0][2] == CROSS && s[1][2] == CROSS && s[2][2] == CROSS)
        return CROSS;

    /* case 7, 8 */
    if (s[0][0] == CROSS && s[1][1] == CROSS && s[2][2] == CROSS)
        return CROSS;
    if (s[0][2] == CROSS && s[1][1] == CROSS && s[2][0] == CROSS)
        return CROSS;

    /* end in a draw */
    if (s[0][0] != EMPTY && s[0][1] != EMPTY && s[0][2] != EMPTY &&
        s[1][0] != EMPTY && s[1][1] != EMPTY && s[1][2] != EMPTY &&
        s[2][0] != EMPTY && s[2][1] != EMPTY && s[2][2] != EMPTY) {
        return DRAW;
    }

    return EMPTY;
}

static bool game_over(status_t winner)
{
    if (winner == EMPTY)
        return false;

    tui_set_stdcolor();
    tui_clear_screen();
    tui_set_bgcolor(cBlack);
    tui_set_fgcolor(cGreen);
    tui_goto_pos(15, 17);

    switch (winner) {
    case NOUGHT:
        write(1, "Nought(O) won", 14);
        return true;

    case CROSS:
        write(1, "Cross(X) won", 13);
        return true;

    case DRAW:
        write(1, "Played in Draw", 15);
        return true;
    default:
	return false;
    }
}

static void update_game_field(int row, int col)
{
    int big[2];

    /* 1: nought, 2: cross */
    for (int i = 0, xpos = 5; i < 3; i++, xpos = xpos + 15) {
        if (row == 0 && col == i) {
            if (game_field[0][i] == NOUGHT)
                tui_draw_big_char(big, 3, xpos, 'o', cBlack, cGreen);
            if (game_field[0][i] == CROSS)
                tui_draw_big_char(big, 3, xpos, 'x', cBlack, cGreen);
            if (game_field[0][i] == EMPTY)
                tui_draw_big_char(big, 3, xpos, '*', cBlack, cGreen);
        } else {
            if (game_field[0][i] == NOUGHT)
                tui_draw_big_char(big, 3, xpos, 'o', cGreen, cBlack);
            if (game_field[0][i] == CROSS)
                tui_draw_big_char(big, 3, xpos, 'x', cGreen, cBlack);
            if (game_field[0][i] == EMPTY)
                tui_draw_big_char(big, 3, xpos, '*', cGreen, cBlack);
        }
    }

    for (int i = 0, xpos = 5; i < 3; i++, xpos = xpos + 15) {
        /* select the desired cell */
        if (row == 1 && col == i) {
            if (game_field[1][i] == NOUGHT)
                tui_draw_big_char(big, 13, xpos, 'o', cBlack, cGreen);
            if (game_field[1][i] == CROSS)
                tui_draw_big_char(big, 13, xpos, 'x', cBlack, cGreen);
            if (game_field[1][i] == EMPTY)
                tui_draw_big_char(big, 13, xpos, '*', cBlack, cGreen);
        } else {
            if (game_field[1][i] == NOUGHT)
                tui_draw_big_char(big, 13, xpos, 'o', cGreen, cBlack);
            if (game_field[1][i] == CROSS)
                tui_draw_big_char(big, 13, xpos, 'x', cGreen, cBlack);
            if (game_field[1][i] == EMPTY)
                tui_draw_big_char(big, 13, xpos, '*', cGreen, cBlack);
        }
    }

    for (int i = 0, xpos = 5; i < 3; i++, xpos = xpos + 15) {
        if (row == 2 && col == i) {
            if (game_field[2][i] == NOUGHT)
                tui_draw_big_char(big, 23, xpos, 'o', cBlack, cGreen);
            if (game_field[2][i] == CROSS)
                tui_draw_big_char(big, 23, xpos, 'x', cBlack, cGreen);
            if (game_field[2][i] == EMPTY)
                tui_draw_big_char(big, 23, xpos, '*', cBlack, cGreen);
        } else {
            if (game_field[2][i] == NOUGHT)
                tui_draw_big_char(big, 23, xpos, 'o', cGreen, cBlack);
            if (game_field[2][i] == CROSS)
                tui_draw_big_char(big, 23, xpos, 'x', cGreen, cBlack);
            if (game_field[2][i] == EMPTY)
                tui_draw_big_char(big, 23, xpos, '*', cGreen, cBlack);
        }
    }
}

static int human_move(int row, int col, status_t player)
{
    pthread_mutex_lock(&game_mutex);

    if (game_field[row][col] == EMPTY && prev_move != player) {
        prev_move = player;
        game_field[row][col] = player; /* put nought(O) */
        update_game_field(row, col);

        /* check if it time to game over */
        is_finished = game_over(play(game_field));
    }

    pthread_mutex_unlock(&game_mutex);
    return 0;
}

static void computer_move(status_t player)
{
    pthread_mutex_lock(&game_mutex);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (game_field[i][j] == EMPTY && prev_move != player) {
                prev_move = player;
                game_field[i][j] = player; /* put nought(O) */
                update_game_field(i, j);
                pthread_mutex_unlock(&game_mutex);
                return;
            }
        }
    }

    is_finished = game_over(play(game_field));
    pthread_mutex_unlock(&game_mutex);
}

static int main_screen()
{
    int tty = open("/dev/tty", O_RDWR);
    if (tty == -1) {
        fprintf(stderr, "Can not open tty\n");
        close(tty);
        return -1;
    }

    tui_clear_screen();
    tui_set_bgcolor(cBlack);
    tui_set_fgcolor(cAqua);

    /* field frame */
    tui_draw_box(1, 1, 32, 46);

    /* firt row */
    tui_draw_box(2, 3, 10, 12);
    tui_draw_box(12, 3, 10, 12);
    tui_draw_box(22, 3, 10, 12);

    /* second row */
    tui_draw_box(2, 18, 10, 12);
    tui_draw_box(12, 18, 10, 12);
    tui_draw_box(22, 18, 10, 12);

    /* third row */
    tui_draw_box(2, 33, 10, 12);
    tui_draw_box(12, 33, 10, 12);
    tui_draw_box(22, 33, 10, 12);

    tui_draw_box(33, 2, 4, 45);

    /* top */
    tui_goto_pos(1, 17);
    write(tty, " Tic-Tac-Toe ", 13);

    /* information */
    tui_goto_pos(33, /*67*/ 4);
    write(tty, " HELP ", 6);

    /* keys */
    tui_goto_pos(34, 5);
    write(tty, "Arrow Keys - cursor movement ", 30);
    tui_goto_pos(35, 5);
    write(tty, "Enter - put ", 13);
    tui_goto_pos(35, 25);
    write(tty, "ESC - quit    ", 15);

    return 0;
}

static int game_session()
{
    pthread_t computer;
    int row = 0, col = 0;

    main_screen();
    update_game_field(0, 0);

    tui_save_term();
    tui_set_term(0, 0, 1, 0, 1);
    tui_set_cursor(0); /* hide cursor */

    /* color palete */
    tui_set_bgcolor(cBlack);
    tui_set_fgcolor(cGreen);

    int unused;
    pthread_create(&computer, 0, computer_func, (void *) &unused);

    int key;
    /* FIXME: we shall check the valid range of row and col */
    while (!is_finished && tui_read_key(&key) == 0 && key != K_ESC) {
        switch (key) {
        case K_UP:
            if (row != 0) {
                row--;
                update_game_field(row, col);
            }
            break;
        case K_DOWN:
            if (row < 2) {
                row++;
                update_game_field(row, col);
            }
            break;
        case K_LEFT:
            if (col != 0) {
                col--;
                update_game_field(row, col);
            }
            break;
        case K_RIGHT:
            if (col < 2) {
                col++;
                update_game_field(row, col);
            }
            break;
        case K_ENTER:
            human_move(row, col, NOUGHT);
            break;
        }
    }

    pthread_cancel(computer);

    tui_restore_term();
    tui_set_cursor(1);
    tui_set_stdcolor();
    tui_goto_pos(39, 1);

    return 0;
}

int main()
{
    return game_session();
}
