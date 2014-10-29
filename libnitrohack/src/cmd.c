/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"
/* #define DEBUG */	/* uncomment for debugging */

/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#define NR_OF_EOFS	20

#define CMD_TRAVEL (char)0x90

static int (*timed_occ_fn)(void);

static int timed_occupation(void);
static int domonability(void);
static int doautoexplore(void);
static int dotravel(int x,int y);
static int wiz_wish(void);
static int wiz_identify(void);
static int wiz_map(void);
static int wiz_genesis(void);
static int wiz_where(void);
static int wiz_detect(void);
static int wiz_panic(void);
static int wiz_polyself(void);
static int wiz_levport_or_tut_rev(void);
static int wiz_level_change(void);
static int wiz_show_seenv(void);
static int wiz_show_vision(void);
static int wiz_mon_polycontrol(void);
static int wiz_show_wmodes(void);
static int wiz_mazewalkmap(void);
static int wiz_show_rooms(void);
extern char SpLev_Map[COLNO][ROWNO];
static void count_obj(struct obj *, long *, long *, boolean, boolean);
static void obj_chain(struct menulist *, const char *, struct obj *, long *, long *);
static void mon_invent_chain(struct menulist *, const char *, struct monst *,
			     long *, long *);
static void mon_chain(struct menulist *, const char *, struct monst *, long *, long *);
static void contained(struct menulist *, const char *, long *, long *);
static int wiz_show_stats(void);
static int doattributes(void);
static int doconduct(void); /**/
static boolean minimal_enlightenment(void);

static void enlght_line(struct menulist *,const char *,const char *,const char *);
static char *enlght_combatinc(const char *,int,int,char *);


#ifndef M
# define M(c)		((c) - 128)
#endif
#ifndef C
#define C(c)		(0x1f & (c))
#endif

