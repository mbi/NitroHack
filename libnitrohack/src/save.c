/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "quest.h"

static void savelevchn(struct memfile *mf);
static void savedamage(struct memfile *mf, struct level *lev);
static void freedamage(struct level *lev);
static void save_mongen_override(struct memfile *mf, struct mon_gen_override *);
static void free_mongen_override(struct mon_gen_override *);
static void save_lvl_sounds(struct memfile *mf, struct lvl_sounds *);
static void free_lvl_sounds(struct lvl_sounds *);
static void saveobjchn(struct memfile *mf, struct obj *);
static void free_objchn(struct obj *otmp);
static void savemonchn(struct memfile *mf, struct monst *);
static void free_monchn(struct monst *mon);
static void savetrapchn(struct memfile *mf, struct level *lev);
static void freetrapchn(struct trap *trap);
static void savegamestate(struct memfile *mf);
static void save_flags(struct memfile *mf);
static void freefruitchn(void);


int dosave(void)
{
	if (yn("Really save?") == 'n') {
		if (multi > 0) nomul(0, NULL);
	} else {
		pline("Saving...");
		if (dosave0(FALSE)) {
			program_state.something_worth_saving = 0;
			u.uhp = -1;		/* universal game's over indicator */
			terminate();
		} else doredraw();
	}
	return 0;
}


/* returns 1 if save successful */
int dosave0(boolean emergency)
{
	int fd;
	struct memfile mf;
	boolean log_disabled = iflags.disable_log;

	mnew(&mf, NULL);

	fd = logfile;

	/* when  we leave via nh_exit, logging is disabled. It needs to be
	 * enabled briefly so that log_finish will update the log header. */
	iflags.disable_log = FALSE;
	log_finish(LS_SAVED);
	iflags.disable_log = log_disabled;
	vision_recalc(2);	/* shut down vision to prevent problems
				   in the event of an impossible() call */

	savegame(&mf);
	store_mf(fd, &mf);	/* also frees mf */

	freedynamicdata();

	return TRUE;
}


void savegame(struct memfile *mf)
{
	int count = 0;
	xchar ltmp;

	/* no tag useful here as store_version adds one */
	store_version(mf);

	/* Place flags, player info & moves at the beginning of the save.
	 * This makes it possible to read them in nh_get_savegame_status without
	 * parsing all the dungeon and level data */
	save_flags(mf);
	save_you(mf, &u);
	mwrite32(mf, moves); /* no tag useful here; you is fixed-length */
	save_mon(mf, &youmonst);

	/* store dungeon layout */
	save_dungeon(mf);
	savelevchn(mf);

	/* store levels */
	mtag(mf, 0, MTAG_LEVELS);
	for (ltmp = 1; ltmp <= maxledgerno(); ltmp++)
	    if (levels[ltmp])
		count++;
	mwrite32(mf, count);
	for (ltmp = 1; ltmp <= maxledgerno(); ltmp++) {
		if (!levels[ltmp])
		    continue;
		mtag(mf, ltmp, MTAG_LEVELS);
		mwrite8(mf, ltmp); /* level number*/
		savelev(mf, ltmp); /* actual level*/
	}
	savegamestate(mf);
}


