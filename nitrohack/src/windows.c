/* Copyright (c) Daniel Thaler, 2011.                             */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <signal.h>
#include <locale.h>

#if !defined(PDCURSES)
/*
 * _nc_unicode_locale(): return true, if the locale supports unicode
 * ncurses has _nc_unicode_locale(), but it is not part of the curses API.
 * For portability this function should probably be reimplemented.
 */
extern int _nc_unicode_locale(void);
#else
# define set_escdelay(x)
# define _nc_unicode_locale() (1) /* ... as a macro, for example ... */
#endif

WINDOW *basewin, *mapwin, *msgwin, *statuswin, *sidebar;
struct gamewin *firstgw, *lastgw;
int orig_cursor;
const char quit_chars[] = " \r\n\033";
static SCREEN *curses_scr;

struct nh_window_procs curses_windowprocs = {
    curses_pause,
    curses_display_buffer,
    curses_update_status,
    curses_print_message,
    curses_display_menu,
    curses_display_objects,
    curses_list_items,
    curses_update_screen,
    curses_raw_print,
    curses_query_key,
    curses_getpos,
    curses_getdir,
    curses_yn_function,
    curses_getline,
    curses_delay_output,
    curses_notify_level_changed,
    curses_outrip,
    curses_print_message_nonblocking,
};

/*----------------------------------------------------------------------------*/

static void define_extra_keypad_keys(void)
{
#if defined(NCURSES_VERSION)
    /* Recognize application keypad mode escape sequences, enabled by default
     * in PuTTY, so numpad keys work out-of-the-box for it. */
    define_key("\033Oq", KEY_C1);	/* numpad 1 */
    define_key("\033Or", KEY_DOWN);	/* numpad 2 */
    define_key("\033Os", KEY_C3);	/* numpad 3 */
    define_key("\033Ot", KEY_LEFT);	/* numpad 4 */
    define_key("\033Ou", KEY_B2);	/* numpad 5 */
    define_key("\033Ov", KEY_RIGHT);	/* numpad 6 */
    define_key("\033Ow", KEY_A1);	/* numpad 7 */
    define_key("\033Ox", KEY_UP);	/* numpad 8 */
    define_key("\033Oy", KEY_A3);	/* numpad 9 */

    /* These numpad keys don't have Curses KEY_* constants to map to. */
    define_key("\033Ol", (int)'+');	/* numpad + */
    define_key("\033On", (int)'.');	/* numpad . */
    define_key("\033Op", (int)'0');	/* numpad 0 */

    /* Quirk: In PuTTY's default config, the Num Lock, '/', '*' and '-' numpad
     * keys masquerade as F1, F2, F3 and F4 respectively, while the F1, F2, F3
     * and F4 keys emit unrecognized escape codes.
     *
     * In theory these numpad keys could be unmapped and have those codes given
     * to the F1-4 keys, but in practice this breaks F1-4 for terminals that
     * correctly map F1-4.  Instead, we'll compromise and map F1-4 without
     * unmapping those numpad keys; we won't be able to tell them apart, but at
     * least F1-4 will map to *something*. */
    define_key("\033[11~", KEY_F(1));
    define_key("\033[12~", KEY_F(2));
    define_key("\033[13~", KEY_F(3));
    define_key("\033[14~", KEY_F(4));

    /* There are probably more keys not listed here that don't work right
     * out-of-the-box with PuTTY... */
#endif
}


void init_curses_ui(void)
{
    /* set up the default system locale by reading the environment variables */
    setlocale(LC_ALL, "");

    curses_scr = newterm(NULL, stdout, stdin);
    set_term(curses_scr);

    if (LINES < 24 || COLS < COLNO) {
	fprintf(stderr, "Sorry, your terminal is too small for DynaHack. Current: (%x, %x)\n", COLS, LINES);
	endwin();
	exit(0);
    }

    noecho();
    raw();
    nonl();
    meta(basewin, TRUE);
    leaveok(basewin, TRUE);
    orig_cursor = curs_set(1);
    keypad(basewin, TRUE);
    define_extra_keypad_keys();
    set_escdelay(20);

    init_nhcolors();
    ui_flags.playmode = MODE_NORMAL;
    ui_flags.unicode = _nc_unicode_locale();

    /* with PDCurses/Win32 stdscr is not NULL before newterm runs, which caused
     * crashes. So basewin is a copy of stdscr which is known to be NULL before
     * curses is inited. */
    basewin = stdscr;

#if defined(PDCURSES)
    PDC_set_title("DynaHack");
#if defined(WIN32)
    /* Force the console to use codepage 437. This seems to be the default
     * on european windows, but not on asian systems. Aparrently there is no
     * such thing as a Unicode console in windows (EPIC FAIL!) and all output
     * characters are always transformed according to a code page. */
    SetConsoleOutputCP(437);
    if (settings.win_height > 0 && settings.win_width > 0)
	resize_term(settings.win_height, settings.win_width);
#endif
#endif
}


