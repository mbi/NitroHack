/* Copyright (c) Daniel Thaler, 2011				  */
/* DynaHack may be freely redistributed.  See license for details. */

#ifndef NHCURSES_H
#define NHCURSES_H

/* _GNU_SOURCE activates lots of stuff in the in glibc headers.
 * _XOPEN_SOURCE_EXTENDED is needed for ncurses to activate widechars */
#define _GNU_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define UNICODE
#define _CRT_SECURE_NO_WARNINGS /* huge warning spew from MS CRT otherwise */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* This needs to be included before curses.h with PDCurses.
 * curses.h for PDCurses pulls in wchar.h in MinGW, declaring
 * _wunlink() which conflicts with another _wunlink() in io.h
 * pulled in by fcntl.h if this isn't included first. */
#include <fcntl.h>

#if !defined(WIN32) /* UNIX + APPLE */
# include <unistd.h>
#define FILE_OPEN_MASK 0660
#else	/* WINDOWS */

# include <windows.h>
# include <shlobj.h>
# undef MOUSE_MOVED /* this definition from windows.h conflicts with a definition from curses */
# define srandom srand
# define random rand

# if defined (_MSC_VER)
#  include <io.h>
#  pragma warning(disable:4996) /* disable warnings about deprecated posix function names */
/* If we're using the Microsoft compiler, we also get the Microsoft C lib which
 * doesn't have plain snprintf.  Note that this macro is an MSVC-style variadic
 * macro, which is not understood by gcc (it uses a different notation). */
#  define snprintf(buf, len, fmt, ...) _snprintf_s(buf, len, len-1, fmt, __VA_ARGS__)
#  define strncasecmp _strnicmp
# endif
#define FILE_OPEN_MASK (_S_IREAD | _S_IWRITE)
#endif

/* File handling compatibility.
 * On UNIX + APPLE, the normal API can handle multibyte strings, so no special
 * consideration for international charsets is required.  Windows on the other
 * hand has special variants of all api functions that take wchar_t strings.
 * The following abstactions are introduced to handle this:
 *  fnchar: a filename character
 *  fnncat: strncat for fnchars
 *  sys_open: system-dependent open
 */
#if !defined(WIN32) /* UNIX + APPLE */
typedef char fnchar;
# define sys_open  open
# define fnncat(str1, str2, len) strncat(str1, str2, len)
# define FN(x) (x)
# define FN_FMT "%s"
#else

typedef wchar_t fnchar;
# define umask(x)
# define fnncat(str1, str2, len) wcsncat(str1, str2, len)
# define fopen(name, mode)  _wfopen(name, L ## mode)
# define sys_open(name, flags, perm)  _wopen(name, flags | _O_BINARY, perm)
# define unlink _wunlink
# define FN(x) (L ## x)
# define FN_FMT "%ls"
#endif


#include "nitrohack.h"

#ifdef NETCLIENT
# define NHNET_TRANSPARENT
# include "nitrohack_client.h"
#endif

#ifndef PDCURSES
# ifdef USE_OSX_HOMEBREW_CURSES
# include <ncurses/curses.h>
# else
# include <ncursesw/curses.h>
# endif
#else
# define PDC_WIDE
# ifdef WIN32
#  include <curses.h>
/* Windows support for Underline is a joke: it simply switches to color 1 (dark blue!) */
#  undef A_UNDERLINE
#  define A_UNDERLINE A_NORMAL
# else
#  include <xcurses.h>
# endif
# define CCHARW_MAX 1 /* not set by pdcurses */
#endif

#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
# define min(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef DYNAHACKDIR
#define DYNAHACKDIR "/usr/share/DynaHack/"
#endif

#define KEY_ESC 27
#define KEY_BACKDEL 127

#define MAX_MSGLINES 40

/* attributes for dialog frames */
#define FRAME_ATTRS  (COLOR_PAIR(6)) /* magenta frames for better visibility */

/* Background color support
 *
 * We have 6 non-default background colors (don't use white, there are terminals
 * that hate it, combined with FG_COLOR_COUNT possible foregrounds for each. */
#define BG_COLOR_COUNT 7
#define FG_COLOR_COUNT (COLORS >= 16 ? 16 : 8) /* 8 is if we're using bold */
#define BG_COLOR_SUPPORT (COLOR_PAIRS > BG_COLOR_COUNT * FG_COLOR_COUNT)

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))


