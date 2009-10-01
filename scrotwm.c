/* $scrotwm$ */
/*
 * Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Much code and ideas taken from dwm under the following license:
 * MIT/X Consortium License
 *
 * 2006-2008 Anselm R Garbe <garbeam at gmail dot com>
 * 2006-2007 Sander van Dijk <a dot h dot vandijk at gmail dot com>
 * 2006-2007 Jukka Salmi <jukka at salmi dot ch>
 * 2007 Premysl Hruby <dfenze at gmail dot com>
 * 2007 Szabolcs Nagy <nszabolcs at gmail dot com>
 * 2007 Christof Musik <christof at sendfax dot de>
 * 2007-2008 Enno Gottox Boland <gottox at s01 dot de>
 * 2007-2008 Peter Hartlich <sgkkr at hartlich dot com>
 * 2008 Martin Hurton <martin dot hurton at gmail dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

static const char	*cvstag = "$scrotwm$";

#define	SWM_VERSION	"0.9.9"

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <util.h>
#include <pwd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/select.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#if RANDR_MAJOR < 1
#  error XRandR versions less than 1.0 are not supported
#endif

#if RANDR_MAJOR >= 1
#if RANDR_MINOR >= 2
#define SWM_XRR_HAS_CRTC
#endif
#endif

/* #define SWM_DEBUG */
#ifdef SWM_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while(0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while(0)
#define	SWM_D_MISC		0x0001
#define	SWM_D_EVENT		0x0002
#define	SWM_D_WS		0x0004
#define	SWM_D_FOCUS		0x0008
#define	SWM_D_MOVE		0x0010
#define	SWM_D_STACK		0x0020
#define	SWM_D_MOUSE		0x0040
#define	SWM_D_PROP		0x0080
#define	SWM_D_CLASS		0x0100
#define SWM_D_KEY		0x0200
#define SWM_D_QUIRK		0x0400
#define SWM_D_SPAWN		0x0800
#define SWM_D_EVENTQ		0x1000
#define SWM_D_CONF		0x2000

u_int32_t		swm_debug = 0
			    | SWM_D_MISC
			    | SWM_D_EVENT
			    | SWM_D_WS
			    | SWM_D_FOCUS
			    | SWM_D_MOVE
			    | SWM_D_STACK
			    | SWM_D_MOUSE
			    | SWM_D_PROP
			    | SWM_D_CLASS
			    | SWM_D_KEY
			    | SWM_D_QUIRK
			    | SWM_D_SPAWN
			    | SWM_D_EVENTQ
			    | SWM_D_CONF
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define LENGTH(x)		(sizeof x / sizeof x[0])
#define MODKEY			Mod1Mask
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define BUTTONMASK		(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK		(BUTTONMASK|PointerMotionMask)
#define SWM_PROPLEN		(16)
#define SWM_FUNCNAME_LEN	(32)
#define SWM_KEYS_LEN		(255)
#define SWM_QUIRK_LEN		(64)
#define X(r)			(r)->g.x
#define Y(r)			(r)->g.y
#define WIDTH(r)		(r)->g.w
#define HEIGHT(r)		(r)->g.h
#define SWM_MAX_FONT_STEPS	(3)

#ifndef SWM_LIB
#define SWM_LIB			"/usr/local/lib/libswmhack.so"
#endif

char			**start_argv;
Atom			astate;
Atom			aprot;
Atom			adelete;
volatile sig_atomic_t   running = 1;
int			outputs = 0;
int			(*xerrorxlib)(Display *, XErrorEvent *);
int			other_wm;
int			ss_enabled = 0;
int			xrandr_support;
int			xrandr_eventbase;
int			ignore_enter = 0;
unsigned int		numlockmask = 0;
Display			*display;

int			cycle_empty = 0;
int			cycle_visible = 0;
int			term_width = 0;
int			font_adjusted = 0;
unsigned int		mod_key = MODKEY;

/* dialog windows */
double			dialog_ratio = .6;
/* status bar */
#define SWM_BAR_MAX	(256)
char			*bar_argv[] = { NULL, NULL };
int			bar_pipe[2];
char			bar_ext[SWM_BAR_MAX];
char			bar_vertext[SWM_BAR_MAX];
int			bar_version = 0;
sig_atomic_t		bar_alarm = 0;
int			bar_delay = 30;
int			bar_enabled = 1;
int			bar_extra = 1;
int			bar_extra_running = 0;
int			bar_verbose = 1;
int			bar_height = 0;
int			clock_enabled = 1;
int			title_name_enabled = 0;
int			title_class_enabled = 0;
pid_t			bar_pid;
GC			bar_gc;
XGCValues		bar_gcv;
int			bar_fidx = 0;
XFontStruct		*bar_fs;
char			*bar_fonts[] = { NULL, NULL, NULL, NULL };/* XXX Make fully dynamic */
char			*spawn_term[] = { NULL, NULL };		/* XXX Make fully dynamic */

#define SWM_MENU_FN	(2)
#define SWM_MENU_NB	(4)
#define SWM_MENU_NF	(6)
#define SWM_MENU_SB	(8)
#define SWM_MENU_SF	(10)

/* layout manager data */
struct swm_geometry {
	int			x;
	int			y;
	int			w;
	int			h;
};

struct swm_screen;
struct workspace;

/* virtual "screens" */
struct swm_region {
	TAILQ_ENTRY(swm_region)	entry;
	struct swm_geometry	g;
	struct workspace	*ws;	/* current workspace on this region */
	struct swm_screen	*s;	/* screen idx */
	Window			bar_window;
};
TAILQ_HEAD(swm_region_list, swm_region);

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	Window			id;
	struct swm_geometry	g;
	int			floating;
	int			transient;
	int			manual;
	int			font_size_boundary[SWM_MAX_FONT_STEPS];
	int			font_steps;
	int			last_inc;
	int			can_delete;
	unsigned long		quirks;
	struct workspace	*ws;	/* always valid */
	struct swm_screen	*s;	/* always valid, never changes */
	XWindowAttributes	wa;
	XSizeHints		sh;
	XClassHint		ch;
};
TAILQ_HEAD(ws_win_list, ws_win);

/* user/key callable function IDs */
enum keyfuncid {
	kf_cycle_layout,
	kf_stack_reset,
	kf_master_shrink,
	kf_master_grow,
	kf_master_add,
	kf_master_del,
	kf_stack_inc,
	kf_stack_dec,
	kf_swap_main,
	kf_focus_next,
	kf_focus_prev,
	kf_swap_next,
	kf_swap_prev,
	kf_spawn_term,
	kf_spawn_menu,
	kf_quit,
	kf_restart,
	kf_focus_main,
	kf_ws_1,
	kf_ws_2,
	kf_ws_3,
	kf_ws_4,
	kf_ws_5,
	kf_ws_6,
	kf_ws_7,
	kf_ws_8,
	kf_ws_9,
	kf_ws_10,
	kf_ws_next,
	kf_ws_prev,
	kf_screen_next,
	kf_screen_prev,
	kf_mvws_1,
	kf_mvws_2,
	kf_mvws_3,
	kf_mvws_4,
	kf_mvws_5,
	kf_mvws_6,
	kf_mvws_7,
	kf_mvws_8,
	kf_mvws_9,
	kf_mvws_10,
	kf_bar_toggle,
	kf_wind_kill,
	kf_wind_del,
	kf_screenshot_all,
	kf_screenshot_wind,
	kf_float_toggle,
	kf_version,
	kf_spawn_lock,
	kf_spawn_initscr,
	kf_spawn_custom,
	kf_invalid
};

/* layout handlers */
void	stack(void);
void	vertical_config(struct workspace *, int);
void	vertical_stack(struct workspace *, struct swm_geometry *);
void	horizontal_config(struct workspace *, int);
void	horizontal_stack(struct workspace *, struct swm_geometry *);
void	max_stack(struct workspace *, struct swm_geometry *);

void	grabbuttons(struct ws_win *, int);
void	new_region(struct swm_screen *, int, int, int, int);

struct layout {
	void		(*l_stack)(struct workspace *, struct swm_geometry *);
	void		(*l_config)(struct workspace *, int);
} layouts[] =  {
	/* stack,		configure */
	{ vertical_stack,	vertical_config},
	{ horizontal_stack,	horizontal_config},
	{ max_stack,		NULL},
	{ NULL,			NULL},
};

#define SWM_H_SLICE		(32)
#define SWM_V_SLICE		(32)

/* define work spaces */
struct workspace {
	int			idx;		/* workspace index */
	int			restack;	/* restack on switch */
	struct layout		*cur_layout;	/* current layout handlers */
	struct ws_win		*focus;		/* may be NULL */
	struct ws_win		*focus_prev;	/* may be NULL */
	struct swm_region	*r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */

	/* stacker state */
	struct {
				int horizontal_msize;
				int horizontal_mwin;
				int horizontal_stacks;
				int vertical_msize;
				int vertical_mwin;
				int vertical_stacks;
	} l_state;
};

enum	{ SWM_S_COLOR_BAR, SWM_S_COLOR_BAR_BORDER, SWM_S_COLOR_BAR_FONT,
	  SWM_S_COLOR_FOCUS, SWM_S_COLOR_UNFOCUS, SWM_S_COLOR_MAX };

/* physical screen mapping */
#define SWM_WS_MAX		(10)		/* XXX Too small? */
struct swm_screen {
	int			idx;		/* screen index */
	struct swm_region_list	rl;	/* list of regions on this screen */
	struct swm_region_list	orl;	/* list of old regions */
	Window			root;
	struct workspace	ws[SWM_WS_MAX];

	/* colors */
	struct {
		unsigned long	color;
		char		*name;
	} c[SWM_S_COLOR_MAX];
};
struct swm_screen	*screens;
int			num_screens;

/* args to functions */
union arg {
	int			id;
#define SWM_ARG_ID_FOCUSNEXT	(0)
#define SWM_ARG_ID_FOCUSPREV	(1)
#define SWM_ARG_ID_FOCUSMAIN	(2)
#define SWM_ARG_ID_SWAPNEXT	(3)
#define SWM_ARG_ID_SWAPPREV	(4)
#define SWM_ARG_ID_SWAPMAIN	(5)
#define SWM_ARG_ID_MASTERSHRINK (6)
#define SWM_ARG_ID_MASTERGROW	(7)
#define SWM_ARG_ID_MASTERADD	(8)
#define SWM_ARG_ID_MASTERDEL	(9)
#define SWM_ARG_ID_STACKRESET	(10)
#define SWM_ARG_ID_STACKINIT	(11)
#define SWM_ARG_ID_CYCLEWS_UP	(12)
#define SWM_ARG_ID_CYCLEWS_DOWN	(13)
#define SWM_ARG_ID_CYCLESC_UP	(14)
#define SWM_ARG_ID_CYCLESC_DOWN	(15)
#define SWM_ARG_ID_STACKINC	(16)
#define SWM_ARG_ID_STACKDEC	(17)
#define SWM_ARG_ID_SS_ALL	(0)
#define SWM_ARG_ID_SS_WINDOW	(1)
#define SWM_ARG_ID_DONTCENTER	(0)
#define SWM_ARG_ID_CENTER	(1)
#define SWM_ARG_ID_KILLWINDOW	(0)
#define SWM_ARG_ID_DELETEWINDOW	(1)
	char			**argv;
};

/* quirks */
struct quirk {
	char			*class;
	char			*name;
	unsigned long		quirk;
#define SWM_Q_FLOAT		(1<<0)	/* float this window */
#define SWM_Q_TRANSSZ		(1<<1)	/* transiend window size too small */
#define SWM_Q_ANYWHERE		(1<<2)	/* don't position this window */
#define SWM_Q_XTERM_FONTADJ	(1<<3)	/* adjust xterm fonts when resizing */
#define SWM_Q_FULLSCREEN	(1<<4)	/* remove border */
};
int				quirks_size = 0, quirks_length = 0;
struct quirk			*quirks = NULL;

/* events */
#ifdef SWM_DEBUG
void
dumpevent(XEvent *e)
{
	char			*name = NULL;

	switch (e->type) {
	case KeyPress:
		name = "KeyPress";
		break;
	case KeyRelease:
		name = "KeyRelease";
		break;
	case ButtonPress:
		name = "ButtonPress";
		break;
	case ButtonRelease:
		name = "ButtonRelease";
		break;
	case MotionNotify:
		name = "MotionNotify";
		break;
	case EnterNotify:
		name = "EnterNotify";
		break;
	case LeaveNotify:
		name = "LeaveNotify";
		break;
	case FocusIn:
		name = "FocusIn";
		break;
	case FocusOut:
		name = "FocusOut";
		break;
	case KeymapNotify:
		name = "KeymapNotify";
		break;
	case Expose:
		name = "Expose";
		break;
	case GraphicsExpose:
		name = "GraphicsExpose";
		break;
	case NoExpose:
		name = "NoExpose";
		break;
	case VisibilityNotify:
		name = "VisibilityNotify";
		break;
	case CreateNotify:
		name = "CreateNotify";
		break;
	case DestroyNotify:
		name = "DestroyNotify";
		break;
	case UnmapNotify:
		name = "UnmapNotify";
		break;
	case MapNotify:
		name = "MapNotify";
		break;
	case MapRequest:
		name = "MapRequest";
		break;
	case ReparentNotify:
		name = "ReparentNotify";
		break;
	case ConfigureNotify:
		name = "ConfigureNotify";
		break;
	case ConfigureRequest:
		name = "ConfigureRequest";
		break;
	case GravityNotify:
		name = "GravityNotify";
		break;
	case ResizeRequest:
		name = "ResizeRequest";
		break;
	case CirculateNotify:
		name = "CirculateNotify";
		break;
	case CirculateRequest:
		name = "CirculateRequest";
		break;
	case PropertyNotify:
		name = "PropertyNotify";
		break;
	case SelectionClear:
		name = "SelectionClear";
		break;
	case SelectionRequest:
		name = "SelectionRequest";
		break;
	case SelectionNotify:
		name = "SelectionNotify";
		break;
	case ColormapNotify:
		name = "ColormapNotify";
		break;
	case ClientMessage:
		name = "ClientMessage";
		break;
	case MappingNotify:
		name = "MappingNotify";
		break;
	}

	if (name)
		DNPRINTF(SWM_D_EVENTQ ,"window: %lu event: %s (%d), %d "
		    "remaining\n",
		    e->xany.window, name, e->type, QLength(display));
	else
		DNPRINTF(SWM_D_EVENTQ, "window: %lu unknown event %d, %d "
		    "remaining\n",
		    e->xany.window, e->type, QLength(display));
}
#else
#define dumpevent(e)
#endif /* SWM_DEBUG */

void			expose(XEvent *);
void			keypress(XEvent *);
void			buttonpress(XEvent *);
void			configurerequest(XEvent *);
void			configurenotify(XEvent *);
void			destroynotify(XEvent *);
void			enternotify(XEvent *);
void			focusin(XEvent *);
void			focusout(XEvent *);
void			mappingnotify(XEvent *);
void			maprequest(XEvent *);
void			propertynotify(XEvent *);
void			unmapnotify(XEvent *);
void			visibilitynotify(XEvent *);

