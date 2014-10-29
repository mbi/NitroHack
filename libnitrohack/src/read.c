/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* KMH -- Copied from pray.c; this really belongs in a header file */
#define DEVOUT 14
#define STRIDENT 4

#define Your_Own_Role(mndx) \
	((mndx) == urole.malenum || \
	 (urole.femalenum != NON_PM && (mndx) == urole.femalenum))
#define Your_Own_Race(mndx) \
	((mndx) == urace.malenum || \
	 (urace.femalenum != NON_PM && (mndx) == urace.femalenum))

static const char readable[] =
		   { COIN_CLASS, ALL_CLASSES, SCROLL_CLASS, SPBOOK_CLASS, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };

static void wand_explode(struct obj *);
static void stripspe(struct obj *);
static void p_glow1(struct obj *);
static void p_glow2(struct obj *,const char *);
static void randomize(int *, int);
static void forget_single_object(int);
static void maybe_tame(struct monst *,struct obj *);

static void do_flood(int,int,void *);
static void undo_flood(int,int,void *);
static void set_lit(int,int,void *);

int doread(struct obj *scroll)
{
	boolean confused, known;

	known = FALSE;
	if (check_capacity(NULL)) return 0;
	
	if (scroll && !validate_object(scroll, readable, "read"))
	    return 0;
	else if (!scroll)
	    scroll = getobj(readable, "read", NULL);
	if (!scroll) return 0;

	/* outrumor has its own blindness check */
	if (scroll->otyp == FORTUNE_COOKIE) {
	    if (flags.verbose)
		pline("You break up the cookie and throw away the pieces.");
	    if (u.roleplay.illiterate) {
		pline("The cookie had a scrap of paper inside.");
		pline("What a pity that you cannot read!");
	    } else {
		outrumor(bcsign(scroll), BY_COOKIE);
		if (!Blind) violated(CONDUCT_ILLITERACY);
	    }
	    useup(scroll);
	    return 1;
	} else if (scroll->otyp == T_SHIRT) {
	    static const char *const shirt_msgs[] = { /* Scott Bigham */
    "I explored the Dungeons of Doom and all I got was this lousy T-shirt!",
    "Is that Mjollnir in your pocket or are you just happy to see me?",
    "It's not the size of your sword, it's how #enhance'd you are with it.",
    "Madame Elvira's House O' Succubi Lifetime Customer",
    "Madame Elvira's House O' Succubi Employee of the Month",
    "Ludios Vault Guards Do It In Small, Dark Rooms",
    "Yendor Military Soldiers Do It In Large Groups",
    "I survived Yendor Military Boot Camp",
    "Ludios Accounting School Intra-Mural Lacrosse Team",
    "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
    "Hey, black dragon!  Disintegrate THIS!",
    "I'm With Stupid -->",
    "Don't blame me, I voted for Izchak!",
    "Frodo Lives!",
    "Actually, I AM a quantum mechanic.",
    "I beat the Sword Master",			/* Monkey Island */
    "Don't Panic",				/* HHGTTG */
    "Furinkan High School Athletic Dept.",	/* Ranma 1/2 */
    "Hel-LOOO, Nurse!",			/* Animaniacs */
    /* NAO */
    "=^.^=",
    "100% goblin hair - do not wash",
    "Aim >>> <<< here",
    "cK -- Cockatrice touches the Kop",
    "Don't ask me, I only adventure here",
    "Down With Pants!",
    "Gehennoms Angels",
    "Glutton For Punishment",
    "Go Team Ant!",
    "Got Newt?",
    "Heading for Godhead",
    "Hello, my darlings!", /* Charlie Drake */
    "Hey! Nymphs! Steal This T-Shirt!",
    "I <3 Dungeon of Doom",
    "I am a Valkyrie. If you see me running, try to keep up.",
    "I Am Not a Pack Rat - I Am a Collector",
    "I bounced off a rubber tree",
    "If you can read this, I can hit you with my polearm",
    "I Support Single Succubi",
    "I want to live forever or die in the attempt.",
    "Kop Killaz",
    "Lichen Park",
    "LOST IN THOUGHT - please send search party",
    "Minetown Watch",
    "Ms. Palm's House of Negotiable Affection -- A Very Reputable House Of Disrepute",
    "^^  My eyes are up there!  ^^",
    "Next time you wave at me, use more than one finger, please.",
    "Objects In This Shirt Are Closer Than They Appear",
    "Protection Racketeer",
    "P Happens",
    "Real men love Crom",
    "Sokoban Gym -- Get Strong or Die Trying",
    "Somebody stole my Mojo!",
    "The Hellhound Gang",
    "The Werewolves",
    "They Might Be Storm Giants",
    "Weapons don't kill people, I kill people",
    "White Zombie",
    "Worship me",
    "You laugh because I'm different, I laugh because you're about to die",
    "You should hear what the voices in my head are saying about you.",
    "Anhur State University - Home of the Fighting Fire Ants!",
    "FREE HUGS",
    "Serial Ascender",
    "I couldn't afford this T-shirt so I stole it!",
    "End Mercantile Opacity Discrimination Now: Let Invisible Customers Shop!",
    "Elvira's House O'Succubi, granting the gift of immorality!",
    /* UnNetHack */
    "I made a NetHack fork and all I got was this lousy T-shirt!",	/* galehar */
	    };
	    char buf[BUFSZ];
	    int erosion;

	    if (Blind) {
		pline("You can't feel any Braille writing.");
		return 0;
	    }
	    if (u.roleplay.illiterate) {
		pline("Unfortunately, you cannot read.");
		return 0;
	    } else {
		violated(CONDUCT_ILLITERACY);
		if (flags.verbose)
		    pline("It reads:");
		strcpy(buf, shirt_msgs[scroll->o_id % SIZE(shirt_msgs)]);
	    }
	    erosion = greatest_erosion(scroll);
	    if (erosion)
		wipeout_text(buf,
			(int)(strlen(buf) * erosion / (2*MAX_ERODE)),
			     scroll->o_id ^ (unsigned)u.ubirthday);
	    pline("\"%s\"", buf);
	    return 1;
	} else if (scroll->otyp == CREDIT_CARD) {
	    static const char *card_msgs[] = {
		"Leprechaun Gold Tru$t - Shamrock Card",
		"Magic Memory Vault Charge Card",
		"Larn National Bank", /* Larn */
		"First Bank of Omega", /* Omega */
		"Bank of Zork - Frobozz Magic Card", /* Zork */
		"Ankh-Morpork Merchant's Guild Barter Card",
		"Ankh-Morpork Thieves' Guild Unlimited Transaction Card",
		"Ransmannsby Moneylenders Association",
		"Bank of Gehennom - 99% Interest Card",
		"Yendorian Express - Copper Card",
		"Yendorian Express - Silver Card",
		"Yendorian Express - Gold Card",
		"Yendorian Express - Mithril Card",
		"Yendorian Express - Platinum Card",
	    };
	    if (u.roleplay.illiterate) {
		if (Blind)
		    pline("You feel embossed writing.");
		else if (flags.verbose)
		    pline("There is writing on the %s.", xname(scroll));
		pline("Unfortunately, you cannot read.");
		return 0;
	    }
	    if (Blind) {
		pline("You feel the embossed numbers:");
	    } else {
		if (flags.verbose)
		    pline("It reads:");
		pline("\"%s\"", scroll->oartifact ?
		      card_msgs[SIZE(card_msgs) - 1] :
		      card_msgs[scroll->o_id % (SIZE(card_msgs) - 1)]);
	    }
	    /* Make a credit card number */
	    pline("\"%d0%d %d%d1 0%d%d0\"",
		  (scroll->o_id % 89) + 10, scroll->o_id % 4,
		  ((scroll->o_id * 499) % 899999) + 100000, scroll->o_id % 10,
		  !(scroll->o_id % 3), (scroll->o_id * 7) % 10);
	    violated(CONDUCT_ILLITERACY);
	    return 1;
	} else if (scroll->otyp == TIN ||
		   scroll->otyp == CAN_OF_GREASE ||
		   scroll->otyp == CANDY_BAR) {
	    pline("This %s has no %s.", singular(scroll, xname),
		  scroll->otyp == CANDY_BAR ? "wrapper" : "label");
	    return 0;
	} else if (scroll->otyp == MAGIC_MARKER) {
	    if (Blind) {
		pline("You can't feel any Braille writing.");
		return 0;
	    }
	    if (u.roleplay.illiterate) {
		if (flags.verbose)
		    pline("There is writing on the %s.", xname(scroll));
		pline("Unfortunately, you cannot read.");
		return 0;
	    }
	    if (flags.verbose)
		pline("It reads:");
	    pline("\"Magic Marker(TM) Red Ink Marker Pen. Water Soluble.\"");
	    violated(CONDUCT_ILLITERACY);
	    return 1;
	} else if (scroll->oclass == COIN_CLASS) {
	    char tmp[BUFSZ];
	    if (u.roleplay.illiterate) {
		if (Blind)
		    pline("You feel the embossed words.");
		else if (flags.verbose)
		    pline("There is writing on the %s.", singular(scroll, xname));
		pline("Unfortunately, you cannot read.");
		return 0;
	    }
	    if (Blind)
		pline("You feel the embossed words:");
	    else if (flags.verbose)
		pline("You read:");
	    strcpy(tmp, currency(1));
	    pline("\"1 %s. 857 GUE. In Frobs We Trust.\"", upstart(tmp));
	    violated(CONDUCT_ILLITERACY);
	    return 1;
	} else if (scroll->oartifact == ART_ORB_OF_FATE) {
	    if (u.roleplay.illiterate) {
		if (Blind)
		    pline("You feel an engraved signature.");
		else if (flags.verbose)
		    pline("The %s is signed.", xname(scroll));
		pline("Unfortunately, you cannot read.");
		return 0;
	    }
	    if (Blind)
		pline("You feel the engraved signature:");
	    else
		pline("It is signed:");
	    pline("\"Odin.\"");
	    violated(CONDUCT_ILLITERACY);
	    return 1;
	} else if (OBJ_DESCR(objects[scroll->otyp]) &&
		   !strncmp(OBJ_DESCR(objects[scroll->otyp]), "runed", 5)) {
	    if (scroll->otyp == RUNESWORD) {
		pline("You can't decipher the arcane runes.");
		return 0;
	    } else if (!Race_if(PM_ELF) && !Role_if(PM_ARCHEOLOGIST)) {
		pline("You can't decipher the Elvish runes.");
		return 0;
	    }
	    if (u.roleplay.illiterate) {
		if (Blind)
		    pline("You feel engraved runes.");
		else if (flags.verbose)
		    pline("There are runes on the %s.", xname(scroll));
		pline("Unfortunately, you cannot read.");
		return 0;
	    }
	    violated(CONDUCT_ILLITERACY);
	    if (objects[scroll->otyp].oc_merge) {
		if (Blind)
		    pline("You feel the engraved runes:");
		else if (flags.verbose)
		    pline("The runes read:");
		pline("\"Made in Elfheim.\"");
		return 1;
	    } else {
		/* "Avoid any artifact with Runes on it, even if the Runes
		 *  prove only to spell the maker's name." -- Diana Wynne Jones
		 */
		/* Elf name fragments courtesy of ToME */
		static const char *elf_syllable1[] = {
		    "Al", "An",
		    "Bal", "Bel",
		    "Cal", "Cel",
		    "El", "Elr", "Elv", "Eow", "Ear",
		    "F", "Fal", "Fel", "Fin",
		    "G", "Gal", "Gel", "Gl",
		    "Is", "Lan", "Leg", "Lom",
		    "N", "Nal","Nel",
		    "S", "Sal", "Sel",
		    "T", "Tal", "Tel", "Thr", "Tin",
		};
		static const char *elf_syllable2[] = {
		    "a", "adrie", "ara",
		    "e", "ebri", "ele", "ere",
		    "i", "io", "ithra", "ilma", "il-Ga", "ili",
		    "o", "orfi",
		    "u",
		    "y",
		};
		static const char *elf_syllable3[] = {
		    "l", "las", "lad", "ldor", "ldur", "linde", "lith",
		    "mir", "mbor",
		    "n", "nd", "ndel", "ndil", "ndir", "nduil", "ng",
		    "r", "rith", "ril", "riand", "rion",
		    "s",
		    "thien",
		    "viel",
		    "wen", "wyn",
		};
		if (Blind)
		    pline("You feel the engraved signature:");
		else
		    pline("It is signed:");
		pline("\"%s%s%s\"",
		      elf_syllable1[scroll->o_id % SIZE(elf_syllable1)],
		      elf_syllable2[scroll->o_id % SIZE(elf_syllable2)],
		      elf_syllable3[scroll->o_id % SIZE(elf_syllable3)]);
		return 1;
	    }
	} else if (scroll->oclass != SCROLL_CLASS
		&& scroll->oclass != SPBOOK_CLASS) {
	    pline("That is a silly thing to read.");
	    return 0;
	} else if (u.roleplay.illiterate && scroll->otyp != SPE_BOOK_OF_THE_DEAD) {
	    pline("Unfortunately, you cannot read.");
	    return 0;
	} else if (Blind && scroll->otyp != SPE_BOOK_OF_THE_DEAD) {
	    const char *what = 0;
	    if (scroll->oclass == SPBOOK_CLASS)
		what = "mystic runes";
	    else if (!scroll->dknown)
		what = "formula on the scroll";
	    if (what) {
		pline("Being blind, you cannot read the %s.", what);
		return 0;
	    }
	}

	/* Actions required to win the game aren't counted towards conduct */
	if (scroll->otyp != SPE_BOOK_OF_THE_DEAD &&
		scroll->otyp != SPE_BLANK_PAPER &&
		scroll->otyp != SCR_BLANK_PAPER)
	    violated(CONDUCT_ILLITERACY);

	confused = (Confusion != 0);
	if (scroll->oclass == SPBOOK_CLASS) {
	    return study_book(scroll);
	}
	scroll->in_use = TRUE;	/* scroll, not spellbook, now being read */
	if (scroll->otyp != SCR_BLANK_PAPER) {
	  if (Blind)
	    pline("As you %s the formula on it, the scroll disappears.",
			is_silent(youmonst.data) ? "cogitate" : "pronounce");
	  else
	    pline("As you read the scroll, it disappears.");
	  if (confused) {
	    if (Hallucination)
		pline("Being so trippy, you screw up...");
	    else
		pline("Being confused, you mis%s the magic words...",
			is_silent(youmonst.data) ? "understand" : "pronounce");
	  }
	}
	if (!seffects(scroll, &known))  {
		if (!objects[scroll->otyp].oc_name_known) {
		    if (known) {
			makeknown(scroll->otyp);
			more_experienced(0, 0, 10);
		    } else if (!objects[scroll->otyp].oc_uname)
			docall(scroll);
		}
		if (scroll->otyp != SCR_BLANK_PAPER)
			useup(scroll);
		else scroll->in_use = FALSE;
	}
	return 1;
}