/* dungeon name option values */
enum dgn_name_value {
    DGN_NAME_AUTO = -1,
    DGN_NAME_DLVL = 0,
    DGN_NAME_SHORT,
    DGN_NAME_FULL
};


enum game_dirs {
    CONFIG_DIR,
    SAVE_DIR,
    LOG_DIR,
    DUMP_DIR
};


struct interface_flags {
    nh_bool done_hup;
    nh_bool ingame;
    nh_bool draw_frame;
    nh_bool draw_sidebar;
    nh_bool status3; /* draw the 3 line status instead of the classic 2 lines */
    nh_bool color; /* the terminal has color capability */
    nh_bool unicode; /* ncurses detected a unicode locale */
    int levelmode;
    int playmode;
    int viewheight;
    int msgheight; /* actual height */
};

struct settings {
    char     plname[BUFSZ]; /* standard player name;
                             * size is BUFSZ rather than PL_NSIZ, because the
			     * buffer may be written to directly via curses_getline */
    nh_bool  end_own;	/* list all own scores */
    int      end_top, end_around;	/* describe desired score list */
    int      graphics;
    int      menu_headings; /* ATR for menu headings */
    int      msgheight; /* requested height of the message win */
    int      msghistory;/* # of historic messages to keep for prevmsg display */
    int      optstyle;	/* option display style */
    int      win_height;/* window height, PDCurses on WIN32 only */
    int      win_width; /* window height, PDCurses on WIN32 only */
    nh_bool  darkgray;  /* use bolded black instead of dark blue for CLR_BLACK */
    nh_bool  darkmsg;	/* show past messages in a darker color */
    nh_bool  darkroom;	/* show spaces out-of-sight in darker colors */
    nh_bool  extmenu;	/* extended commands use menu interface */
    nh_bool  hilite_peaceful; /* hilight peaceful monsters */
    nh_bool  hilite_pet;/* hilight pets */
    nh_bool  mapcolors;	/* use thematic colors for special map regions */
    nh_bool  msg_per_line; /* show each message on a new line */
    nh_bool  repeat_num_auto; /* number keys automatically show repeat count prompt */
    nh_bool  showexp;	/* show experience points */
    nh_bool  showscore;	/* show score */
    nh_bool  standout;	/* use standout for --More-- */
    nh_bool  time;	/* display elapsed 'time' */
    nh_bool  invweight;	/* show item weight in the inventory */
    nh_bool  sidebar;   /* draw the inventory sidebar */
    nh_bool  frame;     /* draw a frame around the window sections */
    nh_bool  frame_hp_color; /* recolor frame according to HP */
    nh_bool  classic_status; /* use classic NetHack layout for status lines */
    nh_bool  status3;	/* draw 3 line status */
    enum dgn_name_value dungeon_name; /* how to show dungeon name and/or depth */
};


/* curses_symdef is like nh_symdef, but with extra unicode sauce */
struct curses_symdef {
    char *symname;
    int color;
    wchar_t unichar[CCHARW_MAX+1];
    short ch; /* for non-unicode displays */
    nh_bool custom; /* true if this is a custom value that was explicitly changed by the user */
};


struct curses_drawing_info {
    /* background layer symbols: nh_dbuf_entry.bg */
    struct curses_symdef *bgelements;
    /* background layer symbols: nh_dbuf_entry.bg */
    struct curses_symdef *traps;
    /* object layer symbols: nh_dbuf_entry.obj */
    struct curses_symdef *objects;
    /* invisible monster symbol: show this if nh_dbuf_entry.invis is true */
    struct curses_symdef *invis;
    /* monster layer symbols: nh_dbuf_entry.mon
     * symbols with id <= num_monsters are actual monsters, followed by warnings */
    struct curses_symdef *monsters;
    struct curses_symdef *warnings;
    /* effect layer symbols: nh_dbuf_entry.effect
     * NH_EFFECT_TYPE */
    struct curses_symdef *explsyms;
    struct curses_symdef *expltypes;
    struct curses_symdef *zapsyms; /* default zap symbols; no color info */
    struct curses_symdef *zaptypes; /* zap beam types + colors. no symbols */
    struct curses_symdef *breathsyms; /* like zapsyms; uses zaptypes for colors */
    struct curses_symdef *effects; /* shield, boomerang, digbeam, flashbeam, gascloud */
    struct curses_symdef *swallowsyms; /* no color info: use the color of the swallower */
    int num_bgelements;
    int num_traps;
    int num_objects;
    int num_monsters;
    int num_warnings;
    int num_expltypes;
    int num_zaptypes;
    int num_effects;
    