void			(*handler[LASTEvent])(XEvent *) = {
				[Expose] = expose,
				[KeyPress] = keypress,
				[ButtonPress] = buttonpress,
				[ConfigureRequest] = configurerequest,
				[ConfigureNotify] = configurenotify,
				[DestroyNotify] = destroynotify,
				[EnterNotify] = enternotify,
				[FocusIn] = focusin,
				[FocusOut] = focusout,
				[MappingNotify] = mappingnotify,
				[MapRequest] = maprequest,
				[PropertyNotify] = propertynotify,
				[UnmapNotify] = unmapnotify,
				[VisibilityNotify] = visibilitynotify,
};

void
sighdlr(int sig)
{
	pid_t			pid;

	switch (sig) {
	case SIGCHLD:
		while ((pid = waitpid(WAIT_ANY, NULL, WNOHANG)) != -1) {
			DNPRINTF(SWM_D_MISC, stderr, "reaping: %d\n", pid);
			if (pid <= 0)
				break;
		}
		break;
	case SIGINT:
	case SIGTERM:
	case SIGHUP:
	case SIGQUIT:
		running = 0;
		break;
	}
}

void
installsignal(int sig, char *name)
{
	struct sigaction	sa;

	sa.sa_handler = sighdlr;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(sig, &sa, NULL) == -1)
		err(1, "could not install %s handler", name);
}

unsigned long
name_to_color(char *colorname)
{
	Colormap		cmap;
	Status			status;
	XColor			screen_def, exact_def;
	unsigned long		result = 0;
	char			cname[32] = "#";

	cmap = DefaultColormap(display, screens[0].idx);
	status = XAllocNamedColor(display, cmap, colorname,
	    &screen_def, &exact_def);
	if (!status) {
		strlcat(cname, colorname + 2, sizeof cname - 1);
		status = XAllocNamedColor(display, cmap, cname, &screen_def,
		    &exact_def);
	}
	if (status)
		result = screen_def.pixel;
	else
		fprintf(stderr, "color '%s' not found.\n", colorname);

	return (result);
}

void
setscreencolor(char *val, int i, int c)
{
	if (i > 0 && i <= ScreenCount(display)) {
		screens[i - 1].c[c].color = name_to_color(val);
		free(screens[i - 1].c[c].name);
		if ((screens[i - 1].c[c].name = strdup(val)) == NULL)
			errx(1, "strdup");
	} else if (i == -1) {
		for (i = 0; i < ScreenCount(display); i++) {
			screens[i].c[c].color = name_to_color(val);
			free(screens[i].c[c].name);
			if ((screens[i].c[c].name = strdup(val)) == NULL)
				errx(1, "strdup");
		}
	} else
		errx(1, "invalid screen index: %d out of bounds (maximum %d)\n",
		    i, ScreenCount(display));
}

void
custom_region(char *val)
{
	unsigned int			sidx, x, y, w, h;

	if (sscanf(val, "screen[%u]:%ux%u+%u+%u", &sidx, &w, &h, &x, &y) != 5)
		errx(1, "invalid custom region, "
		    "should be 'screen[<n>]:<n>x<n>+<n>+<n>\n");
	if (sidx < 1 || sidx > ScreenCount(display))
		errx(1, "invalid screen index: %d out of bounds (maximum %d)\n",
		    sidx, ScreenCount(display));
	sidx--;

	if (w < 1 || h < 1)
		errx(1, "region %ux%u+%u+%u too small\n", w, h, x, y);

	if (x  < 0 || x > DisplayWidth(display, sidx) ||
	    y < 0 || y > DisplayHeight(display, sidx) ||
	    w + x > DisplayWidth(display, sidx) ||
	    h + y > DisplayHeight(display, sidx))
		errx(1, "region %ux%u+%u+%u not within screen boundaries "
		    "(%ux%u)\n", w, h, x, y,
		    DisplayWidth(display, sidx), DisplayHeight(display, sidx));

	new_region(&screens[sidx], x, y, w, h);
}

void
socket_setnonblock(int fd)
{
	int			flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		err(1, "fcntl F_GETFL");
	flags |= O_NONBLOCK;
	if ((flags = fcntl(fd, F_SETFL, flags)) == -1)
		err(1, "fcntl F_SETFL");
}

void
bar_print(struct swm_region *r, char *s)
{
	XClearWindow(display, r->bar_window);
	XSetForeground(display, bar_gc, r->s->c[SWM_S_COLOR_BAR_FONT].color);
	XDrawString(display, r->bar_window, bar_gc, 4, bar_fs->ascent, s,
	    strlen(s));
}

void
bar_extra_stop(void)
{
	if (bar_pipe[0]) {
		close(bar_pipe[0]);
		bzero(bar_pipe, sizeof bar_pipe);
	}
	if (bar_pid) {
		kill(bar_pid, SIGTERM);
		bar_pid = 0;
	}
	strlcpy(bar_ext, "", sizeof bar_ext);
	bar_extra = 0;
}

void
bar_class_name(char *s, ssize_t sz, struct ws_win *cur_focus)
{
	int			do_class, do_name;
	Status			status;
	XClassHint		*xch = NULL;

	if ((title_name_enabled == 1 || title_class_enabled == 1) &&
	    cur_focus != NULL) {
		if ((xch = XAllocClassHint()) == NULL)
			goto out;
		status = XGetClassHint(display, cur_focus->id, xch);
		if (status == BadWindow || status == BadAlloc)
			goto out;
		do_class = (title_class_enabled && xch->res_class != NULL);
		do_name = (title_name_enabled && xch->res_name != NULL);
		if (do_class)
			strlcat(s, xch->res_class, sz);
		if (do_class && do_name)
			strlcat(s, ":", sz);
		if (do_name)
			strlcat(s, xch->res_name, sz);
		strlcat(s, "    ", sz);
	}
out:
	if (xch)
		XFree(xch);
}

void
bar_update(void)
{
	time_t			tmt;
	struct tm		tm;
	struct swm_region	*r;
	int			i, x;
	size_t			len;
	char			s[SWM_BAR_MAX];
	char			loc[SWM_BAR_MAX];
	char			*b;

	if (bar_enabled == 0)
		return;
	if (bar_extra && bar_extra_running) {
		/* ignore short reads; it'll correct itself */
		while ((b = fgetln(stdin, &len)) != NULL)
			if (b && b[len - 1] == '\n') {
				b[len - 1] = '\0';
				strlcpy(bar_ext, b, sizeof bar_ext);
			}
		if (b == NULL && errno != EAGAIN) {
			fprintf(stderr, "bar_extra failed: errno: %d %s\n",
			    errno, strerror(errno));
			bar_extra_stop();
		}
	} else
		strlcpy(bar_ext, "", sizeof bar_ext);

	if (clock_enabled == 0)
		strlcpy(s, "", sizeof s);
	else {
		time(&tmt);
		localtime_r(&tmt, &tm);
		strftime(s, sizeof s, "%a %b %d %R %Z %Y    ", &tm);
	}

	for (i = 0; i < ScreenCount(display); i++) {
		x = 1;
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (r && r->ws)
				bar_class_name(s, sizeof s, r->ws->focus);

			snprintf(loc, sizeof loc, "%d:%d    %s %s    %s",
			    x++, r->ws->idx + 1, s, bar_ext, bar_vertext);
			bar_print(r, loc);
		}
	}
	XSync(display, False);
	alarm(bar_delay);
}

void
bar_signal(int sig)
{
	bar_alarm = 1;
}

void
bar_toggle(struct swm_region *r, union arg *args)
{
	struct swm_region	*tmpr;
	int			i, j, sc = ScreenCount(display);

	DNPRINTF(SWM_D_MISC, "bar_toggle\n");

	if (bar_enabled)
		for (i = 0; i < sc; i++)
			TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
				XUnmapWindow(display, tmpr->bar_window);
	else
		for (i = 0; i < sc; i++)
			TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
				XMapRaised(display, tmpr->bar_window);

	bar_enabled = !bar_enabled;
	for (i = 0; i < sc; i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			screens[i].ws[j].restack = 1;

	stack();
	/* must be after stack */
	bar_update();
}

void
bar_refresh(void)
{
	XSetWindowAttributes	wa;
	struct swm_region	*r;
	int			i;

	/* do this here because the conf file is in memory */
	if (bar_extra && bar_extra_running == 0 && bar_argv[0]) {
		/* launch external status app */
		bar_extra_running = 1;
		if (pipe(bar_pipe) == -1)
			err(1, "pipe error");
		socket_setnonblock(bar_pipe[0]);
		socket_setnonblock(bar_pipe[1]); /* XXX hmmm, really? */
		if (dup2(bar_pipe[0], 0) == -1)
			errx(1, "dup2");
		if (dup2(bar_pipe[1], 1) == -1)
			errx(1, "dup2");
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			err(1, "could not disable SIGPIPE");
		switch (bar_pid = fork()) {
		case -1:
			err(1, "cannot fork");
			break;
		case 0: /* child */
			close(bar_pipe[0]);
			execvp(bar_argv[0], bar_argv);
			err(1, "%s external app failed", bar_argv[0]);
			break;
		default: /* parent */
			close(bar_pipe[1]);
			break;
		}
	}

	bzero(&wa, sizeof wa);
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			wa.border_pixel =
			    screens[i].c[SWM_S_COLOR_BAR_BORDER].color;
			wa.background_pixel =
			    screens[i].c[SWM_S_COLOR_BAR].color;
			XChangeWindowAttributes(display, r->bar_window,
			    CWBackPixel | CWBorderPixel, &wa);
		}
	bar_update();
}

void
bar_setup(struct swm_region *r)
{
	int			i;

	for (i = 0; bar_fonts[i] != NULL; i++) {
		bar_fs = XLoadQueryFont(display, bar_fonts[i]);
		if (bar_fs) {
			bar_fidx = i;
			break;
		}
	}
	if (bar_fonts[i] == NULL)
			errx(1, "couldn't load font");
	bar_height = bar_fs->ascent + bar_fs->descent + 3;

	r->bar_window = XCreateSimpleWindow(display,
	    r->s->root, X(r), Y(r), WIDTH(r) - 2, bar_height - 2,
	    1, r->s->c[SWM_S_COLOR_BAR_BORDER].color,
	    r->s->c[SWM_S_COLOR_BAR].color);
	bar_gc = XCreateGC(display, r->bar_window, 0, &bar_gcv);
	XSetFont(display, bar_gc, bar_fs->fid);
	XSelectInput(display, r->bar_window, VisibilityChangeMask);
	if (bar_enabled)
		XMapRaised(display, r->bar_window);
	DNPRINTF(SWM_D_MISC, "bar_setup: bar_window %lu\n", r->bar_window);

	if (signal(SIGALRM, bar_signal) == SIG_ERR)
		err(1, "could not install bar_signal");
	bar_refresh();
}

void
version(struct swm_region *r, union arg *args)
{
	bar_version = !bar_version;
	if (bar_version)
		snprintf(bar_vertext, sizeof bar_vertext, "Version: %s CVS: %s",
		    SWM_VERSION, cvstag);
	else
		strlcpy(bar_vertext, "", sizeof bar_vertext);
	bar_update();
}

void
client_msg(struct ws_win *win, Atom a)
{
	XClientMessageEvent	cm;

	bzero(&cm, sizeof cm);
	cm.type = ClientMessage;
	cm.window = win->id;
	cm.message_type = aprot;
	cm.format = 32;
	cm.data.l[0] = a;
	cm.data.l[1] = CurrentTime;
	XSendEvent(display, win->id, False, 0L, (XEvent *)&cm);
}

void
config_win(struct ws_win *win)
{
	XConfigureEvent		ce;

	DNPRINTF(SWM_D_MISC, "config_win: win %lu x %d y %d w %d h %d\n",
	    win->id, win->g.x, win->g.y, win->g.w, win->g.h);
	ce.type = ConfigureNotify;
	ce.display = display;
	ce.event = win->id;
	ce.window = win->id;
	ce.x = win->g.x;
	ce.y = win->g.y;
	ce.width = win->g.w;
	ce.height = win->g.h;
	ce.border_width = 1; /* XXX store this! */
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(display, win->id, False, StructureNotifyMask, (XEvent *)&ce);
}

int
count_win(struct workspace *ws, int count_transient)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (count_transient == 0 && win->floating)
			continue;
		if (count_transient == 0 && win->transient)
			continue;
		count++;
	}
	DNPRINTF(SWM_D_MISC, "count_win: %d\n", count);

	return (count);
}

void
quit(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "quit\n");
	running = 0;
}

void
unmap_all(void)
{
	struct ws_win		*win;
	int			i, j;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				XUnmapWindow(display, win->id);
}

void
fake_keypress(struct ws_win *win, int keysym, int modifiers)
{
	XKeyEvent event;

	event.display = display;	/* Ignored, but what the hell */
	event.window = win->id;
	event.root = win->s->root;
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = win->g.x;
	event.y = win->g.y;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = True;
	event.keycode = XKeysymToKeycode(display, keysym);
	event.state = modifiers;

	event.type = KeyPress;
	XSendEvent(event.display, event.window, True,
	    KeyPressMask, (XEvent *)&event);

	event.type = KeyRelease;
	XSendEvent(event.display, event.window, True,
	    KeyPressMask, (XEvent *)&event);

}

void
restart(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "restart: %s\n", start_argv[0]);

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		errx(1, "can't disable alarm");

	bar_extra_stop();
	bar_extra = 1;
	unmap_all();
	XCloseDisplay(display);
	execvp(start_argv[0], start_argv);
	fprintf(stderr, "execvp failed\n");
	perror(" failed");
	quit(NULL, NULL);
}

struct swm_region *
root_to_region(Window root)
{
	struct swm_region	*r = NULL;
	Window			rr, cr;
	int			i, x, y, wx, wy;
	unsigned int		mask;

	for (i = 0; i < ScreenCount(display); i++)
		if (screens[i].root == root)
			break;

	if (XQueryPointer(display, screens[i].root,
	    &rr, &cr, &x, &y, &wx, &wy, &mask) != False) {
		/* choose a region based on pointer location */
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (x >= X(r) && x <= X(r) + WIDTH(r) &&
			    y >= Y(r) && y <= Y(r) + HEIGHT(r))
				break;
	}

	if (r == NULL)
		r = TAILQ_FIRST(&screens[i].rl);

	return (r);
}

struct ws_win *
find_window(Window id)
{
	struct ws_win		*win;
	int			i, j;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (id == win->id)
					return (win);
	return (NULL);
}

