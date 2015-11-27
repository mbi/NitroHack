/* Copyright (c) Daniel Thaler, 2011.                             */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>

/* save more of long messages than could be displayed in COLNO chars */
#define MSGLEN 119

struct message {
    char *msg;
    int turn;
};

static struct message *msghistory;
static int histsize, histpos;
static char msglines[MAX_MSGLINES][COLNO+1];
static int curline;
static int start_of_turn_curline = -1;
static int last_redraw_curline;
static nh_bool skip_more;
static int prevturn, action, prevaction;

static void newline(void);


static void store_message(int turn, const char *msg)
{
    if (!*msg)
	return;
    
    histpos++;
    if (histpos >= histsize)
	histpos = 0;
    
    msghistory[histpos].turn = turn;
    if (msghistory[histpos].msg)
	free(msghistory[histpos].msg);
    msghistory[histpos].msg = strdup(msg);
}


void alloc_hist_array(void)
{
    struct message *oldhistory = msghistory;
    int i, oldsize = histsize, oldpos = histpos;
    
    msghistory = calloc(settings.msghistory, sizeof(struct message));
    histsize = settings.msghistory;
    histpos = -1; /* histpos is incremented before any message is stored */
    for (i = 0; i < histsize; i++) {
	msghistory[i].turn = 0;
	msghistory[i].msg = NULL;
    }

    if (!oldhistory)
	return;
    
    for (i = 0; i < oldsize; i++) {
	int pos = oldpos + i + 1;
	if (pos >= oldsize) /* wrap around eventually */
	    pos -= oldsize;
	
	if (oldhistory[pos].turn)
	    store_message(oldhistory[pos].turn, oldhistory[pos].msg);
	if (oldhistory[pos].msg)
	    free(oldhistory[pos].msg);
    }
    
    free(oldhistory);
}


static void newline(void)
{
    if (msglines[curline][0]) { /* do nothing if the line is empty */
	curline++;
	if (curline >= MAX_MSGLINES)
	    curline = 0;
	
	msglines[curline][0] = '\0';
    }
}


static void prune_messages(int maxturn)
{
    int i, pos, removed;
    const char *msg;
    
    if (!msghistory)
	return;
    
    /* remove future messages from the history */
    removed = 0;
    while (msghistory[histpos].turn >= maxturn && removed < histsize) {
	msghistory[histpos].turn = 0;
	if (msghistory[histpos].msg) {
	    free(msghistory[histpos].msg);
	    msghistory[histpos].msg = NULL;
	}
	removed++;
	histpos--;
	if (histpos < 0)
	    histpos += histsize;
    }
    
    /* rebuild msglines */
    curline = 0;
    for (i = 0; i < MAX_MSGLINES; i++)
	msglines[i][0] = '\0';
    for (i = 0; i < histsize; i++) {
	pos = histpos + i + 1;
	if (pos >= histsize)
	    pos -= histsize;
	
	msg = msghistory[pos].msg;
	if (!msg)
	    continue;
	if (msghistory[pos].turn > prevturn)
	    start_of_turn_curline = last_redraw_curline = curline;
	prevturn = msghistory[pos].turn;

	if (strlen(msglines[curline]) + strlen(msg) + 2 < COLNO) {
	    if (msglines[curline][0])
		strcat(msglines[curline], "  ");
	    strcat(msglines[curline], msg);
	} else {
	    int j, output_count;
	    char **output;
	    ui_wrap_text(COLNO, msg, &output_count, &output);
	    for (j = 0; j < output_count; j++) {
		if (msglines[curline][0])
		    newline();
		strcpy(msglines[curline], output[j]);
	    }
	    ui_free_wrap(output);
	}
    }
}


void draw_msgwin(void)
{
    int i, pos;
    nh_bool drew_older = FALSE;

    for (i = getmaxy(msgwin) - 1; i >= 0; i--) {
	pos = curline - getmaxy(msgwin) + 1 + i;
	if (pos < 0)
	    pos += MAX_MSGLINES;
	if (pos == start_of_turn_curline) {
	    if (settings.darkmsg)
		wattron(msgwin, curses_color_attr(CLR_BLUE, 0));
	    drew_older = TRUE;
	}
	wmove(msgwin, i, 0);
	waddstr(msgwin, msglines[pos]);
	/* Only clear the remainder of the line if the cursor did not wrap. */
	if (getcurx(msgwin) || !*msglines[pos])
	    wclrtoeol(msgwin);
    }

    if (drew_older) {
	if (settings.darkmsg)
	    wattroff(msgwin, curses_color_attr(CLR_BLUE, 0));
    } else {
	/* Don't dim out messages if the message buffer wraps. */
	start_of_turn_curline = -1;
    }

    wnoutrefresh(msgwin);
}


