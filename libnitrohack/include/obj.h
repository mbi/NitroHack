/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#ifndef OBJ_H
#define OBJ_H

enum obj_where {
	OBJ_FREE,	/* object not attached to anything */
	OBJ_FLOOR,	/* object on floor */
	OBJ_CONTAINED,	/* object in a container */
	OBJ_INVENT,	/* object in the hero's inventory */
	OBJ_MINVENT,	/* object in a monster inventory */
	OBJ_BURIED,	/* object buried */
	OBJ_ONBILL,	/* object on shk bill */
	OBJ_MAGIC_CHEST,/* object in the shared contents of magic chests */
	NOBJ_STATES
};

union vptrs {
	    struct obj *v_nexthere;	/* floor location lists */
	    struct obj *v_ocontainer;	/* point back to container */
	    struct monst *v_ocarry;	/* point back to carrying monst */
};

struct obj {
	struct obj *nobj;
	union vptrs v;
#define nexthere	v.v_nexthere
#define ocontainer	v.v_ocontainer
#define ocarry		v.v_ocarry

	struct obj *cobj;	/* contents list for containers */
	unsigned int o_id;
	struct level *olev;	/* the level it is on */
	xchar ox,oy;
	short otyp;		/* object class number */
	unsigned owt;
	int quan;		/* number of items */

	schar spe;		/* quality of weapon, armor or ring (+ or -)
				   number of charges for wand ( >= -1 )
				   marks your eggs, spinach tins
				   royal coffers for a court ( == 2)
				   tells which fruit a fruit is
				   special for uball and amulet
				   historic and gender for statues */
#define STATUE_HISTORIC 0x01
#define STATUE_MALE     0x02
#define STATUE_FEMALE   0x04
	char	oclass;		/* object class */
	char	invlet;		/* designation in inventory */
	char	oartifact;	/* artifact array index */

	long	oprops;		/* object properties; *band's "ego" items */
	long	oprops_known;	/* known object properties */

	xchar where;		/* where the object thinks it is */
	xchar timed;		/* # of fuses (timers) attached to this obj */

	unsigned cursed:1;
	unsigned blessed:1;
	unsigned unpaid:1;	/* on some bill */
	unsigned no_charge:1;	/* if shk shouldn't charge for this */
	unsigned known:1;	/* exact nature known */
	unsigned dknown:1;	/* color or text known */
	unsigned bknown:1;	/* blessing or curse known */
	unsigned rknown:1;	/* rustproof or not known */

	unsigned oeroded:2;	/* rusted/burnt weapon/armor */
	unsigned oeroded2:2;	/* corroded/rotted weapon/armor */
#define greatest_erosion(otmp) (int)((otmp)->oeroded > (otmp)->oeroded2 ? (otmp)->oeroded : (otmp)->oeroded2)
#define MAX_ERODE 3
#define orotten oeroded		/* rotten food */
#define odiluted oeroded	/* diluted potions */
#define norevive oeroded2
	unsigned oerodeproof:1; /* erodeproof weapon/armor */
	unsigned olocked:1;	/* object is locked */
#define oprize olocked		/* special flag for sokoban prize */
	unsigned obroken:1;	/* lock has been broken */
	unsigned otrapped:1;	/* container is trapped */
				/* or accidental tripped rolling boulder trap */
#define opoisoned otrapped	/* object (weapon) is coated with poison */

	unsigned recharged:3;	/* number of times it's been recharged */
	unsigned lamplit:1;	/* a light-source -- can be lit */
#ifdef INVISIBLE_OBJECTS
	unsigned oinvis:1;	/* invisible */
#endif
	unsigned greased:1;	/* covered with grease */
	unsigned oattached:2;	/* obj struct has special attachment */
#define OATTACHED_NOTHING 0
#define OATTACHED_MONST   1	/* monst struct in oextra */
#define OATTACHED_M_ID    2	/* monst id in oextra */
#define OATTACHED_UNUSED3 3

	unsigned in_use:1;	/* for magic items before useup items */
	unsigned was_dropped:1;	/* dropped deliberately by the hero */
	unsigned was_thrown:1;	/* thrown by the hero since last picked up */
	unsigned bypass:1;	/* mark this as an object to be skipped by bhito() */
	unsigned odrained:1;	/* drained corpse */
	/* 3 free bits */

	int	corpsenm;	/* type of corpse is mons[corpsenm] */
#define leashmon  corpsenm	/* gets m_id of attached pet */
#define spestudied corpsenm	/* # of times a spellbook has been studied */
#define fromsink  corpsenm	/* a potion from a sink */
	unsigned oeaten;	/* nutrition left in food, if partly eaten */

	uchar onamelth;		/* length of name (following oxlth) */
	short oxlth;		/* length of following data */
	int age;		/* creation date */
	int owornmask;
	int oextra[1];		/* used for name of ordinary objects - length
				   is flexible; amount for tmp gold objects */
};