void
spawn(struct swm_region *r, union arg *args)
{
	char			*ret;
	int			si;

	DNPRINTF(SWM_D_MISC, "spawn: %s\n", args->argv[0]);
	/*
	 * The double-fork construct avoids zombie processes and keeps the code
	 * clean from stupid signal handlers.
	 */
	if (fork() == 0) {
		if (fork() == 0) {
			if (display)
				close(ConnectionNumber(display));
			setenv("LD_PRELOAD", SWM_LIB, 1);
			if (asprintf(&ret, "%d", r->ws->idx)) {
				setenv("_SWM_WS", ret, 1);
				free(ret);
			}
			if (asprintf(&ret, "%d", getpid())) {
				setenv("_SWM_PID", ret, 1);
				free(ret);
			}
			setsid();
			/* kill stdin, mplayer, ssh-add etc. need that */
			si = open("/dev/null", O_RDONLY, 0);
			if (si == -1)
				err(1, "open /dev/null");
			if (dup2(si, 0) == -1)
				err(1, "dup2 /dev/null");
			execvp(args->argv[0], args->argv);
			fprintf(stderr, "execvp failed\n");
			perror(" failed");
		}
		exit(0);
	}
	wait(0);
}

void
spawnterm(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "spawnterm\n");

	if (term_width)
		setenv("_SWM_XTERM_FONTADJ", "", 1);
	spawn(r, args);
}

void
unfocus_win(struct ws_win *win)
{
	if (win == NULL)
		return;

	if (win->ws->r == NULL)
		return;

	grabbuttons(win, 0);
	XSetWindowBorder(display, win->id,
	    win->ws->r->s->c[SWM_S_COLOR_UNFOCUS].color);

	if (win->ws->focus == win) {
		win->ws->focus = NULL;
		win->ws->focus_prev = win;
	}
}

void
unfocus_all(void)
{
	struct ws_win		*win;
	int			i, j;

	DNPRINTF(SWM_D_FOCUS, "unfocus_all:\n");

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unfocus_win(win);
	XSync(display, False);
}

void
focus_win(struct ws_win *win)
{
	DNPRINTF(SWM_D_FOCUS, "focus_win: id: %lu\n", win ? win->id : 0);

	if (win == NULL)
		return;

	/* use big hammer to make sure it works under all use cases */
	unfocus_all();
	win->ws->focus = win;

	if (win->ws->r != NULL) {
		XSetWindowBorder(display, win->id,
		    win->ws->r->s->c[SWM_S_COLOR_FOCUS].color);
		grabbuttons(win, 1);
		XSetInputFocus(display, win->id,
		    RevertToPointerRoot, CurrentTime);
		XSync(display, False);
	}
}

void
switchws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct swm_region	*this_r, *other_r;
	struct ws_win		*win, *winfocus = NULL;
	struct workspace	*new_ws, *old_ws;

	this_r = r;
	old_ws = this_r->ws;
	new_ws = &this_r->s->ws[wsid];

	DNPRINTF(SWM_D_WS, "switchws screen[%d]:%dx%d+%d+%d: "
	    "%d -> %d\n", r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r),
	    old_ws->idx, wsid);

	if (new_ws == old_ws)
		return;

	/* get focus window */
	if (new_ws->focus)
		winfocus = new_ws->focus;
	else if (new_ws->focus_prev)
		winfocus = new_ws->focus_prev;
	else
		winfocus = TAILQ_FIRST(&new_ws->winlist);

	other_r = new_ws->r;
	if (other_r == NULL) {
		/* if the other workspace is hidden, switch windows */
		/* map new window first to prevent ugly blinking */
		old_ws->r = NULL;
		old_ws->restack = 1;

		TAILQ_FOREACH(win, &new_ws->winlist, entry)
			XMapRaised(display, win->id);

		TAILQ_FOREACH(win, &old_ws->winlist, entry)
			XUnmapWindow(display, win->id);
	} else {
		other_r->ws = old_ws;
		old_ws->r = other_r;
	}
	this_r->ws = new_ws;
	new_ws->r = this_r;

	ignore_enter = 1;
	stack();
	focus_win(winfocus);
	bar_update();
}

void
cyclews(struct swm_region *r, union arg *args)
{
	union			arg a;
	struct swm_screen	*s = r->s;

	DNPRINTF(SWM_D_WS, "cyclews id %d "
	    "in screen[%d]:%dx%d+%d+%d ws %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	a.id = r->ws->idx;
	do {
		switch (args->id) {
		case SWM_ARG_ID_CYCLEWS_UP:
			if (a.id < SWM_WS_MAX - 1)
				a.id++;
			else
				a.id = 0;
			break;
		case SWM_ARG_ID_CYCLEWS_DOWN:
			if (a.id > 0)
				a.id--;
			else
				a.id = SWM_WS_MAX - 1;
			break;
		default:
			return;
		};

		if (cycle_empty == 0 && TAILQ_EMPTY(&s->ws[a.id].winlist))
			continue;
		if (cycle_visible == 0 && s->ws[a.id].r != NULL)
			continue;

		switchws(r, &a);
	} while (a.id != r->ws->idx);
}

void
cyclescr(struct swm_region *r, union arg *args)
{
	struct swm_region	*rr;
	int			i;

	i = r->s->idx;
	switch (args->id) {
	case SWM_ARG_ID_CYCLESC_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&screens[i].rl);
		break;
	case SWM_ARG_ID_CYCLESC_DOWN:
		rr = TAILQ_PREV(r, swm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&screens[i].rl, swm_region_list);
		break;
	default:
		return;
	};
	unfocus_all();
	XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
	XWarpPointer(display, None, rr->s[i].root, 0, 0, 0, 0, rr->g.x,
	    rr->g.y + bar_enabled ? bar_height : 0);
}

void
swapwin(struct swm_region *r, union arg *args)
{
	struct ws_win		*target, *source;
	struct ws_win		*cur_focus;
	struct ws_win_list	*wl;


	DNPRINTF(SWM_D_WS, "swapwin id %d "
	    "in screen %d region %dx%d+%d+%d ws %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	cur_focus = r->ws->focus;
	if (cur_focus == NULL)
		return;

	source = cur_focus;
	wl = &source->ws->winlist;

	switch (args->id) {
	case SWM_ARG_ID_SWAPPREV:
		target = TAILQ_PREV(source, ws_win_list, entry);
		TAILQ_REMOVE(wl, cur_focus, entry);
		if (target == NULL)
			TAILQ_INSERT_TAIL(wl, source, entry);
		else
			TAILQ_INSERT_BEFORE(target, source, entry);
		break;
	case SWM_ARG_ID_SWAPNEXT:
		target = TAILQ_NEXT(source, entry);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_HEAD(wl, source, entry);
		else
			TAILQ_INSERT_AFTER(wl, target, source, entry);
		break;
	case SWM_ARG_ID_SWAPMAIN:
		target = TAILQ_FIRST(wl);
		if (target == source) {
			if (source->ws->focus_prev != NULL &&
			    source->ws->focus_prev != target)

				source = source->ws->focus_prev;
			else
				return;
                }
		source->ws->focus_prev = target;
		TAILQ_REMOVE(wl, target, entry);
		TAILQ_INSERT_BEFORE(source, target, entry);
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_HEAD(wl, source, entry);
		break;
	default:
		DNPRINTF(SWM_D_MOVE, "invalid id: %d\n", args->id);
		return;
	}

	ignore_enter = 1;
	stack();
}

void
focus(struct swm_region *r, union arg *args)
{
	struct ws_win		*winfocus, *winlostfocus;
	struct ws_win_list	*wl;
	struct ws_win		*cur_focus;

	DNPRINTF(SWM_D_FOCUS, "focus: id %d\n", args->id);

	cur_focus = r->ws->focus;
	if (cur_focus == NULL)
		return;

	wl = &cur_focus->ws->winlist;

	winlostfocus = cur_focus;

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		winfocus = TAILQ_PREV(cur_focus, ws_win_list, entry);
		if (winfocus == NULL)
			winfocus = TAILQ_LAST(wl, ws_win_list);
		break;

	case SWM_ARG_ID_FOCUSNEXT:
		winfocus = TAILQ_NEXT(cur_focus, entry);
		if (winfocus == NULL)
			winfocus = TAILQ_FIRST(wl);
		break;

	case SWM_ARG_ID_FOCUSMAIN:
		winfocus = TAILQ_FIRST(wl);
		if (winfocus == cur_focus)
			winfocus = cur_focus->ws->focus_prev;
		if (winfocus == NULL)
			return;
		break;

	default:
		return;
	}

	if (winfocus == winlostfocus || winfocus == NULL)
		return;

	XMapRaised(display, winfocus->id);
	focus_win(winfocus);
	XSync(display, False);
}

void
cycle_layout(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;
	struct ws_win		*winfocus;

	DNPRINTF(SWM_D_EVENT, "cycle_layout: workspace: %d\n", ws->idx);

	winfocus = ws->focus;

	ws->cur_layout++;
	if (ws->cur_layout->l_stack == NULL)
		ws->cur_layout = &layouts[0];

	ignore_enter = 1;
	stack();
	focus_win(winfocus);
}

void
stack_config(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	DNPRINTF(SWM_D_STACK, "stack_config for workspace %d (id %d\n",
	    args->id, ws->idx);

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != SWM_ARG_ID_STACKINIT);
		stack();
}

void
stack(void) {
	struct swm_geometry	g;
	struct swm_region	*r;
	int			i, j;

	DNPRINTF(SWM_D_STACK, "stack\n");

	for (i = 0; i < ScreenCount(display); i++) {
		j = 0;
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			DNPRINTF(SWM_D_STACK, "stacking workspace %d "
			    "(screen %d, region %d)\n", r->ws->idx, i, j++);

			/* start with screen geometry, adjust for bar */
			g = r->g;
			g.w -= 2;
			g.h -= 2;
			if (bar_enabled) {
				g.y += bar_height;
				g.h -= bar_height;
			}

			r->ws->restack = 0;
			r->ws->cur_layout->l_stack(r->ws, &g);
		}
	}
	if (font_adjusted)
		font_adjusted--;
	XSync(display, False);
}

void
stack_floater(struct ws_win *win, struct swm_region *r)
{
	unsigned int		mask;
	XWindowChanges		wc;

	bzero(&wc, sizeof wc);
	mask = CWX | CWY | CWBorderWidth | CWWidth | CWHeight;
	if ((win->quirks & SWM_Q_FULLSCREEN) && (win->g.w == WIDTH(r)) &&
	    (win->g.h == HEIGHT(r)))
		wc.border_width = 0;
	else
		wc.border_width = 1;
	if (win->transient && (win->quirks & SWM_Q_TRANSSZ)) {
		win->g.w = (double)WIDTH(r) * dialog_ratio;
		win->g.h = (double)HEIGHT(r) * dialog_ratio;
	}
	wc.width = win->g.w;
	wc.height = win->g.h;
	if (win->manual) {
		wc.x = win->g.x;
		wc.y = win->g.y;
	} else {
		wc.x = (WIDTH(r) - win->g.w) / 2;
		wc.y = (HEIGHT(r) - win->g.h) / 2;
	}

	/* adjust for region */
	wc.x += r->g.x;
	wc.y += r->g.y;

	DNPRINTF(SWM_D_STACK, "stack_floater: win %lu x %d y %d w %d h %d\n",
	    win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
}

/*
 * Send keystrokes to terminal to decrease/increase the font size as the
 * window size changes.
 */
void
adjust_font(struct ws_win *win)
{
	if (!(win->quirks & SWM_Q_XTERM_FONTADJ) ||
	    win->floating || win->transient)
		return;

	if (win->sh.width_inc && win->last_inc != win->sh.width_inc &&
	    win->g.w / win->sh.width_inc < term_width &&
	    win->font_steps < SWM_MAX_FONT_STEPS) {
		win->font_size_boundary[win->font_steps] =
		    (win->sh.width_inc * term_width) + win->sh.base_width;
		win->font_steps++;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Subtract, ShiftMask);
	} else if (win->font_steps && win->last_inc != win->sh.width_inc &&
	    win->g.w > win->font_size_boundary[win->font_steps - 1]) {
		win->font_steps--;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Add, ShiftMask);
	}
}

#define SWAPXY(g)	do {				\
	int tmp;					\
	tmp = (g)->y; (g)->y = (g)->x; (g)->x = tmp;	\
	tmp = (g)->h; (g)->h = (g)->w; (g)->w = tmp;	\
} while (0)
void
stack_master(struct workspace *ws, struct swm_geometry *g, int rot, int flip)
{
	XWindowChanges		wc;
	struct swm_geometry	win_g, r_g = *g;
	struct ws_win		*win;
	int			i, j, s, stacks;
	int			w_inc = 1, h_inc, w_base = 1, h_base;
	int			hrh, extra = 0, h_slice, last_h = 0;
	int			split, colno, winno, mwin, msize, mscale;
	int			remain, missing, v_slice;
	unsigned int		mask;

	DNPRINTF(SWM_D_STACK, "stack_master: workspace: %d\n rot=%s flip=%s",
	    ws->idx, rot ? "yes" : "no", flip ? "yes" : "no");

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry)
		if (win->transient == 0 && win->floating == 0)
			break;

	if (win == NULL)
		goto notiles;

	if (rot) {
		w_inc = win->sh.width_inc;
		w_base = win->sh.base_width;
		mwin = ws->l_state.horizontal_mwin;
		mscale = ws->l_state.horizontal_msize;
		stacks = ws->l_state.horizontal_stacks;
		SWAPXY(&r_g);
	} else {
		w_inc = win->sh.height_inc;
		w_base = win->sh.base_height;
		mwin = ws->l_state.vertical_mwin;
		mscale = ws->l_state.vertical_msize;
		stacks = ws->l_state.vertical_stacks;
	}
	win_g = r_g;

	if (stacks > winno - mwin)
		stacks = winno - mwin;
	if (stacks < 1)
		stacks = 1;

	h_slice = r_g.h / SWM_H_SLICE;
	if (mwin && winno > mwin) {
		v_slice = r_g.w / SWM_V_SLICE;

		split = mwin;
		colno = split;
		win_g.w = v_slice * mscale;

		if (w_inc > 1 && w_inc < v_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.w - w_base) % w_inc;
			missing = w_inc - remain;
			win_g.w -= remain;
			extra += remain;
		}

		msize = win_g.w;
		if (flip)
			win_g.x += r_g.w - msize;
	} else {
		msize = -2;
		colno = split = winno / stacks;
		win_g.w = ((r_g.w - (stacks * 2) + 2) / stacks);
	}
	hrh = r_g.h / colno;
	extra = r_g.h - (colno * hrh);
	win_g.h = hrh - 2;

	/*  stack all the tiled windows */
	i = j = 0, s = stacks;
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient != 0 || win->floating != 0)
			continue;

		if (split && i == split) {
			colno = (winno - mwin) / stacks;
			if (s <= (winno - mwin) % stacks)
				colno++;
			split = split + colno;
			hrh = (r_g.h / colno);
			extra = r_g.h - (colno * hrh);
			if (flip)
				win_g.x = r_g.x;
			else
				win_g.x += win_g.w + 2;
			win_g.w = (r_g.w - msize - (stacks * 2)) / stacks;
			if (s == 1)
				win_g.w += (r_g.w - msize - (stacks * 2)) %
				    stacks;
			s--;
			j = 0;
		}
		win_g.h = hrh - 2;
		if (rot) {
			h_inc = win->sh.width_inc;
			h_base = win->sh.base_width;
		} else {
			h_inc =	win->sh.height_inc;
			h_base = win->sh.base_height;
		}
		if (j == colno - 1) {
			win_g.h = hrh + extra;
		} else if (h_inc > 1 && h_inc < h_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.h - h_base) % h_inc;
			missing = h_inc - remain;

			if (missing <= extra || j == 0) {
				extra -= missing;
				win_g.h += missing;
			} else {
				win_g.h -= remain;
				extra += remain;
			}
		}

		if (j == 0)
			win_g.y = r_g.y;
		else
			win_g.y += last_h + 2;

		bzero(&wc, sizeof wc);
		wc.border_width = 1;
		if (rot) {
			win->g.x = wc.x = win_g.y;
			win->g.y = wc.y = win_g.x;
			win->g.w = wc.width = win_g.h;
			win->g.h = wc.height = win_g.w;
		} else {
			win->g.x = wc.x = win_g.x;
			win->g.y = wc.y = win_g.y;
			win->g.w = wc.width = win_g.w;
			win->g.h = wc.height = win_g.h;
		}
		adjust_font(win);
		mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
		XConfigureWindow(display, win->id, mask, &wc);
		XMapRaised(display, win->id);

		last_h = win_g.h;
		i++;
		j++;
	}

 notiles:
	/* now, stack all the floaters and transients */
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient == 0 && win->floating == 0)
			continue;

		stack_floater(win, ws->r);
		XMapRaised(display, win->id);
	}
}

