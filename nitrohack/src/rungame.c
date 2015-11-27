/* Copyright (c) Daniel Thaler, 2011.                             */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#if !defined(WIN32)
# include <dirent.h>
# include <libgen.h>
#endif


#if defined(WIN32)

nh_bool get_gamedir(enum game_dirs dirtype, wchar_t *buf)
{
    wchar_t *subdir;
    wchar_t appPath[MAX_PATH], nhPath[MAX_PATH];

    if (override_userdir) {
	_snwprintf(nhPath, MAX_PATH, L"%S", override_userdir);
	nhPath[MAX_PATH - 1] = 0;
    } else {
	/*
	 * Get the location of "AppData\Roaming" (Vista, 7) or "Application
	 * Data" (XP).  The returned Path does not include a trailing
	 * backslash.
	 */
	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appPath)))
	    return FALSE;
	_snwprintf(nhPath, MAX_PATH, L"%s\\DynaHack", appPath);
	nhPath[MAX_PATH - 1] = 0;
	_wmkdir(nhPath);
    }

    switch (dirtype) {
	case CONFIG_DIR: subdir = L"\\"; break;
	case SAVE_DIR:   subdir = L"\\save\\"; break;
	case LOG_DIR:    subdir = L"\\log\\"; break;
	case DUMP_DIR:   subdir = L"\\dumps\\"; break;
    }

    _snwprintf(buf, BUFSZ, L"%s%s", nhPath, subdir);
    buf[BUFSZ - 1] = 0;
    _wmkdir(buf);

    return TRUE;
}

#else /* defined(WIN32) */

nh_bool get_gamedir(enum game_dirs dirtype, char *buf)
{
    char *envval, *subdir;
    mode_t mask;

    switch (dirtype) {
	case CONFIG_DIR: subdir = ""; break;
	case SAVE_DIR:   subdir = "save/"; break;
	case LOG_DIR:    subdir = "log/"; break;
	case DUMP_DIR:   subdir = "dumps/"; break;
    }

    if (override_userdir && getgid() == getegid()) {
	snprintf(buf, BUFSZ, "%s/%s", override_userdir, subdir);
    } else {
	/* look in regular location */
	envval = getenv("XDG_CONFIG_HOME");
	if (envval) {
	    snprintf(buf, BUFSZ, "%s/DynaHack/%s", envval, subdir);
	} else {
	    envval = getenv("HOME");
	    if (!envval) /* HOME not set? just give up... */
		return FALSE;
	    snprintf(buf, BUFSZ, "%s/.config/DynaHack/%s", envval, subdir);
	}
    }

    mask = umask(0);
    if (mkdir(buf, 0755) == -1 && errno != EEXIST) {
	/* try to create the parent directory too. This is the only problem we
	 * can fix here - permission problems etc. all requre user intervention */
	char dirbuf[BUFSZ], *basedir;
	strcpy(dirbuf, buf);
	basedir = dirname(dirbuf);
	
	mkdir(basedir, 0755); /* no need to check the return value: if it doesn't work, we're screwed */
	if (mkdir(buf, 0755) == -1) {
	    umask(mask);
	    return FALSE;
	}
    }
    umask(mask);

    return TRUE;
}

#endif


int commandloop(void)
{
    const char *cmd;
    int gamestate, count;
    struct nh_cmd_arg cmdarg;
    
    gamestate = READY_FOR_INPUT;
    game_is_running = TRUE;
    reset_prev_cmd();
    reset_last_dir();
    
    while (gamestate < GAME_OVER) {
	count = 0;
	cmd = NULL;
	
	if (gamestate == READY_FOR_INPUT) {
	    update_old_status();
	    cmd = get_command(&count, &cmdarg);
	} else if (gamestate == MULTI_IN_PROGRESS && interrupt_multi) {
	    count = -1;
	}
	
	interrupt_multi = FALSE; /* could have been set while no multi was in progress */
	gamestate = nh_command(cmd, count, &cmdarg);
    }
    
    game_is_running = FALSE;
    return gamestate;
}