void exit_curses_ui(void)
{
    cleanup_sidebar(TRUE);
    curs_set(orig_cursor);
    endwin();
    delscreen(curses_scr);
    basewin = NULL;

#ifdef UNIX
    /* Force the cursor to be visible.  In theory, the curs_set() call above
     * should do that.  In practice, something about the game running in
     * dgamelaunch prevents this from happening properly. */
    write(1, "\033[?25h", 6);
#endif
}


attr_t frame_hp_color(void)
{
    int color = -1;
    attr_t attr = A_NORMAL;

    if (settings.frame_hp_color && ui_flags.ingame && player.x) {
	int percent = 100;
	if (player.hpmax > 0)
	    percent = 100 * player.hp / player.hpmax;

	if (percent < 25) color = CLR_RED;
	else if (percent < 50) color = CLR_ORANGE;
	else if (percent < 75) color = CLR_YELLOW;

	if (color != -1)
	    attr = curses_color_attr(color, 0);
    }

    return attr;
}


static void draw_frame(void)
{
    attr_t frame_attr;

    if (!ui_flags.draw_frame)
	return;

    frame_attr = frame_hp_color();

    /* vertical lines */
    nh_box_mvwvline(basewin, 1, 0, ui_flags.viewheight, frame_attr);
    nh_box_mvwvline(basewin, 1, COLNO + 1, ui_flags.viewheight, frame_attr);

    /* horizontal top line above the message win */
    nh_box_mvwadd_ulcorner(basewin, 0, 0, frame_attr);
    nh_box_whline(basewin, COLNO, frame_attr);
    nh_box_mvwadd_urcorner(basewin, 0, COLNO + 1, frame_attr);

    /* horizontal line between message and map windows */
    nh_box_mvwadd_ltee(basewin, 1 + ui_flags.msgheight, 0, frame_attr);
    nh_box_whline(basewin, COLNO, frame_attr);
    nh_box_mvwadd_rtee(basewin, 1 + ui_flags.msgheight, COLNO + 1, frame_attr);

    /* horizontal line between map and status */
    nh_box_mvwadd_ltee(basewin, 2 + ui_flags.msgheight + ROWNO, 0, frame_attr);
    nh_box_whline(basewin, COLNO, frame_attr);
    nh_box_mvwadd_rtee(basewin, 2 + ui_flags.msgheight + ROWNO, COLNO + 1, frame_attr);

    /* horizontal bottom line */
    nh_box_mvwadd_llcorner(basewin, ui_flags.viewheight + 1, 0, frame_attr);
    nh_box_whline(basewin, COLNO, frame_attr);
    nh_box_mvwadd_lrcorner(basewin, ui_flags.viewheight + 1, COLNO + 1, frame_attr);

    if (ui_flags.draw_sidebar) {
	nh_box_mvwadd_ttee(basewin, 0, COLNO + 1, frame_attr);
	nh_box_whline(basewin, COLS - COLNO - 3, frame_attr);
	nh_box_mvwadd_urcorner(basewin, 0, COLS - 1, frame_attr);

	nh_box_mvwadd_btee(basewin, ui_flags.viewheight + 1, COLNO + 1, frame_attr);
	nh_box_whline(basewin, COLS - COLNO - 3, frame_attr);
	nh_box_mvwadd_lrcorner(basewin, ui_flags.viewheight + 1, COLS - 1, frame_attr);

	nh_box_mvwvline(basewin, 1, COLS - 1, ui_flags.viewheight, frame_attr);
    }
}


