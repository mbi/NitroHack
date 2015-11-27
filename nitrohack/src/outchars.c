/* Copyright (c) Daniel Thaler, 2011 */
/* DynaHack may be freely redistributed.  See license for details. */

/* NOTE: This file is utf-8 encoded; saving with a non utf-8 aware editor WILL
 * damage some symbols */

#include "nhcurses.h"
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>


#define array_size(x) (sizeof(x)/sizeof(x[0]))


static int level_display_mode;
static int altar_id,
	   vwall_id, trwall_id,
	   room_id, darkroom_id,
	   ndoor_id, vcdoor_id, hcdoor_id,
	   upstair_id, upladder_id, upsstair_id,
	   dnstair_id, dnladder_id, dnsstair_id,
	   mportal_id, vibsquare_id;
static int corpse_id;
struct curses_drawing_info *default_drawing, *cur_drawing;
static struct curses_drawing_info *unicode_drawing, *rogue_drawing;


static struct curses_symdef rogue_graphics_ovr[] = {
    {"vodoor",	-1,	{0x002B, 0},	'+'},
    {"hodoor",	-1,	{0x002B, 0},	'+'},
    {"ndoor",	-1,	{0x002B, 0},	'+'},
    {"upstair",	-1,	{0x0025, 0},	'%'},
    {"dnstair",	-1,	{0x0025, 0},	'%'},
    {"upsstair",-1,	{0x0025, 0},	'%'},
    {"dnsstair",-1,	{0x0025, 0},	'%'},
    
    {"gold piece",-1,	{0x002A, 0},	'*'},
    
    {"corpse",	-1,	{0x003A, 0},	':'}, /* the 2 most common food items... */
    {"food ration",-1,	{0x003A, 0},	':'}
};


static struct curses_symdef unicode_graphics_ovr[] = {
    /* bg */
    {"vwall",	-1,	{0x2502, 0},	0},	/* │ vertical rule */
    {"hwall",	-1,	{0x2500, 0},	0},	/* ─ horizontal rule */
    {"tlcorn",	-1,	{0x250C, 0},	0},	/* ┌ top left corner */
    {"trcorn",	-1,	{0x2510, 0},	0},	/* ┐ top right corner */
    {"blcorn",	-1,	{0x2514, 0},	0},	/* └ bottom left */
    {"brcorn",	-1,	{0x2518, 0},	0},	/* ┘ bottom right */
    {"crwall",	-1,	{0x253C, 0},	0},	/* ┼ cross */
    {"tuwall",	-1,	{0x2534, 0},	0},	/* ┴ up */
    {"tdwall",	-1,	{0x252C, 0},	0},	/* ┬ down */
    {"tlwall",	-1,	{0x2524, 0},	0},	/* ┤ left */
    {"trwall",	-1,	{0x251C, 0},	0},	/* ├ right */
    {"pool",	-1,	{0x2248, 0},	0},	/* ≈ almost equal to */
    {"water",	-1,	{0x2248, 0},	0},	/* ≈ almost equal to */
    {"swamp",	-1,	{0x2248, 0},	0},	/* ≈ almost equal to */
    {"lava",	-1,	{0x2248, 0},	0},	/* ≈ almost equal to */
    {"ndoor",	-1,	{0x00B7, 0},	0},	/* · centered dot */
    {"vodoor",	-1,	{0x25A0, 0},	0},	/* ■ black square */
    {"hodoor",	-1,	{0x25A0, 0},	0},	/* ■ black square */
    {"bars",	-1,	{0x2261, 0},	0},	/* ≡ equivalence symbol */
    {"tree",	-1,	{0x00B1, 0},	0},	/* ± plus-minus sign */
    {"deadtree",-1,	{0x00B1, 0},	0},	/* ± plus-minus sign */
    {"fountain",-1,	{0x00B6, 0},	0},	/* ¶ pilcrow sign */
    {"room",	-1,	{0x00B7, 0},	0},	/* · centered dot */
    {"darkroom",-1,	{0x00B7, 0},	0},	/* · centered dot */
    {"upladder",-1,	{0x2264, 0},	0},	/* ≤ less-than-or-equals */
    {"dnladder",-1,	{0x2265, 0},	0},	/* ≥ greater-than-or-equals */
    {"upsstair",-1,	{0x00AB, 0},	0},	/* « left-pointing double angle quotation mark */
    {"dnsstair",-1,	{0x00BB, 0},	0},	/* » right-pointing double angle quotation mark */
    {"altar",	-1,	{0x03A9, 0},	0},	/* Ω GREEK CAPITAL LETTER OMEGA */
    {"magic_chest",-1,	{0x2302, 0},	0},	/* ⌂ house */
    {"ice",	-1,	{0x00B7, 0},	0},	/* · centered dot */