static void game_ended(int status, fnchar *filename)
{
    fnchar logname[1024], savedir[BUFSZ], *bp;
#if !defined(WIN32)
    fnchar fncopy[1024], *fname;
#endif

    if (status != GAME_OVER)
	return;
    
    show_topten(player.plname, settings.end_top, settings.end_around,
		settings.end_own);
    
    /* 
     * The game ended terminally. Now it would be nice to move the saved game
     * out of the save/ dir and into the log/ dir, since it's only use now is as
     * a tropy.
     */
#if !defined(WIN32)
    /* dirname and basename may modify the input string, depending on the system */
    strncpy(fncopy, filename, sizeof(fncopy));
    bp = dirname(fncopy);

    get_gamedir(SAVE_DIR, savedir);
    savedir[strlen(savedir)-1] = '\0'; /* remove the trailing '/' */
    if (strcmp(bp, savedir) != 0)
	return; /* file was not in savedir, so don't touch it */

    get_gamedir(LOG_DIR, logname);
    strncpy(fncopy, filename, sizeof(fncopy));
    fncopy[sizeof(fncopy) - 1] = '\0';
    fname = basename(fncopy);
    strncat(logname, fname, sizeof(logname)-1);

    if (rename(filename, logname) == 0) {
	/* loosen permissions for preserved save log files */
	chmod(logname, 0644);
    }
#else
    bp = wcsrchr(filename, L'\\');
    get_gamedir(SAVE_DIR, savedir);
    if (!bp || wcsncmp(filename, savedir, wcslen(savedir)))
	return;

    get_gamedir(LOG_DIR, logname);
    wcsncat(logname, bp+1, 1024);
    
    /* don't care about errors: rename is nice to have, not essential */
    _wrename(filename, logname);
#endif
}


void rungame(nh_bool tutorial)
{
    int ret, role = initrole, race = initrace, gend = initgend, align = initalign;
    int fd = -1;
    char chardesc[QBUFSZ];
    char nameprompt[QBUFSZ];
    char plname[BUFSZ];
    fnchar filename[1024];
    fnchar savedir[BUFSZ];
    long t;
    
    if (!get_gamedir(SAVE_DIR, savedir)) {
	curses_raw_print("Could not find where to put the logfile for a new game.");
	return;
    }
    
    if (!player_selection(&role, &race, &gend, &align, random_player, tutorial))
	return;
    
    strncpy(plname, settings.plname, PL_NSIZ);
    plname[PL_NSIZ - 1] = 0;
    /* The player name is set to "wizard" (again) in nh_start_game, so setting
     * it here just prevents wizmode player from being asked for a name. */
    if (ui_flags.playmode == MODE_WIZARD)
	strcpy(plname, "wizard");

    nh_root_plselection_prompt(chardesc, QBUFSZ - 1, role, race, gend, align);
    snprintf(nameprompt, QBUFSZ, "You are a %s.  What is your name?", chardesc);
    while (!plname[0]) {
	curses_getline(nameprompt, plname);
	if (strlen(plname) >= PL_NSIZ) {
	    curses_msgwin("That name is too long.");
	    plname[0] = 0;
	}
    }
    if (plname[0] == '\033') /* canceled */
	return;

    t = (long)time(NULL);
#if defined(WIN32)
    _snwprintf(filename, 1024, L"%ls%ld_%hs.nhgame", savedir, t, plname);
    filename[1023] = 0;
#else
    snprintf(filename, 1024, "%s%ld_%s.nhgame", savedir, t, plname);
#endif
    fd = sys_open(filename, O_TRUNC | O_CREAT | O_RDWR, FILE_OPEN_MASK);
    if (fd == -1) {
	curses_raw_print("Could not create the logfile.");
	return;
    }
    
    create_game_windows();
    reset_old_status();
    if (!nh_start_game(fd, plname, role, race, gend, align,
		       tutorial ? MODE_TUTORIAL : ui_flags.playmode)) {
	destroy_game_windows();
	close(fd);
	return;
    }
    
    load_keymap(); /* need to load the keymap after the game has been started */
    ret = commandloop();
    free_keymap();
    close(fd);
    
    destroy_game_windows();
    cleanup_messages();
    game_ended(ret, filename);
}