static void save_flags(struct memfile *mf)
{
	/* no mtag useful; fixed distance after version */
	mwrite32(mf, flags.ident);
	mwrite32(mf, flags.moonphase);
	mwrite32(mf, flags.no_of_wizards);
	mwrite32(mf, flags.init_role);
	mwrite32(mf, flags.init_race);
	mwrite32(mf, flags.init_gend);
	mwrite32(mf, flags.init_align);
	mwrite32(mf, flags.randomall);
	mwrite32(mf, flags.pantheon);
	mwrite32(mf, flags.run);
	mwrite32(mf, flags.warntype);
	mwrite32(mf, flags.warnlevel);
	mwrite32(mf, flags.djinni_count);
	mwrite32(mf, flags.ghost_count);
	mwrite32(mf, flags.pickup_burden);
	
	mwrite8(mf, flags.autodig);
	mwrite8(mf, flags.autoquiver);
	mwrite8(mf, flags.autounlock);
	mwrite8(mf, flags.beginner);
	mwrite8(mf, flags.debug);
	mwrite8(mf, flags.explore);
	mwrite8(mf, flags.tutorial);
	mwrite8(mf, flags.female);
	mwrite8(mf, flags.forcefight);
	mwrite8(mf, flags.friday13);
	mwrite8(mf, flags.legacy);
	mwrite8(mf, flags.lit_corridor);
	mwrite8(mf, flags.made_amulet);
	mwrite8(mf, flags.mon_moving);
	mwrite8(mf, flags.move);
	mwrite8(mf, flags.mv);
	mwrite8(mf, flags.nopick);
	mwrite8(mf, flags.pickup);
	mwrite8(mf, flags.pushweapon);
	mwrite8(mf, flags.rest_on_space);
	mwrite8(mf, flags.safe_dog);
	mwrite8(mf, flags.safe_peaceful);
	mwrite8(mf, flags.silent);
	mwrite8(mf, flags.sortpack);
	mwrite8(mf, flags.soundok);
	mwrite8(mf, flags.sparkle);
	mwrite8(mf, flags.tombstone);
	mwrite8(mf, flags.verbose);
	mwrite8(mf, flags.prayconfirm);
	mwrite8(mf, flags.travel);
	mwrite8(mf, flags.end_disclose);
	mwrite8(mf, flags.menu_style);
	mwrite8(mf, flags.elbereth_enabled);
	mwrite8(mf, flags.rogue_enabled);
	mwrite8(mf, flags.seduce_enabled);
	mwrite8(mf, flags.bones_enabled);
	mwrite8(mf, flags.ascet);
	mwrite8(mf, flags.atheist);
	mwrite8(mf, flags.blindfolded);
	mwrite8(mf, flags.illiterate);
	mwrite8(mf, flags.pacifist);
	mwrite8(mf, flags.nudist);
	mwrite8(mf, flags.vegan);
	mwrite8(mf, flags.vegetarian);
	
	mwrite(mf, flags.inv_order, sizeof(flags.inv_order));
	mwrite(mf, flags.spell_order, sizeof(flags.spell_order));
}


static void save_mvitals(struct memfile *mf)
{
	int i;
	/* mtag useful here because migration is variable-length */
	mtag(mf, 0, MTAG_MVITALS);
	for (i = 0; i < NUMMONS; i++) {
	    mwrite8(mf, mvitals[i].born);
	    mwrite8(mf, mvitals[i].died);
	    mwrite8(mf, mvitals[i].mvflags);
	}
}


static void save_quest_status(struct memfile *mf)
{
	unsigned int qflags;
	
	qflags = (quest_status.first_start << 31) |
	         (quest_status.met_leader << 30) |
	         (quest_status.not_ready << 27) |
	         (quest_status.pissed_off << 26) |
	         (quest_status.got_quest << 25) |
	         (quest_status.first_locate << 24) |
	         (quest_status.met_intermed << 23) |
	         (quest_status.got_final << 22) |
	         (quest_status.made_goal << 19) |
	         (quest_status.met_nemesis << 18) |
	         (quest_status.killed_nemesis << 17) |
	         (quest_status.in_battle << 16) |
	         (quest_status.cheater << 15) |
	         (quest_status.touched_artifact << 14) |
	         (quest_status.offered_artifact << 13) |
	         (quest_status.got_thanks << 12) |
	         (quest_status.leader_is_dead << 11);
	mwrite32(mf, qflags);
	mwrite32(mf, quest_status.leader_m_id);
}


static void save_spellbook(struct memfile *mf)
{
	int i;
	for (i = 0; i < MAXSPELL + 1; i++) {
	    mwrite32(mf, spl_book[i].sp_know);
	    mwrite16(mf, spl_book[i].sp_id);
	    mwrite8(mf, spl_book[i].sp_lev);
	}
}


static void savegamestate(struct memfile *mf)
{
	unsigned book_id;

	mtag(mf, 0, MTAG_GAMESTATE);
	mfmagic_set(mf, STATE_MAGIC);

	/* must come before magic_chest_objs and migrating_mons are freed */
	save_timeout(mf);
	save_timers(mf, level, RANGE_GLOBAL);
	save_light_sources(mf, level, RANGE_GLOBAL);

	saveobjchn(mf, invent);
	saveobjchn(mf, magic_chest_objs);
	savemonchn(mf, migrating_mons);
	save_mvitals(mf);

	save_quest_status(mf);
	save_spellbook(mf);
	save_artifacts(mf);
	save_oracles(mf);

	save_tutorial(mf);

	mwrite(mf, pl_character, sizeof pl_character);
	mwrite(mf, pl_fruit, sizeof pl_fruit);
	mwrite32(mf, current_fruit);
	savefruitchn(mf);
	savenames(mf);
	save_waterlevel(mf);
	mwrite32(mf, lastinvnr);
	save_mt_state(mf);
	save_track(mf);
	save_food(mf);
	save_steal(mf);
	save_dig_status(mf);
	save_occupations(mf);
	book_id = book ? book->o_id : 0;
	mwrite32(mf, book_id);
	mwrite32(mf, stetho_last_used_move);
	mwrite32(mf, stetho_last_used_movement);
	mwrite32(mf, multi);
	save_rndmonst_state(mf);
	save_history(mf);
}