    /* zap */
    {"zap_v",	-1,	{0x2502, 0},	0},	/* │ vertical rule */
    {"zap_h",	-1,	{0x2500, 0},	0},	/* ─ horizontal rule */

    /* swallow */
    {"swallow_top_c",-1,{0x2500, 0},	0},	/* ─ horizontal rule */
    {"swallow_mid_l",-1,{0x2502, 0},	0},	/* │ vertical rule */
    {"swallow_mid_r",-1,{0x2502, 0},	0},	/* │ vertical rule */
    {"swallow_bot_c",-1,{0x2500, 0},	0},	/* ─ horizontal rule */

    /* explosion */
    {"exp_top_c", -1,	{0x2500, 0},	0},	/* ─ horizontal rule */
    {"exp_mid_l", -1,	{0x2502, 0},	0},	/* │ vertical rule */
    {"exp_mid_r", -1,	{0x2502, 0},	0},	/* │ vertical rule */
    {"exp_bot_c", -1,	{0x2500, 0},	0},	/* ─ horizontal rule */

    /* traps */
    {"web",	-1,	{0x256C, 0},	0},	/* ╬ double cross */
};


static nh_bool apply_override_list(struct curses_symdef *list, int len,
				   const struct curses_symdef *ovr, nh_bool cust)
{
    int i;
    for (i = 0; i < len; i++)
	if (!strcmp(list[i].symname, ovr->symname)) {
	    if (ovr->unichar[0])
		memcpy(list[i].unichar, ovr->unichar, sizeof(wchar_t) * CCHARW_MAX);
	    if (ovr->ch)
		list[i].ch = ovr->ch;
	    if (ovr->color != -1)
		list[i].color = ovr->color;
	    list[i].custom = cust;
	    return TRUE;
	}
    return FALSE;
}


static void apply_override(struct curses_drawing_info *di,
			   const struct curses_symdef *ovr, int olen, nh_bool cust)
{
    int i;
    nh_bool ok;
    
    for (i = 0; i < olen; i++) {
	ok = FALSE;
	/* the override will effect exactly one of the symbol lists */
	ok |= apply_override_list(di->bgelements, di->num_bgelements, &ovr[i], cust);
	ok |= apply_override_list(di->traps, di->num_traps, &ovr[i], cust);
	ok |= apply_override_list(di->objects, di->num_objects, &ovr[i], cust);
	ok |= apply_override_list(di->monsters, di->num_monsters, &ovr[i], cust);
	ok |= apply_override_list(di->warnings, di->num_warnings, &ovr[i], cust);
	ok |= apply_override_list(di->invis, 1, &ovr[i], cust);
	ok |= apply_override_list(di->effects, di->num_effects, &ovr[i], cust);
	ok |= apply_override_list(di->expltypes, di->num_expltypes, &ovr[i], cust);
	ok |= apply_override_list(di->explsyms, NUMEXPCHARS, &ovr[i], cust);
	ok |= apply_override_list(di->zaptypes, di->num_zaptypes, &ovr[i], cust);
	ok |= apply_override_list(di->zapsyms, NUMZAPCHARS, &ovr[i], cust);
	ok |= apply_override_list(di->breathsyms, NUMBREATHCHARS, &ovr[i], cust);
	ok |= apply_override_list(di->swallowsyms, NUMSWALLOWCHARS, &ovr[i], cust);
	
	if (!ok)
	    fprintf(stdout, "sym override %s could not be applied\n", ovr[i].symname);
    }
}