void redraw_frame(void)
{
    struct gamewin *gw;

    if (!ui_flags.draw_frame || !ui_flags.ingame)
	return;

    draw_frame();
    wnoutrefresh(basewin);

    /* The order of the following is based on redraw_game_windows(). */
    touchwin(mapwin);
    touchwin(msgwin);
    wnoutrefresh(mapwin);
    wnoutrefresh(msgwin);
    if (statuswin) {
	touchwin(statuswin);
	wnoutrefresh(statuswin);
    }
    if (sidebar) {
	draw_sidebar_divider();
	touchwin(sidebar);
	wnoutrefresh(sidebar);
    }

    for (gw = firstgw; gw; gw = gw->next) {
	touchwin(gw->win);
	wnoutrefresh(gw->win);
    }
}


static void layout_game_windows(void)
{
    int statusheight, msgheight;
    ui_flags.draw_frame = ui_flags.draw_sidebar = FALSE;
    statusheight = settings.status3 ? 3 : 2;
    const int frame_hlines = 4;

    /* 3 variable elements contribute to height: 
     *  - message area (most important)
     *  - better status
     *  - horizontal frame lines (least important)
     */
    if (settings.msgheight == 0) {
	/* auto msgheight */
	if (settings.frame && COLS >= COLNO + 2 &&
	    LINES >= ROWNO + frame_hlines + 1 /* min msgheight */ + statusheight)
	    ui_flags.draw_frame = TRUE;
	msgheight = LINES - (ROWNO + statusheight);
	if (ui_flags.draw_frame) msgheight -= frame_hlines;
	if (msgheight < 1) msgheight = 1;
	if (msgheight > MAX_MSGLINES) msgheight = MAX_MSGLINES;
    } else {
	/* explicit msgheight */
	msgheight = settings.msgheight;
	if (settings.frame && COLS >= COLNO + 2 &&
	    LINES >= ROWNO + frame_hlines + msgheight + statusheight)
	    ui_flags.draw_frame = TRUE;
    }

    if (settings.sidebar && COLS >= COLNO + 20)
	ui_flags.draw_sidebar = TRUE;
    
    /* create subwindows */
    if (ui_flags.draw_frame) {
	ui_flags.msgheight = msgheight;
	ui_flags.status3 = settings.status3;
	ui_flags.viewheight = ui_flags.msgheight + ROWNO + statusheight + 2;
    } else {
	int spare_lines = LINES - ROWNO - 2;
	spare_lines = spare_lines >= 1 ? spare_lines : 1;
	ui_flags.msgheight = min(msgheight, spare_lines);
	if (ui_flags.msgheight < spare_lines)
	    ui_flags.status3 = settings.status3;
	else {
	    ui_flags.status3 = FALSE;
	    statusheight = 2;
	}
	    
	ui_flags.viewheight = ui_flags.msgheight + ROWNO + statusheight;
    }
}


void create_game_windows(void)
{
    int statusheight;

    layout_game_windows();
    statusheight = ui_flags.status3 ? 3 : 2;
    
    werase(basewin);
    if (ui_flags.draw_frame) {
	msgwin = newwin(ui_flags.msgheight, COLNO, 1, 1);
	mapwin = newwin(ROWNO, COLNO, ui_flags.msgheight + 2, 1);
	statuswin = derwin(basewin, statusheight, COLNO,
			   ui_flags.msgheight + ROWNO + 3, 1);
	
	if (ui_flags.draw_sidebar)
	    sidebar = derwin(basewin, ui_flags.viewheight, COLS - COLNO - 3, 1, COLNO+2);
	
	draw_frame();
    } else {
	msgwin = newwin(ui_flags.msgheight, COLNO, 0, 0);
	mapwin = newwin(ROWNO, COLNO, ui_flags.msgheight, 0);
	statuswin = derwin(basewin, statusheight, COLNO, ui_flags.msgheight + ROWNO, 0);
	
	if (ui_flags.draw_sidebar)
	    sidebar = derwin(basewin, ui_flags.viewheight, COLS - COLNO, 0, COLNO);
    }
    
    keypad(mapwin, TRUE);
    keypad(msgwin, TRUE);
    leaveok(mapwin, FALSE);
    leaveok(msgwin, FALSE);
    leaveok(statuswin, TRUE);
    if (sidebar)
	leaveok(sidebar, TRUE);
    
    ui_flags.ingame = TRUE;
    redraw_game_windows();
}