static void stripspe(struct obj *obj)
{
	if (obj->blessed) pline("Nothing happens.");
	else {
		if (obj->spe > 0) {
		    obj->spe = 0;
		    if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
			obj->age = 0;
		    pline("Your %s %s briefly.",xname(obj), otense(obj, "vibrate"));
		} else pline("Nothing happens.");
	}
}

static void p_glow1(struct obj *otmp)
{
	pline("Your %s %s briefly.", xname(otmp),
	     otense(otmp, Blind ? "vibrate" : "glow"));
}

static void p_glow2(struct obj *otmp, const char *color)
{
	pline("Your %s %s%s%s for a moment.",
		xname(otmp),
		otense(otmp, Blind ? "vibrate" : "glow"),
		Blind ? "" : " ",
		Blind ? nul : hcolor(color));
}

/* Is the object chargeable?  For purposes of inventory display; it is */
/* possible to be able to charge things for which this returns FALSE. */
boolean is_chargeable(struct obj *obj)
{
	if (obj->oclass == WAND_CLASS) return TRUE;
	/* known && !uname is possible after amnesia/mind flayer */
	if (obj->oclass == RING_CLASS)
	    return (boolean)(objects[obj->otyp].oc_charged &&
			(obj->known || objects[obj->otyp].oc_uname));
	if (is_weptool(obj))	/* specific check before general tools */
	    return FALSE;
	/* Magic lamps can't be recharged, but they should be listed
	 * to prevent telling them apart from oil lamps with charging.
	 */
	if (obj->oclass == TOOL_CLASS) {
	    return (boolean)(objects[obj->otyp].oc_charged ||
			     obj->otyp == BRASS_LANTERN ||
			     obj->otyp == OIL_LAMP ||
			     obj->otyp == MAGIC_LAMP);
	}
	return FALSE; /* why are weapons/armor considered charged anyway? */
}

/*
 * recharge an object; curse_bless is -1 if the recharging implement
 * was cursed, +1 if blessed, 0 otherwise.
 */