static struct curses_symdef *load_nh_symarray(const struct nh_symdef *src, int len)
{
    int i;
    struct curses_symdef *copy = malloc(len * sizeof(struct curses_symdef));
    memset(copy, 0, len * sizeof(struct curses_symdef));
    
    for (i = 0; i < len; i++) {
	copy[i].symname = strdup(src[i].symname);
	copy[i].ch = src[i].ch;
	copy[i].color = src[i].color;
	
	/* this works because ASCII 0x?? (for ?? < 128) == Unicode U+00?? */
	copy[i].unichar[0] = (wchar_t)src[i].ch; 
    }
    
    return copy;
}


static struct curses_drawing_info *load_nh_drawing_info(const struct nh_drawing_info *orig)
{
    struct curses_drawing_info *copy = malloc(sizeof(struct curses_drawing_info));
    
    copy->num_bgelements = orig->num_bgelements;
    copy->num_traps = orig->num_traps;
    copy->num_objects = orig->num_objects;
    copy->num_monsters = orig->num_monsters;
    copy->num_warnings = orig->num_warnings;
    copy->num_expltypes = orig->num_expltypes;
    copy->num_zaptypes = orig->num_zaptypes;
    copy->num_effects = orig->num_effects;
    copy->bg_feature_offset = orig->bg_feature_offset;
    
    copy->bgelements = load_nh_symarray(orig->bgelements, orig->num_bgelements);
    copy->traps = load_nh_symarray(orig->traps, orig->num_traps);
    copy->objects = load_nh_symarray(orig->objects, orig->num_objects);
    copy->monsters = load_nh_symarray(orig->monsters, orig->num_monsters);
    copy->warnings = load_nh_symarray(orig->warnings, orig->num_warnings);
    copy->invis = load_nh_symarray(orig->invis, 1);
    copy->effects = load_nh_symarray(orig->effects, orig->num_effects);
    copy->expltypes = load_nh_symarray(orig->expltypes, orig->num_expltypes);
    copy->explsyms = load_nh_symarray(orig->explsyms, NUMEXPCHARS);
    copy->zaptypes = load_nh_symarray(orig->zaptypes, orig->num_zaptypes);
    copy->zapsyms = load_nh_symarray(orig->zapsyms, NUMZAPCHARS);
    copy->breathsyms = load_nh_symarray(orig->breathsyms, NUMBREATHCHARS);
    copy->swallowsyms = load_nh_symarray(orig->swallowsyms, NUMSWALLOWCHARS);
    
    return copy;
}


static void read_sym_line(char *line)
{
    struct curses_symdef ovr;
    char symname[64];
    char *bp;
    
    if (!strlen(line) || line[0] != '!' || line[1] != '"')
	return;
    
    line++; /* skip the ! */
    memset(&ovr, 0, sizeof(struct curses_symdef));
    
    /* line format: "symbol name" color unicode [combining marks] */
    bp = &line[1];
    while (*bp && *bp != '"') bp++; /* find the end of the symname */
    strncpy(symname, &line[1], bp - &line[1]);
    symname[bp - &line[1]] = '\0';
    ovr.symname = symname;
    bp++; /* go past the " at the end of the symname */
    
    while (*bp && isspace((unsigned char)*bp)) bp++; /* find the start of the next value */
    sscanf(bp, "%d", &ovr.color);
    
    while (*bp && !isspace((unsigned char)*bp)) bp++; /* go past the previous value */
#if defined(__CYGWIN__) || defined(__MINGW32__)
    /* can't find a format string that makes Windows ports of GCC
     * and everything else happy at the same time :( */
    sscanf(bp, "%hx", &ovr.unichar[0]);
#else
    sscanf(bp, "%x", &ovr.unichar[0]);
#endif
    
    apply_override(unicode_drawing, &ovr, 1,  TRUE);
}


static void read_unisym_config(void)
{
    fnchar filename[BUFSZ];
    char *data, *line;
    int fd, size;
    
    filename[0] = '\0';
    if (!get_gamedir(CONFIG_DIR, filename))
	return;
    fnncat(filename, FN("unicode.conf"), BUFSZ);
    
    fd = sys_open(filename, O_RDONLY, 0);
    if (fd == -1)
	return;
    
    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    data = malloc(size + 1);
    read(fd, data, size);
    data[size] = '\0';
    close(fd);
    
    line = strtok(data, "\r\n");
    while (line) {
	read_sym_line(line);
	
	line = strtok(NULL, "\r\n");
    }
    
    free(data);
}