static void resize_game_windows(void)
{
    int statusheight = ui_flags.status3 ? 3 : 2;
    
    layout_game_windows();
    
    if (!ui_flags.ingame)
	return;
    
    /* statuswin and sidebar never accept input, so simply recreating those is
     * easiest. */
    delwin(statuswin);
    if (sidebar) {
	delwin(sidebar);
	sidebar = NULL;
    }
    
    /* ncurses might have automatically changed the window sizes in resizeterm
     * while trying to do the right thing. Of course no size other than
     * COLNO x ROWNO is ever right for the map... */
    wresize(msgwin, ui_flags.msgheight, COLNO);
    wresize(mapwin, ROWNO, COLNO);
    
    if (ui_flags.draw_frame) {
	mvwin(msgwin, 1, 1);
	mvwin(mapwin, ui_flags.msgheight + 2, 1);
	statuswin = derwin(basewin, statusheight, COLNO,
			   ui_flags.msgheight + ROWNO + 3, 1);
	
	if (ui_flags.draw_sidebar)
	    sidebar = derwin(basewin, ui_flags.viewheight, COLS - COLNO - 3, 1, COLNO+2);
	draw_frame();
    } else {
	mvwin(msgwin, 0, 0);
	mvwin(mapwin, ui_flags.msgheight, 0);
	statuswin = derwin(basewin, statusheight, COLNO, ui_flags.msgheight + ROWNO, 0);
	
	if (ui_flags.draw_sidebar)
	    sidebar = derwin(basewin, ui_flags.viewheight, COLS - COLNO, 0, COLNO);
    }
    
    leaveok(statuswin, TRUE);
    if (sidebar)
	leaveok(sidebar, TRUE);
    
    redraw_game_windows();
    doupdate();
}


void destroy_game_windows(void)
{
    if (ui_flags.ingame) {
	delwin(msgwin);
	delwin(mapwin);
	delwin(statuswin);
	if (sidebar || ui_flags.draw_sidebar) {
	    cleanup_sidebar(FALSE);
	    delwin(sidebar);
	}
	msgwin = mapwin = statuswin = sidebar = NULL;
    }
    
    ui_flags.ingame = FALSE;
}


void redraw_game_windows(void)
{
    struct gamewin *gw;
    
    redrawwin(basewin);
    wnoutrefresh(basewin);
    
    if (ui_flags.ingame) {
	redrawwin(mapwin);
	redrawwin(msgwin);
	
	wnoutrefresh(mapwin);
	wnoutrefresh(msgwin);
	
	/* statuswin can become NULL if the terminal is resized to microscopic dimensions */
	if (statuswin) {
	    redrawwin(statuswin);
	    wnoutrefresh(statuswin);
	}
	
	if (sidebar) {
	    draw_sidebar_divider();
	    redrawwin(sidebar);
	    wnoutrefresh(sidebar);
	}
	
	draw_frame();
    }
    
    for (gw = firstgw; gw; gw = gw->next) {
	gw->draw(gw);
	redrawwin(gw->win);
	wnoutrefresh(gw->win);
    }
}


void rebuild_ui(void)
{
    if (ui_flags.ingame) {
	wclear(basewin);
	resize_game_windows();
	
	/* some windows are now empty because they were re-created */
	draw_msgwin();
	draw_map(player.x, player.y);
	curses_update_status(&player);
	draw_sidebar();
	
	redraw_game_windows();
    } else if (basewin) {
	redrawwin(basewin);
	wrefresh(basewin);
    }
}


void handle_resize(void)
{
    struct gamewin *gw;

    for (gw = firstgw; gw; gw = gw->next) {
	if (!gw->resize)
	    continue;

	gw->resize(gw);
	redrawwin(gw->win);
	wnoutrefresh(gw->win);
    }

    rebuild_ui();
    doupdate();
}