#define newobj(xl)	malloc((unsigned)(xl) + sizeof(struct obj))
#define ONAME(otmp)	(((char *)(otmp)->oextra) + (otmp)->oxlth)

/* Weapons and weapon-tools */
/* KMH -- now based on skill categories.  Formerly:
 *	#define is_sword(otmp)	(otmp->oclass == WEAPON_CLASS && \
 *			 objects[otmp->otyp].oc_wepcat == WEP_SWORD)
 *	#define is_blade(otmp)	(otmp->oclass == WEAPON_CLASS && \
 *			 (objects[otmp->otyp].oc_wepcat == WEP_BLADE || \
 *			  objects[otmp->otyp].oc_wepcat == WEP_SWORD))
 *	#define is_weptool(o)	((o)->oclass == TOOL_CLASS && \
 *			 objects[(o)->otyp].oc_weptool)
 *	#define is_multigen(otyp) (otyp <= SHURIKEN)
 *	#define is_poisonable(otyp) (otyp <= BEC_DE_CORBIN)
 */
#define is_blade(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= P_DAGGER && \
			 objects[otmp->otyp].oc_skill <= P_SABER)
#define is_axe(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_skill == P_AXE)
#define is_pick(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_skill == P_PICK_AXE)
#define is_sword(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= P_SHORT_SWORD && \
			 objects[otmp->otyp].oc_skill <= P_SABER)
#define is_pole(otmp)	((otmp->oclass == WEAPON_CLASS || \
			otmp->oclass == TOOL_CLASS) && \
			 (objects[otmp->otyp].oc_skill == P_POLEARMS || \
			 objects[otmp->otyp].oc_skill == P_LANCE))
#define is_spear(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= P_SPEAR && \
			 objects[otmp->otyp].oc_skill <= P_JAVELIN)
#define is_launcher(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= P_BOW && \
			 objects[otmp->otyp].oc_skill <= P_CROSSBOW)
#define is_ammo(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == GEM_CLASS) && \
			 objects[otmp->otyp].oc_skill >= -P_CROSSBOW && \
			 objects[otmp->otyp].oc_skill <= -P_BOW)
#define ammo_and_launcher(otmp,ltmp) \
			 (is_ammo(otmp) && (ltmp) && \
			 objects[(otmp)->otyp].oc_skill == -objects[(ltmp)->otyp].oc_skill)
#define is_missile(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_skill >= -P_BOOMERANG && \
			 objects[otmp->otyp].oc_skill <= -P_DART)
#define is_weptool(o)	((o)->oclass == TOOL_CLASS && \
			 objects[(o)->otyp].oc_skill != P_NONE)
#define bimanual(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_bimanual)
#define is_multigen(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= -P_SHURIKEN && \
			 objects[otmp->otyp].oc_skill <= -P_BOW)
#define is_poisonable(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= -P_SHURIKEN && \
			 objects[otmp->otyp].oc_skill <= -P_BOW)
#define uslinging()	(uwep && objects[uwep->otyp].oc_skill == P_SLING)
/* multiply inflicted damage when thrown/fired and disable multishot (if any) */
#define is_heavyshot(otmp,ltmp) \
			(otmp->oclass == WEAPON_CLASS && \
			 (!is_ammo(otmp) || ammo_and_launcher(otmp, ltmp)) && \
			 (is_spear(otmp) || \
			  objects[otmp->otyp].oc_skill == -P_CROSSBOW || \
			  objects[otmp->otyp].oc_skill == -P_SHURIKEN || \
			  objects[otmp->otyp].oc_skill == -P_BOOMERANG))

/* Armor */
#define is_shield(otmp) (otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_SHIELD)
#define is_helmet(otmp) (otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_HELM)
#define is_boots(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_BOOTS)
#define is_gloves(otmp) (otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_GLOVES)
#define is_cloak(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_CLOAK)
#define is_shirt(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_SHIRT)
#define is_suit(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_SUIT)

/* http://nethackwiki.com/wiki/Unofficial_conduct#Racial */
#define is_elven_armor(otmp)	((otmp)->otyp == ELVEN_LEATHER_HELM\
				|| (otmp)->otyp == ELVEN_MITHRIL_COAT\
				|| (otmp)->otyp == ELVEN_CLOAK\
				|| (otmp)->otyp == ELVEN_SHIELD\
				|| (otmp)->otyp == ELVEN_BOOTS)
#define is_orcish_armor(otmp)	((otmp)->otyp == ORCISH_HELM\
				|| (otmp)->otyp == ORCISH_CHAIN_MAIL\
				|| (otmp)->otyp == ORCISH_RING_MAIL\
				|| (otmp)->otyp == ORCISH_CLOAK\
				|| (otmp)->otyp == URUK_HAI_SHIELD\
				|| (otmp)->otyp == ORCISH_SHIELD)