static void write_symlist(int fd, const struct curses_symdef *list, int len)
{
    char buf[BUFSZ];
    int i;
    
    for (i = 0; i < len; i++) {
	sprintf(buf, "%c\"%s\"\t%d\t%04x\n", list[i].custom ? '!' : '#',
		list[i].symname, list[i].color, list[i].unichar[0]);
	write(fd, buf, strlen(buf));
    }
}

static const char uniconf_header[] =
"# Unicode symbol configuration for DynaHack\n"
"# Lines that begin with '#' are commented out.\n"
"# Change the '#' to an '!' to activate a line.\n";

static void write_unisym_config(void)
{
    fnchar filename[BUFSZ];
    int fd;
    
    filename[0] = '\0';
    if (!get_gamedir(CONFIG_DIR, filename))
	return;
    fnncat(filename, FN("unicode.conf"), BUFSZ);
    
    fd = sys_open(filename, O_TRUNC | O_CREAT | O_RDWR, 0644);
    if (fd == -1)
	return;
#ifdef UNIX
    /* bypass umask and set 0644 for real */
    fchmod(fd, 0644);
#endif

    write(fd, uniconf_header, strlen(uniconf_header));
    write_symlist(fd, unicode_drawing->bgelements, unicode_drawing->num_bgelements);
    write_symlist(fd, unicode_drawing->traps, unicode_drawing->num_traps);
    write_symlist(fd, unicode_drawing->objects, unicode_drawing->num_objects);
    write_symlist(fd, unicode_drawing->monsters, unicode_drawing->num_monsters);
    write_symlist(fd, unicode_drawing->warnings, unicode_drawing->num_warnings);
    write_symlist(fd, unicode_drawing->invis, 1);
    write_symlist(fd, unicode_drawing->effects, unicode_drawing->num_effects);
    write_symlist(fd, unicode_drawing->expltypes, unicode_drawing->num_expltypes);
    write_symlist(fd, unicode_drawing->explsyms, NUMEXPCHARS);
    write_symlist(fd, unicode_drawing->zaptypes, unicode_drawing->num_zaptypes);
    write_symlist(fd, unicode_drawing->zapsyms, NUMZAPCHARS);
    write_symlist(fd, unicode_drawing->breathsyms, NUMBREATHCHARS);
    write_symlist(fd, unicode_drawing->swallowsyms, NUMSWALLOWCHARS);
    
    close(fd);
}