void recharge(struct obj *obj, int curse_bless)
{
	int n;
	boolean is_cursed, is_blessed;

	is_cursed = curse_bless < 0;
	is_blessed = curse_bless > 0;

	/* Scrolls of charging now ID charge count, as well as doing
	 * the charging, unless cursed. */
	if (!is_cursed) obj->known = 1;

	if (obj->oclass == WAND_CLASS) {
	    /* undo any prior cancellation, even when is_cursed */
	    if (obj->spe == -1) obj->spe = 0;

	    /*
	     * Recharging might cause wands to explode.
	     *	v = number of previous recharges
	     *	      v = percentage chance to explode on this attempt
	     *		      v = cumulative odds for exploding
	     *	0 :   0       0
	     *	1 :   0.29    0.29
	     *	2 :   2.33    2.62
	     *	3 :   7.87   10.28
	     *	4 :  18.66   27.02
	     *	5 :  36.44   53.62
	     *	6 :  62.97   82.83
	     *	7 : 100     100
	     */
	    n = (int)obj->recharged;
	    if (n > 0 && (obj->otyp == WAN_WISHING ||
		    (n * n * n > rn2(7*7*7)))) {	/* recharge_limit */
		wand_explode(obj);
		return;
	    }
	    /* didn't explode, so increment the recharge count */
	    obj->recharged = (unsigned)(n + 1);

	    /* now handle the actual recharging */
	    if (is_cursed) {
		stripspe(obj);
	    } else {
		int lim = (obj->otyp == WAN_WISHING) ? 3 :
			(objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;

		n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
		if (!is_blessed) n = rnd(n);

		if (obj->spe < n) obj->spe = n;
		else obj->spe++;
		if (obj->otyp == WAN_WISHING && obj->spe > 3) {
		    wand_explode(obj);
		    return;
		}
		if (obj->spe >= lim) p_glow2(obj, "blue");
		else p_glow1(obj);
	    }

	} else if (obj->oclass == RING_CLASS &&
					objects[obj->otyp].oc_charged) {
	    /* charging does not affect ring's curse/bless status */
	    int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
	    boolean is_on = (obj == uleft || obj == uright);

	    /* destruction depends on current state, not adjustment */
	    if (obj->spe > rn2(6) + 3 || (is_cursed && obj->spe <= -5)) {
		pline("Your %s %s momentarily, then %s!",
		     xname(obj), otense(obj,"pulsate"), otense(obj,"explode"));
		if (is_on) Ring_gone(obj);
		s = rnd(3 * abs(obj->spe));	/* amount of damage */
		useup(obj);
		losehp(s, "exploding ring", KILLED_BY_AN);
	    } else {
		long mask = is_on ? (obj == uleft ? LEFT_RING :
				     RIGHT_RING) : 0L;
		pline("Your %s spins %sclockwise for a moment.",
		     xname(obj), s < 0 ? "counter" : "");
		/* cause attributes and/or properties to be updated */
		if (is_on) Ring_off(obj);
		obj->spe += s;	/* update the ring while it's off */
		if (is_on) setworn(obj, mask), Ring_on(obj, FALSE);
		/* oartifact: if a touch-sensitive artifact ring is
		   ever created the above will need to be revised  */
	    }

	} else if (obj->oclass == TOOL_CLASS) {
	    int rechrg = (int)obj->recharged;

	    /* tools don't have a limit, but the counter used does */
	    if (rechrg < 7)	/* recharge_limit */
		obj->recharged++;

	    switch(obj->otyp) {
	    case BELL_OF_OPENING:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) obj->spe += rnd(3);
		else obj->spe += 1;
		if (obj->spe > 5) obj->spe = 5;
		break;
	    case MAGIC_MARKER:
	    case TINNING_KIT:
	    case EXPENSIVE_CAMERA:
		if (is_cursed) stripspe(obj);
		else if (rechrg && obj->otyp == MAGIC_MARKER) {	/* previously recharged */
		    obj->recharged = 1;	/* override increment done above */
		    if (obj->spe < 3)
			pline("Your marker seems permanently dried out.");
		    else
			pline("Nothing happens.");
		} else if (is_blessed) {
		    n = rn1(16,15);		/* 15..30 */
		    if (obj->spe + n <= 50)
			obj->spe = 50;
		    else if (obj->spe + n <= 75)
			obj->spe = 75;
		    else {
			int chrg = (int)obj->spe;
			if ((chrg + n) > 127)
				obj->spe = 127;
			else
				obj->spe += n;
		    }
		    p_glow2(obj, "blue");
		} else {
		    n = rn1(11,10);		/* 10..20 */
		    if (obj->spe + n <= 50)
			obj->spe = 50;
		    else {
			int chrg = (int)obj->spe;
			if ((chrg + n) > 127)
				obj->spe = 127;
			else
				obj->spe += n;
		    }
		    p_glow2(obj, "white");
		}
		break;
	    case OIL_LAMP:
	    case BRASS_LANTERN:
		if (is_cursed) {
		    stripspe(obj);
		    if (obj->lamplit) {
			if (!Blind)
			    pline("%s out!", Tobjnam(obj, "go"));
			end_burn(obj, TRUE);
		    }
		} else if (is_blessed) {
		    obj->spe = 1;
		    obj->age = 1500;
		    p_glow2(obj, "blue");
		} else {
		    obj->spe = 1;
		    obj->age += 750;
		    if (obj->age > 1500) obj->age = 1500;
		    p_glow1(obj);
		}
		break;
	    case CRYSTAL_BALL:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    obj->spe = 6;
		    p_glow2(obj, "blue");
		} else {
		    if (obj->spe < 5) {
			obj->spe++;
			p_glow1(obj);
		    } else pline("Nothing happens.");
		}
		break;
	    case BAG_OF_TRICKS:
		/* if there are any objects inside the bag, devour them */
		if (!is_cursed) {
		    struct obj *curr, *otmp;
		    struct monst *shkp;
		    int lcnt = 0;
		    long loss = 0L;

		    makeknown(BAG_OF_TRICKS);
		    for (curr = obj->cobj; curr; curr = otmp) {
			otmp = curr->nobj;
			obj_extract_self(curr);
			lcnt++;
			if (*u.ushops && (shkp = shop_keeper(level, *u.ushops)) != 0) {
			    if (curr->unpaid)
				loss += stolen_value(curr, u.ux, u.uy,
						     (boolean)shkp->mpeaceful, TRUE);
			}
			/* obfree() will free all contained objects */
			obfree(curr, NULL);
		    }

		    if (lcnt)
			You_hear("loud crunching sounds from inside %s.", yname(obj));
		    if (lcnt && loss)
			pline("You owe %ld %s for lost item%s.",
			      loss, currency(loss), lcnt > 1 ? "s" : "");
		}
		/* fall through */
	    case HORN_OF_PLENTY:
	    case CAN_OF_GREASE:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (obj->spe <= 10)
			obj->spe += rn1(10, 6);
		    else obj->spe += rn1(5, 6);
		    if (obj->spe > 50) obj->spe = 50;
		    p_glow2(obj, "blue");
		} else {
		    obj->spe += rnd(5);
		    if (obj->spe > 50) obj->spe = 50;
		    p_glow1(obj);
		}
		break;
	    case MAGIC_FLUTE:
	    case MAGIC_HARP:
	    case FROST_HORN:
	    case FIRE_HORN:
	    case DRUM_OF_EARTHQUAKE:
		if (is_cursed) {
		    stripspe(obj);
		} else if (is_blessed) {
		    obj->spe += dice(2,4);
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow2(obj, "blue");
		} else {
		    obj->spe += rnd(4);
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow1(obj);
		}
		break;
	    default:
		goto not_chargable;
		/*NOTREACHED*/
		break;
	    } /* switch */

	} else {
 not_chargable:
	    pline("You have a feeling of loss.");
	}
}


/* Forget known information about this object class. */
static void forget_single_object(int obj_id)
{
	objects[obj_id].oc_name_known = 0;
	objects[obj_id].oc_pre_discovered = 0;	/* a discovery when relearned */
	if (objects[obj_id].oc_uname) {
	    free(objects[obj_id].oc_uname);
	    objects[obj_id].oc_uname = 0;
	}
	undiscover_object(obj_id);	/* after clearing oc_name_known */

	if (Is_dragon_scales(obj_id) &&
	    objects[Dragon_scales_to_mail(obj_id)].oc_name_known) {
	    forget_single_object(Dragon_scales_to_mail(obj_id));
	} else if (Is_dragon_mail(obj_id) &&
		   objects[Dragon_mail_to_scales(obj_id)].oc_name_known) {
	    forget_single_object(Dragon_mail_to_scales(obj_id));
	}
	/* clear & free object names from matching inventory items too? */
}


/* randomize the given list of numbers  0 <= i < count */
static void randomize(int *indices, int count)
{
	int i, iswap, temp;

	for (i = count - 1; i > 0; i--) {
	    if ((iswap = rn2(i + 1)) == i) continue;
	    temp = indices[i];
	    indices[i] = indices[iswap];
	    indices[iswap] = temp;
	}
}


/* Forget % of known objects. */
void forget_objects(int percent)
{
	int i, count;
	int indices[NUM_OBJECTS];

	if (percent == 0) return;
	if (percent <= 0 || percent > 100) {
	    impossible("forget_objects: bad percent %d", percent);
	    return;
	}

	for (count = 0, i = 1; i < NUM_OBJECTS; i++)
	    if (OBJ_DESCR(objects[i]) &&
		    (objects[i].oc_name_known || objects[i].oc_uname))
		indices[count++] = i;

	randomize(indices, count);

	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++)
	    forget_single_object(indices[i]);
}


/* Forget some or all of map (depends on parameters). */
static void forget_map(struct level *lev, boolean forget_all)
{
	int zx, zy;

	if (In_sokoban(&lev->z))
	    return;

	for (zx = 0; zx < COLNO; zx++) for(zy = 0; zy < ROWNO; zy++)
	    if (forget_all || rn2(7)) {
		/* Zonk all memory of this location. */
		lev->locations[zx][zy].seenv = 0;
		lev->locations[zx][zy].waslit = 0;
		lev->locations[zx][zy].mem_bg = S_unexplored;
		lev->locations[zx][zy].mem_trap = 0;
		lev->locations[zx][zy].mem_obj = 0;
		lev->locations[zx][zy].mem_obj_mn = 0;
		lev->locations[zx][zy].mem_obj_stacks = 0;
		lev->locations[zx][zy].mem_obj_prize = 0;
		lev->locations[zx][zy].mem_invis = 0;
		lev->locations[zx][zy].mem_stepped = 0;
	    }
}