static void more(void)
{
    int key;
    int cursx, cursy;
    attr_t attr = A_NORMAL;

    if (skip_more)
	return;

    draw_msgwin();

    if (settings.standout)
	attr = A_STANDOUT;

    if (getmaxy(msgwin) == 1) {
	wmove(msgwin, getmaxy(msgwin)-1, COLNO-8);
	wattron(msgwin, attr);
	waddstr(msgwin, "--More--");
	wattroff(msgwin, attr);
	wrefresh(msgwin);
    } else {
	newline();
	draw_msgwin();
	wmove(msgwin, getmaxy(msgwin)-1, 0);
	wclrtoeol(msgwin);
	wmove(msgwin, getmaxy(msgwin)-1, COLNO/2 - 4);
	wattron(msgwin, attr);
	waddstr(msgwin, "--More--");
	wattroff(msgwin, attr);
	wrefresh(msgwin);
    }

    getyx(msgwin, cursy, cursx);
    do {
	key = nh_wgetch(msgwin);
	draw_map(player.x, player.y);
	wmove(msgwin, cursy, cursx);
	doupdate();
    } while (key != '\n' && key != '\r' && key != ' ' && key != KEY_ESC);

    /* Clean up after the --More--. */
    if (getmaxy(msgwin) == 1) {
	newline();
    } else {
	wmove(msgwin, getmaxy(msgwin)-1, 0);
	wclrtoeol(msgwin);
    }
    draw_msgwin();

    if (key == KEY_ESC)
	skip_more = TRUE;

    /* we want to --more-- by screenfuls, not lines */
    last_redraw_curline = curline;
}


/* called from get_command */
void new_action(void)
{
    action++;
    start_of_turn_curline = last_redraw_curline = curline;
    draw_msgwin();
}


static void curses_print_message_core(int turn, const char *msg, nh_bool canblock)
{
    int hsize, vsize, maxlen;
    nh_bool use_current_line, died;

    if (!msghistory)
	alloc_hist_array();

    if (turn < prevturn) /* going back in time can happen during replay */
	prune_messages(turn);

    if (!*msg)
	return; /* empty message. done. */

    if (action > prevaction) {
	/* re-enable output if it was stopped and start a new line */
	skip_more = FALSE;
	newline();
    }
    prevturn = turn;
    prevaction = action;

    store_message(turn, msg);

    /*
     * generally we want to put as many messages on one line as possible to
     * maximize space usage. A new line is begun after each player turn or if
     * more() is called via pause_messages(). "You die" also deserves its own line.
     *
     * If the message area is only one line high, space for "--More--" must be
     * reserved at the end of the line, otherwise  --More-- is shown on a new line.
     */
    getmaxyx(msgwin, vsize, hsize);
    maxlen = hsize;
    if (maxlen >= COLNO) maxlen = COLNO - 1;
    if (vsize == 1) maxlen -= 8; /* for "--More--" */

    if (settings.msg_per_line) {
	use_current_line = !msglines[curline][0];
    } else {
	use_current_line = (strlen(msglines[curline]) + strlen(msg) + 2 < maxlen);
    }
    died = !strncmp(msg, "You die", 7);
    if (use_current_line && !died) {
	if (msglines[curline][0])
	    strcat(msglines[curline], "  ");
	strcat(msglines[curline], msg);
    } else {
	int i, output_count;
	char **output;

	ui_wrap_text(maxlen, msg, &output_count, &output);

	for (i = 0; i < output_count; i++) {
	    if (msglines[curline][0]) {
		/*
		 * If we would scroll a message off the screen that
		 * the user hasn't had a chance to look at this redraw,
		 * then run more(), else newline().  Because the
		 * --More-- takes up a line by itself, we need to
		 * offset that by one line.  Thus, a message window of
		 * height 2 requires us to always show a --More-- even
		 * if we're on the first message of the redraw; with
		 * height 3, we can safely newline after the first
		 * line of messages but must --More-- after the
		 * second, etc.  getmaxy() gives the height of the window
		 * minus 1, which is why we only subtract 2 not 3.
		 */
		if (canblock &&
		    (curline + MAX_MSGLINES - last_redraw_curline) %
		    MAX_MSGLINES > getmaxy(msgwin) - 2)
		    more();
		else
		    newline();
	    }
	    strcpy(msglines[curline], output[i]);
	}

	ui_free_wrap(output);
    }

    draw_msgwin();
}


void curses_print_message(int turn, const char *inmsg)
{
    curses_print_message_core(turn, inmsg, TRUE);
}


void curses_print_message_nonblocking(int turn, const char *inmsg)
{
    curses_print_message_core(turn, inmsg, FALSE);
}



void pause_messages(void)
{
    draw_msgwin();
    if (msglines[curline][0]) /* new text printed this turn */
	more();
}