#define is_dwarvish_armor(otmp)	((otmp)->otyp == DWARVISH_IRON_HELM\
				|| (otmp)->otyp == DWARVISH_MITHRIL_COAT\
				|| (otmp)->otyp == DWARVISH_CLOAK\
				|| (otmp)->otyp == DWARVISH_ROUNDSHIELD)
#define is_gnomish_armor(otmp)	(FALSE)

				
/* Eggs and other food */
#define MAX_EGG_HATCH_TIME 200	/* longest an egg can remain unhatched */
#define stale_egg(egg)	((moves - (egg)->age) > (2*MAX_EGG_HATCH_TIME))
#define ofood(o) ((o)->otyp == CORPSE || (o)->otyp == EGG || (o)->otyp == TIN)
#define polyfodder(obj) (ofood(obj) && \
			 pm_to_cham((obj)->corpsenm) != CHAM_ORDINARY)
#define mlevelgain(obj) (ofood(obj) && (obj)->corpsenm == PM_WRAITH)
#define mhealup(obj)	(ofood(obj) && (obj)->corpsenm == PM_NURSE)
#define drainlevel(corpse) (mons[(corpse)->corpsenm].cnutrit * 4 / 5)

/* Containers */
#define carried(o)	((o)->where == OBJ_INVENT)
#define mcarried(o)	((o)->where == OBJ_MINVENT)
#define Has_contents(o) (/* (Is_container(o) || (o)->otyp == STATUE) && */ \
			 (o)->cobj != NULL)
#define Is_container(o) ((o)->otyp >= LARGE_BOX && (o)->otyp <= BAG_OF_TRICKS)
#define Is_box(otmp)	(otmp->otyp == LARGE_BOX || otmp->otyp == CHEST || \
			 otmp->otyp == IRON_SAFE)
#define Is_mbag(otmp)	(otmp->otyp == BAG_OF_HOLDING || \
			 otmp->otyp == BAG_OF_TRICKS)

#define Is_prize(otmp)	((otmp)->oprize && !Is_box(otmp))

/* dragon gear */
#define Is_dragon_scales(idx)	((idx) >= GRAY_DRAGON_SCALES && \
				 (idx) <= CHROMATIC_DRAGON_SCALES)
#define Is_dragon_mail(idx)	((idx) >= GRAY_DRAGON_SCALE_MAIL && \
				 (idx) <= CHROMATIC_DRAGON_SCALE_MAIL)
#define Is_dragon_armor(idx)	(Is_dragon_scales(idx) || Is_dragon_mail(idx))
#define Is_gold_dragon_armor(idx) (Is_dragon_armor(idx) && \
				   OBJ_DESCR(objects[idx]) && \
				   !strncmp(OBJ_DESCR(objects[idx]), "gold ", 5))
#define Dragon_scales_to_mail(idx) ((idx) - GRAY_DRAGON_SCALES + GRAY_DRAGON_SCALE_MAIL)
#define Dragon_mail_to_scales(idx) ((idx) - GRAY_DRAGON_SCALE_MAIL + GRAY_DRAGON_SCALES)
#define Dragon_scales_to_pm(obj) &mons[PM_GRAY_DRAGON + (obj)->otyp \
				       - GRAY_DRAGON_SCALES]
#define Dragon_mail_to_pm(obj)	&mons[PM_GRAY_DRAGON + (obj)->otyp \
				      - GRAY_DRAGON_SCALE_MAIL]
#define Dragon_to_scales(pm)	(GRAY_DRAGON_SCALES + ((pm) - &mons[PM_GRAY_DRAGON]))

/* http://nethackwiki.com/wiki/Unofficial_conduct#Racial */
/* Elven gear */
#define is_elven_weapon(otmp)	((otmp)->otyp == ELVEN_ARROW\
				|| (otmp)->otyp == ELVEN_SPEAR\
				|| (otmp)->otyp == ELVEN_DAGGER\
				|| (otmp)->otyp == ELVEN_SHORT_SWORD\
				|| (otmp)->otyp == ELVEN_BROADSWORD\
				|| (otmp)->otyp == ELVEN_BOW)
#define is_elven_obj(otmp)	(is_elven_armor(otmp) || is_elven_weapon(otmp))

/* Orcish gear */
#define is_orcish_weapon(otmp)	((otmp)->otyp == ORCISH_ARROW\
				|| (otmp)->otyp == ORCISH_SPEAR\
				|| (otmp)->otyp == ORCISH_DAGGER\
				|| (otmp)->otyp == ORCISH_SHORT_SWORD\
				|| (otmp)->otyp == ORCISH_BOW)
#define is_orcish_obj(otmp)	(is_orcish_armor(otmp) || is_orcish_weapon(otmp))