void describe_game(char *buf, enum nh_log_status status, struct nh_game_info *gi)
{
    const char *mode_desc[] = {"", "\t[tutorial]", "\t[explore]", "\t[wizard]"};
    switch (status) {
	case LS_CRASHED:
	    snprintf(buf, BUFSZ, "%s\t%3.3s-%3.3s-%3.3s-%3.3s  (crashed)\t%s",
		    gi->name, gi->plrole, gi->plrace, gi->plgend, gi->plalign,
		    mode_desc[gi->playmode]);
	    break;
	    
	case LS_IN_PROGRESS:
	    snprintf(buf, BUFSZ, "    %s\t%3.3s-%3.3s-%3.3s-%3.3s  (in progress)\t%s",
		    gi->name, gi->plrole, gi->plrace, gi->plgend, gi->plalign,
		    mode_desc[gi->playmode]);
	    break;
	    
	case LS_SAVED:
	    snprintf(buf, BUFSZ, "%s\t%3.3s-%3.3s-%3.3s-%3.3s %s%s\tafter %d moves%s",
		    gi->name, gi->plrole, gi->plrace, gi->plgend, gi->plalign,
		    gi->level_desc, gi->has_amulet ? " with the amulet" : "",
		    gi->moves, mode_desc[gi->playmode]);
	    break;
	    
	case LS_DONE:
	    snprintf(buf, BUFSZ, "%s\t%3.3s-%3.3s-%3.3s-%3.3s %s\tafter %d moves%s",
		    gi->name, gi->plrole, gi->plrace, gi->plgend, gi->plalign,
		    gi->death, gi->moves, mode_desc[gi->playmode]);
	    break;
	    
	    
	default:
	    buf[0] = '\0';
	    break;
    }
}


#if defined(WIN32)


static int compare_filetime(const void *arg1, const void *arg2)
{
    const WIN32_FIND_DATA *file1 = (const WIN32_FIND_DATA*)arg1;
    const WIN32_FIND_DATA *file2 = (const WIN32_FIND_DATA*)arg2;
    
    if (file1->ftLastWriteTime.dwHighDateTime != file2->ftLastWriteTime.dwHighDateTime)
	return file2->ftLastWriteTime.dwHighDateTime - file1->ftLastWriteTime.dwHighDateTime;
    return file2->ftLastWriteTime.dwLowDateTime - file1->ftLastWriteTime.dwLowDateTime;
}


wchar_t **list_gamefiles(wchar_t *dir, int *count)
{
    wchar_t filepattern[1024], fullname[1024], **filenames;
    WIN32_FIND_DATA *find_data;
    HANDLE h_search;
    int fd, i;
    enum nh_log_status status;

    wcsncpy(filepattern, dir, 1024);
    wcsncat(filepattern, L"*.nhgame", 1024);
    *count = 0;
    find_data = malloc(1 * sizeof(WIN32_FIND_DATA));

    h_search = FindFirstFile(filepattern, &find_data[0]);
    if (h_search == INVALID_HANDLE_VALUE)
	return NULL;

    do {
	if (find_data[*count].nFileSizeHigh == 0 && find_data[*count].nFileSizeLow < 64)
	    continue; /* too small to be valid */

	_snwprintf(fullname, 1024, L"%s%s", dir, find_data[*count].cFileName);
	fullname[1023] = 0;
	fd = _wopen(fullname, O_RDWR, _S_IREAD | _S_IWRITE);
	if (fd == -1)
	    continue;

	status = nh_get_savegame_status(fd, NULL);
	close(fd);

	if (status != LS_INVALID) {
	     (*count)++;
	     find_data = realloc(find_data, (*count + 1) * sizeof(WIN32_FIND_DATA));
	}
    } while (FindNextFile(h_search, &find_data[*count]));

    if (h_search != INVALID_HANDLE_VALUE)
	FindClose(h_search);

    qsort(find_data, *count, sizeof(WIN32_FIND_DATA), compare_filetime);

    filenames = malloc(*count * sizeof(wchar_t*));
    for (i = 0; i < *count; i++) {
	_snwprintf(fullname, 1024, L"%s%s", dir, find_data[i].cFileName);
	fullname[1023] = 0;
	filenames[i] = _wcsdup(fullname);
    }
    free(find_data);

    return filenames;
}