/* Forget all traps on the level. */
static void forget_traps(struct level *lev)
{
	struct trap *trap;

	/* forget all traps (except the one the hero is in :-) */
	for (trap = lev->lev_traps; trap; trap = trap->ntrap)
	    if ((trap->tx != u.ux || trap->ty != u.uy) && (trap->ttyp != HOLE))
		trap->tseen = 0;
}

/*
 * Forget given % of all levels that the hero has visited and not forgotten,
 * except this one.
 */
void forget_levels(int percent)
{
	int i, count;
	xchar  maxl, this_lev;
	int indices[MAXLINFO];

	if (percent == 0) return;

	if (percent <= 0 || percent > 100) {
	    impossible("forget_levels: bad percent %d", percent);
	    return;
	}

	this_lev = ledger_no(&u.uz);
	maxl = maxledgerno();

	/* count & save indices of non-forgotten visited levels */
	/* Sokoban levels are pre-mapped for the player, and should stay
	 * so, or they become nearly impossible to solve.  But try to
	 * shift the forgetting elsewhere by fiddling with percent
	 * instead of forgetting fewer levels.
	 */
	for (count = 0, i = 0; i <= maxl; i++)
	    if ((levels[i] && !levels[i]->flags.forgotten) && i != this_lev) {
		if (ledger_to_dnum(i) == sokoban_dnum)
		    percent += 2;
		else
		    indices[count++] = i;
	    }
	
	if (percent > 100) percent = 100;

	randomize(indices, count);

	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++) {
	    levels[indices[i]]->flags.forgotten = TRUE;
	    forget_map(levels[indices[i]], TRUE);
	    forget_traps(levels[indices[i]]);
	    levels[indices[i]]->levname[0] = '\0';
	}
}

/* monster is hit by scroll of taming's effect */
static void maybe_tame(struct monst *mtmp, struct obj *sobj)
{
	if (sobj->cursed || Is_blackmarket(&u.uz)) {
	    setmangry(mtmp);
	} else {
	    if (mtmp->isshk)
		make_happy_shk(mtmp, FALSE);
	    else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
		tamedog(mtmp, NULL);
	}
}

/* Remove water tile at (x, y). */
static void undo_flood(int x, int y, void *roomcnt)
{
	struct rm *loc = &level->locations[x][y];

	if (loc->typ != POOL &&
	    loc->typ != MOAT &&
	    loc->typ != WATER &&
	    loc->typ != FOUNTAIN)
	    return;

	(*(int *)roomcnt)++;

	/* Get rid of a pool at x, y */
	loc->typ = ROOM;
	newsym(x, y);
}

static void do_flood(int x, int y, void *poolcnt)
{
	struct monst *mtmp;
	struct trap *ttmp;

	if (nexttodoor(level, x, y) ||
	    rn2(1 + distmin(u.ux, u.uy, x, y)) ||
	    sobj_at(BOULDER, level, x, y) ||
	    level->locations[x][y].typ != ROOM)
	    return;

	if ((ttmp = t_at(level, x, y)) != 0 && !delfloortrap(ttmp))
	    return;

	(*(int *)poolcnt)++;

	if (!((*(int *)poolcnt) && x == u.ux && y == u.uy)) {
	    /* Put a pool at x, y */
	    level->locations[x][y].typ = POOL;
	    del_engr_at(level, x, y);
	    water_damage(level->objects[x][y], FALSE, TRUE);

	    if ((mtmp = m_at(level, x, y)) != 0)
		minliquid(mtmp);
	    else
		newsym(x, y);
	} else if (x == u.ux && y == u.uy) {
	    (*(int *)poolcnt)--;
	}
}

/*
 * scroll effects
 *
 * Returns 0 if the caller should use up the object,
 * 1 if the object is used up internally, and
 * 2 if the effect is a casted spell that should be aborted.
 */