const struct cmd_desc cmdlist[] = {
	/* "str", "", defkey, altkey, wiz, buried, func, arg*/
	{"adjust", "adjust inventory letters", M('a'), 0, TRUE, doorganize, CMD_ARG_NONE | CMD_ARG_OBJ | CMD_EXT},
	{"annotate", "name the current level", 0, C('f'), TRUE, donamelevel, CMD_ARG_NONE | CMD_EXT},
	{"apply", "use a tool or dip into a potion", 'a', 0, FALSE, doapply, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"attributes", "show your attributes", C('x'), 0, TRUE, doattributes, CMD_ARG_NONE},
	{"autoexplore", "automatically explore until something happens", 'v', 0, FALSE, doautoexplore, CMD_ARG_NONE},
	{"cast", "cast a spell from memory", 'Z', 0, TRUE, docast, CMD_ARG_NONE},
	{"chat", "talk to someone", 'c', M('c'), TRUE, dotalk, CMD_ARG_NONE | CMD_EXT},	/* converse? */
	{"close", "close a door", 0, 0, FALSE, doclose, CMD_ARG_DIR},
	{"conduct", "list which challenges you have adhered to", 0, 0, TRUE, doconduct, CMD_ARG_NONE | CMD_EXT | CMD_NOTIME},
	{"countgold", "show gold, debt, credit, and unpaid items", '$', 0, TRUE, doprgold, CMD_ARG_NONE | CMD_NOTIME},
	{"dip", "dip an object into something", M('d'), 0, FALSE, dodip, CMD_ARG_NONE | CMD_EXT | CMD_ARG_OBJ},
	{"discoveries", "show your knowledge about items", '\\', 0, TRUE, dodiscovered, CMD_ARG_NONE | CMD_NOTIME},
	{"drink", "quaff a potion", 'q', 0, FALSE, dodrink, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"drop", "drop one item", 'd', 0, FALSE, dodrop, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"eat", "eat an item from inventory or the floor", 'e', 0, FALSE, doeat, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"elbereth", "write an Elbereth in the dust", C('e'), 0, FALSE, doelbereth, CMD_ARG_NONE},
	{"enhance", "advance or check weapons skills", M('e'), 0, TRUE, enhance_weapon_skill, CMD_ARG_NONE | CMD_EXT},
	{"engrave", "write on the floor", 'E', 0, FALSE, doengrave, CMD_ARG_NONE | CMD_ARG_OBJ | CMD_ZEROOBJ},
	{"farlook", "say what is on a distant square", ';', 0, TRUE, doquickwhatis, CMD_ARG_NONE | CMD_NOTIME},
	{"fight", "attack even if no hostile monster is visible", 'F', 0, FALSE, dofight, CMD_ARG_DIR},
	{"fire", "throw your quivered item", 'f', 0, FALSE, dofire, CMD_ARG_NONE},
	{"force", "force a lock", M('f'), 0, FALSE, doforce, CMD_ARG_NONE | CMD_EXT},
	{"history", "show a list of your historic deeds", 0, 0, TRUE, dohistory, CMD_ARG_NONE | CMD_EXT | CMD_NOTIME},
	{"idtrap", "identify a trap", '^', 0, TRUE, doidtrap, CMD_ARG_NONE | CMD_NOTIME},
	{"inventory", "show your inventory", 'i', 0, TRUE, ddoinv, CMD_ARG_NONE | CMD_NOTIME},
	{"invoke", "invoke an object's powers", 'V', M('i'), TRUE, doinvoke, CMD_ARG_NONE | CMD_EXT | CMD_ARG_OBJ},
	{"jump", "jump to a location", M('j'), 'j', FALSE, dojump, CMD_ARG_NONE | CMD_EXT},
	{"kick", "kick an adjacent object or monster", C('d'), 'k', FALSE, dokick, CMD_ARG_NONE},
	{"license", "show the DynaHack license", 0, 0, TRUE, dolicense, CMD_ARG_NONE | CMD_HELP | CMD_NOTIME},
	{"lookhere", "describe the current square", ':', 0, TRUE, dolook, CMD_ARG_NONE | CMD_NOTIME},
	{"loot", "loot a bag or box on the floor", M('l'), 'l', FALSE, doloot, CMD_ARG_NONE | CMD_EXT},
	{"menuinv", "show a partial inventory", 'I', 0, TRUE, dotypeinv, CMD_ARG_NONE | CMD_NOTIME},
	{"monster", "use a monster's special ability", 'M', M('m'), TRUE, domonability, CMD_ARG_NONE | CMD_EXT},
	{"multidrop", "drop multiple items", 'D', 0, FALSE, doddrop, CMD_ARG_NONE},
	{"name", "name a monster, item or type of object", M('n'), 'C', TRUE, do_naming, CMD_ARG_NONE | CMD_ARG_OBJ | CMD_EXT},
	{"name mon", "christen a monster", 0, 0, TRUE, do_mname, CMD_ARG_NONE},
	{"open", "open a door", 'o', 0, FALSE, doopen, CMD_ARG_NONE | CMD_ARG_DIR},
	{"overview", "show an overview of the dungeon", C('o'), 0, TRUE, dooverview, CMD_ARG_NONE | CMD_EXT | CMD_NOTIME},
	{"pay", "pay a shopkeeper", 'p', 0, FALSE, dopay, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"pickup", "take items from the floor", ',', 0, FALSE, dopickup, CMD_ARG_NONE},
	{"pray", "pray to the gods for help", M('p'), 0, TRUE, dopray, CMD_ARG_NONE | CMD_EXT},
	{"put on", "put on jewellery or accessories", 'P', 0, FALSE, doputon, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"quit", "exit without saving current game", M('q'), 0, TRUE, done2, CMD_ARG_NONE | CMD_EXT},
	{"quiver", "ready an item for firing", 'Q', 0, FALSE, dowieldquiver, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"read", "read a scroll or spellbook", 'r', 0, FALSE, doread, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"redraw", "redraw the screen", C('r'), C('l'), TRUE, doredraw, CMD_ARG_NONE | CMD_NOTIME},
	{"remove", "remove jewellery or accessories", 'R', 0, FALSE, doremring, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"removearm", "remove multiple pieces of equipment", 'A', 0, FALSE, doddoremarm, CMD_ARG_NONE},
	{"ride", "ride (or stop riding) a monster", 0, 0, FALSE, doride, CMD_ARG_NONE | CMD_EXT},
	{"rub", "rub a lamp or a stone", M('r'), 0, FALSE, dorub, CMD_ARG_NONE | CMD_EXT | CMD_ARG_OBJ},
	{"sacrifice", "offer a sacrifice to the gods", M('o'), 0, FALSE, dosacrifice, CMD_ARG_NONE | CMD_EXT | CMD_ARG_OBJ},
	{"save", "save the game and exit", 'S', 0, TRUE, dosave, CMD_ARG_NONE},
	{"search", "search for hidden doors and traps", 's', 0, TRUE, dosearch, CMD_ARG_NONE, "searching"},
	{"showamulets", "list the amulets in your inventory", '"', 0, TRUE, dopramulet, CMD_ARG_NONE | CMD_NOTIME},
	{"showarmor", "list the armor in your inventory", '[', 0, TRUE, doprarm, CMD_ARG_NONE | CMD_NOTIME},
	{"showrings", "list the rings in your inventory", '=', 0, TRUE, doprring, CMD_ARG_NONE | CMD_NOTIME},
	{"showtools", "list the tools in your inventory", '(', 0, TRUE, doprtool, CMD_ARG_NONE | CMD_NOTIME},
	{"showweapon", "list the weapons in your inventory", ')', 0, TRUE, doprwep, CMD_ARG_NONE | CMD_NOTIME},
	{"showworn", "show the items you are wearing", '*', 0, TRUE, doprinuse, CMD_ARG_NONE | CMD_NOTIME},
	{"sit", "sit down", M('s'), 0, FALSE, dosit, CMD_ARG_NONE | CMD_EXT},
	{"spellbook", "display and change letters of spells", '+', 0, TRUE, dovspell, CMD_ARG_NONE},
	{"swapweapon", "exchange wielded and alternate weapon", 'x', 0, FALSE, doswapweapon, CMD_ARG_NONE},
	{"takeoff", "take off an item you are wearing", 'T', 0, FALSE, dotakeoff, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"teleport", "use intrinsic or magical teleportation ability", C('t'), 0, TRUE, dotele, CMD_ARG_NONE},
	{"throw", "throw an item", 't', 0, FALSE, dothrow, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"tip", "empty a container of its contents", 0, 0, FALSE, dotip, CMD_ARG_NONE | CMD_ARG_OBJ | CMD_EXT},
	{"togglepickup", "toggle the autopickup option", '@', 0, TRUE, dotogglepickup, CMD_ARG_NONE},
	{"travel", "walk until a given square is reached", '_', 0, TRUE, dotravel, CMD_ARG_NONE | CMD_ARG_POS},
	{"turn", "turn undead", M('t'), 0, TRUE, doturn, CMD_ARG_NONE | CMD_EXT},
	{"tutorialreview", "review tutorial messages", C('v'), 0, TRUE, wiz_levport_or_tut_rev, CMD_HELP | CMD_ARG_NONE | CMD_NOTIME},
	{"twoweapon", "toggle two-weapon combat", M('2'), 'X', FALSE, dotwoweapon, CMD_ARG_NONE | CMD_EXT},
	{"untrap", "untrap something", M('u'), 'u', FALSE, dountrap, CMD_ARG_NONE | CMD_EXT},
	{"version", "show version number", 0, 0, TRUE, doversion, CMD_HELP | CMD_ARG_NONE | CMD_NOTIME | CMD_EXT},
	{"verhistory", "show version history", 0, 0, TRUE, doverhistory, CMD_HELP | CMD_ARG_NONE | CMD_NOTIME | CMD_EXT},
	{"wait", "do nothing for one turn", '.', 0, TRUE, donull, CMD_ARG_NONE, "waiting"},
	{"wear", "wear clothing or armor", 'W', 0, FALSE, dowear, CMD_ARG_NONE | CMD_ARG_OBJ},
	{"wield", "hold an item in your hands", 'w', 0, FALSE, dowield, CMD_ARG_NONE | CMD_ARG_OBJ | CMD_ZEROOBJ},
	{"wipe", "wipe off your face", M('w'), 0, FALSE, dowipe, CMD_ARG_NONE | CMD_EXT},
	{"whatis", "describe a symbol", '/', 0, TRUE, dowhatis, CMD_HELP | CMD_ARG_NONE | CMD_NOTIME},
	{"whatisinv", "describe an object in your inventory", 0, 0, TRUE, dowhatisinv, CMD_HELP | CMD_ARG_NONE | CMD_ARG_OBJ | CMD_NOTIME},
	{"zap", "zap a wand to use its magic", 'z', 0, FALSE, dozap, CMD_ARG_NONE | CMD_ARG_OBJ},
	
	{"move", "move one step", 0, 0, FALSE, domovecmd, CMD_ARG_DIR | CMD_MOVE},
	{"move nopickup", "move, but don't fight or pick anything up", 'm', 0, FALSE, domovecmd_nopickup, CMD_ARG_DIR | CMD_MOVE},
	{"run", "run until something interesting is seen", 0, 0, FALSE, dorun, CMD_ARG_DIR | CMD_MOVE},
	{"run nopickup", "run without picking anything up", 0, 'M', FALSE, dorun_nopickup, CMD_ARG_DIR | CMD_MOVE},
	{"go", "move, stopping for anything interesting", 'g', 0, FALSE, dogo, CMD_ARG_DIR | CMD_MOVE},
	{"go2", "like go, but branching corridors are boring", 'G', 0, FALSE, dogo2, CMD_ARG_DIR | CMD_MOVE},
	
	{"create monster", "(DEBUG) create a monster", C('g'), 0, TRUE, wiz_genesis, CMD_ARG_NONE | CMD_DEBUG},
	{"detect", "(DEBUG) detect monsters", 0, 0, TRUE, wiz_detect, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"identify", "(DEBUG) identify all items in the inventory", C('i'), 0, TRUE, wiz_identify, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"levelteleport", "(DEBUG) telport to a different level", 0, 0, TRUE, wiz_levport_or_tut_rev, CMD_ARG_NONE | CMD_DEBUG},
	{"levelchange", "(DEBUG) change experience level", 0, 0, TRUE, wiz_level_change, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"lightsources", "(DEBUG) show mobile light sources", 0, 0, TRUE, wiz_light_sources, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"mazewalkmap", "(DEBUG) show MAZEWALK paths", 0, 0, TRUE, wiz_mazewalkmap, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"monpolycontrol", "(DEBUG) control monster polymorphs", 0, 0, TRUE, wiz_mon_polycontrol, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"panic", "(DEBUG) test panic routine (fatal to game)", 0, 0, TRUE, wiz_panic, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"polyself", "(DEBUG) polymorph self", 0, 0, TRUE, wiz_polyself, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"printdungeon", "(DEBUG) print dungeon structure", 0, 0, TRUE, wiz_where, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"rooms", "(DEBUG) show room numbers and shared boundaries", 0, 0, TRUE, wiz_show_rooms, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"seenv", "(DEBUG) show seen vectors", 0, 0, TRUE, wiz_show_seenv, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"showmap", "(DEBUG) reveal the entire map", 0, 0, TRUE, wiz_map, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT},
	{"stats", "(DEBUG) show memory statistics", 0, 0, TRUE, wiz_show_stats, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"timeout", "(DEBUG) look at timeout queue", 0, 0, TRUE, wiz_timeout_queue, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"vision", "(DEBUG) show vision array", 0, 0, TRUE, wiz_show_vision, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	{"wish", "(DEBUG) wish for an item", C('w'), 0, TRUE, wiz_wish, CMD_ARG_NONE | CMD_DEBUG},
	{"wmode", "(DEBUG) show wall modes", 0, 0, TRUE, wiz_show_wmodes, CMD_ARG_NONE | CMD_DEBUG | CMD_EXT | CMD_NOTIME},
	
	{NULL, NULL, 0, 0, 0, 0, 0, NULL}
};


/* Count down by decrementing multi */
static int timed_occupation(void)
{
	(*timed_occ_fn)();
	if (multi > 0)
		multi--;
	return multi > 0;
}

/* If you have moved since initially setting some occupations, they
 * now shouldn't be able to restart.
 *
 * The basic rule is that if you are carrying it, you can continue
 * since it is with you.  If you are acting on something at a distance,
 * your orientation to it must have changed when you moved.
 *
 * The exception to this is taking off items, since they can be taken
 * off in a number of ways in the intervening time, screwing up ordering.
 *
 *	Currently:	Take off all armor.
 *			Picking Locks / Forcing Chests.
 *			Setting traps.
 */
void reset_occupations(void)
{
	reset_remarm();
	reset_pick();
	reset_trapset();
}

void save_occupations(struct memfile *mf)
{
	save_remarm(mf);
	save_pick(mf);
	save_trapset(mf);
}

void restore_occupations(struct memfile *mf, struct level *lev)
{
	restore_remarm(mf);
	restore_pick(mf, lev);
	restore_trapset(mf, lev);
}

/* If a time is given, use it to timeout this function, otherwise the
 * function times out by its own means.
 */
void set_occupation(int (*fn)(void), const char *txt, int xtime)
{
	if (xtime) {
		occupation = timed_occupation;
		timed_occ_fn = fn;
	} else
		occupation = fn;
	occtxt = txt;
	occtime = 0;
	return;
}


/* #monster command - use special monster ability while polymorphed */
static int domonability(void)
{
	if (can_breathe(youmonst.data)) return dobreathe();
	else if (attacktype(youmonst.data, AT_SPIT)) return dospit();
	else if (youmonst.data->mlet == S_NYMPH) return doremove();
	else if (attacktype(youmonst.data, AT_GAZE)) return dogaze();
	else if (is_were(youmonst.data)) return dosummon();
	else if (webmaker(youmonst.data)) return dospinweb();
	else if (is_hider(youmonst.data)) return dohide();
	else if (is_mind_flayer(youmonst.data)) return domindblast();
	else if (u.umonnum == PM_GREMLIN) {
	    if (IS_FOUNTAIN(level->locations[u.ux][u.uy].typ)) {
		if (split_mon(&youmonst, NULL))
		    dryup(u.ux, u.uy, TRUE);
	    } else pline("There is no fountain here.");
	} else if (is_unicorn(youmonst.data)) {
	    fix_attributes_and_properties(NULL, 0);
	    return 1;
	} else if (youmonst.data->msound == MS_SHRIEK) {
	    pline("You shriek.");
	    if (u.uburied)
		pline("Unfortunately sound does not carry well through rock.");
	    else aggravate();
	} else if (Upolyd)
		pline("Any special ability you may have is purely reflexive.");
	else pline("You don't have a special ability in your normal form!");
	return 0;
}

/* ^W command - wish for something */
static int wiz_wish(void)	/* Unlimited wishes for debug mode by Paul Polderman */
{
	if (wizard) {
	    boolean save_verbose = flags.verbose;

	    flags.verbose = FALSE;
	    makewish(TRUE);
	    flags.verbose = save_verbose;
	    encumber_msg();
	} else
	    pline("Unavailable command '^W'.");
	return 0;
}

/* ^I command - identify hero's inventory */
static int wiz_identify(void)
{
	if (wizard)	identify_pack(0);
	else		pline("Unavailable command '^I'.");
	return 0;
}

/* ^F command - reveal the level map and any traps on it */
static int wiz_map(void)
{
	if (wizard) {
	    struct trap *t;
	    long save_Hconf = HConfusion,
		 save_Hhallu = HHallucination;

	    HConfusion = HHallucination = 0L;
	    for (t = level->lev_traps; t != 0; t = t->ntrap) {
		t->tseen = 1;
		map_trap(t, TRUE);
	    }
	    do_mapping();
	    HConfusion = save_Hconf;
	    HHallucination = save_Hhallu;
	} else
	    pline("Unavailable command '^F'.");
	return 0;
}

/* ^G command - generate monster(s); a count prefix will be honored */
static int wiz_genesis(void)
{
	if (wizard)	create_particular();
	else		pline("Unavailable command '^G'.");
	return 0;
}

/* ^O command - display dungeon layout */
static int wiz_where(void)
{
	if (wizard) print_dungeon(FALSE, NULL, NULL);
	else	    pline("Unavailable command '^O'.");
	return 0;
}

/* ^E command - detect unseen (secret doors, traps, hidden monsters) */
static int wiz_detect(void)
{
	if (wizard)  findit();
	else	    pline("Unavailable command '^E'.");
	return 0;
}

/* ^V command - level teleport (wizard), or tutorial review */
static int wiz_levport_or_tut_rev(void)
{
	if (wizard)
	    level_tele();
	else if (flags.tutorial)
	    tutorial_redisplay();
	else
	    pline("Tutorial review is only available in tutorial mode.");
	return 0;
}

/* #monpolycontrol command - choose new form for shapechangers, polymorphees */
static int wiz_mon_polycontrol(void)
{
    iflags.mon_polycontrol = !iflags.mon_polycontrol;
    pline("Monster polymorph control is %s.",
	  iflags.mon_polycontrol ? "on" : "off");
    return 0;
}

/* #levelchange command - adjust hero's experience level */
static int wiz_level_change(void)
{
    char buf[BUFSZ];
    int newlevel;
    int ret;

    getlin("To what experience level do you want to be set?", buf);
    mungspaces(buf);
    if (buf[0] == '\033' || buf[0] == '\0') ret = 0;
    else ret = sscanf(buf, "%d", &newlevel);

    if (ret != 1) {
	pline("Never mind.");
	return 0;
    }
    if (newlevel == u.ulevel) {
	pline("You are already that experienced.");
    } else if (newlevel < u.ulevel) {
	if (u.ulevel == 1) {
	    pline("You are already as inexperienced as you can get.");
	    return 0;
	}
	if (newlevel < 1) newlevel = 1;
	while (u.ulevel > newlevel)
	    losexp("#levelchange");
    } else {
	if (u.ulevel >= MAXULEV) {
	    pline("You are already as experienced as you can get.");
	    return 0;
	}
	if (newlevel > MAXULEV) newlevel = MAXULEV;
	while (u.ulevel < newlevel)
	    pluslvl(FALSE);
    }
    u.ulevelmax = u.ulevel;
    return 0;
}

/* #panic command - test program's panic handling */
static int wiz_panic(void)
{
	if (yn("Do you want to call panic() and end your game?") == 'y')
		panic("crash test.");
        return 0;
}

/* #polyself command - change hero's form */
static int wiz_polyself(void)
{
	polyself(TRUE);
	return 0;
}

/* #seenv command */
static int wiz_show_seenv(void)
{
	struct menulist menu;
	int x, y, v, startx, stopx, curx;
	char row[COLNO+1];

	init_menulist(&menu);
	/*
	 * Each seenv description takes up 2 characters, so center
	 * the seenv display around the hero.
	 */
	startx = max(1, u.ux-(COLNO/4));
	stopx = min(startx+(COLNO/2), COLNO);
	/* can't have a line exactly 80 chars long */
	if (stopx - startx == COLNO/2) startx++;

	for (y = 0; y < ROWNO; y++) {
	    for (x = startx, curx = 0; x < stopx; x++, curx += 2) {
		if (x == u.ux && y == u.uy) {
		    row[curx] = row[curx+1] = '@';
		} else {
		    v = level->locations[x][y].seenv & 0xff;
		    if (v == 0)
			row[curx] = row[curx+1] = ' ';
		    else
			sprintf(&row[curx], "%02x", v);
		}
	    }
	    /* remove trailing spaces */
	    for (x = curx-1; x >= 0; x--)
		if (row[x] != ' ') break;
	    row[x+1] = '\0';

	    add_menutext(&menu, row);
	}
	display_menu(menu.items, menu.icount, NULL, PICK_NONE, NULL);
	free(menu.items);
	return 0;
}

/* #vision command */
static int wiz_show_vision(void)
{
	struct menulist menu;
	int x, y, v;
	char row[COLNO+1];

	init_menulist(&menu);
	sprintf(row, "Flags: 0x%x could see, 0x%x in sight, 0x%x temp lit",
		COULD_SEE, IN_SIGHT, TEMP_LIT);
	add_menutext(&menu, row);
	add_menutext(&menu, "");
	for (y = 0; y < ROWNO; y++) {
	    for (x = 1; x < COLNO; x++) {
		if (x == u.ux && y == u.uy)
		    row[x] = '@';
		else {
		    v = viz_array[y][x]; /* data access should be hidden */
		    if (v == 0)
			row[x] = ' ';
		    else
			row[x] = '0' + viz_array[y][x];
		}
	    }
	    /* remove trailing spaces */
	    for (x = COLNO-1; x >= 1; x--)
		if (row[x] != ' ') break;
	    row[x+1] = '\0';

	    add_menutext(&menu, &row[1]);
	}
	display_menu(menu.items, menu.icount, NULL, PICK_NONE, NULL);
	free(menu.items);
	return 0;
}

/* #wmode command */
static int wiz_show_wmodes(void)
{
	struct menulist menu;
	int x,y;
	char row[COLNO+1];
	struct rm *loc;

	init_menulist(&menu);
	for (y = 0; y < ROWNO; y++) {
	    for (x = 0; x < COLNO; x++) {
		loc = &level->locations[x][y];
		if (x == u.ux && y == u.uy)
		    row[x] = '@';
		else if (IS_WALL(loc->typ) || loc->typ == SDOOR)
		    row[x] = '0' + (loc->wall_info & WM_MASK);
		else if (loc->typ == CORR)
		    row[x] = '#';
		else if (IS_ROOM(loc->typ) || IS_DOOR(loc->typ))
		    row[x] = '.';
		else
		    row[x] = 'x';
	    }
	    row[COLNO] = '\0';
	    add_menutext(&menu, row);
	}
	display_menu(menu.items, menu.icount, NULL, PICK_NONE, NULL);
	free(menu.items);
	return 0;
}

/* #mazewalkmap command */
static int wiz_mazewalkmap(void)
{
	struct menulist menu;
	int x, y;
	char row[COLNO+1];

	init_menulist(&menu);

	for (y = 0; y < ROWNO; y++) {
	    for (x = 0; x < COLNO; x++)
		row[x] = SpLev_Map[x][y] ? '1' : '.';
	    if (y == u.uy)
		row[u.ux] = '@';
	    row[x] = '\0';
	    add_menutext(&menu, row);
	}
	display_menu(menu.items, menu.icount, NULL, PICK_NONE, NULL);
	free(menu.items);
	return 0;
}

/* #rooms command */
static int wiz_show_rooms(void)
{
	struct menulist menu;
	int x, y;
	char row[COLNO+1];

	init_menulist(&menu);

	for (y = 0; y < ROWNO; y++) {
	    for (x = 0; x < COLNO; x++) {
		int rno = level->locations[x][y].roomno;
		if (rno == NO_ROOM) {
		    row[x] = '.';
		} else if (rno == SHARED) {
		    row[x] = '+';
		} else if (rno == SHARED_PLUS) {
		    row[x] = '*';
		} else {
		    int i = (rno - ROOMOFFSET) % 52;
		    row[x] = (i < 26) ? ('a' + i) : ('A' + i);
		}
	    }
	    row[COLNO] = '\0';
	    add_menutext(&menu, row);
	}
	display_menu(menu.items, menu.icount, NULL, PICK_NONE, NULL);
	free(menu.items);
	return 0;
}


/* -enlightenment and conduct- */
static const char
	You_[] = "You ",
	are[]  = "are ",  were[]  = "were ",
	have[] = "have ", had[]   = "had ",
	can[]  = "can ",  could[] = "could ";
static const char
	have_been[]  = "have been ",
	have_never[] = "have never ", never[] = "never ";

#define enl_msg(menu,prefix,present,past,suffix) \
			enlght_line(menu,prefix, final ? past : present, suffix)
#define you_are(menu,attr)	enl_msg(menu,You_,are,were,attr)
#define you_have(menu,attr)	enl_msg(menu,You_,have,had,attr)
#define you_can(menu,attr)	enl_msg(menu,You_,can,could,attr)
#define you_resist(menu,attr)	enl_msg(menu,"You resist"," ","ed ",attr)
#define you_have_been(menu,goodthing) enl_msg(menu,You_,have_been,were,goodthing)
#define you_have_never(menu,badthing) enl_msg(menu,You_,have_never,never,badthing)
#define you_have_X(menu,something)	enl_msg(menu,You_,have,(const char *)"", something)

static void enlght_line(struct menulist *menu,
			const char *start, const char *middle, const char *end)
{
	char buf[BUFSZ];

	sprintf(buf, "%s%s%s.", start, middle, end);
	add_menutext(menu, buf);
}

/* format increased damage or chance to hit */
static char *enlght_combatinc(const char *inctyp, int incamt, int final, char *outbuf)
{
	char numbuf[24];
	const char *modif, *bonus;

	if (final || wizard) {
	    sprintf(numbuf, "%s%d",
		    (incamt > 0) ? "+" : "", incamt);
	    modif = (const char *) numbuf;
	} else {
	    int absamt = abs(incamt);

	    if (absamt <= 3) modif = "small";
	    else if (absamt <= 6) modif = "moderate";
	    else if (absamt <= 12) modif = "large";
	    else modif = "huge";
	}
	bonus = (incamt > 0) ? "bonus" : "penalty";
	/* "bonus to hit" vs "damage bonus" */
	if (!strcmp(inctyp, "damage")) {
	    const char *ctmp = inctyp;
	    inctyp = bonus;
	    bonus = ctmp;
	}
	sprintf(outbuf, "%s %s %s", an(modif), bonus, inctyp);
	return outbuf;
}


void enlightenment(int final)
    /* final: 0 => still in progress; 1 => over, survived; 2 => dead */
{
	int ltmp;
	char buf[BUFSZ], *title;
	struct menulist menu;

	init_menulist(&menu);
	title = final ? "Final Attributes:" : "Current Attributes:";

	if (flags.elbereth_enabled && u.uevent.uhand_of_elbereth) {
	    static const char * const hofe_titles[3] = {
				"the Hand of Elbereth",
				"the Envoy of Balance",
				"the Glory of Arioch"
	    };
	    you_are(&menu, hofe_titles[u.uevent.uhand_of_elbereth - 1]);
	}

	/* note: piousness 20 matches MIN_QUEST_ALIGN (quest.h) */
	if (u.ualign.record >= 20)	you_are(&menu, "piously aligned");
	else if (u.ualign.record > 13)	you_are(&menu, "devoutly aligned");
	else if (u.ualign.record > 8)	you_are(&menu, "fervently aligned");
	else if (u.ualign.record > 3)	you_are(&menu, "stridently aligned");
	else if (u.ualign.record == 3)	you_are(&menu, "aligned");
	else if (u.ualign.record > 0)	you_are(&menu, "haltingly aligned");
	else if (u.ualign.record == 0)	you_are(&menu, "nominally aligned");
	else if (u.ualign.record >= -3)	you_have(&menu, "strayed");
	else if (u.ualign.record >= -8)	you_have(&menu, "sinned");
	else you_have(&menu, "transgressed");

	if (wizard || final) {
		sprintf(buf, " %d", u.uhunger);
		enl_msg(&menu, "Hunger level ", "is", "was", buf);

		sprintf(buf, " %d / %ld", u.ualign.record, ALIGNLIM);
		enl_msg(&menu, "Your alignment ", "is", "was", buf);

		sprintf(buf, " %d - %d",
			min_monster_difficulty(&u.uz),
			max_monster_difficulty(&u.uz));
		enl_msg(&menu, "Monster difficulty range ", "is", "was", buf);
		sprintf(buf, " %d", level_difficulty(&u.uz));
		enl_msg(&menu, "Level difficulty ", "is", "was", buf);
	}

	/*** Resistances to troubles ***/
	if (PFire_resistance) you_resist(&menu, "fire");
	else if (FFire_resistance) you_are(&menu, "immune to fire");

	if (PCold_resistance) you_resist(&menu, "cold");
	else if (FCold_resistance) you_are(&menu, "immune to cold");

	if (PSleep_resistance) you_resist(&menu, "sleep");
	else if (FSleep_resistance) you_are(&menu, "immune to sleep");

	if (PDisint_resistance) you_resist(&menu, "disintegration");
	else if (FDisint_resistance) you_are(&menu, "immune to disintegration");

	if (PShock_resistance) you_resist(&menu, "electricity");
	else if (FShock_resistance) you_are(&menu, "immune to electricity");

	if (PPoison_resistance) you_resist(&menu, "poison");
	else if (FPoison_resistance) you_are(&menu, "immune to poison");

	if (Drain_resistance) you_are(&menu, "immune to level-drain");
	if (Halluc_resistance) you_resist(&menu, "hallucinations");
	if (Sick_resistance) you_are(&menu, "immune to sickness");
	if (Antimagic) you_are(&menu, "magic-protected");
	if (Acid_resistance) you_are(&menu, "immune to acid");
	if (Stone_resistance)
		you_are(&menu, "immune to petrification");
	if (Half_physical_damage) you_resist(&menu, "physical damage");
	if (Half_spell_damage) you_resist(&menu, "spell damage");
	if (Invulnerable) you_are(&menu, "invulnerable");
	if (u.uedibility) you_can(&menu, "recognize detrimental food");

	/*** Troubles ***/
	if (final) {
		if (Hallucination) you_are(&menu, "hallucinating");
		if (Stunned) you_are(&menu, "stunned");
		if (Confusion) you_are(&menu, "confused");
		if (Blinded) you_are(&menu, "blinded");
		if (Sick) {
			if (u.usick_type & SICK_VOMITABLE)
				you_are(&menu, "sick from food poisoning");
			if (u.usick_type & SICK_NONVOMITABLE)
				you_are(&menu, "sick from illness");
		}
		if (Punished)
			you_are(&menu, "punished");
	}
	if (Stoned) you_are(&menu, "turning to stone");
	if (Slimed) you_are(&menu, "turning into slime");
	if (Strangled) you_are(&menu, (u.uburied) ? "buried" : "being strangled");
	if (Glib) {
		sprintf(buf, "slippery %s", makeplural(body_part(FINGER)));
		you_have(&menu, buf);
	}
	if (Fumbling) enl_msg(&menu, "You fumble", "", "d", "");
	if (Wounded_legs && !u.usteed) {
		sprintf(buf, "wounded %s", makeplural(body_part(LEG)));
		you_have(&menu, buf);
	}
	if (Wounded_legs && u.usteed && (wizard || final)) {
	    strcpy(buf, x_monnam(u.usteed, ARTICLE_YOUR, NULL,
		    SUPPRESS_SADDLE | SUPPRESS_HALLUCINATION, FALSE));
	    *buf = highc(*buf);
	    enl_msg(&menu, buf, " has", " had", " wounded legs");
	}
	
	if (Sleeping) enl_msg(&menu, "You ", "fall", "fell", " asleep");
	if (Hunger) enl_msg(&menu, "You hunger", "", "ed", " rapidly");

	/*** Vision and senses ***/
	if (See_invisible) enl_msg(&menu, You_, "see", "saw", " invisible");
	if (Blind_telepat) you_are(&menu, "telepathic");
	if (Warning) you_are(&menu, "warned");
	if (Warn_of_mon && flags.warntype) {
		sprintf(buf, "aware of the presence of %s",
			(flags.warntype & M2_ORC) ? "orcs" :
			(flags.warntype & M2_DEMON) ? "demons" :
			(flags.warntype & M2_GIANT) ? "giants" :
			(flags.warntype & M2_WERE) ? "lycanthropes" :
			(flags.warntype & M2_UNDEAD) ? "undead" :
			"something");
		you_are(&menu, buf);
	} else if (Warn_of_mon && uwep) {
		const char *monster_name = get_warned_of_monster(uwep);
		if (monster_name) {
			sprintf(buf, "aware of the presence of %s", monster_name);
			you_are(&menu, buf);
		}
	}
	if (Undead_warning) you_are(&menu, "warned of undead");
	if (Searching) you_have(&menu, "automatic searching");
	if (Clairvoyant) you_are(&menu, "clairvoyant");
	if (Infravision) you_have(&menu, "infravision");
	if (Detect_monsters) you_are(&menu, "sensing the presence of monsters");
	if (u.umconf) you_are(&menu, "going to confuse monsters");

	/*** Appearance and behavior ***/
	if (Adornment) {
	    int adorn = 0;

	    if (uleft && uleft->otyp == RIN_ADORNMENT) adorn += uleft->spe;
	    if (uright && uright->otyp == RIN_ADORNMENT) adorn += uright->spe;
	    if (adorn < 0)
		you_are(&menu, "poorly adorned");
	    else
		you_are(&menu, "adorned");
	}
	if (Invisible) you_are(&menu, "invisible");
	else if (Invis) you_are(&menu, "invisible to others");
	/* ordinarily "visible" is redundant; this is a special case for
	   the situation when invisibility would be an expected attribute */
	else if ((HInvis || EInvis || pm_invisible(youmonst.data)) && BInvis)
	    you_are(&menu, "visible");
	if (Displaced) you_are(&menu, "displaced");
	if (Stealth) you_are(&menu, "stealthy");
	if (Aggravate_monster) enl_msg(&menu, "You aggravate", "", "d", " monsters");
	if (Conflict) enl_msg(&menu, "You cause", "", "d", " conflict");

	/*** Transportation ***/
	if (Jumping) you_can(&menu, "jump");
	if (Teleportation) you_can(&menu, "teleport");
	if (Teleport_control) you_have(&menu, "teleport control");
	if (Lev_at_will) you_are(&menu, "levitating, at will");
	else if (Levitation) you_are(&menu, "levitating");	/* without control */
	else if (Flying) you_can(&menu, "fly");
	if (Wwalking) you_can(&menu, "walk on water");
	if (Swimming) you_can(&menu, "swim");
	if (Breathless) you_can(&menu, "survive without air");
	else if (Amphibious) you_can(&menu, "breathe water");
	if (Passes_walls) you_can(&menu, "walk through walls");
	
	/* If you die while dismounting, u.usteed is still set.  Since several
	 * places in the done() sequence depend on u.usteed, just detect this
	 * special case. */
	if (u.usteed && (final < 2 || strcmp(killer, "riding accident"))) {
	    sprintf(buf, "riding %s", y_monnam(u.usteed));
	    you_are(&menu, buf);
	}
	if (u.uswallow) {
	    sprintf(buf, "swallowed by %s", a_monnam(u.ustuck));
	    if (wizard || final) sprintf(eos(buf), " (%u)", u.uswldtim);
	    you_are(&menu, buf);
	} else if (u.ustuck) {
	    sprintf(buf, "%s %s",
		    (Upolyd && sticks(youmonst.data)) ? "holding" : "held by",
		    a_monnam(u.ustuck));
	    you_are(&menu, buf);
	}

	/*** Physical attributes ***/
	if (u.uhitinc)
	    you_have(&menu, enlght_combatinc("to hit", u.uhitinc, final, buf));
	if (u.udaminc)
	    you_have(&menu, enlght_combatinc("damage", u.udaminc, final, buf));
	if (Slow_digestion) you_have(&menu, "slower digestion");
	if (Regeneration) enl_msg(&menu, "You regenerate", "", "d", "");
	if (u.uspellprot || Protection) {
	    int prot = 0;

	    if (uleft && uleft->otyp == RIN_PROTECTION) prot += uleft->spe;
	    if (uright && uright->otyp == RIN_PROTECTION) prot += uright->spe;
	    if (HProtection & INTRINSIC) prot += u.ublessed;
	    prot += u.uspellprot;

	    if (prot < 0)
		you_are(&menu, "ineffectively protected");
	    else if (prot > 0)
		you_are(&menu, "protected");
	}
	if (Protection_from_shape_changers)
		you_are(&menu, "protected from shape changers");
	if (Polymorph) you_are(&menu, "polymorphing");
	if (Polymorph_control) you_have(&menu, "polymorph control");
	if (u.ulycn >= LOW_PM) {
		strcpy(buf, an(mons_mname(&mons[u.ulycn])));
		you_are(&menu, buf);
	}
	if (Upolyd) {
	    if (u.umonnum == u.ulycn) strcpy(buf, "in beast form");
	    else sprintf(buf, "polymorphed into %s", an(mons_mname(youmonst.data)));
	    if (wizard || final) sprintf(eos(buf), " (%d)", u.mtimedone);
	    you_are(&menu, buf);
	}
	if (Unchanging) you_can(&menu, "not change from your current form");
	if (Fast) you_are(&menu, Very_fast ? "very fast" : "fast");
	if (Reflecting) you_have(&menu, "reflection");
	if (Free_action) you_have(&menu, "free action");
	if (Fixed_abil) you_have(&menu, "fixed abilities");
	if (Lifesaved)
		enl_msg(&menu, "Your life ", "will be", "would have been", " saved");
	if (u.twoweap) you_are(&menu, "wielding two weapons at once");

	/*** Miscellany ***/
	if (Luck) {
	    ltmp = abs((int)Luck);
	    sprintf(buf, "%s%slucky",
		    ltmp >= 10 ? "extremely " : ltmp >= 5 ? "very " : "",
		    Luck < 0 ? "un" : "");
	    if (wizard || final) sprintf(eos(buf), " (%d)", Luck);
	    you_are(&menu, buf);
	} else if (wizard || final) {
	    enl_msg(&menu, "Your luck ", "is", "was", " zero");
	}
	if (u.moreluck > 0) you_have(&menu, "extra luck");
	else if (u.moreluck < 0) you_have(&menu, "reduced luck");
	if (carrying(LUCKSTONE) || stone_luck(TRUE)) {
	    ltmp = stone_luck(FALSE);
	    if (ltmp <= 0)
		enl_msg(&menu, "Bad luck ", "does", "did", " not time out for you");
	    if (ltmp >= 0)
		enl_msg(&menu, "Good luck ", "does", "did", " not time out for you");
	}

	if (u.ugangr) {
	    sprintf(buf, " %sangry with you",
		    u.ugangr > 6 ? "extremely " : u.ugangr > 3 ? "very " : "");
	    if (wizard || final) sprintf(eos(buf), " (%d)", u.ugangr);
	    enl_msg(&menu, u_gname(), " is", " was", buf);
	} else
	    /*
	     * We need to suppress this when the game is over, because death
	     * can change the value calculated by can_pray(), potentially
	     * resulting in a false claim that you could have prayed safely.
	     */
	  if (!final) {
	    sprintf(buf, "%ssafely pray", can_pray(FALSE) ? "" : "not ");
	    if (wizard) sprintf(eos(buf), " (%d)", u.ublesscnt);
	    you_can(&menu, buf);
	}

    {
	const char *p;

	buf[0] = '\0';
	if (final < 2) {    /* still in progress, or quit/escaped/ascended */
	    p = "survived after being killed ";
	    switch (u.umortality) {
	    case 0:  p = !final ? NULL : "survived";  break;
	    case 1:  strcpy(buf, "once");  break;
	    case 2:  strcpy(buf, "twice");  break;
	    case 3:  strcpy(buf, "thrice");  break;
	    default: sprintf(buf, "%d times", u.umortality);
		     break;
	    }
	} else {		/* game ended in character's death */
	    p = "are dead";
	    switch (u.umortality) {
	    case 0:  warning("dead without dying?");
	    case 1:  break;			/* just "are dead" */
	    default: sprintf(buf, " (%d%s time!)", u.umortality,
			     ordin(u.umortality));
		     break;
	    }
	}
	if (p) enl_msg(&menu, You_, "have been killed ", p, buf);
    }

	display_menu(menu.items, menu.icount, title, PICK_NONE, NULL);
	free(menu.items);
	return;
}

static void unspoilered_intrinsics(void)
{
	int n;
	struct menulist menu;

	init_menulist(&menu);

	/* Intrinsic list
	   This lists only intrinsics that produce messages when gained
	   and/or lost, to avoid giving away information not in vanilla
	   NetHack. */
	n = menu.icount;

	/* Resistances */
	if (HFire_resistance & ~FROMOUTSIDE)
	    add_menutext(&menu, "You are immune to fire.");
	else if (HFire_resistance & FROMOUTSIDE)
	    add_menutext(&menu, "You resist fire.");

	if (HCold_resistance & ~FROMOUTSIDE)
	    add_menutext(&menu, "You are immune to cold.");
	else if (HCold_resistance & FROMOUTSIDE)
	    add_menutext(&menu, "You resist cold.");

	if (HSleep_resistance & ~FROMOUTSIDE)
	    add_menutext(&menu, "You are immune to sleep.");
	else if (HSleep_resistance & FROMOUTSIDE)
	    add_menutext(&menu, "You resist sleep.");

	if (HDisint_resistance & ~FROMOUTSIDE)
	    add_menutext(&menu, "You are immune to disintegration.");
	else if (HDisint_resistance & FROMOUTSIDE)
	    add_menutext(&menu, "You resist disintegration.");

	if (HShock_resistance & ~FROMOUTSIDE)
	    add_menutext(&menu, "You are immune to electricity.");
	else if (HShock_resistance & FROMOUTSIDE)
	    add_menutext(&menu, "You resist electricity.");

	if (HPoison_resistance & ~FROMOUTSIDE)
	    add_menutext(&menu, "You are immune to poison.");
	else if (HPoison_resistance & FROMOUTSIDE)
	    add_menutext(&menu, "You resist poison.");

	if (HDrain_resistance) add_menutext(&menu, "You are immune to level-drain.");
	if (HSick_resistance) add_menutext(&menu, "You are immune to sickness.");

	/* Senses */
	if (HSee_invisible) add_menutext(&menu, "You see invisible.");
	if (HTelepat) add_menutext(&menu, "You are telepathic.");
	if (HWarning) add_menutext(&menu, "You are warned.");
	if (HSearching) add_menutext(&menu, "You have automatic searching.");
	if (HInfravision) add_menutext(&menu, "You have infravision.");

	/* Appearance, behaviour */
	if (HInvis && Invisible) add_menutext(&menu, "You are invisible.");
	if (HInvis && !Invisible) add_menutext(&menu, "You are invisible to others.");
	if (HStealth) add_menutext(&menu, "You are stealthy.");
	if (HAggravate_monster) add_menutext(&menu, "You aggravte monsters.");
	if (HConflict) add_menutext(&menu, "You cause conflict.");

	/* Movement */
	if (HJumping) add_menutext(&menu, "You can jump.");
	if (HTeleportation) add_menutext(&menu, "You can teleport.");
	if (HTeleport_control) add_menutext(&menu, "You have teleport control.");
	if (HSwimming) add_menutext(&menu, "You can swim.");
	if (HMagical_breathing) add_menutext(&menu, "You can survive without air.");
	if (HPasses_walls) add_menutext(&menu, "You can walk through walls.");
	if (HProtection) add_menutext(&menu, "You are protected.");
	if (HPolymorph) add_menutext(&menu, "You are polymorhing.");
	if (HPolymorph_control) add_menutext(&menu, "You have polymorph control.");
	if (HFast) add_menutext(&menu, "You are fast.");
	if (n == menu.icount) add_menutext(&menu, "You are rather mundane.");

	display_menu(menu.items, menu.icount, "Your Intrinsics:", PICK_NONE, NULL);
	free(menu.items);
}

/*
 * Courtesy function for non-debug, non-explorer mode players
 * to help refresh them about who/what they are.
 * Returns FALSE if menu cancelled (dismissed with ESC), TRUE otherwise.
 */
static boolean minimal_enlightenment(void)
{
	int genidx, n, selected[1];
	char buf[BUFSZ], buf2[BUFSZ];
	static const char fmtstr[] = "%-10s: %-12s (started %s)";
	static const char fmtstr_noorig[] = "%-10s: %-12s";
	static const char deity_fmtstr[] = "%-17s%s";
	struct menulist menu;

	init_menulist(&menu);

	buf[0] = buf2[0] = '\0';
	add_menuheading(&menu, "Character Information");

	/* Starting and current name, race, role, gender, alignment */
	sprintf(buf, fmtstr_noorig, "name", plname);
	add_menutext(&menu, buf);

	if (Upolyd) {
	    sprintf(buf, fmtstr, "race", mons_mname(youmonst.data), urace.noun);
	} else {
	    sprintf(buf, fmtstr_noorig, "race", urace.noun);
	}
	add_menutext(&menu, buf);

	sprintf(buf, fmtstr_noorig, "role",
		((Upolyd ? u.mfemale : flags.female) && urole.name.f) ?
		urole.name.f : urole.name.m);
	add_menutext(&menu, buf);

	/* don't want poly_gender() here; it forces `2' for non-humanoids */
	genidx = is_neuter(youmonst.data) ? 2 : flags.female;
	if (genidx != u.initgend) {
	    sprintf(buf, fmtstr, "gender", genders[genidx].adj,
		    genders[u.initgend].adj);
	} else {
	    sprintf(buf, fmtstr_noorig, "gender", genders[genidx].adj);
	}
	add_menutext(&menu, buf);

	if (u.ualign.type != u.ualignbase[A_ORIGINAL]) {
	    sprintf(buf, fmtstr, "alignment", align_str(u.ualign.type),
		    align_str(u.ualignbase[A_ORIGINAL]));
	} else {
	    sprintf(buf, fmtstr_noorig, "alignment", align_str(u.ualign.type));
	}
	add_menutext(&menu, buf);

	if (ACURR(A_STR) > 18) {
	    if (ACURR(A_STR) > STR18(100))
		sprintf(buf, "attributes: St:%2d ", ACURR(A_STR) - 100);
	    else if (ACURR(A_STR) < STR18(100))
		sprintf(buf, "attributes: St:18/%02d ", ACURR(A_STR) - 18);
	    else
		sprintf(buf, "attributes: St:18/** ");
	} else {
	    sprintf(buf, "attributes: St:%-1d ", ACURR(A_STR));
	}
	sprintf(eos(buf), "Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
		ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS), ACURR(A_CHA));
	add_menutext(&menu, buf);

	if (u.ulevel < 30) {
	    sprintf(buf, "%-10s: %d (exp: %d, %ld needed)", "level",
		    u.ulevel, u.uexp, newuexp(u.ulevel));
	} else {
	    sprintf(buf, "%-10s: %d (exp: %d)", "level",
		    u.ulevel, u.uexp);
	}
	add_menutext(&menu, buf);

	/* Current position of hero */
	if (wizard) {
	    sprintf(buf2, "(%2d,%2d)", u.ux, u.uy);
	    sprintf(buf, fmtstr_noorig, "position", buf2);
	    add_menutext(&menu, buf);
	}

	/* Deity list */
	add_menutext(&menu, "");
	sprintf(buf, "%s Deities",
		((Upolyd ? u.mfemale : flags.female) && urole.name.f) ?
		urole.name.f : urole.name.m);
	add_menuheading(&menu, buf);

	sprintf(buf2, deity_fmtstr, align_gname(A_CHAOTIC),
	    (u.ualignbase[A_ORIGINAL] == u.ualign.type
		&& u.ualign.type == A_CHAOTIC) ? " (s,c)" :
	    (u.ualignbase[A_ORIGINAL] == A_CHAOTIC)       ? " (s)" :
	    (u.ualign.type   == A_CHAOTIC)       ? " (c)" : "");
	sprintf(buf, fmtstr_noorig, "Chaotic", buf2);
	add_menutext(&menu, buf);

	sprintf(buf2, deity_fmtstr, align_gname(A_NEUTRAL),
	    (u.ualignbase[A_ORIGINAL] == u.ualign.type
		&& u.ualign.type == A_NEUTRAL) ? " (s,c)" :
	    (u.ualignbase[A_ORIGINAL] == A_NEUTRAL)       ? " (s)" :
	    (u.ualign.type   == A_NEUTRAL)       ? " (c)" : "");
	sprintf(buf, fmtstr_noorig, "Neutral", buf2);
	add_menutext(&menu, buf);

	sprintf(buf2, deity_fmtstr, align_gname(A_LAWFUL),
	    (u.ualignbase[A_ORIGINAL] == u.ualign.type &&
		u.ualign.type == A_LAWFUL)  ? " (s,c)" :
	    (u.ualignbase[A_ORIGINAL] == A_LAWFUL)        ? " (s)" :
	    (u.ualign.type   == A_LAWFUL)        ? " (c)" : "");
	sprintf(buf, fmtstr_noorig, "Lawful", buf2);
	add_menutext(&menu, buf);

	add_menutext(&menu, "");
	add_menuheading(&menu, "Other Information");
	add_menuitem(&menu, 'i', "Inventory", 'i', FALSE);
	add_menuitem(&menu, 's', "Skills", 's', FALSE);
	add_menuitem(&menu, 'n', "Intrinsics", 'n', FALSE);
	if (num_vanquished() > 0)
	    add_menuitem(&menu, 'v', "Vanquished creatures", 'v', FALSE);
	if (num_genocides() > 0 || num_extinctions() > 0)
	    add_menuitem(&menu, 'g', "Genocided/extinct creatures", 'g', FALSE);
	add_menuitem(&menu, 'c', "Conducts followed", 'c', FALSE);
	if (wizard || discover)
	    add_menuitem(&menu, 'w', "Debug/explore mode spoilers", 'w', FALSE);

	n = display_menu(menu.items, menu.icount, "Your Attributes:",
			 PICK_ONE, selected);

	if (n == 1) {
	    switch (selected[0]) {
	    case 'i': ddoinv(); break;
	    case 's': enhance_weapon_skill(); break;
	    case 'n': unspoilered_intrinsics(); break;
	    case 'v': list_vanquished('y', FALSE); break;
	    case 'g': list_genocided('y', FALSE); break;
	    case 'c': doconduct(); break;
	    case 'w': if (wizard || discover) enlightenment(0); break;
	    }
	}

	free(menu.items);
	return n != -1;
}