#define META(c)  ((c)|0x80) /* bit 8 */
int nh_wgetch(WINDOW *win)
{
    int key = 0;

    doupdate(); /* required by pdcurses, noop for ncurses */
    do {
	key = wgetch(win);

	if (key == ERR) {
	    /* player disconnected so there's no input stream and the handler
	     * for SIGHUP is on a coffee-break, so just "crash" the game */
	    exit(2);
	}

#ifdef UNIX
	if (key == 0x3 && ui_flags.playmode == MODE_WIZARD) {
	    /* we're running in raw mode, so ctrl+c doesn't work.
	     * for wizard we emulate this to allow breaking into gdb. */
	    kill(0, SIGINT);
	    key = 0;
	}
#endif

	if (key == KEY_RESIZE) {
	    key = 0;
#ifdef PDCURSES
	    resize_term(0, 0);
#endif
	    handle_resize();
	}

#if !defined(WIN32)
	/* "hackaround": some terminals / shells / whatever don't directly pass
	 * on any combinations with the alt key. Instead these become ESC,<key>
	 * Try to reverse that here...
	 */
	if (key == KEY_ESC) {
	    int key2;

	    nodelay(win, TRUE);
	    key2 = wgetch(win); /* check for a following letter */
	    nodelay(win, FALSE);

	    if ('a' <= key2 && key2 <= 'z')
		key = META(key2);
	}
#endif

    } while (!key);

#if defined(PDCURSES)
    /* PDCurses provides exciting new names for the enter key.
     * Translate these here, instead of checking for them all over the place. */
    if (key == PADENTER)
	key = '\r';
#endif

    return key;
}


struct gamewin *alloc_gamewin(int extra)
{
    struct gamewin *gw = malloc(sizeof(struct gamewin) + extra);
    memset(gw, 0, sizeof(struct gamewin) + extra);
    
    if (firstgw == NULL && lastgw == NULL)
	firstgw = lastgw = gw;
    else {
	lastgw->next = gw;
	gw->prev = lastgw;
	lastgw = gw;
    }
    
    return gw;
}


void delete_gamewin(struct gamewin *gw)
{
    if (firstgw == gw)
	firstgw = gw->next;
    if (lastgw == gw)
	lastgw = gw->prev;
    
    if (gw->prev)
	gw->prev->next = gw->next;
    if (gw->next)
	gw->next->prev = gw->prev;
    
    free(gw);
}


/*----------------------------------------------------------------------------*/
/* misc api functions */

void curses_pause(enum nh_pause_reason reason)
{
    doupdate();
    if (reason == P_MESSAGE && msgwin != NULL)
	pause_messages();
    else if (mapwin != NULL)
	/* P_MAP: pause to show the result of detection or similar */
	get_map_key(FALSE);
}


/* expand tabs into proper number of spaces */
static char *tabexpand(char *sbuf)
{
    char buf[BUFSZ];
    char *bp, *s = sbuf;
    int idx;

    if (!*s) return sbuf;

    /* warning: no bounds checking performed */
    for (bp = buf, idx = 0; *s; s++)
	if (*s == '\t') {
	    do *bp++ = ' '; while (++idx % 8);
	} else {
	    *bp++ = *s;
	    idx++;
	}
    *bp = 0;
    return strcpy(sbuf, buf);
}


void curses_display_buffer(const char *inbuf, nh_bool trymove)
{
    char *line, *line_end, *buf;
    char linebuf[BUFSZ];
    int icount, size;
    struct nh_menuitem *items;

    buf = strdup(inbuf);
    icount = 0;
    size = 10;
    items = malloc(size * sizeof(struct nh_menuitem));

    line = buf;
    while (*line) {
	line_end = strchr(line, '\n');
	if (line_end) {
	    /* deal with the CR that Windows adds to the LF */
	    if (line_end > line && *(line_end - 1) == '\r')
		*(line_end - 1) = '\0';
	    *line_end = '\0';
	}

	strncpy(linebuf, line, BUFSZ);
	linebuf[BUFSZ - 1] = '\0';
	if (strchr(linebuf, '\t') != 0)
	    tabexpand(linebuf);
	add_menu_txt(items, size, icount, linebuf, MI_TEXT);

	if (line_end) {
	    line = line_end + 1;
	} else {
	    /* null terminator guaranteed if earlier strdup() succeeded */
	    line = strchr(line, '\0');
	}
    }

    curses_display_menu(items, icount, NULL, PICK_NONE, NULL);
    free(items);
    free(buf);
}


void curses_raw_print(const char *str)
{
    endwin();
    fprintf(stderr, "%s\n", str);
    refresh();
    
    curses_msgwin(str);
}


/* sleep for 50 ms */
void curses_delay_output(void)
{
    doupdate();
#if defined(WIN32)
    Sleep(45);
#else
    usleep(50 * 1000);
#endif
}