static void save_location(struct memfile *mf, struct rm *loc)
{
	unsigned int lflags1;
	unsigned int lflags2;

	lflags1 = 0 |
		  (loc->mem_bg		<< 26) |
		  (loc->mem_trap	<< 21) |
		  (loc->mem_obj		<< 11) |
		  (loc->mem_obj_mn	<<  2) |
		  (loc->mem_obj_stacks	<<  1) |
		  (loc->mem_obj_prize	<<  0);

	lflags2 = 0 |
		  (loc->mem_door_l	<< 18) |
		  (loc->mem_door_t	<< 17) |
		  (loc->mem_stepped	<< 16) |
		  (loc->mem_invis	<< 15) |
		  (loc->flags		<< 10) |
		  (loc->horizontal	<<  9) |
		  (loc->lit		<<  8) |
		  (loc->waslit		<<  7) |
		  (loc->roomno		<<  1) |
		  (loc->edge		<<  0);

	mwrite32(mf, lflags1);
	mwrite8(mf, loc->typ);
	mwrite8(mf, loc->seenv);
	mwrite32(mf, lflags2);
}


void savelev(struct memfile *mf, xchar levnum)
{
	int x, y;
	unsigned int lflags;
	struct level *lev = levels[levnum];

	if (lev->flags.purge_monsters) {
		/* purge any dead monsters (necessary if we're starting
		 * a panic save rather than a normal one, or sometimes
		 * when changing levels without taking time -- e.g.
		 * create statue trap then immediately level teleport) */
		dmonsfree(lev);
	}

	/* mtagging for this already done in save_game */
	mfmagic_set(mf, LEVEL_MAGIC);

	mwrite8(mf, lev->z.dnum);
	mwrite8(mf, lev->z.dlevel);
	mwrite(mf, lev->levname, sizeof(lev->levname));

	for (x = 0; x < COLNO; x++)
	    for (y = 0; y < ROWNO; y++)
		save_location(mf, &lev->locations[x][y]);

	mwrite32(mf, lev->lastmoves);
	mwrite(mf, &lev->upstair, sizeof(stairway));
	mwrite(mf, &lev->dnstair, sizeof(stairway));
	mwrite(mf, &lev->upladder, sizeof(stairway));
	mwrite(mf, &lev->dnladder, sizeof(stairway));
	mwrite(mf, &lev->sstairs, sizeof(stairway));
	mwrite(mf, &lev->updest, sizeof(dest_area));
	mwrite(mf, &lev->dndest, sizeof(dest_area));
	mwrite8(mf, lev->flags.nfountains);
	mwrite8(mf, lev->flags.nsinks);

	lflags = (lev->flags.has_shop		<< 31) |
	         (lev->flags.has_vault		<< 30) |
	         (lev->flags.has_zoo		<< 29) |
	         (lev->flags.has_court		<< 28) |
	         (lev->flags.has_morgue		<< 27) |
	         (lev->flags.has_garden		<< 26) |
	         (lev->flags.has_beehive	<< 25) |
	         (lev->flags.has_barracks	<< 24) |
	         (lev->flags.has_temple		<< 23) |
	         (lev->flags.has_lemurepit	<< 22) |
	         (lev->flags.has_swamp		<< 21) |
	         (lev->flags.noteleport		<< 20) |
	         (lev->flags.hardfloor		<< 19) |
	         (lev->flags.nommap		<< 18) |
	         (lev->flags.hero_memory	<< 17) |
	         (lev->flags.shortsighted	<< 16) |
	         (lev->flags.graveyard		<< 15) |
	         (lev->flags.is_maze_lev	<< 14) |
	         (lev->flags.stormy		<< 13) |
	         (lev->flags.is_cavernous_lev	<< 12) |
	         (lev->flags.arboreal		<< 11) |
	         (lev->flags.sky		<< 10) |
	         (lev->flags.forgotten		<<  9);
	mwrite32(mf, lflags);
	mwrite(mf, lev->doors, sizeof(lev->doors));
	save_rooms(mf, lev);	/* no dynamic memory to reclaim */

	/* must be saved before mons, objs, and buried objs */
	save_timers(mf, lev, RANGE_LEVEL);
	save_light_sources(mf, lev, RANGE_LEVEL);

	savemonchn(mf, lev->monlist);
	save_worm(mf, lev);	/* save worm information */
	savetrapchn(mf, lev);
	saveobjchn(mf, lev->objlist);
	saveobjchn(mf, lev->buriedobjlist);
	saveobjchn(mf, lev->billobjs);
	save_mongen_override(mf, lev->mon_gen);
	save_lvl_sounds(mf, lev->sounds);
	save_engravings(mf, lev);
	savedamage(mf, lev);
	save_regions(mf, lev);
}