    int bg_feature_offset;
};

/*
 * Graphics sets for display symbols
 */
enum nh_text_mode {
    ASCII_GRAPHICS,	/* regular characters: '-', '+', &c */
    UNICODE_GRAPHICS	/* uses whatever charecters we want: they're ALL available */
};

typedef nh_bool (*getlin_hook_proc)(char *, void *);



struct gamewin {
    void (*draw)(struct gamewin *gw);
    void (*resize)(struct gamewin *gw);
    WINDOW *win;
    struct gamewin *next, *prev;
    void *extra[0];
};


#define MAXCOLS 16
struct win_menu {
    WINDOW *content;
    struct nh_menuitem *items;
    char *selected;
    const char *title;
    int icount, how, offset;
    int height, frameheight, innerheight;
    int width, innerwidth, colpos[MAXCOLS], maxcol;
    int x1, y1, x2, y2;
};

struct win_objmenu {
    WINDOW *content;
    struct nh_objitem *items;
    int *selected;
    const char *title;
    int icount, how, offset, selcount;
    int height, frameheight, innerheight;
    int width, innerwidth;
};

struct win_getline {
    char *buf;
    const char *query;
    int pos;
};

/*----------------------------------------------------------------------------*/

extern struct settings settings;
extern struct interface_flags ui_flags;
extern nh_bool interrupt_multi, game_is_running;
extern const char quit_chars[];
extern struct nh_window_procs curses_windowprocs;
extern WINDOW *basewin, *mapwin, *msgwin, *statuswin, *sidebar;
extern struct curses_drawing_info *default_drawing, *cur_drawing;
extern struct nh_player_info player;
extern int initrole, initrace, initgend, initalign;
extern nh_bool random_player;
extern struct nh_cmd_desc *keymap[KEY_MAX];
extern char *override_hackdir, *override_userdir;

/*----------------------------------------------------------------------------*/