#else

static int compare_filetime(const void *arg1, const void *arg2)
{
    const char *file1 = *(const char**)arg1;
    const char *file2 = *(const char**)arg2;
    
    struct stat s1, s2;
    
    stat(file1, &s1);
    stat(file2, &s2);
    
    return s2.st_mtime - s1.st_mtime;
}


char **list_gamefiles(char *dir, int *count)
{
    char filename[1024], **files;
    DIR *dirp;
    struct dirent *dp;
    int namelen, fd;
    enum nh_log_status status;
    
    files = NULL;
    *count = 0;
    
    dirp = opendir(dir);
    while ((dp = readdir(dirp)) != NULL) {
	namelen = strlen(dp->d_name);
	if (namelen > 7 && /* ".nhgame" */
	    !strcmp(&dp->d_name[namelen-7], ".nhgame")) {
	    snprintf(filename, sizeof(filename), "%s%s", dir, dp->d_name);
	    
	    fd = open(filename, O_RDWR, 0660);
	    status = nh_get_savegame_status(fd, NULL);
	    close(fd);
	    
	    if (status == LS_INVALID)
		continue;
	    
	    (*count)++;
	    files = realloc(files, (*count) * sizeof(char*));
	    files[*count - 1] = strdup(filename);
	}
    }
    closedir(dirp);
    
    if (files)
	qsort(files, *count, sizeof(char*), compare_filetime);
    
    return files;
}
#endif


nh_bool loadgame(void)
{
    char buf[BUFSZ];
    fnchar savedir[BUFSZ], filename[1024], **files;
    struct nh_menuitem *items;
    int size, icount, fd, i, n, ret, pick[1];
    enum nh_log_status status;
    struct nh_game_info gi;
    
    if (!get_gamedir(SAVE_DIR, savedir)) {
	curses_raw_print("Could not find or create the save directory.");
	return FALSE;
    }
    
    files = list_gamefiles(savedir, &size);
    if (!size) {
	curses_msgwin("No saved games found.");
	return FALSE;
    }
    
    icount = 0;
    items = malloc(size * sizeof(struct nh_menuitem));

    for (i = 0; i < size; i++) {
	fd = sys_open(files[i], O_RDWR, FILE_OPEN_MASK);
	status = nh_get_savegame_status(fd, &gi);
	close(fd);
	
	describe_game(buf, status, &gi);
	add_menu_item(items, size, icount, (status == LS_IN_PROGRESS) ? 0 : icount + 1,
			buf, 0, FALSE);
    }

    n = curses_display_menu(items, icount, "saved games", PICK_ONE, pick);    
    free(items);
    filename[0] = '\0';
    if (n > 0)
	fnncat(filename, files[pick[0]-1], sizeof(filename)/sizeof(fnchar)-1);

    for (i = 0; i < icount; i++)
	free(files[i]);
    free(files);
    if (n <= 0)
	return FALSE;
    
    fd = sys_open(filename, O_RDWR, FILE_OPEN_MASK);
    create_game_windows();
    reset_old_status();
    if (nh_restore_game(fd, NULL, FALSE) != GAME_RESTORED) {
	destroy_game_windows();
	close(fd);
	if (curses_yn_function("Failed to load the save. Do you wish to delete the file?", "yn", 'n') == 'y')
	    unlink(filename);
	return FALSE;
    }
    
    load_keymap(); /* need to load the keymap after the game has been started */
    ret = commandloop();
    free_keymap();
    close(fd);
    
    destroy_game_windows();
    cleanup_messages();
    game_ended(ret, filename);
    
    return TRUE;
}