void init_displaychars(void)
{
    int i;
    struct nh_drawing_info *dinfo = nh_get_drawing_info();
    
    default_drawing = load_nh_drawing_info(dinfo);
    unicode_drawing = load_nh_drawing_info(dinfo);
    rogue_drawing = load_nh_drawing_info(dinfo);
    
    apply_override(unicode_drawing, unicode_graphics_ovr,
		   array_size(unicode_graphics_ovr), FALSE);
    apply_override(rogue_drawing, rogue_graphics_ovr, array_size(rogue_graphics_ovr), FALSE);
    
    read_unisym_config();
    
    cur_drawing = default_drawing;

    /* find bg elements that could use special treatment */
    for (i = 0; i < cur_drawing->num_bgelements; i++) {
	if (!strcmp("altar", cur_drawing->bgelements[i].symname))
	    altar_id = i;
	else if (!strcmp("vwall", cur_drawing->bgelements[i].symname))
	    vwall_id = i;
	else if (!strcmp("trwall", cur_drawing->bgelements[i].symname))
	    trwall_id = i;
	else if (!strcmp("room", cur_drawing->bgelements[i].symname))
	    room_id = i;
	else if (!strcmp("darkroom", cur_drawing->bgelements[i].symname))
	    darkroom_id = i;
	else if (!strcmp("ndoor", cur_drawing->bgelements[i].symname))
	    ndoor_id = i;
	else if (!strcmp("vcdoor", cur_drawing->bgelements[i].symname))
	    vcdoor_id = i;
	else if (!strcmp("hcdoor", cur_drawing->bgelements[i].symname))
	    hcdoor_id = i;
	else if (!strcmp("upstair", cur_drawing->bgelements[i].symname))
	    upstair_id = i;
	else if (!strcmp("upladder", cur_drawing->bgelements[i].symname))
	    upladder_id = i;
	else if (!strcmp("upsstair", cur_drawing->bgelements[i].symname))
	    upsstair_id = i;
	else if (!strcmp("dnstair", cur_drawing->bgelements[i].symname))
	    dnstair_id = i;
	else if (!strcmp("dnladder", cur_drawing->bgelements[i].symname))
	    dnladder_id = i;
	else if (!strcmp("dnsstair", cur_drawing->bgelements[i].symname))
	    dnsstair_id = i;
    }

    /* find traps that could use special treatment */
    for (i = 0; i < cur_drawing->num_traps; i++) {
	if (!strcmp("magic portal", cur_drawing->traps[i].symname))
	    mportal_id = i;
	else if (!strcmp("vibrating square", cur_drawing->traps[i].symname))
	    vibsquare_id = i;
    }

    /* find objects that could use special treatment */
    for (i = 0; i < cur_drawing->num_objects; i++) {
	if (!strcmp("corpse", cur_drawing->objects[i].symname))
	    corpse_id = i;
    }

    /* options are parsed before display is initialized, so redo switch */
    switch_graphics(settings.graphics);
}


static void free_symarray(struct curses_symdef *array, int len)
{
    int i;
    for (i = 0; i < len; i++)
	free((char*)array[i].symname);
    
    free(array);
}


static void free_drawing_info(struct curses_drawing_info *di)
{
    free_symarray(di->bgelements, di->num_bgelements);
    free_symarray(di->traps, di->num_traps);
    free_symarray(di->objects, di->num_objects);
    free_symarray(di->monsters, di->num_monsters);
    free_symarray(di->warnings, di->num_warnings);
    free_symarray(di->invis, 1);
    free_symarray(di->effects, di->num_effects);
    free_symarray(di->expltypes, di->num_expltypes);
    free_symarray(di->explsyms, NUMEXPCHARS);
    free_symarray(di->zaptypes, di->num_zaptypes);
    free_symarray(di->zapsyms, NUMZAPCHARS);
    free_symarray(di->breathsyms, NUMBREATHCHARS);
    free_symarray(di->swallowsyms, NUMSWALLOWCHARS);
    
    free(di);
}


void free_displaychars(void)
{
    write_unisym_config();
    
    free_drawing_info(default_drawing);
    free_drawing_info(unicode_drawing);
    free_drawing_info(rogue_drawing);
    
    default_drawing = rogue_drawing = NULL;
}


static void darken_symdef(struct curses_symdef *sym)
{
    /*
     * Avoid black-on-black with darkgray trick on blank spaces (see darken()),
     * and a blinking dark blue cursor on black otherwise.
     */
    if ((ui_flags.unicode && (sym->unichar[0] == ' ' ||
			      (sym->unichar[0] == '\0' && sym->ch == ' '))) ||
	(!ui_flags.unicode && sym->ch == ' '))
	return;
    sym->color = darken(sym->color);
}


static void valley_symdef(struct curses_symdef *sym)
{
    if (!settings.mapcolors)
	return;

    if (level_display_mode == LDM_VALLEY) {
	if (sym->color != CLR_BLACK)
	    sym->color = (sym->color < NO_COLOR) ? CLR_GRAY : CLR_WHITE;
    }
}