void doprev_message(void)
{
    /* 80 chars - 2 (borders) - 2 (gutters) - "T:" - 1 (divider) */
    const int hist_msg_width = 73;

    int prevturn;
    int pos;
    int maxturn, maxturn_width;

    struct nh_menuitem *items;
    char buf[MSGLEN+1];
    int icount, size, i;

    /* keep the previous messages window within 80 characters */
    maxturn = 0;
    for (i = 0; i < histsize; i++) {
	pos = (histpos + i + 1) % histsize;
	if (msghistory[pos].turn > maxturn)
	    maxturn = msghistory[pos].turn;
    }
    maxturn_width = 0;
    while (maxturn) {
	maxturn_width++;
	maxturn /= 10;
    }
    if (maxturn_width < 1)
	maxturn_width = 1;

    /* Prevent duplicate turn stamps. */
    prevturn = 0;

    icount = 0;
    size = 10;
    items = malloc(size * sizeof(struct nh_menuitem));

    for (i = 0; i < histsize; i++) {
	int turn;
	const char *msg;

	pos = (histpos + i + 1) % histsize;

	if (!msghistory[pos].turn)
	    continue;

	turn = msghistory[pos].turn;
	msg = msghistory[pos].msg;

	if (strlen(msg) <= hist_msg_width - maxturn_width) {
	    if (turn != prevturn)
		snprintf(buf, MSGLEN+1, "T:%*d\t%s", maxturn_width, turn, msg);
	    else
		snprintf(buf, MSGLEN+1, "\t%s", msg);
	    add_menu_txt(items, size, icount, buf, MI_TEXT);
	} else {
	    int output_count, j;
	    char **output;

	    ui_wrap_text(hist_msg_width - maxturn_width, msg, &output_count, &output);

	    if (turn != prevturn)
		snprintf(buf, MSGLEN+1, "T:%*d\t%s", maxturn_width, turn, output[0]);
	    else
		snprintf(buf, MSGLEN+1, "\t%s", output[0]);
	    add_menu_txt(items, size, icount, buf, MI_TEXT);

	    for (j = 1; j < output_count; j++) {
		snprintf(buf, MSGLEN+1, "\t%s", output[j]);
		add_menu_txt(items, size, icount, buf, MI_TEXT);
	    }

	    ui_free_wrap(output);
	}

	prevturn = turn;
    }

    curses_display_menu_bottom(items, icount, "Message history:", PICK_NONE, NULL);
    free(items);
}


void cleanup_messages(void)
{
    int i;

    for (i = 0; i < histsize; i++) {
	if (msghistory[i].msg)
	    free(msghistory[i].msg);
    }
    free(msghistory);
    prevturn = 0;

    /* extra cleanup to prevent old messages from appearing in a new game */
    msghistory = NULL;
    curline = last_redraw_curline = 0;
    start_of_turn_curline = -1;
    histsize = histpos = 0;
    for (i = 0; i < MAX_MSGLINES; i++)
	msglines[i][0] = '\0';
}


/*
 * Given the string "input", generate a series of strings of the given maximum
 * width, wrapping lines at spaces in the text.
 *
 * The number of lines will be stored into *output_count, and an array of the
 * output lines will be stored in *output.
 *
 * The memory for both the output strings and the output array is obtained via
 * malloc and should be freed when no longer needed.
 */
void ui_wrap_text(int width, const char *input, int *output_count, char ***output)
{
    const int min_width = 20, max_wrap = 20;

    int len = strlen(input);
    int input_idx, input_lidx;
    int idx, outcount;

    *output = malloc(max_wrap * sizeof(char *));
    for (idx = 0; idx < max_wrap; idx++)
	(*output)[idx] = NULL;

    input_idx = 0;
    outcount = 0;
    do {
	size_t outsize;
	char *outbuf;

	if (len - input_idx <= width) {
	    /* enough room for the rest of the input */
	    (*output)[outcount] = strdup(input + input_idx);
	    outcount++;
	    break;
	}

	/* find nearest space in input to right edge that doesn't exceed width */
	input_lidx = input_idx + width;
	while (!isspace((unsigned char)input[input_lidx]) &&
	       input_lidx - input_idx > min_width)
	    input_lidx--;
	/* didn't find a space, so just go with width-worth of characters */
	if (!isspace((unsigned char)input[input_lidx]))
	    input_lidx = input_idx + width;

	outsize = input_lidx - input_idx;
	outbuf = malloc(outsize + 1);
	strncpy(outbuf, input + input_idx, outsize);
	outbuf[outsize] = '\0';
	(*output)[outcount] = outbuf;
	outcount++;

	/* skip extra spaces in break */
	input_idx = input_lidx;
	while (isspace((unsigned char)input[input_idx]))
	    input_idx++;
    } while (input[input_idx] && outcount < max_wrap);

    *output_count = outcount;
}

void ui_free_wrap(char **wrap_output)
{
    const int max_wrap = 20;
    int idx;

    for (idx = 0; idx < max_wrap && wrap_output[idx]; idx++)
	free(wrap_output[idx]);
    free(wrap_output);
}