void freelev(xchar levnum)
{
	struct level *lev = levels[levnum];
	
	/* must be freed before mons, objs, and buried objs */
	free_timers(lev);
	free_light_sources(lev);

	free_monchn(lev->monlist);
	free_worm(lev);
	freetrapchn(lev->lev_traps);
	free_objchn(lev->objlist);
	free_objchn(lev->buriedobjlist);
	free_objchn(lev->billobjs);
	free_mongen_override(lev->mon_gen);
	free_lvl_sounds(lev->sounds);

	lev->monlist = NULL;
	lev->lev_traps = NULL;
	lev->objlist = NULL;
	lev->buriedobjlist = NULL;
	lev->billobjs = NULL;
	lev->mon_gen = NULL;
	lev->sounds = NULL;

	free_engravings(lev);
	freedamage(lev);
	free_regions(lev);
	
	free(lev);
	levels[levnum] = NULL;
}


static void savelevchn(struct memfile *mf)
{
	s_level	*tmplev;
	int cnt = 0;

	for (tmplev = sp_levchn; tmplev; tmplev = tmplev->next)
	    cnt++;
	mwrite32(mf, cnt);

	for (tmplev = sp_levchn; tmplev; tmplev = tmplev->next) {
	    save_d_flags(mf, tmplev->flags);
	    mwrite(mf, &tmplev->dlevel, sizeof(tmplev->dlevel));
	    mwrite(mf, tmplev->proto, sizeof(tmplev->proto));
	    mwrite8(mf, tmplev->boneid);
	    mwrite8(mf, tmplev->rndlevs);
	}
}


static void savedamage(struct memfile *mf, struct level *lev)
{
	struct damage *damageptr;
	unsigned int xl = 0;

	mtag(mf, ledger_no(&lev->z), MTAG_DAMAGE);

	for (damageptr = lev->damagelist; damageptr; damageptr = damageptr->next)
	    xl++;
	mwrite32(mf, xl);

	for (damageptr = lev->damagelist; damageptr; damageptr = damageptr->next) {
	    mtag(mf, damageptr->when, MTAG_DAMAGEVALUE);
	    mwrite32(mf, damageptr->when);
	    mwrite32(mf, damageptr->cost);
	    mwrite8(mf, damageptr->place.x);
	    mwrite8(mf, damageptr->place.y);
	    mwrite8(mf, damageptr->typ);
	}
}


static void freedamage(struct level *lev)
{
	struct damage *damageptr, *tmp_dam;

	damageptr = lev->damagelist;
	while (damageptr) {
	    tmp_dam = damageptr;
	    damageptr = damageptr->next;
	    free(tmp_dam);
	}
	lev->damagelist = NULL;
}


static void save_mongen_override(struct memfile *mf, struct mon_gen_override *or)
{
	struct mon_gen_tuple *mt;

	mfmagic_set(mf, MONGEN_MAGIC);

	if (!or) {
	    /* mon gen marker == 0: override doesn't exist */
	    mwrite8(mf, 0);
	} else {
	    /* mon gen marker == 1: override exists */
	    mwrite8(mf, 1);
	    mwrite32(mf, or->override_chance);
	    mwrite32(mf, or->total_mon_freq);

	    for (mt = or->gen_chances; mt; mt = mt->next) {
		/* Write tuple marker. */
		mwrite8(mf, 1);

		mfmagic_set(mf, MONGENTUPLE_MAGIC);

		mwrite32(mf, mt->freq);
		mwrite8(mf, mt->is_sym ? 1 : 0);
		mwrite32(mf, mt->monid);
	    }

	    /* End-of-tuples marker. */
	    mwrite8(mf, 0);
	}
}