static void recolor_bg(const struct nh_dbuf_entry *dbe, struct curses_symdef *sym)
{
    int id;

    if (!settings.mapcolors)
	return;

    id = dbe->bg;

    switch (level_display_mode) {
    case LDM_MINES:
	/*
	 * NH_DF_BGHINT_MINEROOM is excluded from coloring in the Mines;
	 * in fact, all rooms are.
	 */
	if (id >= vwall_id && id <= trwall_id && !(dbe->dgnflags & NH_DF_BGHINT_MASK))
	    sym->color = CLR_BROWN;
	break;
    case LDM_SOKOBAN:
	if (id >= vwall_id && id <= trwall_id)
	    sym->color = CLR_BRIGHT_BLUE;
	break;
    case LDM_JUIBLEX:
	if (id == room_id)
	    sym->color = CLR_BRIGHT_GREEN;
	else if (id == darkroom_id)
	    sym->color = CLR_GREEN;
	break;
    case LDM_HELL:
	if (id >= vwall_id && id <= trwall_id)
	    sym->color = CLR_ORANGE;
	break;
    }

    switch (dbe->dgnflags & NH_DF_BGHINT_MASK) {
    case NH_DF_BGHINT_BEEHIVE:
	if ((id >= vwall_id && id <= trwall_id) || id == room_id || id == ndoor_id)
	    sym->color = CLR_YELLOW;
	else if (id == darkroom_id)
	    sym->color = CLR_BROWN;
	break;
    case NH_DF_BGHINT_GARDEN:
	if ((id >= vwall_id && id <= trwall_id) || id == room_id || id == ndoor_id)
	    sym->color = CLR_BRIGHT_GREEN;
	else if (id == darkroom_id)
	    sym->color = CLR_GREEN;
	break;
    case NH_DF_BGHINT_WIZTOWER:
	if (id >= vwall_id && id <= trwall_id)
	    sym->color = CLR_BRIGHT_MAGENTA;
	break;
    }

    if (id == vcdoor_id || id == hcdoor_id) {
	if (dbe->dgnflags & NH_DF_DOORTRAP_TRAPPED)
	    sym->color = CLR_CYAN;
	else if (dbe->dgnflags & NH_DF_DOORLOCK_LOCKED)
	    sym->color = CLR_RED;
	else if (dbe->dgnflags & NH_DF_DOORLOCK_UNLOCKED)
	    sym->color = CLR_GREEN;
    }

    if (id == altar_id) {
	if (level_display_mode == LDM_ENDGAME) {
	    sym->color = CLR_BRIGHT_MAGENTA;
	} else {
	    short a = (dbe->dgnflags & NH_DF_ALTARALIGN_MASK);
	    sym->color = (a == NH_DF_ALTARALIGN_OTHER) ? CLR_RED :
			 (a == NH_DF_ALTARALIGN_LAWFUL) ? CLR_WHITE :
			 (a == NH_DF_ALTARALIGN_NEUTRAL) ? CLR_GRAY :
			 (a == NH_DF_ALTARALIGN_CHAOTIC) ? CLR_BLACK :
			 CLR_BRIGHT_GREEN; /* shouldn't happen */
	}
    }
}


void object_symdef(int otyp, int obj_mn, struct curses_symdef *sym)
{
    *sym = cur_drawing->objects[otyp];
    if (otyp == corpse_id)
	sym->color = cur_drawing->monsters[obj_mn].color;
}