void
vertical_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "vertical_resize: workspace: %d\n", ws->idx);

	switch (id) {
	case SWM_ARG_ID_STACKRESET:
	case SWM_ARG_ID_STACKINIT:
		ws->l_state.vertical_msize = SWM_V_SLICE / 2;
		ws->l_state.vertical_mwin = 1;
		ws->l_state.vertical_stacks = 1;
		break;
	case SWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.vertical_msize > 1)
			ws->l_state.vertical_msize--;
		break;
	case SWM_ARG_ID_MASTERGROW:
		if (ws->l_state.vertical_msize < SWM_V_SLICE - 1)
			ws->l_state.vertical_msize++;
		break;
	case SWM_ARG_ID_MASTERADD:
		ws->l_state.vertical_mwin++;
		break;
	case SWM_ARG_ID_MASTERDEL:
		if (ws->l_state.vertical_mwin > 0)
			ws->l_state.vertical_mwin--;
		break;
	case SWM_ARG_ID_STACKINC:
		ws->l_state.vertical_stacks++;
		break;
	case SWM_ARG_ID_STACKDEC:
		if (ws->l_state.vertical_stacks > 1)
			ws->l_state.vertical_stacks--;
		break;
	default:
		return;
	}
}

void
vertical_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "vertical_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, 0);
}

void
horizontal_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "horizontal_config: workspace: %d\n", ws->idx);

	switch (id) {
	case SWM_ARG_ID_STACKRESET:
	case SWM_ARG_ID_STACKINIT:
		ws->l_state.horizontal_mwin = 1;
		ws->l_state.horizontal_msize = SWM_H_SLICE / 2;
		ws->l_state.horizontal_stacks = 1;
		break;
	case SWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.horizontal_msize > 1)
			ws->l_state.horizontal_msize--;
		break;
	case SWM_ARG_ID_MASTERGROW:
		if (ws->l_state.horizontal_msize < SWM_H_SLICE - 1)
			ws->l_state.horizontal_msize++;
		break;
	case SWM_ARG_ID_MASTERADD:
		ws->l_state.horizontal_mwin++;
		break;
	case SWM_ARG_ID_MASTERDEL:
		if (ws->l_state.horizontal_mwin > 0)
			ws->l_state.horizontal_mwin--;
		break;
	case SWM_ARG_ID_STACKINC:
		ws->l_state.horizontal_stacks++;
		break;
	case SWM_ARG_ID_STACKDEC:
		if (ws->l_state.horizontal_stacks > 1)
			ws->l_state.horizontal_stacks--;
		break;
	default:
		return;
	}
}

void
horizontal_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "vertical_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, 0);
}

/* fullscreen view */
void
max_stack(struct workspace *ws, struct swm_geometry *g)
{
	XWindowChanges		wc;
	struct swm_geometry	gg = *g;
	struct ws_win		*win;
	unsigned int		mask;
	int			winno;

	/* XXX this function needs to be rewritten it sucks crap */

	DNPRINTF(SWM_D_STACK, "max_stack: workspace: %d\n", ws->idx);

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient != 0 || win->floating != 0) {
			if (win == ws->focus) {
				stack_floater(win, ws->r);
				XMapRaised(display, win->id);
				focus_win(win); /* override */
			}
		} else {
			bzero(&wc, sizeof wc);
			wc.border_width = 1;
			win->g.x = wc.x = gg.x;
			win->g.y = wc.y = gg.y;
			win->g.w = wc.width = gg.w;
			win->g.h = wc.height = gg.h;
			mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
			XConfigureWindow(display, win->id, mask, &wc);
		}
	}
}

void
send_to_ws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win = r->ws->focus, *winfocus = NULL;
	struct workspace	*ws, *nws;
	Atom			ws_idx_atom = 0;
	unsigned char		ws_idx_str[SWM_PROPLEN];

	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MOVE, "send_to_ws: win: %lu\n", win->id);

	ws = win->ws;
	nws = &win->s->ws[wsid];

	/* find a window to focus */
	winfocus = TAILQ_PREV(win, ws_win_list, entry);
	if (TAILQ_FIRST(&ws->winlist) == win)
		winfocus = TAILQ_NEXT(win, entry);
	else {
		winfocus = TAILQ_PREV(ws->focus, ws_win_list, entry);
		if (winfocus == NULL)
			winfocus = TAILQ_LAST(&ws->winlist, ws_win_list);
	}
	/* out of windows in ws so focus on nws instead */
	if (winfocus == NULL)
		winfocus = win;

	XUnmapWindow(display, win->id);

	TAILQ_REMOVE(&ws->winlist, win, entry);

	TAILQ_INSERT_TAIL(&nws->winlist, win, entry);
	win->ws = nws;

	/* Try to update the window's workspace property */
	ws_idx_atom = XInternAtom(display, "_SWM_WS", False);
	if (ws_idx_atom &&
	    snprintf(ws_idx_str, SWM_PROPLEN, "%d", nws->idx) < SWM_PROPLEN) {
		DNPRINTF(SWM_D_PROP, "setting property _SWM_WS to %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, SWM_PROPLEN);
	}

	if (count_win(nws, 1) == 1)
		nws->focus = win;
	ws->restack = 1;
	nws->restack = 1;

	stack();
	focus_win(winfocus);
}

void
wkill(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "wkill %d\n", args->id);

	if(r->ws->focus == NULL)
		return;

	if (args->id == SWM_ARG_ID_KILLWINDOW)
		XKillClient(display, r->ws->focus->id);
	else
		if (r->ws->focus->can_delete)
			client_msg(r->ws->focus, adelete);
}

void
floating_toggle(struct swm_region *r, union arg *args)
{
	struct ws_win	*win = r->ws->focus;

	if (win == NULL)
		return;

	win->floating = !win->floating;
	win->manual = 0;
	stack();
	focus_win(win);
}

void
resize_window(struct ws_win *win, int center)
{
	unsigned int		mask;
	XWindowChanges		wc;
	struct swm_region	*r;

	r = root_to_region(win->wa.root);
	bzero(&wc, sizeof wc);
	mask = CWBorderWidth | CWWidth | CWHeight;
	wc.border_width = 1;
	wc.width = win->g.w;
	wc.height = win->g.h;
	if (center == SWM_ARG_ID_CENTER) {
		wc.x = (WIDTH(r) - win->g.w) / 2;
		wc.y = (HEIGHT(r) - win->g.h) / 2;
		mask |= CWX | CWY;
	}

	DNPRINTF(SWM_D_STACK, "resize_window: win %lu x %d y %d w %d h %d\n",
	    win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
	config_win(win);
}

void
resize(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;

	DNPRINTF(SWM_D_MOUSE, "resize: win %lu floating %d trans %d\n",
	    win->id, win->floating, win->transient);

	if (!(win->transient != 0 || win->floating != 0))
		return;

	if (XGrabPointer(display, win->id, False, MOUSEMASK, GrabModeAsync,
	    GrabModeAsync, None, None /* cursor */, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(display, None, win->id, 0, 0, 0, 0, win->g.w, win->g.h);
	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if (ev.xmotion.x <= 1)
				ev.xmotion.x = 1;
			if (ev.xmotion.y <= 1)
				ev.xmotion.y = 1;
			win->g.w = ev.xmotion.x;
			win->g.h = ev.xmotion.y;

			/* not free, don't sync more than 60 times / second */
			if ((ev.xmotion.time - time) > (1000 / 60) ) {
				time = ev.xmotion.time;
				XSync(display, False);
				resize_window(win, args->id);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		XSync(display, False);
		resize_window(win, args->id);
	}
	XWarpPointer(display, None, win->id, 0, 0, 0, 0, win->g.w - 1,
	    win->g.h - 1);
	XUngrabPointer(display, CurrentTime);

	/* drain events */
	while (XCheckMaskEvent(display, EnterWindowMask, &ev));
}

void
move_window(struct ws_win *win)
{
	unsigned int		mask;
	XWindowChanges		wc;
	struct swm_region	*r;

	r = root_to_region(win->wa.root);
	bzero(&wc, sizeof wc);
	mask = CWX | CWY;
	wc.x = win->g.x;
	wc.y = win->g.y;

	DNPRINTF(SWM_D_STACK, "move_window: win %lu x %d y %d w %d h %d\n",
	    win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
	config_win(win);
}

void
move(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	int			restack = 0;

	DNPRINTF(SWM_D_MOUSE, "move: win %lu floating %d trans %d\n",
	    win->id, win->floating, win->transient);

	if (win->floating == 0) {
		win->floating = 1;
		win->manual = 1;
		restack = 1;
	}

	if (XGrabPointer(display, win->id, False, MOUSEMASK, GrabModeAsync,
	    GrabModeAsync, None, None /* cursor */, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(display, None, win->id, 0, 0, 0, 0, 0, 0);
	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			win->g.x = ev.xmotion.x_root;
			win->g.y = ev.xmotion.y_root;

			/* not free, don't sync more than 60 times / second */
			if ((ev.xmotion.time - time) > (1000 / 60) ) {
				time = ev.xmotion.time;
				XSync(display, False);
				move_window(win);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		XSync(display, False);
		move_window(win);
	}
	XWarpPointer(display, None, win->id, 0, 0, 0, 0, 0, 0);
	XUngrabPointer(display, CurrentTime);
	if (restack)
		stack();

	/* drain events */
	while (XCheckMaskEvent(display, EnterWindowMask, &ev));
}

/* key definitions */
void dummykeyfunc(struct swm_region *r, union arg *args) {};
void legacyfunc(struct swm_region *r, union arg *args) {};

struct keyfunc {
	char			name[SWM_FUNCNAME_LEN];
	void			(*func)(struct swm_region *r, union arg *);
	union arg		args;
} keyfuncs[kf_invalid + 1] = {
	/* name			function	argument */
	{ "cycle_layout",	cycle_layout,	{0} },
	{ "stack_reset",	stack_config,	{.id = SWM_ARG_ID_STACKRESET} },
	{ "master_shrink",	stack_config,	{.id = SWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	{.id = SWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	{.id = SWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	{.id = SWM_ARG_ID_MASTERDEL} },
	{ "stack_inc",		stack_config,	{.id = SWM_ARG_ID_STACKINC} },
	{ "stack_dec",		stack_config,	{.id = SWM_ARG_ID_STACKDEC} },
	{ "swap_main",		swapwin,	{.id = SWM_ARG_ID_SWAPMAIN} },
	{ "focus_next",		focus,		{.id = SWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		{.id = SWM_ARG_ID_FOCUSPREV} },
	{ "swap_next",		swapwin,	{.id = SWM_ARG_ID_SWAPNEXT} },
	{ "swap_prev",		swapwin,	{.id = SWM_ARG_ID_SWAPPREV} },
	{ "spawn_term",		spawnterm,	{.argv = spawn_term} },
	{ "spawn_menu",		legacyfunc,	{0} },
	{ "quit",		quit,		{0} },
	{ "restart",		restart,	{0} },
	{ "focus_main",		focus,		{.id = SWM_ARG_ID_FOCUSMAIN} },
	{ "ws_1",		switchws,	{.id = 0} },
	{ "ws_2",		switchws,	{.id = 1} },
	{ "ws_3",		switchws,	{.id = 2} },
	{ "ws_4",		switchws,	{.id = 3} },
	{ "ws_5",		switchws,	{.id = 4} },
	{ "ws_6",		switchws,	{.id = 5} },
	{ "ws_7",		switchws,	{.id = 6} },
	{ "ws_8",		switchws,	{.id = 7} },
	{ "ws_9",		switchws,	{.id = 8} },
	{ "ws_10",		switchws,	{.id = 9} },
	{ "ws_next",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP} },
	{ "ws_prev",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN} },
	{ "screen_next",	cyclescr,	{.id = SWM_ARG_ID_CYCLESC_UP} },
	{ "screen_prev",	cyclescr,	{.id = SWM_ARG_ID_CYCLESC_DOWN} },
	{ "mvws_1",		send_to_ws,	{.id = 0} },
	{ "mvws_2",		send_to_ws,	{.id = 1} },
	{ "mvws_3",		send_to_ws,	{.id = 2} },
	{ "mvws_4",		send_to_ws,	{.id = 3} },
	{ "mvws_5",		send_to_ws,	{.id = 4} },
	{ "mvws_6",		send_to_ws,	{.id = 5} },
	{ "mvws_7",		send_to_ws,	{.id = 6} },
	{ "mvws_8",		send_to_ws,	{.id = 7} },
	{ "mvws_9",		send_to_ws,	{.id = 8} },
	{ "mvws_10",		send_to_ws,	{.id = 9} },
	{ "bar_toggle",		bar_toggle,	{0} },
	{ "wind_kill",		wkill,		{.id = SWM_ARG_ID_KILLWINDOW} },
	{ "wind_del",		wkill,		{.id = SWM_ARG_ID_DELETEWINDOW} },
	{ "screenshot_all",	legacyfunc,	{0} },
	{ "screenshot_wind",	legacyfunc,	{0} },
	{ "float_toggle",	floating_toggle,{0} },
	{ "version",		version,	{0} },
	{ "spawn_lock",		legacyfunc,	{0} },
	{ "spawn_initscr",	legacyfunc,	{0} },
	{ "spawn_custom",	dummykeyfunc,	{0} },
	{ "invalid key func",	NULL,		{0} },
};
struct key {
	unsigned int		mod;
	KeySym			keysym;
	enum keyfuncid		funcid;
	char			*spawn_name;
};
int				keys_size = 0, keys_length = 0;
struct key			*keys = NULL;

/* mouse */
enum { client_click, root_click };
struct button {
	unsigned int		action;
	unsigned int		mask;
	unsigned int		button;
	void			(*func)(struct ws_win *, union arg *);
	union arg		args;
} buttons[] = {
	  /* action	key		mouse button	func	args */
	{ client_click,	MODKEY,		Button3,	resize,	{.id = SWM_ARG_ID_DONTCENTER} },
	{ client_click,	MODKEY | ShiftMask, Button3,	resize,	{.id = SWM_ARG_ID_CENTER} },
	{ client_click,	MODKEY,		Button1,	move,	{0} },
};

void
update_modkey(unsigned int mod)
{
	int			i;

	mod_key = mod;
	for (i = 0; i < keys_length; i++)
		if (keys[i].mod & ShiftMask)
			keys[i].mod = mod | ShiftMask;
		else
			keys[i].mod = mod;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].mask & ShiftMask)
			buttons[i].mask = mod | ShiftMask;
		else
			buttons[i].mask = mod;
}

/* spawn */
struct spawn_prog {
	char			*name;
	int			argc;
	char			**argv;
};

int				spawns_size = 0, spawns_length = 0;
struct spawn_prog		*spawns = NULL;

void
spawn_custom(struct swm_region *r, union arg *args, char *spawn_name)
{
	union arg		a;
	struct spawn_prog	*prog = NULL;
	int			i;
	char			*ap, **real_args;

	DNPRINTF(SWM_D_SPAWN, "spawn_custom %s\n", spawn_name);

	/* find program */
	for (i = 0; i < spawns_length; i++) {
		if (!strcasecmp(spawn_name, spawns[i].name))
			prog = &spawns[i];
	}
	if (prog == NULL) {
		fprintf(stderr, "spawn_custom: program %s not found\n",
		    spawn_name);
		return;
	}

	/* make room for expanded args */
	if ((real_args = calloc(prog->argc + 1, sizeof(char *))) == NULL)
		err(1, "spawn_custom: calloc real_args");

	/* expand spawn_args into real_args */
	for (i = 0; i < prog->argc; i++) {
		ap = prog->argv[i];
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: raw arg = %s\n", ap);
		if (!strcasecmp(ap, "$bar_border")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_BORDER].name))
			    == NULL)
				err(1,  "spawn_custom border color");
		} else if (!strcasecmp(ap, "$bar_color")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_BAR].name))
			    == NULL)
				err(1, "spawn_custom bar color");
		} else if (!strcasecmp(ap, "$bar_font")) {
			if ((real_args[i] = strdup(bar_fonts[bar_fidx]))
			    == NULL)
				err(1, "spawn_custom bar fonts");
		} else if (!strcasecmp(ap, "$bar_font_color")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_FONT].name))
			    == NULL)
				err(1, "spawn_custom color font");
		} else if (!strcasecmp(ap, "$color_focus")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_FOCUS].name))
			    == NULL)
				err(1, "spawn_custom color focus");
		} else if (!strcasecmp(ap, "$color_unfocus")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_UNFOCUS].name))
			    == NULL)
				err(1, "spawn_custom color unfocus");
		} else {
			/* no match --> copy as is */
			if ((real_args[i] = strdup(ap)) == NULL)
				err(1, "spawn_custom strdup(ap)");
		}
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: cooked arg = %s\n",
		    real_args[i]);
	}