static int doattributes(void)
{
	minimal_enlightenment();
	return 0;
}

/* KMH, #conduct
 * (shares enlightenment's tense handling)
 */
static int doconduct(void)
{
	show_conduct(0);
	return 0;
}

void show_conduct(int final)
{
	char buf[BUFSZ];
	int ngenocided;
	int cdt;
	struct menulist menu;

	/* Create the conduct window */
	init_menulist(&menu);

	/* List all major conducts. */
	for (cdt = FIRST_CONDUCT; cdt <= LAST_CONDUCT; cdt++) {
	    if (successful_cdt(cdt)) {
		if (!superfluous_cdt(cdt)) {
		    enl_msg(&menu,
			    conducts[cdt].prefix,	/* "You "       */
			    conducts[cdt].presenttxt,	/* "have been"  */
			    conducts[cdt].pasttxt,	/* "were"       */
			    conducts[cdt].suffix);	/* "a pacifist" */
		}
	    } else if (intended_cdt(cdt)) {
		/* "pretended to be a pacifist" */
		you_have_X(&menu, conducts[cdt].failtxt);
	    }
	}
	if (failed_cdt(CONDUCT_PACIFISM) || failed_cdt(CONDUCT_SADISM)) {
	    if (u.uconduct.killer == 0) {
		you_have_never(&menu, "killed a creature");
	    } else {
		sprintf(buf, "killed %u creature%s", u.uconduct.killer,
			plur(u.uconduct.killer));
		you_have_X(&menu, buf);
	    }
	}

	/* Now list the remaining statistical details. */

	if (!u.uconduct.weaphit)
	    you_have_never(&menu, "hit with a wielded weapon");
	else if (wizard || final) {
	    sprintf(buf, "used a wielded weapon %u time%s",
		    u.uconduct.weaphit, plur(u.uconduct.weaphit));
	    you_have_X(&menu, buf);
	}

	if ((wizard || final) && u.uconduct.literate) {
	    sprintf(buf, "read items or engraved %u time%s",
		    u.uconduct.literate, plur(u.uconduct.literate));
	    you_have_X(&menu, buf);
	}

	if ((wizard || final) && u.uconduct.armoruses) {
	    sprintf(buf, "put on armor %u time%s",
		    u.uconduct.armoruses, plur(u.uconduct.armoruses));
	    you_have_X(&menu, buf);
	}

	if (!u.uconduct.unracearmor && u.uconduct.armoruses > 0) {
	    sprintf(buf, "wearing only %s armor", urace.adj);
	    you_have_been(&menu, buf);
	}

	ngenocided = num_genocides();
	if (ngenocided == 0) {
	    you_have_never(&menu, "genocided any monsters");
	} else {
	    sprintf(buf, "genocided %d type%s of monster%s",
		    ngenocided, plur(ngenocided), plur(ngenocided));
	    you_have_X(&menu, buf);
	}

	if (!u.uconduct.polypiles)
	    you_have_never(&menu, "polymorphed an object");
	else if ((wizard || final)) {
	    sprintf(buf, "polymorphed %u item%s",
		    u.uconduct.polypiles, plur(u.uconduct.polypiles));
	    you_have_X(&menu, buf);
	}

	if (!u.uconduct.polyselfs)
	    you_have_never(&menu, "changed form");
	else if ((wizard || final)) {
	    sprintf(buf, "changed form %u time%s",
		    u.uconduct.polyselfs, plur(u.uconduct.polyselfs));
	    you_have_X(&menu, buf);
	}

	if (!u.uconduct.wishes)
	    you_have_X(&menu, "used no wishes");
	else {
	    sprintf(buf, "used %u wish%s",
		    u.uconduct.wishes, (u.uconduct.wishes > 1L) ? "es" : "");
	    you_have_X(&menu, buf);

	    if (u.uconduct.wishmagic) {
		sprintf(buf, "used %u wish%s for magical items",
			u.uconduct.wishmagic, u.uconduct.wishmagic > 1 ? "es" : "");
		you_have_X(&menu, buf);
	    }

	    if (!u.uconduct.wisharti)
		enl_msg(&menu, You_, "have not wished", "did not wish",
			" for any artifacts");
	}

	if (flags.elbereth_enabled) {
	    if (!u.uconduct.elbereths)
		you_have_never(&menu, "wrote Elbereth's name");
	    else {
		sprintf(buf, " Elbereth's name %u time%s",
			u.uconduct.elbereths, plur(u.uconduct.elbereths));
		enl_msg(&menu, You_, "have written", "wrote", buf);
	    }
	} else {
	    you_have_X(&menu, "abstained from Elbereth's help");
	}

	if ((wizard || final) && u.uconduct.bones) {
	    sprintf(buf, "encountered %u bones level%s",
		    u.uconduct.bones, plur(u.uconduct.bones));
	    you_have_X(&menu, buf);
	} else if (wizard || final) {
	    you_have_never(&menu, "encountered a bones level");
	}

	if ((wizard || final) && !u.uconduct.sokoban) {
	    you_have_never(&menu, "used any Sokoban shortcuts");
	} else if (wizard || final) {
	    sprintf(buf, "used %u Sokoban shortcut%s",
		    u.uconduct.sokoban, plur(u.uconduct.sokoban));
	    you_have_X(&menu, buf);
	}

	/* Pop up the window and wait for a key */
	display_menu(menu.items, menu.icount, "Voluntary challenges:", PICK_NONE, NULL);
	free(menu.items);
}