/* Dwarvish gear */
#define is_dwarvish_weapon(otmp) ((otmp)->otyp == DWARVISH_SPEAR\
				|| (otmp)->otyp == DWARVISH_SHORT_SWORD\
				|| (otmp)->otyp == DWARVISH_MATTOCK)
#define is_dwarvish_obj(otmp)	(is_dwarvish_armor(otmp) || is_dwarvish_weapon(otmp))

/* Gnomish gear */
#define is_gnomish_weapon(otmp)	(FALSE)
#define is_gnomish_obj(otmp)	(is_gnomish_armor(otmp) || is_gnomish_weapon(otmp))

/* Light sources */
#define Is_candle(otmp) (otmp->otyp == TALLOW_CANDLE || \
			 otmp->otyp == WAX_CANDLE)
#define MAX_OIL_IN_FLASK 400	/* maximum amount of oil in a potion of oil */

/* MAGIC_LAMP intentionally excluded below */
/* age field of this is relative age rather than absolute */
#define age_is_relative(otmp)	((otmp)->otyp == BRASS_LANTERN\
				|| (otmp)->otyp == OIL_LAMP\
				|| (otmp)->otyp == CANDELABRUM_OF_INVOCATION\
				|| (otmp)->otyp == TALLOW_CANDLE\
				|| (otmp)->otyp == WAX_CANDLE\
				|| (otmp)->otyp == POT_OIL)
/* object can be ignited */
#define ignitable(otmp)	((otmp)->otyp == BRASS_LANTERN\
				|| (otmp)->otyp == OIL_LAMP\
				|| (otmp)->otyp == CANDELABRUM_OF_INVOCATION\
				|| (otmp)->otyp == TALLOW_CANDLE\
				|| (otmp)->otyp == WAX_CANDLE\
				|| (otmp)->otyp == POT_OIL)

/* special stones */
#define is_graystone(obj)	((obj)->otyp == LUCKSTONE || \
				 (obj)->otyp == LOADSTONE || \
				 (obj)->otyp == FLINT     || \
				 (obj)->otyp == TOUCHSTONE)

/* misc */
#define is_flimsy(otmp)		(objects[(otmp)->otyp].oc_material <= LEATHER || \
				 (otmp)->otyp == RUBBER_HOSE)

#define oresist_disintegration(obj) \
				(objects[obj->otyp].oc_oprop == DISINT_RES || \
				 obj_resists(obj, 5, 50) || is_quest_artifact(obj) )
#define weight_dmg(i)		{  \
				    i = (i <= 100) ? 1 : i / 100; \
				    i = rnd(i); \
				    if (i > 6) i = 6; \
				}

/* helpers, simple enough to be macros */
#define is_plural(o)	((o)->quan > 1 || \
			 (o)->oartifact == ART_EYES_OF_THE_OVERWORLD)

/* Flags for get_obj_location(). */
#define CONTAINED_TOO	0x1
#define BURIED_TOO	0x2

/* Object properties. */
#define ITEM_FIRE		0x00000001L /* fire damage or resistance */
#define ITEM_FROST		0x00000002L /* frost damage or resistance */
#define ITEM_DRLI		0x00000004L /* drains life or resists it */
#define ITEM_VORPAL		0x00000008L /* capable of beheading or bisecting */
#define ITEM_REFLECTION		0x00000010L /* reflects */
#define ITEM_SPEED		0x00000020L /* grants speed */
#define ITEM_OILSKIN		0x00000040L /* permanently greased */
#define ITEM_POWER		0x00000080L /* grants a strength bonus */
#define ITEM_DEXTERITY		0x00000100L /* grants a dexterity bonus */
#define ITEM_BRILLIANCE		0x00000200L /* grants an int/wiz bonus */
#define ITEM_ESP		0x00000400L /* grants extrinsic telepathy  */
#define ITEM_DISPLACEMENT	0x00000800L /* grants displacement */
#define ITEM_SEARCHING		0x00001000L /* grants searching  */
#define ITEM_WARNING		0x00002000L /* grants warning  */
#define ITEM_STEALTH		0x00004000L /* grants stealth  */
#define ITEM_FUMBLING		0x00008000L /* fumbling */
#define ITEM_CLAIRVOYANCE	0x00010000L /* clairvoyance */
#define ITEM_DETONATIONS	0x00020000L /* projectile weapon goes boom  */
#define ITEM_HUNGER		0x00040000L /* consumes extra hunger */
#define ITEM_AGGRAVATE		0x00080000L /* aggravates monsters */

#define ITEM_MAGICAL		0x80000000L /* known to have magical properties */

#define ITEM_PROP_MASK		0x000FFFFFL /* all current properties */
#define MAX_ITEM_PROPS		20

#endif /* OBJ_H */