static void free_mongen_override(struct mon_gen_override *or)
{
	struct mon_gen_tuple *mt;
	struct mon_gen_tuple *prev;

	if (!or)
	    return;

	mt = or->gen_chances;
	while (mt) {
	    prev = mt;
	    mt = mt->next;
	    free(prev);
	}

	free(or);
}


static void save_lvl_sounds(struct memfile *mf, struct lvl_sounds *ls)
{
	mfmagic_set(mf, LVLSOUNDS_MAGIC);

	if (!ls) {
	    /* lvl sounds marker == 0: no sounds defined */
	    mwrite8(mf, 0);
	} else {
	    int i;

	    /* lvl sounds marker == 1: sounds defined */
	    mwrite8(mf, 1);
	    mwrite32(mf, ls->freq);
	    mwrite32(mf, ls->n_sounds);

	    for (i = 0; i < ls->n_sounds; i++) {
		unsigned int len;

		mfmagic_set(mf, LVLSOUNDBITE_MAGIC);

		mwrite32(mf, ls->sounds[i].flags);
		len = strlen(ls->sounds[i].msg) + 1;
		mwrite32(mf, len);
		mwrite(mf, ls->sounds[i].msg, len);
	    }
	}
}


static void free_lvl_sounds(struct lvl_sounds *ls)
{
	int i;

	if (!ls)
	    return;

	for (i = 0; i < ls->n_sounds; i++) {
	    free(ls->sounds[i].msg);
	    ls->sounds[i].msg = NULL;
	}
	free(ls->sounds);
	ls->sounds = NULL;

	free(ls);
}


static void free_objchn(struct obj *otmp)
{
	struct obj *otmp2;
	while (otmp) {
	    otmp2 = otmp->nobj;
	    if (Has_contents(otmp))
		free_objchn(otmp->cobj);
	    
	    otmp->where = OBJ_FREE;	/* set to free so dealloc will work */
	    otmp->timed = 0;	/* not timed any more */
	    otmp->lamplit = 0;	/* caller handled lights */
	    dealloc_obj(otmp);
	    otmp = otmp2;
	}
}


static void saveobjchn(struct memfile *mf, struct obj *otmp)
{
	int count = 0;
	struct obj *otmp2;
	
	mfmagic_set(mf, OBJCHAIN_MAGIC);
	for (otmp2 = otmp; otmp2; otmp2 = otmp2->nobj)
	    count++;
	mwrite32(mf, count);
	
	while (otmp) {
	    save_obj(mf, otmp);
	    if (Has_contents(otmp))
		saveobjchn(mf, otmp->cobj);
	    
	    otmp = otmp->nobj;
	}
}


static void free_monchn(struct monst *mon)
{
	struct monst *mtmp2;

	while (mon) {
	    mtmp2 = mon->nmon;
	    
	    if (mon->minvent)
		free_objchn(mon->minvent);
	    dealloc_monst(mon);
	    mon = mtmp2;
	}
}


static void savemonchn(struct memfile *mf, struct monst *mtmp)
{
	struct monst *mtmp2;
	unsigned int count = 0;
	
	mfmagic_set(mf, MONCHAIN_MAGIC);
	for (mtmp2 = mtmp; mtmp2; mtmp2 = mtmp2->nmon)
	    count++;
	mwrite32(mf, count);

	while (mtmp) {
	    mtmp2 = mtmp->nmon;
	    save_mon(mf, mtmp);
	    
	    if (mtmp->minvent)
		saveobjchn(mf, mtmp->minvent);
	    mtmp = mtmp2;
	}
}