struct nh_cmd_desc *nh_get_commands(int *count)
{
	int i, j, cmdcount = 0;
	struct nh_cmd_desc *ui_cmd;
	
	for (i = 0; cmdlist[i].name; i++)
	    if (wizard || !(cmdlist[i].flags & CMD_DEBUG))
		cmdcount++;
    
	ui_cmd = xmalloc(sizeof(struct nh_cmd_desc) * cmdcount);
	if (!ui_cmd)
	    return NULL;
	memset(ui_cmd, 0, sizeof(struct nh_cmd_desc) * cmdcount);

	j = 0;
	for (i = 0; cmdlist[i].name; i++)
	    if (wizard || !(cmdlist[i].flags & CMD_DEBUG)) {
		strncpy(ui_cmd[j].name, cmdlist[i].name, sizeof(ui_cmd[j].name));
		strncpy(ui_cmd[j].desc, cmdlist[i].desc, sizeof(ui_cmd[j].desc));
		ui_cmd[j].defkey = cmdlist[i].defkey;
		ui_cmd[j].altkey = cmdlist[i].altkey;
		ui_cmd[j].flags = cmdlist[i].flags;
		j++;
	    }
    
	*count = cmdcount;
	return ui_cmd;
}


/* for better readability below */
#define SET_OBJ_CMD(caccel, cname, cinvlet, cdesc) \
do {\
	int _o_c_idx = get_command_idx(cname);\
	obj_cmd[i].defkey = (caccel);\
	obj_cmd[i].altkey = (cinvlet);\
	strncpy(obj_cmd[i].name, (cname), sizeof(obj_cmd[i].name));\
	strncpy(obj_cmd[i].desc, (cdesc), sizeof(obj_cmd[i].desc));\
	obj_cmd[i].flags = cmdlist[_o_c_idx].flags;\
	i++;\
} while (0)