int seffects(struct obj *sobj, boolean *known)
{
	int cval;
	boolean confused = (Confusion != 0);
	struct obj *otmp;

	if (objects[sobj->otyp].oc_magic)
		exercise(A_WIS, TRUE);		/* just for trying */
	switch(sobj->otyp) {
	case SCR_ENCHANT_ARMOR:
	    {
		schar s;
		boolean special_armor;
		boolean same_color;

		otmp = some_armor(&youmonst);
		if (!otmp) {
			strange_feeling(sobj,
					!Blind ? "Your skin glows then fades." :
					"Your skin feels warm for a moment.");
			exercise(A_CON, !sobj->cursed);
			exercise(A_STR, !sobj->cursed);
			return 1;
		}
		if (confused) {
			otmp->oerodeproof = !(sobj->cursed);
			if (Blind) {
			    otmp->rknown = sobj->bknown;
			    pline("Your %s %s warm for a moment.",
				xname(otmp), otense(otmp, "feel"));
			} else {
			    otmp->rknown = TRUE;
			    pline("Your %s %s covered by a %s %s %s!",
				xname(otmp), otense(otmp, "are"),
				sobj->cursed ? "mottled" : "shimmering",
				 hcolor(sobj->cursed ? "black" : "golden"),
				sobj->cursed ? "glow" :
				  (is_shield(otmp) ? "layer" : "shield"));
			}
			if (otmp->oerodeproof &&
			    (otmp->oeroded || otmp->oeroded2)) {
			    otmp->oeroded = otmp->oeroded2 = 0;
			    pline("Your %s %s as good as new!",
				 xname(otmp),
				 otense(otmp, Blind ? "feel" : "look"));
			}
			break;
		}
		/* sometimes armor can be enchanted to a higher limit than usual */
		special_armor = is_elven_armor(otmp) ||
			(Role_if (PM_WIZARD) && otmp->otyp == CORNUTHAUM);
		if (sobj->cursed)
		    same_color =
			(otmp->otyp == BLACK_DRAGON_SCALE_MAIL ||
			 otmp->otyp == BLACK_DRAGON_SCALES);
		else
		    same_color =
			(otmp->otyp == SILVER_DRAGON_SCALE_MAIL ||
			 otmp->otyp == SILVER_DRAGON_SCALES ||
			 otmp->otyp == SHIELD_OF_REFLECTION);
		if (Blind) same_color = FALSE;

		/* KMH -- catch underflow */
		s = sobj->cursed ? -otmp->spe : otmp->spe;
		if (s > (special_armor ? 5 : 3) && rn2(s)) {
		pline("Your %s violently %s%s%s for a while, then %s.",
		     xname(otmp),
		     otense(otmp, Blind ? "vibrate" : "glow"),
		     (!Blind && !same_color) ? " " : nul,
		     (Blind || same_color) ? nul :
			hcolor(sobj->cursed ? "black" : "silver"),
		     otense(otmp, "evaporate"));
			if (is_cloak(otmp)) Cloak_off();
			if (is_boots(otmp)) Boots_off();
			if (is_helmet(otmp)) Helmet_off();
			if (is_gloves(otmp)) Gloves_off();
			if (is_shield(otmp)) Shield_off();
			if (otmp == uarm) Armor_gone(FALSE);
			useup(otmp);
			break;
		}
		s = sobj->cursed ? -1 :
		    otmp->spe >= 9 ? (rn2(otmp->spe) == 0) :
		    sobj->blessed ? rnd(3-otmp->spe/3) : 1;
		if (s >= 0 && otmp->otyp >= GRAY_DRAGON_SCALES &&
			      otmp->otyp <= CHROMATIC_DRAGON_SCALES) {
			/* dragon scales get turned into dragon scale mail */
			pline("Your %s merges and hardens!", xname(otmp));
			setworn(NULL, W_ARM);
			/* assumes same order */
			otmp->otyp = GRAY_DRAGON_SCALE_MAIL +
						otmp->otyp - GRAY_DRAGON_SCALES;
			otmp->cursed = 0;
			if (sobj->blessed) {
				otmp->spe++;
				otmp->blessed = 1;
			}
			otmp->known = 1;
			setworn(otmp, W_ARM);
			*known = TRUE;
			break;
		}
		pline("Your %s %s%s%s%s for a %s.",
			xname(otmp),
		        s == 0 ? "violently " : nul,
			otense(otmp, Blind ? "vibrate" : "glow"),
			(!Blind && !same_color) ? " " : nul,
			(Blind || same_color) ? nul : hcolor(sobj->cursed ? "black" : "silver"),
			  (s*s>1) ? "while" : "moment");
		otmp->cursed = sobj->cursed;
		if (!otmp->blessed || sobj->cursed)
			otmp->blessed = sobj->blessed;
		if (s) {
			otmp->spe += s;
			adj_abon(otmp, s);
			*known = otmp->known;
		}

		/* armor vibrates warningly when enchanted beyond a limit */
		if (otmp->spe > (special_armor ? 5 : 3))
			pline("Your %s suddenly %s %s.",
				xname(otmp), otense(otmp, "vibrate"),
				Blind ? "again" : "unexpectedly");
		break;
	    }
	case SCR_DESTROY_ARMOR:
	    {
		otmp = some_armor(&youmonst);
		if (confused) {
			if (!otmp) {
				strange_feeling(sobj,"Your bones itch.");
				exercise(A_STR, FALSE);
				exercise(A_CON, FALSE);
				return 1;
			}
			otmp->oerodeproof = sobj->cursed;
			p_glow2(otmp, "purple");
			break;
		}
		if (!sobj->cursed || !otmp || !otmp->cursed) {
		    if (!destroy_arm(otmp, FALSE)) {
			strange_feeling(sobj,"Your skin itches.");
			exercise(A_STR, FALSE);
			exercise(A_CON, FALSE);
			return 1;
		    } else
			*known = TRUE;
		} else {	/* armor and scroll both cursed */
		    pline("Your %s %s.", xname(otmp), otense(otmp, "vibrate"));
		    if (otmp->spe >= -6) {
			otmp->spe--;
			if (otmp->otyp == HELM_OF_BRILLIANCE) {
			    ABON(A_INT)--;
			    ABON(A_WIS)--;
			    makeknown(otmp->otyp);
			    iflags.botl = 1;
			}
			if (otmp->otyp == GAUNTLETS_OF_DEXTERITY) {
			    ABON(A_DEX)--;
			    makeknown(otmp->otyp);
			    iflags.botl = 1;
			}
		    }
		    make_stunned(HStun + rn1(10, 10), TRUE);
		}
	    }
	    break;
	case SCR_CONFUSE_MONSTER:
	case SPE_CONFUSE_MONSTER:
		if (youmonst.data->mlet != S_HUMAN || sobj->cursed) {
			if (!HConfusion) pline("You feel confused.");
			make_confused(HConfusion + rnd(100),FALSE);
		} else  if (confused) {
		    if (!sobj->blessed) {
			pline("Your %s begin to %s%s.",
			    makeplural(body_part(HAND)),
			    Blind ? "tingle" : "glow ",
			    Blind ? nul : hcolor("purple"));
			make_confused(HConfusion + rnd(100),FALSE);
		    } else {
			pline("A %s%s surrounds your %s.",
			    Blind ? nul : hcolor("red"),
			    Blind ? "faint buzz" : " glow",
			    body_part(HEAD));
			make_confused(0L,TRUE);
		    }
		} else {
		    if (!sobj->blessed) {
			pline("Your %s%s %s%s.",
			makeplural(body_part(HAND)),
			Blind ? "" : " begin to glow",
			Blind ? (const char *)"tingle" : hcolor("red"),
			u.umconf ? " even more" : "");
			u.umconf++;
		    } else {
			if (Blind)
			    pline("Your %s tingle %s sharply.",
				makeplural(body_part(HAND)),
				u.umconf ? "even more" : "very");
			else
			    pline("Your %s glow a%s brilliant %s.",
				makeplural(body_part(HAND)),
				u.umconf ? "n even more" : "",
				hcolor("red"));
			/* after a while, repeated uses become less effective */
			if (u.umconf >= 40)
			    u.umconf++;
			else
			    u.umconf += rn1(8, 2);
		    }
		}
		break;
	case SCR_SCARE_MONSTER:
	case SPE_CAUSE_FEAR:
	    {	int ct = 0;
		struct monst *mtmp;

		for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if (cansee(mtmp->mx,mtmp->my)) {
			if (confused || sobj->cursed) {
			    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
			    mtmp->mcanmove = 1;
			} else
			    if (! resist(mtmp, sobj->oclass, 0, NOTELL))
				monflee(mtmp, 0, FALSE, FALSE);
			if (!mtmp->mtame) ct++;	/* pets don't laugh at you */
		    }
		}
		if (!ct)
		      You_hear("%s in the distance.",
			       (confused || sobj->cursed) ? "sad wailing" :
							"maniacal laughter");
		else if (sobj->otyp == SCR_SCARE_MONSTER)
			You_hear("%s close by.",
				  (confused || sobj->cursed) ? "sad wailing" :
						 "maniacal laughter");
		break;
	    }
	case SCR_BLANK_PAPER:
	    if (Blind)
		pline("You don't remember there being any magic words on this scroll.");
	    else
		pline("This scroll seems to be blank.");
	    *known = TRUE;
	    break;
	case SCR_REMOVE_CURSE:
	case SPE_REMOVE_CURSE:
	    {	struct obj *obj;
		if (confused)
		    if (Hallucination)
			pline("You feel the power of the Force against you!");
		    else
			pline("You feel like you need some help.");
		else
		    if (Hallucination)
			pline("You feel in touch with the Universal Oneness.");
		    else
			pline("You feel like someone is helping you.");

		if (sobj->cursed) {
		    pline("The scroll disintegrates.");
		} else {
		    for (obj = invent; obj; obj = obj->nobj) {
			long wornmask;
			/* gold isn't subject to cursing and blessing */
			if (obj->oclass == COIN_CLASS) continue;

			wornmask = (obj->owornmask & ~(W_BALL|W_ART|W_ARTI));
			if (wornmask && !sobj->blessed) {
			    /* handle a couple of special cases; we don't
			       allow auxiliary weapon slots to be used to
			       artificially increase number of worn items */
			    if (obj == uswapwep) {
				if (!u.twoweap) wornmask = 0L;
			    } else if (obj == uquiver) {
				if (obj->oclass == WEAPON_CLASS) {
				    /* mergeable weapon test covers ammo,
				       missiles, spears, daggers & knives */
				    if (!objects[obj->otyp].oc_merge) 
					wornmask = 0L;
				} else if (obj->oclass == GEM_CLASS) {
				    /* possibly ought to check whether
				       alternate weapon is a sling... */
				    if (!uslinging()) wornmask = 0L;
				} else {
				    /* weptools don't merge and aren't
				       reasonable quivered weapons */
				    wornmask = 0L;
				}
			    }
			}
			if (sobj->blessed || wornmask ||
			     obj->otyp == LOADSTONE ||
			     (obj->otyp == LEASH && obj->leashmon)) {
			    if (confused) blessorcurse(obj, 2);
			    else uncurse(obj);
			}
		    }
		}
		if (Punished && !confused) unpunish();
		update_inventory();
		break;
	    }
	case SCR_CREATE_MONSTER:
	case SPE_CREATE_MONSTER:
	    if (create_critters(1 + ((confused || sobj->cursed) ? 12 : 0) +
				((sobj->blessed || rn2(73)) ? 0 : rnd(4)),
			confused ? &mons[PM_ACID_BLOB] : NULL))
		*known = TRUE;
	    /* no need to flush monsters; we ask for identification only if the
	     * monsters are not visible
	     */
	    break;
	case SCR_ENCHANT_WEAPON:
		if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep))
			&& confused) {
		/* oclass check added 10/25/86 GAN */
			uwep->oerodeproof = !(sobj->cursed);
			if (Blind) {
			    uwep->rknown = sobj->bknown;
			    pline("Your weapon feels warm for a moment.");
			} else {
			    uwep->rknown = TRUE;
			    pline("Your %s covered by a %s %s %s!",
				aobjnam(uwep, "are"),
				sobj->cursed ? "mottled" : "shimmering",
				hcolor(sobj->cursed ? "purple" : "golden"),
				sobj->cursed ? "glow" : "shield");
			}
			if (uwep->oerodeproof && (uwep->oeroded || uwep->oeroded2)) {
			    uwep->oeroded = uwep->oeroded2 = 0;
			    pline("Your %s as good as new!",
				 aobjnam(uwep, Blind ? "feel" : "look"));
			}
		} else return !chwepon(sobj,
				       sobj->cursed ? -1 :
				       !uwep ? 1 :
				       uwep->spe >= 9 ? (rn2(uwep->spe) == 0) :
				       sobj->blessed ? rnd(3-uwep->spe/3) : 1);
		break;
	case SCR_TAMING:
	case SPE_CHARM_MONSTER:
		if (u.uswallow) {
		    maybe_tame(u.ustuck, sobj);
		} else {
		    int i, j, bd = confused ? 5 : 1;
		    struct monst *mtmp;

		    for (i = -bd; i <= bd; i++) for (j = -bd; j <= bd; j++) {
			if (!isok(u.ux + i, u.uy + j)) continue;
			if ((mtmp = m_at(level, u.ux + i, u.uy + j)) != 0)
			    maybe_tame(mtmp, sobj);
		    }
		}
		break;
	case SCR_GENOCIDE:
		pline("You have found a scroll of genocide!");
		*known = TRUE;
		do_genocide((!sobj->cursed) | (2 * !!Confusion), !sobj->blessed);
		break;
	case SCR_LIGHT:
		if (!Blind) *known = TRUE;
		litroom(!confused && !sobj->cursed, sobj);
		break;
	case SCR_TELEPORTATION:
		if (confused || sobj->cursed) level_tele();
		else {
			if (sobj->blessed && !Teleport_control) {
				*known = TRUE;
				if (yn("Do you wish to teleport?")=='n')
					break;
			}
			tele(NULL);
			if (Teleport_control || !couldsee(u.ux0, u.uy0) ||
			   (distu(u.ux0, u.uy0) >= 16))
				*known = TRUE;
		}
		break;
	case SCR_GOLD_DETECTION:
		if (confused || (sobj->cursed && In_endgame(&u.uz))) {
		    /* detect random item class */
		    int random_classes[] = {
			WEAPON_CLASS, ARMOR_CLASS, RING_CLASS, AMULET_CLASS,
			TOOL_CLASS, FOOD_CLASS, POTION_CLASS, SCROLL_CLASS,
			SPBOOK_CLASS, WAND_CLASS, COIN_CLASS, GEM_CLASS
		    };
		    return object_detect(NULL,
					 random_classes[rn2(SIZE(random_classes))],
					 FALSE);
		} else if (sobj->cursed) return trap_detect(sobj);
		else return gold_detect(sobj, known);
	case SCR_FOOD_DETECTION:
	case SPE_DETECT_FOOD:
		if (food_detect(sobj, known))
			return 1;	/* nothing detected */
		break;
	case SPE_IDENTIFY:
		cval = rn2(5);
		goto id;
	case SCR_IDENTIFY:
		/* known = TRUE; */
		if (confused)
			pline("You identify this as an identify scroll.");
		else
			pline("This is an identify scroll.");
		win_pause_output(P_MESSAGE);	/* --More-- */
		/* identify 3 to 6 items regardless of BUC status */
		cval = rn1(4, 3);
		if (!objects[sobj->otyp].oc_name_known) more_experienced(0, 0, 10);
		useup(sobj);
		makeknown(SCR_IDENTIFY);
	id:
		if (invent && !confused) {
		    int num_identified;
		    num_identified = identify_pack(cval);
		    if (sobj->otyp == SPE_IDENTIFY && num_identified == 0) {
			/* allow identify spell to be aborted */
			return 2;
		    }
		}
		return 1;
	case SCR_CHARGING:
		if (confused) {
		    pline("You feel charged up!");
		    if (u.uen < u.uenmax)
			u.uen = u.uenmax;
		    else
			u.uen = (u.uenmax += dice(5,4));
		    iflags.botl = 1;
		    break;
		}
		*known = TRUE;
		pline("This is a charging scroll.");
		otmp = getobj(all_count, "charge", NULL);
		if (!otmp) break;
		recharge(otmp, sobj->cursed ? -1 : (sobj->blessed ? 1 : 0));
		break;
	case SCR_MAGIC_MAPPING:
		if (level->flags.nommap) {
		    pline("Your mind is filled with crazy lines!");
		    if (Hallucination)
			pline("Wow!  Modern art.");
		    else
			pline("Your %s spins in bewilderment.", body_part(HEAD));
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		/* reveal secret doors for uncursed and blessed scrolls */
		if (!sobj->cursed) {
		    int x, y;

		    for (x = 1; x < COLNO; x++)
			for (y = 0; y < ROWNO; y++)
			    if (level->locations[x][y].typ == SDOOR)
				cvt_sdoor_to_door(&level->locations[x][y], &u.uz);
		    /* do_mapping() already reveals secret passages */
		}
		*known = TRUE;
	case SPE_MAGIC_MAPPING:
		if (level->flags.nommap) {
		    pline("Your %s spins as something blocks the spell!", body_part(HEAD));
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		pline("A map coalesces in your mind!");
		cval = (sobj->cursed && !confused);
		if (cval) HConfusion = 1;	/* to screw up map */
		do_mapping();
		/* objects, too, pal! */
		if (sobj->blessed && !cval)
		    object_detect(sobj, 0, TRUE);
		if (cval) {
		    HConfusion = 0;		/* restore */
		    pline("Unfortunately, you can't grasp the details.");
		}
		break;
	case SCR_FLOOD:
		if (confused) {
		    /* remove water from vicinity of player */
		    int maderoom = 0;
		    do_clear_area(u.ux, u.uy, 4 + 2 * bcsign(sobj),
				  undo_flood, &maderoom);
		    if (maderoom) {
			*known = TRUE;
			pline("You are suddenly very dry!");
		    }
		} else {
		    int madepool = 0;
		    int stilldry = -1;
		    int x, y, safe_pos = 0;

		    if (!sobj->cursed)
			do_clear_area(u.ux, u.uy, 5, do_flood, &madepool);

		    /* check if there safe tiles around the player */
		    for (x = u.ux - 1; x <= u.ux + 1; x++) {
			for (y = u.uy - 1; y <= u.uy + 1; y++) {
			    if (x != u.ux && y != u.uy &&
				goodpos(level, x, y, &youmonst, 0)) {
				safe_pos++;
			    }
			}
		    }
		    /* cursed/uncursed may put water on player's position */
		    if (!sobj->blessed && safe_pos > 0)
			do_flood(u.ux, u.uy, &stilldry);

		    if (!madepool && stilldry)
			break;
		    if (madepool)
			pline(Hallucination ? "A totally gnarly wave comes in!" :
					      "A flood surges through the area!" );
		    if (!stilldry && !Wwalking && !Flying && !Levitation)
			drown();
		    *known = TRUE;
		}
		break;
	case SCR_FIRE:
		/*
		 * Note: Modifications have been made as of 3.0 to allow for
		 * some damage under all potential cases.
		 */
		cval = bcsign(sobj);
		if (!objects[sobj->otyp].oc_name_known) more_experienced(0, 0, 10);
		useup(sobj);
		makeknown(SCR_FIRE);
		if (confused) {
		    if (FFire_resistance) {
			shieldeff(u.ux, u.uy);
			if (!Blind)
			    pline("Oh, look, what a pretty fire in your %s.",
				makeplural(body_part(HAND)));
			else pline("You feel a pleasant warmth in your %s.",
				makeplural(body_part(HAND)));
		    } else {
			pline("The scroll catches fire and you burn your %s.",
				makeplural(body_part(HAND)));
			losehp(1, "scroll of fire", KILLED_BY_AN);
		    }
		    return 1;
		}
		if (Underwater)
			pline("The water around you vaporizes violently!");
		else {
		    pline("The scroll erupts in a tower of flame!");
		    burn_away_slime();
		}
		explode(u.ux, u.uy, 11, (2*(rn1(3, 3) + 2 * cval) + 1)/3,
							SCROLL_CLASS, EXPL_FIERY);
		return 1;
	case SCR_EARTH:
	    /* TODO: handle steeds */
	    if (!Is_rogue_level(&u.uz) &&
		(!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
		int x, y;
		int boulder_created = 0;

		/* Identify the scroll */
		pline("The %s rumbles %s you!", ceiling(u.ux,u.uy),
				sobj->blessed ? "around" : "above");
		*known = TRUE;
		if (In_sokoban(&u.uz))
		    sokoban_trickster();	/* Sokoban guilt */

		/* Loop through the surrounding squares */
		if (!sobj->cursed) for (x = u.ux-1; x <= u.ux+1; x++) {
		    for (y = u.uy-1; y <= u.uy+1; y++) {
			/* Is this a suitable spot? */
			if (isok(x, y) &&
				!closed_door(level, x, y) &&
				!IS_ROCK(level->locations[x][y].typ) &&
				!IS_AIR(level->locations[x][y].typ) &&
				/* make less boulders in Black Market */
				!(Is_blackmarket(&u.uz) && rn2(2)) &&
				(x != u.ux || y != u.uy)) {
			    boulder_created += drop_boulder_on_monster(x, y, confused, TRUE);
			}
		    }
		}
		/* Attack the player */
		if (!sobj->blessed) {
		    drop_boulder_on_player(confused, !sobj->cursed, TRUE, FALSE);
		} else {
		    if (boulder_created == 0)
			pline("But nothing else happens.");
		}
	    } else if (In_endgame(&u.uz)) {
		You_hear("the %s rumbling below you!", surface(u.ux, u.uy));
	    }
	    break;
	case SCR_PUNISHMENT:
		*known = TRUE;
		if (confused || sobj->blessed) {
			pline("You feel guilty.");
			break;
		}
		punish(sobj);
		break;
	case SCR_STINKING_CLOUD: {
	        coord cc;

		pline("You have found a scroll of stinking cloud!");
		*known = TRUE;
		pline("Where do you want to center the cloud?");
		cc.x = u.ux;
		cc.y = u.uy;
		if (getpos(&cc, FALSE, "the desired position") < 0) {
		    pline("Never mind.");
		    return 0;
		}
		if (!cansee(cc.x, cc.y) || distu(cc.x, cc.y) >= 32) {
		    pline("You smell rotten eggs.");
		    return 0;
		}
		create_gas_cloud(level, cc.x, cc.y, 3+bcsign(sobj),
				 8+4*bcsign(sobj), rn1(3, 4));
		break;
	}
	default:
		impossible("What weird effect is this? (%u)", sobj->otyp);
	}
	return 0;
}

static void wand_explode(struct obj *obj)
{
    obj->in_use = TRUE;	/* in case losehp() is fatal */
    pline("Your %s vibrates violently, and explodes!",xname(obj));
    losehp(rnd(2*(u.uhpmax+1)/3), "exploding wand", KILLED_BY_AN);
    useup(obj);
    exercise(A_STR, FALSE);
}

/*
 * Low-level lit-field update routine.
 */
static void set_lit(int x, int y, void *val)
{
	if (val)
	    level->locations[x][y].lit = 1;
	else {
	    level->locations[x][y].lit = 0;
	    snuff_light_source(x, y);
	}
}

void litroom(boolean on, struct obj *obj)
{
	char is_lit;	/* value is irrelevant; we use its address
			   as a `not null' flag for set_lit() */

	/* first produce the text (provided you're not blind) */
	if (!on) {
		struct obj *otmp;

		if (!Blind) {
		    if (u.uswallow) {
			pline("It seems even darker in here than before.");
			return;
		    }
		    if (uwep && artifact_light(uwep) && uwep->lamplit)
			pline("Suddenly, the only light left comes from %s!",
				the(xname(uwep)));
		    else
			pline("You are surrounded by darkness!");
		}

		/* the magic douses lamps, et al, too */
		for (otmp = invent; otmp; otmp = otmp->nobj)
		    if (otmp->lamplit)
			snuff_lit(otmp);
		if (Blind) goto do_it;
	} else {
		if (Blind) goto do_it;
		if (u.uswallow){
			if (is_animal(u.ustuck->data))
				pline("%s %s is lit.",
				        s_suffix(Monnam(u.ustuck)),
					mbodypart(u.ustuck, STOMACH));
			else
				if (is_whirly(u.ustuck->data))
					pline("%s shines briefly.",
					      Monnam(u.ustuck));
				else
					pline("%s glistens.", Monnam(u.ustuck));
			return;
		}
		pline("A lit field surrounds you!");
	}

do_it:
	/* No-op in water - can only see the adjacent squares and that's it! */
	if (Underwater || Is_waterlevel(&u.uz)) return;
	/*
	 *  If we are darkening the room and the hero is punished but not
	 *  blind, then we have to pick up and replace the ball and chain so
	 *  that we don't remember them if they are out of sight.
	 */
	if (Punished && !on && !Blind)
	    move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

	if (Is_rogue_level(&u.uz)) {
	    /* Can't use do_clear_area because MAX_RADIUS is too small */
	    /* rogue lighting must light the entire room */
	    int rnum = level->locations[u.ux][u.uy].roomno - ROOMOFFSET;
	    int rx, ry;
	    if (rnum >= 0) {
		for (rx = level->rooms[rnum].lx-1; rx <= level->rooms[rnum].hx+1; rx++)
		    for (ry = level->rooms[rnum].ly-1; ry <= level->rooms[rnum].hy+1; ry++)
			set_lit(rx, ry,
				(on ? &is_lit : NULL));
		level->rooms[rnum].rlit = on;
	    }
	    /* hallways remain dark on the rogue level */
	} else
	    do_clear_area(u.ux,u.uy,
		(obj && obj->oclass==SCROLL_CLASS && obj->blessed) ? 9 : 5,
		set_lit, (on ? &is_lit : NULL));

	/*
	 *  If we are not blind, then force a redraw on all positions in sight
	 *  by temporarily blinding the hero.  The vision recalculation will
	 *  correctly update all previously seen positions *and* correctly
	 *  set the waslit bit [could be messed up from above].
	 */
	if (!Blind) {
	    vision_recalc(2);

	    /* replace ball&chain */
	    if (Punished && !on)
		move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
	}

	vision_full_recalc = 1;	/* delayed vision recalculation */
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
/* how: 0 = no genocide; create monsters (cursed scroll) */
/*      1 = normal genocide */
/*      3 = forced genocide of player */
/*      5 (4 | 1) = normal genocide from throne */
/* only_on_level: if TRUE only genocide monsters on current level */
void do_genocide(int how, boolean only_on_level)
{
	char buf[BUFSZ];
	int	i, killplayer = 0;
	int mndx;
	const struct permonst *ptr;
	const char *which;

	if (how & PLAYER) {
		mndx = u.umonster;	/* non-polymorphed mon num */
		ptr = &mons[mndx];
		strcpy(buf, mons_mname(ptr));
		killplayer++;
	} else {
	    for (i = 0; ; i++) {
		if (i >= 5) {
		    pline("That's enough tries!");
		    return;
		}
		getlin("What monster do you want to genocide? [type the name]",
			buf);
		mungspaces(buf);
		/* choosing "none" preserves genocideless conduct */
		if (!strcmpi(buf, "none") || !strcmpi(buf, "nothing")) {
		    /* ... but no free pass if cursed */
		    if (!(how & REALLY)) {
			ptr = rndmonst(level);
			if (!ptr) return; /* no message, like normal case */
			mndx = monsndx(ptr);
			break;		/* remaining checks don't apply */
		    } else return;
		}

		if (wizard && buf[0] == '*') {
		    /* to aid in topology testing; remove pesky monsters */
		    struct monst *mtmp, *mtmp2;

		    int gonecnt = 0;
		    for (mtmp = level->monlist; mtmp; mtmp = mtmp2) {
			mtmp2 = mtmp->nmon;
			if (DEADMONSTER(mtmp)) continue;
			mongone(mtmp);
			gonecnt++;
		    }
		    pline("Eliminated %d monster%s.", gonecnt, plur(gonecnt));
		    return;
		}

		mndx = name_to_mon(buf);
		if (mndx == NON_PM || (mvitals[mndx].mvflags & G_GENOD)) {
			pline("Such creatures %s exist in this world.",
			      (mndx == NON_PM) ? "do not" : "no longer");
			continue;
		}
		ptr = &mons[mndx];
		/* Although "genus" is Latin for race, the hero benefits
		 * from both race and role; thus genocide affects either.
		 */
		if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
			killplayer++;
			break;
		}
		if (is_human(ptr)) adjalign(-sgn(u.ualign.type));
		if (is_demon(ptr)) adjalign(sgn(u.ualign.type));

		if (!(ptr->geno & G_GENO)) {
			if (flags.soundok) {
	/* fixme: unconditional "caverns" will be silly in some circumstances */
			    if (flags.verbose)
			pline("A thunderous voice booms through the caverns:");
			    verbalize("No, mortal!  That will not be done.");
			}
			continue;
		}
		/* KMH -- Unchanging prevents rehumanization */
		if (Unchanging && ptr == youmonst.data)
		    killplayer++;
		break;
	    }
	}

	which = "all ";
	if (Hallucination) {
	    if (Upolyd)
		strcpy(buf, mons_mname(youmonst.data));
	    else {
		strcpy(buf, (flags.female && urole.name.f) ?
				urole.name.f : urole.name.m);
		buf[0] = lowc(buf[0]);
	    }
	} else {
	    strcpy(buf, mons_mname(ptr)); /* make sure we have standard singular */
	    if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_PRIEST])
		which = !type_is_pname(ptr) ? "the " : "";
	}
	if (how & REALLY) {
	    /* setting no-corpse affects wishing and random tin generation */
	    if (!only_on_level)
		mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
	    pline("Wiped out %s%s%s.", which,
		  (*which != 'a') ? buf : makeplural(buf),
		  only_on_level ? " from this level" : "");
	    if (!only_on_level) {
		historic_event(FALSE, "genocided %s%s", which,
			       (*which != 'a') ? buf : makeplural(buf));
	    }

	    if (killplayer) {
		/* might need to wipe out dual role */
		if (!only_on_level) {
		    if (urole.femalenum != NON_PM && mndx == urole.malenum)
			mvitals[urole.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
		    if (urole.femalenum != NON_PM && mndx == urole.femalenum)
			mvitals[urole.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
		    if (urace.femalenum != NON_PM && mndx == urace.malenum)
			mvitals[urace.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
		    if (urace.femalenum != NON_PM && mndx == urace.femalenum)
			mvitals[urace.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
		}

		u.uhp = -1;
		if (how & PLAYER) {
		    killer_format = KILLED_BY;
		    killer = "genocidal confusion";
		} else if (how & ONTHRONE) {
		    /* player selected while on a throne */
		    killer_format = KILLED_BY_AN;
		    killer = "imperious order";
		} else { /* selected player deliberately, not confused */
		    killer_format = KILLED_BY_AN;
		    killer = "scroll of genocide";
		}

	/* Polymorphed characters will die as soon as they're rehumanized. */
	/* KMH -- Unchanging prevents rehumanization */
		if (Upolyd && ptr != youmonst.data) {
			delayed_killer = killer;
			killer = 0;
			pline("You feel dead inside.");
		} else
			done(GENOCIDED);
	    } else if (ptr == youmonst.data) {
		rehumanize();
	    }

	    if (only_on_level) {
		kill_monster_on_level(mndx);
	    } else {
		reset_rndmonst(mndx);
		kill_genocided_monsters();
		update_inventory();	/* in case identified eggs were affected */
	    }
	} else {
	    int cnt = 0;

	    if (!(mons[mndx].geno & G_UNIQ) &&
		    !(mvitals[mndx].mvflags & (G_GENOD | G_EXTINCT)))
		for (i = rn1(3, 4); i > 0; i--) {
		    if (!makemon(ptr, level, u.ux, u.uy, NO_MINVENT))
			break;	/* couldn't make one */
		    ++cnt;
		    if (mvitals[mndx].mvflags & G_EXTINCT)
			break;	/* just made last one */
		}
	    if (cnt)
		pline("Sent in some %s.", makeplural(buf));
	    else
		pline("Nothing happens.");
	}
}

void punish(struct obj *sobj)
{
	struct obj *otmp;

	/* KMH -- Punishment is still okay when you are riding */
	pline("You are being punished for your misbehavior!");
	if (Punished){
		pline("Your iron ball gets heavier.");
		uball->owt += 160 * (1 + sobj->cursed);
		return;
	}
	if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
		pline("A ball and chain appears, then falls away.");
		dropy(mkobj(level, BALL_CLASS, TRUE));
		return;
	}

	/* The Iron Ball of Liberation chains itself back to you. */
	setworn(mkobj(level, CHAIN_CLASS, TRUE), W_CHAIN);
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (otmp->otyp == HEAVY_IRON_BALL &&
		otmp->oartifact == ART_IRON_BALL_OF_LIBERATION)
		break;
	}
	if (otmp) {
	    setworn(otmp, W_BALL);
	    pline("Your %s chains itself to you!", xname(otmp));
	} else {
	    setworn(mkobj(level, BALL_CLASS, TRUE), W_BALL);
	}
	uball->spe = 1;		/* special ball (see save) */

	/*
	 *  Place ball & chain if not swallowed.  If swallowed, the ball &
	 *  chain variables will be set at the next call to placebc().
	 */
	if (!u.uswallow) {
	    placebc();
	    if (Blind) set_bc(1);	/* set up ball and chain variables */
	    newsym(u.ux,u.uy);		/* see ball&chain if can't see self */
	}
}

void unpunish(void)
{	    /* remove the ball and chain */
	struct obj *savechain = uchain;

	obj_extract_self(uchain);
	newsym(uchain->ox,uchain->oy);
	setworn(NULL, W_CHAIN);
	dealloc_obj(savechain);
	uball->spe = 0;
	setworn(NULL, W_BALL);
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to revive
 * one, the disoriented creature becomes a zombie
 */
boolean cant_create(int *mtype, boolean revival)
{

	/* SHOPKEEPERS can be revived now */
	if (*mtype==PM_GUARD || (*mtype==PM_SHOPKEEPER && !revival)
	     || *mtype==PM_ALIGNED_PRIEST || *mtype==PM_ANGEL) {
		*mtype = PM_HUMAN_ZOMBIE;
		return TRUE;
	} else if (*mtype==PM_LONG_WORM_TAIL) {	/* for create_particular() */
		*mtype = PM_LONG_WORM;
		return TRUE;
	}
	return FALSE;
}


/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic; this behavior quirk is useful so don't "fix" it...
 */
boolean create_particular(void)
{
	char buf[BUFSZ], *bufp, monclass = MAXMCLASSES;
	int which, tries, i;
	const struct permonst *whichpm;
	struct monst *mtmp;
	boolean madeany = FALSE;
	boolean maketame, makepeaceful, makehostile;

	tries = 0;
	do {
	    which = urole.malenum;	/* an arbitrary index into mons[] */
	    maketame = makepeaceful = makehostile = FALSE;
	    getlin("Create what kind of monster? [type the name or symbol]",
		   buf);
	    bufp = mungspaces(buf);
	    if (*bufp == '\033') return FALSE;
	    /* allow the initial disposition to be specified */
	    if (!strncmpi(bufp, "tame ", 5)) {
		bufp += 5;
		maketame = TRUE;
	    } else if (!strncmpi(bufp, "peaceful ", 9)) {
		bufp += 9;
		makepeaceful = TRUE;
	    } else if (!strncmpi(bufp, "hostile ", 8)) {
		bufp += 8;
		makehostile = TRUE;
	    }
	    /* decide whether a valid monster was chosen */
	    if (strlen(bufp) == 1) {
		monclass = def_char_to_monclass(*bufp);
		if (monclass != MAXMCLASSES) break;	/* got one */
	    } else {
		which = name_to_mon(bufp);
		if (which >= LOW_PM) break;		/* got one */
	    }
	    /* no good; try again... */
	    pline("I've never heard of such monsters.");
	} while (++tries < 5);

	if (tries == 5) {
	    pline("That's enough tries!");
	} else {
	    cant_create(&which, FALSE);
	    whichpm = &mons[which];
	    for (i = 0; i <= multi; i++) {
		if (monclass != MAXMCLASSES)
		    whichpm = mkclass(&u.uz, monclass, 0);
		if (maketame) {
		    mtmp = makemon(whichpm, level, u.ux, u.uy, MM_EDOG);
		    if (mtmp) {
			initedog(mtmp);
			set_malign(mtmp);
		    }
		} else {
		    mtmp = makemon(whichpm, level, u.ux, u.uy, NO_MM_FLAGS);
		    if ((makepeaceful || makehostile) && mtmp) {
			mtmp->mtame = 0;	/* sanity precaution */
			mtmp->mpeaceful = makepeaceful ? 1 : 0;
			set_malign(mtmp);
		    }
		}
		if (mtmp) madeany = TRUE;
	    }
	}
	return madeany;
}

void drop_boulder_on_player(boolean confused, boolean helmet_protects,
			    boolean by_player, boolean drop_directly_to_floor)
{
	int dmg;
	struct obj *otmp;

	/* hit monster if swallowed */
	if (u.uswallow && !drop_directly_to_floor) {
	    drop_boulder_on_monster(u.ux, u.uy, confused, by_player);
	    return;
	}

	otmp = mksobj(level, confused ? ROCK : BOULDER, FALSE, FALSE);
	if (!otmp) return;
	otmp->quan = confused ? rn1(5,2) : 1;
	otmp->owt = weight(otmp);
	if (!amorphous(youmonst.data) &&
		!Passes_walls &&
		!noncorporeal(youmonst.data) &&
		!unsolid(youmonst.data)) {
	    pline("You are hit by %s!", doname(otmp));
	    dmg = dmgval(NULL, otmp, TRUE, &youmonst) * otmp->quan;
	    if (uarmh && helmet_protects) {
		if (is_metallic(uarmh)) {
		    pline("Fortunately, you are wearing a hard helmet.");
		    if (dmg > 2) dmg = 2;
		} else if (flags.verbose) {
		    pline("Your %s does not protect you.", xname(uarmh));
		}
	    }
	} else
	    dmg = 0;

	/* Must be before the losehp(), for bones files */
	if (!flooreffects(otmp, u.ux, u.uy, "fall")) {
	    place_object(otmp, level, u.ux, u.uy);
	    stackobj(otmp);
	    newsym(u.ux, u.uy);
	}

	if (dmg) losehp(dmg, "scroll of earth", KILLED_BY_AN);
}

int drop_boulder_on_monster(int x, int y, boolean confused, boolean by_player)
{
	struct obj *otmp;
	struct monst *mtmp;

	/* Make the object(s) */
	otmp = mksobj(level, confused ? ROCK : BOULDER, FALSE, FALSE);
	if (!otmp) return 0;  /* Shouldn't happen */
	otmp->quan = confused ? rn1(5,2) : 1;
	otmp->owt = weight(otmp);

	/* Find the monster here (won't be player) */
	mtmp = m_at(level, x, y);
	if (mtmp && !amorphous(mtmp->data) &&
		    !passes_walls(mtmp->data) &&
		    !noncorporeal(mtmp->data) &&
		    !unsolid(mtmp->data)) {
	    const struct obj *helmet = which_armor(mtmp, W_ARMH);
	    int mdmg;

	    if (cansee(mtmp->mx, mtmp->my)) {
		pline("%s is hit by %s!", Monnam(mtmp), doname(otmp));
		if (mtmp->minvis && !canspotmon(level, mtmp))
		    map_invisible(mtmp->mx, mtmp->my);
	    } else if (u.uswallow && mtmp == u.ustuck) {
		if (flags.soundok) {
		    You_hear("something hit %s %s over your %s!",
			     s_suffix(mon_nam(mtmp)),
			     mbodypart(mtmp, STOMACH),
			     body_part(HEAD));
		}
	    }
	    mdmg = dmgval(NULL, otmp, TRUE, mtmp) * otmp->quan;
	    if (helmet) {
		if (is_metallic(helmet)) {
		    if (canspotmon(level, mtmp))
			pline("Fortunately, %s is wearing a hard helmet.",
			      mon_nam(mtmp));
		    else if (flags.soundok)
			You_hear("a clanging sound.");
		    if (mdmg > 2) mdmg = 2;
		} else {
		    if (canspotmon(level, mtmp))
			pline("%s's %s does not protect %s.", Monnam(mtmp),
			      xname(helmet), mhim(level, mtmp));
		}
	    }
	    mtmp->mhp -= mdmg;
	    if (mtmp->mhp <= 0) {
		if (by_player) {
		    xkilled(mtmp, 1);
		} else {
		    pline("%s is killed.", Monnam(mtmp));
		    mondied(mtmp);
		}
	    }
	} else if (u.uswallow && mtmp == u.ustuck) {
	    obfree(otmp, NULL);
	    /* fall through to player */
	    drop_boulder_on_player(confused, TRUE, FALSE, TRUE);
	    return 1;
	}
	/* Drop the rock/boulder to the floor */
	if (!flooreffects(otmp, x, y, "fall")) {
	    place_object(otmp, level, x, y);
	    stackobj(otmp);
	    newsym(x, y);  /* map the rock */
	}

	return 1;
}

/*read.c*/