static void savetrapchn(struct memfile *mf, struct level *lev)
{
	struct trap *trap, *trap2;
	unsigned short tflags;
	int count = 0;

	mfmagic_set(mf, TRAPCHAIN_MAGIC);

	trap = lev->lev_traps;

	for (trap2 = trap; trap2; trap2 = trap2->ntrap)
	    count++;
	mwrite32(mf, count);

	for (; trap; trap = trap->ntrap) {
	    /* To distinguish traps from each other in tags, we use x/y/z coords. */
	    mtag(mf, ledger_no(&lev->z) + ((int)trap->tx << 8) +
		 ((int)trap->ty << 16), MTAG_TRAP);
	    mwrite8(mf, trap->tx);
	    mwrite8(mf, trap->ty);
	    mwrite8(mf, trap->dst.dnum);
	    mwrite8(mf, trap->dst.dlevel);
	    mwrite8(mf, trap->launch.x);
	    mwrite8(mf, trap->launch.y);
	    tflags = (trap->ttyp << 11) | (trap->tseen << 10) |
			(trap->once << 9) | (trap->madeby_u << 8);
	    mwrite16(mf, tflags);
	    mwrite16(mf, trap->vl.v_launch_otyp);
	}
}


static void freetrapchn(struct trap *trap)
{
	struct trap *trap2;
	
	while (trap) {
	    trap2 = trap->ntrap;
	    dealloc_trap(trap);
	    trap = trap2;
	}
}

/* save all the fruit names and ID's; this is used only in saving whole games
 * (not levels) and in saving bones levels.  When saving a bones level,
 * we only want to save the fruits which exist on the bones level; the bones
 * level routine marks nonexistent fruits by making the fid negative.
 */
void savefruitchn(struct memfile *mf)
{
	struct fruit *f1;
	unsigned int count = 0;
	
	mfmagic_set(mf, FRUITCHAIN_MAGIC);
	for (f1 = ffruit; f1; f1 = f1->nextf)
	    if (f1->fid >= 0)
		count++;
	mwrite32(mf, count);

	for (f1 = ffruit; f1; f1 = f1->nextf) {
	    if (f1->fid >= 0) {
		mtag(mf, f1->fid, MTAG_FRUIT);
		mwrite(mf, f1->fname, sizeof(f1->fname));
		mwrite32(mf, f1->fid);
	    }
	}
}


static void freefruitchn(void)
{
	struct fruit *f2, *f1 = ffruit;
	while (f1) {
	    f2 = f1->nextf;
	    dealloc_fruit(f1);
	    f1 = f2;
	}
	ffruit = NULL;
}


void freedynamicdata(void)
{
	int i;
	struct level *lev;
	
	if (!objects)
	    return; /* no cleanup necessary */
	
	unload_qtlist();
	free_invbuf();	/* let_to_name (invent.c) */
	free_youbuf();	/* You_buf,&c (pline.c) */
	tmp_at(DISP_FREEMEM, 0);	/* temporary display effects */
# define free_animals()	 mon_animal_list(FALSE)

	for (i = 0; i < MAXLINFO; i++) {
	    lev = levels[i];
	    levels[i] = NULL;
	    if (!lev) continue;
	    
	    /* level-specific data */
	    dmonsfree(lev);	/* release dead monsters */
	    free_timers(lev);
	    free_light_sources(lev);
	    free_monchn(lev->monlist);
	    free_worm(lev);		/* release worm segment information */
	    freetrapchn(lev->lev_traps);
	    free_objchn(lev->objlist);
	    free_objchn(lev->buriedobjlist);
	    free_objchn(lev->billobjs);
	    free_engravings(lev);
	    freedamage(lev);
	    
	    free(lev);
	}

	/* game-state data */
	free_objchn(invent);
	free_objchn(magic_chest_objs);
	free_monchn(migrating_mons);
	free_monchn(mydogs);		/* ascension or dungeon escape */
	free_animals();
	free_oracles();
	freefruitchn();
	freenames();
	free_waterlevel();
	free_dungeon();
	free_history();

	if (iflags.ap_rules) {
	    free(iflags.ap_rules->rules);
	    iflags.ap_rules->rules = NULL;
	    free(iflags.ap_rules);
	}
	iflags.ap_rules = NULL;
	if (iflags.mt_rules) {
	    free(iflags.mt_rules->rules);
	    iflags.mt_rules->rules = NULL;
	    free(iflags.mt_rules);
	    iflags.mt_rules = NULL;
	}
	if (iflags.hp_notify_fmt) {
	    free(iflags.hp_notify_fmt);
	    iflags.hp_notify_fmt = NULL;
	}

	free(artilist);
	free(objects);
	objects = NULL;
	artilist = NULL;
	
	if (active_birth_options)
	    free_optlist(active_birth_options);
	active_birth_options = NULL;
	
	return;
}

/*save.c*/