#define SET_OBJ_CMD2(caccel, cname, cdesc) \
	SET_OBJ_CMD((caccel), (cname), obj->invlet, (cdesc))


struct nh_cmd_desc *nh_get_object_commands(int *count, char invlet)
{
	int i, cmdcount = 0;
	struct nh_cmd_desc *obj_cmd;
	struct obj *obj;
	struct monst *shkp;
	
	/* returning a list of commands when .viewing is true doesn't hurt
	 * anything, but since they won't work there is no point. */
	if (program_state.viewing)
	    return NULL;
	
	for (obj = invent; obj; obj = obj->nobj)
	    if (obj->invlet == invlet)
		break;
	
	if (!obj)
	    return NULL;
	
	check_tutorial_oclass(obj->oclass);
	
	for (i = 0; cmdlist[i].name; i++)
	    if (cmdlist[i].flags & CMD_ARG_OBJ)
		cmdcount++;
	
	/* alloc space for all potential commands, even if fewer will apply to the
	* given object. This greatly simplifies the counting above. */
	obj_cmd = xmalloc(sizeof(struct nh_cmd_desc) * cmdcount);
	if (!obj_cmd)
	    return NULL;
	memset(obj_cmd, 0, sizeof(struct nh_cmd_desc) * cmdcount);
	