/* boxdraw.c */
extern void nh_box_set_graphics(enum nh_text_mode mode);
extern int nh_box_mvwadd_ulcorner(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_llcorner(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_urcorner(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_lrcorner(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_ltee(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_rtee(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_btee(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_ttee(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_hline(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_vline(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwadd_plus(WINDOW *win, int y, int x, attr_t attrs);
extern int nh_box_mvwhline(WINDOW *win, int y, int x, int n, attr_t attrs);
extern int nh_box_mvwvline(WINDOW *win, int y, int x, int n, attr_t attrs);
extern int nh_box_wborder(WINDOW *, attr_t attrs);
extern int nh_box_whline(WINDOW *win, int n, attr_t attrs);

/* color.c */
extern void init_nhcolors(void);
extern attr_t curses_color_attr(int nh_color, int bg_color);
extern void set_darkgray(void);
extern int darken(int nh_color);

/* dialog.c */
extern WINDOW *newdialog(int height, int width);
extern void reset_last_dir(void);
extern enum nh_direction curses_getdir(const char *query, nh_bool restricted);
extern char curses_yn_function(const char *query, const char *resp, char def);
extern char curses_query_key(const char *query, int *count);
extern int curses_msgwin(const char *msg);

/* gameover.c */
extern void curses_outrip(struct nh_menuitem *items, int icount, nh_bool tombstone,
	const char *plname, int gold, const char *killbuf, int how, int year);


/* getline.c */
extern void draw_getline(struct gamewin *gw);
extern nh_bool curses_get_ext_cmd(char *cmd_out, const char **namelist,
			   const char **desclist, int listlen);
extern void curses_getline(const char *query, char *buffer);

/* keymap.c */
extern void reset_prev_cmd(void);
extern nh_bool check_prev_cmd_same(void);
extern int get_current_cmd_key(void);
extern const char *curses_keyname(int key);
extern void handle_internal_cmd(struct nh_cmd_desc **cmd, struct nh_cmd_arg *arg, int *count);
extern const char *get_command(int *count, struct nh_cmd_arg *arg);
extern void set_next_command(const char *cmd, struct nh_cmd_arg *arg);
extern void load_keymap(void);
extern void free_keymap(void);
extern void show_keymap_menu(nh_bool readonly);
extern enum nh_direction key_to_dir(int key);

/* main.c */
extern const char **get_logo(nh_bool large);

/* map.c */
extern int get_map_key(int place_cursor);
extern void curses_update_screen(struct nh_dbuf_entry dbuf[ROWNO][COLNO], int ux, int uy);
extern int curses_getpos(int *x, int *y, nh_bool force, const char *goal);
extern void draw_map(int cx, int cy);

/* menu.c */
extern void draw_menu(struct gamewin *gw);
extern int curses_display_menu(struct nh_menuitem *items, int icount,
			       const char *title, int how, int *results);
extern int curses_display_menu_bottom(struct nh_menuitem *items, int icount,
				      const char *title, int how, int *results);
extern int curses_display_menu_core(struct nh_menuitem *items, int icount,
			     const char *title, int how, int *results,
			     int x1, int y1, int x2, int y2,
			     nh_bool (*changefn)(struct win_menu*, int),
			     nh_bool start_at_bottom);
extern int curses_display_objects(struct nh_objitem *items, int icount,
		  const char *title, int how, struct nh_objresult *pick_list);
extern void draw_objlist(WINDOW *win, int icount, struct nh_objitem *items,
		  int *selected, int how);

/* messages.c */
extern void alloc_hist_array(void);
extern void curses_print_message(int turn, const char *msg);
extern void curses_print_message_nonblocking(int turn, const char *inmsg);
extern void draw_msgwin(void);
extern void pause_messages(void);
extern void doprev_message(void);
extern void cleanup_messages(void);
extern void new_action(void);
extern void ui_wrap_text(int width, const char *input,
			 int *output_count, char ***output);
extern void ui_free_wrap(char **wrap_output);

/* options.c */
extern void display_options(nh_bool change_birth_opt);
extern void print_options(void);
extern void init_options(void);
extern void read_nh_config(void);
extern void write_config(void);

/* outchars.c */
extern void init_displaychars(void);
extern void free_displaychars(void);
extern void object_symdef(int otyp, int obj_mn, struct curses_symdef *sym);
extern int mapglyph(struct nh_dbuf_entry *dbe, struct curses_symdef *syms,
		    int *bg_color);
extern void set_rogue_level(nh_bool enable);
extern void switch_graphics(enum nh_text_mode mode);
extern void print_sym(WINDOW *win, struct curses_symdef *sym,
		      attr_t extra_attrs, int bg_color);
extern void curses_notify_level_changed(int dmode);

/* playerselect.c */
extern nh_bool player_selection(int *out_role, int *out_race, int *out_gend,
				int *out_align, int randomall, nh_bool tutorial);

/* replay.c */
extern void replay(void);
extern void describe_game(char *buf, enum nh_log_status status, struct nh_game_info *gi);
extern void replay_commandloop(int fd);

/* rungame.c */
extern nh_bool get_gamedir(enum game_dirs dirtype, fnchar *buf);
extern int commandloop(void);
extern void rungame(nh_bool tutorial);
extern nh_bool loadgame(void);
extern fnchar **list_gamefiles(fnchar *dir, int *count);

/* sidebar.c */
extern void reset_old_status(void);
extern void update_old_status(void);
extern void draw_sidebar_divider(void);
extern void draw_sidebar(void);
extern nh_bool curses_list_items(struct nh_objitem *items, int icount, nh_bool invent);
extern nh_bool curses_list_items_nonblocking(struct nh_objitem *items, int icount, nh_bool invent);
extern void cleanup_sidebar(nh_bool dealloc);

/* status.c */
extern void curses_update_status(struct nh_player_info *pi);
extern void curses_update_status_silent(struct nh_player_info *pi);

/* topten.c */
extern void show_topten(char *player, int top, int around, nh_bool own);

/* windows.c */
extern void init_curses_ui(void);
extern void exit_curses_ui(void);
extern attr_t frame_hp_color(void);
extern void redraw_frame(void);
extern void create_game_windows(void);
extern void destroy_game_windows(void);
extern void redraw_game_windows(void);
extern void handle_resize(void);
extern void rebuild_ui(void);
extern int nh_wgetch(WINDOW *win);
extern struct gamewin *alloc_gamewin(int extra);
extern void delete_gamewin(struct gamewin *win);
extern void curses_pause(enum nh_pause_reason reason);
extern void curses_display_buffer(const char *buf, nh_bool trymove);
extern void curses_raw_print(const char *str);
extern void curses_delay_output(void);

#if defined(NETCLIENT)
/* netgame.c */
extern void netgame(void);

/* netplay.c */
extern void net_rungame(nh_bool tutorial);
extern void net_loadgame(void);
extern void net_replay(void);
#endif

#endif