#ifdef SWM_DEBUG
	if ((swm_debug & SWM_D_SPAWN) != 0) {
		fprintf(stderr, "spawn_custom: result = ");
		for (i = 0; i < prog->argc; i++)
			fprintf(stderr, "\"%s\" ", real_args[i]);
		fprintf(stderr, "\n");
	}
#endif

	a.argv = real_args;
	spawn(r, &a);
	for (i = 0; i < prog->argc; i++)
		free(real_args[i]);
	free(real_args);
}

void
setspawn(struct spawn_prog *prog)
{
	int			i, j;

	if (prog == NULL || prog->name == NULL)
		return;

	/* find existing */
	for (i = 0; i < spawns_length; i++) {
		if (!strcmp(spawns[i].name, prog->name)) {
			/* found */
			if (prog->argv == NULL) {
				/* delete */
				DNPRINTF(SWM_D_SPAWN,
				    "setspawn: delete #%d %s\n",
				    i, spawns[i].name);
				free(spawns[i].name);
				for (j = 0; j < spawns[i].argc; j++)
					free(spawns[i].argv[j]);
				free(spawns[i].argv);
				j = spawns_length - 1;
				if (i < j)
					spawns[i] = spawns[j];
				spawns_length--;
				free(prog->name);
			} else {
				/* replace */
				DNPRINTF(SWM_D_SPAWN,
				    "setspawn: replace #%d %s\n",
				    i, spawns[i].name);
				free(spawns[i].name);
				for (j = 0; j < spawns[i].argc; j++)
					free(spawns[i].argv[j]);
				free(spawns[i].argv);
				spawns[i] = *prog;
			}
			/* found case handled */
			free(prog);
			return;
		}
	}

	if (prog->argv == NULL) {
		fprintf(stderr,
		    "error: setspawn: cannot find program %s", prog->name);
		free(prog);
		return;
	}

	/* not found: add */
	if (spawns_size == 0 || spawns == NULL) {
		spawns_size = 4;
		DNPRINTF(SWM_D_SPAWN, "setspawn: init list %d\n", spawns_size);
		spawns = malloc((size_t)spawns_size *
		    sizeof(struct spawn_prog));
		if (spawns == NULL) {
			fprintf(stderr, "setspawn: malloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	} else if (spawns_length == spawns_size) {
		spawns_size *= 2;
		DNPRINTF(SWM_D_SPAWN, "setspawn: grow list %d\n", spawns_size);
		spawns = realloc(spawns, (size_t)spawns_size *
		    sizeof(struct spawn_prog));
		if (spawns == NULL) {
			fprintf(stderr, "setspawn: realloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	}

	if (spawns_length < spawns_size) {
		DNPRINTF(SWM_D_SPAWN, "setspawn: add #%d %s\n",
		    spawns_length, prog->name);
		i = spawns_length++;
		spawns[i] = *prog;
	} else {
		fprintf(stderr, "spawns array problem?\n");
		if (spawns == NULL) {
			fprintf(stderr, "spawns array is NULL!\n");
			quit(NULL, NULL);
		}
	}
	free(prog);
}

int
setconfspawn(char *selector, char *value, int flags)
{
	struct spawn_prog	*prog;
	char			*vp, *cp, *word;

	DNPRINTF(SWM_D_SPAWN, "setconfspawn: [%s] [%s]\n", selector, value);
	if ((prog = calloc(1, sizeof *prog)) == NULL)
		err(1, "setconfspawn: calloc prog");
	prog->name = strdup(selector);
	if (prog->name == NULL)
		err(1, "setconfspawn prog->name");
	if ((cp = vp = strdup(value)) == NULL)
		err(1, "setconfspawn: strdup(value) ");
	while ((word = strsep(&cp, " \t")) != NULL) {
		DNPRINTF(SWM_D_SPAWN, "setconfspawn: arg [%s]\n", word);
		if (cp)
			cp += (long)strspn(cp, " \t");
		if (strlen(word) > 0) {
			prog->argc++;
			prog->argv = realloc(prog->argv,
			    prog->argc * sizeof(char *));
			if ((prog->argv[prog->argc - 1] = strdup(word)) == NULL)
				err(1, "setconfspawn: strdup");
		}
	}
	free(vp);

	setspawn(prog);

	DNPRINTF(SWM_D_SPAWN, "setconfspawn: done\n");
	return (0);
}

void
setup_spawn(void)
{
	setconfspawn("term",		"xterm",		0);
	setconfspawn("screenshot_all",	"screenshot.sh full",	0);
	setconfspawn("screenshot_wind",	"screenshot.sh window",	0);
	setconfspawn("lock",		"xlock",		0);
	setconfspawn("initscr",		"initscreen.sh",	0);
	setconfspawn("menu",		"dmenu_run"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);
}

/* key bindings */
#define SWM_MODNAME_SIZE	32
#define	SWM_KEY_WS		"\n+ \t"
int
parsekeys(char *keystr, unsigned int currmod, unsigned int *mod, KeySym *ks)
{
	char			*cp, *name;
	KeySym			uks;
	DNPRINTF(SWM_D_KEY, "parsekeys: enter [%s]\n", keystr);
	if (mod == NULL || ks == NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: no mod or key vars\n");
		return (1);
	}
	if (keystr == NULL || strlen(keystr) == 0) {
		DNPRINTF(SWM_D_KEY, "parsekeys: no keystr\n");
		return (1);
	}
	cp = keystr;
	*ks = NoSymbol;
	*mod = 0;
	while ((name = strsep(&cp, SWM_KEY_WS)) != NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: key [%s]\n", name);
		if (cp)
			cp += (long)strspn(cp, SWM_KEY_WS);
		if (strncasecmp(name, "MOD", SWM_MODNAME_SIZE) == 0)
			*mod |= currmod;
		else if (!strncasecmp(name, "Mod1", SWM_MODNAME_SIZE))
			*mod |= Mod1Mask;
		else if (!strncasecmp(name, "Mod2", SWM_MODNAME_SIZE))
			*mod += Mod2Mask;
		else if (!strncmp(name, "Mod3", SWM_MODNAME_SIZE))
			*mod |= Mod3Mask;
		else if (!strncmp(name, "Mod4", SWM_MODNAME_SIZE))
			*mod |= Mod4Mask;
		else if (strncasecmp(name, "SHIFT", SWM_MODNAME_SIZE) == 0)
			*mod |= ShiftMask;
		else if (strncasecmp(name, "CONTROL", SWM_MODNAME_SIZE) == 0)
			*mod |= ControlMask;
		else {
			*ks = XStringToKeysym(name);
			XConvertCase(*ks, ks, &uks);
			if (ks == NoSymbol) {
				DNPRINTF(SWM_D_KEY,
				    "parsekeys: invalid key %s\n",
				    name);
				return (1);
			}
		}
	}
	DNPRINTF(SWM_D_KEY, "parsekeys: leave ok\n");
	return (0);
}
char *
strdupsafe(char *str)
{
	if (str == NULL)
		return (NULL);
	else
		return (strdup(str));
}
void
setkeybinding(unsigned int mod, KeySym ks, enum keyfuncid kfid, char *spawn_name)
{
	int			i, j;
	DNPRINTF(SWM_D_KEY, "setkeybinding: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);
	/* find existing */
	for (i = 0; i < keys_length; i++) {
		if (keys[i].mod == mod && keys[i].keysym == ks) {
			if (kfid == kf_invalid) {
				/* found: delete */
				DNPRINTF(SWM_D_KEY,
				    "setkeybinding: delete #%d %s\n",
				    i, keyfuncs[keys[i].funcid].name);
				free(keys[i].spawn_name);
				j = keys_length - 1;
				if (i < j)
					keys[i] = keys[j];
				keys_length--;
				DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
				return;
			} else {
				/* found: replace */
				DNPRINTF(SWM_D_KEY,
				    "setkeybinding: replace #%d %s %s\n",
				    i, keyfuncs[keys[i].funcid].name,
				    spawn_name);
				free(keys[i].spawn_name);
				keys[i].mod = mod;
				keys[i].keysym = ks;
				keys[i].funcid = kfid;
				keys[i].spawn_name = strdupsafe(spawn_name);
				DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
				return;
			}
		}
	}
	if (kfid == kf_invalid) {
		fprintf(stderr,
		    "error: setkeybinding: cannot find mod/key combination");
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}
	/* not found: add */
	if (keys_size == 0 || keys == NULL) {
		keys_size = 4;
		DNPRINTF(SWM_D_KEY, "setkeybinding: init list %d\n", keys_size);
		keys = malloc((size_t)keys_size * sizeof(struct key));
		if (!keys) {
			fprintf(stderr, "malloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	} else if (keys_length == keys_size) {
		keys_size *= 2;
		DNPRINTF(SWM_D_KEY, "setkeybinding: grow list %d\n", keys_size);
		keys = realloc(keys, (size_t)keys_size * sizeof(struct key));
		if (!keys) {
			fprintf(stderr, "realloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	}
	if (keys_length < keys_size) {
		j = keys_length++;
		DNPRINTF(SWM_D_KEY, "setkeybinding: add #%d %s %s\n",
		    j, keyfuncs[kfid].name, spawn_name);
		keys[j].mod = mod;
		keys[j].keysym = ks;
		keys[j].funcid = kfid;
		keys[j].spawn_name = strdupsafe(spawn_name);
	} else {
		fprintf(stderr, "keys array problem?\n");
		if (!keys) {
			fprintf(stderr, "keys array problem\n");
			quit(NULL, NULL);
		}
	}
	DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
}
int
setconfbinding(char *selector, char *value, int flags)
{
	enum keyfuncid		kfid;
	unsigned int		mod;
	KeySym			ks;
	int			i;
	DNPRINTF(SWM_D_KEY, "setconfbinding: enter\n");
	if (selector == NULL) {
		DNPRINTF(SWM_D_KEY, "setconfbinding: unbind %s\n", value);
		if (parsekeys(value, mod_key, &mod, &ks) == 0) {
			kfid = kf_invalid;
			setkeybinding(mod, ks, kfid, NULL);
			return (0);
		} else
			return (1);
	}
	/* search by key function name */
	for (kfid = 0; kfid < kf_invalid; (kfid)++) {
		if (strncasecmp(selector, keyfuncs[kfid].name,
		    SWM_FUNCNAME_LEN) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kfid, NULL);
				return (0);
			} else
				return (1);
		}
	}
	/* search by custom spawn name */
	for (i = 0; i < spawns_length; i++) {
		if (strcasecmp(selector, spawns[i].name) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kf_spawn_custom,
				    spawns[i].name);
				return (0);
			} else
				return (1);
		}
	}
	DNPRINTF(SWM_D_KEY, "setconfbinding: no match\n");
	return (1);
}
void
setup_keys(void)
{
	setkeybinding(MODKEY,		XK_space,	kf_cycle_layout,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_space,	kf_stack_reset,	NULL);
	setkeybinding(MODKEY,		XK_h,		kf_master_shrink,NULL);
	setkeybinding(MODKEY,		XK_l,		kf_master_grow,	NULL);
	setkeybinding(MODKEY,		XK_comma,	kf_master_add,	NULL);
	setkeybinding(MODKEY,		XK_period,	kf_master_del,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_comma,	kf_stack_inc,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_period,	kf_stack_dec,	NULL);
	setkeybinding(MODKEY,		XK_Return,	kf_swap_main,	NULL);
	setkeybinding(MODKEY,		XK_j,		kf_focus_next,	NULL);
	setkeybinding(MODKEY,		XK_k,		kf_focus_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_j,		kf_swap_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_k,		kf_swap_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Return,	kf_spawn_term,	NULL);
	setkeybinding(MODKEY,		XK_p,		kf_spawn_custom,	"menu");
	setkeybinding(MODKEY|ShiftMask,	XK_q,		kf_quit,	NULL);
	setkeybinding(MODKEY,		XK_q,		kf_restart,	NULL);
	setkeybinding(MODKEY,		XK_m,		kf_focus_main,	NULL);
	setkeybinding(MODKEY,		XK_1,		kf_ws_1,	NULL);
	setkeybinding(MODKEY,		XK_2,		kf_ws_2,	NULL);
	setkeybinding(MODKEY,		XK_3,		kf_ws_3,	NULL);
	setkeybinding(MODKEY,		XK_4,		kf_ws_4,	NULL);
	setkeybinding(MODKEY,		XK_5,		kf_ws_5,	NULL);
	setkeybinding(MODKEY,		XK_6,		kf_ws_6,	NULL);
	setkeybinding(MODKEY,		XK_7,		kf_ws_7,	NULL);
	setkeybinding(MODKEY,		XK_8,		kf_ws_8,	NULL);
	setkeybinding(MODKEY,		XK_9,		kf_ws_9,	NULL);
	setkeybinding(MODKEY,		XK_0,		kf_ws_10,	NULL);
	setkeybinding(MODKEY,		XK_Right,	kf_ws_next,	NULL);
	setkeybinding(MODKEY,		XK_Left,	kf_ws_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Right,	kf_screen_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Left,	kf_screen_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_1,		kf_mvws_1,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_2,		kf_mvws_2,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_3,		kf_mvws_3,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_4,		kf_mvws_4,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_5,		kf_mvws_5,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_6,		kf_mvws_6,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_7,		kf_mvws_7,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_8,		kf_mvws_8,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_9,		kf_mvws_9,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_0,		kf_mvws_10,	NULL);
	setkeybinding(MODKEY,		XK_b,		kf_bar_toggle,	NULL);
	setkeybinding(MODKEY,		XK_Tab,		kf_focus_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Tab,		kf_focus_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_x,		kf_wind_kill,	NULL);
	setkeybinding(MODKEY,		XK_x,		kf_wind_del,	NULL);
	setkeybinding(MODKEY,		XK_s,		kf_spawn_custom,	"screenshot_all");
	setkeybinding(MODKEY|ShiftMask,	XK_s,		kf_spawn_custom,	"screenshot_wind");
	setkeybinding(MODKEY,		XK_t,		kf_float_toggle,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_v,		kf_version,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Delete,	kf_spawn_custom,	"lock");
	setkeybinding(MODKEY|ShiftMask,	XK_i,		kf_spawn_custom,	"initscr");
}
void
updatenumlockmask(void)
{
	unsigned int		i, j;
	XModifierKeymap		*modmap;

	DNPRINTF(SWM_D_MISC, "updatenumlockmask\n");
	numlockmask = 0;
	modmap = XGetModifierMapping(display);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			  == XKeysymToKeycode(display, XK_Num_Lock))
				numlockmask = (1 << i);

	XFreeModifiermap(modmap);
}

void
grabkeys(void)
{
	unsigned int		i, j, k;
	KeyCode			code;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask | LockMask };

	DNPRINTF(SWM_D_MISC, "grabkeys\n");
	updatenumlockmask();

	for (k = 0; k < ScreenCount(display); k++) {
		if (TAILQ_EMPTY(&screens[k].rl))
			continue;
		XUngrabKey(display, AnyKey, AnyModifier, screens[k].root);
		for (i = 0; i < keys_length; i++) {
			if ((code = XKeysymToKeycode(display, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(display, code,
					    keys[i].mod | modifiers[j],
					    screens[k].root, True,
					    GrabModeAsync, GrabModeAsync);
		}
	}
}

void
grabbuttons(struct ws_win *win, int focused)
{
	unsigned int		i, j;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask|LockMask };

	updatenumlockmask();
	XUngrabButton(display, AnyButton, AnyModifier, win->id);
	if(focused) {
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].action == client_click)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(display, buttons[i].button,
					    buttons[i].mask | modifiers[j],
					    win->id, False, BUTTONMASK,
					    GrabModeAsync, GrabModeSync, None,
					    None);
	} else
		XGrabButton(display, AnyButton, AnyModifier, win->id, False,
		    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
}

void
expose(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "expose: window: %lu\n", e->xexpose.window);
}

void
keypress(XEvent *e)
{
	unsigned int		i;
	KeySym			keysym;
	XKeyEvent		*ev = &e->xkey;

	DNPRINTF(SWM_D_EVENT, "keypress: window: %lu\n", ev->window);

	keysym = XKeycodeToKeysym(display, (KeyCode)ev->keycode, 0);
	for (i = 0; i < keys_length; i++)
		if (keysym == keys[i].keysym
		   && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		   && keyfuncs[keys[i].funcid].func) {
			if (keys[i].funcid == kf_spawn_custom)
				spawn_custom(
				    root_to_region(ev->root),
				    &(keyfuncs[keys[i].funcid].args),
				    keys[i].spawn_name
				    );
			else
				keyfuncs[keys[i].funcid].func(
				    root_to_region(ev->root),
				    &(keyfuncs[keys[i].funcid].args)
				    );
		}
}

void
buttonpress(XEvent *e)
{
	XButtonPressedEvent	*ev = &e->xbutton;

	struct ws_win		*win;
	int			i, action;

	DNPRINTF(SWM_D_EVENT, "buttonpress: window: %lu\n", ev->window);

	action = root_click;
	if ((win = find_window(ev->window)) == NULL)
		return;
	else {
		focus_win(win);
		action = client_click;
	}

	for (i = 0; i < LENGTH(buttons); i++)
		if (action == buttons[i].action && buttons[i].func &&
		    buttons[i].button == ev->button &&
		    CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(win, &buttons[i].args);
}

void
set_win_state(struct ws_win *win, long state)
{
	long			data[] = {state, None};

	DNPRINTF(SWM_D_EVENT, "set_win_state: window: %lu\n", win->id);

	XChangeProperty(display, win->id, astate, astate, 32, PropModeReplace,
	    (unsigned char *)data, 2);
}

const char *quirkname[] = {
	"NONE",		/* config string for "no value" */
	"FLOAT",
	"TRANSSZ",
	"ANYWHERE",
	"XTERM_FONTADJ",
	"FULLSCREEN",
};

/* SWM_Q_WS: retain '|' for back compat for now (2009-08-11) */
#define	SWM_Q_WS		"\n|+ \t"
int
parsequirks(char *qstr, unsigned long *quirk)
{
	char			*cp, *name;
	int			i;
	if (quirk == NULL)
		return (1);
	cp = qstr;
	*quirk = 0;
	while ((name = strsep(&cp, SWM_Q_WS)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, SWM_Q_WS);
		for (i = 0; i < LENGTH(quirkname); i++) {
			if (!strncasecmp(name, quirkname[i], SWM_QUIRK_LEN)) {
				DNPRINTF(SWM_D_QUIRK, "parsequirks: %s\n", name);
				if (i == 0) {
					*quirk = 0;
					return (0);
				}
				*quirk |= 1 << (i-1);
				break;
			}
		}
		if (i >= LENGTH(quirkname)) {
			DNPRINTF(SWM_D_QUIRK,
			    "parsequirks: invalid quirk [%s]\n", name);
			return (1);
		}
	}
	return (0);
}
void
setquirk(const char *class, const char *name, const int quirk)
{
	int			i, j;
	/* find existing */
	for (i = 0; i < quirks_length; i++) {
		if (!strcmp(quirks[i].class, class) &&
		    !strcmp(quirks[i].name, name)) {
			if (!quirk) {
				/* found: delete */
				DNPRINTF(SWM_D_QUIRK,
				    "setquirk: delete #%d %s:%s\n",
				    i, quirks[i].class, quirks[i].name);
				free(quirks[i].class);
				free(quirks[i].name);
				j = quirks_length - 1;
				if (i < j)
					quirks[i] = quirks[j];
				quirks_length--;
				return;
			} else {
				/* found: replace */
				DNPRINTF(SWM_D_QUIRK,
				    "setquirk: replace #%d %s:%s\n",
				    i, quirks[i].class, quirks[i].name);
				free(quirks[i].class);
				free(quirks[i].name);
				quirks[i].class = strdup(class);
				quirks[i].name = strdup(name);
				quirks[i].quirk = quirk;
				return;
			}
		}
	}
	if (!quirk) {
		fprintf(stderr,
		    "error: setquirk: cannot find class/name combination");
		return;
	}
	/* not found: add */
	if (quirks_size == 0 || quirks == NULL) {
		quirks_size = 4;
		DNPRINTF(SWM_D_QUIRK, "setquirk: init list %d\n", quirks_size);
		quirks = malloc((size_t)quirks_size * sizeof(struct quirk));
		if (!quirks) {
			fprintf(stderr, "setquirk: malloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	} else if (quirks_length == quirks_size) {
		quirks_size *= 2;
		DNPRINTF(SWM_D_QUIRK, "setquirk: grow list %d\n", quirks_size);
		quirks = realloc(quirks, (size_t)quirks_size * sizeof(struct quirk));
		if (!quirks) {
			fprintf(stderr, "setquirk: realloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	}
	if (quirks_length < quirks_size) {
		DNPRINTF(SWM_D_QUIRK, "setquirk: add %d\n", quirks_length);
		j = quirks_length++;
		quirks[j].class = strdup(class);
		quirks[j].name = strdup(name);
		quirks[j].quirk = quirk;
	} else {
		fprintf(stderr, "quirks array problem?\n");
		if (!quirks) {
			fprintf(stderr, "quirks array problem!\n");
			quit(NULL, NULL);
		}
	}
}
int
setconfquirk(char *selector, char *value, int flags)
{
	char			*cp, *class, *name;
	int			retval;
	unsigned long		quirks;
	if (selector == NULL)
		return (0);
	if ((cp = strchr(selector, ':')) == NULL)
		return (0);
	*cp = '\0';
	class = selector;
	name = cp + 1;
	if ((retval = parsequirks(value, &quirks)) == 0)
		setquirk(class, name, quirks);
	return (retval);
}

void
setup_quirks(void)
{
	setquirk("MPlayer",		"xv",		SWM_Q_FLOAT | SWM_Q_FULLSCREEN);
	setquirk("OpenOffice.org 2.4",	"VCLSalFrame",	SWM_Q_FLOAT);
	setquirk("OpenOffice.org 3.0",	"VCLSalFrame",	SWM_Q_FLOAT);
	setquirk("Firefox-bin",		"firefox-bin",	SWM_Q_TRANSSZ);
	setquirk("Firefox",		"Dialog",	SWM_Q_FLOAT);
	setquirk("Gimp",		"gimp",		SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("XTerm",		"xterm",	SWM_Q_XTERM_FONTADJ);
	setquirk("xine",		"Xine Window",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("Xitk",		"Xitk Combo",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("xine",		"xine Panel",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("Xitk",		"Xine Window",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("xine",		"xine Video Fullscreen Window",	SWM_Q_FULLSCREEN | SWM_Q_FLOAT);
	setquirk("pcb",			"pcb",		SWM_Q_FLOAT);
}

/* conf file stuff */
#define SWM_CONF_FILE	"scrotwm.conf"

enum	{ SWM_S_BAR_DELAY, SWM_S_BAR_ENABLED, SWM_S_CLOCK_ENABLED,
	  SWM_S_CYCLE_EMPTY, SWM_S_CYCLE_VISIBLE, SWM_S_SS_ENABLED,
	  SWM_S_TERM_WIDTH, SWM_S_TITLE_CLASS_ENABLED, SWM_S_TITLE_NAME_ENABLED,
	  SWM_S_BAR_FONT, SWM_S_BAR_ACTION, SWM_S_SPAWN_TERM, SWM_S_SS_APP,
	  SWM_S_DIALOG_RATIO };

int
setconfvalue(char *selector, char *value, int flags)
{
	switch (flags) {
	case SWM_S_BAR_DELAY:
		bar_delay = atoi(value);
		break;
	case SWM_S_BAR_ENABLED:
		bar_enabled = atoi(value);
		break;
	case SWM_S_CLOCK_ENABLED:
		clock_enabled = atoi(value);
		break;
	case SWM_S_CYCLE_EMPTY:
		cycle_empty = atoi(value);
		break;
	case SWM_S_CYCLE_VISIBLE:
		cycle_visible = atoi(value);
		break;
	case SWM_S_SS_ENABLED:
		ss_enabled = atoi(value);
		break;
	case SWM_S_TERM_WIDTH:
		term_width = atoi(value);
		break;
	case SWM_S_TITLE_CLASS_ENABLED:
		title_class_enabled = atoi(value);
		break;
	case SWM_S_TITLE_NAME_ENABLED:
		title_name_enabled = atoi(value);
		break;
	case SWM_S_BAR_FONT:
		free(bar_fonts[0]);
		if ((bar_fonts[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_font");
		break;
	case SWM_S_BAR_ACTION:
		free(bar_argv[0]);
		if ((bar_argv[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_action");
		break;
	case SWM_S_SPAWN_TERM:
		free(spawn_term[0]);
		if ((spawn_term[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: spawn_term");
		break;
	case SWM_S_SS_APP:
		break;
	case SWM_S_DIALOG_RATIO:
		dialog_ratio = atof(value);
		if (dialog_ratio > 1.0 || dialog_ratio <= .3)
			dialog_ratio = .6;
		break;
	default:
		return (1);
	}
	return (0);
}

int
setconfmodkey(char *selector, char *value, int flags)
{
	if (!strncasecmp(value, "Mod1", strlen("Mod1")))
		update_modkey(Mod1Mask);
	else if (!strncasecmp(value, "Mod2", strlen("Mod2")))
		update_modkey(Mod2Mask);
	else if (!strncasecmp(value, "Mod3", strlen("Mod3")))
		update_modkey(Mod3Mask);
	else if (!strncasecmp(value, "Mod4", strlen("Mod4")))
		update_modkey(Mod4Mask);
	else
		return (1);
	return (0);
}

int
setconfcolor(char *selector, char *value, int flags)
{
	setscreencolor(value, ((selector == NULL)?-1:atoi(selector)), flags);
	return (0);
}

int
setconfregion(char *selector, char *value, int flags)
{
	custom_region(value);
	return (0);
}

/* config options */
struct config_option {
	char			*optname;
	int (*func)(char*, char*, int);
	int funcflags;
};
struct config_option configopt[] = {
	{ "bar_enabled",		setconfvalue,	SWM_S_BAR_ENABLED },
	{ "bar_border",			setconfcolor,	SWM_S_COLOR_BAR_BORDER },
	{ "bar_color",			setconfcolor,	SWM_S_COLOR_BAR },
	{ "bar_font_color",		setconfcolor,	SWM_S_COLOR_BAR_FONT },
	{ "bar_font",			setconfvalue,	SWM_S_BAR_FONT },
	{ "bar_action",			setconfvalue,	SWM_S_BAR_ACTION },
	{ "bar_delay",			setconfvalue,	SWM_S_BAR_DELAY },
	{ "bind",			setconfbinding,	0 },
	{ "clock_enabled",		setconfvalue,	SWM_S_CLOCK_ENABLED },
	{ "color_focus",		setconfcolor,	SWM_S_COLOR_FOCUS },
	{ "color_unfocus",		setconfcolor,	SWM_S_COLOR_UNFOCUS },
	{ "cycle_empty",		setconfvalue,	SWM_S_CYCLE_EMPTY },
	{ "cycle_visible",		setconfvalue,	SWM_S_CYCLE_VISIBLE },
	{ "dialog_ratio",		setconfvalue,	SWM_S_DIALOG_RATIO },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "spawn_term",			setconfvalue,	SWM_S_SPAWN_TERM },
	{ "screenshot_enabled",		setconfvalue,	SWM_S_SS_ENABLED },
	{ "screenshot_app",		setconfvalue,	SWM_S_SS_APP },
	{ "term_width",			setconfvalue,	SWM_S_TERM_WIDTH },
	{ "title_class_enabled",	setconfvalue,	SWM_S_TITLE_CLASS_ENABLED },
	{ "title_name_enabled",		setconfvalue,	SWM_S_TITLE_NAME_ENABLED }
};


int
conf_load(char *filename)
{
	FILE			*config;
	char			*line, *cp, *optsub, *optval;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optind;
	struct config_option	*opt;

	DNPRINTF(SWM_D_CONF, "conf_load begin\n");

	if (filename == NULL) {
		fprintf(stderr, "conf_load: no filename\n");
		return (1);
	}
	if ((config = fopen(filename, "r")) == NULL) {
		warn("conf_load: fopen");
		return (1);
	}

	while (!feof(config)) {
		if ((line = fparseln(config, &linelen, &lineno, NULL, 0))
		    == NULL) {
			if (ferror(config))
				err(1, "%s", filename);
			else
				continue;
		}
		cp = line;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}
		/* get config option */
		wordlen = strcspn(cp, "=[ \t\n");
		if (wordlen == 0) {
			warnx("%s: line %zd: no option found",
			    filename, lineno);
			return (1);
		}
		optind = -1;
		for (i = 0; i < LENGTH(configopt); i++) {
			opt = &configopt[i];
			if (!strncasecmp(cp, opt->optname, wordlen) &&
			    strlen(opt->optname) == wordlen) {
				optind = i;
				break;
			}
		}
		if (optind == -1) {
			warnx("%s: line %zd: unknown option %.*s",
			    filename, lineno, wordlen, cp);
			return (1);
		}
		cp += wordlen;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		/* get [selector] if any */
		optsub = NULL;
		if (*cp == '[') {
			cp++;
			wordlen = strcspn(cp, "]");
			if (*cp != ']') {
				if (wordlen == 0) {
					warnx("%s: line %zd: syntax error",
					    filename, lineno);
					return (1);
				}
				asprintf(&optsub, "%.*s", wordlen, cp);
			}
			cp += wordlen;
			cp += strspn(cp, "] \t\n"); /* eat trailing */
		}
		cp += strspn(cp, "= \t\n"); /* eat trailing */
		/* get RHS value */
		optval = strdup(cp);
		/* call function to deal with it all */
		if (configopt[optind].func(optsub, optval,
		    configopt[optind].funcflags) != 0) {
			fprintf(stderr, "%s line %zd: %s\n",
			    filename, lineno, line);
			errx(1, "%s: line %zd: invalid data for %s",
			    filename, lineno, configopt[optind].optname);
		}
		free(optval);
		free(optsub);
		free(line);
	}

	fclose(config);
	DNPRINTF(SWM_D_CONF, "conf_load end\n");

	return (0);
}

struct ws_win *
manage_window(Window id)
{
	Window			trans;
	struct workspace	*ws;
	struct ws_win		*win, *ww;
	int			format, i, ws_idx, n, border_me = 0;
	unsigned long		nitems, bytes;
	Atom			ws_idx_atom = 0, type;
	Atom			*prot = NULL, *pp;
	unsigned char		ws_idx_str[SWM_PROPLEN], *prop = NULL;
	struct swm_region	*r;
	long			mask;
	const char		*errstr;
	XWindowChanges		wc;

	if ((win = find_window(id)) != NULL)
			return (win);	/* already being managed */

	if ((win = calloc(1, sizeof(struct ws_win))) == NULL)
		errx(1, "calloc: failed to allocate memory for new window");

	/* Get all the window data in one shot */
	ws_idx_atom = XInternAtom(display, "_SWM_WS", False);
	if (ws_idx_atom)
		XGetWindowProperty(display, id, ws_idx_atom, 0, SWM_PROPLEN,
		    False, XA_STRING, &type, &format, &nitems, &bytes, &prop);
	XGetWindowAttributes(display, id, &win->wa);
	XGetTransientForHint(display, id, &trans);
	XGetWMNormalHints(display, id, &win->sh, &mask); /* XXX function? */
	if (trans) {
		win->transient = trans;
		DNPRINTF(SWM_D_MISC, "manage_window: win %u transient %u\n",
		    (unsigned)win->id, win->transient);
	}
	/* get supported protocols */
	if (XGetWMProtocols(display, id, &prot, &n)) {
		for (i = 0, pp = prot; i < n; i++, pp++)
			if (*pp == adelete)
				win->can_delete = 1;
		if (prot)
			XFree(prot);
	}

	/*
	 * Figure out where to put the window. If it was previously assigned to
	 * a workspace (either by spawn() or manually moving), and isn't
	 * transient, * put it in the same workspace
	 */
	r = root_to_region(win->wa.root);
	if (prop && win->transient == 0) {
		DNPRINTF(SWM_D_PROP, "got property _SWM_WS=%s\n", prop);
		ws_idx = strtonum(prop, 0, 9, &errstr);
		if (errstr) {
			DNPRINTF(SWM_D_EVENT, "window idx is %s: %s",
			    errstr, prop);
		}
		ws = &r->s->ws[ws_idx];
	} else {
		ws = r->ws;
		/* this should launch transients in the same ws as parent */
		/* XXX doesn't work for intel xrandr */
		if (id && trans)
			if ((ww = find_window(trans)) != NULL)
				if (ws->r) {
					ws = ww->ws;
					r = ww->ws->r;
					border_me = 1;
				}
	}

	/* set up the window layout */
	win->id = id;
	win->ws = ws;
	win->s = r->s;	/* this never changes */
	TAILQ_INSERT_TAIL(&ws->winlist, win, entry);

	win->g.w = win->wa.width;
	win->g.h = win->wa.height;
	win->g.x = win->wa.x;
	win->g.y = win->wa.y;

	/* Set window properties so we can remember this after reincarnation */
	if (ws_idx_atom && prop == NULL &&
	    snprintf(ws_idx_str, SWM_PROPLEN, "%d", ws->idx) < SWM_PROPLEN) {
		DNPRINTF(SWM_D_PROP, "setting property _SWM_WS to %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, SWM_PROPLEN);
	}
	XFree(prop);

	if (XGetClassHint(display, win->id, &win->ch)) {
		DNPRINTF(SWM_D_CLASS, "class: %s name: %s\n",
		    win->ch.res_class, win->ch.res_name);
		for (i = 0; i < quirks_length; i++){
			if (!strcmp(win->ch.res_class, quirks[i].class) &&
			    !strcmp(win->ch.res_name, quirks[i].name)) {
				DNPRINTF(SWM_D_CLASS, "found: %s name: %s\n",
				    win->ch.res_class, win->ch.res_name);
				if (quirks[i].quirk & SWM_Q_FLOAT)
					win->floating = 1;
				win->quirks = quirks[i].quirk;
			}
		}
	}

	/* alter window position if quirky */
	if (win->quirks & SWM_Q_ANYWHERE) {
		win->manual = 1; /* don't center the quirky windows */
		bzero(&wc, sizeof wc);
		mask = 0;
		if (win->g.y < bar_height) {
			win->g.y = wc.y = bar_height;
			mask |= CWY;
		}
		if (win->g.w + win->g.x > WIDTH(r)) {
			win->g.x = wc.x = WIDTH(r) - win->g.w - 2;
			mask |= CWX;
		}
		border_me = 1;
	}

	/* Reset font sizes (the bruteforce way; no default keybinding). */
	if (win->quirks & SWM_Q_XTERM_FONTADJ) {
		for (i = 0; i < SWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Subtract, ShiftMask);
		for (i = 0; i < SWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Add, ShiftMask);
	}

	/* border me */
	if (border_me) {
		bzero(&wc, sizeof wc);
		wc.border_width = 1;
		mask = CWBorderWidth;
		XConfigureWindow(display, win->id, mask, &wc);
	}

	XSelectInput(display, id, EnterWindowMask | FocusChangeMask |
	    PropertyChangeMask | StructureNotifyMask);

	set_win_state(win, NormalState);

	/* floaters need to be mapped if they are in the current workspace */
	if ((win->floating || win->transient) && (ws->idx == r->ws->idx))
		XMapRaised(display, win->id);

	return (win);
}

void
unmanage_window(struct ws_win *win)
{
	struct workspace	*ws;

	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "unmanage_window:  %lu\n", win->id);

	ws = win->ws;
	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	set_win_state(win, WithdrawnState);
	if (win->ch.res_class)
		XFree(win->ch.res_class);
	if (win->ch.res_name)
		XFree(win->ch.res_name);
	free(win);
}

void
configurerequest(XEvent *e)
{
	XConfigureRequestEvent	*ev = &e->xconfigurerequest;
	struct ws_win		*win;
	int			new = 0;
	XWindowChanges		wc;

	if ((win = find_window(ev->window)) == NULL)
		new = 1;

	if (new) {
		DNPRINTF(SWM_D_EVENT, "configurerequest: new window: %lu\n",
		    ev->window);
		bzero(&wc, sizeof wc);
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(display, ev->window, ev->value_mask, &wc);
	} else {
		DNPRINTF(SWM_D_EVENT, "configurerequest: change window: %lu\n",
		    ev->window);
		if (win->floating) {
			if (ev->value_mask & CWX)
				win->g.x = ev->x;
			if (ev->value_mask & CWY)
				win->g.y = ev->y;
			if (ev->value_mask & CWWidth)
				win->g.w = ev->width;
			if (ev->value_mask & CWHeight)
				win->g.h = ev->height;
			if (win->ws->r != NULL) {
				/* this seems to be full screen */
				if (win->g.w >= WIDTH(win->ws->r)) {
					win->g.x = 0;
					win->g.w = WIDTH(win->ws->r);
					ev->value_mask |= CWX | CWWidth;
				}
				if (win->g.h >= HEIGHT(win->ws->r)) {
					/* kill border */
					win->g.y = 0;
					win->g.h = HEIGHT(win->ws->r);
					ev->value_mask |= CWY | CWHeight;
				}
			}
			if ((ev->value_mask & (CWX | CWY)) &&
			    !(ev->value_mask & (CWWidth | CWHeight)))
				config_win(win);
			XMoveResizeWindow(display, win->id,
			    win->g.x, win->g.y, win->g.w, win->g.h);
		} else
			config_win(win);
	}
}

void
configurenotify(XEvent *e)
{
	struct ws_win		*win;
	long			mask;

	DNPRINTF(SWM_D_EVENT, "configurenotify: window: %lu\n",
	    e->xconfigure.window);

	XMapWindow(display, e->xconfigure.window);
	win = find_window(e->xconfigure.window);
	if (win) {
		XGetWMNormalHints(display, win->id, &win->sh, &mask);
		adjust_font(win);
		XMapWindow(display, win->id);
		if (font_adjusted)
			stack();
	}
}

void
destroynotify(XEvent *e)
{
	struct ws_win		*win, *winfocus = NULL;
	struct workspace	*ws;
	struct ws_win_list	*wl;

	XDestroyWindowEvent	*ev = &e->xdestroywindow;

	DNPRINTF(SWM_D_EVENT, "destroynotify: window %lu\n", ev->window);

	if ((win = find_window(ev->window)) != NULL) {
		/* find a window to focus */
		ws = win->ws;
		wl = &ws->winlist;
		if (ws->focus == win) {
			if (TAILQ_FIRST(wl) == win)
				winfocus = TAILQ_NEXT(win, entry);
			else {
				winfocus = TAILQ_PREV(ws->focus, ws_win_list, entry);
				if (winfocus == NULL)
					winfocus = TAILQ_LAST(wl, ws_win_list);
			}
		}

		unmanage_window(win);
		stack();
		if (winfocus)
			focus_win(winfocus);
	}
}

void
enternotify(XEvent *e)
{
	XCrossingEvent		*ev = &e->xcrossing;
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "enternotify: window: %lu\n", ev->window);

	if (ignore_enter) {
		/* eat event(r) to prevent autofocus */
		ignore_enter--;
		return;
	}
	/*
	 * happens when a window is created or destroyed and the border
	 * crosses the mouse pointer
	 */
	if (QLength(display))
		return;

	if ((win = find_window(ev->window)) != NULL)
		focus_win(win);
}

void
focusin(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "focusin: window: %lu\n", e->xfocus.window);
}

void
focusout(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "focusout: window: %lu\n", e->xfocus.window);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent		*ev = &e->xmapping;

	DNPRINTF(SWM_D_EVENT, "mappingnotify: window: %lu\n", ev->window);

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	struct ws_win		*win;
	struct swm_region	*r;

	XMapRequestEvent	*ev = &e->xmaprequest;
	XWindowAttributes	wa;

	DNPRINTF(SWM_D_EVENT, "maprequest: window: %lu\n",
	    e->xmaprequest.window);

	if (!XGetWindowAttributes(display, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;

	manage_window(e->xmaprequest.window);

	stack();

	/* make new win focused */
	win = find_window(ev->window);
	r = root_to_region(win->wa.root);
	if (win->ws == r->ws)
		focus_win(win);
}

void
propertynotify(XEvent *e)
{
	struct ws_win		*win;
	XPropertyEvent		*ev = &e->xproperty;

	DNPRINTF(SWM_D_EVENT, "propertynotify: window: %lu\n",
	    ev->window);

	if (ev->state == PropertyDelete)
		return; /* ignore */
	win = find_window(ev->window);
	if (win == NULL)
		return;

	switch (ev->atom) {
	case XA_WM_NORMAL_HINTS:
#if 0
		long		mask;
		XGetWMNormalHints(display, win->id, &win->sh, &mask);
		fprintf(stderr, "normal hints: flag 0x%x\n", win->sh.flags);
		if (win->sh.flags & PMinSize) {
			win->g.w = win->sh.min_width;
			win->g.h = win->sh.min_height;
			fprintf(stderr, "min %d %d\n", win->g.w, win->g.h);
		}
		XMoveResizeWindow(display, win->id,
		    win->g.x, win->g.y, win->g.w, win->g.h);
#endif
		break;
	default:
		break;
	}
}

void
unmapnotify(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "unmapnotify: window: %lu\n", e->xunmap.window);
}

void
visibilitynotify(XEvent *e)
{
	int			i;
	struct swm_region	*r;

	DNPRINTF(SWM_D_EVENT, "visibilitynotify: window: %lu\n",
	    e->xvisibility.window);
	if (e->xvisibility.state == VisibilityUnobscured)
		for (i = 0; i < ScreenCount(display); i++)
			TAILQ_FOREACH(r, &screens[i].rl, entry)
				if (e->xvisibility.window == r->bar_window)
					bar_update();
}

int
xerror_start(Display *d, XErrorEvent *ee)
{
	other_wm = 1;
	return (-1);
}

int
xerror(Display *d, XErrorEvent *ee)
{
	/* fprintf(stderr, "error: %p %p\n", display, ee); */
	return (-1);
}

int
active_wm(void)
{
	other_wm = 0;
	xerrorxlib = XSetErrorHandler(xerror_start);

	/* this causes an error if some other window manager is running */
	XSelectInput(display, DefaultRootWindow(display),
	    SubstructureRedirectMask);
	XSync(display, False);
	if (other_wm)
		return (1);

	XSetErrorHandler(xerror);
	XSync(display, False);
	return (0);
}

long
getstate(Window w)
{
	int			format, status;
	long			result = -1;
	unsigned char		*p = NULL;
	unsigned long		n, extra;
	Atom			real;

	status = XGetWindowProperty(display, w, astate, 0L, 2L, False, astate,
	    &real, &format, &n, &extra, (unsigned char **)&p);
	if (status != Success)
		return (-1);
	if (n != 0)
		result = *((long *)p);
	XFree(p);
	return (result);
}

void
new_region(struct swm_screen *s, int x, int y, int w, int h)
{
	struct swm_region	*r, *n;
	struct workspace	*ws = NULL;
	int			i;

	DNPRINTF(SWM_D_MISC, "new region: screen[%d]:%dx%d+%d+%d\n",
	     s->idx, w, h, x, y);

	/* remove any conflicting regions */
	n = TAILQ_FIRST(&s->rl);
	while (n) {
		r = n;
		n = TAILQ_NEXT(r, entry);
		if (X(r) < (x + w) &&
		    (X(r) + WIDTH(r)) > x &&
		    Y(r) < (y + h) &&
		    (Y(r) + HEIGHT(r)) > y) {
			XDestroyWindow(display, r->bar_window);
			TAILQ_REMOVE(&s->rl, r, entry);
			TAILQ_INSERT_TAIL(&s->orl, r, entry);
		}
	}

	/* search old regions for one to reuse */

	/* size + location match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (X(r) == x && Y(r) == y &&
		    HEIGHT(r) == h && WIDTH(r) == w)
			break;

	/* size match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (HEIGHT(r) == h && WIDTH(r) == w)
			break;

	if (r != NULL) {
		TAILQ_REMOVE(&s->orl, r, entry);
		/* try to use old region's workspace */
		if (r->ws->r == NULL)
			ws = r->ws;
	} else
		if ((r = calloc(1, sizeof(struct swm_region))) == NULL)
			errx(1, "calloc: failed to allocate memory for screen");

	/* if we don't have a workspace already, find one */
	if (ws == NULL) {
		for (i = 0; i < SWM_WS_MAX; i++)
			if (s->ws[i].r == NULL) {
				ws = &s->ws[i];
				break;
			}
	}

	if (ws == NULL)
		errx(1, "no free workspaces\n");

	X(r) = x;
	Y(r) = y;
	WIDTH(r) = w;
	HEIGHT(r) = h;
	r->s = s;
	r->ws = ws;
	ws->r = r;
	TAILQ_INSERT_TAIL(&s->rl, r, entry);
}

void
scan_xrandr(int i)
{
#ifdef SWM_XRR_HAS_CRTC
	XRRCrtcInfo		*ci;
	XRRScreenResources	*sr;
	int			c;
	int			ncrtc = 0;
#endif /* SWM_XRR_HAS_CRTC */
	struct swm_region	*r;


	if (i >= ScreenCount(display))
		errx(1, "invalid screen");

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&screens[i].rl)) != NULL) {
		r->ws->r = NULL;
		XDestroyWindow(display, r->bar_window);
		TAILQ_REMOVE(&screens[i].rl, r, entry);
		TAILQ_INSERT_TAIL(&screens[i].orl, r, entry);
	}

	/* map virtual screens onto physical screens */
#ifdef SWM_XRR_HAS_CRTC
	outputs = 0;
	if (xrandr_support) {
		sr = XRRGetScreenResources(display, screens[i].root);
		if (sr == NULL)
			new_region(&screens[i], 0, 0,
			    DisplayWidth(display, i),
			    DisplayHeight(display, i));
		else
			ncrtc = sr->ncrtc;

		for (c = 0, ci = NULL; c < ncrtc; c++) {
			ci = XRRGetCrtcInfo(display, sr, sr->crtcs[c]);
			if (ci->noutput == 0)
				continue;
			outputs++;

			if (ci != NULL && ci->mode == None)
				new_region(&screens[i], 0, 0,
				    DisplayWidth(display, i),
				    DisplayHeight(display, i));
			else
				new_region(&screens[i],
				    ci->x, ci->y, ci->width, ci->height);
		}
		if (ci)
			XRRFreeCrtcInfo(ci);
		XRRFreeScreenResources(sr);
	} else
#endif /* SWM_XRR_HAS_CRTC */
	{
		new_region(&screens[i], 0, 0, DisplayWidth(display, i),
		    DisplayHeight(display, i));
	}
}

void
screenchange(XEvent *e) {
	XRRScreenChangeNotifyEvent	*xe = (XRRScreenChangeNotifyEvent *)e;
	struct swm_region		*r;
	struct ws_win			*win;
	int				i;

	DNPRINTF(SWM_D_EVENT, "screenchange: %lu\n", xe->root);

	if (!XRRUpdateConfiguration(e))
		return;

	/* silly event doesn't include the screen index */
	for (i = 0; i < ScreenCount(display); i++)
		if (screens[i].root == xe->root)
			break;
	if (i >= ScreenCount(display))
		errx(1, "screenchange: screen not found\n");

	/* brute force for now, just re-enumerate the regions */
	scan_xrandr(i);

	/* hide any windows that went away */
	TAILQ_FOREACH(r, &screens[i].rl, entry)
		TAILQ_FOREACH(win, &r->ws->winlist, entry)
			XUnmapWindow(display, win->id);

	/* add bars to all regions */
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			bar_setup(r);
	stack();
}

void
setup_screens(void)
{
	Window			d1, d2, *wins = NULL;
	XWindowAttributes	wa;
	unsigned int		no;
        int			i, j, k;
	int			errorbase, major, minor;
	struct workspace	*ws;
	int			ws_idx_atom;


	if ((screens = calloc(ScreenCount(display),
	     sizeof(struct swm_screen))) == NULL)
		errx(1, "calloc: screens");

	ws_idx_atom = XInternAtom(display, "_SWM_WS", False);

	/* initial Xrandr setup */
	xrandr_support = XRRQueryExtension(display,
	    &xrandr_eventbase, &errorbase);
	if (xrandr_support)
		if (XRRQueryVersion(display, &major, &minor) && major < 1)
			xrandr_support = 0;

	/* map physical screens */
	for (i = 0; i < ScreenCount(display); i++) {
		DNPRINTF(SWM_D_WS, "setup_screens: init screen %d\n", i);
		screens[i].idx = i;
		TAILQ_INIT(&screens[i].rl);
		TAILQ_INIT(&screens[i].orl);
		screens[i].root = RootWindow(display, i);

		/* set default colors */
		setscreencolor("red", i + 1, SWM_S_COLOR_FOCUS);
		setscreencolor("rgb:88/88/88", i + 1, SWM_S_COLOR_UNFOCUS);
		setscreencolor("rgb:00/80/80", i + 1, SWM_S_COLOR_BAR_BORDER);
		setscreencolor("black", i + 1, SWM_S_COLOR_BAR);
		setscreencolor("rgb:a0/a0/a0", i + 1, SWM_S_COLOR_BAR_FONT);

		/* init all workspaces */
		/* XXX these should be dynamically allocated too */
		for (j = 0; j < SWM_WS_MAX; j++) {
			ws = &screens[i].ws[j];
			ws->idx = j;
			ws->restack = 1;
			ws->focus = NULL;
			ws->r = NULL;
			TAILQ_INIT(&ws->winlist);

			for (k = 0; layouts[k].l_stack != NULL; k++)
				if (layouts[k].l_config != NULL)
					layouts[k].l_config(ws,
					    SWM_ARG_ID_STACKINIT);
			ws->cur_layout = &layouts[0];
		}
		/* grab existing windows (before we build the bars)*/
		if (!XQueryTree(display, screens[i].root, &d1, &d2, &wins, &no))
			continue;

		scan_xrandr(i);

		if (xrandr_support)
			XRRSelectInput(display, screens[i].root,
			    RRScreenChangeNotifyMask);

		/* attach windows to a region */
		/* normal windows */
		for (j = 0; j < no; j++) {
                        XGetWindowAttributes(display, wins[j], &wa);
			if (!XGetWindowAttributes(display, wins[j], &wa) ||
			    wa.override_redirect ||
			    XGetTransientForHint(display, wins[j], &d1))
				continue;

			if (wa.map_state == IsViewable ||
			    getstate(wins[j]) == NormalState)
				manage_window(wins[j]);
		}
		/* transient windows */
		for (j = 0; j < no; j++) {
			if (!XGetWindowAttributes(display, wins[j], &wa))
				continue;

			if (XGetTransientForHint(display, wins[j], &d1) &&
			    (wa.map_state == IsViewable || getstate(wins[j]) ==
			    NormalState))
				manage_window(wins[j]);
                }
                if (wins) {
                        XFree(wins);
			wins = NULL;
		}
	}
}
void
setup_globals(void)
{
	if ((bar_fonts[0] = strdup("-*-terminus-medium-*-*-*-*-*-*-*-*-*-*-*"))
	    == NULL)
		err(1, "setup_globals: strdup");
	if ((bar_fonts[1] = strdup("-*-times-medium-r-*-*-*-*-*-*-*-*-*-*"))
	    == NULL)
		err(1, "setup_globals: strdup");
	if ((bar_fonts[2] = strdup("-misc-fixed-medium-r-*-*-*-*-*-*-*-*-*-*"))
	    == NULL)
		err(1, "setup_globals: strdup");
	if ((spawn_term[0] = strdup("xterm")) == NULL)
		err(1, "setup_globals: strdup");
}

void
workaround(void)
{
	int			i;
	Atom			netwmcheck, netwmname, utf8_string;
	Window			root;

	/* work around sun jdk bugs, code from wmname */
	netwmcheck = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
	netwmname = XInternAtom(display, "_NET_WM_NAME", False);
	utf8_string = XInternAtom(display, "UTF8_STRING", False);
	for (i = 0; i < ScreenCount(display); i++) {
		root = screens[i].root;
		XChangeProperty(display, root, netwmcheck, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&root, 1);
		XChangeProperty(display, root, netwmname, utf8_string, 8,
		    PropModeReplace, "LG3D", strlen("LG3D"));
	}
}

int
main(int argc, char *argv[])
{
	struct passwd		*pwd;
	struct swm_region	*r, *rr;
	struct ws_win		*winfocus = NULL;
	char			conf[PATH_MAX], *cfile = NULL;
	struct stat		sb;
	XEvent			e;
	int			xfd, i;
	fd_set			rd;

	start_argv = argv;
	fprintf(stderr, "Welcome to scrotwm V%s cvs tag: %s\n",
	    SWM_VERSION, cvstag);
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		warnx("no locale support");

	if (!(display = XOpenDisplay(0)))
		errx(1, "can not open display");

	if (active_wm())
		errx(1, "other wm running");

	/* handle some signale */
	installsignal(SIGINT, "INT");
	installsignal(SIGHUP, "HUP");
	installsignal(SIGQUIT, "QUIT");
	installsignal(SIGTERM, "TERM");
	installsignal(SIGCHLD, "CHLD");

	astate = XInternAtom(display, "WM_STATE", False);
	aprot = XInternAtom(display, "WM_PROTOCOLS", False);
	adelete = XInternAtom(display, "WM_DELETE_WINDOW", False);

	/* look for local and global conf file */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user %d", getuid());

	setup_screens();
	setup_globals();
	setup_keys();
	setup_quirks();
	setup_spawn();

	snprintf(conf, sizeof conf, "%s/.%s", pwd->pw_dir, SWM_CONF_FILE);
	if (stat(conf, &sb) != -1) {
		if (S_ISREG(sb.st_mode))
			cfile = conf;
	} else {
		/* try global conf file */
		snprintf(conf, sizeof conf, "/etc/%s", SWM_CONF_FILE);
		if (!stat(conf, &sb))
			if (S_ISREG(sb.st_mode))
				cfile = conf;
	}
	if (cfile)
		conf_load(cfile);

	/* setup all bars */
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (winfocus == NULL)
				winfocus = TAILQ_FIRST(&r->ws->winlist);
			bar_setup(r);
		}

	/* set some values to work around bad programs */
	workaround();

	grabkeys();
	stack();

	xfd = ConnectionNumber(display);
	while (running) {
		while (XPending(display)) {
			XNextEvent(display, &e);
			if (running == 0)
				goto done;
			if (e.type < LASTEvent) {
				dumpevent(&e);
				if (handler[e.type])
					handler[e.type](&e);
				else
					DNPRINTF(SWM_D_EVENT,
					    "win: %lu unknown event: %d\n",
					    e.xany.window, e.type);
			} else {
				switch (e.type - xrandr_eventbase) {
				case RRScreenChangeNotify:
					screenchange(&e);
					break;
				default:
					DNPRINTF(SWM_D_EVENT,
					    "win: %lu unknown xrandr event: "
					    "%d\n", e.xany.window, e.type);
					break;
				}
			}
		}

		/* if we are being restarted go focus on first window */
		if (winfocus) {
			rr = TAILQ_FIRST(&screens[0].rl);
			/* move pointer to first screen if multi screen */
			if (ScreenCount(display) > 1 || outputs > 1)
				XWarpPointer(display, None, rr->s[0].root,
				    0, 0, 0, 0, rr->g.x,
				    rr->g.y + bar_enabled ? bar_height : 0);

			focus_win(winfocus);
			winfocus = NULL;
			continue;
		}

		FD_ZERO(&rd);
		FD_SET(xfd, &rd);
		if (select(xfd + 1, &rd, NULL, NULL, NULL) == -1)
			if (errno != EINTR)
				DNPRINTF(SWM_D_MISC, "select failed");
		if (running == 0)
			goto done;
		if (bar_alarm) {
			bar_alarm = 0;
			bar_update();
		}
	}
done:
	bar_extra_stop();

	XCloseDisplay(display);

	return (0);
}