	i = 0; /* incremented by the SET_OBJ_CMD macro */
	/* apply item; this can mean almost anything depending on the item */
	if (obj->otyp == CREAM_PIE)
	    SET_OBJ_CMD2('a', "apply", "apply (to self)");
	else if (obj->otyp == BULLWHIP)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == GRAPPLING_HOOK)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (Is_container(obj) || obj->otyp == BAG_OF_TRICKS)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == CAN_OF_GREASE)
	    SET_OBJ_CMD2('a', "apply", "apply (to object)");
	else if (obj->otyp == LOCK_PICK || obj->otyp == CREDIT_CARD || obj->otyp == SKELETON_KEY)
	    SET_OBJ_CMD2('a', "apply", "apply (to lock)");
	else if (obj->otyp == TINNING_KIT)
	    SET_OBJ_CMD2('a', "apply", "apply (to corpse)");
	else if (obj->otyp == LEASH)
	    SET_OBJ_CMD2('a', "apply", "apply (to monster)");
	else if (obj->otyp == SADDLE)
	    SET_OBJ_CMD2('a', "apply", "apply (to monster)");
	else if (obj->otyp == MAGIC_WHISTLE || obj->otyp == TIN_WHISTLE)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == EUCALYPTUS_LEAF)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == STETHOSCOPE)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == MIRROR)
	    SET_OBJ_CMD2('a', "apply", "apply (to monster)");
	else if (obj->otyp == BELL || obj->otyp == BELL_OF_OPENING)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == CANDELABRUM_OF_INVOCATION)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if ((obj->otyp == WAX_CANDLE || obj->otyp == TALLOW_CANDLE) &&
			carrying(CANDELABRUM_OF_INVOCATION))
	    SET_OBJ_CMD2('a', "apply", "apply (attach to candelabrum)");
	else if (obj->otyp == WAX_CANDLE || obj->otyp == TALLOW_CANDLE)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
			obj->otyp == BRASS_LANTERN)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == POT_OIL && objects[obj->otyp].oc_name_known)
	    SET_OBJ_CMD2('a', "apply", "apply (ignite)");
	else if (obj->otyp == EXPENSIVE_CAMERA)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == TOWEL)
	    SET_OBJ_CMD2('a', "apply", "apply (to self)");
	else if (obj->otyp == CRYSTAL_BALL)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == MAGIC_MARKER)
	    SET_OBJ_CMD2('a', "apply", "apply (to object)");
	else if (obj->otyp == FIGURINE)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == UNICORN_HORN)
	    SET_OBJ_CMD2('a', "apply", "apply (to self)");
	else if (obj->otyp >= WOODEN_FLUTE && obj->otyp <= DRUM_OF_EARTHQUAKE)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == LAND_MINE || obj->otyp == BEARTRAP)
	    SET_OBJ_CMD2('a', "apply", "apply");
	else if (obj->otyp == PICK_AXE || obj->otyp == DWARVISH_MATTOCK)
	    SET_OBJ_CMD2('a', "apply", "apply (dig)");

	/* adjust; avoid non-letter items like gold */
	if ((obj->invlet >= 'a' && obj->invlet <= 'z') ||
	    (obj->invlet >= 'A' && obj->invlet <= 'Z'))
	    SET_OBJ_CMD2('A', "adjust", "adjust letter");

	/* drop item, works on almost everything
	   (Yes, this reveals loadstones to be undroppable, but if it's
	   gotten to this point, that info is too late to be useful anyway.)*/
	if (canletgo(obj, ""))
	    SET_OBJ_CMD2('d', "drop", "drop");
	
	/* dip */
	if (obj->oclass == POTION_CLASS)
	    SET_OBJ_CMD2('D', "dip", "dip (object into this)");

	/* eat item; eat.c provides is_edible to check */
	if (obj->otyp == TIN && uwep && uwep->otyp == TIN_OPENER)
	    SET_OBJ_CMD2('e', "eat", "eat");
	else if (obj->otyp == TIN)
	    SET_OBJ_CMD2('e', "eat", "eat");
	else if (is_edible(obj))
	    SET_OBJ_CMD2('e', "eat", "eat");
	
	/* engrave with item */
	if (obj->otyp == TOWEL)
	    SET_OBJ_CMD2('E', "engrave", "engrave (wipe floor)");
	else if (obj->otyp == MAGIC_MARKER)
	    SET_OBJ_CMD2('E', "engrave", "engrave");
	else if (obj->oclass == WEAPON_CLASS || obj->oclass == WAND_CLASS ||
			obj->oclass == GEM_CLASS || obj->oclass == RING_CLASS)
	    SET_OBJ_CMD2('E', "engrave", "engrave");

	/* name this item or call all items like this */
	if (obj->oclass != COIN_CLASS)
	    SET_OBJ_CMD2('n', "name", "name");

	/* pay for this item */
	if ((shkp = shop_keeper(level, *in_rooms(level, u.ux, u.uy, SHOPBASE))) &&
	    inhishop(shkp) && obj->unpaid) {
	    char desc[80];	/* == sizeof(obj_cmd[i].desc) */
	    long price = unpaid_cost(obj);
	    boolean unpaid_contents = Has_contents(obj) &&
				      contained_cost(obj, shkp, 0L, FALSE, TRUE) > 0;
	    snprintf(desc, 80, "pay (%ld %s%s)", price, currency(price),
		     unpaid_contents ? ", excluding contents" : "");
	    SET_OBJ_CMD2('p', "pay", desc);
	}

	/* drink item; strangely, this one seems to have no exceptions */
	if (obj->oclass == POTION_CLASS)
	    SET_OBJ_CMD2('q', "drink", "quaff");
	
	/* quiver throwable item
	   (Why are weapons not designed for throwing included, I wonder?) */
	if ((obj->oclass == GEM_CLASS || obj->oclass == WEAPON_CLASS) && !obj->owornmask)
	    SET_OBJ_CMD2('Q', "quiver", "quiver");
	
	/* read item
	  note: Fortune Cookies and T-shirt are intentionally omitted here,
	        as getobj() also goes to some lengths to omit them from the list
	        of items available for reading */
	if (obj->oclass == SCROLL_CLASS)
	    SET_OBJ_CMD2('r', "read", "read");
	else if (obj->oclass == SPBOOK_CLASS)
	    SET_OBJ_CMD2('r', "read", "read");
	
	/* rub */
	if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP || obj->otyp == BRASS_LANTERN)
	    SET_OBJ_CMD2('R', "rub", "rub");
	else if (obj->oclass == GEM_CLASS && is_graystone(obj))
	    SET_OBJ_CMD2('R', "rub", "rub (on object)");
	
	/* throw item, works on almost everything */
	if (!(obj->owornmask & ~(W_WEP | W_QUIVER | W_SWAPWEP))) {
	    /* you automatically throw only 1 item - except for gold */
	    if (obj->oclass == COIN_CLASS)
		SET_OBJ_CMD2('t', "throw", "throw");
	    else if (ammo_and_launcher(obj, uwep))
		SET_OBJ_CMD2('t', "throw", "throw (fire)");
	    else
		SET_OBJ_CMD2('t', "throw", "throw");
	}
	
	/* unequip armor or tip container */
	if (obj->oclass == ARMOR_CLASS && (obj->owornmask & W_ARMOR))
	    SET_OBJ_CMD2('T', "takeoff", "take off");
	else if ((obj->oclass == RING_CLASS || obj->otyp == MEAT_RING) &&
		 (obj->owornmask & W_RING))
	    SET_OBJ_CMD2('T', "remove", "take off");
	else if (obj->oclass == AMULET_CLASS && (obj->owornmask & W_AMUL))
	    SET_OBJ_CMD2('T', "remove", "take off");
	else if (obj->oclass == TOOL_CLASS && (obj->owornmask & W_TOOL))
	    SET_OBJ_CMD2('T', "remove", "take off");
	else if (Is_container(obj))
	    SET_OBJ_CMD2('T', "tip", "tip (empty contents)");
	
	/* invoke */
	if ((obj->otyp == FAKE_AMULET_OF_YENDOR && !obj->known) ||
			obj->oartifact || objects[obj->otyp].oc_unique ||
			obj->otyp == MIRROR) /* deception, according to object_selection_checks */
	    SET_OBJ_CMD2('V', "invoke", "invoke");
	else if (obj->oclass == WAND_CLASS)
	    SET_OBJ_CMD2('V', "invoke", "invoke (break!)");
	
	/* wield: hold in hands */
	if (obj->owornmask & W_WEP) {
	    /* show unwield for wielded weapon */
	    SET_OBJ_CMD('w', "wield", '-', "unwield (empty)");
	} else if (obj->owornmask & ~(W_SWAPWEP|W_ART|W_ARTI|W_BALL)) {
	    /* empty for anything worn except the above */
	} else if (obj->oclass == WEAPON_CLASS || obj->otyp == PICK_AXE ||
		   obj->otyp == UNICORN_HORN) {
	    SET_OBJ_CMD2('w', "wield", "wield");
	} else if (obj->otyp == TIN_OPENER) {
	    SET_OBJ_CMD2('w', "wield", "wield");
	} else if (obj->oclass != ARMOR_CLASS) { /* armor-wield typo begone! */
	    SET_OBJ_CMD2('w', "wield", "wield (hold)");
	}
	
	/* wear: Equip this item */
	if (!(obj->owornmask & ~(W_WEP|W_QUIVER|W_SWAPWEP|W_ART|W_ARTI))) {
	    if (obj->oclass == ARMOR_CLASS)
		SET_OBJ_CMD2('W', "wear", "wear");
	    else if (obj->oclass == RING_CLASS || obj->otyp == MEAT_RING)
		SET_OBJ_CMD2('W', "put on", "wear");
	    else if (obj->oclass == AMULET_CLASS)
		SET_OBJ_CMD2('W', "put on", "wear");
	    else if (obj->otyp == TOWEL || obj->otyp == BLINDFOLD)
		SET_OBJ_CMD2('W', "put on", "wear");
	    else if (obj->otyp == LENSES)
		SET_OBJ_CMD2('W', "put on", "wear");
	}

	/* zap wand */
	if (obj->oclass == WAND_CLASS)
	    SET_OBJ_CMD2('z', "zap", "zap");
	
	/* sacrifice object */
	if (IS_ALTAR(level->locations[u.ux][u.uy].typ) && !u.uswallow) {
	    if (In_endgame(&u.uz) &&
		(obj->otyp == AMULET_OF_YENDOR || obj->otyp == FAKE_AMULET_OF_YENDOR))
		SET_OBJ_CMD2('S', "sacrifice", "sacrifice (on altar)");
	    else if (obj->otyp == CORPSE)
		SET_OBJ_CMD2('S', "sacrifice", "sacrifice (on altar)");
	}

	/* describe object */
	SET_OBJ_CMD2('/', "whatisinv", "describe");

	*count = i;
	return obj_cmd;
}

