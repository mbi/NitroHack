/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

/* If you change the flag structure make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.  Changing the instance_flags structure does	   */
/* not require incrementing EDITLEVEL.					   */

#ifndef FLAG_H
#define FLAG_H

/*
 * Persistent flags that are saved and restored with the game.
 *
 */

struct flag {
	boolean  autodig;       /* MRKR: Automatically dig */
	boolean  autoquiver;	/* Automatically fill quiver */
	boolean  autounlock;	/* Automatically ask to apply unlocking tool */
	boolean  beginner;
	boolean  debug;		/* in debugging mode */
#define wizard	 flags.debug
	boolean  explore;	/* in exploration mode */
#define discover flags.explore
	boolean  tutorial;	/* in tutorial mode */
	boolean  female;
	boolean  forcefight;
	boolean  friday13;	/* it's Friday the 13th */
	boolean  legacy;	/* print game entry "story" */
	boolean  lit_corridor;	/* show a dark corr as lit if it is in sight */
	boolean  made_amulet;
	boolean  mon_moving;	/* monsters' turn to move */
	boolean  move;		/* normally 1, unless an action during your turn
				 * did NOT use up the move */
	boolean  mv;
	boolean  bypasses;	/* bypass flag is set on at least one fobj */
	boolean  nopick;	/* do not pickup objects (as when running) */
	boolean  pickup;	/* whether you pickup or move and look */

	boolean  pushweapon;	/* When wielding, push old weapon into second slot */
	boolean  rest_on_space; /* space means rest */
	boolean  safe_dog;	/* give complete protection to the dog */
	char	 safe_peaceful;	/* prevent accidentally hitting peaceful monsters
				 * 'y': yes, 'n': no, 'a': ask */
	boolean  silent;	/* whether the bell rings or not */
	boolean  sortpack;	/* sorted inventory */
	boolean  soundok;	/* ok to tell about sounds heard */
	char	 sparkle;	/* show "resisting" special FX (Scott Bigham) */
	boolean  tombstone;	/* print tombstone */
	boolean  verbose;	/* max battle info */
	boolean  prayconfirm;	/* confirm before praying */
	unsigned ident;		/* social security number for each monster */
	unsigned moonphase;
#define NEW_MOON	0
#define FULL_MOON	4
	unsigned no_of_wizards; /* 0, 1 or 2 (wizard and his shadow) */
	boolean  travel;	/* find way automatically to u.tx,u.ty */
	unsigned run;		/* 0: h (etc), 1: H (etc), 2: fh (etc) */
				/* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
				/* 8: travel */
	unsigned int warntype; /* warn_of_mon monster type M2 */
	int	 djinni_count, ghost_count;	/* potion effect tuning */
	int	 pickup_burden;		/* maximum burden before prompt */
	char	 inv_order[MAXOCLASSES];
	char	 spell_order[52];	/* spell menu letters (a-zA-Z) */
#define DISCLOSE_PROMPT_DEFAULT_YES	'y'
#define DISCLOSE_PROMPT_DEFAULT_NO	'n'
#define DISCLOSE_YES_WITHOUT_PROMPT	'+'
#define DISCLOSE_NO_WITHOUT_PROMPT	'-'
	char	 end_disclose;  /* disclose various info upon exit */
	char	 menu_style;	/* User interface style setting */

	/* KMH, role patch -- Variables used during startup.
	 *
	 * If the user wishes to select a role, race, gender, and/or alignment
	 * during startup, the choices should be recorded here.  This
	 * might be specified through command-line options, environmental
	 * variables, a popup dialog box, menus, etc.
	 *
	 * These values are each an index into an array.  They are not
	 * characters or letters, because that limits us to 26 roles.
	 * They are not booleans, because someday someone may need a neuter
	 * gender.  Negative values are used to indicate that the user
	 * hasn't yet specified that particular value.	If you determine
	 * that the user wants a random choice, then you should set an
	 * appropriate random value; if you just left the negative value,
	 * the user would be asked again!
	 *
	 * These variables are stored here because the u structure is
	 * cleared during character initialization, and because the
	 * flags structure is restored for saved games.  Thus, we can
	 * use the same parameters to build the role entry for both
	 * new and restored games.
	 *
	 * These variables should not be referred to after the character
	 * is initialized or restored (specifically, after role_init()
	 * is called).
	 */