int mapglyph(struct nh_dbuf_entry *dbe, struct curses_symdef *syms, int *bg_color)
{
    int id, count = 0;

    if (dbe->effect) {
	id = NH_EFFECT_ID(dbe->effect);

	switch (NH_EFFECT_TYPE(dbe->effect)) {
	    case E_EXPLOSION:
		syms[0] = cur_drawing->explsyms[id % NUMEXPCHARS];
		syms[0].color = cur_drawing->expltypes[id / NUMEXPCHARS].color;
		break;

	    case E_SWALLOW:
		syms[0] = cur_drawing->swallowsyms[id & 0x7];
		syms[0].color = cur_drawing->monsters[id >> 3].color;
		break;

	    case E_ZAP:
		syms[0] = cur_drawing->zapsyms[id & 0x3];
		syms[0].color = cur_drawing->zaptypes[id >> 2].color;
		break;

	    case E_BREATH:
		syms[0] = cur_drawing->breathsyms[id & 0x3];
		syms[0].color = cur_drawing->zaptypes[id >> 2].color;
		break;

	    case E_MISC:
		syms[0] = cur_drawing->effects[id];
		syms[0].color = cur_drawing->effects[id].color;
		break;
	}

	valley_symdef(&syms[0]);

	return 1; /* we don't want to show other glyphs under effects */
    }
    
    if (dbe->invis) {
	syms[count] = cur_drawing->invis[0];
	valley_symdef(&syms[count]);
	count++;
    } else if (dbe->mon) {
	if (dbe->mon > cur_drawing->num_monsters && (dbe->monflags & MON_WARNING)) {
	    id = dbe->mon - 1 - cur_drawing->num_monsters;
	    syms[count] = cur_drawing->warnings[id];
	} else {
	    id = dbe->mon - 1;
	    syms[count] = cur_drawing->monsters[id];
	}
	valley_symdef(&syms[count]);
	count++;
    }
    
    if (dbe->obj) {
	object_symdef(dbe->obj - 1, dbe->obj_mn - 1, &syms[count]);

	if (dbe->objflags & DOBJ_PRIZE)
	    syms[count].color = CLR_BRIGHT_MAGENTA;
	valley_symdef(&syms[count]);
	if (settings.darkroom && !(dbe->dgnflags & NH_DF_VISIBLE_MASK))
	    darken_symdef(&syms[count]);

	count++;
    }
    
    if (dbe->trap) {
	id = dbe->trap - 1;
	syms[count] = cur_drawing->traps[id];

	valley_symdef(&syms[count]);
	if (settings.darkroom && !(dbe->dgnflags & NH_DF_VISIBLE_MASK))
	    darken_symdef(&syms[count]);

	if (id == mportal_id || id == vibsquare_id)
	    *bg_color = CLR_RED;

	count++;
    } 
    
    /* omit the background symbol from the list if it is boring */
    if (count == 0 || dbe->bg >= cur_drawing->bg_feature_offset) {
	syms[count] = cur_drawing->bgelements[dbe->bg];
	recolor_bg(dbe, &syms[count]);
	valley_symdef(&syms[count]);
	if (settings.darkroom && !(dbe->dgnflags & NH_DF_VISIBLE_MASK))
	    darken_symdef(&syms[count]);
	count++;
    }

    if (dbe->bg == upstair_id  || dbe->bg == dnstair_id  ||
	dbe->bg == upladder_id || dbe->bg == dnladder_id ||
	dbe->bg == upsstair_id || dbe->bg == dnsstair_id) {
	*bg_color = CLR_RED;
    }

    /* monsters override background color, NAO-style */
    if (dbe->mon || dbe->invis)
	*bg_color = 0;

    return count; /* count <= 4 */
}


void set_rogue_level(nh_bool enable)
{
    if (enable)
	cur_drawing = rogue_drawing;
    else
	switch_graphics(settings.graphics);
}


void curses_notify_level_changed(int dmode)
{
    level_display_mode = dmode;
    set_rogue_level(dmode == LDM_ROGUE);
}


void switch_graphics(enum nh_text_mode mode)
{
    switch (mode) {
	default:
	case ASCII_GRAPHICS:
	    cur_drawing = default_drawing;
	    break;

/*
 * Drawing with the full unicode charset. Naturally this requires a unicode terminal.
 */
	case UNICODE_GRAPHICS:
	    if (ui_flags.unicode)
		cur_drawing = unicode_drawing;
	    else
		cur_drawing = default_drawing;
	    break;
    }

    /* Set box drawing characters. */
    nh_box_set_graphics(mode);
}


void print_sym(WINDOW *win, struct curses_symdef *sym,
	       attr_t extra_attrs, int bg_color)
{
    attr_t attr;
    cchar_t uni_out;

    /* in-game color index -> curses color */
    attr = A_NORMAL | extra_attrs;
    if (ui_flags.color) {
	attr |= curses_color_attr(sym->color & CLR_MASK, bg_color);
	if (sym->color & HI_ULINE) attr |= A_UNDERLINE;
    }

    /* print it; preferably as unicode */
    if (sym->unichar[0] && ui_flags.unicode) {
	int color = PAIR_NUMBER(attr);
	setcchar(&uni_out, sym->unichar, attr, color, NULL);
	wadd_wch(win, &uni_out);
    } else {
	wattron(win, attr);
	waddch(win, sym->ch);
	wattroff(win, attr);
    }
}

/* outchars.c */