#undef SET_OBJ_CMD2


static const char template[] = "%-18s %4ld  %6ld";
static const char count_str[] = "                   count  bytes";
static const char separator[] = "------------------ -----  ------";

static void count_obj(struct obj *chain, long *total_count, long *total_size,
		      boolean top, boolean recurse)
{
	long count, size;
	struct obj *obj;

	for (count = size = 0, obj = chain; obj; obj = obj->nobj) {
	    if (top) {
		count++;
		size += sizeof(struct obj) + obj->oxlth + obj->onamelth;
	    }
	    if (recurse && obj->cobj)
		count_obj(obj->cobj, total_count, total_size, TRUE, TRUE);
	}
	*total_count += count;
	*total_size += size;
}

static void obj_chain(struct menulist *menu, const char *src, struct obj *chain,
		      long *total_count, long *total_size)
{
	char buf[BUFSZ];
	long count = 0, size = 0;

	count_obj(chain, &count, &size, TRUE, FALSE);
	*total_count += count;
	*total_size += size;
	sprintf(buf, template, src, count, size);
	add_menutext(menu, buf);
}

static void mon_invent_chain(struct menulist *menu, const char *src, struct monst *chain,
			     long *total_count, long *total_size)
{
	char buf[BUFSZ];
	long count = 0, size = 0;
	struct monst *mon;

	for (mon = chain; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, TRUE, FALSE);
	*total_count += count;
	*total_size += size;
	sprintf(buf, template, src, count, size);
	add_menutext(menu, buf);
}

static void contained(struct menulist *menu, const char *src, long *total_count, long *total_size)
{
	char buf[BUFSZ];
	long count = 0, size = 0;
	struct monst *mon;

	count_obj(invent, &count, &size, FALSE, TRUE);
	count_obj(level->objlist, &count, &size, FALSE, TRUE);
	count_obj(level->buriedobjlist, &count, &size, FALSE, TRUE);
	count_obj(magic_chest_objs, &count, &size, FALSE, TRUE);
	/* DEADMONSTER check not required in this loop since they have no inventory */
	for (mon = level->monlist; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, FALSE, TRUE);
	for (mon = migrating_mons; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, FALSE, TRUE);

	*total_count += count; *total_size += size;

	sprintf(buf, template, src, count, size);
	add_menutext(menu, buf);
}