	/* Default starting role, race, gender and alignment, as per the options.
	 * The actual values in use for an ongoing game are in struct you. */
	int	init_role; /* (index into roles[])   */
	int	init_race; /* (index into races[])   */
	int	init_gend; /* (index into genders[]) */
	int	init_align;/* (index into aligns[])  */

	int randomall;	/* randomly assign everything not specified */
	int pantheon;	/* deity selection for priest character */

	/* birth option flags */
	boolean  elbereth_enabled; /* should the E-word repel monsters? */
	boolean  rogue_enabled;    /* create a rogue level */
	boolean  seduce_enabled;   /* succubus sduction */
	boolean  bones_enabled;    /* allow loading bones levels */

	/* --- initial roleplay flags ---
	 * These flags represent the player's conduct/roleplay
	 * intention at character creation.
	 *
	 * First the player can set some of these at character
	 * creation (via configuration).
	 * Then role_init() may set/prevent certain combinations,
	 * e.g. Monks get the vegetarian flag, vegans should also be
	 * vegetarians.
	 *
	 * After that the initial flags shouldn't be modified.
	 * In u_init() the flags can be used to put some
	 * roleplay-intrinsics into the u structure. Only those
	 * should be modified during gameplay.
	 */
	boolean  ascet;
	boolean  atheist;
	boolean  blindfolded;
	boolean  illiterate;
	boolean  pacifist;
	boolean  nudist;
	boolean  vegan;
	boolean  vegetarian;
};


/*
 * Flags that are set each time the game is started.
 * These are not saved with the game.
 *
 */
struct instance_flags {
	boolean  vision_inited; /* true if vision is ready */
	boolean  pickup_dropped;	/* auto-pickup items you dropped */
	boolean  pickup_thrown; /* auto-pickup items you threw */
	boolean  travel1;	/* first travel step */
	coord    travelcc;	/* coordinates for travel_cache */
	boolean  mon_polycontrol;	/* debug: control monster polymorphs */
	boolean  next_msg_nonblocking;	/* suppress a --More-- after this message */
	boolean  autodigdown;	/* autodigging down tries to create a pit/hole */
	boolean  menu_match_tight;	/* use tighter matching for multi-drop/loot */

	boolean paranoid_hit;	/* Ask for 'yes' when hitting peacefuls */
	boolean paranoid_quit;	/* Ask for 'yes' when quitting */
	boolean paranoid_trap;	/* Ask before walking into known traps */
	boolean paranoid_chat;	/* Always ask for direction when chatting */
	boolean paranoid_loot;	/* Always ask to loot even with only one container */
	boolean paranoid_lava;	/* Require m-direction to move into lava. */
	boolean paranoid_water;	/* Require m-direciton to move into water. */

	boolean hp_notify;	/* Show a message when HP changes. */
	char	*hp_notify_fmt;	/* hp_notify message format. */
	boolean delay_msg;	/* Show message for multi-turn actions. */

	/* Items which belong in flags, but are here to allow save compatibility */
	boolean  show_uncursed;	/* always show uncursed items as such */
	boolean  showrace;	/* show hero glyph by race rather than by role */
	int	 runmode;	/* update screen display during run moves */
	int	 pilesize;	/* max number of floor items to list automatically */
	boolean  disable_log;   /* don't append anything to the logfile */
	boolean  botl;		/* redo status line */
	boolean  autoexplore;	/* currently autoexploring */
	struct nh_autopickup_rules *ap_rules;
	struct nh_msgtype_rules *mt_rules;
};

extern struct flag flags;
extern struct instance_flags iflags;

/* runmode options */
#define RUN_TPORT	0	/* don't update display until movement stops */
#define RUN_LEAP	1	/* update display every 7 steps */
#define RUN_STEP	2	/* update display every single step */
#define RUN_CRAWL	3	/* walk w/ extra delay after each update */

#endif /* FLAG_H */