static void mon_chain(struct menulist *menu, const char *src, struct monst *chain,
		      long *total_count, long *total_size)
{
	char buf[BUFSZ];
	long count, size;
	struct monst *mon;

	for (count = size = 0, mon = chain; mon; mon = mon->nmon) {
	    count++;
	    size += sizeof(struct monst) + mon->mxlth + mon->mnamelth;
	}
	*total_count += count;
	*total_size += size;
	sprintf(buf, template, src, count, size);
	add_menutext(menu, buf);
}

/*
 * Display memory usage of all monsters and objects on the level.
 */
static int wiz_show_stats(void)
{
	char buf[BUFSZ];
	struct menulist menu;
	long total_obj_size = 0, total_obj_count = 0;
	long total_mon_size = 0, total_mon_count = 0;

	init_menulist(&menu);
	add_menutext(&menu, "Current memory statistics:");
	add_menutext(&menu, "");
	sprintf(buf, "Objects, size %d", (int) sizeof(struct obj));
	add_menutext(&menu, buf);
	add_menutext(&menu, "");
	add_menutext(&menu, count_str);

	obj_chain(&menu, "invent", invent, &total_obj_count, &total_obj_size);
	obj_chain(&menu, "level->objlist", level->objlist, &total_obj_count, &total_obj_size);
	obj_chain(&menu, "buried", level->buriedobjlist,
				&total_obj_count, &total_obj_size);
	obj_chain(&menu, "magic chest obj", magic_chest_objs,
				&total_obj_count, &total_obj_size);
	mon_invent_chain(&menu, "minvent", level->monlist,
				&total_obj_count,&total_obj_size);
	mon_invent_chain(&menu, "migrating minvent", migrating_mons,
				&total_obj_count, &total_obj_size);

	contained(&menu, "contained",
				&total_obj_count, &total_obj_size);

	add_menutext(&menu, separator);
	sprintf(buf, template, "Total", total_obj_count, total_obj_size);
	add_menutext(&menu, buf);

	add_menutext(&menu, "");
	add_menutext(&menu, "");
	sprintf(buf, "Monsters, size %d", (int) sizeof(struct monst));
	add_menutext(&menu, buf);
	add_menutext(&menu, "");

	mon_chain(&menu, "level->monlist", level->monlist,
				&total_mon_count, &total_mon_size);
	mon_chain(&menu, "migrating", migrating_mons,
				&total_mon_count, &total_mon_size);

	add_menutext(&menu, separator);
	sprintf(buf, template, "Total", total_mon_count, total_mon_size);
	add_menutext(&menu, buf);

	display_menu(menu.items, menu.icount, NULL, PICK_NONE, NULL);
	free(menu.items);
	return 0;
}


boolean dir_to_delta(enum nh_direction dir, schar *dx, schar *dy, schar *dz)
{
	if (dir == DIR_NONE)
	    return FALSE;
	
	*dx = xdir[dir];
	*dy = ydir[dir];
	*dz = zdir[dir];
	
	return TRUE;
}


int get_command_idx(const char *command)
{
	int i;
	
	if (!command || !command[0])
	    return -1;
	
	for (i = 0; cmdlist[i].name; i++)
	    if (!strcmp(command, cmdlist[i].name) &&
		(wizard || !(cmdlist[i].flags & CMD_DEBUG)))
		return i;
	
	return -1;
}

static int prev_command;
static struct nh_cmd_arg prev_arg = {CMD_ARG_NONE};
static int prev_repcount = 0;

int do_command(int command, int repcount, boolean firsttime, struct nh_cmd_arg *arg)
{
	schar dx, dy, dz;
	int x, y;
	int res, (*func)(void), (*func_dir)(int, int, int), (*func_pos)(int,int);
	int (*func_obj)(struct obj*);
	struct obj *obj, *otmp;
	struct nh_cmd_arg noarg = {CMD_ARG_NONE};
	int argtype, functype;
	boolean allow_zeroobj;
	
	/* for multi-turn movement, we use re-invocation of do_command rather
	 * than set_occupation, so the relevant command must be saved and restored */
	if (command == -1) {
	    command = prev_command;
	    arg = &prev_arg;
	    repcount = prev_repcount;
	}
	
	/* NULL arg is synonymous to CMD_ARG_NONE */
	if (!arg)
	    arg = &noarg;
	
	prev_command = command;
	prev_arg = *arg;
	prev_repcount = repcount;
	
	flags.move = FALSE;
	multi = 0;
	
	if (firsttime) {
	    u.last_str_turn = 0; /* for flags.run > 1 movement */
	    flags.nopick = 0;
	}
	
	if (command == -1)
	    return COMMAND_UNKNOWN;
	
	/* Collect commands to show suitable ambient tutorial messages. */
	if (firsttime)
	    check_tutorial_command(command, arg);
	
	/* in some cases, a command function will accept either it's proper argument
	 * type or no argument; we're looking for the possible type of the argument here */
	functype = (cmdlist[command].flags & CMD_ARG_FLAGS);
	if (!functype)
	    functype = CMD_ARG_NONE;
	allow_zeroobj = (cmdlist[command].flags & CMD_ZEROOBJ) ? TRUE : FALSE;

	argtype = (arg->argtype & cmdlist[command].flags);
	if (!argtype)
	    return COMMAND_BAD_ARG;
	
	if (u.uburied && !cmdlist[command].can_if_buried) {
	    pline("You can't do that while you are buried!");
	    res = 0;
	} else {
	    multi = repcount;
	    
	    switch (functype) {
		case CMD_ARG_NONE:
		    func = cmdlist[command].func;
		    if (cmdlist[command].text && !occupation && multi > 1)
			set_occupation(func, cmdlist[command].text, multi - 1);
		    flags.move = TRUE;
		    res = (*func)();		/* perform the command */
		    break;
		
		case CMD_ARG_DIR:
		    func_dir = cmdlist[command].func;
		    if (!firsttime) {
			/* for multi-move commands such as dogo ('g') and
			 * dogo2 ('G'), the last-used direction is kept in u.dx
			 * and u.dy. lookaround() may change these values! */
			dx = u.dx;
			dy = u.dy;
			dz = 0;
		    }
		    else if (argtype == CMD_ARG_DIR) {
			if (!dir_to_delta(arg->d, &dx, &dy, &dz))
			    return COMMAND_BAD_ARG;
			if (dx && dy && u.umonnum == PM_GRID_BUG)
			    return COMMAND_BAD_ARG;
			
		    } else {
			 /* invalid direction deltas indicate that no arg was given */
			dx = -2; dy = -2; dz = -2;
		    }
		    flags.move = TRUE;
		    if (firsttime) {
			u.dx = dx;
			u.dy = dy;
		    }
		    res = func_dir(dx, dy, dz);
		    break;
		
		case CMD_ARG_POS:
		    func_pos = cmdlist[command].func;
		    if (argtype == CMD_ARG_POS) {
			x = arg->pos.x;
			y = arg->pos.y;
		    } else {
			x = -1;
			y = -1;
		    }
		    flags.move = TRUE;
		    res = func_pos(x, y);
		    break;
		
		case CMD_ARG_OBJ:
		    func_obj = cmdlist[command].func;
		    obj = NULL;
		    if (argtype == CMD_ARG_OBJ) {
			if (allow_zeroobj && arg->invlet == '-')
			    obj = &zeroobj;
			for (otmp = invent; otmp && !obj; otmp = otmp->nobj)
			    if (otmp->invlet == arg->invlet)
				obj = otmp;
		    }
		    flags.move = TRUE;
		    res = func_obj(obj);
		    break;
		    
		default:
		    multi = 0;
		    return COMMAND_BAD_ARG;
	    }
	    
	    if (multi > 0)
		--multi;
	}
	
	if (!res) {
	    flags.move = FALSE;
	    multi = 0;
	}
	
	return COMMAND_OK;
}



int xytod(schar x, schar y)	/* convert an x,y pair into a direction code */
{
	int dd;

	for (dd = 0; dd < 8; dd++)
	    if (x == xdir[dd] && y == ydir[dd]) return dd;

	return -1;
}

void dtoxy(coord *cc, int dd)	/* convert a direction code into an x,y pair */
{
	cc->x = xdir[dd];
	cc->y = ydir[dd];
	return;
}


/*
 * uses getdir() but unlike getdir() it specifically
 * produces coordinates using the direction from getdir()
 * and verifies that those coordinates are ok.
 *
 * If the call to getdir() returns 0, "Never mind." is displayed.
 * If the resulting coordinates are not okay, emsg is displayed.
 *
 * Returns non-zero if coordinates in cc are valid.
 */
int get_adjacent_loc(const char *prompt, const char *emsg, xchar x, xchar y,
		     coord *cc, schar *dz)
{
	xchar new_x, new_y;
	schar dx, dy;
	
	if (!getdir(prompt, &dx, &dy, dz)) {
		pline("Never mind.");
		return 0;
	}
	new_x = x + dx;
	new_y = y + dy;
	if (cc && isok(new_x, new_y)) {
		cc->x = new_x;
		cc->y = new_y;
	} else {
		if (emsg)
		    pline(emsg);
		return 0;
	}
	return 1;
}


void confdir(schar *dx, schar *dy)
{
	int x = (u.umonnum == PM_GRID_BUG) ? 2*rn2(4) : rn2(8);
	*dx = xdir[x];
	*dy = ydir[x];
	return;
}


static int doautoexplore(void)
{
	iflags.autoexplore = TRUE;
	flags.travel = 1;
	iflags.travel1 = 1;
	flags.run = 8;
	flags.nopick = FALSE;
	if (!multi)
	    multi = max(COLNO,ROWNO);
	u.last_str_turn = 0;
	flags.mv = TRUE;

	return domove(0, 0, 0);
}


static int dotravel(int x, int y)
{
	/* Keyboard travel command */
	coord cc;

	if (x == -1 && y == -1) {
	    cc.x = iflags.travelcc.x;
	    cc.y = iflags.travelcc.y;
	    if (cc.x == -1 && cc.y == -1) {
		/* No cached destination, start attempt from current position */
		cc.x = u.ux;
		cc.y = u.uy;
	    }
	    pline("Where do you want to travel to?");
	    if (getpos(&cc, FALSE, "the desired destination") < 0) {
		if (flags.verbose)
		    pline("Never mind.");
		return 0;
	    }
	} else {
	    cc.x = x;
	    cc.y = y;
	}
	iflags.travelcc.x = u.tx = cc.x;
	iflags.travelcc.y = u.ty = cc.y;

	iflags.autoexplore = FALSE;
	flags.travel = 1;
	iflags.travel1 = 1;
	flags.run = 8;
	flags.nopick = TRUE;
	if (!multi)
	    multi = max(COLNO,ROWNO);
	u.last_str_turn = 0;
	flags.mv = TRUE;

	return domove(0, 0, 0);
}


/*
 * Track how many times the player "cheated" in Sokoban
 * before picking up the prize.
 */
void sokoban_trickster(void)
{
	if (!u.uevent.finished_sokoban)
	    u.uconduct.sokoban++;
}

/*cmd.c*/
