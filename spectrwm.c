/*
 * Copyright (c) 2009-2012 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009-2011 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 * Copyright (c) 2010 Tuukka Kataja <stuge@xor.fi>
 * Copyright (c) 2011 Jason L. Wright <jason@thought.net>
 * Copyright (c) 2011-2014 Reginald Kennedy <rk@rejii.com>
 * Copyright (c) 2011-2012 Lawrence Teo <lteo@lteo.net>
 * Copyright (c) 2011-2012 Tiago Cunha <tcunha@gmx.com>
 * Copyright (c) 2012-2013 David Hill <dhill@mindcry.org>
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

/* kernel includes */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/select.h>
#if defined(__linux__)
#include "tree.h"
#elif defined(__OpenBSD__)
#include <sys/tree.h>
#elif defined(__FreeBSD__)
#include <sys/tree.h>
#else
#include "tree.h"
#endif

/* /usr/includes */
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <locale.h>
#include <paths.h>
#include <pwd.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xtest.h>
#include <xcb/randr.h>

/* local includes */
#include "version.h"
#ifdef __OSX__
#include <osx.h>
#endif

#ifdef SPECTRWM_BUILDSTR
static const char	*buildstr = SPECTRWM_BUILDSTR;
#else
static const char	*buildstr = SPECTRWM_VERSION;
#endif

#if !defined(__CYGWIN__) /* cygwin chokes on randr stuff */
#  if RANDR_MAJOR < 1
#    error RandR versions less than 1.0 are not supported
#endif

#  if RANDR_MAJOR >= 1
#    if RANDR_MINOR >= 2
#      define SWM_XRR_HAS_CRTC
#    endif
#  endif
#endif /* __CYGWIN__ */

#ifndef XCB_ICCCM_NUM_WM_HINTS_ELEMENTS
#define XCB_ICCCM_SIZE_HINT_P_MIN_SIZE		XCB_SIZE_HINT_P_MIN_SIZE
#define XCB_ICCCM_SIZE_HINT_P_MAX_SIZE		XCB_SIZE_HINT_P_MAX_SIZE
#define XCB_ICCCM_SIZE_HINT_P_RESIZE_INC	XCB_SIZE_HINT_P_RESIZE_INC
#define XCB_ICCCM_WM_HINT_INPUT			XCB_WM_HINT_INPUT
#define XCB_ICCCM_WM_HINT_X_URGENCY		XCB_WM_HINT_X_URGENCY
#define XCB_ICCCM_WM_STATE_ICONIC		XCB_WM_STATE_ICONIC
#define XCB_ICCCM_WM_STATE_WITHDRAWN		XCB_WM_STATE_WITHDRAWN
#define XCB_ICCCM_WM_STATE_NORMAL		XCB_WM_STATE_NORMAL
#define xcb_icccm_get_text_property_reply_t	xcb_get_text_property_reply_t
#define xcb_icccm_get_text_property_reply_wipe	xcb_get_text_property_reply_wipe
#define xcb_icccm_get_wm_class			xcb_get_wm_class
#define xcb_icccm_get_wm_class_reply		xcb_get_wm_class_reply
#define xcb_icccm_get_wm_class_reply_t		xcb_get_wm_class_reply_t
#define xcb_icccm_get_wm_class_reply_wipe	xcb_get_wm_class_reply_wipe
#define xcb_icccm_get_wm_hints			xcb_get_wm_hints
#define xcb_icccm_wm_hints_get_urgency		xcb_wm_hints_get_urgency
#define xcb_icccm_get_wm_hints_reply		xcb_get_wm_hints_reply
#define xcb_icccm_get_wm_name			xcb_get_wm_name
#define xcb_icccm_get_wm_name_reply		xcb_get_wm_name_reply
#define xcb_icccm_get_wm_normal_hints		xcb_get_wm_normal_hints
#define xcb_icccm_get_wm_normal_hints_reply	xcb_get_wm_normal_hints_reply
#define xcb_icccm_get_wm_protocols		xcb_get_wm_protocols
#define xcb_icccm_get_wm_protocols_reply	xcb_get_wm_protocols_reply
#define xcb_icccm_get_wm_protocols_reply_t	xcb_get_wm_protocols_reply_t
#define xcb_icccm_get_wm_protocols_reply_wipe	xcb_get_wm_protocols_reply_wipe
#define xcb_icccm_get_wm_transient_for		xcb_get_wm_transient_for
#define xcb_icccm_get_wm_transient_for_reply	xcb_get_wm_transient_for_reply
#define xcb_icccm_wm_hints_t			xcb_wm_hints_t
#endif

/*#define SWM_DEBUG*/
#ifdef SWM_DEBUG
#define DPRINTF(x...) do {							\
	if (swm_debug)								\
		fprintf(stderr, x);						\
} while (0)
#define DNPRINTF(n,x...) do {							\
	if (swm_debug & n) {							\
		fprintf(stderr, "%ld ", (long)(time(NULL) - time_started));	\
		fprintf(stderr, x);						\
	}									\
} while (0)
#define SWM_D_MISC		0x0001
#define SWM_D_EVENT		0x0002
#define SWM_D_WS		0x0004
#define SWM_D_FOCUS		0x0008
#define SWM_D_MOVE		0x0010
#define SWM_D_STACK		0x0020
#define SWM_D_MOUSE		0x0040
#define SWM_D_PROP		0x0080
#define SWM_D_CLASS		0x0100
#define SWM_D_KEY		0x0200
#define SWM_D_QUIRK		0x0400
#define SWM_D_SPAWN		0x0800
#define SWM_D_EVENTQ		0x1000
#define SWM_D_CONF		0x2000
#define SWM_D_BAR		0x4000
#define SWM_D_INIT		0x8000

uint32_t		swm_debug = 0
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
			    | SWM_D_BAR
			    | SWM_D_INIT
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define SWM_EWMH_ACTION_COUNT_MAX	(8)
#define EWMH_F_FULLSCREEN		(0x001)
#define EWMH_F_ABOVE			(0x002)
#define EWMH_F_HIDDEN			(0x004)
#define EWMH_F_MAXIMIZED_VERT		(0x008)
#define EWMH_F_MAXIMIZED_HORZ		(0x010)
#define EWMH_F_SKIP_PAGER		(0x020)
#define EWMH_F_SKIP_TASKBAR		(0x040)
#define SWM_F_MANUAL			(0x080)

#define EWMH_F_MAXIMIZED	(EWMH_F_MAXIMIZED_VERT | EWMH_F_MAXIMIZED_HORZ)

/* convert 8-bit to 16-bit */
#define RGB_8_TO_16(col)	(((col) << 8) + (col))

#define PIXEL_TO_XRENDERCOLOR(px, xrc)					\
	xrc.red = RGB_8_TO_16((px) >> 16 & 0xff);			\
	xrc.green = RGB_8_TO_16((px) >> 8 & 0xff);			\
	xrc.blue = RGB_8_TO_16((px) & 0xff);				\
	xrc.alpha = 0xffff;

#define LENGTH(x)		(int)(sizeof (x) / sizeof (x)[0])
#define MODKEY			XCB_MOD_MASK_1
#define CLEANMASK(mask)		((mask) & ~(numlockmask | XCB_MOD_MASK_LOCK))
#define BUTTONMASK		(XCB_EVENT_MASK_BUTTON_PRESS |		\
    XCB_EVENT_MASK_BUTTON_RELEASE)
#define MOUSEMASK		(BUTTONMASK|XCB_EVENT_MASK_POINTER_MOTION)
#define SWM_PROPLEN		(16)
#define SWM_FUNCNAME_LEN	(32)
#define SWM_KEYS_LEN		(255)
#define SWM_QUIRK_LEN		(64)
#define X(r)			((r)->g.x)
#define Y(r)			((r)->g.y)
#define WIDTH(r)		((r)->g.w)
#define HEIGHT(r)		((r)->g.h)
#define BORDER(w)		((w)->bordered ? border_width : 0)
#define MAX_X(r)		((r)->g.x + (r)->g.w)
#define MAX_Y(r)		((r)->g.y + (r)->g.h)
#define SH_MIN(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
#define SH_MIN_W(w)		((w)->sh.min_width)
#define SH_MIN_H(w)		((w)->sh.min_height)
#define SH_MAX(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
#define SH_MAX_W(w)		((w)->sh.max_width)
#define SH_MAX_H(w)		((w)->sh.max_height)
#define SH_INC(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC)
#define SH_INC_W(w)		((w)->sh.width_inc)
#define SH_INC_H(w)		((w)->sh.height_inc)
#define SWM_MAX_FONT_STEPS	(3)
#define WINID(w)		((w) ? (w)->id : XCB_WINDOW_NONE)
#define WS_FOCUSED(ws)		((ws)->r && (ws)->r->s->r_focus == (ws)->r)
#define YESNO(x)		((x) ? "yes" : "no")
#define ICONIC(w)		((w)->ewmh_flags & EWMH_F_HIDDEN)
#define ABOVE(w)		((w)->ewmh_flags & EWMH_F_ABOVE)
#define FULLSCREEN(w)		((w)->ewmh_flags & EWMH_F_FULLSCREEN)
#define MAXIMIZED_VERT(w)	((w)->ewmh_flags & EWMH_F_MAXIMIZED_VERT)
#define MAXIMIZED_HORZ(w)	((w)->ewmh_flags & EWMH_F_MAXIMIZED_HORZ)
#define MAXIMIZED(w)		(MAXIMIZED_VERT(w) || MAXIMIZED_HORZ(w))
#define MANUAL(w)		((w)->ewmh_flags & SWM_F_MANUAL)
#define TRANS(w)		((w)->transient != XCB_WINDOW_NONE)
#define FLOATING(w)		(ABOVE(w) || TRANS(w) || FULLSCREEN(w) ||      \
    MAXIMIZED(w))

/* Constrain Window flags */
#define SWM_CW_RESIZABLE	(0x01)
#define SWM_CW_SOFTBOUNDARY	(0x02)
#define SWM_CW_HARDBOUNDARY	(0x04)
#define SWM_CW_RIGHT		(0x10)
#define SWM_CW_LEFT		(0x20)
#define SWM_CW_BOTTOM		(0x40)
#define SWM_CW_TOP		(0x80)
#define SWM_CW_ALLSIDES		(0xf0)

#define SWM_FOCUS_DEFAULT	(0)
#define SWM_FOCUS_FOLLOW	(1)
#define SWM_FOCUS_MANUAL	(2)

#define SWM_CK_NONE		(0)
#define SWM_CK_ALL		(0xf)
#define SWM_CK_FOCUS		(0x1)
#define SWM_CK_POINTER		(0x2)
#define SWM_CK_FALLBACK		(0x4)
#define SWM_CK_REGION		(0x8)

#define SWM_G_ALL		(0xf)
#define SWM_G_SIZE		(0x1)
#define SWM_G_POS		(0x2)

#define SWM_CONF_DEFAULT	(0)
#define SWM_CONF_KEYMAPPING	(1)

#ifndef SWM_LIB
#define SWM_LIB			"/usr/local/lib/libswmhack.so"
#endif

char			**start_argv;
xcb_atom_t		a_state;
xcb_atom_t		a_prot;
xcb_atom_t		a_delete;
xcb_atom_t		a_net_wm_check;
xcb_atom_t		a_net_supported;
xcb_atom_t		a_takefocus;
xcb_atom_t		a_utf8_string;
xcb_atom_t		a_swm_ws;
volatile sig_atomic_t   running = 1;
volatile sig_atomic_t   restart_wm = 0;
xcb_timestamp_t		last_event_time = 0;
int			outputs = 0;
bool			randr_support;
int			randr_eventbase;
unsigned int		numlockmask = 0;

Display			*display;
xcb_connection_t	*conn;
xcb_key_symbols_t	*syms;

int			boundary_width = 50;
bool			cycle_empty = false;
bool			cycle_visible = false;
int			term_width = 0;
int			font_adjusted = 0;
unsigned int		mod_key = MODKEY;
bool			warp_pointer = false;

/* dmenu search */
struct swm_region	*search_r;
int			select_list_pipe[2];
int			select_resp_pipe[2];
pid_t			searchpid;
volatile sig_atomic_t	search_resp;
int			search_resp_action;

struct search_window {
	TAILQ_ENTRY(search_window)	entry;
	int				idx;
	struct ws_win			*win;
	xcb_gcontext_t			gc;
	xcb_window_t			indicator;
};
TAILQ_HEAD(search_winlist, search_window);
struct search_winlist			search_wl;

/* search actions */
enum {
	SWM_SEARCH_NONE,
	SWM_SEARCH_UNICONIFY,
	SWM_SEARCH_NAME_WORKSPACE,
	SWM_SEARCH_SEARCH_WORKSPACE,
	SWM_SEARCH_SEARCH_WINDOW
};

#define SWM_STACK_TOP		(0)
#define SWM_STACK_BOTTOM	(1)
#define SWM_STACK_ABOVE		(2)
#define SWM_STACK_BELOW		(3)

/* dialog windows */
double			dialog_ratio = 0.6;
/* status bar */
#define SWM_BAR_MAX		(256)
#define SWM_BAR_JUSTIFY_LEFT	(0)
#define SWM_BAR_JUSTIFY_CENTER	(1)
#define SWM_BAR_JUSTIFY_RIGHT	(2)
#define SWM_BAR_OFFSET		(4)
#define SWM_BAR_FONTS		"-*-terminus-medium-*-*-*-12-*-*-*-*-*-*-*,"	\
				"-*-profont-*-*-*-*-12-*-*-*-*-*-*-*,"		\
				"-*-times-medium-r-*-*-12-*-*-*-*-*-*-*,"	\
				"-misc-fixed-medium-r-*-*-12-*-*-*-*-*-*-*,"	\
				"-*-*-*-r-*-*-*-*-*-*-*-*-*-*"

#ifdef X_HAVE_UTF8_STRING
#define DRAWSTRING(x...)	Xutf8DrawString(x)
#else
#define DRAWSTRING(x...)	XmbDrawString(x)
#endif

char		*bar_argv[] = { NULL, NULL };
int		 bar_pipe[2];
char		 bar_ext[SWM_BAR_MAX];
char		 bar_ext_buf[SWM_BAR_MAX];
char		 bar_vertext[SWM_BAR_MAX];
bool		 bar_version = false;
bool		 bar_enabled = true;
int		 bar_border_width = 1;
bool		 bar_at_bottom = false;
bool		 bar_extra = false;
int		 bar_height = 0;
int		 bar_justify = SWM_BAR_JUSTIFY_LEFT;
char		 *bar_format = NULL;
bool		 stack_enabled = true;
bool		 clock_enabled = true;
bool		 iconic_enabled = false;
bool		 urgent_enabled = false;
bool		 urgent_collapse = false;
char		*clock_format = NULL;
bool		 window_class_enabled = false;
bool		 window_instance_enabled = false;
bool		 window_name_enabled = false;
int		 focus_mode = SWM_FOCUS_DEFAULT;
int		 focus_close = SWM_STACK_BELOW;
bool		 focus_close_wrap = true;
int		 focus_default = SWM_STACK_TOP;
int		 spawn_position = SWM_STACK_TOP;
bool		 disable_border = false;
int		 border_width = 1;
int		 region_padding = 0;
int		 tile_gap = 0;
bool		 java_workaround = true;
bool		 verbose_layout = false;
#ifdef SWM_DEBUG
time_t		 time_started;
#endif
pid_t		 bar_pid;
XFontSet	 bar_fs;
XFontSetExtents	*bar_fs_extents;
XftFont		*bar_font;
bool		 bar_font_legacy = true;
char		*bar_fonts;
XftColor	 bar_font_color;
XftColor	 search_font_color;
struct passwd	*pwd;
char		*startup_exception;
unsigned int	 nr_exceptions = 0;

/* layout manager data */
struct swm_geometry {
	int			x;
	int			y;
	int			w;
	int			h;
};

struct swm_screen;
struct workspace;

struct swm_bar {
	xcb_window_t		id;
	xcb_pixmap_t		buffer;
	struct swm_geometry	g;
};

/* virtual "screens" */
struct swm_region {
	TAILQ_ENTRY(swm_region)	entry;
	xcb_window_t		id;
	struct swm_geometry	g;
	struct workspace	*ws;	/* current workspace on this region */
	struct workspace	*ws_prior; /* prior workspace on this region */
	struct swm_screen	*s;	/* screen idx */
	struct swm_bar		*bar;
};
TAILQ_HEAD(swm_region_list, swm_region);

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	TAILQ_ENTRY(ws_win)	stack_entry;
	xcb_window_t		id;
	xcb_window_t		transient;
	struct ws_win		*focus_child;	/* focus on child transient */
	struct swm_geometry	g;		/* current geometry */
	struct swm_geometry	g_float;	/* region coordinates */
	bool			g_floatvalid;	/* g_float geometry validity */
	bool			mapped;
	bool			bordered;
	uint32_t		ewmh_flags;
	int			font_size_boundary[SWM_MAX_FONT_STEPS];
	int			font_steps;
	int			last_inc;
	bool			can_delete;
	bool			take_focus;
	bool			java;
	uint32_t		quirks;
	struct workspace	*ws;	/* always valid */
	struct swm_screen	*s;	/* always valid, never changes */
	xcb_size_hints_t	sh;
	xcb_icccm_get_wm_class_reply_t	ch;
	xcb_icccm_wm_hints_t	hints;
};
TAILQ_HEAD(ws_win_list, ws_win);
TAILQ_HEAD(ws_win_stack, ws_win);

/* pid goo */
struct pid_e {
	TAILQ_ENTRY(pid_e)	entry;
	pid_t			pid;
	int			ws;
};
TAILQ_HEAD(pid_list, pid_e);
struct pid_list			pidlist = TAILQ_HEAD_INITIALIZER(pidlist);

/* layout handlers */
void	stack(void);
void	vertical_config(struct workspace *, int);
void	vertical_stack(struct workspace *, struct swm_geometry *);
void	horizontal_config(struct workspace *, int);
void	horizontal_stack(struct workspace *, struct swm_geometry *);
void	max_stack(struct workspace *, struct swm_geometry *);
void	plain_stacker(struct workspace *);
void	fancy_stacker(struct workspace *);

struct layout {
	void		(*l_stack)(struct workspace *, struct swm_geometry *);
	void		(*l_config)(struct workspace *, int);
	uint32_t	flags;
#define SWM_L_FOCUSPREV		(1<<0)
#define SWM_L_MAPONFOCUS	(1<<1)
	void		(*l_string)(struct workspace *);
} layouts[] =  {
	/* stack,		configure */
	{ vertical_stack,	vertical_config,	0,	plain_stacker },
	{ horizontal_stack,	horizontal_config,	0,	plain_stacker },
	{ max_stack,		NULL,
	  SWM_L_MAPONFOCUS | SWM_L_FOCUSPREV,			plain_stacker },
	{ NULL,			NULL,			0,	NULL  },
};

/* position of max_stack mode in the layouts array, index into layouts! */
#define SWM_V_STACK		(0)
#define SWM_H_STACK		(1)
#define SWM_MAX_STACK		(2)

#define SWM_H_SLICE		(32)
#define SWM_V_SLICE		(32)

/* define work spaces */
struct workspace {
	int			idx;		/* workspace index */
	char			*name;		/* workspace name */
	bool			always_raise;	/* raise windows on focus */
	bool			bar_enabled;	/* bar visibility */
	struct layout		*cur_layout;	/* current layout handlers */
	struct ws_win		*focus;		/* may be NULL */
	struct ws_win		*focus_prev;	/* may be NULL */
	struct ws_win		*focus_pending;	/* may be NULL */
	struct swm_region	*r;		/* may be NULL */
	struct swm_region	*old_r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */
	struct ws_win_list	unmanagedlist;	/* list of dead windows in ws */
	struct ws_win_stack	stack;		/* stacking order */
	int			state;		/* mapping state */
	char			stacker[10];	/* display stacker and layout */

	/* stacker state */
	struct {
				int horizontal_msize;
				int horizontal_mwin;
				int horizontal_stacks;
				bool horizontal_flip;
				int vertical_msize;
				int vertical_mwin;
				int vertical_stacks;
				bool vertical_flip;
	} l_state;
};

enum {
	SWM_WS_STATE_HIDDEN,
	SWM_WS_STATE_MAPPING,
	SWM_WS_STATE_MAPPED,
};

enum {
	SWM_S_COLOR_BAR,
	SWM_S_COLOR_BAR_BORDER,
	SWM_S_COLOR_BAR_BORDER_UNFOCUS,
	SWM_S_COLOR_BAR_FONT,
	SWM_S_COLOR_FOCUS,
	SWM_S_COLOR_FOCUS_MAXIMIZED,
	SWM_S_COLOR_UNFOCUS,
	SWM_S_COLOR_UNFOCUS_MAXIMIZED,
	SWM_S_COLOR_MAX
};

/* physical screen mapping */
#define SWM_WS_MAX		(22)	/* hard limit */
int		workspace_limit = 10;	/* soft limit */

struct swm_screen {
	int			idx;	/* screen index */
	struct swm_region_list	rl;	/* list of regions on this screen */
	struct swm_region_list	orl;	/* list of old regions */
	xcb_window_t		root;
	struct workspace	ws[SWM_WS_MAX];
	struct swm_region	*r_focus;

	/* colors */
	struct {
		uint32_t	pixel;
		char		*name;
		int		manual;
	} c[SWM_S_COLOR_MAX];

	xcb_gcontext_t		bar_gc;
	GC			bar_gc_legacy;
};
struct swm_screen	*screens;

/* args to functions */
union arg {
	int			id;
#define SWM_ARG_ID_FOCUSNEXT	(0)
#define SWM_ARG_ID_FOCUSPREV	(1)
#define SWM_ARG_ID_FOCUSMAIN	(2)
#define SWM_ARG_ID_FOCUSURGENT	(3)
#define SWM_ARG_ID_SWAPNEXT	(10)
#define SWM_ARG_ID_SWAPPREV	(11)
#define SWM_ARG_ID_SWAPMAIN	(12)
#define SWM_ARG_ID_MOVELAST	(13)
#define SWM_ARG_ID_MASTERSHRINK (20)
#define SWM_ARG_ID_MASTERGROW	(21)
#define SWM_ARG_ID_MASTERADD	(22)
#define SWM_ARG_ID_MASTERDEL	(23)
#define SWM_ARG_ID_FLIPLAYOUT	(24)
#define SWM_ARG_ID_STACKRESET	(30)
#define SWM_ARG_ID_STACKINIT	(31)
#define SWM_ARG_ID_CYCLEWS_UP	(40)
#define SWM_ARG_ID_CYCLEWS_DOWN	(41)
#define SWM_ARG_ID_CYCLERG_UP	(42)
#define SWM_ARG_ID_CYCLERG_DOWN	(43)
#define SWM_ARG_ID_CYCLEWS_UP_ALL	(44)
#define SWM_ARG_ID_CYCLEWS_DOWN_ALL	(45)
#define SWM_ARG_ID_CYCLEWS_MOVE_UP	(46)
#define SWM_ARG_ID_CYCLEWS_MOVE_DOWN	(47)
#define SWM_ARG_ID_STACKINC	(50)
#define SWM_ARG_ID_STACKDEC	(51)
#define SWM_ARG_ID_SS_ALL	(60)
#define SWM_ARG_ID_SS_WINDOW	(61)
#define SWM_ARG_ID_DONTCENTER	(70)
#define SWM_ARG_ID_CENTER	(71)
#define SWM_ARG_ID_KILLWINDOW	(80)
#define SWM_ARG_ID_DELETEWINDOW	(81)
#define SWM_ARG_ID_WIDTHGROW	(90)
#define SWM_ARG_ID_WIDTHSHRINK	(91)
#define SWM_ARG_ID_HEIGHTGROW	(92)
#define SWM_ARG_ID_HEIGHTSHRINK	(93)
#define SWM_ARG_ID_MOVEUP	(100)
#define SWM_ARG_ID_MOVEDOWN	(101)
#define SWM_ARG_ID_MOVELEFT	(102)
#define SWM_ARG_ID_MOVERIGHT	(103)
#define SWM_ARG_ID_RAISE	(105)
#define SWM_ARG_ID_LOWER	(106)
#define SWM_ARG_ID_BAR_TOGGLE	(110)
#define SWM_ARG_ID_BAR_TOGGLE_WS	(111)
	char			**argv;
};

/* quirks */
struct quirk {
	TAILQ_ENTRY(quirk)	entry;
	char			*class;		/* WM_CLASS:class */
	char			*instance;	/* WM_CLASS:instance */
	char			*name;		/* WM_NAME */
	regex_t			regex_class;
	regex_t			regex_instance;
	regex_t			regex_name;
	uint32_t		quirk;
	int			ws;		/* Initial workspace. */
#define SWM_Q_FLOAT		(1<<0)	/* float this window */
#define SWM_Q_TRANSSZ		(1<<1)	/* transiend window size too small */
#define SWM_Q_ANYWHERE		(1<<2)	/* don't position this window */
#define SWM_Q_XTERM_FONTADJ	(1<<3)	/* adjust xterm fonts when resizing */
#define SWM_Q_FULLSCREEN	(1<<4)	/* remove border */
#define SWM_Q_FOCUSPREV		(1<<5)	/* focus on caller */
#define SWM_Q_NOFOCUSONMAP	(1<<6)	/* Don't focus on window when mapped. */
#define SWM_Q_FOCUSONMAP_SINGLE	(1<<7)	/* Only focus if single win of type. */
#define SWM_Q_OBEYAPPFOCUSREQ	(1<<8)	/* Focus when applications ask. */
#define SWM_Q_IGNOREPID		(1<<9)	/* Ignore PID when determining ws. */
#define SWM_Q_IGNORESPAWNWS	(1<<10)	/* Ignore _SWM_WS when managing win. */
};
TAILQ_HEAD(quirk_list, quirk);
struct quirk_list		quirks = TAILQ_HEAD_INITIALIZER(quirks);

/*
 * Supported EWMH hints should be added to
 * both the enum and the ewmh array
 */
enum {
	_NET_ACTIVE_WINDOW,
	_NET_CLIENT_LIST,
	_NET_CLOSE_WINDOW,
	_NET_CURRENT_DESKTOP,
	_NET_DESKTOP_GEOMETRY,
	_NET_DESKTOP_NAMES,
	_NET_DESKTOP_VIEWPORT,
	_NET_MOVERESIZE_WINDOW,
	_NET_NUMBER_OF_DESKTOPS,
	_NET_RESTACK_WINDOW,
	_NET_WM_ACTION_ABOVE,
	_NET_WM_ACTION_CLOSE,
	_NET_WM_ACTION_FULLSCREEN,
	_NET_WM_ACTION_MOVE,
	_NET_WM_ACTION_RESIZE,
	_NET_WM_ALLOWED_ACTIONS,
	_NET_WM_DESKTOP,
	_NET_WM_FULL_PLACEMENT,
	_NET_WM_NAME,
	_NET_WM_STATE,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_SWM_WM_STATE_MANUAL,
	SWM_EWMH_HINT_MAX
};

struct ewmh_hint {
	char		*name;
	xcb_atom_t	atom;
} ewmh[SWM_EWMH_HINT_MAX] =	{
    /* must be in same order as in the enum */
    {"_NET_ACTIVE_WINDOW", XCB_ATOM_NONE},
    {"_NET_CLIENT_LIST", XCB_ATOM_NONE},
    {"_NET_CLOSE_WINDOW", XCB_ATOM_NONE},
    {"_NET_CURRENT_DESKTOP", XCB_ATOM_NONE},
    {"_NET_DESKTOP_GEOMETRY", XCB_ATOM_NONE},
    {"_NET_DESKTOP_NAMES", XCB_ATOM_NONE},
    {"_NET_DESKTOP_VIEWPORT", XCB_ATOM_NONE},
    {"_NET_MOVERESIZE_WINDOW", XCB_ATOM_NONE},
    {"_NET_NUMBER_OF_DESKTOPS", XCB_ATOM_NONE},
    {"_NET_RESTACK_WINDOW", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_ABOVE", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_CLOSE", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_FULLSCREEN", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_MOVE", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_RESIZE", XCB_ATOM_NONE},
    {"_NET_WM_ALLOWED_ACTIONS", XCB_ATOM_NONE},
    {"_NET_WM_DESKTOP", XCB_ATOM_NONE},
    {"_NET_WM_FULL_PLACEMENT", XCB_ATOM_NONE},
    {"_NET_WM_NAME", XCB_ATOM_NONE},
    {"_NET_WM_STATE", XCB_ATOM_NONE},
    {"_NET_WM_STATE_ABOVE", XCB_ATOM_NONE},
    {"_NET_WM_STATE_FULLSCREEN", XCB_ATOM_NONE},
    {"_NET_WM_STATE_HIDDEN", XCB_ATOM_NONE},
    {"_NET_WM_STATE_MAXIMIZED_VERT", XCB_ATOM_NONE},
    {"_NET_WM_STATE_MAXIMIZED_HORZ", XCB_ATOM_NONE},
    {"_NET_WM_STATE_SKIP_PAGER", XCB_ATOM_NONE},
    {"_NET_WM_STATE_SKIP_TASKBAR", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DIALOG", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DOCK", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_NORMAL", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_SPLASH", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_TOOLBAR", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_UTILITY", XCB_ATOM_NONE},
    {"_SWM_WM_STATE_MANUAL", XCB_ATOM_NONE},
};

/* EWMH source type */
enum {
	EWMH_SOURCE_TYPE_NONE = 0,
	EWMH_SOURCE_TYPE_NORMAL = 1,
	EWMH_SOURCE_TYPE_OTHER = 2,
};

/* Cursors */
enum {
	XC_FLEUR,
	XC_LEFT_PTR,
	XC_BOTTOM_LEFT_CORNER,
	XC_BOTTOM_RIGHT_CORNER,
	XC_SIZING,
	XC_TOP_LEFT_CORNER,
	XC_TOP_RIGHT_CORNER,
	XC_MAX
};

struct cursors {
	char		*name; /* Name used by Xcursor .*/
	uint8_t		cf_char; /* cursorfont index. */
	xcb_cursor_t	cid;
} cursors[XC_MAX] =	{
	{"fleur", XC_fleur, XCB_CURSOR_NONE},
	{"left_ptr", XC_left_ptr, XCB_CURSOR_NONE},
	{"bottom_left_corner", XC_bottom_left_corner, XCB_CURSOR_NONE},
	{"bottom_right_corner", XC_bottom_right_corner, XCB_CURSOR_NONE},
	{"sizing", XC_sizing, XCB_CURSOR_NONE},
	{"top_left_corner", XC_top_left_corner, XCB_CURSOR_NONE},
	{"top_right_corner", XC_top_right_corner, XCB_CURSOR_NONE},
};

#define SWM_SPAWN_OPTIONAL		0x1

/* spawn */
struct spawn_prog {
	TAILQ_ENTRY(spawn_prog)	entry;
	char			*name;
	int			argc;
	char			**argv;
	int			flags;
};
TAILQ_HEAD(spawn_list, spawn_prog);
struct spawn_list		spawns = TAILQ_HEAD_INITIALIZER(spawns);

/* user/key callable function IDs */
enum keyfuncid {
	KF_BAR_TOGGLE,
	KF_BAR_TOGGLE_WS,
	KF_BUTTON2,
	KF_CYCLE_LAYOUT,
	KF_FLIP_LAYOUT,
	KF_FLOAT_TOGGLE,
	KF_FOCUS_MAIN,
	KF_FOCUS_NEXT,
	KF_FOCUS_PREV,
	KF_FOCUS_URGENT,
	KF_MAXIMIZE_TOGGLE,
	KF_HEIGHT_GROW,
	KF_HEIGHT_SHRINK,
	KF_ICONIFY,
	KF_MASTER_SHRINK,
	KF_MASTER_GROW,
	KF_MASTER_ADD,
	KF_MASTER_DEL,
	KF_MOVE_DOWN,
	KF_MOVE_LEFT,
	KF_MOVE_RIGHT,
	KF_MOVE_UP,
	KF_MVRG_1,
	KF_MVRG_2,
	KF_MVRG_3,
	KF_MVRG_4,
	KF_MVRG_5,
	KF_MVRG_6,
	KF_MVRG_7,
	KF_MVRG_8,
	KF_MVRG_9,
	KF_MVWS_1,
	KF_MVWS_2,
	KF_MVWS_3,
	KF_MVWS_4,
	KF_MVWS_5,
	KF_MVWS_6,
	KF_MVWS_7,
	KF_MVWS_8,
	KF_MVWS_9,
	KF_MVWS_10,
	KF_MVWS_11,
	KF_MVWS_12,
	KF_MVWS_13,
	KF_MVWS_14,
	KF_MVWS_15,
	KF_MVWS_16,
	KF_MVWS_17,
	KF_MVWS_18,
	KF_MVWS_19,
	KF_MVWS_20,
	KF_MVWS_21,
	KF_MVWS_22,
	KF_NAME_WORKSPACE,
	KF_QUIT,
	KF_RAISE_TOGGLE,
	KF_RESTART,
	KF_RG_1,
	KF_RG_2,
	KF_RG_3,
	KF_RG_4,
	KF_RG_5,
	KF_RG_6,
	KF_RG_7,
	KF_RG_8,
	KF_RG_9,
	KF_RG_NEXT,
	KF_RG_PREV,
	KF_SCREEN_NEXT,
	KF_SCREEN_PREV,
	KF_SEARCH_WIN,
	KF_SEARCH_WORKSPACE,
	KF_SPAWN_CUSTOM,
	KF_STACK_INC,
	KF_STACK_DEC,
	KF_STACK_RESET,
	KF_SWAP_MAIN,
	KF_SWAP_NEXT,
	KF_SWAP_PREV,
	KF_UNICONIFY,
	KF_VERSION,
	KF_WIDTH_GROW,
	KF_WIDTH_SHRINK,
	KF_WIND_DEL,
	KF_WIND_KILL,
	KF_WS_1,
	KF_WS_2,
	KF_WS_3,
	KF_WS_4,
	KF_WS_5,
	KF_WS_6,
	KF_WS_7,
	KF_WS_8,
	KF_WS_9,
	KF_WS_10,
	KF_WS_11,
	KF_WS_12,
	KF_WS_13,
	KF_WS_14,
	KF_WS_15,
	KF_WS_16,
	KF_WS_17,
	KF_WS_18,
	KF_WS_19,
	KF_WS_20,
	KF_WS_21,
	KF_WS_22,
	KF_WS_NEXT,
	KF_WS_NEXT_ALL,
	KF_WS_NEXT_MOVE,
	KF_WS_PREV,
	KF_WS_PREV_ALL,
	KF_WS_PREV_MOVE,
	KF_WS_PRIOR,
	KF_DUMPWINS, /* MUST BE LAST */
	KF_INVALID
};

struct key {
        RB_ENTRY(key)           entry;
        unsigned int            mod;
        KeySym                  keysym;
        enum keyfuncid          funcid;
        char                    *spawn_name;
};
RB_HEAD(key_tree, key);

/* function prototypes */
void	 adjust_font(struct ws_win *);
char	*argsep(char **);
void	 bar_cleanup(struct swm_region *);
void	 bar_extra_setup(void);
void	 bar_extra_stop(void);
int	 bar_extra_update(void);
void	 bar_fmt(const char *, char *, struct swm_region *, size_t);
void	 bar_fmt_expand(char *, size_t);
void	 bar_draw(void);
void	 bar_print(struct swm_region *, const char *);
void	 bar_print_legacy(struct swm_region *, const char *);
void	 bar_replace(char *, char *, struct swm_region *, size_t);
void	 bar_replace_pad(char *, int *, size_t);
char	*bar_replace_seq(char *, char *, struct swm_region *, size_t *, size_t);
void	 bar_setup(struct swm_region *);
void	 bar_toggle(struct swm_region *, union arg *);
void	 bar_urgent(char *, size_t);
void	 bar_window_class(char *, size_t, struct swm_region *);
void	 bar_window_class_instance(char *, size_t, struct swm_region *);
void	 bar_window_float(char *, size_t, struct swm_region *);
void	 bar_window_instance(char *, size_t, struct swm_region *);
void	 bar_window_name(char *, size_t, struct swm_region *);
void	 bar_window_state(char *, size_t, struct swm_region *);
void	 bar_workspace_name(char *, size_t, struct swm_region *);
void	 buttonpress(xcb_button_press_event_t *);
void	 center_pointer(struct swm_region *);
void	 check_conn(void);
void	 clear_keys(void);
int	 clear_maximized(struct workspace *);
void	 clientmessage(xcb_client_message_event_t *);
void	 client_msg(struct ws_win *, xcb_atom_t, xcb_timestamp_t);
int	 conf_load(const char *, int);
void	 configurenotify(xcb_configure_notify_event_t *);
void	 configurerequest(xcb_configure_request_event_t *);
void	 config_win(struct ws_win *, xcb_configure_request_event_t *);
void	 constrain_window(struct ws_win *, struct swm_geometry *, int *);
int	 count_win(struct workspace *, bool);
void	 cursors_cleanup(void);
void	 cursors_load(void);
void	 custom_region(const char *);
void	 cyclerg(struct swm_region *, union arg *);
void	 cyclews(struct swm_region *, union arg *);
void	 cycle_layout(struct swm_region *, union arg *);
void	 destroynotify(xcb_destroy_notify_event_t *);
void	 dumpwins(struct swm_region *, union arg *);
int	 enable_wm(void);
void	 enternotify(xcb_enter_notify_event_t *);
void	 event_drain(uint8_t);
void	 event_error(xcb_generic_error_t *);
void	 event_handle(xcb_generic_event_t *);
void	 ewmh_apply_flags(struct ws_win *, uint32_t);
void	 ewmh_autoquirk(struct ws_win *);
void	 ewmh_get_desktop_names(void);
void	 ewmh_get_wm_state(struct ws_win *);
void	 ewmh_update_actions(struct ws_win *);
void	 ewmh_update_client_list(void);
void	 ewmh_update_current_desktop(void);
void	 ewmh_update_desktop_names(void);
void	 ewmh_update_desktops(void);
void	 ewmh_change_wm_state(struct ws_win *, xcb_atom_t, long);
void	 ewmh_update_wm_state(struct ws_win *);
char	*expand_tilde(const char *);
void	 expose(xcb_expose_event_t *);
void	 fake_keypress(struct ws_win *, xcb_keysym_t, uint16_t);
struct pid_e	*find_pid(pid_t);
struct ws_win	*find_unmanaged_window(xcb_window_t);
struct ws_win	*find_window(xcb_window_t);
void	 floating_toggle(struct swm_region *, union arg *);
void	 focus(struct swm_region *, union arg *);
#ifdef SWM_DEBUG
void	 focusin(xcb_focus_in_event_t *);
void	 focusout(xcb_focus_out_event_t *);
#endif
void	 focus_flush(void);
void	 focus_region(struct swm_region *);
void	 focusrg(struct swm_region *, union arg *);
void	 focus_win(struct ws_win *);
void	 fontset_init(void);
void	 free_window(struct ws_win *);
xcb_atom_t get_atom_from_string(const char *);
#ifdef SWM_DEBUG
char	*get_atom_name(xcb_atom_t);
#endif
struct ws_win   *get_focus_magic(struct ws_win *);
struct ws_win   *get_focus_prev(struct ws_win *);
#ifdef SWM_DEBUG
char	*get_notify_detail_label(uint8_t);
char	*get_notify_mode_label(uint8_t);
#endif
struct ws_win	*get_pointer_win(xcb_window_t);
struct ws_win	*get_region_focus(struct swm_region *);
int	 get_region_index(struct swm_region *);
xcb_screen_t	*get_screen(int);
int	 get_screen_count(void);
#ifdef SWM_DEBUG
char	*get_source_type_label(uint32_t);
char	*get_stack_mode_name(uint8_t);
#endif
int32_t	 get_swm_ws(xcb_window_t);
bool	 get_urgent(struct ws_win *);
char	*get_win_name(xcb_window_t);
uint8_t	 get_win_state(xcb_window_t);
void	 get_wm_protocols(struct ws_win *);
int	 get_ws_idx(struct ws_win *);
void	 grabbuttons(struct ws_win *);
void	 grabkeys(void);
void	 grab_windows(void);
void	 iconify(struct swm_region *, union arg *);
bool	 isxlfd(char *);
void	 keypress(xcb_key_press_event_t *);
int	 key_cmp(struct key *, struct key *);
void	 key_insert(unsigned int, KeySym, enum keyfuncid, const char *);
struct key	*key_lookup(unsigned int, KeySym);
void	 key_remove(struct key *);
void	 key_replace(struct key *, unsigned int, KeySym, enum keyfuncid,
	     const char *);
void	 kill_bar_extra_atexit(void);
void	 kill_refs(struct ws_win *);
#ifdef SWM_DEBUG
void	 leavenotify(xcb_leave_notify_event_t *);
#endif
void	 load_float_geom(struct ws_win *);
void	 lower_window(struct ws_win *);
struct ws_win	*manage_window(xcb_window_t, int, bool);
void	 map_window(struct ws_win *);
void	 mapnotify(xcb_map_notify_event_t *);
void	 mappingnotify(xcb_mapping_notify_event_t *);
void	 maprequest(xcb_map_request_event_t *);
void	 maximize_toggle(struct swm_region *, union arg *);
void	 motionnotify(xcb_motion_notify_event_t *);
void	 move(struct ws_win *, union arg *);
void	 move_step(struct swm_region *, union arg *);
uint32_t name_to_pixel(int, const char *);
void	 name_workspace(struct swm_region *, union arg *);
void	 new_region(struct swm_screen *, int, int, int, int);
int	 parsekeys(const char *, unsigned int, unsigned int *, KeySym *);
int	 parsequirks(const char *, uint32_t *, int *);
int	 parse_rgb(const char *, uint16_t *, uint16_t *, uint16_t *);
void	 pressbutton(struct swm_region *, union arg *);
void	 priorws(struct swm_region *, union arg *);
#ifdef SWM_DEBUG
void	 print_win_geom(xcb_window_t);
#endif
void	 propertynotify(xcb_property_notify_event_t *);
void	 quirk_free(struct quirk *);
void	 quirk_insert(const char *, const char *, const char *, uint32_t, int);
void	 quirk_remove(struct quirk *);
void	 quirk_replace(struct quirk *, const char *, const char *, const char *,
	     uint32_t, int);
void	 quit(struct swm_region *, union arg *);
void	 raise_toggle(struct swm_region *, union arg *);
void	 raise_window(struct ws_win *);
void	 region_containment(struct ws_win *, struct swm_region *, int);
struct swm_region	*region_under(struct swm_screen *, int, int);
void	 regionize(struct ws_win *, int, int);
void	 resize(struct ws_win *, union arg *);
void	 resize_step(struct swm_region *, union arg *);
void	 restart(struct swm_region *, union arg *);
struct swm_region	*root_to_region(xcb_window_t, int);
void	 screenchange(xcb_randr_screen_change_notify_event_t *);
void	 scan_randr(int);
void	 search_do_resp(void);
void	 search_resp_name_workspace(const char *, size_t);
void	 search_resp_search_window(const char *);
void	 search_resp_search_workspace(const char *);
void	 search_resp_uniconify(const char *, size_t);
void	 search_win(struct swm_region *, union arg *);
void	 search_win_cleanup(void);
void	 search_workspace(struct swm_region *, union arg *);
void	 send_to_rg(struct swm_region *, union arg *);
void	 send_to_ws(struct swm_region *, union arg *);
void	 set_region(struct swm_region *);
int	 setautorun(const char *, const char *, int);
int	 setconfbinding(const char *, const char *, int);
int	 setconfcolor(const char *, const char *, int);
int	 setconfmodkey(const char *, const char *, int);
int	 setconfquirk(const char *, const char *, int);
int	 setconfregion(const char *, const char *, int);
int	 setconfspawn(const char *, const char *, int);
int	 setconfvalue(const char *, const char *, int);
void	 setkeybinding(unsigned int, KeySym, enum keyfuncid, const char *);
int	 setkeymapping(const char *, const char *, int);
int	 setlayout(const char *, const char *, int);
void	 setquirk(const char *, const char *, const char *, uint32_t, int);
void	 setscreencolor(const char *, int, int);
void	 setspawn(const char *, const char *, int);
void	 setup_ewmh(void);
void	 setup_globals(void);
void	 setup_keys(void);
void	 setup_quirks(void);
void	 setup_screens(void);
void	 setup_spawn(void);
void	 set_child_transient(struct ws_win *, xcb_window_t *);
void	 set_win_state(struct ws_win *, uint8_t);
void	 shutdown_cleanup(void);
void	 sighdlr(int);
void	 socket_setnonblock(int);
void	 sort_windows(struct ws_win_list *);
void	 spawn(int, union arg *, bool);
void	 spawn_custom(struct swm_region *, union arg *, const char *);
int	 spawn_expand(struct swm_region *, union arg *, const char *, char ***);
void	 spawn_insert(const char *, const char *, int);
struct spawn_prog	*spawn_find(const char *);
void	 spawn_remove(struct spawn_prog *);
void	 spawn_replace(struct spawn_prog *, const char *, const char *, int);
void	 spawn_select(struct swm_region *, union arg *, const char *, int *);
void	 stack_config(struct swm_region *, union arg *);
void	 stack_master(struct workspace *, struct swm_geometry *, int, bool);
void	 store_float_geom(struct ws_win *);
char	*strdupsafe(const char *);
void	 swapwin(struct swm_region *, union arg *);
void	 switchws(struct swm_region *, union arg *);
void	 teardown_ewmh(void);
void	 unescape_selector(char *);
void	 unfocus_win(struct ws_win *);
void	 uniconify(struct swm_region *, union arg *);
void	 unmanage_window(struct ws_win *);
void	 unmapnotify(xcb_unmap_notify_event_t *);
void	 unmap_all(void);
void	 unmap_window(struct ws_win *);
void	 updatenumlockmask(void);
void	 update_floater(struct ws_win *);
void	 update_modkey(unsigned int);
void	 update_win_stacking(struct ws_win *);
void	 update_window(struct ws_win *);
void	 update_window_color(struct ws_win *);
void	 update_wm_state(struct  ws_win *win);
void	 validate_spawns(void);
int	 validate_win(struct ws_win *);
int	 validate_ws(struct workspace *);
void	 version(struct swm_region *, union arg *);
void	 win_to_ws(struct ws_win *, int, bool);
pid_t	 window_get_pid(xcb_window_t);
void	 wkill(struct swm_region *, union arg *);
void	 update_ws_stack(struct workspace *);
void	 xft_init(struct swm_region *);
void	 _add_startup_exception(const char *, va_list);
void	 add_startup_exception(const char *, ...);

RB_PROTOTYPE(key_tree, key, entry, key_cmp);
RB_GENERATE(key_tree, key, entry, key_cmp);
struct key_tree                 keys;

void
cursors_load(void)
{
	xcb_font_t	cf = XCB_NONE;
	int		i;

	for (i = 0; i < LENGTH(cursors); ++i) {
		/* try to load Xcursor first. */
		cursors[i].cid = XcursorLibraryLoadCursor(display,
		    cursors[i].name);

		/* fallback to cursorfont. */
		if (cursors[i].cid == XCB_CURSOR_NONE) {
			if (cf == XCB_NONE) {
				cf = xcb_generate_id(conn);
				xcb_open_font(conn, cf, strlen("cursor"),
				    "cursor");
			}

			cursors[i].cid = xcb_generate_id(conn);
			xcb_create_glyph_cursor(conn, cursors[i].cid, cf, cf,
			    cursors[i].cf_char, cursors[i].cf_char + 1, 0, 0, 0,
			    0xffff, 0xffff, 0xffff);

		}
	}

	if (cf != XCB_NONE)
		xcb_close_font(conn, cf);
}

void
cursors_cleanup(void)
{
	int	i;
	for (i = 0; i < LENGTH(cursors); ++i)
		xcb_free_cursor(conn, cursors[i].cid);
}

char *
expand_tilde(const char *s)
{
	struct passwd           *ppwd;
	int                     i;
	long			max;
	char                    *user;
	const char              *sc = s;
	char			*result;

	if (s == NULL)
		errx(1, "expand_tilde: NULL string.");

	if (s[0] != '~') {
		result = strdup(sc);
		goto out;
	}

	++s;

	if ((max = sysconf(_SC_LOGIN_NAME_MAX)) == -1)
		errx(1, "expand_tilde: sysconf");

	if ((user = calloc(1, max + 1)) == NULL)
		errx(1, "expand_tilde: calloc");

	for (i = 0; s[i] != '/' && s[i] != '\0'; ++i)
		user[i] = s[i];
	user[i] = '\0';
	s = &s[i];

	ppwd = strlen(user) == 0 ? getpwuid(getuid()) : getpwnam(user);
	free(user);

	if (ppwd == NULL)
		result = strdup(sc);
	else
		if (asprintf(&result, "%s%s", ppwd->pw_dir, s) == -1)
			result = NULL;
out:
	if (result == NULL)
		errx(1, "expand_tilde: failed to allocate memory.");

	return result;
}

int
parse_rgb(const char *rgb, uint16_t *rr, uint16_t *gg, uint16_t *bb)
{
	unsigned int	tmpr, tmpg, tmpb;

	if (sscanf(rgb, "rgb:%x/%x/%x", &tmpr, &tmpg, &tmpb) != 3)
		return (-1);

	*rr = RGB_8_TO_16(tmpr);
	*gg = RGB_8_TO_16(tmpg);
	*bb = RGB_8_TO_16(tmpb);

	return (0);
}

xcb_screen_t *
get_screen(int screen)
{
	const xcb_setup_t	*r;
	xcb_screen_iterator_t	iter;

	if ((r = xcb_get_setup(conn)) == NULL) {
		DNPRINTF(SWM_D_MISC, "get_screen: xcb_get_setup\n");
		check_conn();
	}

	iter = xcb_setup_roots_iterator(r);
	for (; iter.rem; --screen, xcb_screen_next(&iter))
		if (screen == 0)
			return (iter.data);

	return (NULL);
}

int
get_screen_count(void)
{
	const xcb_setup_t	*r;

	if ((r = xcb_get_setup(conn)) == NULL) {
		DNPRINTF(SWM_D_MISC, "get_screen_count: xcb_get_setup\n");
		check_conn();
	}

	return xcb_setup_roots_length(r);
}

int
get_region_index(struct swm_region *r)
{
	struct swm_region	*rr;
	int			 ridx = 0;

	if (r == NULL)
		return -1;

	TAILQ_FOREACH(rr, &r->s->rl, entry) {
		if (rr == r)
			break;
		++ridx;
	}

	if (rr == NULL)
		return -1;

	return ridx;
}

void
focus_flush(void)
{
	if (focus_mode == SWM_FOCUS_DEFAULT)
		event_drain(XCB_ENTER_NOTIFY);
	else
		xcb_flush(conn);
}

xcb_atom_t
get_atom_from_string(const char *str)
{
	xcb_intern_atom_cookie_t	c;
	xcb_intern_atom_reply_t		*r;
	xcb_atom_t			atom;

	c = xcb_intern_atom(conn, 0, strlen(str), str);
	r = xcb_intern_atom_reply(conn, c, NULL);
	if (r) {
		atom = r->atom;
		free(r);

		return (atom);
	}

	return (XCB_ATOM_NONE);
}

void
get_wm_protocols(struct ws_win *win) {
	int				i;
	xcb_icccm_get_wm_protocols_reply_t	wpr;

	if (xcb_icccm_get_wm_protocols_reply(conn,
	    xcb_icccm_get_wm_protocols(conn, win->id, a_prot),
	    &wpr, NULL)) {
		for (i = 0; i < (int)wpr.atoms_len; i++) {
			if (wpr.atoms[i] == a_takefocus)
				win->take_focus = true;
			if (wpr.atoms[i] == a_delete)
				win->can_delete = true;
		}
		xcb_icccm_get_wm_protocols_reply_wipe(&wpr);
	}
}

void
setup_ewmh(void)
{
	xcb_window_t			root, win;
	int				i, j, num_screens;

	for (i = 0; i < LENGTH(ewmh); i++)
		ewmh[i].atom = get_atom_from_string(ewmh[i].name);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		root = screens[i].root;

		/* Set up _NET_SUPPORTING_WM_CHECK. */
		win = xcb_generate_id(conn);
		xcb_create_window(conn, XCB_COPY_FROM_PARENT, win, root,
		    0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		    XCB_COPY_FROM_PARENT, 0, NULL);

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
		    a_net_wm_check, XCB_ATOM_WINDOW, 32, 1, &win);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
		    a_net_wm_check, XCB_ATOM_WINDOW, 32, 1, &win);

		/*
		 * Impersonate LG3D non-reparenting WM, written by Sun, to
		 * workaround a Java GUI rendering issue.
		 */
		if (java_workaround)
			xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    ewmh[_NET_WM_NAME].atom, a_utf8_string,
			    8, strlen("LG3D"), "LG3D");
		else
			xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    ewmh[_NET_WM_NAME].atom, a_utf8_string,
			    8, strlen("spectrwm"), "spectrwm");

		/* Report supported atoms */
		xcb_delete_property(conn, root, a_net_supported);
		for (j = 0; j < LENGTH(ewmh); j++)
			xcb_change_property(conn, XCB_PROP_MODE_APPEND, root,
			    a_net_supported, XCB_ATOM_ATOM, 32, 1,
			    &ewmh[j].atom);

	}

	ewmh_update_desktops();
	ewmh_get_desktop_names();
}

void
teardown_ewmh(void)
{
	int				i, num_screens;
	xcb_window_t			id;
	xcb_get_property_cookie_t	pc;
	xcb_get_property_reply_t	*pr;

	num_screens = get_screen_count();

	for (i = 0; i < num_screens; i++) {
		/* Get the support check window and destroy it */
		pc = xcb_get_property(conn, 0, screens[i].root, a_net_wm_check,
		    XCB_ATOM_WINDOW, 0, 1);
		pr = xcb_get_property_reply(conn, pc, NULL);
		if (pr == NULL)
			continue;
		if (pr->format == a_net_wm_check) {
			id = *((xcb_window_t *)xcb_get_property_value(pr));

			xcb_destroy_window(conn, id);
			xcb_delete_property(conn, screens[i].root,
			    a_net_wm_check);
			xcb_delete_property(conn, screens[i].root,
			    a_net_supported);
		}
		free(pr);
	}
}

void
ewmh_autoquirk(struct ws_win *win)
{
	xcb_get_property_reply_t	*r;
	xcb_get_property_cookie_t	c;
	xcb_atom_t			*type;
	int				i, n;

	c = xcb_get_property(conn, 0, win->id,
	    ewmh[_NET_WM_WINDOW_TYPE].atom, XCB_ATOM_ATOM, 0, UINT32_MAX);
	r = xcb_get_property_reply(conn, c, NULL);
	if (r == NULL)
		return;

	type = xcb_get_property_value(r);
	n = xcb_get_property_value_length(r) / sizeof(xcb_atom_t);

	for (i = 0; i < n; i++) {
		if (type[i] == ewmh[_NET_WM_WINDOW_TYPE_NORMAL].atom)
			break;
		if (type[i] == ewmh[_NET_WM_WINDOW_TYPE_DOCK].atom ||
		    type[i] == ewmh[_NET_WM_WINDOW_TYPE_TOOLBAR].atom ||
		    type[i] == ewmh[_NET_WM_WINDOW_TYPE_UTILITY].atom) {
			win->quirks = SWM_Q_FLOAT | SWM_Q_ANYWHERE;
			break;
		}
		if (type[i] == ewmh[_NET_WM_WINDOW_TYPE_SPLASH].atom ||
		    type[i] == ewmh[_NET_WM_WINDOW_TYPE_DIALOG].atom) {
			win->quirks = SWM_Q_FLOAT;
			break;
		}
	}
	free(r);
}

void
ewmh_update_actions(struct ws_win *win)
{
	xcb_atom_t		actions[SWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (win == NULL)
		return;

	actions[n++] = ewmh[_NET_WM_ACTION_CLOSE].atom;

	if (ABOVE(win)) {
		actions[n++] = ewmh[_NET_WM_ACTION_MOVE].atom;
		actions[n++] = ewmh[_NET_WM_ACTION_RESIZE].atom;
		actions[n++] = ewmh[_NET_WM_ACTION_ABOVE].atom;
	}

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
	    ewmh[_NET_WM_ALLOWED_ACTIONS].atom, XCB_ATOM_ATOM, 32, 1, actions);
}

#define _NET_WM_STATE_REMOVE	0    /* remove/unset property */
#define _NET_WM_STATE_ADD	1    /* add/set property */
#define _NET_WM_STATE_TOGGLE	2    /* toggle property */

void
ewmh_change_wm_state(struct ws_win *win, xcb_atom_t state, long action)
{
	uint32_t		flag = 0;
	uint32_t		new_flags;
#ifdef SWM_DEBUG
	char			*name;

	name = get_atom_name(state);
	DNPRINTF(SWM_D_PROP, "ewmh_change_wm_state: win %#x, state: %s, "
	    "action: %ld\n", WINID(win), name, action);
	free(name);
#endif
	if (win == NULL)
		goto out;

	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		flag = EWMH_F_FULLSCREEN;
	else if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		flag = EWMH_F_ABOVE;
	else if (state == ewmh[_NET_WM_STATE_HIDDEN].atom)
		flag = EWMH_F_HIDDEN;
	else if (state == ewmh[_NET_WM_STATE_MAXIMIZED_VERT].atom ||
	    state == ewmh[_NET_WM_STATE_MAXIMIZED_HORZ].atom)
		flag = EWMH_F_MAXIMIZED;
	else if (state == ewmh[_SWM_WM_STATE_MANUAL].atom)
		flag = SWM_F_MANUAL;
	else if (state == ewmh[_NET_WM_STATE_SKIP_PAGER].atom)
		flag = EWMH_F_SKIP_PAGER;
	else if (state == ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom)
		flag = EWMH_F_SKIP_TASKBAR;

	/* Disallow unfloating transients. */
	if (TRANS(win) && flag == EWMH_F_ABOVE)
		goto out;

	new_flags = win->ewmh_flags;

	switch (action) {
	case _NET_WM_STATE_REMOVE:
		new_flags &= ~flag;
		break;
	case _NET_WM_STATE_ADD:
		new_flags |= flag;
		break;
	case _NET_WM_STATE_TOGGLE:
		new_flags ^= flag;
		break;
	}

	ewmh_apply_flags(win, new_flags);

out:
	DNPRINTF(SWM_D_PROP, "ewmh_change_wm_state: done.\n");
}

void
ewmh_apply_flags(struct ws_win *win, uint32_t pending)
{
	struct workspace	*ws;
	uint32_t		changed;

	changed = win->ewmh_flags ^ pending;
	if (changed == 0)
		return;

	DNPRINTF(SWM_D_PROP, "ewmh_apply_flags: pending: %d\n", pending);

	win->ewmh_flags = pending;
	ws = win->ws;

	if (changed & EWMH_F_HIDDEN) {
		if (ICONIC(win)) {
			if (focus_mode != SWM_FOCUS_FOLLOW)
				ws->focus_pending = get_focus_prev(win);

			unfocus_win(win);
			unmap_window(win);
		} else {
			/* Reload floating geometry in case region changed. */
			if (FLOATING(win))
				load_float_geom(win);

			/* The window is no longer iconic, prepare focus. */
			if (focus_mode != SWM_FOCUS_FOLLOW)
				ws->focus_pending = get_focus_magic(win);
			raise_window(win);
		}
	}

	if (changed & EWMH_F_ABOVE) {
		if (ws->cur_layout != &layouts[SWM_MAX_STACK]) {
			if (ABOVE(win))
				load_float_geom(win);
			else if (!MAXIMIZED(win))
				store_float_geom(win);

			win->ewmh_flags &= ~EWMH_F_MAXIMIZED;
			changed &= ~EWMH_F_MAXIMIZED;
			raise_window(win);
		} else {
			/* Revert. */
			win->ewmh_flags ^= EWMH_F_ABOVE & pending;
		}
	}

	if (changed & EWMH_F_MAXIMIZED) {
		/* VERT and/or HORZ changed. */
		if (ABOVE(win)) {
			if (!MAXIMIZED(win))
				load_float_geom(win);
			else
				store_float_geom(win);
		}

		if (MAXIMIZED(win)) {
			if (focus_mode != SWM_FOCUS_FOLLOW &&
			    ws->cur_layout != &layouts[SWM_MAX_STACK]) {
				if (WS_FOCUSED(ws))
					focus_win(win);
				else
					ws->focus_pending = win;
			}
		}

		update_window_color(win);
		raise_window(win);
	}

	if (changed & EWMH_F_FULLSCREEN) {
		if (FULLSCREEN(win)) {
			if (focus_mode != SWM_FOCUS_FOLLOW) {
				if (WS_FOCUSED(ws))
					focus_win(win);
				else
					ws->focus_pending = win;
			}
		} else {
			load_float_geom(win);
		}

		win->ewmh_flags &= ~EWMH_F_MAXIMIZED;
		raise_window(win);
	}

	DNPRINTF(SWM_D_PROP, "ewmh_apply_flags: done.\n");
}

void
ewmh_update_wm_state(struct  ws_win *win) {
	xcb_atom_t		vals[SWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (ICONIC(win))
		vals[n++] = ewmh[_NET_WM_STATE_HIDDEN].atom;
	if (FULLSCREEN(win))
		vals[n++] = ewmh[_NET_WM_STATE_FULLSCREEN].atom;
	if (MAXIMIZED_VERT(win))
		vals[n++] = ewmh[_NET_WM_STATE_MAXIMIZED_VERT].atom;
	if (MAXIMIZED_HORZ(win))
		vals[n++] = ewmh[_NET_WM_STATE_MAXIMIZED_HORZ].atom;
	if (win->ewmh_flags & EWMH_F_SKIP_PAGER)
		vals[n++] = ewmh[_NET_WM_STATE_SKIP_PAGER].atom;
	if (win->ewmh_flags & EWMH_F_SKIP_TASKBAR)
		vals[n++] = ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom;
	if (win->ewmh_flags & EWMH_F_ABOVE)
		vals[n++] = ewmh[_NET_WM_STATE_ABOVE].atom;
	if (win->ewmh_flags & SWM_F_MANUAL)
		vals[n++] = ewmh[_SWM_WM_STATE_MANUAL].atom;

	if (n > 0)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
		    ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, n, vals);
	else
		xcb_delete_property(conn, win->id, ewmh[_NET_WM_STATE].atom);
}

void
ewmh_get_wm_state(struct ws_win *win)
{
	xcb_atom_t			*states;
	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;
	int				i, n;

	if (win == NULL)
		return;

	win->ewmh_flags = 0;

	c = xcb_get_property(conn, 0, win->id, ewmh[_NET_WM_STATE].atom,
	    XCB_ATOM_ATOM, 0, UINT32_MAX);
	r = xcb_get_property_reply(conn, c, NULL);
	if (r == NULL)
		return;

	states = xcb_get_property_value(r);
	n = xcb_get_property_value_length(r) / sizeof(xcb_atom_t);

	for (i = 0; i < n; i++)
		ewmh_change_wm_state(win, states[i], _NET_WM_STATE_ADD);

	free(r);
}

/* events */
#ifdef SWM_DEBUG
void
dumpwins(struct swm_region *r, union arg *args)
{
	struct ws_win				*w;
	uint32_t				state;
	xcb_get_window_attributes_cookie_t	c;
	xcb_get_window_attributes_reply_t	*wa;

	/* suppress unused warning since var is needed */
	(void)args;

	if (r->ws == NULL) {
		DPRINTF("dumpwins: invalid workspace\n");
		return;
	}

	DPRINTF("=== managed window list ws %02d ===\n", r->ws->idx);
	TAILQ_FOREACH(w, &r->ws->winlist, entry) {
		state = get_win_state(w->id);
		c = xcb_get_window_attributes(conn, w->id);
		wa = xcb_get_window_attributes_reply(conn, c, NULL);
		if (wa) {
			DPRINTF("win %#x, map_state: %d, state: %u, "
			    "transient: %#x\n", w->id, wa->map_state,
			    state, w->transient);
			free(wa);
		} else
			DPRINTF("win %#x, failed xcb_get_window_attributes\n",
			    w->id);
	}

	DPRINTF("=== stacking order (top down) === \n");
	TAILQ_FOREACH(w, &r->ws->stack, stack_entry) {
		DPRINTF("win %#x, fs: %s, maximized: %s, above: %s, "
		    "iconic: %s\n", w->id, YESNO(FULLSCREEN(w)),
		    YESNO(MAXIMIZED(w)), YESNO(ABOVE(w)), YESNO(ICONIC(w)));
	}

	DPRINTF("===== unmanaged window list =====\n");
	TAILQ_FOREACH(w, &r->ws->unmanagedlist, entry) {
		state = get_win_state(w->id);
		c = xcb_get_window_attributes(conn, w->id);
		wa = xcb_get_window_attributes_reply(conn, c, NULL);
		if (wa) {
			DPRINTF("win %#x, map_state: %d, state: %u, "
			    "transient: %#x\n", w->id, wa->map_state,
			    state, w->transient);
			free(wa);
		} else
			DPRINTF("win %#x, failed xcb_get_window_attributes\n",
			    w->id);
	}

	DPRINTF("=================================\n");
}
#else
void
dumpwins(struct swm_region *r, union arg *s)
{
	(void)r;
	(void)s;
}
#endif /* SWM_DEBUG */

void
sighdlr(int sig)
{
	int			saved_errno, status;
	pid_t			pid;

	saved_errno = errno;

	switch (sig) {
	case SIGCHLD:
		while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) != 0) {
			if (pid == -1) {
				if (errno == EINTR)
					continue;
#ifdef SWM_DEBUG
				if (errno != ECHILD)
					warn("sighdlr: waitpid");
#endif /* SWM_DEBUG */
				break;
			}
			if (pid == searchpid)
				search_resp = 1;

#ifdef SWM_DEBUG
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0)
					warnx("sighdlr: child exit status: %d",
					    WEXITSTATUS(status));
			} else
				warnx("sighdlr: child is terminated "
				    "abnormally");
#endif /* SWM_DEBUG */
		}
		break;

	case SIGHUP:
		restart_wm = 1;
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		running = 0;
		break;
	}

	errno = saved_errno;
}

struct pid_e *
find_pid(pid_t pid)
{
	struct pid_e		*p = NULL;

	DNPRINTF(SWM_D_MISC, "find_pid: %d\n", pid);

	if (pid == 0)
		return (NULL);

	TAILQ_FOREACH(p, &pidlist, entry) {
		if (p->pid == pid)
			return (p);
	}

	return (NULL);
}

uint32_t
name_to_pixel(int sidx, const char *colorname)
{
	uint32_t			result = 0;
	char				cname[32] = "#";
	xcb_screen_t			*screen;
	xcb_colormap_t			cmap;
	xcb_alloc_color_reply_t		*cr;
	xcb_alloc_named_color_reply_t	*nr;
	uint16_t			rr, gg, bb;

	screen = get_screen(sidx);
	cmap = screen->default_colormap;

	/* color is in format rgb://rr/gg/bb */
	if (strncmp(colorname, "rgb:", 4) == 0) {
		if (parse_rgb(colorname, &rr, &gg, &bb) == -1)
			warnx("could not parse rgb %s", colorname);
		else {
			cr = xcb_alloc_color_reply(conn,
			    xcb_alloc_color(conn, cmap, rr, gg, bb),
			    NULL);
			if (cr) {
				result = cr->pixel;
				free(cr);
			} else
				warnx("color '%s' not found", colorname);
		}
	} else {
		nr = xcb_alloc_named_color_reply(conn,
			xcb_alloc_named_color(conn, cmap, strlen(colorname),
			    colorname), NULL);
		if (nr == NULL) {
			strlcat(cname, colorname + 2, sizeof cname - 1);
			nr = xcb_alloc_named_color_reply(conn,
			    xcb_alloc_named_color(conn, cmap, strlen(cname),
			    cname), NULL);
		}
		if (nr) {
			result = nr->pixel;
			free(nr);
		} else
			warnx("color '%s' not found", colorname);
	}

	return (result);
}

void
setscreencolor(const char *val, int i, int c)
{
	if (i < 0 || i >= get_screen_count())
		return;

	screens[i].c[c].pixel = name_to_pixel(i, val);
	free(screens[i].c[c].name);
	if ((screens[i].c[c].name = strdup(val)) == NULL)
		err(1, "strdup");
}

void
fancy_stacker(struct workspace *ws)
{
	strlcpy(ws->stacker, "[   ]", sizeof ws->stacker);
	if (ws->cur_layout->l_stack == vertical_stack)
		snprintf(ws->stacker, sizeof ws->stacker,
		    ws->l_state.vertical_flip ? "[%d>%d]" : "[%d|%d]",
		    ws->l_state.vertical_mwin, ws->l_state.vertical_stacks);
	else if (ws->cur_layout->l_stack == horizontal_stack)
		snprintf(ws->stacker, sizeof ws->stacker,
		    ws->l_state.horizontal_flip ? "[%dv%d]" : "[%d-%d]",
		    ws->l_state.horizontal_mwin, ws->l_state.horizontal_stacks);
}

void
plain_stacker(struct workspace *ws)
{
	strlcpy(ws->stacker, "[ ]", sizeof ws->stacker);
	if (ws->cur_layout->l_stack == vertical_stack)
		strlcpy(ws->stacker, ws->l_state.vertical_flip ? "[>]" : "[|]",
		    sizeof ws->stacker);
	else if (ws->cur_layout->l_stack == horizontal_stack)
		strlcpy(ws->stacker, ws->l_state.horizontal_flip ? "[v]" : "[-]",
		    sizeof ws->stacker);
}

void
custom_region(const char *val)
{
	unsigned int			x, y, w, h;
	int				sidx, num_screens;
	xcb_screen_t			*screen;

	num_screens = get_screen_count();
	if (sscanf(val, "screen[%d]:%ux%u+%u+%u", &sidx, &w, &h, &x, &y) != 5)
		errx(1, "invalid custom region, "
		    "should be 'screen[<n>]:<n>x<n>+<n>+<n>");
	if (sidx < 1 || sidx > num_screens)
		errx(1, "invalid screen index: %d out of bounds (maximum %d)",
		    sidx, num_screens);
	sidx--;

	if ((screen = get_screen(sidx)) == NULL)
		errx(1, "ERROR: can't get screen %d.", sidx);

	if (w < 1 || h < 1)
		errx(1, "region %ux%u+%u+%u too small", w, h, x, y);

	if (x > screen->width_in_pixels ||
	    y > screen->height_in_pixels ||
	    w + x > screen->width_in_pixels ||
	    h + y > screen->height_in_pixels) {
		warnx("ignoring region %ux%u+%u+%u - not within screen "
		    "boundaries (%ux%u)", w, h, x, y,
		    screen->width_in_pixels, screen->height_in_pixels);
		return;
	}

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
bar_print_legacy(struct swm_region *r, const char *s)
{
	xcb_rectangle_t		rect;
	uint32_t		gcv[1];
	XGCValues		gcvd;
	int			x = 0;
	size_t			len;
	XRectangle		ibox, lbox;
	GC			draw;

	len = strlen(s);
	XmbTextExtents(bar_fs, s, len, &ibox, &lbox);

	switch (bar_justify) {
	case SWM_BAR_JUSTIFY_LEFT:
		x = SWM_BAR_OFFSET;
		break;
	case SWM_BAR_JUSTIFY_CENTER:
		x = (WIDTH(r) - lbox.width) / 2;
		break;
	case SWM_BAR_JUSTIFY_RIGHT:
		x = WIDTH(r) - lbox.width - SWM_BAR_OFFSET;
		break;
	}

	if (x < SWM_BAR_OFFSET)
		x = SWM_BAR_OFFSET;

	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);

	/* clear back buffer */
	gcv[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	xcb_change_gc(conn, r->s->bar_gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->bar_gc, 1, &rect);

	/* draw back buffer */
	gcvd.graphics_exposures = 0;
	draw = XCreateGC(display, r->bar->buffer, GCGraphicsExposures, &gcvd);
	XSetForeground(display, draw, r->s->c[SWM_S_COLOR_BAR_FONT].pixel);
	DRAWSTRING(display, r->bar->buffer, bar_fs, draw,
	    x, (bar_fs_extents->max_logical_extent.height - lbox.height) / 2 -
	    lbox.y, s, len);
	XFreeGC(display, draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->bar_gc, 0, 0,
	    0, 0, WIDTH(r->bar), HEIGHT(r->bar));
}

void
bar_print(struct swm_region *r, const char *s)
{
	size_t				len;
	xcb_rectangle_t			rect;
	uint32_t			gcv[1];
	int32_t				x = 0;
	XGlyphInfo			info;
	XftDraw				*draw;

	len = strlen(s);

	XftTextExtentsUtf8(display, bar_font, (FcChar8 *)s, len, &info);

	switch (bar_justify) {
	case SWM_BAR_JUSTIFY_LEFT:
		x = SWM_BAR_OFFSET;
		break;
	case SWM_BAR_JUSTIFY_CENTER:
		x = (WIDTH(r) - info.width) / 2;
		break;
	case SWM_BAR_JUSTIFY_RIGHT:
		x = WIDTH(r) - info.width - SWM_BAR_OFFSET;
		break;
	}

	if (x < SWM_BAR_OFFSET)
		x = SWM_BAR_OFFSET;

	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);

	/* clear back buffer */
	gcv[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	xcb_change_gc(conn, r->s->bar_gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->bar_gc, 1, &rect);

	/* draw back buffer */
	draw = XftDrawCreate(display, r->bar->buffer,
	    DefaultVisual(display, r->s->idx),
	    DefaultColormap(display, r->s->idx));

	XftDrawStringUtf8(draw, &bar_font_color, bar_font, x,
	    (HEIGHT(r->bar) + bar_font->height) / 2 - bar_font->descent,
	    (FcChar8 *)s, len);

	XftDrawDestroy(draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->bar_gc, 0, 0,
	    0, 0, WIDTH(r->bar), HEIGHT(r->bar));
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
	bar_extra = false;
}

void
bar_window_class(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;
	if (r->ws->focus->ch.class_name != NULL)
		strlcat(s, r->ws->focus->ch.class_name, sz);
}

void
bar_window_instance(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;
	if (r->ws->focus->ch.instance_name != NULL)
		strlcat(s, r->ws->focus->ch.instance_name, sz);
}

void
bar_window_class_instance(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;

	bar_window_class(s, sz, r);
	strlcat(s, ":", sz);
	bar_window_instance(s, sz, r);
}

void
bar_window_state(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r ->ws == NULL || r->ws->focus == NULL)
		return;
	if (MAXIMIZED(r->ws->focus))
		strlcat(s, "(m)", sz);
	else if (ABOVE(r->ws->focus))
		strlcat(s, "(f)", sz);
}

void
bar_window_name(char *s, size_t sz, struct swm_region *r)
{
	char		*title;

	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;

	title = get_win_name(r->ws->focus->id);
	strlcat(s, title, sz);
	free(title);
}

bool
get_urgent(struct ws_win *win)
{
	xcb_icccm_wm_hints_t		hints;
	xcb_get_property_cookie_t	c;
	bool				urgent = false;

	if (win) {
		c = xcb_icccm_get_wm_hints(conn, win->id);
		if (xcb_icccm_get_wm_hints_reply(conn, c, &hints, NULL))
			urgent = xcb_icccm_wm_hints_get_urgency(&hints);
	}

	return urgent;
}

void
bar_urgent(char *s, size_t sz)
{
	struct ws_win		*win;
	int			i, j, num_screens;
	bool			urgent[SWM_WS_MAX];
	char			b[8];

	for (i = 0; i < workspace_limit; i++)
		urgent[i] = false;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (get_urgent(win))
					urgent[j] = true;

	for (i = 0; i < workspace_limit; i++) {
		if (urgent[i]) {
			snprintf(b, sizeof b, "%d ", i + 1);
			strlcat(s, b, sz);
		} else if (!urgent_collapse) {
			strlcat(s, "- ", sz);
		}
	}
}

void
bar_workspace_name(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL)
		return;
	if (r->ws->name != NULL)
		strlcat(s, r->ws->name, sz);
}

/* build the default bar format according to the defined enabled options */
void
bar_fmt(const char *fmtexp, char *fmtnew, struct swm_region *r, size_t sz)
{
	struct ws_win		*w;

	/* if format provided, just copy the buffers */
	if (bar_format != NULL) {
		strlcpy(fmtnew, fmtexp, sz);
		return;
	}

	/* reset the output buffer */
	*fmtnew = '\0';

	strlcat(fmtnew, "+N:+I ", sz);
	if (stack_enabled)
		strlcat(fmtnew, "+S", sz);
	strlcat(fmtnew, " ", sz);

	/* only show the workspace name if there's actually one */
	if (r != NULL && r->ws != NULL && r->ws->name != NULL)
		strlcat(fmtnew, "<+D>", sz);

	/* If enabled, only show the iconic count if there are iconic wins. */
	if (iconic_enabled && r != NULL && r->ws != NULL)
		TAILQ_FOREACH(w, &r->ws->winlist, entry)
			if (ICONIC(w)) {
				strlcat(fmtnew, "{+M}", sz);
				break;
			}

	strlcat(fmtnew, "+3<", sz);

	if (clock_enabled) {
		strlcat(fmtnew, fmtexp, sz);
		strlcat(fmtnew, "+4<", sz);
	}

	/* bar_urgent already adds the space before the last asterisk */
	if (urgent_enabled)
		strlcat(fmtnew, "* +U*+4<", sz);

	if (window_class_enabled) {
		strlcat(fmtnew, "+C", sz);
		if (!window_instance_enabled)
			strlcat(fmtnew, "+4<", sz);
	}

	/* checks needed by the colon and floating strlcat(3) calls below */
	if (r != NULL && r->ws != NULL && r->ws->focus != NULL) {
		if (window_instance_enabled) {
			if (window_class_enabled)
				strlcat(fmtnew, ":", sz);
			strlcat(fmtnew, "+T+4<", sz);
		}
		if (window_name_enabled) {
			if (ABOVE(r->ws->focus) || MAXIMIZED(r->ws->focus))
				strlcat(fmtnew, "+F ", sz);
			strlcat(fmtnew, "+64W ", sz);
		}
	}

	/* finally add the action script output and the version */
	strlcat(fmtnew, "+4<+A+4<+V", sz);
}

void
bar_replace_pad(char *tmp, int *limit, size_t sz)
{
	/* special case; no limit given, pad one space, instead */
	if (*limit == (int)sz - 1)
		*limit = 1;
	snprintf(tmp, sz, "%*s", *limit, " ");
}

/* replaces the bar format character sequences (like in tmux(1)) */
char *
bar_replace_seq(char *fmt, char *fmtrep, struct swm_region *r, size_t *offrep,
    size_t sz)
{
	struct ws_win		*w;
	char			*ptr;
	char			tmp[SWM_BAR_MAX];
	int			limit, size, count;
	size_t			len;

	/* reset strlcat(3) buffer */
	*tmp = '\0';

	/* get number, if any */
	fmt++;
	size = 0;
	if (sscanf(fmt, "%d%n", &limit, &size) != 1)
		limit = sizeof tmp - 1;
	if (limit <= 0 || limit >= (int)sizeof tmp)
		limit = sizeof tmp - 1;

	/* there is nothing to replace (ie EOL) */
	fmt += size;
	if (*fmt == '\0')
		return (fmt);

	switch (*fmt) {
	case '<':
		bar_replace_pad(tmp, &limit, sizeof tmp);
		break;
	case 'A':
		snprintf(tmp, sizeof tmp, "%s", bar_ext);
		break;
	case 'C':
		bar_window_class(tmp, sizeof tmp, r);
		break;
	case 'D':
		bar_workspace_name(tmp, sizeof tmp, r);
		break;
	case 'F':
		bar_window_state(tmp, sizeof tmp, r);
		break;
	case 'I':
		snprintf(tmp, sizeof tmp, "%d", r->ws->idx + 1);
		break;
	case 'M':
		count = 0;
		TAILQ_FOREACH(w, &r->ws->winlist, entry)
			if (ICONIC(w))
				++count;

		snprintf(tmp, sizeof tmp, "%d", count);
		break;
	case 'N':
		snprintf(tmp, sizeof tmp, "%d", r->s->idx + 1);
		break;
	case 'P':
		bar_window_class_instance(tmp, sizeof tmp, r);
		break;
	case 'S':
		snprintf(tmp, sizeof tmp, "%s", r->ws->stacker);
		break;
	case 'T':
		bar_window_instance(tmp, sizeof tmp, r);
		break;
	case 'U':
		bar_urgent(tmp, sizeof tmp);
		break;
	case 'V':
		snprintf(tmp, sizeof tmp, "%s", bar_vertext);
		break;
	case 'W':
		bar_window_name(tmp, sizeof tmp, r);
		break;
	default:
		/* unknown character sequence; copy as-is */
		snprintf(tmp, sizeof tmp, "+%c", *fmt);
		break;
	}

	len = strlen(tmp);
	ptr = tmp;
	if ((int)len < limit)
		limit = len;
	while (limit-- > 0) {
		if (*offrep >= sz - 1)
			break;
		fmtrep[(*offrep)++] = *ptr++;
	}

	fmt++;
	return (fmt);
}

void
bar_replace(char *fmt, char *fmtrep, struct swm_region *r, size_t sz)
{
	size_t			off;

	off = 0;
	while (*fmt != '\0') {
		if (*fmt != '+') {
			/* skip ordinary characters */
			if (off >= sz - 1)
				break;
			fmtrep[off++] = *fmt++;
			continue;
		}

		/* character sequence found; replace it */
		fmt = bar_replace_seq(fmt, fmtrep, r, &off, sz);
		if (off >= sz - 1)
			break;
	}

	fmtrep[off] = '\0';
}

void
bar_fmt_expand(char *fmtexp, size_t sz)
{
	char			*fmt = NULL;
	size_t			len;
	struct tm		tm;
	time_t			tmt;

	/* start by grabbing the current time and date */
	time(&tmt);
	localtime_r(&tmt, &tm);

	/* figure out what to expand */
	if (bar_format != NULL)
		fmt = bar_format;
	else if (bar_format == NULL && clock_enabled)
		fmt = clock_format;
	/* if nothing to expand bail out */
	if (fmt == NULL) {
		*fmtexp = '\0';
		return;
	}

	/* copy as-is, just in case the format shouldn't be expanded below */
	strlcpy(fmtexp, fmt, sz);
	/* finally pass the string through strftime(3) */
#ifndef SWM_DENY_CLOCK_FORMAT
	if ((len = strftime(fmtexp, sz, fmt, &tm)) == 0)
		warnx("format too long");
	fmtexp[len] = '\0';
#endif
}

/* Redraws the bar; need to follow with xcb_flush() or focus_flush(). */
void
bar_draw(void)
{
	char			fmtexp[SWM_BAR_MAX], fmtnew[SWM_BAR_MAX];
	char			fmtrep[SWM_BAR_MAX];
	int			i, num_screens;
	struct swm_region	*r;

	/* expand the format by first passing it through strftime(3) */
	bar_fmt_expand(fmtexp, sizeof fmtexp);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (r->bar == NULL)
				continue;

			if (bar_enabled && r->ws->bar_enabled)
				xcb_map_window(conn, r->bar->id);
			else {
				xcb_unmap_window(conn, r->bar->id);
				continue;
			}

			if (startup_exception)
				snprintf(fmtrep, sizeof fmtrep, "total "
				    "exceptions: %d, first exception: %s",
				    nr_exceptions,
				    startup_exception);
			else {
				bar_fmt(fmtexp, fmtnew, r, sizeof fmtnew);
				bar_replace(fmtnew, fmtrep, r, sizeof fmtrep);
			}
			if (bar_font_legacy)
				bar_print_legacy(r, fmtrep);
			else
				bar_print(r, fmtrep);
		}
	}
}

/*
 * Reads external script output; call when stdin is readable.
 * Returns 1 if bar_ext was updated; otherwise 0.
 */
int
bar_extra_update(void)
{
	size_t		len;
	char		b[SWM_BAR_MAX];
	bool		changed = false;

	if (!bar_extra)
		return changed;

	while (fgets(b, sizeof(b), stdin) != NULL) {
		if (bar_enabled) {
			len = strlen(b);
			if (b[len - 1] == '\n') {
				/* Remove newline. */
				b[--len] = '\0';

				/* "Clear" bar_ext. */
				bar_ext[0] = '\0';

				/* Flush buffered output. */
				strlcpy(bar_ext, bar_ext_buf, sizeof(bar_ext));
				bar_ext_buf[0] = '\0';

				/* Append new output to bar. */
				strlcat(bar_ext, b, sizeof(bar_ext));

				changed = true;
			} else {
				/* Buffer output. */
				strlcat(bar_ext_buf, b, sizeof(bar_ext_buf));
			}
		}
	}

	if (errno != EAGAIN) {
		warn("bar_action failed");
		bar_extra_stop();
		changed = true;
	}

	return changed;
}

void
bar_toggle(struct swm_region *r, union arg *args)
{
	struct swm_region	*tmpr;
	int			i, num_screens;

	/* suppress unused warnings since vars are needed */
	(void)r;
	(void)args;

	DNPRINTF(SWM_D_BAR, "bar_toggle\n");

	switch (args->id) {
	case SWM_ARG_ID_BAR_TOGGLE_WS:
		/* Only change if master switch is enabled. */
		if (bar_enabled)
			r->ws->bar_enabled = !r->ws->bar_enabled;
		else
			bar_enabled = r->ws->bar_enabled = true;
		break;
	case SWM_ARG_ID_BAR_TOGGLE:
		bar_enabled = !bar_enabled;
		break;
	}

	/* update bars as necessary */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
			if (tmpr->bar) {
				if (bar_enabled && tmpr->ws->bar_enabled)
					xcb_map_window(conn, tmpr->bar->id);
				else
					xcb_unmap_window(conn, tmpr->bar->id);
			}

	stack();

	/* must be after stack */
	bar_draw();

	focus_flush();
}

void
bar_extra_setup(void)
{
	/* do this here because the conf file is in memory */
	if (!bar_extra && bar_argv[0]) {
		/* launch external status app */
		bar_extra = true;
		if (pipe(bar_pipe) == -1)
			err(1, "pipe error");
		socket_setnonblock(bar_pipe[0]);
		socket_setnonblock(bar_pipe[1]); /* XXX hmmm, really? */

		/* Set stdin to read from the pipe. */
		if (dup2(bar_pipe[0], STDIN_FILENO) == -1)
			err(1, "dup2");

		/* Set stdout to write to the pipe. */
		if (dup2(bar_pipe[1], STDOUT_FILENO) == -1)
			err(1, "dup2");

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

		atexit(kill_bar_extra_atexit);
	}
}

void
kill_bar_extra_atexit(void)
{
	if (bar_pid)
		kill(bar_pid, SIGTERM);
}

bool
isxlfd(char *s)
{
	int	 count = 0;

	while ((s = index(s, '-'))) {
		++count;
		++s;
	}

	return (count == 14);
}

void
fontset_init(void)
{
	char			*default_string;
	char			**missing_charsets;
	int			num_missing_charsets = 0;
	int			i;

	if (bar_fs) {
		XFreeFontSet(display, bar_fs);
		bar_fs = NULL;
	}

	DNPRINTF(SWM_D_INIT, "fontset_init: loading bar_fonts: %s\n", bar_fonts);

	bar_fs = XCreateFontSet(display, bar_fonts, &missing_charsets,
	    &num_missing_charsets, &default_string);

	if (num_missing_charsets > 0) {
		warnx("Unable to load charset(s):");

		for (i = 0; i < num_missing_charsets; ++i)
			warnx("%s", missing_charsets[i]);

		XFreeStringList(missing_charsets);

		if (strcmp(default_string, ""))
			warnx("Glyphs from those sets will be replaced "
			    "by '%s'.", default_string);
		else
			warnx("Glyphs from those sets won't be drawn.");
	}

	if (bar_fs == NULL)
		errx(1, "Error creating font set structure.");

	bar_fs_extents = XExtentsOfFontSet(bar_fs);

	bar_height = bar_fs_extents->max_logical_extent.height +
	    2 * bar_border_width;

	if (bar_height < 1)
		bar_height = 1;
}

void
xft_init(struct swm_region *r)
{
	char			*font, *d, *search;
	XRenderColor		color;

	if (bar_font == NULL) {
		if ((d = strdup(bar_fonts)) == NULL)
			errx(1, "insufficient memory.");
		search = d;
		while ((font = strsep(&search, ",")) != NULL) {
			if (*font == '\0')
				continue;

			DNPRINTF(SWM_D_INIT, "xft_init: try font %s\n", font);

			if (isxlfd(font)) {
				bar_font = XftFontOpenXlfd(display, r->s->idx,
						font);
			} else {
				bar_font = XftFontOpenName(display, r->s->idx,
						font);
			}

			if (bar_font == NULL) {
				warnx("unable to load font %s", font);
				continue;
			} else {
				DNPRINTF(SWM_D_INIT, "successfully opened "
				    "font %s\n", font);
				break;
			}
		}
		free(d);
	}

	if (bar_font == NULL)
		errx(1, "unable to open a font");

	PIXEL_TO_XRENDERCOLOR(r->s->c[SWM_S_COLOR_BAR_FONT].pixel, color);

	if (!XftColorAllocValue(display, DefaultVisual(display, r->s->idx),
	    DefaultColormap(display, r->s->idx), &color, &bar_font_color))
		warn("Xft error: unable to allocate color.");

	PIXEL_TO_XRENDERCOLOR(r->s->c[SWM_S_COLOR_BAR].pixel, color);

	if (!XftColorAllocValue(display, DefaultVisual(display, r->s->idx),
	    DefaultColormap(display, r->s->idx), &color, &search_font_color))
		warn("Xft error: unable to allocate color.");

	bar_height = bar_font->height + 2 * bar_border_width;

	if (bar_height < 1)
		bar_height = 1;
}

void
bar_setup(struct swm_region *r)
{
	xcb_screen_t	*screen;
	uint32_t	 wa[3];

	DNPRINTF(SWM_D_BAR, "bar_setup: screen %d.\n",
	    r->s->idx);

	if ((screen = get_screen(r->s->idx)) == NULL)
		errx(1, "ERROR: can't get screen %d.", r->s->idx);

	if (r->bar != NULL)
		return;

	if ((r->bar = calloc(1, sizeof(struct swm_bar))) == NULL)
		err(1, "bar_setup: calloc: failed to allocate memory.");

	if (bar_font_legacy)
		fontset_init();
	else
		xft_init(r);

	X(r->bar) = X(r);
	Y(r->bar) = bar_at_bottom ? (Y(r) + HEIGHT(r) - bar_height) : Y(r);
	WIDTH(r->bar) = WIDTH(r) - 2 * bar_border_width;
	HEIGHT(r->bar) = bar_height - 2 * bar_border_width;

	/* Assume region is unfocused when we create the bar. */
	r->bar->id = xcb_generate_id(conn);
	wa[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	wa[1] = r->s->c[SWM_S_COLOR_BAR_BORDER_UNFOCUS].pixel;
	wa[2] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
	    XCB_EVENT_MASK_POINTER_MOTION_HINT;

	xcb_create_window(conn, XCB_COPY_FROM_PARENT, r->bar->id, r->s->root,
	    X(r->bar), Y(r->bar), WIDTH(r->bar), HEIGHT(r->bar),
	    bar_border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT,
	    XCB_COPY_FROM_PARENT, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
	    | XCB_CW_EVENT_MASK, wa);

	/* Stack bar window above region window to start. */
	wa[0] = r->id;
	wa[1] = XCB_STACK_MODE_ABOVE;

	xcb_configure_window(conn, r->bar->id, XCB_CONFIG_WINDOW_SIBLING |
	    XCB_CONFIG_WINDOW_STACK_MODE, wa);

	r->bar->buffer = xcb_generate_id(conn);
	xcb_create_pixmap(conn, screen->root_depth, r->bar->buffer, r->bar->id,
	    WIDTH(r->bar), HEIGHT(r->bar));

	if (randr_support)
		xcb_randr_select_input(conn, r->bar->id,
		    XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);

	if (bar_enabled)
		xcb_map_window(conn, r->bar->id);

	DNPRINTF(SWM_D_BAR, "bar_setup: win %#x, (x,y) w x h: (%d,%d) "
	    "%d x %d\n", WINID(r->bar), X(r->bar), Y(r->bar), WIDTH(r->bar),
	    HEIGHT(r->bar));

	bar_extra_setup();
}

void
bar_cleanup(struct swm_region *r)
{
	if (r->bar == NULL)
		return;
	xcb_destroy_window(conn, r->bar->id);
	xcb_free_pixmap(conn, r->bar->buffer);
	free(r->bar);
	r->bar = NULL;
}

void
set_win_state(struct ws_win *win, uint8_t state)
{
	uint16_t		data[2] = { state, XCB_ATOM_NONE };

	DNPRINTF(SWM_D_EVENT, "set_win_state: win %#x, state: %u\n",
	    win->id, state);

	if (win == NULL)
		return;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id, a_state,
	    a_state, 32, 2, data);
}

uint8_t
get_win_state(xcb_window_t w)
{
	xcb_get_property_reply_t	*r;
	xcb_get_property_cookie_t	c;
	uint32_t			result = 0;

	c = xcb_get_property(conn, 0, w, a_state, a_state, 0L, 2L);
	r = xcb_get_property_reply(conn, c, NULL);
	if (r) {
		if (r->type == a_state && r->format == 32 && r->length == 2)
			result = *((uint32_t *)xcb_get_property_value(r));
		free(r);
	}

	DNPRINTF(SWM_D_MISC, "get_win_state property: win %#x state %u\n", w,
	    result);
	return (result);
}

void
version(struct swm_region *r, union arg *args)
{
	/* suppress unused warnings since vars are needed */
	(void)r;
	(void)args;

	bar_version = !bar_version;
	if (bar_version)
		snprintf(bar_vertext, sizeof bar_vertext,
		    "Version: %s Build: %s", SPECTRWM_VERSION, buildstr);
	else
		strlcpy(bar_vertext, "", sizeof bar_vertext);

	bar_draw();
	xcb_flush(conn);
}

void
client_msg(struct ws_win *win, xcb_atom_t a, xcb_timestamp_t t)
{
	xcb_client_message_event_t	ev;
#ifdef SWM_DEBUG
	char				*name;
#endif

	if (win == NULL)
		return;
#ifdef SWM_DEBUG
	name = get_atom_name(a);
	DNPRINTF(SWM_D_EVENT, "client_msg: win %#x, atom: %s(%u), "
	    "time: %#x\n",
	    win->id, name, a, t);
	free(name);
#endif

	bzero(&ev, sizeof ev);
	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.window = win->id;
	ev.type = a_prot;
	ev.format = 32;
	ev.data.data32[0] = a;
	ev.data.data32[1] = t;

	xcb_send_event(conn, 0, win->id,
	    XCB_EVENT_MASK_NO_EVENT, (const char *)&ev);
}

/* synthetic response to a ConfigureRequest when not making a change */
void
config_win(struct ws_win *win, xcb_configure_request_event_t *ev)
{
	xcb_configure_notify_event_t ce;

	if (win == NULL)
		return;

	/* send notification of unchanged state. */
	bzero(&ce, sizeof(ce));
	ce.response_type = XCB_CONFIGURE_NOTIFY;
	ce.x = X(win);
	ce.y = Y(win);
	ce.width = WIDTH(win);
	ce.height = HEIGHT(win);
	ce.override_redirect = 0;

	if (ev == NULL) {
		/* EWMH */
		ce.event = win->id;
		ce.window = win->id;
		ce.border_width = BORDER(win);
		ce.above_sibling = XCB_WINDOW_NONE;
	} else {
		/* normal */
		ce.event = ev->window;
		ce.window = ev->window;

		/* make response appear more WM_SIZE_HINTS-compliant */
		if (win->sh.flags) {
			DNPRINTF(SWM_D_MISC, "config_win: hints: win %#x,"
			    " sh.flags: %u, min: %d x %d, max: %d x %d, inc: "
			    "%d x %d\n", win->id, win->sh.flags, SH_MIN_W(win),
			    SH_MIN_H(win), SH_MAX_W(win), SH_MAX_H(win),
			    SH_INC_W(win), SH_INC_H(win));
		}

		/* min size */
		if (SH_MIN(win)) {
			/* the hint may be set... to 0! */
			if (SH_MIN_W(win) > 0 && ce.width < SH_MIN_W(win))
				ce.width = SH_MIN_W(win);
			if (SH_MIN_H(win) > 0 && ce.height < SH_MIN_H(win))
				ce.height = SH_MIN_H(win);
		}

		/* max size */
		if (SH_MAX(win)) {
			/* may also be advertized as 0 */
			if (SH_MAX_W(win) > 0 && ce.width > SH_MAX_W(win))
				ce.width = SH_MAX_W(win);
			if (SH_MAX_H(win) > 0 && ce.height > SH_MAX_H(win))
				ce.height = SH_MAX_H(win);
		}

		/* resize increment. */
		if (SH_INC(win)) {
			if (SH_INC_W(win) > 1 && ce.width > SH_INC_W(win))
				ce.width -= (ce.width - SH_MIN_W(win)) %
				    SH_INC_W(win);
			if (SH_INC_H(win) > 1 && ce.height > SH_INC_H(win))
				ce.height -= (ce.height - SH_MIN_H(win)) %
				    SH_INC_H(win);
		}

		/* adjust x and y for requested border_width. */
		ce.x += BORDER(win) - ev->border_width;
		ce.y += BORDER(win) - ev->border_width;
		ce.border_width = ev->border_width;
		ce.above_sibling = ev->sibling;
	}

	DNPRINTF(SWM_D_MISC, "config_win: ewmh: %s, win %#x, (x,y) w x h: "
	    "(%d,%d) %d x %d, border: %d\n", YESNO(ev == NULL), win->id, ce.x,
	    ce.y, ce.width, ce.height, ce.border_width);

	xcb_send_event(conn, 0, win->id, XCB_EVENT_MASK_STRUCTURE_NOTIFY,
	    (char *)&ce);
}

int
count_win(struct workspace *ws, bool count_transient)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (!count_transient && FLOATING(win))
			continue;
		if (ICONIC(win))
			continue;
		count++;
	}
	DNPRINTF(SWM_D_MISC, "count_win: %d\n", count);

	return (count);
}

void
quit(struct swm_region *r, union arg *args)
{
	/* suppress unused warnings since vars are needed */
	(void)r;
	(void)args;

	DNPRINTF(SWM_D_MISC, "quit\n");
	running = 0;
}

void
lower_window(struct ws_win *win)
{
	struct ws_win		*target = NULL;
	struct workspace	*ws;

	if (win == NULL)
		return;

	ws = win->ws;

	DNPRINTF(SWM_D_EVENT, "lower_window: win %#x\n", win->id);

	TAILQ_FOREACH(target, &ws->stack, stack_entry) {
		if (target == win || ICONIC(target))
			continue;
		if (ws->cur_layout == &layouts[SWM_MAX_STACK])
			break;
		if (TRANS(win)) {
			if (win->transient == target->transient)
				continue;
			if (win->transient == target->id)
				break;
		}
		if (FULLSCREEN(target))
			continue;
		if (FULLSCREEN(win))
			break;
		if (MAXIMIZED(target))
			continue;
		if (MAXIMIZED(win))
			break;
		if (ABOVE(target) || TRANS(target))
			continue;
		if (ABOVE(win) || TRANS(win))
			break;
	}

	/* Change stack position. */
	TAILQ_REMOVE(&ws->stack, win, stack_entry);
	if (target)
		TAILQ_INSERT_BEFORE(target, win, stack_entry);
	else
		TAILQ_INSERT_TAIL(&ws->stack, win, stack_entry);

	update_win_stacking(win);

#ifdef SWM_DEBUG
	if (swm_debug & SWM_D_STACK) {
		DPRINTF("=== stacking order (top down) === \n");
		TAILQ_FOREACH(target, &ws->stack, stack_entry) {
			DPRINTF("win %#x, fs: %s, maximized: %s, above: %s, "
			    "iconic: %s\n", target->id, YESNO(FULLSCREEN(target)),
			    YESNO(MAXIMIZED(target)), YESNO(ABOVE(target)),
			    YESNO(ICONIC(target)));
		}
	}
#endif
	DNPRINTF(SWM_D_EVENT, "lower_window: done\n");
}

void
raise_window(struct ws_win *win)
{
	struct ws_win		*target = NULL;
	struct workspace	*ws;

	if (win == NULL)
		return;
	ws = win->ws;

	DNPRINTF(SWM_D_EVENT, "raise_window: win %#x\n", win->id);

	TAILQ_FOREACH(target, &ws->stack, stack_entry) {
		if (target == win || ICONIC(target))
			continue;
		if (ws->cur_layout == &layouts[SWM_MAX_STACK])
			break;
		if (TRANS(win) && (win->transient == target->transient ||
		    win->transient == target->id))
			break;
		if (FULLSCREEN(win))
			break;
		if (FULLSCREEN(target))
			continue;
		if (MAXIMIZED(win))
			break;
		if (MAXIMIZED(target))
			continue;
		if (ABOVE(win) || TRANS(win) ||
		    (win->ws->focus == win && ws->always_raise))
			break;
		if (!ABOVE(target) && !TRANS(target))
			break;
	}

	TAILQ_REMOVE(&ws->stack, win, stack_entry);
	if (target)
		TAILQ_INSERT_BEFORE(target, win, stack_entry);
	else
		TAILQ_INSERT_TAIL(&ws->stack, win, stack_entry);

	update_win_stacking(win);

#ifdef SWM_DEBUG
	if (swm_debug & SWM_D_STACK) {
		DPRINTF("=== stacking order (top down) === \n");
		TAILQ_FOREACH(target, &ws->stack, stack_entry) {
			DPRINTF("win %#x, fs: %s, maximized: %s, above: %s, "
			    "iconic: %s\n", target->id, YESNO(FULLSCREEN(target)),
			    YESNO(MAXIMIZED(target)), YESNO(ABOVE(target)),
			    YESNO(ICONIC(target)));
		}
	}
#endif
	DNPRINTF(SWM_D_EVENT, "raise_window: done\n");
}

void
update_win_stacking(struct ws_win *win)
{
	struct ws_win		*sibling;
	struct swm_region	*r;
	uint32_t		val[2];

	if (win == NULL || (r = win->ws->r) == NULL)
		return;

	sibling = TAILQ_NEXT(win, stack_entry);
	if (sibling != NULL && (FLOATING(win) == FLOATING(sibling) ||
	    (win->ws->always_raise && win->ws->focus == win)))
		val[0] = sibling->id;
	else if (FLOATING(win) || (win->ws->always_raise &&
	    win->ws->focus == win))
		val[0] = r->bar->id;
	else
		val[0] = r->id;

	DNPRINTF(SWM_D_EVENT, "update_win_stacking: %#x, sibling %#x\n",
	    win->id, val[0]);

	val[1] = XCB_STACK_MODE_ABOVE;

	xcb_configure_window(conn, win->id, XCB_CONFIG_WINDOW_SIBLING |
	    XCB_CONFIG_WINDOW_STACK_MODE, val);
}

void
map_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "map_window: win %#x, mapped: %s\n",
	    win->id, YESNO(win->mapped));

	if (win->mapped)
		return;

	xcb_map_window(conn, win->id);
	set_win_state(win, XCB_ICCCM_WM_STATE_NORMAL);
	win->mapped = true;
}

void
unmap_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "unmap_window: win %#x, mapped: %s\n", win->id,
	    YESNO(win->mapped));

	if (!win->mapped)
		return;

	xcb_unmap_window(conn, win->id);
	set_win_state(win, XCB_ICCCM_WM_STATE_ICONIC);
	win->mapped = false;
}

void
unmap_all(void)
{
	struct ws_win		*win;
	int			i, j, num_screens;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unmap_window(win);
}

void
fake_keypress(struct ws_win *win, xcb_keysym_t keysym, uint16_t modifiers)
{
	xcb_key_press_event_t	event;
	xcb_keycode_t		*keycode;

	if (win == NULL)
		return;

	keycode = xcb_key_symbols_get_keycode(syms, keysym);

	DNPRINTF(SWM_D_MISC, "fake_keypress: win %#x, keycode %u\n",
	    win->id, *keycode);

	bzero(&event, sizeof(event));
	event.event = win->id;
	event.root = win->s->root;
	event.child = XCB_WINDOW_NONE;
	event.time = XCB_CURRENT_TIME;
	event.event_x = X(win);
	event.event_y = Y(win);
	event.root_x = 1;
	event.root_y = 1;
	event.same_screen = 1;
	event.detail = *keycode;
	event.state = modifiers;

	event.response_type = XCB_KEY_PRESS;
	xcb_send_event(conn, 1, win->id,
	    XCB_EVENT_MASK_KEY_PRESS, (const char *)&event);

	event.response_type = XCB_KEY_RELEASE;
	xcb_send_event(conn, 1, win->id,
	    XCB_EVENT_MASK_KEY_RELEASE, (const char *)&event);

	free(keycode);
}

void
restart(struct swm_region *r, union arg *args)
{
	/* suppress unused warning since var is needed */
	(void)r;
	(void)args;

	DNPRINTF(SWM_D_MISC, "restart: %s\n", start_argv[0]);

	shutdown_cleanup();

	execvp(start_argv[0], start_argv);
	warn("execvp failed");
	quit(NULL, NULL);
}

struct ws_win *
get_pointer_win(xcb_window_t root)
{
	struct ws_win			*win = NULL;
	xcb_query_pointer_reply_t	*r;

	DNPRINTF(SWM_D_EVENT, "get_pointer_win: root: %#x.\n", root);

	r = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, root), NULL);
	if (r) {
		win = find_window(r->child);
		if (win) {
			DNPRINTF(SWM_D_EVENT, "get_pointer_win: %#x.\n",
			    win->id);
		} else {
			DNPRINTF(SWM_D_EVENT, "get_pointer_win: none.\n");
		}
		free(r);
	}

	return win;
}

void
center_pointer(struct swm_region *r)
{
	struct ws_win			*win;

	if (!warp_pointer || r == NULL)
		return;

	win = r->ws->focus;

	DNPRINTF(SWM_D_EVENT, "center_pointer: win %#x.\n", WINID(win));

	if (win && win->mapped)
		xcb_warp_pointer(conn, XCB_NONE, win->id, 0, 0, 0, 0,
		    WIDTH(win) / 2, HEIGHT(win) / 2);
	else
		xcb_warp_pointer(conn, XCB_NONE, r->id, 0, 0, 0, 0,
		    WIDTH(r) / 2, HEIGHT(r) / 2);
}

struct swm_region *
root_to_region(xcb_window_t root, int check)
{
	struct ws_win			*cfw;
	struct swm_region		*r = NULL;
	int				i, num_screens;
	xcb_query_pointer_reply_t	*qpr;
	xcb_get_input_focus_reply_t	*gifr;

	DNPRINTF(SWM_D_MISC, "root_to_region: win %#x\n", root);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == root)
			break;

	if (check & SWM_CK_REGION)
		r = screens[i].r_focus;

	if (r == NULL && check & SWM_CK_FOCUS) {
		/* Try to find an actively focused window */
		gifr = xcb_get_input_focus_reply(conn,
		    xcb_get_input_focus(conn), NULL);
		if (gifr) {
			cfw = find_window(gifr->focus);
			if (cfw && cfw->ws->r)
				r = cfw->ws->r;

			free(gifr);
		}
	}

	if (r == NULL && check & SWM_CK_POINTER) {
		/* No region with an active focus; try to use pointer. */
		qpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn,
		    screens[i].root), NULL);
		if (qpr) {
			DNPRINTF(SWM_D_MISC, "root_to_region: pointer: "
			    "(%d,%d)\n", qpr->root_x, qpr->root_y);
			TAILQ_FOREACH(r, &screens[i].rl, entry)
				if (X(r) <= qpr->root_x &&
				    qpr->root_x < MAX_X(r) &&
				    Y(r) <= qpr->root_y &&
				    qpr->root_y < MAX_Y(r))
					break;
			free(qpr);
		}
	}

	/* Last resort. */
	if (r == NULL && check & SWM_CK_FALLBACK)
		r = TAILQ_FIRST(&screens[i].rl);

	DNPRINTF(SWM_D_MISC, "root_to_region: idx: %d\n", get_region_index(r));

	return (r);
}

struct ws_win *
find_unmanaged_window(xcb_window_t id)
{
	struct ws_win		*win;
	int			i, j, num_screens;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].unmanagedlist,
			    entry)
				if (id == win->id)
					return (win);
	return (NULL);
}

struct ws_win *
find_window(xcb_window_t id)
{
	struct ws_win		*win;
	int			i, j, num_screens;
	xcb_query_tree_reply_t	*r;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (id == win->id)
					return (win);

	r = xcb_query_tree_reply(conn, xcb_query_tree(conn, id), NULL);
	if (r == NULL)
		return (NULL);

	/* if we were looking for the parent return that window instead */
	if (r->parent == 0 || r->root == r->parent) {
		free(r);
		return (NULL);
	}

	/* look for parent */
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (r->parent == win->id) {
					free(r);
					return (win);
				}

	free(r);
	return (NULL);
}

void
spawn(int ws_idx, union arg *args, bool close_fd)
{
	int			fd;
	char			*ret = NULL;

	DNPRINTF(SWM_D_MISC, "spawn: %s\n", args->argv[0]);

	close(xcb_get_file_descriptor(conn));

	setenv("LD_PRELOAD", SWM_LIB, 1);

	if (asprintf(&ret, "%d", ws_idx) == -1) {
		warn("spawn: asprintf SWM_WS");
		_exit(1);
	}
	setenv("_SWM_WS", ret, 1);
	free(ret);
	ret = NULL;

	if (asprintf(&ret, "%d", getpid()) == -1) {
		warn("spawn: asprintf _SWM_PID");
		_exit(1);
	}
	setenv("_SWM_PID", ret, 1);
	free(ret);
	ret = NULL;

	if (setsid() == -1) {
		warn("spawn: setsid");
		_exit(1);
	}

	if (close_fd) {
		/*
		 * close stdin and stdout to prevent interaction between apps
		 * and the baraction script
		 * leave stderr open to record errors
		*/
		if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) == -1) {
			warn("spawn: open");
			_exit(1);
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		if (fd > 2)
			close(fd);
	}

	execvp(args->argv[0], args->argv);

	warn("spawn: execvp");
	_exit(1);
}

void
kill_refs(struct ws_win *win)
{
	int			i, x, num_screens;
	struct swm_region	*r;
	struct workspace	*ws;

	if (win == NULL)
		return;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < workspace_limit; x++) {
				ws = &r->s->ws[x];
				if (win == ws->focus)
					ws->focus = NULL;
				if (win == ws->focus_prev)
					ws->focus_prev = NULL;
			}
}

int
validate_win(struct ws_win *testwin)
{
	struct ws_win		*win;
	struct workspace	*ws;
	struct swm_region	*r;
	int			i, x, num_screens;

	if (testwin == NULL)
		return (0);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < workspace_limit; x++) {
				ws = &r->s->ws[x];
				TAILQ_FOREACH(win, &ws->winlist, entry)
					if (win == testwin)
						return (0);
			}
	return (1);
}

int
validate_ws(struct workspace *testws)
{
	struct swm_region	*r;
	struct workspace	*ws;
	int			i, x, num_screens;

	/* validate all ws */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < workspace_limit; x++) {
				ws = &r->s->ws[x];
				if (ws == testws)
					return (0);
			}
	return (1);
}

void
unfocus_win(struct ws_win *win)
{
	xcb_window_t		none = XCB_WINDOW_NONE;

	DNPRINTF(SWM_D_FOCUS, "unfocus_win: win %#x\n", WINID(win));

	if (win == NULL)
		return;

	if (win->ws == NULL) {
		DNPRINTF(SWM_D_FOCUS, "unfocus_win: NULL ws.\n");
		return;
	}

	if (validate_ws(win->ws)) {
		DNPRINTF(SWM_D_FOCUS, "unfocus_win: invalid ws.\n");
		return;
	}

	if (win->ws->r == NULL) {
		DNPRINTF(SWM_D_FOCUS, "unfocus_win: NULL region.\n");
		return;
	}

	if (validate_win(win)) {
		DNPRINTF(SWM_D_FOCUS, "unfocus_win: invalid win.\n");
		kill_refs(win);
		return;
	}

	if (win->ws->focus == win) {
		win->ws->focus = NULL;
		win->ws->focus_prev = win;
	}

	if (validate_win(win->ws->focus)) {
		kill_refs(win->ws->focus);
		win->ws->focus = NULL;
	}

	if (validate_win(win->ws->focus_prev)) {
		kill_refs(win->ws->focus_prev);
		win->ws->focus_prev = NULL;
	}

	update_window_color(win);

	/* Raise window to "top unfocused position." */
	if (win->ws->always_raise)
		raise_window(win);

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->s->root,
	    ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1, &none);

	DNPRINTF(SWM_D_FOCUS, "unfocus_win: done.\n");
}

void
focus_win(struct ws_win *win)
{
	struct ws_win			*cfw = NULL, *parent = NULL, *w, *tmpw;
	struct workspace		*ws;
	xcb_get_input_focus_reply_t	*gifr;

	DNPRINTF(SWM_D_FOCUS, "focus_win: win %#x\n", WINID(win));

	if (win == NULL || win->ws == NULL || !win->mapped)
		goto out;

	ws = win->ws;

	if (validate_ws(ws))
		goto out;

	if (validate_win(win)) {
		kill_refs(win);
		goto out;
	}

	gifr = xcb_get_input_focus_reply(conn, xcb_get_input_focus(conn), NULL);
	if (gifr) {
		cfw = find_window(gifr->focus);
		if (cfw != NULL && cfw != win) {
			if (cfw->ws != ws && cfw->ws->r != NULL) {
				/* Change border to unfocused color. */
				xcb_change_window_attributes(conn, cfw->id,
				    XCB_CW_BORDER_PIXEL,
				    &cfw->s->c[(MAXIMIZED(cfw) ?
				    SWM_S_COLOR_UNFOCUS_MAXIMIZED :
				    SWM_S_COLOR_UNFOCUS)].pixel);
			} else {
				unfocus_win(cfw);
			}
		}
		free(gifr);
	}

	if (ws->focus != win) {
		if (ws->focus && ws->focus != cfw)
			unfocus_win(ws->focus);
		ws->focus = win;
	}

	/* If this window directs focus to a child window, then clear. */
	if (win->focus_child)
		win->focus_child = NULL;

	/* If transient, adjust parent's focus child for focus_magic. */
	if (TRANS(win)) {
		parent = find_window(win->transient);
		if (parent && parent->focus_child != win)
			parent->focus_child = win;
	}

	if (cfw != win && ws->r != NULL) {
		/* Set input focus if no input hint, or indicated by hint. */
		if (!(win->hints.flags & XCB_ICCCM_WM_HINT_INPUT) ||
		    (win->hints.flags & XCB_ICCCM_WM_HINT_INPUT &&
		     win->hints.input))
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT,
					win->id, last_event_time);
		else
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT,
			    ws->r->id, XCB_CURRENT_TIME);

		/* Tell app it can adjust focus to a specific window. */
		if (win->take_focus) {
			/* java is special; always tell parent */
			if (TRANS(win) && win->java)
				client_msg(parent, a_takefocus,
				    last_event_time);
			else
				client_msg(win, a_takefocus, last_event_time);
		}

		if (ws->cur_layout->flags & SWM_L_MAPONFOCUS ||
		    ws->always_raise) {
			/* If a parent exists, map it first. */
			if (parent) {
				raise_window(parent);
				map_window(parent);

				/* Map siblings next. */
				TAILQ_FOREACH_SAFE(w, &ws->stack, stack_entry,
				    tmpw)
					if (w != win && !ICONIC(w) &&
					    w->transient == parent->id) {
						raise_window(w);
						map_window(w);
					}
			}

			/* Map focused window. */
			raise_window(win);
			map_window(win);

			/* Stack any children of focus window. */
			TAILQ_FOREACH_SAFE(w, &ws->stack, stack_entry, tmpw)
				if (w->transient == win->id && !ICONIC(w)) {
					raise_window(w);
					map_window(w);
				}
		} else if (tile_gap < 0 && !ABOVE(win)) {
			/*
			 * Windows overlap in the layout.
			 * Raise focused win above all tiled wins.
			 */
			raise_window(win);
			map_window(win);
		}

		set_region(ws->r);

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->s->root,
		    ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1,
		    &win->id);
	}

	if (cfw != win)
		/* Update window border even if workspace is hidden. */
		update_window_color(win);

out:
	bar_draw();

	DNPRINTF(SWM_D_FOCUS, "focus_win: done.\n");
}

/* If a child window should have focus instead, return it. */
struct ws_win *
get_focus_magic(struct ws_win *win)
{
	struct ws_win	*parent = NULL;
	struct ws_win	*child = NULL;

	DNPRINTF(SWM_D_FOCUS, "get_focus_magic: win %#x\n", WINID(win));
	if (win == NULL)
		return win;

	if (TRANS(win)) {
		parent = find_window(win->transient);

		/* If parent prefers focus elsewhere, then try to do so. */
		if (parent && (child = parent->focus_child)) {
			if (validate_win(child) == 0 && child->mapped)
				win = child;
			else
				parent->focus_child = NULL;
		}
	}

	/* If this window prefers focus elsewhere, then try to do so. */
	if ((child = win->focus_child)) {
		if (validate_win(child) == 0 && child->mapped)
			win = child;
		else
			win->focus_child = NULL;
	}

	return win;
}

void
event_drain(uint8_t rt)
{
	xcb_generic_event_t	*evt;

	/* ensure all pending requests have been processed before filtering. */
	xcb_aux_sync(conn);
	while ((evt = xcb_poll_for_event(conn))) {
		if (XCB_EVENT_RESPONSE_TYPE(evt) != rt)
			event_handle(evt);

		free(evt);
	}
}

void
set_region(struct swm_region *r)
{
	struct swm_region	*rf;
	int			vals[2];

	if (r == NULL)
		return;

	rf = r->s->r_focus;
	/* Unfocus old region bar. */
	if (rf != NULL) {
		if (rf == r)
			return;

		xcb_change_window_attributes(conn, rf->bar->id,
		    XCB_CW_BORDER_PIXEL,
		    &r->s->c[SWM_S_COLOR_BAR_BORDER_UNFOCUS].pixel);
	}

	if (rf != NULL && rf != r && (X(rf) != X(r) || Y(rf) != Y(r) ||
	    WIDTH(rf) != WIDTH(r) || HEIGHT(rf) != HEIGHT(r))) {
		/* Set _NET_DESKTOP_GEOMETRY. */
		vals[0] = WIDTH(r);
		vals[1] = HEIGHT(r);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, r->s->root,
		    ewmh[_NET_DESKTOP_GEOMETRY].atom, XCB_ATOM_CARDINAL, 32, 2,
		    &vals);
	}

	/* Set region bar border to focus_color. */
	xcb_change_window_attributes(conn, r->bar->id,
	    XCB_CW_BORDER_PIXEL, &r->s->c[SWM_S_COLOR_BAR_BORDER].pixel);

	r->s->r_focus = r;

	ewmh_update_current_desktop();
}

void
focus_region(struct swm_region *r)
{
	struct ws_win		*nfw;
	struct swm_region	*old_r;

	if (r == NULL)
		return;

	old_r = r->s->r_focus;
	set_region(r);

	nfw = get_region_focus(r);
	if (nfw) {
		focus_win(nfw);
	} else {
		/* New region is empty; need to manually unfocus win. */
		if (old_r)
			unfocus_win(old_r->ws->focus);

		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT, r->id,
		    XCB_CURRENT_TIME);

		/* Clear bar since empty. */
		bar_draw();
	}
}

void
switchws(struct swm_region *r, union arg *args)
{
	struct swm_region	*this_r, *other_r;
	struct ws_win		*win;
	struct workspace	*new_ws, *old_ws;
	xcb_window_t		none = XCB_WINDOW_NONE;
	int			wsid = args->id;
	bool			unmap_old = false;

	if (!(r && r->s))
		return;

	if (wsid >= workspace_limit)
		return;

	this_r = r;
	old_ws = this_r->ws;
	new_ws = &this_r->s->ws[wsid];

	DNPRINTF(SWM_D_WS, "switchws: screen[%d]:%dx%d+%d+%d: %d -> %d\n",
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), old_ws->idx, wsid);

	if (new_ws == NULL || old_ws == NULL)
		return;
	if (new_ws == old_ws)
		return;

	if ((win = old_ws->focus) != NULL) {
		update_window_color(win);

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->s->root,
		    ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1,
		    &none);
	}

	other_r = new_ws->r;
	if (other_r == NULL) {
		/* the other workspace is hidden, hide this one */
		old_ws->r = NULL;
		unmap_old = true;
	} else {
		/* the other ws is visible in another region, exchange them */
		other_r->ws_prior = new_ws;
		other_r->ws = old_ws;
		old_ws->r = other_r;
	}
	this_r->ws_prior = old_ws;
	this_r->ws = new_ws;
	new_ws->r = this_r;

	/* Set focus_pending before stacking, if needed. */
	if (focus_mode != SWM_FOCUS_FOLLOW && (!new_ws->focus_pending ||
	    validate_win(new_ws->focus_pending))) {
		new_ws->focus_pending = get_region_focus(new_ws->r);
		new_ws->focus = new_ws->focus_prev;
		new_ws->focus_prev = NULL;
	}

	new_ws->state = SWM_WS_STATE_MAPPING;
	stack();

	/* unmap old windows */
	if (unmap_old) {
		TAILQ_FOREACH(win, &old_ws->winlist, entry)
			unmap_window(win);
		old_ws->state = SWM_WS_STATE_HIDDEN;
	}

	/* if workspaces were swapped, then don't wait to set focus */
	if (old_ws->r && focus_mode != SWM_FOCUS_FOLLOW) {
		if (new_ws->focus_pending) {
			focus_win(new_ws->focus_pending);
			new_ws->focus_pending = NULL;
		}
	}

	/* Clear bar and set focus on region input win if new ws is empty. */
	if (new_ws->focus_pending == NULL && new_ws->focus == NULL) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT, r->id,
		    XCB_CURRENT_TIME);
		bar_draw();
	}

	ewmh_update_current_desktop();

	center_pointer(r);
	focus_flush();
	new_ws->state = SWM_WS_STATE_MAPPED;

	DNPRINTF(SWM_D_WS, "switchws: done.\n");
}

void
cyclews(struct swm_region *r, union arg *args)
{
	union			arg a;
	struct swm_screen	*s = r->s;
	bool			cycle_all = false, mv = false;

	DNPRINTF(SWM_D_WS, "cyclews: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	a.id = r->ws->idx;

	do {
		switch (args->id) {
		case SWM_ARG_ID_CYCLEWS_MOVE_UP:
			mv = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP_ALL:
			cycle_all = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP:
			a.id = (a.id < workspace_limit - 1) ? a.id + 1 : 0;
			break;
		case SWM_ARG_ID_CYCLEWS_MOVE_DOWN:
			mv = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN_ALL:
			cycle_all = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN:
			a.id = (a.id > 0) ? a.id - 1 : workspace_limit - 1;
			break;
		default:
			return;
		};

		if (!cycle_all &&
		    (!cycle_empty && TAILQ_EMPTY(&s->ws[a.id].winlist)))
			continue;
		if (!cycle_visible && s->ws[a.id].r != NULL)
			continue;

		if (mv)
			send_to_ws(r, &a);

		switchws(r, &a);
	} while (a.id != r->ws->idx);

	DNPRINTF(SWM_D_FOCUS, "cyclews: done\n");
}

void
priorws(struct swm_region *r, union arg *args)
{
	union arg		a;

	(void)args;

	DNPRINTF(SWM_D_WS, "priorws: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	if (r->ws_prior == NULL)
		return;

	a.id = r->ws_prior->idx;
	switchws(r, &a);
	DNPRINTF(SWM_D_FOCUS, "priorws: done\n");
}

void
focusrg(struct swm_region *r, union arg *args)
{
	int			ridx = args->id, i, num_screens;
	struct swm_region	*rr = NULL;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	DNPRINTF(SWM_D_FOCUS, "focusrg: id: %d\n", ridx);

	rr = TAILQ_FIRST(&r->s->rl);
	for (i = 0; i < ridx && rr != NULL; ++i)
		rr = TAILQ_NEXT(rr, entry);

	if (rr == NULL)
		return;

	focus_region(rr);
	center_pointer(rr);
	focus_flush();
	DNPRINTF(SWM_D_FOCUS, "focusrg: done\n");
}

void
cyclerg(struct swm_region *r, union arg *args)
{
	struct swm_region	*rr = NULL;
	int			i, num_screens;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	i = r->s->idx;
	DNPRINTF(SWM_D_FOCUS, "cyclerg: id: %d, region: %d\n", args->id, i);

	switch (args->id) {
	case SWM_ARG_ID_CYCLERG_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&screens[i].rl);
		break;
	case SWM_ARG_ID_CYCLERG_DOWN:
		rr = TAILQ_PREV(r, swm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&screens[i].rl, swm_region_list);
		break;
	default:
		return;
	};
	if (rr == NULL)
		return;

	focus_region(rr);
	center_pointer(rr);
	focus_flush();
	DNPRINTF(SWM_D_FOCUS, "cyclerg: done\n");
}

/* Sorts transients after parent. */
void
sort_windows(struct ws_win_list *wl)
{
	struct ws_win		*win, *parent, *nxt;

	if (wl == NULL)
		return;

	for (win = TAILQ_FIRST(wl); win != TAILQ_END(wl); win = nxt) {
		nxt = TAILQ_NEXT(win, entry);
		if (TRANS(win)) {
			parent = find_window(win->transient);
			if (parent == NULL) {
				warnx("not possible bug");
				continue;
			}
			TAILQ_REMOVE(wl, win, entry);
			TAILQ_INSERT_AFTER(wl, parent, win, entry);
		}
	}
}

void
swapwin(struct swm_region *r, union arg *args)
{
	struct ws_win		*target, *source;
	struct ws_win		*cur_focus;
	struct ws_win_list	*wl;

	DNPRINTF(SWM_D_WS, "swapwin: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	cur_focus = r->ws->focus;
	if (cur_focus == NULL || FULLSCREEN(cur_focus))
		return;

	/* Adjust stacking in floating layer. */
	if (ABOVE(cur_focus)) {
		switch (args->id) {
		case SWM_ARG_ID_SWAPPREV:
			target = TAILQ_PREV(cur_focus, ws_win_stack,
			    stack_entry);
			if (target != NULL && FLOATING(target)) {
				TAILQ_REMOVE(&cur_focus->ws->stack, cur_focus,
				    stack_entry);
				TAILQ_INSERT_BEFORE(target, cur_focus,
				    stack_entry);
				update_win_stacking(cur_focus);
				focus_flush();
			}
			break;
		case SWM_ARG_ID_SWAPNEXT:
			target = TAILQ_NEXT(cur_focus, stack_entry);
			if (target != NULL && FLOATING(target)) {
				TAILQ_REMOVE(&cur_focus->ws->stack, cur_focus,
				    stack_entry);
				TAILQ_INSERT_AFTER(&cur_focus->ws->stack,
				    target, cur_focus, stack_entry);
				update_win_stacking(cur_focus);
				focus_flush();
			}
			break;
		}
		goto out;
	}

	if (r->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return;

	clear_maximized(r->ws);

	source = cur_focus;
	wl = &source->ws->winlist;

	switch (args->id) {
	case SWM_ARG_ID_SWAPPREV:
		if (TRANS(source))
			source = find_window(source->transient);
		target = TAILQ_PREV(source, ws_win_list, entry);
		if (target && target->transient)
			target = find_window(target->transient);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_TAIL(wl, source, entry);
		else
			TAILQ_INSERT_BEFORE(target, source, entry);
		break;
	case SWM_ARG_ID_SWAPNEXT:
		target = TAILQ_NEXT(source, entry);
		/* move the parent and let the sort handle the move */
		if (TRANS(source))
			source = find_window(source->transient);
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
		if (target == NULL || source == NULL)
			return;
		source->ws->focus_prev = target;
		TAILQ_REMOVE(wl, target, entry);
		TAILQ_INSERT_BEFORE(source, target, entry);
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_HEAD(wl, source, entry);
		break;
	case SWM_ARG_ID_MOVELAST:
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_TAIL(wl, source, entry);
		break;
	default:
		DNPRINTF(SWM_D_MOVE, "swapwin: invalid id: %d\n", args->id);
		return;
	}

	sort_windows(wl);
	ewmh_update_client_list();

	stack();
	center_pointer(r);
	focus_flush();
out:
	DNPRINTF(SWM_D_MOVE, "swapwin: done\n");
}

struct ws_win *
get_focus_prev(struct ws_win *win)
{
	struct ws_win		*winfocus = NULL;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	if (!(win && win->ws))
		return NULL;

	ws = win->ws;
	wl = &ws->winlist;
	cur_focus = ws->focus;

	DNPRINTF(SWM_D_FOCUS, "get_focus_prev: win %#x, cur_focus: %#x, "
	    "focus_prev: %#x\n", WINID(win), WINID(cur_focus),
	    WINID(ws->focus_prev));

	/* pickle, just focus on whatever */
	if (cur_focus == NULL) {
		/* use prev_focus if valid */
		if (ws->focus_prev && find_window(ws->focus_prev->id))
			winfocus = ws->focus_prev;
		goto done;
	}

	/* if transient focus on parent */
	if (TRANS(cur_focus)) {
		winfocus = find_window(cur_focus->transient);
		goto done;
	}

	/* if in max_stack try harder */
	if ((win->quirks & SWM_Q_FOCUSPREV) ||
	    (ws->cur_layout->flags & SWM_L_FOCUSPREV)) {
		if (cur_focus != ws->focus_prev)
			winfocus = ws->focus_prev;
		else
			winfocus = TAILQ_PREV(win, ws_win_list, entry);
		if (winfocus)
			goto done;
	}

	DNPRINTF(SWM_D_FOCUS, "get_focus_prev: focus_close: %d\n", focus_close);

	if (winfocus == NULL || winfocus == win) {
		switch (focus_close) {
		case SWM_STACK_BOTTOM:
			TAILQ_FOREACH(winfocus, wl, entry)
				if (!ICONIC(winfocus) && winfocus != cur_focus)
					break;
			break;
		case SWM_STACK_TOP:
			TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list, entry)
				if (!ICONIC(winfocus) && winfocus != cur_focus)
					break;
			break;
		case SWM_STACK_ABOVE:
			winfocus = TAILQ_NEXT(cur_focus, entry);
			while (winfocus && ICONIC(winfocus))
				winfocus = TAILQ_NEXT(winfocus, entry);

			if (winfocus == NULL) {
				if (focus_close_wrap) {
					TAILQ_FOREACH(winfocus, wl, entry)
						if (!ICONIC(winfocus) &&
						    winfocus != cur_focus)
							break;
				} else {
					TAILQ_FOREACH_REVERSE(winfocus, wl,
					    ws_win_list, entry)
						if (!ICONIC(winfocus) &&
						    winfocus != cur_focus)
							break;
				}
			}
			break;
		case SWM_STACK_BELOW:
			winfocus = TAILQ_PREV(cur_focus, ws_win_list, entry);
			while (winfocus && ICONIC(winfocus))
				winfocus = TAILQ_PREV(winfocus, ws_win_list,
				    entry);

			if (winfocus == NULL) {
				if (focus_close_wrap) {
					TAILQ_FOREACH_REVERSE(winfocus, wl,
					    ws_win_list, entry)
						if (!ICONIC(winfocus) &&
						    winfocus != cur_focus)
							break;
				} else {
					TAILQ_FOREACH(winfocus, wl, entry)
						if (!ICONIC(winfocus) &&
						    winfocus != cur_focus)
							break;
				}
			}
			break;
		}
	}
done:
	if (winfocus == NULL ||
	    (winfocus && (ICONIC(winfocus) || winfocus == cur_focus))) {
		if (focus_default == SWM_STACK_TOP) {
			TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list, entry)
				if (!ICONIC(winfocus) && winfocus != cur_focus)
					break;
		} else {
			TAILQ_FOREACH(winfocus, wl, entry)
				if (!ICONIC(winfocus) && winfocus != cur_focus)
					break;
		}
	}

	kill_refs(win);

	return get_focus_magic(winfocus);
}

struct ws_win *
get_region_focus(struct swm_region *r)
{
	struct ws_win		*winfocus = NULL;

	if (!(r && r->ws))
		return NULL;

	if (r->ws->focus && !ICONIC(r->ws->focus))
		winfocus = r->ws->focus;
	else if (r->ws->focus_prev && !ICONIC(r->ws->focus_prev))
		winfocus = r->ws->focus_prev;
	else
		TAILQ_FOREACH(winfocus, &r->ws->winlist, entry)
			if (!ICONIC(winfocus))
				break;

	return get_focus_magic(winfocus);
}

void
focus(struct swm_region *r, union arg *args)
{
	struct ws_win		*head, *cur_focus = NULL, *winfocus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;
	union arg		a;
	int			i;

	if (!(r && r->ws))
		goto out;

	cur_focus = r->ws->focus;
	ws = r->ws;
	wl = &ws->winlist;

	DNPRINTF(SWM_D_FOCUS, "focus: id: %d, cur_focus: %#x\n", args->id,
	    WINID(cur_focus));

	/* Make sure an uniconified window has focus, if one exists. */
	if (cur_focus == NULL) {
		cur_focus = TAILQ_FIRST(wl);
		while (cur_focus != NULL && ICONIC(cur_focus))
			cur_focus = TAILQ_NEXT(cur_focus, entry);

		DNPRINTF(SWM_D_FOCUS, "focus: new cur_focus: %#x\n",
		    WINID(cur_focus));
	}

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		if (cur_focus == NULL)
			goto out;

		winfocus = cur_focus;
		do {
			winfocus = TAILQ_PREV(winfocus, ws_win_list, entry);
			if (winfocus == NULL)
				winfocus = TAILQ_LAST(wl, ws_win_list);
			if (winfocus == cur_focus)
				break;
		} while (winfocus && (ICONIC(winfocus) ||
		    winfocus->id == cur_focus->transient ||
		    (cur_focus->transient != XCB_WINDOW_NONE &&
		    winfocus->transient == cur_focus->transient)));
		break;
	case SWM_ARG_ID_FOCUSNEXT:
		if (cur_focus == NULL)
			goto out;

		winfocus = cur_focus;
		do {
			winfocus = TAILQ_NEXT(winfocus, entry);
			if (winfocus == NULL)
				winfocus = TAILQ_FIRST(wl);
			if (winfocus == cur_focus)
				break;
		} while (winfocus && (ICONIC(winfocus) ||
		    winfocus->id == cur_focus->transient ||
		    (cur_focus->transient != XCB_WINDOW_NONE &&
		    winfocus->transient == cur_focus->transient)));
		break;
	case SWM_ARG_ID_FOCUSMAIN:
		if (cur_focus == NULL)
			goto out;

		winfocus = TAILQ_FIRST(wl);
		if (winfocus == cur_focus)
			winfocus = cur_focus->ws->focus_prev;
		break;
	case SWM_ARG_ID_FOCUSURGENT:
		/* Search forward for the next urgent window. */
		winfocus = NULL;
		head = cur_focus;

		for (i = 0; i <= workspace_limit; ++i) {
			if (head == NULL)
				head = TAILQ_FIRST(&r->s->ws[(ws->idx + i) %
				    workspace_limit].winlist);

			while (head) {
				if (head == cur_focus) {
					if (i > 0) {
						winfocus = cur_focus;
						break;
					}
				} else if (get_urgent(head)) {
					winfocus = head;
					break;
				}

				head = TAILQ_NEXT(head, entry);
			}

			if (winfocus)
				break;
		}

		/* Switch ws if new focus is on a different ws. */
		if (winfocus && winfocus->ws != ws) {
			a.id = winfocus->ws->idx;
			switchws(r, &a);
		}
		break;
	default:
		goto out;
	}

	if (clear_maximized(ws) > 0)
		stack();

	focus_win(get_focus_magic(winfocus));
	center_pointer(r);
	focus_flush();

out:
	DNPRINTF(SWM_D_FOCUS, "focus: done\n");
}

void
cycle_layout(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	/* suppress unused warning since var is needed */
	(void)args;

	DNPRINTF(SWM_D_EVENT, "cycle_layout: workspace: %d\n", ws->idx);

	ws->cur_layout++;
	if (ws->cur_layout->l_stack == NULL)
		ws->cur_layout = &layouts[0];

	clear_maximized(ws);

	stack();
	bar_draw();

	focus_win(get_region_focus(r));

	center_pointer(r);
	focus_flush();
}

void
stack_config(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	DNPRINTF(SWM_D_STACK, "stack_config: id: %d workspace: %d\n",
	    args->id, ws->idx);

	if (clear_maximized(ws) > 0)
		stack();

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != SWM_ARG_ID_STACKINIT)
		stack();
	bar_draw();

	center_pointer(r);
	focus_flush();
}

void
stack(void) {
	struct swm_geometry	g;
	struct swm_region	*r, *r_prev = NULL;
	int			i, num_screens;
	uint32_t		val[2];
#ifdef SWM_DEBUG
	int j;
#endif

	DNPRINTF(SWM_D_STACK, "stack: begin\n");

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
#ifdef SWM_DEBUG
		j = 0;
#endif
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			/* Adjust stack area for region bar and padding. */
			g = r->g;
			g.x += region_padding;
			g.y += region_padding;
			g.w -= 2 * border_width + 2 * region_padding;
			g.h -= 2 * border_width + 2 * region_padding;
			if (bar_enabled && r->ws->bar_enabled) {
				if (!bar_at_bottom)
					g.y += bar_height;
				g.h -= bar_height;
			}

			DNPRINTF(SWM_D_STACK, "stack: workspace: %d (screen: "
			    "%d, region: %d), (x,y) WxH: (%d,%d) %d x %d\n",
			    r->ws->idx, i, j++, g.x, g.y, g.w, g.h);

			if (r_prev) {
				/* Stack bar/input relative to prev. region. */
				val[1] = XCB_STACK_MODE_ABOVE;

				val[0] = r_prev->id;
				DNPRINTF(SWM_D_STACK, "stack: region input %#x "
				    "relative to %#x.\n", r->id, val[0]);
				xcb_configure_window(conn, r->id,
				    XCB_CONFIG_WINDOW_SIBLING |
				    XCB_CONFIG_WINDOW_STACK_MODE, val);

				val[0] = r_prev->bar->id;
				DNPRINTF(SWM_D_STACK, "stack: region bar %#x "
				    "relative to %#x.\n", r->bar->id, val[0]);
				xcb_configure_window(conn, r->bar->id,
				    XCB_CONFIG_WINDOW_SIBLING |
				    XCB_CONFIG_WINDOW_STACK_MODE, val);
			}

			r->ws->cur_layout->l_stack(r->ws, &g);
			r->ws->cur_layout->l_string(r->ws);
			/* save r so we can track region changes */
			r->ws->old_r = r;
			r_prev = r;
		}
	}
	if (font_adjusted)
		font_adjusted--;

	DNPRINTF(SWM_D_STACK, "stack: end\n");
}

void
store_float_geom(struct ws_win *win)
{
	if (win == NULL || win->ws->r == NULL)
		return;

	/* retain window geom and region geom */
	win->g_float = win->g;
	win->g_float.x -= X(win->ws->r);
	win->g_float.y -= Y(win->ws->r);
	win->g_floatvalid = true;
	DNPRINTF(SWM_D_MISC, "store_float_geom: win %#x, g: (%d,%d)"
	    " %d x %d, g_float: (%d,%d) %d x %d\n", win->id, X(win), Y(win),
	    WIDTH(win), HEIGHT(win), win->g_float.x, win->g_float.y,
	    win->g_float.w, win->g_float.h);
}

void
load_float_geom(struct ws_win *win)
{
	if (win == NULL || win->ws->r == NULL)
		return;

	if (win->g_floatvalid) {
		win->g = win->g_float;
		X(win) += X(win->ws->r);
		Y(win) += Y(win->ws->r);
		DNPRINTF(SWM_D_MISC, "load_float_geom: win %#x, g: (%d,%d)"
		    "%d x %d\n", win->id, X(win), Y(win), WIDTH(win),
		    HEIGHT(win));
	} else {
		DNPRINTF(SWM_D_MISC, "load_float_geom: win %#x, g_float "
		    "is not set.\n", win->id);
	}
}

void
update_floater(struct ws_win *win)
{
	struct workspace	*ws;
	struct swm_region	*r;

	if (win == NULL)
		return;

	ws = win->ws;

	if ((r = ws->r) == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "update_floater: win %#x\n", win->id);

	win->bordered = true;

	if (FULLSCREEN(win)) {
		/* _NET_WM_FULLSCREEN: fullscreen without border. */
		if (!win->g_floatvalid)
			store_float_geom(win);

		win->g = r->g;
		win->bordered = false;
	} else if (MAXIMIZED(win)) {
		/* Maximize: like a single stacked window. */
		if (!win->g_floatvalid)
			store_float_geom(win);

		win->g = r->g;

		if (bar_enabled && ws->bar_enabled) {
			if (!bar_at_bottom)
				Y(win) += bar_height;
			HEIGHT(win) -= bar_height;
		} else if (disable_border) {
			win->bordered = false;
		}

		if (win->bordered) {
			HEIGHT(win) -= 2 * border_width;
			WIDTH(win) -= 2 * border_width;
		}
	} else {
		/* Normal floating window. */
		/* Update geometry if new region. */
		if (r != ws->old_r)
			load_float_geom(win);

		if ((win->quirks & SWM_Q_FULLSCREEN) &&
		    WIDTH(win) >= WIDTH(r) && HEIGHT(win) >= HEIGHT(r)) {
			/* Remove border for FULLSCREEN quirk. */
			win->bordered = false;
		} else if (!MANUAL(win)) {
			if (TRANS(win) && (win->quirks & SWM_Q_TRANSSZ)) {
				/* Adjust size on TRANSSZ quirk. */
				WIDTH(win) = (double)WIDTH(r) * dialog_ratio;
				HEIGHT(win) = (double)HEIGHT(r) * dialog_ratio;
			}

			if (!(win->quirks & SWM_Q_ANYWHERE)) {
				/*
				 * Floaters and transients are auto-centred
				 * unless manually moved, resized or ANYWHERE
				 * quirk is set.
				 */
				X(win) = X(r) + (WIDTH(r) - WIDTH(win)) / 2 -
				    BORDER(win);
				Y(win) = Y(r) + (HEIGHT(r) - HEIGHT(win)) / 2 -
				    BORDER(win);
				store_float_geom(win);
			}
		}
	}

	/* Ensure at least 1 pixel of the window is in the region. */
	region_containment(win, r, SWM_CW_ALLSIDES);

	update_window(win);
}

/*
 * Send keystrokes to terminal to decrease/increase the font size as the
 * window size changes.
 */
void
adjust_font(struct ws_win *win)
{
	if (!(win->quirks & SWM_Q_XTERM_FONTADJ) ||
	    ABOVE(win) || TRANS(win))
		return;

	if (win->sh.width_inc && win->last_inc != win->sh.width_inc &&
	    WIDTH(win) / win->sh.width_inc < term_width &&
	    win->font_steps < SWM_MAX_FONT_STEPS) {
		win->font_size_boundary[win->font_steps] =
		    (win->sh.width_inc * term_width) + win->sh.base_width;
		win->font_steps++;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Subtract, XCB_MOD_MASK_SHIFT);
	} else if (win->font_steps && win->last_inc != win->sh.width_inc &&
	    WIDTH(win) > win->font_size_boundary[win->font_steps - 1]) {
		win->font_steps--;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Add, XCB_MOD_MASK_SHIFT);
	}
}

#define SWAPXY(g)	do {				\
	int tmp;					\
	tmp = (g)->y; (g)->y = (g)->x; (g)->x = tmp;	\
	tmp = (g)->h; (g)->h = (g)->w; (g)->w = tmp;	\
} while (0)
void
stack_master(struct workspace *ws, struct swm_geometry *g, int rot, bool flip)
{
	struct swm_geometry	win_g, r_g = *g;
	struct ws_win		*win;
	int			i, j, s, stacks;
	int			w_inc = 1, h_inc, w_base = 1, h_base;
	int			hrh, extra = 0, h_slice, last_h = 0;
	int			split, colno, winno, mwin, msize, mscale;
	int			remain, missing, v_slice;
	bool			bordered = true, reconfigure = false;

	DNPRINTF(SWM_D_STACK, "stack_master: workspace: %d, rot: %s, "
	    "flip: %s\n", ws->idx, YESNO(rot), YESNO(flip));

	/* Prepare tiling variables, if needed. */
	if ((winno = count_win(ws, false)) > 0) {
		/* Find first tiled window. */
		TAILQ_FOREACH(win, &ws->winlist, entry)
			if (!FLOATING(win) && !ICONIC(win))
				break;

		/* Take into account size hints of first tiled window. */
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
				/* Adjust for requested size increment. */
				remain = (win_g.w - w_base) % w_inc;
				win_g.w -= remain;
			}

			msize = win_g.w;
			if (flip)
				win_g.x += r_g.w - msize;
		} else {
			msize = -2;
			colno = split = winno / stacks;
			win_g.w = ((r_g.w - (stacks * 2 * border_width) +
			    2 * border_width) / stacks);
		}
		hrh = r_g.h / colno;
		extra = r_g.h - (colno * hrh);
		win_g.h = hrh - 2 * border_width;
		i = j = 0, s = stacks;
	}

	/* Update window geometry. */
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (ICONIC(win))
			continue;

		if (FLOATING(win)) {
			update_floater(win);
			continue;
		}

		/* Tiled. */
		if (split && i == split) {
			colno = (winno - mwin) / stacks;
			if (s <= (winno - mwin) % stacks)
				colno++;
			split += colno;
			hrh = r_g.h / colno;
			extra = r_g.h - (colno * hrh);

			if (!flip)
				win_g.x += win_g.w + 2 * border_width +
				    tile_gap;

			win_g.w = (r_g.w - msize -
			    (stacks * (2 * border_width + tile_gap))) / stacks;
			if (s == 1)
				win_g.w += (r_g.w - msize -
				    (stacks * (2 * border_width + tile_gap))) %
				    stacks;

			if (flip)
				win_g.x -= win_g.w + 2 * border_width +
				    tile_gap;
			s--;
			j = 0;
		}

		win_g.h = hrh - 2 * border_width - tile_gap;

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
			win_g.y += last_h + 2 * border_width + tile_gap;

		if (disable_border && !(bar_enabled && ws->bar_enabled) &&
		    winno == 1) {
			bordered = false;
			win_g.w += 2 * border_width;
			win_g.h += 2 * border_width;
		} else {
			bordered = true;
		}

		if (rot) {
			if (X(win) != win_g.y || Y(win) != win_g.x ||
			    WIDTH(win) != win_g.h || HEIGHT(win) != win_g.w) {
				reconfigure = true;
				X(win) = win_g.y;
				Y(win) = win_g.x;
				WIDTH(win) = win_g.h;
				HEIGHT(win) = win_g.w;
			}
		} else {
			if (X(win) != win_g.x || Y(win) != win_g.y ||
			    WIDTH(win) != win_g.w || HEIGHT(win) != win_g.h) {
				reconfigure = true;
				X(win) = win_g.x;
				Y(win) = win_g.y;
				WIDTH(win) = win_g.w;
				HEIGHT(win) = win_g.h;
			}
		}

		if (bordered != win->bordered) {
			reconfigure = true;
			win->bordered = bordered;
		}

		if (reconfigure) {
			adjust_font(win);
			update_window(win);
		}

		last_h = win_g.h;
		i++;
		j++;
	}

	/* Stack all windows from bottom up. */
	TAILQ_FOREACH_REVERSE(win, &ws->stack, ws_win_stack, stack_entry)
		if (!ICONIC(win))
			update_win_stacking(win);

	/* Map all windows from top down. */
	TAILQ_FOREACH(win, &ws->stack, stack_entry)
		if (!ICONIC(win))
			map_window(win);

	DNPRINTF(SWM_D_STACK, "stack_master: done\n");
}

void
vertical_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "vertical_config: id: %d, workspace: %d\n",
	    id, ws->idx);

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
	case SWM_ARG_ID_FLIPLAYOUT:
		ws->l_state.vertical_flip = !ws->l_state.vertical_flip;
		break;
	default:
		return;
	}
}

void
vertical_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "vertical_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, ws->l_state.vertical_flip);
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
	case SWM_ARG_ID_FLIPLAYOUT:
		ws->l_state.horizontal_flip = !ws->l_state.horizontal_flip;
		break;
	default:
		return;
	}
}

void
horizontal_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "horizontal_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, ws->l_state.horizontal_flip);
}

/* fullscreen view */
void
max_stack(struct workspace *ws, struct swm_geometry *g)
{
	struct swm_geometry	gg = *g;
	struct ws_win		*w, *win = NULL, *parent = NULL, *tmpw;
	int			winno;

	DNPRINTF(SWM_D_STACK, "max_stack: workspace: %d\n", ws->idx);

	if (ws == NULL)
		return;

	winno = count_win(ws, false);
	if (winno == 0 && count_win(ws, true) == 0)
		return;

	/* Figure out which top level window should be visible. */
	if (ws->focus_pending)
		win = ws->focus_pending;
	else if (ws->focus)
		win = ws->focus;
	else if (ws->focus_prev)
		win = ws->focus_prev;
	else
		win = TAILQ_FIRST(&ws->winlist);

	DNPRINTF(SWM_D_STACK, "max_stack: focus_pending: %#x, focus: %#x, "
	    "focus_prev: %#x, first: %#x, win: %#x\n", WINID(ws->focus_pending),
	    WINID(ws->focus), WINID(ws->focus_prev),
	    WINID(TAILQ_FIRST(&ws->winlist)), win->id);

	/* Update window geometry. */
	TAILQ_FOREACH(w, &ws->winlist, entry) {
		if (ICONIC(w))
			continue;

		if (TRANS(w)) {
			update_floater(w);
			continue;
		}

		/* Set maximized flag for all maxed windows. */
		if (!MAXIMIZED(w)) {
			/* Preserve floating geometry. */
			if (ABOVE(w))
				store_float_geom(w);

			ewmh_apply_flags(w, w->ewmh_flags | EWMH_F_MAXIMIZED);
			ewmh_update_wm_state(w);
		}

		/* Only reconfigure if necessary. */
		if (X(w) != gg.x || Y(w) != gg.y || WIDTH(w) != gg.w ||
		    HEIGHT(w) != gg.h) {
			w->g = gg;

			if (disable_border && !(bar_enabled && ws->bar_enabled)) {
				w->bordered = false;
				WIDTH(w) += 2 * border_width;
				HEIGHT(w) += 2 * border_width;
			} else {
				w->bordered = true;
			}

			update_window(w);
		}
	}

	/* If transient, stack parent and its children. */
	if (TRANS(win) && (parent = find_window(win->transient))) {
		raise_window(parent);

		TAILQ_FOREACH_SAFE(w, &ws->stack, stack_entry, tmpw)
			if (w->transient == parent->id)
				raise_window(w);
	}

	/* Make sure focus window is on top. */
	raise_window(win);

	/* Stack any children of focus window. */
	TAILQ_FOREACH_SAFE(w, &ws->stack, stack_entry, tmpw)
		if (w->transient == win->id)
			raise_window(w);

	/* Map all windows. */
	TAILQ_FOREACH(w, &ws->stack, stack_entry)
		if (!ICONIC(w))
			map_window(w);
}

void
send_to_rg(struct swm_region *r, union arg *args)
{
	int			ridx = args->id, i, num_screens;
	struct swm_region	*rr = NULL;
	union arg		a;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	DNPRINTF(SWM_D_FOCUS, "send_to_rg: id: %d\n", ridx);

	rr = TAILQ_FIRST(&r->s->rl);
	for (i = 0; i < ridx && rr != NULL; ++i)
		rr = TAILQ_NEXT(rr, entry);

	if (rr == NULL)
		return;

	a.id = rr->ws->idx;

	send_to_ws(r, &a);
}

struct swm_region *
region_under(struct swm_screen *s, int x, int y)
{
	struct swm_region	*r = NULL;

	TAILQ_FOREACH(r, &s->rl, entry) {
		DNPRINTF(SWM_D_MISC, "region_under: ws: %d, region g: (%d,%d) "
		    "%d x %d, coords: (%d,%d)\n", r->ws->idx, X(r), Y(r),
		    WIDTH(r), HEIGHT(r), x, y);
		if (X(r) <= x && x < MAX_X(r))
			if (Y(r) <= y && y < MAX_Y(r))
				return (r);
	}

	return (NULL);
}

/* Transfer focused window to target workspace and focus. */
void
send_to_ws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	DNPRINTF(SWM_D_MOVE, "send_to_ws: win %#x, ws %d\n", win->id, wsid);

	if (wsid < 0 || wsid >= workspace_limit)
		return;

	if (win->ws->idx == wsid)
		return;

	win_to_ws(win, wsid, true);

	/* Set new focus on target ws. */
	if (focus_mode != SWM_FOCUS_FOLLOW) {
		win->ws->focus_prev = win->ws->focus;
		win->ws->focus = win;
		win->ws->focus_pending = NULL;

		if (win->ws->focus_prev)
			update_window_color(win->ws->focus_prev);
	}

	DNPRINTF(SWM_D_STACK, "send_to_ws: focus_pending: %#x, focus: %#x, "
	    "focus_prev: %#x, first: %#x, win: %#x\n",
	    WINID(r->ws->focus_pending), WINID(r->ws->focus),
	    WINID(r->ws->focus_prev), WINID(TAILQ_FIRST(&r->ws->winlist)),
	    win->id);

	ewmh_apply_flags(win, win->ewmh_flags & ~EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(win);

	/* Restack and set new focus on current ws. */
	if (FLOATING(win))
		load_float_geom(win);

	stack();

	if (focus_mode != SWM_FOCUS_FOLLOW) {
		if (r->ws->focus != NULL) {
			focus_win(r->ws->focus);
		} else {
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT, r->id,
			    XCB_CURRENT_TIME);
			bar_draw();
		}
	}

	center_pointer(r);
	focus_flush();
}

void
win_to_ws(struct ws_win *win, int wsid, bool unfocus)
{
	struct ws_win		*parent;
	struct workspace	*ws, *nws, *pws;

	if (wsid < 0 || wsid >= workspace_limit)
		return;

	if (win->ws->idx == wsid)
		return;

	ws = win->ws;
	nws = &win->s->ws[wsid];

	DNPRINTF(SWM_D_MOVE, "win_to_ws: win %#x, ws %d -> %d\n", win->id,
	    ws->idx, wsid);

	/* Cleanup focus on source ws. */
	if (focus_mode != SWM_FOCUS_FOLLOW &&
	    (ws->focus == win || ws->focus_pending == win))
		ws->focus_pending = get_focus_prev(win);

	/* Move the parent if this is a transient window. */
	if (TRANS(win)) {
		parent = find_window(win->transient);
		if (parent) {
			pws = parent->ws;
			/* Set new focus in parent's ws if needed. */
			if (pws->focus == parent) {
				if (focus_mode != SWM_FOCUS_FOLLOW)
					pws->focus_pending =
					    get_focus_prev(parent);

				unfocus_win(parent);

				if (focus_mode != SWM_FOCUS_FOLLOW) {
					pws->focus = pws->focus_pending;
					pws->focus_pending = NULL;
				}
			}

			/* Don't unmap parent if new ws is visible */
			if (nws->r == NULL)
				unmap_window(parent);

			/* Transfer */
			TAILQ_REMOVE(&ws->winlist, parent, entry);
			TAILQ_REMOVE(&ws->stack, parent, stack_entry);
			TAILQ_INSERT_TAIL(&nws->winlist, parent, entry);
			TAILQ_INSERT_TAIL(&nws->stack, parent, stack_entry);
			parent->ws = nws;

			DNPRINTF(SWM_D_PROP, "win_to_ws: set property: "
			    "_NET_WM_DESKTOP: %d\n", wsid);
			xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
			    parent->id, ewmh[_NET_WM_DESKTOP].atom,
			    XCB_ATOM_CARDINAL, 32, 1, &wsid);
		}
	}

	if (unfocus)
		unfocus_win(win);

	if (ws->focus_prev == win)
		ws->focus_prev = NULL;

	if (focus_mode != SWM_FOCUS_FOLLOW && ws->focus_pending != NULL) {
		ws->focus = ws->focus_pending;
		ws->focus_pending = NULL;
	}

	/* Don't unmap if new ws is visible */
	if (nws->r == NULL)
		unmap_window(win);

	/* Transfer */
	TAILQ_REMOVE(&ws->winlist, win, entry);
	TAILQ_REMOVE(&ws->stack, win, stack_entry);
	TAILQ_INSERT_TAIL(&nws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&nws->stack, win, stack_entry);
	win->ws = nws;

	/* Update the window's workspace property: _NET_WM_DESKTOP */
	DNPRINTF(SWM_D_PROP, "win_to_ws: set property: "
	    "_NET_WM_DESKTOP: %d\n", wsid);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
	    ewmh[_NET_WM_DESKTOP].atom, XCB_ATOM_CARDINAL, 32, 1, &wsid);

	ewmh_update_client_list();

	DNPRINTF(SWM_D_MOVE, "win_to_ws: done.\n");
}

void
pressbutton(struct swm_region *r, union arg *args)
{
	/* suppress unused warning since var is needed */
	(void)r;

	xcb_test_fake_input(conn, XCB_BUTTON_PRESS, args->id,
	    XCB_CURRENT_TIME, XCB_WINDOW_NONE, 0, 0, 0);
	xcb_test_fake_input(conn, XCB_BUTTON_RELEASE, args->id,
	    XCB_CURRENT_TIME, XCB_WINDOW_NONE, 0, 0, 0);
}

void
raise_toggle(struct swm_region *r, union arg *args)
{
	/* Suppress warning. */
	(void)args;

	if (r == NULL || r->ws == NULL)
		return;

	if (r->ws->focus && MAXIMIZED(r->ws->focus))
		return;

	r->ws->always_raise = !r->ws->always_raise;

	/* Update focused win stacking order based on new always_raise value. */
	raise_window(r->ws->focus);

	focus_flush();
}

void
iconify(struct swm_region *r, union arg *args)
{
	struct ws_win		*w;

	/* Suppress warning. */
	(void)args;

	if ((w = r->ws->focus) == NULL)
		return;

	ewmh_apply_flags(w, w->ewmh_flags | EWMH_F_HIDDEN);
	ewmh_update_wm_state(w);

	stack();

	focus_flush();
}

char *
get_win_name(xcb_window_t win)
{
	char				*name = NULL;
	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;

	/* First try _NET_WM_NAME for UTF-8. */
	c = xcb_get_property(conn, 0, win, ewmh[_NET_WM_NAME].atom,
	    XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);
	r = xcb_get_property_reply(conn, c, NULL);
	if (r && r->type == XCB_NONE) {
		free(r);
		/* Use WM_NAME instead; no UTF-8. */
		c = xcb_get_property(conn, 0, win, XCB_ATOM_WM_NAME,
		    XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);
		r = xcb_get_property_reply(conn, c, NULL);
	}

	if (r && r->type != XCB_NONE && r->length > 0)
		name = strndup(xcb_get_property_value(r),
		    xcb_get_property_value_length(r));
	else
		name = strdup("");

	if (name == NULL)
		err(1, "get_win_name: failed to allocate memory.");

	free(r);

	return (name);
}

void
uniconify(struct swm_region *r, union arg *args)
{
	struct ws_win		*win;
	FILE			*lfile;
	char			*name;
	int			count = 0;

	DNPRINTF(SWM_D_MISC, "uniconify\n");

	if (r == NULL || r->ws == NULL)
		return;

	/* make sure we have anything to uniconify */
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (win->ws == NULL)
			continue; /* should never happen */
		if (!ICONIC(win))
			continue;
		count++;
	}

	DNPRINTF(SWM_D_MISC, "uniconify: count: %d\n", count);

	if (count == 0)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_UNICONIFY;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (win->ws == NULL)
			continue; /* should never happen */
		if (!ICONIC(win))
			continue;

		name = get_win_name(win->id);
		fprintf(lfile, "%s.%u\n", name, win->id);
		free(name);
	}

	fclose(lfile);
}

void
name_workspace(struct swm_region *r, union arg *args)
{
	FILE			*lfile;

	DNPRINTF(SWM_D_MISC, "name_workspace\n");

	if (r == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_NAME_WORKSPACE;

	spawn_select(r, args, "name_workspace", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	fprintf(lfile, "%s", "");
	fclose(lfile);
}

void
search_workspace(struct swm_region *r, union arg *args)
{
	int			i;
	struct workspace	*ws;
	FILE			*lfile;

	DNPRINTF(SWM_D_MISC, "search_workspace\n");

	if (r == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WORKSPACE;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	for (i = 0; i < workspace_limit; i++) {
		ws = &r->s->ws[i];
		if (ws == NULL)
			continue;
		fprintf(lfile, "%d%s%s\n", ws->idx + 1,
		    (ws->name ? ":" : ""), (ws->name ? ws->name : ""));
	}

	fclose(lfile);
}

void
search_win_cleanup(void)
{
	struct search_window	*sw = NULL;

	while ((sw = TAILQ_FIRST(&search_wl)) != NULL) {
		xcb_destroy_window(conn, sw->indicator);
		TAILQ_REMOVE(&search_wl, sw, entry);
		free(sw);
	}
}

void
search_win(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;
	struct search_window	*sw = NULL;
	xcb_window_t		w;
	uint32_t		wa[3];
	xcb_screen_t		*screen;
	int			i, width, height;
	char			s[8];
	FILE			*lfile;
	size_t			len;
	XftDraw			*draw;
	XGlyphInfo		info;
	GC			l_draw;
	XGCValues		l_gcv;
	XRectangle		l_ibox, l_lbox;

	DNPRINTF(SWM_D_MISC, "search_win\n");

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WINDOW;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	if ((screen = get_screen(r->s->idx)) == NULL)
		errx(1, "ERROR: can't get screen %d.", r->s->idx);

	TAILQ_INIT(&search_wl);

	i = 1;
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (ICONIC(win))
			continue;

		sw = calloc(1, sizeof(struct search_window));
		if (sw == NULL) {
			warn("search_win: calloc");
			fclose(lfile);
			search_win_cleanup();
			return;
		}
		sw->idx = i;
		sw->win = win;

		snprintf(s, sizeof s, "%d", i);
		len = strlen(s);

		w = xcb_generate_id(conn);
		wa[0] = r->s->c[SWM_S_COLOR_FOCUS].pixel;
		wa[1] = r->s->c[SWM_S_COLOR_UNFOCUS].pixel;
		wa[2] = screen->default_colormap;

		if (bar_font_legacy) {
			XmbTextExtents(bar_fs, s, len, &l_ibox, &l_lbox);
			width = l_lbox.width + 4;
			height = bar_fs_extents->max_logical_extent.height + 4;
		} else {
			XftTextExtentsUtf8(display, bar_font, (FcChar8 *)s, len,
			    &info);
			width = info.width + 4;
			height = bar_font->height + 4;
		}

		xcb_create_window(conn, screen->root_depth, w, win->id, 0, 0,
		    width, height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		    screen->root_visual, XCB_CW_BACK_PIXEL |
		    XCB_CW_BORDER_PIXEL | XCB_CW_COLORMAP, wa);

		xcb_map_window(conn, w);

		sw->indicator = w;
		TAILQ_INSERT_TAIL(&search_wl, sw, entry);

		if (bar_font_legacy) {
			l_gcv.graphics_exposures = 0;
			l_draw = XCreateGC(display, w, 0, &l_gcv);

			XSetForeground(display, l_draw,
				r->s->c[SWM_S_COLOR_BAR].pixel);

			DRAWSTRING(display, w, bar_fs, l_draw, 2,
			    (bar_fs_extents->max_logical_extent.height -
			    l_lbox.height) / 2 - l_lbox.y, s, len);

			XFreeGC(display, l_draw);
		} else {

			draw = XftDrawCreate(display, w,
			    DefaultVisual(display, r->s->idx),
			    DefaultColormap(display, r->s->idx));

			XftDrawStringUtf8(draw, &search_font_color, bar_font, 2,
			    (HEIGHT(r->bar) + bar_font->height) / 2 -
			    bar_font->descent, (FcChar8 *)s, len);

			XftDrawDestroy(draw);
		}

		DNPRINTF(SWM_D_MISC, "search_win: mapped win %#x\n", w);

		fprintf(lfile, "%d\n", i);
		i++;
	}

	fclose(lfile);

	xcb_flush(conn);
}

void
search_resp_uniconify(const char *resp, size_t len)
{
	char			*name;
	struct ws_win		*win;
	char			*s;

	DNPRINTF(SWM_D_MISC, "search_resp_uniconify: resp: %s\n", resp);

	TAILQ_FOREACH(win, &search_r->ws->winlist, entry) {
		if (!ICONIC(win))
			continue;
		name = get_win_name(win->id);
		if (asprintf(&s, "%s.%u", name, win->id) == -1) {
			free(name);
			continue;
		}
		free(name);
		if (strncmp(s, resp, len) == 0) {
			/* XXX this should be a callback to generalize */
			ewmh_apply_flags(win, win->ewmh_flags & ~EWMH_F_HIDDEN);
			ewmh_update_wm_state(win);
			stack();
			free(s);
			break;
		}
		free(s);
	}
}

void
search_resp_name_workspace(const char *resp, size_t len)
{
	struct workspace	*ws;

	DNPRINTF(SWM_D_MISC, "search_resp_name_workspace: resp: %s\n", resp);

	if (search_r->ws == NULL)
		return;
	ws = search_r->ws;

	if (ws->name) {
		free(search_r->ws->name);
		search_r->ws->name = NULL;
	}

	if (len > 1) {
		ws->name = strdup(resp);
		if (ws->name == NULL) {
			DNPRINTF(SWM_D_MISC, "search_resp_name_workspace: "
			    "strdup: %s", strerror(errno));
			return;
		}

		ewmh_update_desktop_names();
		ewmh_get_desktop_names();
	}
}

void
ewmh_update_desktop_names(void)
{
	char			*name_list = NULL, *p;
	int			num_screens, i, j, len = 0, tot = 0;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i) {
		for (j = 0; j < workspace_limit; ++j) {
			if (screens[i].ws[j].name != NULL)
				len += strlen(screens[i].ws[j].name);
			++len;
		}

		if((name_list = calloc(len, sizeof(char))) == NULL)
			err(1, "update_desktop_names: calloc: failed to "
			    "allocate memory.");

		p = name_list;
		for (j = 0; j < workspace_limit; ++j) {
			if (screens[i].ws[j].name != NULL) {
				len = strlen(screens[i].ws[j].name);
				memcpy(p, screens[i].ws[j].name, len);
			} else {
				len = 0;
			}

			p += len + 1;
			tot += len + 1;
		}

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
		    screens[i].root, ewmh[_NET_DESKTOP_NAMES].atom,
		    a_utf8_string, 8, tot, name_list);

		free(name_list);
		name_list = NULL;
	}

	free(name_list);
}

void
ewmh_get_desktop_names(void)
{
	char				*names = NULL;
	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;
	int				num_screens, i, j, n, k;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i) {
		for (j = 0; j < workspace_limit; ++j) {
			free(screens[i].ws[j].name);
			screens[i].ws[j].name = NULL;
		}

		c = xcb_get_property(conn, 0, screens[i].root,
		    ewmh[_NET_DESKTOP_NAMES].atom,
		    a_utf8_string, 0, UINT32_MAX);
		r = xcb_get_property_reply(conn, c, NULL);
		if (r == NULL)
			continue;

		names = xcb_get_property_value(r);
		n = xcb_get_property_value_length(r);

		for (j = 0, k = 0; j < n; ++j) {
			if (*(names + j) != '\0') {
				screens[i].ws[k].name = strdup(names + j);
				j += strlen(names + j);
			}
			++k;
		}
		free(r);
	}
}

void
ewmh_update_client_list(void)
{
	struct ws_win		*win;
	int			num_screens, i, j, k = 0, count = 0;
	xcb_window_t		*wins;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i) {
		for (j = 0; j < workspace_limit; ++j)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				++count;

		DNPRINTF(SWM_D_PROP, "ewmh_update_client_list: win count: %d\n",
		    count);

		if (count == 0)
			continue;

		wins = calloc(count, sizeof(xcb_window_t));
		if (wins == NULL)
			err(1, "ewmh_update_client_list: calloc: failed to "
			    "allocate memory.");

		for (j = 0, k = 0; j < workspace_limit; ++j)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				wins[k++] = win->id;

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
		    screens[i].root, ewmh[_NET_CLIENT_LIST].atom,
		    XCB_ATOM_WINDOW, 32, count, wins);

		free(wins);
	}
}

void
ewmh_update_current_desktop(void)
{
	int			num_screens, i;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
		    screens[i].root, ewmh[_NET_CURRENT_DESKTOP].atom,
		    XCB_ATOM_CARDINAL, 32, 1, &screens[i].r_focus->ws->idx);
}

void
ewmh_update_desktops(void)
{
	int			num_screens, i, j;
	uint32_t		*vals;

	vals = calloc(workspace_limit * 2, sizeof(uint32_t));
	if (vals == NULL)
		err(1, "ewmh_update_desktops: calloc: failed to allocate "
		    "memory.");

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
		    screens[i].root, ewmh[_NET_NUMBER_OF_DESKTOPS].atom,
		    XCB_ATOM_CARDINAL, 32, 1, &workspace_limit);

		for (j = 0; j < workspace_limit; ++j) {
			if (screens[i].ws[j].r != NULL) {
				vals[j * 2] = X(screens[i].ws[j].r);
				vals[j * 2 + 1] = Y(screens[i].ws[j].r);
			} else if (screens[i].ws[j].old_r != NULL) {
				vals[j * 2] = X(screens[i].ws[j].old_r);
				vals[j * 2 + 1] = Y(screens[i].ws[j].old_r);
			} else {
				vals[j * 2] = vals[j * 2 + 1] = 0;
			}
		}

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
		    screens[i].root, ewmh[_NET_DESKTOP_VIEWPORT].atom,
		    XCB_ATOM_CARDINAL, 32, workspace_limit * 2, vals);
	}

	free(vals);
}

void
search_resp_search_workspace(const char *resp)
{
	char			*p, *q;
	int			ws_idx;
	const char		*errstr;
	union arg		a;

	DNPRINTF(SWM_D_MISC, "search_resp_search_workspace: resp: %s\n", resp);

	q = strdup(resp);
	if (q == NULL) {
		DNPRINTF(SWM_D_MISC, "search_resp_search_workspace: strdup: %s",
		    strerror(errno));
		return;
	}
	p = strchr(q, ':');
	if (p != NULL)
		*p = '\0';
	ws_idx = (int)strtonum(q, 1, workspace_limit, &errstr);
	if (errstr) {
		DNPRINTF(SWM_D_MISC, "workspace idx is %s: %s",
		    errstr, q);
		free(q);
		return;
	}
	free(q);
	a.id = ws_idx - 1;
	switchws(search_r, &a);
}

void
search_resp_search_window(const char *resp)
{
	char			*s;
	int			idx;
	const char		*errstr;
	struct search_window	*sw;

	DNPRINTF(SWM_D_MISC, "search_resp_search_window: resp: %s\n", resp);

	s = strdup(resp);
	if (s == NULL) {
		DNPRINTF(SWM_D_MISC, "search_resp_search_window: strdup: %s",
		    strerror(errno));
		return;
	}

	idx = (int)strtonum(s, 1, INT_MAX, &errstr);
	if (errstr) {
		DNPRINTF(SWM_D_MISC, "window idx is %s: %s",
		    errstr, s);
		free(s);
		return;
	}
	free(s);

	TAILQ_FOREACH(sw, &search_wl, entry)
		if (idx == sw->idx) {
			focus_win(sw->win);
			break;
		}
}

#define MAX_RESP_LEN	1024

void
search_do_resp(void)
{
	ssize_t			rbytes;
	char			*resp;
	size_t			len;

	DNPRINTF(SWM_D_MISC, "search_do_resp:\n");

	search_resp = 0;
	searchpid = 0;

	if ((resp = calloc(1, MAX_RESP_LEN + 1)) == NULL) {
		warn("search: calloc");
		goto done;
	}

	rbytes = read(select_resp_pipe[0], resp, MAX_RESP_LEN);
	if (rbytes <= 0) {
		warn("search: read error");
		goto done;
	}
	resp[rbytes] = '\0';

	/* XXX:
	 * Older versions of dmenu (Atleast pre 4.4.1) do not send a
	 * newline, so work around that by sanitizing the resp now.
	 */
	resp[strcspn(resp, "\n")] = '\0';
	len = strlen(resp);

	switch (search_resp_action) {
	case SWM_SEARCH_UNICONIFY:
		search_resp_uniconify(resp, len);
		break;
	case SWM_SEARCH_NAME_WORKSPACE:
		search_resp_name_workspace(resp, len);
		break;
	case SWM_SEARCH_SEARCH_WORKSPACE:
		search_resp_search_workspace(resp);
		break;
	case SWM_SEARCH_SEARCH_WINDOW:
		search_resp_search_window(resp);
		break;
	}

done:
	if (search_resp_action == SWM_SEARCH_SEARCH_WINDOW)
		search_win_cleanup();

	search_resp_action = SWM_SEARCH_NONE;
	close(select_resp_pipe[0]);
	free(resp);

	xcb_flush(conn);
}

void
wkill(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "wkill: win %#x, id: %d\n", WINID(r->ws->focus),
	    args->id);

	if (r->ws->focus == NULL)
		return;

	if (args->id == SWM_ARG_ID_KILLWINDOW)
		xcb_kill_client(conn, r->ws->focus->id);
	else
		if (r->ws->focus->can_delete)
			client_msg(r->ws->focus, a_delete, 0);

	focus_flush();
}

int
clear_maximized(struct workspace *ws)
{
	struct ws_win		*w;
	int			count = 0;

	/* Clear any maximized win(s) on ws, from bottom up. */
	TAILQ_FOREACH_REVERSE(w, &ws->stack, ws_win_stack, stack_entry)
		if (MAXIMIZED(w)) {
			ewmh_apply_flags(w, w->ewmh_flags & ~EWMH_F_MAXIMIZED);
			ewmh_update_wm_state(w);
			++count;
		}

	return count;
}

void
maximize_toggle(struct swm_region *r, union arg *args)
{
	struct ws_win		*w = r->ws->focus;

	/* suppress unused warning since var is needed */
	(void)args;

	if (w == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "maximize_toggle: win %#x\n", w->id);

	if (FULLSCREEN(w))
		return;

	if (w->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return;

	ewmh_apply_flags(w, w->ewmh_flags ^ EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(w);

	stack();

	if (w == w->ws->focus)
		focus_win(w);

	center_pointer(r);
	focus_flush();
	DNPRINTF(SWM_D_MISC, "maximize_toggle: done\n");
}

void
floating_toggle(struct swm_region *r, union arg *args)
{
	struct ws_win		*w = r->ws->focus;

	/* suppress unused warning since var is needed */
	(void)args;

	if (w == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "floating_toggle: win %#x\n", w->id);

	if (FULLSCREEN(w) || TRANS(w))
		return;

	if (w->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return;

	ewmh_apply_flags(w, w->ewmh_flags ^ EWMH_F_ABOVE);
	ewmh_update_wm_state(w);

	stack();

	if (w == w->ws->focus)
		focus_win(w);

	center_pointer(r);
	focus_flush();
	DNPRINTF(SWM_D_MISC, "floating_toggle: done\n");
}

void
region_containment(struct ws_win *win, struct swm_region *r, int opts)
{
	struct swm_geometry		g = r->g;
	int				rt, lt, tp, bm, bw;

	bw = (opts & SWM_CW_SOFTBOUNDARY) ? boundary_width : 0;

	/*
	 * Perpendicular distance of each side of the window to the respective
	 * side of the region boundary.  Positive values indicate the side of
	 * the window has passed beyond the region boundary.
	 */
	rt = opts & SWM_CW_RIGHT ? MAX_X(win) + BORDER(win) - MAX_X(r) : bw;
	lt = opts & SWM_CW_LEFT ? X(r) - X(win) + BORDER(win) : bw;
	bm = opts & SWM_CW_BOTTOM ? MAX_Y(win) + BORDER(win) - MAX_Y(r) : bw;
	tp = opts & SWM_CW_TOP ? Y(r) - Y(win) + BORDER(win) : bw;

	DNPRINTF(SWM_D_MISC, "region_containment: win %#x, rt: %d, lt: %d, "
	    "bm: %d, tp: %d, SOFTBOUNDARY: %s, HARDBOUNDARY: %s\n", win->id, rt,
	    lt, bm, tp, YESNO(opts & SWM_CW_SOFTBOUNDARY),
	    YESNO(opts & SWM_CW_HARDBOUNDARY));

	/*
	 * Disable containment if any of the flagged sides went beyond the
	 * containment boundary, or if containment is disabled.
	 */
	if (!(opts & SWM_CW_HARDBOUNDARY || opts & SWM_CW_SOFTBOUNDARY) ||
	    (bw != 0 && ((rt > bw) || (lt > bw) || (bm > bw) || (tp > bw)))) {
		/* Make sure window has at least 1 pixel in the region */
		g.x += 1 - WIDTH(win);
		g.y += 1 - HEIGHT(win);
		g.w += 2 * WIDTH(win) - 2;
		g.h += 2 * HEIGHT(win) - 2;
	}

	constrain_window(win, &g, &opts);
}

/* Move or resize a window so that flagged side(s) fit into the supplied box. */
void
constrain_window(struct ws_win *win, struct swm_geometry *b, int *opts)
{
	DNPRINTF(SWM_D_MISC, "constrain_window: win %#x, (x,y) w x h: "
	    "(%d,%d) %d x %d, box: (x,y) w x h: (%d,%d) %d x %d, rt: %s, "
	    "lt: %s, bt: %s, tp: %s, allow resize: %s\n", win->id, X(win),
	    Y(win), WIDTH(win), HEIGHT(win), b->x, b->y, b->w, b->h,
	    YESNO(*opts & SWM_CW_RIGHT), YESNO(*opts & SWM_CW_LEFT),
	    YESNO(*opts & SWM_CW_BOTTOM), YESNO(*opts & SWM_CW_TOP),
	    YESNO(*opts & SWM_CW_RESIZABLE));

	if ((*opts & SWM_CW_RIGHT) && MAX_X(win) + BORDER(win) > b->x + b->w) {
		if (*opts & SWM_CW_RESIZABLE)
			WIDTH(win) = b->x + b->w - X(win) - BORDER(win);
		else
			X(win) = b->x + b->w - WIDTH(win) - BORDER(win);
	}

	if ((*opts & SWM_CW_LEFT) && X(win) + BORDER(win) < b->x) {
		if (*opts & SWM_CW_RESIZABLE)
			WIDTH(win) -= b->x - X(win) - BORDER(win);

		X(win) = b->x - BORDER(win);
	}

	if ((*opts & SWM_CW_BOTTOM) && MAX_Y(win) + BORDER(win) > b->y + b->h) {
		if (*opts & SWM_CW_RESIZABLE)
			HEIGHT(win) = b->y + b->h - Y(win) - BORDER(win);
		else
			Y(win) = b->y + b->h - HEIGHT(win) - BORDER(win);
	}

	if ((*opts & SWM_CW_TOP) && Y(win) + BORDER(win) < b->y) {
		if (*opts & SWM_CW_RESIZABLE)
			HEIGHT(win) -= b->y - Y(win) - BORDER(win);

		Y(win) = b->y - BORDER(win);
	}

	if (*opts & SWM_CW_RESIZABLE) {
		if (WIDTH(win) < 1)
			WIDTH(win) = 1;
		if (HEIGHT(win) < 1)
			HEIGHT(win) = 1;
	}
}

void
update_window_color(struct ws_win *win)
{
	uint32_t	*pixel;

	if (WS_FOCUSED(win->ws) && win->ws->focus == win)
		pixel = MAXIMIZED(win) ?
		    &win->s->c[SWM_S_COLOR_FOCUS_MAXIMIZED].pixel :
		    &win->s->c[SWM_S_COLOR_FOCUS].pixel;
	else
		pixel = MAXIMIZED(win) ?
		    &win->s->c[SWM_S_COLOR_UNFOCUS_MAXIMIZED].pixel :
		    &win->s->c[SWM_S_COLOR_UNFOCUS].pixel;

	xcb_change_window_attributes(conn, win->id,
	    XCB_CW_BORDER_PIXEL, pixel);
}

void
update_window(struct ws_win *win)
{
	uint16_t	mask;
	uint32_t	wc[5];

	mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
	    XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
	    XCB_CONFIG_WINDOW_BORDER_WIDTH;
	wc[0] = X(win);
	wc[1] = Y(win);
	wc[2] = WIDTH(win);
	wc[3] = HEIGHT(win);
	wc[4] = BORDER(win);

	DNPRINTF(SWM_D_EVENT, "update_window: win %#x, (x,y) w x h: "
	    "(%d,%d) %d x %d, bordered: %s\n", win->id, wc[0], wc[1], wc[2],
	    wc[3], YESNO(win->bordered));

	xcb_configure_window(conn, win->id, mask, wc);
}

#define SWM_RESIZE_STEPS	(50)

void
resize(struct ws_win *win, union arg *args)
{
	xcb_timestamp_t		timestamp = 0;
	struct swm_region	*r = NULL;
	struct swm_geometry	g;
	int			resize_stp = 0;
	int			top = 0, left = 0;
	int			dx, dy;
	xcb_cursor_t			cursor;
	xcb_query_pointer_reply_t	*xpr = NULL;
	xcb_generic_event_t		*evt;
	xcb_motion_notify_event_t	*mne;
	bool			resizing;

	if (win == NULL)
		return;
	r = win->ws->r;

	if (FULLSCREEN(win))
		return;

	/* In max_stack mode, should only resize transients. */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK] && !TRANS(win))
		return;

	DNPRINTF(SWM_D_EVENT, "resize: win %#x, floating: %s, "
	    "transient: %#x\n", win->id, YESNO(ABOVE(win)),
	    win->transient);

	if (MAXIMIZED(win))
		store_float_geom(win);
	else if (!(TRANS(win) || ABOVE(win)))
		return;

	ewmh_apply_flags(win, (win->ewmh_flags | SWM_F_MANUAL | EWMH_F_ABOVE) &
	    ~EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(win);

	stack();

	focus_flush();

	/* It's possible for win to have been freed during focus_flush(). */
	if (validate_win(win)) {
		DNPRINTF(SWM_D_EVENT, "resize: invalid win.\n");
		goto out;
	}

	switch (args->id) {
	case SWM_ARG_ID_WIDTHSHRINK:
		WIDTH(win) -= SWM_RESIZE_STEPS;
		resize_stp = 1;
		break;
	case SWM_ARG_ID_WIDTHGROW:
		WIDTH(win) += SWM_RESIZE_STEPS;
		resize_stp = 1;
		break;
	case SWM_ARG_ID_HEIGHTSHRINK:
		HEIGHT(win) -= SWM_RESIZE_STEPS;
		resize_stp = 1;
		break;
	case SWM_ARG_ID_HEIGHTGROW:
		HEIGHT(win) += SWM_RESIZE_STEPS;
		resize_stp = 1;
		break;
	default:
		break;
	}
	if (resize_stp) {
		region_containment(win, r, SWM_CW_ALLSIDES | SWM_CW_RESIZABLE |
		    SWM_CW_HARDBOUNDARY);
		update_window(win);
		store_float_geom(win);
		return;
	}

	region_containment(win, r, SWM_CW_ALLSIDES | SWM_CW_RESIZABLE |
	    SWM_CW_SOFTBOUNDARY);
	update_window(win);

	/* get cursor offset from window root */
	xpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win->id),
	    NULL);
	if (xpr == NULL)
		return;

	g = win->g;

	if (xpr->win_x < WIDTH(win) / 2)
		left = 1;

	if (xpr->win_y < HEIGHT(win) / 2)
		top = 1;

	if (args->id == SWM_ARG_ID_CENTER)
		cursor = cursors[XC_SIZING].cid;
	else if (top)
		cursor = cursors[left ? XC_TOP_LEFT_CORNER :
		    XC_TOP_RIGHT_CORNER].cid;
	else
		cursor = cursors[left ? XC_BOTTOM_LEFT_CORNER :
		    XC_BOTTOM_RIGHT_CORNER].cid;

	xcb_grab_pointer(conn, 0, win->id, MOUSEMASK,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE, cursor,
	    XCB_CURRENT_TIME),

	xcb_flush(conn);
	resizing = true;
	while (resizing && (evt = xcb_wait_for_event(conn))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_BUTTON_RELEASE:
			DNPRINTF(SWM_D_EVENT, "resize: BUTTON_RELEASE\n");
			resizing = false;
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)evt;
			/* cursor offset/delta from start of the operation */
			dx = mne->root_x - xpr->root_x;
			dy = mne->root_y - xpr->root_y;

			/* vertical */
			if (top)
				dy = -dy;

			if (args->id == SWM_ARG_ID_CENTER) {
				if (g.h / 2 + dy < 1)
					dy = 1 - g.h / 2;

				Y(win) = g.y - dy;
				HEIGHT(win) = g.h + 2 * dy;
			} else {
				if (g.h + dy < 1)
					dy = 1 - g.h;

				if (top)
					Y(win) = g.y - dy;

				HEIGHT(win) = g.h + dy;
			}

			/* horizontal */
			if (left)
				dx = -dx;

			if (args->id == SWM_ARG_ID_CENTER) {
				if (g.w / 2 + dx < 1)
					dx = 1 - g.w / 2;

				X(win) = g.x - dx;
				WIDTH(win) = g.w + 2 * dx;
			} else {
				if (g.w + dx < 1)
					dx = 1 - g.w;

				if (left)
					X(win) = g.x - dx;

				WIDTH(win) = g.w + dx;
			}

			/* not free, don't sync more than 120 times / second */
			if ((mne->time - timestamp) > (1000 / 120) ) {
				timestamp = mne->time;
				regionize(win, mne->root_x, mne->root_y);
				region_containment(win, r, SWM_CW_ALLSIDES |
				    SWM_CW_RESIZABLE | SWM_CW_HARDBOUNDARY |
				    SWM_CW_SOFTBOUNDARY);
				update_window(win);
				xcb_flush(conn);
			}
			break;
		case XCB_KEY_PRESS:
			/* Ignore. */
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    ((xcb_key_press_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		default:
			event_handle(evt);

			/* It's possible for win to have been freed above. */
			if (validate_win(win)) {
				DNPRINTF(SWM_D_EVENT, "resize: invalid win.\n");
				goto out;
			}
			break;
		}
		free(evt);
	}
	if (timestamp) {
		region_containment(win, r, SWM_CW_ALLSIDES | SWM_CW_RESIZABLE |
		    SWM_CW_HARDBOUNDARY | SWM_CW_SOFTBOUNDARY);
		update_window(win);
		xcb_flush(conn);
	}
	store_float_geom(win);
out:
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	free(xpr);
	DNPRINTF(SWM_D_EVENT, "resize: done.\n");
}

void
resize_step(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	resize(win, args);
	center_pointer(r);
	focus_flush();
}

/* Try to set window region based on supplied coordinates or window center. */
void
regionize(struct ws_win *win, int x, int y)
{
	struct swm_region *r = NULL;

	r = region_under(win->s, x, y);
	if (r == NULL)
		r = region_under(win->s, X(win) + WIDTH(win) / 2,
		    Y(win) + HEIGHT(win) / 2);

	if (r != NULL && r != win->ws->r) {
		if (clear_maximized(r->ws) > 0)
			stack();

		win_to_ws(win, r->ws->idx, false);

		/* Set focus on new ws. */
		unfocus_win(r->ws->focus);
		r->ws->focus = win;

		set_region(r);
		raise_window(win);
	}
}

#define SWM_MOVE_STEPS	(50)

void
move(struct ws_win *win, union arg *args)
{
	struct swm_region		*r;
	xcb_timestamp_t			timestamp = 0;
	int				move_stp = 0;
	xcb_query_pointer_reply_t	*qpr = NULL;
	xcb_generic_event_t		*evt;
	xcb_motion_notify_event_t	*mne;
	bool				moving, restack = false;

	if (win == NULL)
		return;

	if ((r = win->ws->r) == NULL)
		return;

	if (FULLSCREEN(win))
		return;

	DNPRINTF(SWM_D_EVENT, "move: win %#x, floating: %s, transient: "
	    "%#x\n", win->id, YESNO(ABOVE(win)), win->transient);

	/* in max_stack mode should only move transients */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK] && !TRANS(win))
		return;

	if (!(ABOVE(win) || TRANS(win)) || MAXIMIZED(win)) {
		store_float_geom(win);
		restack = true;
	}

	ewmh_apply_flags(win, (win->ewmh_flags | SWM_F_MANUAL | EWMH_F_ABOVE) &
	    ~EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(win);

	if (restack)
		stack();

	focus_flush();

	/* It's possible for win to have been freed during focus_flush(). */
	if (validate_win(win)) {
		DNPRINTF(SWM_D_EVENT, "move: invalid win.\n");
		goto out;
	}

	move_stp = 0;
	switch (args->id) {
	case SWM_ARG_ID_MOVELEFT:
		X(win) -= (SWM_MOVE_STEPS - border_width);
		move_stp = 1;
		break;
	case SWM_ARG_ID_MOVERIGHT:
		X(win) += (SWM_MOVE_STEPS - border_width);
		move_stp = 1;
		break;
	case SWM_ARG_ID_MOVEUP:
		Y(win) -= (SWM_MOVE_STEPS - border_width);
		move_stp = 1;
		break;
	case SWM_ARG_ID_MOVEDOWN:
		Y(win) += (SWM_MOVE_STEPS - border_width);
		move_stp = 1;
		break;
	default:
		break;
	}
	if (move_stp) {
		regionize(win, -1, -1);
		region_containment(win, win->ws->r, SWM_CW_ALLSIDES);
		update_window(win);
		store_float_geom(win);
		return;
	}

	xcb_grab_pointer(conn, 0, win->id, MOUSEMASK,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
	    XCB_WINDOW_NONE, cursors[XC_FLEUR].cid, XCB_CURRENT_TIME);

	/* get cursor offset from window root */
	qpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win->id),
		NULL);
	if (qpr == NULL) {
		xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
		return;
	}

	regionize(win, qpr->root_x, qpr->root_y);
	region_containment(win, win->ws->r, SWM_CW_ALLSIDES |
	    SWM_CW_SOFTBOUNDARY);
	update_window(win);
	xcb_flush(conn);
	moving = true;
	while (moving && (evt = xcb_wait_for_event(conn))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_BUTTON_RELEASE:
			DNPRINTF(SWM_D_EVENT, "move: BUTTON_RELEASE\n");
			moving = false;
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)evt;
			DNPRINTF(SWM_D_EVENT, "motion: root: %#x\n", mne->root);
			X(win) = mne->root_x - qpr->win_x - border_width;
			Y(win) = mne->root_y - qpr->win_y - border_width;

			/* not free, don't sync more than 120 times / second */
			if ((mne->time - timestamp) > (1000 / 120) ) {
				timestamp = mne->time;
				regionize(win, mne->root_x, mne->root_y);
				region_containment(win, win->ws->r,
				    SWM_CW_ALLSIDES | SWM_CW_SOFTBOUNDARY);
				update_window(win);
				xcb_flush(conn);
			}
			break;
		case XCB_KEY_PRESS:
			/* Ignore. */
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    ((xcb_key_press_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		default:
			event_handle(evt);

			/* It's possible for win to have been freed above. */
			if (validate_win(win)) {
				DNPRINTF(SWM_D_EVENT, "move: invalid win.\n");
				goto out;
			}
			break;
		}
		free(evt);
	}
	if (timestamp) {
		region_containment(win, win->ws->r, SWM_CW_ALLSIDES |
		    SWM_CW_SOFTBOUNDARY);
		update_window(win);
		xcb_flush(conn);
	}
	store_float_geom(win);

	/* New region set to fullscreen layout. */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK]) {
		stack();
		focus_flush();
	}

out:
	free(qpr);
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	DNPRINTF(SWM_D_EVENT, "move: done.\n");
}

void
move_step(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	if (!TRANS(win) && !ABOVE(win))
		return;

	move(win, args);
	center_pointer(r);
	focus_flush();
}

/* key definitions */
struct keyfunc {
	char			name[SWM_FUNCNAME_LEN];
	void			(*func)(struct swm_region *r, union arg *);
	union arg		args;
} keyfuncs[KF_INVALID + 1] = {
	/* name			function	argument */
	{ "bar_toggle",		bar_toggle,	{.id = SWM_ARG_ID_BAR_TOGGLE} },
	{ "bar_toggle_ws",	bar_toggle,	{.id = SWM_ARG_ID_BAR_TOGGLE_WS} },
	{ "button2",		pressbutton,	{2} },
	{ "cycle_layout",	cycle_layout,	{0} },
	{ "flip_layout",	stack_config,	{.id = SWM_ARG_ID_FLIPLAYOUT} },
	{ "float_toggle",	floating_toggle,{0} },
	{ "focus_main",		focus,		{.id = SWM_ARG_ID_FOCUSMAIN} },
	{ "focus_next",		focus,		{.id = SWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		{.id = SWM_ARG_ID_FOCUSPREV} },
	{ "focus_urgent",	focus,		{.id = SWM_ARG_ID_FOCUSURGENT} },
	{ "maximize_toggle",	maximize_toggle,{0} },
	{ "height_grow",	resize_step,	{.id = SWM_ARG_ID_HEIGHTGROW} },
	{ "height_shrink",	resize_step,	{.id = SWM_ARG_ID_HEIGHTSHRINK} },
	{ "iconify",		iconify,	{0} },
	{ "master_shrink",	stack_config,	{.id = SWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	{.id = SWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	{.id = SWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	{.id = SWM_ARG_ID_MASTERDEL} },
	{ "move_down",		move_step,	{.id = SWM_ARG_ID_MOVEDOWN} },
	{ "move_left",		move_step,	{.id = SWM_ARG_ID_MOVELEFT} },
	{ "move_right",		move_step,	{.id = SWM_ARG_ID_MOVERIGHT} },
	{ "move_up",		move_step,	{.id = SWM_ARG_ID_MOVEUP} },
	{ "mvrg_1",		send_to_rg,	{.id = 0} },
	{ "mvrg_2",		send_to_rg,	{.id = 1} },
	{ "mvrg_3",		send_to_rg,	{.id = 2} },
	{ "mvrg_4",		send_to_rg,	{.id = 3} },
	{ "mvrg_5",		send_to_rg,	{.id = 4} },
	{ "mvrg_6",		send_to_rg,	{.id = 5} },
	{ "mvrg_7",		send_to_rg,	{.id = 6} },
	{ "mvrg_8",		send_to_rg,	{.id = 7} },
	{ "mvrg_9",		send_to_rg,	{.id = 8} },
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
	{ "mvws_11",		send_to_ws,	{.id = 10} },
	{ "mvws_12",		send_to_ws,	{.id = 11} },
	{ "mvws_13",		send_to_ws,	{.id = 12} },
	{ "mvws_14",		send_to_ws,	{.id = 13} },
	{ "mvws_15",		send_to_ws,	{.id = 14} },
	{ "mvws_16",		send_to_ws,	{.id = 15} },
	{ "mvws_17",		send_to_ws,	{.id = 16} },
	{ "mvws_18",		send_to_ws,	{.id = 17} },
	{ "mvws_19",		send_to_ws,	{.id = 18} },
	{ "mvws_20",		send_to_ws,	{.id = 19} },
	{ "mvws_21",		send_to_ws,	{.id = 20} },
	{ "mvws_22",		send_to_ws,	{.id = 21} },
	{ "name_workspace",	name_workspace,	{0} },
	{ "quit",		quit,		{0} },
	{ "raise_toggle",	raise_toggle,	{0} },
	{ "restart",		restart,	{0} },
	{ "rg_1",		focusrg,	{.id = 0} },
	{ "rg_2",		focusrg,	{.id = 1} },
	{ "rg_3",		focusrg,	{.id = 2} },
	{ "rg_4",		focusrg,	{.id = 3} },
	{ "rg_5",		focusrg,	{.id = 4} },
	{ "rg_6",		focusrg,	{.id = 5} },
	{ "rg_7",		focusrg,	{.id = 6} },
	{ "rg_8",		focusrg,	{.id = 7} },
	{ "rg_9",		focusrg,	{.id = 8} },
	{ "rg_next",		cyclerg,	{.id = SWM_ARG_ID_CYCLERG_UP} },
	{ "rg_prev",		cyclerg,	{.id = SWM_ARG_ID_CYCLERG_DOWN} },
	{ "screen_next",	cyclerg,	{.id = SWM_ARG_ID_CYCLERG_UP} },
	{ "screen_prev",	cyclerg,	{.id = SWM_ARG_ID_CYCLERG_DOWN} },
	{ "search_win",		search_win,	{0} },
	{ "search_workspace",	search_workspace,	{0} },
	{ "spawn_custom",	NULL,		{0} },
	{ "stack_inc",		stack_config,	{.id = SWM_ARG_ID_STACKINC} },
	{ "stack_dec",		stack_config,	{.id = SWM_ARG_ID_STACKDEC} },
	{ "stack_reset",	stack_config,	{.id = SWM_ARG_ID_STACKRESET} },
	{ "swap_main",		swapwin,	{.id = SWM_ARG_ID_SWAPMAIN} },
	{ "swap_next",		swapwin,	{.id = SWM_ARG_ID_SWAPNEXT} },
	{ "swap_prev",		swapwin,	{.id = SWM_ARG_ID_SWAPPREV} },
	{ "uniconify",		uniconify,	{0} },
	{ "version",		version,	{0} },
	{ "width_grow",		resize_step,	{.id = SWM_ARG_ID_WIDTHGROW} },
	{ "width_shrink",	resize_step,	{.id = SWM_ARG_ID_WIDTHSHRINK} },
	{ "wind_del",		wkill,		{.id = SWM_ARG_ID_DELETEWINDOW} },
	{ "wind_kill",		wkill,		{.id = SWM_ARG_ID_KILLWINDOW} },
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
	{ "ws_11",		switchws,	{.id = 10} },
	{ "ws_12",		switchws,	{.id = 11} },
	{ "ws_13",		switchws,	{.id = 12} },
	{ "ws_14",		switchws,	{.id = 13} },
	{ "ws_15",		switchws,	{.id = 14} },
	{ "ws_16",		switchws,	{.id = 15} },
	{ "ws_17",		switchws,	{.id = 16} },
	{ "ws_18",		switchws,	{.id = 17} },
	{ "ws_19",		switchws,	{.id = 18} },
	{ "ws_20",		switchws,	{.id = 19} },
	{ "ws_21",		switchws,	{.id = 20} },
	{ "ws_22",		switchws,	{.id = 21} },
	{ "ws_next",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP} },
	{ "ws_next_all",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP_ALL} },
	{ "ws_next_move",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_MOVE_UP} },
	{ "ws_prev",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN} },
	{ "ws_prev_all",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN_ALL} },
	{ "ws_prev_move",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_MOVE_DOWN} },
	{ "ws_prior",		priorws,	{0} },
	{ "dumpwins",		dumpwins,	{0} }, /* MUST BE LAST */
	{ "invalid key func",	NULL,		{0} },
};

int
key_cmp(struct key *kp1, struct key *kp2)
{
	if (kp1->keysym < kp2->keysym)
		return (-1);
	if (kp1->keysym > kp2->keysym)
		return (1);

	if (kp1->mod < kp2->mod)
		return (-1);
	if (kp1->mod > kp2->mod)
		return (1);

	return (0);
}

/* mouse */
enum { client_click, root_click };
struct button {
	unsigned int		action;
	unsigned int		mask;
	unsigned int		button;
	void			(*func)(struct ws_win *, union arg *);
	union arg		args;
} buttons[] = {
#define MODKEY_SHIFT	MODKEY | XCB_MOD_MASK_SHIFT
	  /* action	key		mouse button	func	args */
	{ client_click,	MODKEY,		XCB_BUTTON_INDEX_3,	resize,	{.id = SWM_ARG_ID_DONTCENTER} },
	{ client_click,	MODKEY_SHIFT,	XCB_BUTTON_INDEX_3,	resize,	{.id = SWM_ARG_ID_CENTER} },
	{ client_click,	MODKEY,		XCB_BUTTON_INDEX_1,	move,	{0} },
#undef MODKEY_SHIFT
};

void
update_modkey(unsigned int mod)
{
	int			i;
	struct key		*kp;

	mod_key = mod;
	RB_FOREACH(kp, key_tree, &keys)
		if (kp->mod & XCB_MOD_MASK_SHIFT)
			kp->mod = mod | XCB_MOD_MASK_SHIFT;
		else
			kp->mod = mod;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].mask & XCB_MOD_MASK_SHIFT)
			buttons[i].mask = mod | XCB_MOD_MASK_SHIFT;
		else
			buttons[i].mask = mod;
}

int
spawn_expand(struct swm_region *r, union arg *args, const char *spawn_name,
    char ***ret_args)
{
	struct spawn_prog	*prog = NULL;
	int			i, c;
	char			*ap, **real_args;

	/* suppress unused warning since var is needed */
	(void)args;

	DNPRINTF(SWM_D_SPAWN, "spawn_expand: %s\n", spawn_name);

	/* find program */
	TAILQ_FOREACH(prog, &spawns, entry) {
		if (strcasecmp(spawn_name, prog->name) == 0)
			break;
	}
	if (prog == NULL) {
		warnx("spawn_custom: program %s not found", spawn_name);
		return (-1);
	}

	/* make room for expanded args */
	if ((real_args = calloc(prog->argc + 1, sizeof(char *))) == NULL)
		err(1, "spawn_custom: calloc real_args");

	/* expand spawn_args into real_args */
	for (i = c = 0; i < prog->argc; i++) {
		ap = prog->argv[i];
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: raw arg: %s\n", ap);
		if (strcasecmp(ap, "$bar_border") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_BORDER].name))
			    == NULL)
				err(1,  "spawn_custom border color");
		} else if (strcasecmp(ap, "$bar_color") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_BAR].name))
			    == NULL)
				err(1, "spawn_custom bar color");
		} else if (strcasecmp(ap, "$bar_font") == 0) {
			if ((real_args[c] = strdup(bar_fonts))
			    == NULL)
				err(1, "spawn_custom bar fonts");
		} else if (strcasecmp(ap, "$bar_font_color") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_FONT].name))
			    == NULL)
				err(1, "spawn_custom color font");
		} else if (strcasecmp(ap, "$color_focus") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_FOCUS].name))
			    == NULL)
				err(1, "spawn_custom color focus");
		} else if (strcasecmp(ap, "$color_focus_maximized") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_FOCUS_MAXIMIZED].name))
			    == NULL)
				err(1, "spawn_custom color focus maximized");
		} else if (strcasecmp(ap, "$color_unfocus") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_UNFOCUS].name))
			    == NULL)
				err(1, "spawn_custom color unfocus");
		} else if (strcasecmp(ap, "$color_unfocus_maximized") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_UNFOCUS_MAXIMIZED].name))
			    == NULL)
				err(1, "spawn_custom color unfocus maximized");
		} else if (strcasecmp(ap, "$region_index") == 0) {
			if (asprintf(&real_args[c], "%d",
			    get_region_index(r) + 1) < 1)
				err(1, "spawn_custom region index");
		} else if (strcasecmp(ap, "$workspace_index") == 0) {
			if (asprintf(&real_args[c], "%d", r->ws->idx + 1) < 1)
				err(1, "spawn_custom workspace index");
		} else if (strcasecmp(ap, "$dmenu_bottom") == 0) {
			if (!bar_at_bottom)
				continue;
			if ((real_args[c] = strdup("-b")) == NULL)
				err(1, "spawn_custom workspace index");
		} else {
			/* no match --> copy as is */
			if ((real_args[c] = strdup(ap)) == NULL)
				err(1, "spawn_custom strdup(ap)");
		}
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: cooked arg: %s\n",
		    real_args[c]);
		++c;
	}

#ifdef SWM_DEBUG
	DNPRINTF(SWM_D_SPAWN, "spawn_custom: result: ");
	for (i = 0; i < c; ++i)
		DPRINTF("\"%s\" ", real_args[i]);
	DPRINTF("\n");
#endif
	*ret_args = real_args;
	return (c);
}

void
spawn_custom(struct swm_region *r, union arg *args, const char *spawn_name)
{
	union arg		a;
	char			**real_args;
	int			spawn_argc, i;

	if ((spawn_argc = spawn_expand(r, args, spawn_name, &real_args)) < 0)
		return;
	a.argv = real_args;
	if (fork() == 0)
		spawn(r->ws->idx, &a, true);

	for (i = 0; i < spawn_argc; i++)
		free(real_args[i]);
	free(real_args);
}

void
spawn_select(struct swm_region *r, union arg *args, const char *spawn_name,
    int *pid)
{
	union arg		a;
	char			**real_args;
	int			i, spawn_argc;

	if ((spawn_argc = spawn_expand(r, args, spawn_name, &real_args)) < 0)
		return;
	a.argv = real_args;

	if (pipe(select_list_pipe) == -1)
		err(1, "pipe error");
	if (pipe(select_resp_pipe) == -1)
		err(1, "pipe error");

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		err(1, "could not disable SIGPIPE");
	switch (*pid = fork()) {
	case -1:
		err(1, "cannot fork");
		break;
	case 0: /* child */
		if (dup2(select_list_pipe[0], STDIN_FILENO) == -1)
			err(1, "dup2");
		if (dup2(select_resp_pipe[1], STDOUT_FILENO) == -1)
			err(1, "dup2");
		close(select_list_pipe[1]);
		close(select_resp_pipe[0]);
		spawn(r->ws->idx, &a, false);
		break;
	default: /* parent */
		close(select_list_pipe[0]);
		close(select_resp_pipe[1]);
		break;
	}

	for (i = 0; i < spawn_argc; i++)
		free(real_args[i]);
	free(real_args);
}

/* Argument tokenizer. */
char *
argsep(char **sp) {
	char			*arg, *cp, *next;
	bool			single_quoted = false, double_quoted = false;

	if (*sp == NULL)
		return NULL;

	/* Eat and move characters until end of argument is found. */
	for (arg = next = cp = *sp; *cp != '\0'; ++cp) {
		if (!double_quoted && *cp == '\'') {
			/* Eat single-quote. */
			single_quoted = !single_quoted;
		} else if (!single_quoted && *cp == '"') {
			/* Eat double-quote. */
			double_quoted = !double_quoted;
		} else if (!single_quoted && *cp == '\\' && *(cp + 1) == '"') {
			/* Eat backslash; copy escaped character to arg. */
			*next++ = *(++cp);
		} else if (!single_quoted && !double_quoted && *cp == '\\' &&
		    (*(cp + 1) == '\'' || *(cp + 1) == ' ')) {
			/* Eat backslash; move escaped character. */
			*next++ = *(++cp);
		} else if (!single_quoted && !double_quoted &&
		    (*cp == ' ' || *cp == '\t')) {
			/* Terminate argument. */
			*next++ = '\0';
			/* Point sp to beginning of next argument. */
			*sp = ++cp;
			break;
		} else {
			/* Move regular character. */
			*next++ = *cp;
		}
	}

	/* Terminate argument if end of string. */
	if (*cp == '\0') {
		*next = '\0';
		*sp = NULL;
	}

	return arg;
}

void
spawn_insert(const char *name, const char *args, int flags)
{
	struct spawn_prog	*sp;
	char			*arg, *cp, *ptr;

	DNPRINTF(SWM_D_SPAWN, "spawn_insert: %s[%s]\n", name, args);

	if (args == NULL || *args == '\0')
		return;

	if ((sp = calloc(1, sizeof *sp)) == NULL)
		err(1, "spawn_insert: calloc");
	if ((sp->name = strdup(name)) == NULL)
		err(1, "spawn_insert: strdup");

	/* Convert the arguments to an argument list. */
	if ((ptr = cp = strdup(args)) == NULL)
		err(1, "spawn_insert: strdup");
	while ((arg = argsep(&ptr)) != NULL) {
		/* Null argument; skip it. */
		if (*arg == '\0')
			continue;

		sp->argc++;
		if ((sp->argv = realloc(sp->argv, sp->argc *
		    sizeof *sp->argv)) == NULL)
			err(1, "spawn_insert: realloc");
		if ((sp->argv[sp->argc - 1] = strdup(arg)) == NULL)
			err(1, "spawn_insert: strdup");
	}
	free(cp);

	sp->flags = flags;

	DNPRINTF(SWM_D_SPAWN, "arg %d: [%s]\n", sp->argc, sp->argv[sp->argc-1]);
	TAILQ_INSERT_TAIL(&spawns, sp, entry);
	DNPRINTF(SWM_D_SPAWN, "spawn_insert: leave\n");
}

void
spawn_remove(struct spawn_prog *sp)
{
	int			i;

	DNPRINTF(SWM_D_SPAWN, "spawn_remove: %s\n", sp->name);

	TAILQ_REMOVE(&spawns, sp, entry);
	for (i = 0; i < sp->argc; i++)
		free(sp->argv[i]);
	free(sp->argv);
	free(sp->name);
	free(sp);

	DNPRINTF(SWM_D_SPAWN, "spawn_remove: leave\n");
}

struct spawn_prog*
spawn_find(const char *name)
{
	struct spawn_prog	*sp;

	TAILQ_FOREACH(sp, &spawns, entry)
		if (strcasecmp(sp->name, name) == 0)
			return sp;

	return NULL;
}

void
setspawn(const char *name, const char *args, int flags)
{
	struct spawn_prog	*sp;

	DNPRINTF(SWM_D_SPAWN, "setspawn: %s\n", name);

	if (name == NULL)
		return;

	/* Remove any old spawn under the same name. */
	if ((sp = spawn_find(name)) != NULL)
		spawn_remove(sp);

	if (*args != '\0')
		spawn_insert(name, args, flags);
	else
		warnx("error: setspawn: cannot find program: %s", name);

	DNPRINTF(SWM_D_SPAWN, "setspawn: leave\n");
}

int
setconfspawn(const char *selector, const char *value, int flags)
{
	char		*args;

	if (selector == NULL || strlen(selector) == 0)
		return (1);

	args = expand_tilde(value);

	DNPRINTF(SWM_D_SPAWN, "setconfspawn: [%s] [%s]\n", selector, args);

	setspawn(selector, args, flags);
	free(args);

	DNPRINTF(SWM_D_SPAWN, "setconfspawn: done.\n");
	return (0);
}

void
validate_spawns(void)
{
	struct spawn_prog	*sp;
	char			which[PATH_MAX];
	size_t			i;

	struct key		*kp;

	RB_FOREACH(kp, key_tree, &keys) {
		if (kp->funcid != KF_SPAWN_CUSTOM)
			continue;

		/* find program */
		sp = spawn_find(kp->spawn_name);
		if (sp == NULL || sp->flags & SWM_SPAWN_OPTIONAL)
			continue;

		/* verify we have the goods */
		snprintf(which, sizeof which, "which %s", sp->argv[0]);
		DNPRINTF(SWM_D_CONF, "validate_spawns: which %s\n",
		    sp->argv[0]);
		for (i = strlen("which "); i < strlen(which); i++)
			if (which[i] == ' ') {
				which[i] = '\0';
				break;
			}
		if (system(which) != 0)
			add_startup_exception("could not find %s",
			    &which[strlen("which ")]);
	}
}

void
setup_spawn(void)
{
	setconfspawn("lock",		"xlock",		0);

	setconfspawn("term",		"xterm",		0);
	setconfspawn("spawn_term",	"xterm",		0);

	setconfspawn("menu",		"dmenu_run"
					" $dmenu_bottom"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);

	setconfspawn("search",		"dmenu"
					" $dmenu_bottom"
					" -i"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);

	setconfspawn("name_workspace",	"dmenu"
					" $dmenu_bottom"
					" -p Workspace"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);

	 /* These are not verified for existence, even with a binding set. */
	setconfspawn("screenshot_all",	"screenshot.sh full",	SWM_SPAWN_OPTIONAL);
	setconfspawn("screenshot_wind",	"screenshot.sh window",	SWM_SPAWN_OPTIONAL);
	setconfspawn("initscr",		"initscreen.sh",	SWM_SPAWN_OPTIONAL);
}

/* key bindings */
#define SWM_MODNAME_SIZE	32
#define SWM_KEY_WS		"\n+ \t"
int
parsekeys(const char *keystr, unsigned int currmod, unsigned int *mod, KeySym *ks)
{
	char			*str, *cp, *name;
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

	if ((cp = str = strdup(keystr)) == NULL)
		err(1, "parsekeys: strdup");

	*ks = XCB_NO_SYMBOL;
	*mod = 0;
	while ((name = strsep(&cp, SWM_KEY_WS)) != NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: key [%s]\n", name);
		if (cp)
			cp += (long)strspn(cp, SWM_KEY_WS);
		if (strncasecmp(name, "MOD", SWM_MODNAME_SIZE) == 0)
			*mod |= currmod;
		else if (strncasecmp(name, "Mod1", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_1;
		else if (strncasecmp(name, "Mod2", SWM_MODNAME_SIZE) == 0)
			*mod += XCB_MOD_MASK_2;
		else if (strncmp(name, "Mod3", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_3;
		else if (strncmp(name, "Mod4", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_4;
		else if (strncasecmp(name, "SHIFT", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_SHIFT;
		else if (strncasecmp(name, "CONTROL", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_CONTROL;
		else {
			*ks = XStringToKeysym(name);
			XConvertCase(*ks, ks, &uks);
			if (ks == XCB_NO_SYMBOL) {
				DNPRINTF(SWM_D_KEY,
				    "parsekeys: invalid key %s\n",
				    name);
				free(str);
				return (1);
			}
		}
	}

	free(str);
	DNPRINTF(SWM_D_KEY, "parsekeys: leave ok\n");
	return (0);
}

char *
strdupsafe(const char *str)
{
	if (str == NULL)
		return (NULL);
	else
		return (strdup(str));
}

void
key_insert(unsigned int mod, KeySym ks, enum keyfuncid kfid,
    const char *spawn_name)
{
	struct key		*kp;

	DNPRINTF(SWM_D_KEY, "key_insert: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);

	if ((kp = malloc(sizeof *kp)) == NULL)
		err(1, "key_insert: malloc");

	kp->mod = mod;
	kp->keysym = ks;
	kp->funcid = kfid;
	kp->spawn_name = strdupsafe(spawn_name);
	RB_INSERT(key_tree, &keys, kp);

	DNPRINTF(SWM_D_KEY, "key_insert: leave\n");
}

struct key *
key_lookup(unsigned int mod, KeySym ks)
{
	struct key		kp;

	kp.keysym = ks;
	kp.mod = mod;

	return (RB_FIND(key_tree, &keys, &kp));
}

void
key_remove(struct key *kp)
{
	DNPRINTF(SWM_D_KEY, "key_remove: %s\n", keyfuncs[kp->funcid].name);

	RB_REMOVE(key_tree, &keys, kp);
	free(kp->spawn_name);
	free(kp);

	DNPRINTF(SWM_D_KEY, "key_remove: leave\n");
}

void
key_replace(struct key *kp, unsigned int mod, KeySym ks, enum keyfuncid kfid,
    const char *spawn_name)
{
	DNPRINTF(SWM_D_KEY, "key_replace: %s [%s]\n", keyfuncs[kp->funcid].name,
	    spawn_name);

	key_remove(kp);
	key_insert(mod, ks, kfid, spawn_name);

	DNPRINTF(SWM_D_KEY, "key_replace: leave\n");
}

void
setkeybinding(unsigned int mod, KeySym ks, enum keyfuncid kfid,
    const char *spawn_name)
{
	struct key		*kp;

	DNPRINTF(SWM_D_KEY, "setkeybinding: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);

	if ((kp = key_lookup(mod, ks)) != NULL) {
		if (kfid == KF_INVALID)
			key_remove(kp);
		else
			key_replace(kp, mod, ks, kfid, spawn_name);
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}
	if (kfid == KF_INVALID) {
		warnx("bind: Key combination already unbound.");
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}

	key_insert(mod, ks, kfid, spawn_name);
	DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
}

int
setconfbinding(const char *selector, const char *value, int flags)
{
	enum keyfuncid		kfid;
	unsigned int		mod;
	KeySym			ks;
	struct spawn_prog	*sp;

	/* suppress unused warning since var is needed */
	(void)flags;

	DNPRINTF(SWM_D_KEY, "setconfbinding: enter selector: [%s], "
	    "value: [%s]\n", selector, value);
	if (selector == NULL || strlen(selector) == 0) {
		DNPRINTF(SWM_D_KEY, "setconfbinding: unbind %s\n", value);
		if (parsekeys(value, mod_key, &mod, &ks) == 0) {
			kfid = KF_INVALID;
			setkeybinding(mod, ks, kfid, NULL);
			return (0);
		} else
			return (1);
	}
	/* search by key function name */
	for (kfid = 0; kfid < KF_INVALID; (kfid)++) {
		if (strncasecmp(selector, keyfuncs[kfid].name,
		    SWM_FUNCNAME_LEN) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match "
			    "keyfunc\n", selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kfid, NULL);
				return (0);
			} else
				return (1);
		}
	}
	/* search by custom spawn name */
	if ((sp = spawn_find(selector)) != NULL) {
		DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match "
		    "spawn\n", selector);
		if (parsekeys(value, mod_key, &mod, &ks) == 0) {
			setkeybinding(mod, ks, KF_SPAWN_CUSTOM,
			    sp->name);
			return (0);
		} else
			return (1);
	}
	DNPRINTF(SWM_D_KEY, "setconfbinding: no match\n");
	return (1);
}

void
setup_keys(void)
{
#define MODKEY_SHIFT	MODKEY | XCB_MOD_MASK_SHIFT
	setkeybinding(MODKEY,		XK_b,		KF_BAR_TOGGLE,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_b,		KF_BAR_TOGGLE_WS,NULL);
	setkeybinding(MODKEY,		XK_v,		KF_BUTTON2,	NULL);
	setkeybinding(MODKEY,		XK_space,	KF_CYCLE_LAYOUT,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_backslash,	KF_FLIP_LAYOUT,	NULL);
	setkeybinding(MODKEY,		XK_t,		KF_FLOAT_TOGGLE,NULL);
	setkeybinding(MODKEY,		XK_m,		KF_FOCUS_MAIN,	NULL);
	setkeybinding(MODKEY,		XK_j,		KF_FOCUS_NEXT,	NULL);
	setkeybinding(MODKEY,		XK_Tab,		KF_FOCUS_NEXT,	NULL);
	setkeybinding(MODKEY,		XK_k,		KF_FOCUS_PREV,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_Tab,		KF_FOCUS_PREV,	NULL);
	setkeybinding(MODKEY,		XK_u,		KF_FOCUS_URGENT,NULL);
	setkeybinding(MODKEY,		XK_e,		KF_MAXIMIZE_TOGGLE,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_equal,	KF_HEIGHT_GROW,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_minus,	KF_HEIGHT_SHRINK,NULL);
	setkeybinding(MODKEY,		XK_w,		KF_ICONIFY,	NULL);
	setkeybinding(MODKEY,		XK_h,		KF_MASTER_SHRINK, NULL);
	setkeybinding(MODKEY,		XK_l,		KF_MASTER_GROW,	NULL);
	setkeybinding(MODKEY,		XK_comma,	KF_MASTER_ADD,	NULL);
	setkeybinding(MODKEY,		XK_period,	KF_MASTER_DEL,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_bracketright,KF_MOVE_DOWN,NULL);
	setkeybinding(MODKEY,		XK_bracketleft,	KF_MOVE_LEFT,NULL);
	setkeybinding(MODKEY,		XK_bracketright,KF_MOVE_RIGHT,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_bracketleft,	KF_MOVE_UP,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_End,	KF_MVRG_1,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Down,	KF_MVRG_2,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Next,	KF_MVRG_3,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Left,	KF_MVRG_4,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Begin,	KF_MVRG_5,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Right,	KF_MVRG_6,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Home,	KF_MVRG_7,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Up,	KF_MVRG_8,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_KP_Prior,	KF_MVRG_9,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_1,		KF_MVWS_1,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_2,		KF_MVWS_2,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_3,		KF_MVWS_3,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_4,		KF_MVWS_4,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_5,		KF_MVWS_5,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_6,		KF_MVWS_6,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_7,		KF_MVWS_7,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_8,		KF_MVWS_8,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_9,		KF_MVWS_9,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_0,		KF_MVWS_10,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F1,		KF_MVWS_11,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F2,		KF_MVWS_12,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F3,		KF_MVWS_13,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F4,		KF_MVWS_14,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F5,		KF_MVWS_15,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F6,		KF_MVWS_16,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F7,		KF_MVWS_17,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F8,		KF_MVWS_18,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F9,		KF_MVWS_19,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F10,		KF_MVWS_20,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F11,		KF_MVWS_21,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_F12,		KF_MVWS_22,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_slash,	KF_NAME_WORKSPACE,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_q,		KF_QUIT,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_r,		KF_RAISE_TOGGLE,NULL);
	setkeybinding(MODKEY,		XK_q,		KF_RESTART,	NULL);
	setkeybinding(MODKEY,		XK_KP_End,	KF_RG_1,	NULL);
	setkeybinding(MODKEY,		XK_KP_Down,	KF_RG_2,	NULL);
	setkeybinding(MODKEY,		XK_KP_Next,	KF_RG_3,	NULL);
	setkeybinding(MODKEY,		XK_KP_Left,	KF_RG_4,	NULL);
	setkeybinding(MODKEY,		XK_KP_Begin,	KF_RG_5,	NULL);
	setkeybinding(MODKEY,		XK_KP_Right,	KF_RG_6,	NULL);
	setkeybinding(MODKEY,		XK_KP_Home,	KF_RG_7,	NULL);
	setkeybinding(MODKEY,		XK_KP_Up,	KF_RG_8,	NULL);
	setkeybinding(MODKEY,		XK_KP_Prior,	KF_RG_9,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_Right,	KF_RG_NEXT,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_Left,	KF_RG_PREV,	NULL);
	setkeybinding(MODKEY,		XK_f,		KF_SEARCH_WIN,	NULL);
	setkeybinding(MODKEY,		XK_slash,	KF_SEARCH_WORKSPACE,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_i,		KF_SPAWN_CUSTOM,"initscr");
	setkeybinding(MODKEY_SHIFT,	XK_Delete,	KF_SPAWN_CUSTOM,"lock");
	setkeybinding(MODKEY,		XK_p,		KF_SPAWN_CUSTOM,"menu");
	setkeybinding(MODKEY,		XK_s,		KF_SPAWN_CUSTOM,"screenshot_all");
	setkeybinding(MODKEY_SHIFT,	XK_s,		KF_SPAWN_CUSTOM,"screenshot_wind");
	setkeybinding(MODKEY_SHIFT,	XK_Return,	KF_SPAWN_CUSTOM,"term");
	setkeybinding(MODKEY_SHIFT,	XK_comma,	KF_STACK_INC,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_period,	KF_STACK_DEC,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_space,	KF_STACK_RESET,	NULL);
	setkeybinding(MODKEY,		XK_Return,	KF_SWAP_MAIN,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_j,		KF_SWAP_NEXT,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_k,		KF_SWAP_PREV,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_w,		KF_UNICONIFY,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_v,		KF_VERSION,	NULL);
	setkeybinding(MODKEY,		XK_equal,	KF_WIDTH_GROW,	NULL);
	setkeybinding(MODKEY,		XK_minus,	KF_WIDTH_SHRINK,NULL);
	setkeybinding(MODKEY,		XK_x,		KF_WIND_DEL,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_x,		KF_WIND_KILL,	NULL);
	setkeybinding(MODKEY,		XK_1,		KF_WS_1,	NULL);
	setkeybinding(MODKEY,		XK_2,		KF_WS_2,	NULL);
	setkeybinding(MODKEY,		XK_3,		KF_WS_3,	NULL);
	setkeybinding(MODKEY,		XK_4,		KF_WS_4,	NULL);
	setkeybinding(MODKEY,		XK_5,		KF_WS_5,	NULL);
	setkeybinding(MODKEY,		XK_6,		KF_WS_6,	NULL);
	setkeybinding(MODKEY,		XK_7,		KF_WS_7,	NULL);
	setkeybinding(MODKEY,		XK_8,		KF_WS_8,	NULL);
	setkeybinding(MODKEY,		XK_9,		KF_WS_9,	NULL);
	setkeybinding(MODKEY,		XK_0,		KF_WS_10,	NULL);
	setkeybinding(MODKEY,		XK_F1,		KF_WS_11,	NULL);
	setkeybinding(MODKEY,		XK_F2,		KF_WS_12,	NULL);
	setkeybinding(MODKEY,		XK_F3,		KF_WS_13,	NULL);
	setkeybinding(MODKEY,		XK_F4,		KF_WS_14,	NULL);
	setkeybinding(MODKEY,		XK_F5,		KF_WS_15,	NULL);
	setkeybinding(MODKEY,		XK_F6,		KF_WS_16,	NULL);
	setkeybinding(MODKEY,		XK_F7,		KF_WS_17,	NULL);
	setkeybinding(MODKEY,		XK_F8,		KF_WS_18,	NULL);
	setkeybinding(MODKEY,		XK_F9,		KF_WS_19,	NULL);
	setkeybinding(MODKEY,		XK_F10,		KF_WS_20,	NULL);
	setkeybinding(MODKEY,		XK_F11,		KF_WS_21,	NULL);
	setkeybinding(MODKEY,		XK_F12,		KF_WS_22,	NULL);
	setkeybinding(MODKEY,		XK_Right,	KF_WS_NEXT,	NULL);
	setkeybinding(MODKEY,		XK_Left,	KF_WS_PREV,	NULL);
	setkeybinding(MODKEY,		XK_Up,		KF_WS_NEXT_ALL,	NULL);
	setkeybinding(MODKEY,		XK_Down,	KF_WS_PREV_ALL,	NULL);
	setkeybinding(MODKEY_SHIFT,	XK_Up,		KF_WS_NEXT_MOVE,NULL);
	setkeybinding(MODKEY_SHIFT,	XK_Down,	KF_WS_PREV_MOVE,NULL);
	setkeybinding(MODKEY,		XK_a,		KF_WS_PRIOR,	NULL);
#ifdef SWM_DEBUG
	setkeybinding(MODKEY_SHIFT,	XK_d,		KF_DUMPWINS,	NULL);
#endif
#undef MODKEY_SHIFT
}

void
clear_keys(void)
{
	struct key		*kp;

	while (RB_EMPTY(&keys) == 0) {
		kp = RB_ROOT(&keys);
		key_remove(kp);
	}
}

int
setkeymapping(const char *selector, const char *value, int flags)
{
	char			*keymapping_file;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	DNPRINTF(SWM_D_KEY, "setkeymapping: enter\n");

	keymapping_file = expand_tilde(value);

	clear_keys();
	/* load new key bindings; if it fails, revert to default bindings */
	if (conf_load(keymapping_file, SWM_CONF_KEYMAPPING)) {
		clear_keys();
		setup_keys();
	}

	free(keymapping_file);

	DNPRINTF(SWM_D_KEY, "setkeymapping: leave\n");
	return (0);
}

void
updatenumlockmask(void)
{
	unsigned int				i, j;
	xcb_get_modifier_mapping_reply_t	*modmap_r;
	xcb_keycode_t				*modmap, kc, *keycode;

	numlockmask = 0;

	modmap_r = xcb_get_modifier_mapping_reply(conn,
	    xcb_get_modifier_mapping(conn),
	    NULL);
	if (modmap_r) {
		modmap = xcb_get_modifier_mapping_keycodes(modmap_r);
		for (i = 0; i < 8; i++) {
			for (j = 0; j < modmap_r->keycodes_per_modifier; j++) {
				kc = modmap[i * modmap_r->keycodes_per_modifier
				    + j];
				keycode = xcb_key_symbols_get_keycode(syms,
						XK_Num_Lock);
				if (keycode) {
					if (kc == *keycode)
						numlockmask = (1 << i);
					free(keycode);
				}
			}
		}
		free(modmap_r);
	}
	DNPRINTF(SWM_D_MISC, "updatenumlockmask: %d\n", numlockmask);
}

void
grabkeys(void)
{
	struct key		*kp;
	int			num_screens, k, j;
	unsigned int		modifiers[4];
	xcb_keycode_t		*code;

	DNPRINTF(SWM_D_MISC, "grabkeys\n");
	updatenumlockmask();

	modifiers[0] = 0;
	modifiers[1] = numlockmask;
	modifiers[2] = XCB_MOD_MASK_LOCK;
	modifiers[3] = numlockmask | XCB_MOD_MASK_LOCK;

	num_screens = get_screen_count();
	for (k = 0; k < num_screens; k++) {
		if (TAILQ_EMPTY(&screens[k].rl))
			continue;
		xcb_ungrab_key(conn, XCB_GRAB_ANY, screens[k].root,
			XCB_MOD_MASK_ANY);
		RB_FOREACH(kp, key_tree, &keys) {
			/* Skip unused ws binds. */
			if ((int)kp->funcid > KF_WS_1 + workspace_limit - 1 &&
			    kp->funcid <= KF_WS_22)
				continue;

			/* Skip unused mvws binds. */
			if ((int)kp->funcid > KF_MVWS_1 + workspace_limit - 1 &&
			    kp->funcid <= KF_MVWS_22)
				continue;

			if ((code = xcb_key_symbols_get_keycode(syms,
					kp->keysym))) {
				for (j = 0; j < LENGTH(modifiers); j++)
					xcb_grab_key(conn, 1,
					    screens[k].root,
					    kp->mod | modifiers[j],
					    *code, XCB_GRAB_MODE_SYNC,
					    XCB_GRAB_MODE_SYNC);
				free(code);
			}
		}
	}
}

void
grabbuttons(struct ws_win *win)
{
	unsigned int	modifiers[4];
	int		i, j;

	DNPRINTF(SWM_D_MOUSE, "grabbuttons: win %#x\n", win->id);
	updatenumlockmask();

	modifiers[0] = 0;
	modifiers[1] = numlockmask;
	modifiers[2] = XCB_MOD_MASK_LOCK;
	modifiers[3] = numlockmask | XCB_MOD_MASK_LOCK;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].action == client_click)
			for (j = 0; j < LENGTH(modifiers); ++j)
				xcb_grab_button(conn, 0, win->id, BUTTONMASK,
				    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC,
				    XCB_WINDOW_NONE, XCB_CURSOR_NONE,
				    buttons[i].button, buttons[i].mask |
				    modifiers[j]);
}

const char *quirkname[] = {
	"NONE",		/* config string for "no value" */
	"FLOAT",
	"TRANSSZ",
	"ANYWHERE",
	"XTERM_FONTADJ",
	"FULLSCREEN",
	"FOCUSPREV",
	"NOFOCUSONMAP",
	"FOCUSONMAP_SINGLE",
	"OBEYAPPFOCUSREQ",
	"IGNOREPID",
	"IGNORESPAWNWS",
};

/* SWM_Q_DELIM: retain '|' for back compat for now (2009-08-11) */
#define SWM_Q_DELIM		"\n|+ \t"
int
parsequirks(const char *qstr, uint32_t *quirk, int *ws)
{
	char			*str, *cp, *name;
	int			i;

	if (quirk == NULL || qstr == NULL)
		return (1);

	if ((str = strdup(qstr)) == NULL)
		err(1, "parsequirks: strdup");

	cp = str;
	*quirk = 0;
	while ((name = strsep(&cp, SWM_Q_DELIM)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, SWM_Q_DELIM);

		if (sscanf(name, "WS[%d]", ws) == 1) {
			if (*ws > 0)
				*ws -= 1;
			continue;
		}

		for (i = 0; i < LENGTH(quirkname); i++) {
			if (strncasecmp(name, quirkname[i],
			    SWM_QUIRK_LEN) == 0) {
				DNPRINTF(SWM_D_QUIRK,
				    "parsequirks: %s\n", name);
				if (i == 0) {
					*quirk = 0;
					free(str);
					return (0);
				}
				*quirk |= 1 << (i-1);
				break;
			}
		}
		if (i >= LENGTH(quirkname)) {
			DNPRINTF(SWM_D_QUIRK,
			    "parsequirks: invalid quirk [%s]\n", name);
			free(str);
			return (1);
		}
	}

	free(str);
	return (0);
}

void
quirk_insert(const char *class, const char *instance, const char *name,
    uint32_t quirk, int ws)
{
	struct quirk		*qp;
	char			*str;
	bool			failed = false;

	DNPRINTF(SWM_D_QUIRK, "quirk_insert: class: %s, instance: %s, name: %s,"
	    " value: %u, ws: %d\n", class, instance, name, quirk, ws);

	if ((qp = malloc(sizeof *qp)) == NULL)
		err(1, "quirk_insert: malloc");

	if ((qp->class = strdup(class)) == NULL)
		err(1, "quirk_insert: strdup");
	if ((qp->instance = strdup(instance)) == NULL)
		err(1, "quirk_insert: strdup");
	if ((qp->name = strdup(name)) == NULL)
		err(1, "quirk_insert: strdup");

	if (asprintf(&str, "^%s$", class) == -1)
		err(1, "quirk_insert: asprintf");
	if (regcomp(&qp->regex_class, str, REG_EXTENDED | REG_NOSUB)) {
		add_startup_exception("regex failed to compile quirk 'class' "
		    "field: %s", class);
		failed = true;
	}
	DNPRINTF(SWM_D_QUIRK, "quirk_insert: compiled: %s\n", str);
	free(str);

	if (asprintf(&str, "^%s$", instance) == -1)
		err(1, "quirk_insert: asprintf");
	if (regcomp(&qp->regex_instance, str, REG_EXTENDED | REG_NOSUB)) {
		add_startup_exception("regex failed to compile quirk 'instance'"
		    " field: %s", instance);
		failed = true;
	}
	DNPRINTF(SWM_D_QUIRK, "quirk_insert: compiled: %s\n", str);
	free(str);

	if (asprintf(&str, "^%s$", name) == -1)
		err(1, "quirk_insert: asprintf");
	if (regcomp(&qp->regex_name, str, REG_EXTENDED | REG_NOSUB)) {
		add_startup_exception("regex failed to compile quirk 'name' "
		    "field: %s", name);
		failed = true;
	}
	DNPRINTF(SWM_D_QUIRK, "quirk_insert: compiled: %s\n", str);
	free(str);

	if (failed) {
		DNPRINTF(SWM_D_QUIRK, "quirk_insert: regex error; skipping.\n");
		quirk_free(qp);
	} else {
		qp->quirk = quirk;
		qp->ws = ws;
		TAILQ_INSERT_TAIL(&quirks, qp, entry);
	}
	DNPRINTF(SWM_D_QUIRK, "quirk_insert: leave\n");
}

void
quirk_remove(struct quirk *qp)
{
	DNPRINTF(SWM_D_QUIRK, "quirk_remove: %s:%s [%u]\n", qp->class,
	    qp->name, qp->quirk);

	TAILQ_REMOVE(&quirks, qp, entry);
	quirk_free(qp);

	DNPRINTF(SWM_D_QUIRK, "quirk_remove: leave\n");
}

void
quirk_free(struct quirk *qp)
{
	regfree(&qp->regex_class);
	regfree(&qp->regex_instance);
	regfree(&qp->regex_name);
	free(qp->class);
	free(qp->instance);
	free(qp->name);
	free(qp);
}

void
quirk_replace(struct quirk *qp, const char *class, const char *instance,
    const char *name, uint32_t quirk, int ws)
{
	DNPRINTF(SWM_D_QUIRK, "quirk_replace: %s:%s:%s [%u], ws: %d\n", qp->class,
	    qp->instance, qp->name, qp->quirk, qp->ws);

	quirk_remove(qp);
	quirk_insert(class, instance, name, quirk, ws);

	DNPRINTF(SWM_D_QUIRK, "quirk_replace: leave\n");
}

void
setquirk(const char *class, const char *instance, const char *name,
    uint32_t quirk, int ws)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "setquirk: enter %s:%s:%s [%u], ws: %d\n", class,
	    instance, name, quirk, ws);

	/* Remove/replace existing quirk. */
	TAILQ_FOREACH(qp, &quirks, entry) {
		if (strcmp(qp->class, class) == 0 &&
		    strcmp(qp->instance, instance) == 0 &&
		    strcmp(qp->name, name) == 0) {
			if (quirk == 0 && ws == -1)
				quirk_remove(qp);
			else
				quirk_replace(qp, class, instance, name, quirk,
				    ws);
			DNPRINTF(SWM_D_QUIRK, "setquirk: leave\n");
			return;
		}
	}

	/* Only insert if quirk is not NONE or forced ws is set. */
	if (quirk || ws != -1)
		quirk_insert(class, instance, name, quirk, ws);

	DNPRINTF(SWM_D_QUIRK, "setquirk: leave\n");
}

/* Eat '\' in str used to escape square brackets and colon. */
void
unescape_selector(char *str)
{
	char *cp;

	for (cp = str; *str != '\0'; ++str, ++cp) {
		if (*str == '\\' && (*(str + 1) == ':' || *(str + 1) == ']' ||
		    *(str + 1) == '['))
			++str;

		*cp = *str;
	}
	*cp = '\0';
}

int
setconfquirk(const char *selector, const char *value, int flags)
{
	char			*str, *cp, *class;
	char			*instance = NULL, *name = NULL;
	int			retval, count = 0, ws = -1;
	uint32_t		qrks;

	/* suppress unused warning since var is needed */
	(void)flags;

	if (selector == NULL || strlen(selector) == 0)
		return (0);

	if ((str = strdup(selector)) == NULL)
		err(1, "setconfquirk: strdup");

	/* Find non-escaped colon. */
	class = cp = str;
	if (*cp == ':') {
		*cp = '\0';
		++count;
	}

	for (++cp; *cp != '\0'; ++cp) {
		if (*cp == ':' && *(cp - 1) != '\\') {
			*cp = '\0';
			++count;
		}
	}

	unescape_selector(class);
	if (count) {
		instance = class + strlen(class) + 1;
		unescape_selector(instance);
	} else {
		instance = ".*";
	}

	if (count > 1) {
		name = instance + strlen(instance) + 1;
		unescape_selector(name);
	} else {
		name = ".*";
	}

	DNPRINTF(SWM_D_CONF, "setconfquirk: class: %s, instance: %s, "
	    "name: %s\n", class, instance, name);

	if ((retval = parsequirks(value, &qrks, &ws)) == 0)
		setquirk(class, instance, name, qrks, ws);

	free(str);
	return (retval);
}

void
setup_quirks(void)
{
	setquirk("MPlayer",		"xv",		".*",
	    SWM_Q_FLOAT | SWM_Q_FULLSCREEN | SWM_Q_FOCUSPREV, -1);
	setquirk("OpenOffice.org 3.2",	"VCLSalFrame",	".*",
	    SWM_Q_FLOAT, -1);
	setquirk("Firefox-bin",		"firefox-bin",	".*",
	    SWM_Q_TRANSSZ, -1);
	setquirk("Firefox",		"Dialog",	".*",
	    SWM_Q_FLOAT, -1);
	setquirk("Gimp",		"gimp",		".*",
	    SWM_Q_FLOAT | SWM_Q_ANYWHERE, -1);
	setquirk("XTerm",		"xterm",	".*",
	    SWM_Q_XTERM_FONTADJ, -1);
	setquirk("xine",		"Xine Window",	".*",
	    SWM_Q_FLOAT | SWM_Q_ANYWHERE, -1);
	setquirk("Xitk",		"Xitk Combo",	".*",
	    SWM_Q_FLOAT | SWM_Q_ANYWHERE, -1);
	setquirk("xine",		"xine Panel",	".*",
	    SWM_Q_FLOAT | SWM_Q_ANYWHERE, -1);
	setquirk("Xitk",		"Xine Window",	".*",
	    SWM_Q_FLOAT | SWM_Q_ANYWHERE, -1);
	setquirk("xine",		"xine Video Fullscreen Window",	".*",
	    SWM_Q_FULLSCREEN | SWM_Q_FLOAT, -1);
	setquirk("pcb",			"pcb",		".*",
	    SWM_Q_FLOAT, -1);
	setquirk("SDL_App",		"SDL_App",	".*",
	    SWM_Q_FLOAT | SWM_Q_FULLSCREEN, -1);
}

/* conf file stuff */
#define SWM_CONF_FILE		"spectrwm.conf"
#define SWM_CONF_FILE_OLD	"scrotwm.conf"

enum {
	SWM_S_BAR_ACTION,
	SWM_S_BAR_AT_BOTTOM,
	SWM_S_BAR_BORDER_WIDTH,
	SWM_S_BAR_DELAY,
	SWM_S_BAR_ENABLED,
	SWM_S_BAR_ENABLED_WS,
	SWM_S_BAR_FONT,
	SWM_S_BAR_FORMAT,
	SWM_S_BAR_JUSTIFY,
	SWM_S_BORDER_WIDTH,
	SWM_S_BOUNDARY_WIDTH,
	SWM_S_CLOCK_ENABLED,
	SWM_S_CLOCK_FORMAT,
	SWM_S_CYCLE_EMPTY,
	SWM_S_CYCLE_VISIBLE,
	SWM_S_DIALOG_RATIO,
	SWM_S_DISABLE_BORDER,
	SWM_S_FOCUS_CLOSE,
	SWM_S_FOCUS_CLOSE_WRAP,
	SWM_S_FOCUS_DEFAULT,
	SWM_S_FOCUS_MODE,
	SWM_S_ICONIC_ENABLED,
	SWM_S_JAVA_WORKAROUND,
	SWM_S_REGION_PADDING,
	SWM_S_SPAWN_ORDER,
	SWM_S_SPAWN_TERM,
	SWM_S_SS_APP,
	SWM_S_SS_ENABLED,
	SWM_S_STACK_ENABLED,
	SWM_S_TERM_WIDTH,
	SWM_S_TILE_GAP,
	SWM_S_URGENT_COLLAPSE,
	SWM_S_URGENT_ENABLED,
	SWM_S_VERBOSE_LAYOUT,
	SWM_S_WARP_POINTER,
	SWM_S_WINDOW_CLASS_ENABLED,
	SWM_S_WINDOW_INSTANCE_ENABLED,
	SWM_S_WINDOW_NAME_ENABLED,
	SWM_S_WORKSPACE_LIMIT,
	SWM_S_WORKSPACE_NAME,
};

int
setconfvalue(const char *selector, const char *value, int flags)
{
	struct workspace	*ws;
	int			i, ws_id, num_screens;
	char			*b, *str, s[1024];

	switch (flags) {
	case SWM_S_BAR_ACTION:
		free(bar_argv[0]);
		if ((bar_argv[0] = expand_tilde(value)) == NULL)
			err(1, "setconfvalue: bar_action");
		break;
	case SWM_S_BAR_AT_BOTTOM:
		bar_at_bottom = (atoi(value) != 0);
		break;
	case SWM_S_BAR_BORDER_WIDTH:
		bar_border_width = atoi(value);
		if (bar_border_width < 0)
			bar_border_width = 0;
		break;
	case SWM_S_BAR_DELAY:
		/* No longer needed; leave to not break old conf files. */
		break;
	case SWM_S_BAR_ENABLED:
		bar_enabled = (atoi(value) != 0);
		break;
	case SWM_S_BAR_ENABLED_WS:
		ws_id = atoi(selector) - 1;
		if (ws_id < 0 || ws_id >= workspace_limit)
			errx(1, "setconfvalue: bar_enabled_ws: invalid "
			    "workspace %d.", ws_id + 1);

		num_screens = get_screen_count();
		for (i = 0; i < num_screens; i++) {
			ws = (struct workspace *)&screens[i].ws;
			ws[ws_id].bar_enabled = (atoi(value) != 0);
		}
		break;
	case SWM_S_BAR_FONT:
		b = bar_fonts;
		if (asprintf(&bar_fonts, "%s,%s", value, bar_fonts) == -1)
			err(1, "setconfvalue: asprintf: failed to allocate "
				"memory for bar_fonts.");
		free(b);

		/* If already in xft mode, then we are done. */
		if (!bar_font_legacy)
			break;

		if ((str = strdup(value)) == NULL)
			err(1, "setconfvalue: strdup");

		/* If there are any non-XLFD entries, switch to Xft mode. */
		while ((b = strsep(&str, ",")) != NULL) {
			if (*b == '\0')
				continue;
			if (!isxlfd(b)) {
				bar_font_legacy = false;
				break;
			}
		}

		free(str);
		break;
	case SWM_S_BAR_FORMAT:
		free(bar_format);
		if ((bar_format = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_format");
		break;
	case SWM_S_BAR_JUSTIFY:
		if (strcmp(value, "left") == 0)
			bar_justify = SWM_BAR_JUSTIFY_LEFT;
		else if (strcmp(value, "center") == 0)
			bar_justify = SWM_BAR_JUSTIFY_CENTER;
		else if (strcmp(value, "right") == 0)
			bar_justify = SWM_BAR_JUSTIFY_RIGHT;
		else
			errx(1, "invalid bar_justify");
		break;
	case SWM_S_BORDER_WIDTH:
		border_width = atoi(value);
		if (border_width < 0)
			border_width = 0;
		break;
	case SWM_S_BOUNDARY_WIDTH:
		boundary_width = atoi(value);
		if (boundary_width < 0)
			boundary_width = 0;
		break;
	case SWM_S_CLOCK_ENABLED:
		clock_enabled = (atoi(value) != 0);
		break;
	case SWM_S_CLOCK_FORMAT:
#ifndef SWM_DENY_CLOCK_FORMAT
		free(clock_format);
		if ((clock_format = strdup(value)) == NULL)
			err(1, "setconfvalue: clock_format");
#endif
		break;
	case SWM_S_CYCLE_EMPTY:
		cycle_empty = (atoi(value) != 0);
		break;
	case SWM_S_CYCLE_VISIBLE:
		cycle_visible = (atoi(value) != 0);
		break;
	case SWM_S_DIALOG_RATIO:
		dialog_ratio = atof(value);
		if (dialog_ratio > 1.0 || dialog_ratio <= .3)
			dialog_ratio = .6;
		break;
	case SWM_S_DISABLE_BORDER:
		disable_border = (atoi(value) != 0);
		break;
	case SWM_S_FOCUS_CLOSE:
		if (strcmp(value, "first") == 0)
			focus_close = SWM_STACK_BOTTOM;
		else if (strcmp(value, "last") == 0)
			focus_close = SWM_STACK_TOP;
		else if (strcmp(value, "next") == 0)
			focus_close = SWM_STACK_ABOVE;
		else if (strcmp(value, "previous") == 0)
			focus_close = SWM_STACK_BELOW;
		else
			errx(1, "focus_close");
		break;
	case SWM_S_FOCUS_CLOSE_WRAP:
		focus_close_wrap = (atoi(value) != 0);
		break;
	case SWM_S_FOCUS_DEFAULT:
		if (strcmp(value, "last") == 0)
			focus_default = SWM_STACK_TOP;
		else if (strcmp(value, "first") == 0)
			focus_default = SWM_STACK_BOTTOM;
		else
			errx(1, "focus_default");
		break;
	case SWM_S_FOCUS_MODE:
		if (strcmp(value, "default") == 0)
			focus_mode = SWM_FOCUS_DEFAULT;
		else if (strcmp(value, "follow") == 0 ||
		    strcmp(value, "follow_cursor") == 0)
			focus_mode = SWM_FOCUS_FOLLOW;
		else if (strcmp(value, "manual") == 0)
			focus_mode = SWM_FOCUS_MANUAL;
		else
			errx(1, "focus_mode");
		break;
	case SWM_S_ICONIC_ENABLED:
		iconic_enabled = (atoi(value) != 0);
		break;
	case SWM_S_JAVA_WORKAROUND:
		java_workaround = (atoi(value) != 0);
		break;
	case SWM_S_REGION_PADDING:
		region_padding = atoi(value);
		if (region_padding < 0)
			region_padding = 0;
		break;
	case SWM_S_SPAWN_ORDER:
		if (strcmp(value, "first") == 0)
			spawn_position = SWM_STACK_BOTTOM;
		else if (strcmp(value, "last") == 0)
			spawn_position = SWM_STACK_TOP;
		else if (strcmp(value, "next") == 0)
			spawn_position = SWM_STACK_ABOVE;
		else if (strcmp(value, "previous") == 0)
			spawn_position = SWM_STACK_BELOW;
		else
			errx(1, "spawn_position");
		break;
	case SWM_S_SPAWN_TERM:
		setconfspawn("term", value, 0);
		setconfspawn("spawn_term", value, 0);
		break;
	case SWM_S_SS_APP:
		/* No longer needed; leave to not break old conf files. */
		break;
	case SWM_S_SS_ENABLED:
		/* No longer needed; leave to not break old conf files. */
		break;
	case SWM_S_STACK_ENABLED:
		stack_enabled = (atoi(value) != 0);
		break;
	case SWM_S_TERM_WIDTH:
		term_width = atoi(value);
		if (term_width < 0)
			term_width = 0;
		break;
	case SWM_S_TILE_GAP:
		tile_gap = atoi(value);
		break;
	case SWM_S_URGENT_COLLAPSE:
		urgent_collapse = (atoi(value) != 0);
		break;
	case SWM_S_URGENT_ENABLED:
		urgent_enabled = (atoi(value) != 0);
		break;
	case SWM_S_VERBOSE_LAYOUT:
		verbose_layout = (atoi(value) != 0);
		for (i = 0; layouts[i].l_stack != NULL; i++) {
			if (verbose_layout)
				layouts[i].l_string = fancy_stacker;
			else
				layouts[i].l_string = plain_stacker;
		}
		break;
	case SWM_S_WARP_POINTER:
		warp_pointer = (atoi(value) != 0);
		break;
	case SWM_S_WINDOW_CLASS_ENABLED:
		window_class_enabled = (atoi(value) != 0);
		break;
	case SWM_S_WINDOW_INSTANCE_ENABLED:
		window_instance_enabled = (atoi(value) != 0);
		break;
	case SWM_S_WINDOW_NAME_ENABLED:
		window_name_enabled = (atoi(value) != 0);
		break;
	case SWM_S_WORKSPACE_LIMIT:
		workspace_limit = atoi(value);
		if (workspace_limit > SWM_WS_MAX)
			workspace_limit = SWM_WS_MAX;
		else if (workspace_limit < 1)
			workspace_limit = 1;

		ewmh_update_desktops();
		break;
	case SWM_S_WORKSPACE_NAME:
		if (getenv("SWM_STARTED") != NULL)
			return (0);

		bzero(s, sizeof s);
		if (sscanf(value, "ws[%d]:%1023c", &ws_id, s) != 2)
			errx(1, "invalid entry, should be 'ws[<idx>]:name'");
		ws_id--;
		if (ws_id < 0 || ws_id >= workspace_limit)
			errx(1, "setconfvalue: workspace_name: invalid "
			    "workspace %d.", ws_id + 1);

		num_screens = get_screen_count();
		for (i = 0; i < num_screens; ++i) {
			ws = (struct workspace *)&screens[i].ws;

			if (strlen(s) > 0) {
				free(ws[ws_id].name);
				if ((ws[ws_id].name = strdup(s)) == NULL)
					err(1, "setconfvalue: workspace_name.");

				ewmh_update_desktop_names();
				ewmh_get_desktop_names();
			}
		}
		break;
	default:
		return (1);
	}
	return (0);
}

int
setconfmodkey(const char *selector, const char *value, int flags)
{
	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (strncasecmp(value, "Mod1", strlen("Mod1")) == 0)
		update_modkey(XCB_MOD_MASK_1);
	else if (strncasecmp(value, "Mod2", strlen("Mod2")) == 0)
		update_modkey(XCB_MOD_MASK_2);
	else if (strncasecmp(value, "Mod3", strlen("Mod3")) == 0)
		update_modkey(XCB_MOD_MASK_3);
	else if (strncasecmp(value, "Mod4", strlen("Mod4")) == 0)
		update_modkey(XCB_MOD_MASK_4);
	else
		return (1);
	return (0);
}

int
setconfcolor(const char *selector, const char *value, int flags)
{
	int	first, last, i = 0, num_screens;

	num_screens = get_screen_count();

	/* conf screen indices begin at 1; treat vals <= 0 as 'all screens.' */
	if (selector == NULL || strlen(selector) == 0 ||
	    (last = atoi(selector) - 1) < 0) {
		first = 0;
		last = num_screens - 1;
	} else {
		first = last;
	}

	if (last >= num_screens) {
		add_startup_exception("invalid screen index: %d out of bounds "
		    "(maximum %d)", last + 1, num_screens);
		return (1);
	}

	for (i = first; i <= last; ++i) {
		setscreencolor(value, i, flags);

		/*
		 * When setting focus/unfocus colors, we need to also
		 * set maximize colors to match if they haven't been customized.
		 */
		if (flags == SWM_S_COLOR_FOCUS &&
		    !screens[i].c[SWM_S_COLOR_FOCUS_MAXIMIZED].manual)
			setscreencolor(value, i, SWM_S_COLOR_FOCUS_MAXIMIZED);
		else if (flags == SWM_S_COLOR_UNFOCUS &&
		    !screens[i].c[SWM_S_COLOR_UNFOCUS_MAXIMIZED].manual)
			setscreencolor(value, i, SWM_S_COLOR_UNFOCUS_MAXIMIZED);

		screens[i].c[flags].manual = 1;
	}

	return (0);
}

int
setconfregion(const char *selector, const char *value, int flags)
{
	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	custom_region(value);
	return (0);
}

int
setautorun(const char *selector, const char *value, int flags)
{
	int			ws_id;
	char			s[1024];
	char			*ap, *sp;
	union arg		a;
	int			argc = 0;
	pid_t			pid;
	struct pid_e		*p;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (getenv("SWM_STARTED"))
		return (0);

	bzero(s, sizeof s);
	if (sscanf(value, "ws[%d]:%1023c", &ws_id, s) != 2)
		errx(1, "invalid autorun entry, should be 'ws[<idx>]:command'");
	ws_id--;
	if (ws_id < 0 || ws_id >= workspace_limit)
		errx(1, "autorun: invalid workspace %d", ws_id + 1);

	sp = expand_tilde((char *)&s);

	/*
	 * This is a little intricate
	 *
	 * If the pid already exists we simply reuse it because it means it was
	 * used before AND not claimed by manage_window.  We get away with
	 * altering it in the parent after INSERT because this can not be a race
	 */
	a.argv = NULL;
	while ((ap = strsep(&sp, " \t")) != NULL) {
		if (*ap == '\0')
			continue;
		DNPRINTF(SWM_D_SPAWN, "setautorun: arg [%s]\n", ap);
		argc++;
		if ((a.argv = realloc(a.argv, argc * sizeof(char *))) == NULL)
			err(1, "setautorun: realloc");
		a.argv[argc - 1] = ap;
	}
	free(sp);

	if ((a.argv = realloc(a.argv, (argc + 1) * sizeof(char *))) == NULL)
		err(1, "setautorun: realloc");
	a.argv[argc] = NULL;

	if ((pid = fork()) == 0) {
		spawn(ws_id, &a, true);
		/* NOTREACHED */
		_exit(1);
	}
	free(a.argv);

	/* parent */
	p = find_pid(pid);
	if (p == NULL) {
		p = calloc(1, sizeof *p);
		if (p == NULL)
			return (1);
		TAILQ_INSERT_TAIL(&pidlist, p, entry);
	}

	p->pid = pid;
	p->ws = ws_id;

	return (0);
}

int
setlayout(const char *selector, const char *value, int flags)
{
	struct workspace	*ws;
	int			ws_id, i, x, mg, ma, si, ar;
	int			st = SWM_V_STACK, num_screens;
	char			s[1024];
	bool			f = false;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (getenv("SWM_STARTED"))
		return (0);

	bzero(s, sizeof s);
	if (sscanf(value, "ws[%d]:%d:%d:%d:%d:%1023c",
	    &ws_id, &mg, &ma, &si, &ar, s) != 6)
		errx(1, "invalid layout entry, should be 'ws[<idx>]:"
		    "<master_grow>:<master_add>:<stack_inc>:<always_raise>:"
		    "<type>'");
	ws_id--;
	if (ws_id < 0 || ws_id >= workspace_limit)
		errx(1, "layout: invalid workspace %d", ws_id + 1);

	if (strcasecmp(s, "vertical") == 0)
		st = SWM_V_STACK;
	else if (strcasecmp(s, "vertical_flip") == 0) {
		st = SWM_V_STACK;
		f = true;
	} else if (strcasecmp(s, "horizontal") == 0)
		st = SWM_H_STACK;
	else if (strcasecmp(s, "horizontal_flip") == 0) {
		st = SWM_H_STACK;
		f = true;
	} else if (strcasecmp(s, "fullscreen") == 0)
		st = SWM_MAX_STACK;
	else
		errx(1, "invalid layout entry, should be 'ws[<idx>]:"
		    "<master_grow>:<master_add>:<stack_inc>:<always_raise>:"
		    "<type>'");

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		ws = (struct workspace *)&screens[i].ws;
		ws[ws_id].cur_layout = &layouts[st];

		ws[ws_id].always_raise = (ar != 0);
		if (st == SWM_MAX_STACK)
			continue;

		/* master grow */
		for (x = 0; x < abs(mg); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    mg >= 0 ?  SWM_ARG_ID_MASTERGROW :
			    SWM_ARG_ID_MASTERSHRINK);
		}
		/* master add */
		for (x = 0; x < abs(ma); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    ma >= 0 ?  SWM_ARG_ID_MASTERADD :
			    SWM_ARG_ID_MASTERDEL);
		}
		/* stack inc */
		for (x = 0; x < abs(si); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    si >= 0 ?  SWM_ARG_ID_STACKINC :
			    SWM_ARG_ID_STACKDEC);
		}
		/* Apply flip */
		if (f) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    SWM_ARG_ID_FLIPLAYOUT);
		}
	}

	return (0);
}

/* config options */
struct config_option {
	char			*optname;
	int			(*func)(const char*, const char*, int);
	int			funcflags;
};
struct config_option configopt[] = {
	{ "autorun",			setautorun,	0 },
	{ "bar_action",			setconfvalue,	SWM_S_BAR_ACTION },
	{ "bar_at_bottom",		setconfvalue,	SWM_S_BAR_AT_BOTTOM },
	{ "bar_border",			setconfcolor,	SWM_S_COLOR_BAR_BORDER },
	{ "bar_border_unfocus",		setconfcolor,	SWM_S_COLOR_BAR_BORDER_UNFOCUS },
	{ "bar_border_width",		setconfvalue,	SWM_S_BAR_BORDER_WIDTH },
	{ "bar_color",			setconfcolor,	SWM_S_COLOR_BAR },
	{ "bar_delay",			setconfvalue,	SWM_S_BAR_DELAY },
	{ "bar_enabled",		setconfvalue,	SWM_S_BAR_ENABLED },
	{ "bar_enabled_ws",		setconfvalue,	SWM_S_BAR_ENABLED_WS },
	{ "bar_font",			setconfvalue,	SWM_S_BAR_FONT },
	{ "bar_font_color",		setconfcolor,	SWM_S_COLOR_BAR_FONT },
	{ "bar_format",			setconfvalue,	SWM_S_BAR_FORMAT },
	{ "bar_justify",		setconfvalue,	SWM_S_BAR_JUSTIFY },
	{ "bind",			setconfbinding,	0 },
	{ "border_width",		setconfvalue,	SWM_S_BORDER_WIDTH },
	{ "boundary_width",		setconfvalue,	SWM_S_BOUNDARY_WIDTH },
	{ "clock_enabled",		setconfvalue,	SWM_S_CLOCK_ENABLED },
	{ "clock_format",		setconfvalue,	SWM_S_CLOCK_FORMAT },
	{ "color_focus",		setconfcolor,	SWM_S_COLOR_FOCUS },
	{ "color_focus_maximized",	setconfcolor,	SWM_S_COLOR_FOCUS_MAXIMIZED },
	{ "color_unfocus",		setconfcolor,	SWM_S_COLOR_UNFOCUS },
	{ "color_unfocus_maximized",	setconfcolor,	SWM_S_COLOR_UNFOCUS_MAXIMIZED },
	{ "cycle_empty",		setconfvalue,	SWM_S_CYCLE_EMPTY },
	{ "cycle_visible",		setconfvalue,	SWM_S_CYCLE_VISIBLE },
	{ "dialog_ratio",		setconfvalue,	SWM_S_DIALOG_RATIO },
	{ "disable_border",		setconfvalue,	SWM_S_DISABLE_BORDER },
	{ "focus_close",		setconfvalue,	SWM_S_FOCUS_CLOSE },
	{ "focus_close_wrap",		setconfvalue,	SWM_S_FOCUS_CLOSE_WRAP },
	{ "focus_default",		setconfvalue,	SWM_S_FOCUS_DEFAULT },
	{ "focus_mode",			setconfvalue,	SWM_S_FOCUS_MODE },
	{ "iconic_enabled",		setconfvalue,	SWM_S_ICONIC_ENABLED },
	{ "java_workaround",		setconfvalue,	SWM_S_JAVA_WORKAROUND },
	{ "keyboard_mapping",		setkeymapping,	0 },
	{ "layout",			setlayout,	0 },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "region_padding",		setconfvalue,	SWM_S_REGION_PADDING },
	{ "screenshot_app",		setconfvalue,	SWM_S_SS_APP },
	{ "screenshot_enabled",		setconfvalue,	SWM_S_SS_ENABLED },
	{ "spawn_position",		setconfvalue,	SWM_S_SPAWN_ORDER },
	{ "spawn_term",			setconfvalue,	SWM_S_SPAWN_TERM },
	{ "stack_enabled",		setconfvalue,	SWM_S_STACK_ENABLED },
	{ "term_width",			setconfvalue,	SWM_S_TERM_WIDTH },
	{ "tile_gap",			setconfvalue,	SWM_S_TILE_GAP },
	{ "title_class_enabled",	setconfvalue,	SWM_S_WINDOW_CLASS_ENABLED }, /* For backwards compat. */
	{ "title_name_enabled",		setconfvalue,	SWM_S_WINDOW_INSTANCE_ENABLED }, /* For backwards compat. */
	{ "urgent_collapse",		setconfvalue,	SWM_S_URGENT_COLLAPSE },
	{ "urgent_enabled",		setconfvalue,	SWM_S_URGENT_ENABLED },
	{ "verbose_layout",		setconfvalue,	SWM_S_VERBOSE_LAYOUT },
	{ "warp_pointer",		setconfvalue,	SWM_S_WARP_POINTER },
	{ "window_class_enabled",	setconfvalue,	SWM_S_WINDOW_CLASS_ENABLED },
	{ "window_instance_enabled",	setconfvalue,	SWM_S_WINDOW_INSTANCE_ENABLED },
	{ "window_name_enabled",	setconfvalue,	SWM_S_WINDOW_NAME_ENABLED },
	{ "workspace_limit",		setconfvalue,	SWM_S_WORKSPACE_LIMIT },
	{ "name",			setconfvalue,	SWM_S_WORKSPACE_NAME },
};

void
_add_startup_exception(const char *fmt, va_list ap)
{
	if (vasprintf(&startup_exception, fmt, ap) == -1)
		warn("%s: asprintf", __func__);
}

void
add_startup_exception(const char *fmt, ...)
{
	va_list ap;

	nr_exceptions++;

	if (startup_exception)
		return;

	/* force bar to be enabled due to exception */
	bar_enabled = true;

	va_start(ap, fmt);
	_add_startup_exception(fmt, ap);
	va_end(ap);
}

int
conf_load(const char *filename, int keymapping)
{
	FILE			*config;
	char			*line = NULL, *cp, *ce, *optsub, *optval = NULL;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optidx, count;
	struct config_option	*opt = NULL;

	DNPRINTF(SWM_D_CONF, "conf_load: begin\n");

	if (filename == NULL) {
		warnx("conf_load: no filename");
		return (1);
	}

	DNPRINTF(SWM_D_CONF, "conf_load: open %s\n", filename);

	if ((config = fopen(filename, "r")) == NULL) {
		warn("conf_load: fopen: %s", filename);
		return (1);
	}

	while (!feof(config)) {
		if (line)
			free(line);

		if ((line = fparseln(config, &linelen, &lineno, NULL,
		    FPARSELN_UNESCCOMM | FPARSELN_UNESCCONT)) == NULL) {
			if (ferror(config))
				err(1, "%s", filename);
			else
				continue;
		}
		cp = line;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		if (cp[0] == '\0') {
			/* empty line */
			continue;
		}
		/* get config option */
		wordlen = strcspn(cp, "=[ \t\n");
		if (wordlen == 0) {
			add_startup_exception("%s: line %zd: no option found",
			    filename, lineno);
			continue;
		}
		optidx = -1;
		for (i = 0; i < LENGTH(configopt); i++) {
			opt = &configopt[i];
			if (strncasecmp(cp, opt->optname, wordlen) == 0 &&
			    (int)strlen(opt->optname) == wordlen) {
				optidx = i;
				break;
			}
		}
		if (optidx == -1) {
			add_startup_exception("%s: line %zd: unknown option "
			    "%.*s", filename, lineno, wordlen, cp);
			continue;
		}
		if (keymapping && opt && strcmp(opt->optname, "bind")) {
			add_startup_exception("%s: line %zd: invalid option "
			    "%.*s", filename, lineno, wordlen, cp);
			continue;
		}
		cp += wordlen;
		cp += strspn(cp, " \t\n"); /* eat whitespace */

		/* get [selector] if any */
		optsub = NULL;
		count = 0;
		if (*cp == '[') {
			++count;
			/* Get length of selector. */
			for (ce = ++cp; *ce != '\0'; ++ce) {
				/* Find matching (not escaped) bracket. */
				if (*ce == ']' && *(ce - 1) != '\\') {
					--count;
					break;
				}
			}

			if (count > 0) {
				add_startup_exception("%s: line %zd: syntax "
				    "error: unterminated selector", filename,
				    lineno);
				continue;
			}

			/* ce points at ']'; terminate optsub. */
			*ce = '\0';
			optsub = cp;
			cp = ce + 1;
		}
		cp += strspn(cp, "= \t\n"); /* eat trailing */
		/* get RHS value */
		optval = cp;
		if (strlen(optval) == 0) {
			add_startup_exception("%s: line %zd: must supply value "
			    "to %s", filename, lineno,
			    configopt[optidx].optname);
			continue;
		}
		/* call function to deal with it all */
		if (configopt[optidx].func(optsub, optval,
		    configopt[optidx].funcflags) != 0) {
			add_startup_exception("%s: line %zd: invalid data for "
			    "%s", filename, lineno, configopt[optidx].optname);
			continue;
		}
	}

	if (line)
		free(line);
	fclose(config);
	DNPRINTF(SWM_D_CONF, "conf_load: end\n");

	return (0);
}

void
set_child_transient(struct ws_win *win, xcb_window_t *trans)
{
	struct ws_win		*parent, *w;
	struct swm_region	*r;
	struct workspace	*ws;
	xcb_icccm_wm_hints_t	wmh;

	parent = find_window(win->transient);
	if (parent)
		parent->focus_child = win;
	else {
		DNPRINTF(SWM_D_MISC, "set_child_transient: parent doesn't exist"
		    " for %#x trans %#x\n", win->id, win->transient);

		r = root_to_region(win->s->root, SWM_CK_ALL);
		ws = r->ws;
		/* parent doen't exist in our window list */
		TAILQ_FOREACH(w, &ws->winlist, entry) {
			if (xcb_icccm_get_wm_hints_reply(conn,
			    xcb_icccm_get_wm_hints(conn, w->id),
			    &wmh, NULL) != 1) {
				warnx("can't get hints for %#x", w->id);
				continue;
			}

			if (win->hints.window_group != wmh.window_group)
				continue;

			w->focus_child = win;
			win->transient = w->id;
			*trans = w->id;
			DNPRINTF(SWM_D_MISC, "set_child_transient: adjusting "
			    "transient to %#x\n", win->transient);
			break;
		}
	}
}

pid_t
window_get_pid(xcb_window_t win)
{
	pid_t				ret = 0;
	const char			*errstr;
	xcb_atom_t			apid;
	xcb_get_property_cookie_t	pc;
	xcb_get_property_reply_t	*pr;

	apid = get_atom_from_string("_NET_WM_PID");
	if (apid == XCB_ATOM_NONE)
		goto tryharder;

	pc = xcb_get_property(conn, 0, win, apid, XCB_ATOM_CARDINAL, 0, 1);
	pr = xcb_get_property_reply(conn, pc, NULL);
	if (pr == NULL)
		goto tryharder;
	if (pr->type != XCB_ATOM_CARDINAL) {
		free(pr);
		goto tryharder;
	}

	if (pr->type == apid && pr->format == 32)
		ret = *((pid_t *)xcb_get_property_value(pr));
	free(pr);

	return (ret);

tryharder:
	apid = get_atom_from_string("_SWM_PID");
	pc = xcb_get_property(conn, 0, win, apid, XCB_ATOM_STRING,
	    0, SWM_PROPLEN);
	pr = xcb_get_property_reply(conn, pc, NULL);
	if (pr == NULL)
		return (0);
	if (pr->type != apid) {
		free(pr);
		return (0);
	}

	ret = (pid_t)strtonum(xcb_get_property_value(pr), 0, INT_MAX, &errstr);
	free(pr);

	return (ret);
}

int
get_swm_ws(xcb_window_t id)
{
	int			ws_idx = -1;
	char			*prop = NULL;
	size_t			proplen;
	const char		*errstr;
	xcb_get_property_reply_t	*gpr;

	gpr = xcb_get_property_reply(conn,
		xcb_get_property(conn, 0, id, a_swm_ws,
		    XCB_ATOM_STRING, 0, SWM_PROPLEN),
		NULL);
	if (gpr == NULL)
		return (-1);
	if (gpr->type) {
		proplen = xcb_get_property_value_length(gpr);
		if (proplen > 0) {
			prop = malloc(proplen + 1);
			if (prop) {
				memcpy(prop,
				    xcb_get_property_value(gpr),
				    proplen);
				prop[proplen] = '\0';
			}
		}
	}
	free(gpr);

	if (prop) {
		DNPRINTF(SWM_D_PROP, "get_swm_ws: _SWM_WS: %s\n", prop);
		ws_idx = (int)strtonum(prop, 0, workspace_limit - 1, &errstr);
		if (errstr) {
			DNPRINTF(SWM_D_PROP, "get_swm_ws: win #%s: %s",
			    errstr, prop);
		}
		free(prop);
	}

	return ws_idx;
}

int
get_ws_idx(struct ws_win *win)
{
	xcb_get_property_reply_t	*gpr;
	int			ws_idx = -1;

	if (win == NULL)
		return -1;

	gpr = xcb_get_property_reply(conn,
		xcb_get_property(conn, 0, win->id, ewmh[_NET_WM_DESKTOP].atom,
		    XCB_ATOM_CARDINAL, 0, 1),
		NULL);
	if (gpr) {
		if (gpr->type == XCB_ATOM_CARDINAL && gpr->format == 32)
			ws_idx = *((int *)xcb_get_property_value(gpr));
		free(gpr);
	}

	if (ws_idx == -1 && !(win->quirks & SWM_Q_IGNORESPAWNWS))
		ws_idx = get_swm_ws(win->id);

	if (ws_idx > workspace_limit - 1 || ws_idx < -1)
		ws_idx = -1;

	DNPRINTF(SWM_D_PROP, "get_ws_idx: win %#x, ws_idx: %d\n", win->id,
	    ws_idx);

	return ws_idx;
}

struct ws_win *
manage_window(xcb_window_t id, int spawn_pos, bool mapped)
{
	struct ws_win		*win, *ww;
	struct swm_region	*r;
	struct pid_e		*p;
	struct quirk		*qp;
	xcb_get_geometry_reply_t	*gr;
	xcb_window_t		trans = XCB_WINDOW_NONE;
	uint32_t		i, wa[2], new_flags;
	int			ws_idx, force_ws = -1;
	char			*class, *instance, *name;

	if ((win = find_window(id)) != NULL) {
		DNPRINTF(SWM_D_MISC, "manage_window: win %#x already "
		    "managed; skipping.)\n", id);
		return (win);	/* Already managed. */
	}

	/* See if window is on the unmanaged list. */
	if ((win = find_unmanaged_window(id)) != NULL) {
		DNPRINTF(SWM_D_MISC, "manage_window: win %#x found on "
		    "unmanaged list.\n", id);
		TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);

		if (TRANS(win))
			set_child_transient(win, &trans);

		goto out;
	} else {
		DNPRINTF(SWM_D_MISC, "manage_window: win %#x is new.\n", id);
	}

	/* Try to get initial window geometry. */
	gr = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, id), NULL);
	if (gr == NULL) {
		DNPRINTF(SWM_D_MISC, "manage_window: get geometry failed.\n");
		return (NULL);
	}

	/* Create and initialize ws_win object. */
	if ((win = calloc(1, sizeof(struct ws_win))) == NULL)
		err(1, "manage_window: calloc: failed to allocate memory for "
		    "new window");

	win->id = id;

	/* Figureout which region the window belongs to. */
	r = root_to_region(gr->root, SWM_CK_ALL);

	/* Ignore window border if there is one. */
	WIDTH(win) = gr->width;
	HEIGHT(win) = gr->height;
	X(win) = gr->x + gr->border_width - border_width;
	Y(win) = gr->y + gr->border_width - border_width;
	win->bordered = true;
	win->mapped = mapped;
	win->s = r->s;	/* this never changes */

	free(gr);

	/* Select which X events to monitor and set border pixel color. */
	wa[0] = win->s->c[SWM_S_COLOR_UNFOCUS].pixel;
	wa[1] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_PROPERTY_CHANGE |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY;
#ifdef SWM_DEBUG
	wa[1] |= XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
#endif

	xcb_change_window_attributes(conn, win->id, XCB_CW_BORDER_PIXEL |
	    XCB_CW_EVENT_MASK, wa);

	/* Get WM_SIZE_HINTS. */
	xcb_icccm_get_wm_normal_hints_reply(conn,
	    xcb_icccm_get_wm_normal_hints(conn, win->id),
	    &win->sh, NULL);

	/* Get WM_HINTS. */
	xcb_icccm_get_wm_hints_reply(conn,
	    xcb_icccm_get_wm_hints(conn, win->id),
	    &win->hints, NULL);

	/* Get WM_TRANSIENT_FOR; see if window is a transient. */
	xcb_icccm_get_wm_transient_for_reply(conn,
	    xcb_icccm_get_wm_transient_for(conn, win->id),
	    &trans, NULL);
	if (trans) {
		win->transient = trans;
		set_child_transient(win, &win->transient);
	}

	/* Get WM_PROTOCOLS. */
	get_wm_protocols(win);

	/* Set initial quirks based on EWMH. */
	ewmh_autoquirk(win);

	/* Determine initial quirks. */
	xcb_icccm_get_wm_class_reply(conn,
	    xcb_icccm_get_wm_class(conn, win->id),
	    &win->ch, NULL);

	class = win->ch.class_name ? win->ch.class_name : "";
	instance = win->ch.instance_name ? win->ch.instance_name : "";
	name = get_win_name(win->id);

	DNPRINTF(SWM_D_CLASS, "manage_window: class: %s, instance: %s, "
	    "name: %s\n", class, instance, name);

	/* java is retarded so treat it special */
	if (strstr(instance, "sun-awt")) {
		DNPRINTF(SWM_D_CLASS, "manage_window: java window detected.\n");
		win->java = true;
	}

	TAILQ_FOREACH(qp, &quirks, entry) {
		if (regexec(&qp->regex_class, class, 0, NULL, 0) == 0 &&
		    regexec(&qp->regex_instance, instance, 0, NULL, 0) == 0 &&
		    regexec(&qp->regex_name, name, 0, NULL, 0) == 0) {
			DNPRINTF(SWM_D_CLASS, "manage_window: matched "
			    "quirk: %s:%s:%s mask: %#x, ws: %d\n", qp->class,
			    qp->instance, qp->name, qp->quirk, qp->ws);
			win->quirks = qp->quirk;
			if (qp->ws >= 0 && qp->ws < workspace_limit)
				force_ws = qp->ws;
		}
	}

	free(name);

	/* Reset font sizes (the bruteforce way; no default keybinding). */
	if (win->quirks & SWM_Q_XTERM_FONTADJ) {
		for (i = 0; i < SWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Subtract, XCB_MOD_MASK_SHIFT);
		for (i = 0; i < SWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Add, XCB_MOD_MASK_SHIFT);
	}

	/* Figure out which workspace the window belongs to. */
	if (!(win->quirks & SWM_Q_IGNOREPID) &&
	    (p = find_pid(window_get_pid(win->id))) != NULL) {
		win->ws = &r->s->ws[p->ws];
		TAILQ_REMOVE(&pidlist, p, entry);
		free(p);
		p = NULL;
	} else if ((ws_idx = get_ws_idx(win)) != -1 &&
	    !TRANS(win)) {
		/* _SWM_WS is set; use that. */
		win->ws = &r->s->ws[ws_idx];
	} else if (trans && (ww = find_window(trans)) != NULL) {
		/* Launch transients in the same ws as parent. */
		win->ws = ww->ws;
	} else {
		win->ws = r->ws;
	}

	if (force_ws != -1)
		win->ws = &r->s->ws[force_ws];

	/* Set the _NET_WM_DESKTOP atom. */
	DNPRINTF(SWM_D_PROP, "manage_window: set _NET_WM_DESKTOP: %d\n",
	    win->ws->idx);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
	    ewmh[_NET_WM_DESKTOP].atom, XCB_ATOM_CARDINAL, 32, 1, &win->ws->idx);

	/* Remove any _SWM_WS now that we set _NET_WM_DESKTOP. */
	xcb_delete_property(conn, win->id, a_swm_ws);

	/* WS must already be set for this to work. */
	store_float_geom(win);

	/* Make sure window is positioned inside its region, if its active. */
	if (win->ws->r) {
		region_containment(win, r, SWM_CW_ALLSIDES |
		    SWM_CW_HARDBOUNDARY);
		update_window(win);
	}

out:
	/* Figure out where to insert the window in the workspace list. */
	if (trans && (ww = find_window(trans)))
		TAILQ_INSERT_AFTER(&win->ws->winlist, ww, win, entry);
	else if (win->ws->focus && spawn_pos == SWM_STACK_ABOVE)
		TAILQ_INSERT_AFTER(&win->ws->winlist, win->ws->focus, win,
		    entry);
	else if (win->ws->focus && spawn_pos == SWM_STACK_BELOW)
		TAILQ_INSERT_BEFORE(win->ws->focus, win, entry);
	else switch (spawn_pos) {
	default:
	case SWM_STACK_TOP:
	case SWM_STACK_ABOVE:
		TAILQ_INSERT_TAIL(&win->ws->winlist, win, entry);
		break;
	case SWM_STACK_BOTTOM:
	case SWM_STACK_BELOW:
		TAILQ_INSERT_HEAD(&win->ws->winlist, win, entry);
	}

	ewmh_update_client_list();

	TAILQ_INSERT_HEAD(&win->ws->stack, win, stack_entry);
	lower_window(win);

	/* Get/apply initial _NET_WM_STATE */
	ewmh_get_wm_state(win);

	/* Apply quirks. */
	new_flags = win->ewmh_flags;

	if (win->quirks & SWM_Q_FLOAT)
		new_flags |= EWMH_F_ABOVE;

	if (win->quirks & SWM_Q_ANYWHERE)
		new_flags |= SWM_F_MANUAL;

	ewmh_apply_flags(win, new_flags);
	ewmh_update_wm_state(win);

	/* Set initial _NET_WM_ALLOWED_ACTIONS */
	ewmh_update_actions(win);

	grabbuttons(win);

	DNPRINTF(SWM_D_MISC, "manage_window: done. win %#x, (x,y) w x h: "
	    "(%d,%d) %d x %d, ws: %d, iconic: %s, transient: %#x\n", win->id,
	    X(win), Y(win), WIDTH(win), HEIGHT(win), win->ws->idx,
	    YESNO(ICONIC(win)), win->transient);

	return (win);
}

void
free_window(struct ws_win *win)
{
	DNPRINTF(SWM_D_MISC, "free_window: win %#x\n", win->id);

	if (win == NULL)
		return;

	TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);

	xcb_icccm_get_wm_class_reply_wipe(&win->ch);

	kill_refs(win);

	/* paint memory */
	memset(win, 0xff, sizeof *win);	/* XXX kill later */

	free(win);
	DNPRINTF(SWM_D_MISC, "free_window: done.\n");
}

void
unmanage_window(struct ws_win *win)
{
	struct ws_win		*parent;

	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "unmanage_window: win %#x\n", win->id);

	if (TRANS(win)) {
		parent = find_window(win->transient);
		if (parent)
			parent->focus_child = NULL;
	}

	TAILQ_REMOVE(&win->ws->stack, win, stack_entry);
	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&win->ws->unmanagedlist, win, entry);

	ewmh_update_client_list();
}

void
expose(xcb_expose_event_t *e)
{
	int			i, num_screens;
	struct swm_region	*r;

	DNPRINTF(SWM_D_EVENT, "expose: win %#x\n", e->window);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (e->window == WINID(r->bar))
				bar_draw();

	xcb_flush(conn);
}

#ifdef SWM_DEBUG
void
focusin(xcb_focus_in_event_t *e)
{
	DNPRINTF(SWM_D_EVENT, "focusin: win %#x, mode: %s(%u), "
	    "detail: %s(%u)\n", e->event, get_notify_mode_label(e->mode),
	    e->mode, get_notify_detail_label(e->detail), e->detail);
}

void
focusout(xcb_focus_out_event_t *e)
{
	DNPRINTF(SWM_D_EVENT, "focusout: win %#x, mode: %s(%u), "
	    "detail: %s(%u)\n", e->event, get_notify_mode_label(e->mode),
	    e->mode, get_notify_detail_label(e->detail), e->detail);
}
#endif

void
keypress(xcb_key_press_event_t *e)
{
	xcb_keysym_t		keysym;
	struct key		*kp;

	keysym = xcb_key_press_lookup_keysym(syms, e, 0);

	DNPRINTF(SWM_D_EVENT, "keypress: keysym: %u, win (x,y): %#x (%d,%d), "
	    "detail: %u, time: %u, root (x,y): %#x (%d,%d), child: %#x, "
	    "state: %u, same_screen: %s\n", keysym, e->event, e->event_x,
	    e->event_y, e->detail, e->time, e->root, e->root_x, e->root_y,
	    e->child, e->state, YESNO(e->same_screen));

	if ((kp = key_lookup(CLEANMASK(e->state), keysym)) == NULL)
		goto out;

	last_event_time = e->time;

	if (kp->funcid == KF_SPAWN_CUSTOM)
		spawn_custom(root_to_region(e->root, SWM_CK_ALL),
		    &(keyfuncs[kp->funcid].args), kp->spawn_name);
	else if (keyfuncs[kp->funcid].func)
		keyfuncs[kp->funcid].func(root_to_region(e->root, SWM_CK_ALL),
		    &(keyfuncs[kp->funcid].args));

out:
	/* Unfreeze grab events. */
	xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD, e->time);
	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "keypress: done.\n");
}

void
buttonpress(xcb_button_press_event_t *e)
{
	struct ws_win		*win = NULL;
	struct swm_region	*r, *old_r;
	int			i;
	bool			handled = false;

	DNPRINTF(SWM_D_EVENT, "buttonpress: win (x,y): %#x (%d,%d), "
	    "detail: %u, time: %u, root (x,y): %#x (%d,%d), child: %#x, "
	    "state: %u, same_screen: %s\n", e->event, e->event_x, e->event_y,
	    e->detail, e->time, e->root, e->root_x, e->root_y, e->child,
	    e->state, YESNO(e->same_screen));

	if (e->event == e->root) {
		if (e->child != 0) {
			win = find_window(e->child);
			/* Pass ButtonPress to window if it isn't managed. */
			if (win == NULL)
				goto out;
		} else {
			/* Focus on empty region */
			/* If no windows on region if its empty. */
			r = root_to_region(e->root, SWM_CK_POINTER);
			if (r == NULL) {
				DNPRINTF(SWM_D_EVENT, "buttonpress: "
				    "NULL region; ignoring.\n");
				goto out;
			}

			if (TAILQ_EMPTY(&r->ws->winlist)) {
				old_r = root_to_region(e->root, SWM_CK_FOCUS);
				if (old_r && old_r != r)
					unfocus_win(old_r->ws->focus);

				xcb_set_input_focus(conn,
				    XCB_INPUT_FOCUS_PARENT, e->root, e->time);

				/* Clear bar since empty. */
				bar_draw();

				handled = true;
				goto out;
			}
		}
	} else {
		win = find_window(e->event);
	}

	if (win == NULL)
		goto out;

	last_event_time = e->time;

	focus_win(get_focus_magic(win));

	for (i = 0; i < LENGTH(buttons); i++)
		if (client_click == buttons[i].action && buttons[i].func &&
		    buttons[i].button == e->detail &&
		    CLEANMASK(buttons[i].mask) == CLEANMASK(e->state)) {
			buttons[i].func(win, &buttons[i].args);
			handled = true;
		}

out:
	if (!handled) {
		DNPRINTF(SWM_D_EVENT, "buttonpress: passing to window.\n");
		/* Replay event to event window */
		xcb_allow_events(conn, XCB_ALLOW_REPLAY_POINTER, e->time);
	} else {
		DNPRINTF(SWM_D_EVENT, "buttonpress: handled.\n");
		/* Unfreeze grab events. */
		xcb_allow_events(conn, XCB_ALLOW_SYNC_POINTER, e->time);
	}

	xcb_flush(conn);
}

#ifdef SWM_DEBUG
void
print_win_geom(xcb_window_t w)
{
	xcb_get_geometry_reply_t	*wa;

	wa = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, w), NULL);
	if (wa == NULL) {
		DNPRINTF(SWM_D_MISC, "print_win_geom: window not found: %#x\n",
		    w);
		return;
	}

	DNPRINTF(SWM_D_MISC, "print_win_geom: win %#x, root: %#x, "
	    "depth: %u, (x,y) w x h: (%d,%d) %d x %d, border: %d\n",
	    w, wa->root, wa->depth, wa->x,  wa->y, wa->width, wa->height,
	    wa->border_width);

	free(wa);
}
#endif

#ifdef SWM_DEBUG
char *
get_stack_mode_name(uint8_t mode)
{
	char	*name;

	switch(mode) {
	case XCB_STACK_MODE_ABOVE:
		name = "Above";
		break;
	case XCB_STACK_MODE_BELOW:
		name = "Below";
		break;
	case XCB_STACK_MODE_TOP_IF:
		name = "TopIf";
		break;
	case XCB_STACK_MODE_BOTTOM_IF:
		name = "BottomIf";
		break;
	case XCB_STACK_MODE_OPPOSITE:
		name = "Opposite";
		break;
	default:
		name = "Unknown";
	}

	return name;
}
#endif

void
configurerequest(xcb_configure_request_event_t *e)
{
	struct ws_win		*win;
	struct swm_region	*r = NULL;
	int			i = 0;
	uint32_t		wc[7] = {0};
	uint16_t		mask = 0;
	bool			new = false;

	if ((win = find_window(e->window)) == NULL)
		if ((win = find_unmanaged_window(e->window)) == NULL)
			new = true;

#ifdef SWM_DEBUG
	if (swm_debug & SWM_D_EVENT) {
		print_win_geom(e->window);

		DNPRINTF(SWM_D_EVENT, "configurerequest: win %#x, "
		    "parent: %#x, new: %s, value_mask: %u { ", e->window,
		    e->parent, YESNO(new), e->value_mask);
		if (e->value_mask & XCB_CONFIG_WINDOW_X)
			DPRINTF("X: %d ", e->x);
		if (e->value_mask & XCB_CONFIG_WINDOW_Y)
			DPRINTF("Y: %d ", e->y);
		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
			DPRINTF("W: %u ", e->width);
		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
			DPRINTF("H: %u ", e->height);
		if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
			DPRINTF("Border: %u ", e->border_width);
		if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING)
			DPRINTF("Sibling: %#x ", e->sibling);
		if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
			DPRINTF("StackMode: %s(%u) ",
			    get_stack_mode_name(e->stack_mode), e->stack_mode);
		DPRINTF("}\n");
	}
#endif

	if (new) {
		if (e->value_mask & XCB_CONFIG_WINDOW_X) {
			mask |= XCB_CONFIG_WINDOW_X;
			wc[i++] = e->x;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
			mask |= XCB_CONFIG_WINDOW_Y;
			wc[i++] = e->y;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
			mask |= XCB_CONFIG_WINDOW_WIDTH;
			wc[i++] = e->width;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
			mask |= XCB_CONFIG_WINDOW_HEIGHT;
			wc[i++] = e->height;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
			mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
			wc[i++] = e->border_width;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
			mask |= XCB_CONFIG_WINDOW_SIBLING;
			wc[i++] = e->sibling;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
			mask |= XCB_CONFIG_WINDOW_STACK_MODE;
			wc[i++] = e->stack_mode;
		}

		if (mask != 0) {
			xcb_configure_window(conn, e->window, mask, wc);
			xcb_flush(conn);
		}
	} else if ((!MANUAL(win) || win->quirks & SWM_Q_ANYWHERE) &&
	    !FULLSCREEN(win) && !MAXIMIZED(win)) {
		if (win->ws->r)
			r = win->ws->r;
		else if (win->ws->old_r)
			r = win->ws->old_r;

		/* windows are centered unless ANYWHERE quirk is set. */
		if (win->quirks & SWM_Q_ANYWHERE) {
			if (e->value_mask & XCB_CONFIG_WINDOW_X) {
				win->g_float.x = e->x;
				if (r)
					win->g_float.x -= X(r);
			}

			if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
				win->g_float.y = e->y;
				if (r)
					win->g_float.y -= Y(r);
			}
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
			win->g_float.w = e->width;

		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
			win->g_float.h = e->height;

		win->g_floatvalid = true;

		if (!MAXIMIZED(win) && !FULLSCREEN(win) &&
		    (TRANS(win) || (ABOVE(win) &&
		    win->ws->cur_layout != &layouts[SWM_MAX_STACK]))) {
			WIDTH(win) = win->g_float.w;
			HEIGHT(win) = win->g_float.h;

			if (r != NULL) {
				update_floater(win);
				focus_flush();
			} else {
				config_win(win, e);
				xcb_flush(conn);
			}
		} else {
			config_win(win, e);
			xcb_flush(conn);
		}
	} else {
		config_win(win, e);
		xcb_flush(conn);
	}

	DNPRINTF(SWM_D_EVENT, "configurerequest: done.\n");
}

void
configurenotify(xcb_configure_notify_event_t *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "configurenotify: win %#x, event win: %#x, "
	    "(x,y) WxH: (%d,%d) %ux%u, border: %u, above_sibling: %#x, "
	    "override_redirect: %s\n", e->window, e->event, e->x, e->y,
	    e->width, e->height, e->border_width, e->above_sibling,
	    YESNO(e->override_redirect));

	win = find_window(e->window);
	if (win) {
		xcb_icccm_get_wm_normal_hints_reply(conn,
		    xcb_icccm_get_wm_normal_hints(conn, win->id),
		    &win->sh, NULL);
		adjust_font(win);
		if (font_adjusted) {
			stack();
			xcb_flush(conn);
		}
	}
}

void
destroynotify(xcb_destroy_notify_event_t *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "destroynotify: win %#x\n", e->window);

	if ((win = find_window(e->window)) == NULL) {
		if ((win = find_unmanaged_window(e->window)) == NULL)
			return;
		free_window(win);
		return;
	}

	if (focus_mode != SWM_FOCUS_FOLLOW) {
		/* If we were focused, make sure we focus on something else. */
		if (win == win->ws->focus)
			win->ws->focus_pending = get_focus_prev(win);
	}

	unmanage_window(win);
	stack();

	if (focus_mode != SWM_FOCUS_FOLLOW && WS_FOCUSED(win->ws)) {
		if (win->ws->focus_pending) {
			focus_win(win->ws->focus_pending);
			win->ws->focus_pending = NULL;
		} else if (win == win->ws->focus) {
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT,
			    win->ws->r->id, XCB_CURRENT_TIME);
		}
	}

	free_window(win);

	focus_flush();
}

#ifdef SWM_DEBUG
char *
get_notify_detail_label(uint8_t detail)
{
	char *label;

	switch (detail) {
	case XCB_NOTIFY_DETAIL_ANCESTOR:
		label = "Ancestor";
		break;
	case XCB_NOTIFY_DETAIL_VIRTUAL:
		label = "Virtual";
		break;
	case XCB_NOTIFY_DETAIL_INFERIOR:
		label = "Inferior";
		break;
	case XCB_NOTIFY_DETAIL_NONLINEAR:
		label = "Nonlinear";
		break;
	case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
		label = "NonlinearVirtual";
		break;
	case XCB_NOTIFY_DETAIL_POINTER:
		label = "Pointer";
		break;
	case XCB_NOTIFY_DETAIL_POINTER_ROOT:
		label = "PointerRoot";
		break;
	case XCB_NOTIFY_DETAIL_NONE:
		label = "None";
		break;
	default:
		label = "Unknown";
	}

	return label;
}

char *
get_notify_mode_label(uint8_t mode)
{
	char *label;

	switch (mode) {
	case XCB_NOTIFY_MODE_NORMAL:
		label = "Normal";
		break;
	case XCB_NOTIFY_MODE_GRAB:
		label = "Grab";
		break;
	case XCB_NOTIFY_MODE_UNGRAB:
		label = "Ungrab";
		break;
	case XCB_NOTIFY_MODE_WHILE_GRABBED:
		label = "WhileGrabbed";
		break;
	default:
		label = "Unknown";
	}

	return label;
}
#endif

void
enternotify(xcb_enter_notify_event_t *e)
{
	struct ws_win		*win;
	struct swm_region	*r;

	DNPRINTF(SWM_D_FOCUS, "enternotify: time: %u, win (x,y): %#x "
	    "(%d,%d), mode: %s(%d), detail: %s(%d), root (x,y): %#x (%d,%d), "
	    "child: %#x, same_screen_focus: %s, state: %d\n",
	    e->time, e->event, e->event_x, e->event_y,
	    get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail,
	    e->root, e->root_x, e->root_y, e->child,
	    YESNO(e->same_screen_focus), e->state);

	if (focus_mode == SWM_FOCUS_MANUAL &&
	    e->mode == XCB_NOTIFY_MODE_NORMAL) {
		DNPRINTF(SWM_D_EVENT, "enternotify: manual focus; ignoring.\n");
		return;
	}

	if (focus_mode != SWM_FOCUS_FOLLOW &&
	    e->mode == XCB_NOTIFY_MODE_UNGRAB) {
		DNPRINTF(SWM_D_EVENT, "enternotify: ungrab; ignoring.\n");
		return;
	}

	last_event_time = e->time;

	if ((win = find_window(e->event)) == NULL) {
		if (e->event == e->root) {
			/* If no windows on pointer region, then focus root. */
			r = root_to_region(e->root, SWM_CK_POINTER);
			if (r == NULL) {
				DNPRINTF(SWM_D_EVENT, "enternotify: "
				    "NULL region; ignoring.\n");
				return;
			}

			focus_region(r);
		} else {
			DNPRINTF(SWM_D_EVENT, "enternotify: window is NULL; "
			    "ignoring\n");
			return;
		}
	} else {
		if (e->mode == XCB_NOTIFY_MODE_NORMAL &&
		    e->detail == XCB_NOTIFY_DETAIL_INFERIOR) {
			DNPRINTF(SWM_D_EVENT, "enternotify: entering from "
			    "inferior; ignoring\n");
			return;
		}

		focus_win(get_focus_magic(win));
	}

	xcb_flush(conn);
}

#ifdef SWM_DEBUG
void
leavenotify(xcb_leave_notify_event_t *e)
{
	DNPRINTF(SWM_D_FOCUS, "leavenotify: time: %u, win (x,y): %#x "
	    "(%d,%d), mode: %s(%d), detail: %s(%d), root (x,y): %#x (%d,%d), "
	    "child: %#x, same_screen_focus: %s, state: %d\n",
	    e->time, e->event, e->event_x, e->event_y,
	    get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail,
	    e->root, e->root_x, e->root_y, e->child,
	    YESNO(e->same_screen_focus), e->state);
}
#endif

void
mapnotify(xcb_map_notify_event_t *e)
{
	struct ws_win		*win, *parent = NULL;
	struct workspace	*ws;

	DNPRINTF(SWM_D_EVENT, "mapnotify: win %#x\n", e->window);

	if ((win = manage_window(e->window, spawn_position, true)) == NULL)
		return;
	ws = win->ws;

	/* Need to know if win was mapped due to ws switch. */
	if (ws->state == SWM_WS_STATE_MAPPED) {
		if (ws->focus_pending && TRANS(ws->focus_pending))
			parent = find_window(win->transient);

		/* If window's parent is maximized, don't clear it. */
		if ((parent == NULL) || !MAXIMIZED(parent))
			if (clear_maximized(ws) > 0)
				stack();
	}

	win->mapped = true;
	set_win_state(win, XCB_ICCCM_WM_STATE_NORMAL);

	if (focus_mode != SWM_FOCUS_FOLLOW && WS_FOCUSED(win->ws)) {
		if (ws->focus_pending == win) {
			focus_win(win);
			ws->focus_pending = NULL;
			center_pointer(win->ws->r);
			focus_flush();
		}
	}

	xcb_flush(conn);
}

void
mappingnotify(xcb_mapping_notify_event_t *e)
{
	struct ws_win	*w;
	int	i, j, num_screens;

	xcb_refresh_keyboard_mapping(syms, e);

	if (e->request == XCB_MAPPING_KEYBOARD) {
		grabkeys();

		/* Regrab buttons on all managed windows. */
		num_screens = get_screen_count();
		for (i = 0; i < num_screens; i++)
			for (j = 0; j < workspace_limit; j++)
				TAILQ_FOREACH(w, &screens[i].ws[j].winlist,
				    entry)
					grabbuttons(w);
	}
}

void
maprequest(xcb_map_request_event_t *e)
{
	struct ws_win		*win, *w = NULL;
	xcb_get_window_attributes_reply_t *war;

	DNPRINTF(SWM_D_EVENT, "maprequest: win %#x\n",
	    e->window);

	war = xcb_get_window_attributes_reply(conn,
	    xcb_get_window_attributes(conn, e->window),
	    NULL);
	if (war == NULL) {
		DNPRINTF(SWM_D_EVENT, "maprequest: window lost.\n");
		goto out;
	}

	if (war->override_redirect) {
		DNPRINTF(SWM_D_EVENT, "maprequest: override_redirect; "
		    "skipping.\n");
		goto out;
	}

	win = manage_window(e->window, spawn_position,
	    (war->map_state == XCB_MAP_STATE_VIEWABLE));
	if (win == NULL)
		goto out;

	/* The new window should get focus; prepare. */
	if (focus_mode != SWM_FOCUS_FOLLOW &&
	    !(win->quirks & SWM_Q_NOFOCUSONMAP) &&
	    (!(win->hints.flags & XCB_ICCCM_WM_HINT_INPUT) ||
	     (win->hints.flags & XCB_ICCCM_WM_HINT_INPUT &&
	      win->hints.input))) {
		if (win->quirks & SWM_Q_FOCUSONMAP_SINGLE) {
			/* See if other wins of same type are already mapped. */
			TAILQ_FOREACH(w, &win->ws->winlist, entry) {
				if (w == win || !w->mapped)
					continue;

				if (w->ch.class_name &&
				    win->ch.class_name &&
				    strcmp(w->ch.class_name,
				    win->ch.class_name) == 0 &&
				    w->ch.instance_name &&
				    win->ch.instance_name &&
				    strcmp(w->ch.instance_name,
				    win->ch.instance_name) == 0)
					break;
			}
		}

		if (w == NULL)
			win->ws->focus_pending = get_focus_magic(win);
	}

	/* All windows need to be mapped if they are in the current workspace.*/
	if (win->ws->r)
		stack();

	/* Ignore EnterNotify to handle the mapnotify without interference. */
	if (focus_mode == SWM_FOCUS_DEFAULT)
		event_drain(XCB_ENTER_NOTIFY);
out:
	free(war);
	DNPRINTF(SWM_D_EVENT, "maprequest: done.\n");
}

void
motionnotify(xcb_motion_notify_event_t *e)
{
	struct swm_region	*r = NULL;
	int			i, num_screens;

	DNPRINTF(SWM_D_FOCUS, "motionnotify: time: %u, win (x,y): %#x "
	    "(%d,%d), detail: %s(%d), root (x,y): %#x (%d,%d), "
	    "child: %#x, same_screen_focus: %s, state: %d\n",
	    e->time, e->event, e->event_x, e->event_y,
	    get_notify_detail_label(e->detail), e->detail,
	    e->root, e->root_x, e->root_y, e->child,
	    YESNO(e->same_screen), e->state);

	last_event_time = e->time;

	if (focus_mode == SWM_FOCUS_MANUAL)
		return;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == e->root)
			break;

	TAILQ_FOREACH(r, &screens[i].rl, entry)
		if (X(r) <= e->root_x && e->root_x < MAX_X(r) &&
		    Y(r) <= e->root_y && e->root_y < MAX_Y(r))
			break;

	focus_region(r);
}

#ifdef SWM_DEBUG
char *
get_atom_name(xcb_atom_t atom)
{
	char				*name = NULL;
#ifdef SWM_DEBUG_ATOM_NAMES
	/*
	 * This should be disabled during most debugging since
	 * xcb_get_* causes an xcb_flush.
	 */
	size_t				len;
	xcb_get_atom_name_reply_t	*r;

	r = xcb_get_atom_name_reply(conn,
	    xcb_get_atom_name(conn, atom),
	    NULL);
	if (r) {
		len = xcb_get_atom_name_name_length(r);
		if (len > 0) {
			name = malloc(len + 1);
			if (name) {
				memcpy(name, xcb_get_atom_name_name(r), len);
				name[len] = '\0';
			}
		}
		free(r);
	}
#else
	(void)atom;
#endif
	return (name);
}
#endif

void
propertynotify(xcb_property_notify_event_t *e)
{
	struct ws_win		*win;
	struct workspace	*ws;
#ifdef SWM_DEBUG
	char			*name;

	name = get_atom_name(e->atom);
	DNPRINTF(SWM_D_EVENT, "propertynotify: win %#x, atom: %s(%u), "
	    "time: %#x, state: %u\n", e->window, name, e->atom, e->time,
	    e->state);
	free(name);
#endif
	win = find_window(e->window);
	if (win == NULL)
		return;

	ws = win->ws;

	last_event_time = e->time;

	if (e->atom == a_state) {
		/* State just changed, make sure it gets focused if mapped. */
		if (e->state == XCB_PROPERTY_NEW_VALUE) {
			if (focus_mode != SWM_FOCUS_FOLLOW && WS_FOCUSED(ws)) {
				if (win->mapped &&
				    ws->focus_pending == win) {
					focus_win(ws->focus_pending);
					ws->focus_pending = NULL;
				}
			}
		}
	} else if (e->atom == XCB_ATOM_WM_CLASS ||
	    e->atom == XCB_ATOM_WM_NAME) {
		bar_draw();
	} else if (e->atom == a_prot) {
		get_wm_protocols(win);
	}

	xcb_flush(conn);
}

void
unmapnotify(xcb_unmap_notify_event_t *e)
{
	struct ws_win		*win;
	struct workspace	*ws;

	DNPRINTF(SWM_D_EVENT, "unmapnotify: win %#x\n", e->window);

	/* If we aren't managing the window, then ignore. */
	win = find_window(e->window);
	if (win == NULL || win->id != e->window)
		return;

	/* Do nothing if already withdrawn. */
	if (!win->mapped && !ICONIC(win))
		return;

	ws = win->ws;
	win->mapped = false;

	/* If win was focused, make sure to focus on something else. */
	if (win == ws->focus) {
		if (focus_mode != SWM_FOCUS_FOLLOW) {
			ws->focus_pending = get_focus_prev(win);
			DNPRINTF(SWM_D_EVENT, "unmapnotify: "
			    "focus_pending: %#x\n",
			    WINID(ws->focus_pending));
		}

		unfocus_win(win);
	}

	if (ICONIC(win)) {
		/* Iconify. */
		set_win_state(win, XCB_ICCCM_WM_STATE_ICONIC);
	} else {
		/* Withdraw. */
		set_win_state(win, XCB_ICCCM_WM_STATE_WITHDRAWN);
		unmanage_window(win);
	}

	if (ws->r)
		stack();

	/* Update focus if ws is active. */
	if (WS_FOCUSED(ws)) {
		if (focus_mode == SWM_FOCUS_FOLLOW) {
			focus_win(get_pointer_win(ws->r->s->root));
		} else if (ws->focus_pending) {
			focus_win(ws->focus_pending);
			ws->focus_pending = NULL;
		} else if (ws->focus == NULL) {
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT,
			    ws->r->id, XCB_CURRENT_TIME);
		}
	}

	center_pointer(ws->r);
	focus_flush();
}

#ifdef SWM_DEBUG
char *
get_source_type_label(uint32_t type)
{
	char *label;

	switch (type) {
	case EWMH_SOURCE_TYPE_NONE:
		label = "None";
		break;
	case EWMH_SOURCE_TYPE_NORMAL:
		label = "Normal";
		break;
	case EWMH_SOURCE_TYPE_OTHER:
		label = "Other";
		break;
	default:
		label = "Invalid";
	}

	return label;
}
#endif

void
clientmessage(xcb_client_message_event_t *e)
{
	struct ws_win		*win;
	struct swm_region	*r = NULL;
	union arg		a;
	uint32_t		val[2];
	int			num_screens, i;
	xcb_map_request_event_t	mre;
#ifdef SWM_DEBUG
	char			*name;

	name = get_atom_name(e->type);
	DNPRINTF(SWM_D_EVENT, "clientmessage: win %#x, atom: %s(%u)\n",
	    e->window, name, e->type);
	free(name);
#endif

	if (e->type == ewmh[_NET_CURRENT_DESKTOP].atom) {
		num_screens = get_screen_count();
		for (i = 0; i < num_screens; i++)
			if (screens[i].root == e->window) {
				r = screens[i].r_focus;
				break;
			}

		if (r && e->data.data32[0] < (uint32_t)workspace_limit) {
			a.id = e->data.data32[0];
			switchws(r, &a);
			focus_flush();
		}

		return;
	}

	win = find_window(e->window);
	if (win == NULL) {
		if (e->type == ewmh[_NET_ACTIVE_WINDOW].atom) {
			/* Manage the window with maprequest. */
			DNPRINTF(SWM_D_EVENT, "clientmessage: request focus on "
			    "unmanaged window.\n");
			mre.window = e->window;
			maprequest(&mre);
		}
		return;
	}

	if (e->type == ewmh[_NET_ACTIVE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_ACTIVE_WINDOW, "
		    "source_type: %s(%d)\n",
		    get_source_type_label(e->data.data32[0]),
		    e->data.data32[0]);

		/*
		 * Allow focus changes that are a result of direct user
		 * action and from applications that use the old EWMH spec.
		 */
		if (e->data.data32[0] != EWMH_SOURCE_TYPE_NORMAL ||
		    win->quirks & SWM_Q_OBEYAPPFOCUSREQ) {
			if (WS_FOCUSED(win->ws))
				focus_win(win);
			else
				win->ws->focus_pending = win;
		}
	} else if (e->type == ewmh[_NET_CLOSE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_CLOSE_WINDOW\n");
		if (win->can_delete)
			client_msg(win, a_delete, 0);
		else
			xcb_kill_client(conn, win->id);
	} else if (e->type == ewmh[_NET_MOVERESIZE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT,
		    "clientmessage: _NET_MOVERESIZE_WINDOW\n");
		if (ABOVE(win)) {
			if (e->data.data32[0] & (1<<8)) /* x */
				X(win) = e->data.data32[1];
			if (e->data.data32[0] & (1<<9)) /* y */
				Y(win) = e->data.data32[2];
			if (e->data.data32[0] & (1<<10)) /* width */
				WIDTH(win) = e->data.data32[3];
			if (e->data.data32[0] & (1<<11)) /* height */
				HEIGHT(win) = e->data.data32[4];

			update_window(win);
		} else {
			/* Notify no change was made. */
			config_win(win, NULL);
			/* TODO: Change stack sizes */
		}
	} else if (e->type == ewmh[_NET_RESTACK_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_RESTACK_WINDOW\n");
		val[0] = e->data.data32[1]; /* Sibling window. */
		val[1] = e->data.data32[2]; /* Stack mode detail. */

		xcb_configure_window(conn, win->id, XCB_CONFIG_WINDOW_SIBLING |
		    XCB_CONFIG_WINDOW_STACK_MODE, val);
	} else 	if (e->type == ewmh[_NET_WM_STATE].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_WM_STATE\n");
		ewmh_change_wm_state(win, e->data.data32[1], e->data.data32[0]);
		if (e->data.data32[2])
			ewmh_change_wm_state(win, e->data.data32[2],
			    e->data.data32[0]);

		ewmh_update_wm_state(win);
		stack();
	} else if (e->type == ewmh[_NET_WM_DESKTOP].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_WM_DESKTOP\n");
		r = win->ws->r;

		win_to_ws(win, e->data.data32[0], true);

		/* Restack if either the source or destination ws is mapped. */
		if (r != NULL || win->ws->r != NULL) {
			if (FLOATING(win))
				load_float_geom(win);

			stack();
		}
	}

	focus_flush();
}

void
check_conn(void)
{
	int	 errcode = xcb_connection_has_error(conn);
#ifdef XCB_CONN_ERROR
	char	*s;
	switch (errcode) {
	case XCB_CONN_ERROR:
		s = "Socket error, pipe error or other stream error.";
		break;
	case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
		s = "Extension not supported.";
		break;
	case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
		s = "Insufficient memory.";
		break;
	case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
		s = "Request length was exceeded.";
		break;
	case XCB_CONN_CLOSED_PARSE_ERR:
		s = "Error parsing display string.";
		break;
	default:
		s = "Unknown error.";
	}
	if (errcode)
		errx(errcode, "X CONNECTION ERROR: %s", s);
#else
	if (errcode)
		errx(errcode, "X CONNECTION ERROR");
#endif
}

int
enable_wm(void)
{
	int			num_screens, i;
	const uint32_t		val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	    XCB_EVENT_MASK_ENTER_WINDOW;
	xcb_screen_t		*sc;
	xcb_void_cookie_t	wac;
	xcb_generic_error_t	*error;

	/* this causes an error if some other window manager is running */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		if ((sc = get_screen(i)) == NULL)
			errx(1, "ERROR: can't get screen %d.", i);
		DNPRINTF(SWM_D_INIT, "enable_wm: screen %d, root: %#x\n",
		    i, sc->root);
		wac = xcb_change_window_attributes_checked(conn, sc->root,
		    XCB_CW_EVENT_MASK, &val);
		if ((error = xcb_request_check(conn, wac))) {
			DNPRINTF(SWM_D_INIT, "enable_wm: error_code: %u\n",
			    error->error_code);
			free(error);
			return 1;
		}

		/* click to focus on empty region */
		xcb_grab_button(conn, 1, sc->root, BUTTONMASK,
		    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
		    XCB_CURSOR_NONE, XCB_BUTTON_INDEX_1, XCB_BUTTON_MASK_ANY);
	}

	return 0;
}

void
new_region(struct swm_screen *s, int x, int y, int w, int h)
{
	struct swm_region	*r = NULL, *n;
	struct workspace	*ws = NULL;
	int			i;
	uint32_t		wa[1];

	DNPRINTF(SWM_D_MISC, "new region: screen[%d]:%dx%d+%d+%d\n",
	     s->idx, w, h, x, y);

	/* remove any conflicting regions */
	n = TAILQ_FIRST(&s->rl);
	while (n) {
		r = n;
		n = TAILQ_NEXT(r, entry);
		if (X(r) < (x + w) && (X(r) + WIDTH(r)) > x &&
		    Y(r) < (y + h) && (Y(r) + HEIGHT(r)) > y) {
			if (r->ws->r != NULL)
				r->ws->old_r = r->ws->r;
			r->ws->r = NULL;
			bar_cleanup(r);
			xcb_destroy_window(conn, r->id);
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
			err(1, "new_region: calloc: failed to allocate memory "
			    "for screen");

	/* if we don't have a workspace already, find one */
	if (ws == NULL) {
		for (i = 0; i < workspace_limit; i++)
			if (s->ws[i].r == NULL) {
				ws = &s->ws[i];
				break;
			}
	}

	if (ws == NULL)
		errx(1, "new_region: no free workspaces");

	if (ws->state == SWM_WS_STATE_HIDDEN)
		ws->state = SWM_WS_STATE_MAPPING;

	X(r) = x;
	Y(r) = y;
	WIDTH(r) = w;
	HEIGHT(r) = h;
	r->s = s;
	r->ws = ws;
	r->ws_prior = NULL;
	ws->r = r;
	outputs++;
	TAILQ_INSERT_TAIL(&s->rl, r, entry);

	/* Invisible region window to detect pointer events on empty regions. */
	r->id = xcb_generate_id(conn);
	wa[0] = XCB_EVENT_MASK_POINTER_MOTION |
	    XCB_EVENT_MASK_POINTER_MOTION_HINT;

	xcb_create_window(conn, XCB_COPY_FROM_PARENT, r->id, r->s->root,
	    X(r), Y(r), WIDTH(r), HEIGHT(r), 0, XCB_WINDOW_CLASS_INPUT_ONLY,
	    XCB_COPY_FROM_PARENT, XCB_CW_EVENT_MASK, wa);

	/* Make sure region input is at the bottom. */
	wa[0] = XCB_STACK_MODE_BELOW;
	xcb_configure_window(conn, r->id, XCB_CONFIG_WINDOW_STACK_MODE, wa);

	xcb_map_window(conn, r->id);
}

void
scan_randr(int idx)
{
#ifdef SWM_XRR_HAS_CRTC
	int						c;
	int						ncrtc = 0;
#endif /* SWM_XRR_HAS_CRTC */
	struct swm_region				*r;
	struct ws_win					*win;
	int						num_screens;
	xcb_randr_get_screen_resources_current_cookie_t	src;
	xcb_randr_get_screen_resources_current_reply_t	*srr;
	xcb_randr_get_crtc_info_cookie_t		cic;
	xcb_randr_get_crtc_info_reply_t			*cir = NULL;
	xcb_randr_crtc_t				*crtc;
	xcb_screen_t					*screen;

	DNPRINTF(SWM_D_MISC, "scan_randr: screen: %d\n", idx);

	if ((screen = get_screen(idx)) == NULL)
		errx(1, "ERROR: can't get screen %d.", idx);

	num_screens = get_screen_count();
	if (idx >= num_screens)
		errx(1, "scan_randr: invalid screen");

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&screens[idx].rl)) != NULL) {
		r->ws->old_r = r->ws->r = NULL;
		bar_cleanup(r);
		xcb_destroy_window(conn, r->id);
		TAILQ_REMOVE(&screens[idx].rl, r, entry);
		TAILQ_INSERT_TAIL(&screens[idx].orl, r, entry);
	}
	outputs = 0;

	/* map virtual screens onto physical screens */
#ifdef SWM_XRR_HAS_CRTC
	if (randr_support) {
		src = xcb_randr_get_screen_resources_current(conn,
		    screens[idx].root);
		srr = xcb_randr_get_screen_resources_current_reply(conn, src,
		    NULL);
		if (srr == NULL) {
			new_region(&screens[idx], 0, 0,
			    screen->width_in_pixels,
			    screen->height_in_pixels);
			goto out;
		} else
			ncrtc = srr->num_crtcs;

		crtc = xcb_randr_get_screen_resources_current_crtcs(srr);
		for (c = 0; c < ncrtc; c++) {
			cic = xcb_randr_get_crtc_info(conn, crtc[c],
			    XCB_CURRENT_TIME);
			cir = xcb_randr_get_crtc_info_reply(conn, cic, NULL);
			if (cir == NULL)
				continue;
			if (cir->num_outputs == 0) {
				free(cir);
				continue;
			}

			if (cir->mode == 0)
				new_region(&screens[idx], 0, 0,
				    screen->width_in_pixels,
				    screen->height_in_pixels);
			else
				new_region(&screens[idx],
				    cir->x, cir->y, cir->width, cir->height);
			free(cir);
		}
		free(srr);
	}
#endif /* SWM_XRR_HAS_CRTC */

	/* If detection failed, create a single region that spans the screen. */
	if (TAILQ_EMPTY(&screens[idx].rl))
		new_region(&screens[idx], 0, 0, screen->width_in_pixels,
		    screen->height_in_pixels);

out:
	/* Cleanup unused previously visible workspaces. */
	TAILQ_FOREACH(r, &screens[idx].orl, entry) {
		TAILQ_FOREACH(win, &r->ws->winlist, entry)
			unmap_window(win);
		r->ws->state = SWM_WS_STATE_HIDDEN;

		/* The screen shouldn't focus on an unused region. */
		if (screens[idx].r_focus == r)
			screens[idx].r_focus = NULL;
	}

	DNPRINTF(SWM_D_MISC, "scan_randr: done.\n");
}

void
screenchange(xcb_randr_screen_change_notify_event_t *e)
{
	struct swm_region		*r;
	int				i, num_screens;

	DNPRINTF(SWM_D_EVENT, "screenchange: root: %#x\n", e->root);

	num_screens = get_screen_count();
	/* silly event doesn't include the screen index */
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == e->root)
			break;
	if (i >= num_screens)
		errx(1, "screenchange: screen not found");

	/* brute force for now, just re-enumerate the regions */
	scan_randr(i);

#ifdef SWM_DEBUG
	print_win_geom(e->root);
#endif
	/* add bars to all regions */
	for (i = 0; i < num_screens; i++) {
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			bar_setup(r);
	}

	stack();

	/* Make sure a region has focus on each screen. */
	for (i = 0; i < num_screens; i++) {
		if (screens[i].r_focus == NULL) {
			r = TAILQ_FIRST(&screens[i].rl);
			if (r != NULL)
				focus_region(r);
		}
	}

	bar_draw();
	focus_flush();

	/* Update workspace state on all regions. */
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			r->ws->state = SWM_WS_STATE_MAPPED;
}

void
grab_windows(void)
{
	struct swm_region	*r = NULL;
	xcb_window_t		*wins = NULL, trans, *cwins = NULL;
	int			i, j, k, n, no, num_screens;
	uint8_t			state;
	bool			manage, mapped;

	xcb_query_tree_cookie_t			qtc;
	xcb_query_tree_reply_t			*qtr;
	xcb_get_window_attributes_cookie_t	gac;
	xcb_get_window_attributes_reply_t	*gar;
	xcb_get_property_cookie_t		pc;
	xcb_get_property_reply_t		*pr;

	DNPRINTF(SWM_D_INIT, "grab_windows: begin\n");
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		qtc = xcb_query_tree(conn, screens[i].root);
		qtr = xcb_query_tree_reply(conn, qtc, NULL);
		if (qtr == NULL)
			continue;
		wins = xcb_query_tree_children(qtr);
		no = xcb_query_tree_children_length(qtr);

		/* Try to sort windows according to _NET_CLIENT_LIST. */
		pr = xcb_get_property_reply(conn, xcb_get_property(conn, 0,
		    screens[i].root, ewmh[_NET_CLIENT_LIST].atom,
		    XCB_ATOM_WINDOW, 0, UINT32_MAX), NULL);
		if (pr != NULL) {
			cwins = xcb_get_property_value(pr);
			n = xcb_get_property_value_length(pr) /
			    sizeof(xcb_atom_t);

			for (j = 0; j < n; ++j) {
				for (k = j; k < no; ++k) {
					if (wins[k] == cwins[j]) {
						/* Swap wins j and k. */
						wins[k] = wins[j];
						wins[j] = cwins[j];
					}
				}
			}

			free(pr);
		}

		/* attach windows to a region */
		/* normal windows */
		DNPRINTF(SWM_D_INIT, "grab_windows: grab top level windows.\n");
		for (j = 0; j < no; j++) {
			TAILQ_FOREACH(r, &screens[i].rl, entry) {
				if (r->id == wins[j]) {
					DNPRINTF(SWM_D_INIT, "grab_windows: "
					    "skip %#x; region input window.\n",
					    wins[j]);
					break;
				} else if (r->bar->id == wins[j]) {
					DNPRINTF(SWM_D_INIT, "grab_windows: "
					    "skip %#x; region bar.\n",
					    wins[j]);
					break;
				}
			}

			if (r)
				continue;

			gac = xcb_get_window_attributes(conn, wins[j]);
			gar = xcb_get_window_attributes_reply(conn, gac, NULL);
			if (gar == NULL) {
				DNPRINTF(SWM_D_INIT, "grab_windows: skip %#x; "
				    "doesn't exist.\n", wins[j]);
				continue;
			}

			if (gar->override_redirect) {
				DNPRINTF(SWM_D_INIT, "grab_windows: skip %#x; "
				    "override_redirect set.\n", wins[j]);
				free(gar);
				continue;
			}

			pc = xcb_icccm_get_wm_transient_for(conn, wins[j]);
			if (xcb_icccm_get_wm_transient_for_reply(conn, pc,
			    &trans, NULL)) {
				DNPRINTF(SWM_D_INIT, "grab_windows: skip %#x; "
				    "is transient for %#x.\n", wins[j], trans);
				free(gar);
				continue;
			}

			state = get_win_state(wins[j]);
			manage = state != XCB_ICCCM_WM_STATE_WITHDRAWN;
			mapped = gar->map_state == XCB_MAP_STATE_VIEWABLE;
			if (mapped || manage)
				manage_window(wins[j], SWM_STACK_TOP, mapped);
			free(gar);
		}
		/* transient windows */
		DNPRINTF(SWM_D_INIT, "grab_windows: grab transient windows.\n");
		for (j = 0; j < no; j++) {
			gac = xcb_get_window_attributes(conn, wins[j]);
			gar = xcb_get_window_attributes_reply(conn, gac, NULL);
			if (gar == NULL) {
				DNPRINTF(SWM_D_INIT, "grab_windows: skip %#x; "
				    "doesn't exist.\n", wins[j]);
				continue;
			}

			if (gar->override_redirect) {
				DNPRINTF(SWM_D_INIT, "grab_windows: skip %#x; "
				    "override_redirect set.\n", wins[j]);
				free(gar);
				continue;
			}

			state = get_win_state(wins[j]);
			manage = state != XCB_ICCCM_WM_STATE_WITHDRAWN;
			mapped = gar->map_state == XCB_MAP_STATE_VIEWABLE;
			pc = xcb_icccm_get_wm_transient_for(conn, wins[j]);
			if (xcb_icccm_get_wm_transient_for_reply(conn, pc,
			    &trans, NULL) && manage)
				manage_window(wins[j], SWM_STACK_TOP, mapped);
			free(gar);
		}
		free(qtr);
	}
	DNPRINTF(SWM_D_INIT, "grab_windows: done.\n");
}

void
setup_screens(void)
{
	int			i, j, k, num_screens;
	struct workspace	*ws;
	uint32_t		gcv[1], wa[1];
	const xcb_query_extension_reply_t *qep;
	xcb_screen_t				*screen;
	xcb_randr_query_version_cookie_t	c;
	xcb_randr_query_version_reply_t		*r;

	num_screens = get_screen_count();
	if ((screens = calloc(num_screens,
	     sizeof(struct swm_screen))) == NULL)
		err(1, "setup_screens: calloc: failed to allocate memory for "
		    "screens");

	/* Initial RandR setup. */
	randr_support = false;
	qep = xcb_get_extension_data(conn, &xcb_randr_id);
	if (qep->present) {
		c = xcb_randr_query_version(conn, 1, 1);
		r = xcb_randr_query_version_reply(conn, c, NULL);
		if (r) {
			if (r->major_version >= 1) {
				randr_support = true;
				randr_eventbase = qep->first_event;
			}
			free(r);
		}
	}

	wa[0] = cursors[XC_LEFT_PTR].cid;

	/* map physical screens */
	for (i = 0; i < num_screens; i++) {
		DNPRINTF(SWM_D_WS, "setup_screens: init screen: %d\n", i);
		screens[i].idx = i;
		screens[i].r_focus = NULL;

		TAILQ_INIT(&screens[i].rl);
		TAILQ_INIT(&screens[i].orl);
		if ((screen = get_screen(i)) == NULL)
			errx(1, "ERROR: can't get screen %d.", i);
		screens[i].root = screen->root;

		/* set default colors */
		setscreencolor("red", i, SWM_S_COLOR_FOCUS);
		setscreencolor("rgb:88/88/88", i, SWM_S_COLOR_UNFOCUS);
		setscreencolor("rgb:00/80/80", i, SWM_S_COLOR_BAR_BORDER);
		setscreencolor("rgb:00/40/40", i,
		    SWM_S_COLOR_BAR_BORDER_UNFOCUS);
		setscreencolor("black", i, SWM_S_COLOR_BAR);
		setscreencolor("rgb:a0/a0/a0", i, SWM_S_COLOR_BAR_FONT);
		setscreencolor("red", i, SWM_S_COLOR_FOCUS_MAXIMIZED);
		setscreencolor("rgb:88/88/88", i,
		    SWM_S_COLOR_UNFOCUS_MAXIMIZED);

		/* create graphics context on screen */
		screens[i].bar_gc = xcb_generate_id(conn);
		gcv[0] = 0;
		xcb_create_gc(conn, screens[i].bar_gc, screens[i].root,
		    XCB_GC_GRAPHICS_EXPOSURES, gcv);

		/* set default cursor */
		xcb_change_window_attributes(conn, screens[i].root,
		    XCB_CW_CURSOR, wa);

		/* init all workspaces */
		/* XXX these should be dynamically allocated too */
		for (j = 0; j < SWM_WS_MAX; j++) {
			ws = &screens[i].ws[j];
			ws->idx = j;
			ws->name = NULL;
			ws->bar_enabled = true;
			ws->focus = NULL;
			ws->focus_prev = NULL;
			ws->focus_pending = NULL;
			ws->r = NULL;
			ws->old_r = NULL;
			ws->state = SWM_WS_STATE_HIDDEN;
			TAILQ_INIT(&ws->stack);
			TAILQ_INIT(&ws->winlist);
			TAILQ_INIT(&ws->unmanagedlist);

			for (k = 0; layouts[k].l_stack != NULL; k++)
				if (layouts[k].l_config != NULL)
					layouts[k].l_config(ws,
					    SWM_ARG_ID_STACKINIT);
			ws->cur_layout = &layouts[0];
			ws->cur_layout->l_string(ws);
		}

		scan_randr(i);

		if (randr_support)
			xcb_randr_select_input(conn, screens[i].root,
			    XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	}
}

void
setup_globals(void)
{
	if ((bar_fonts = strdup(SWM_BAR_FONTS)) == NULL)
		err(1, "setup_globals: strdup: failed to allocate memory.");

	if ((clock_format = strdup("%a %b %d %R %Z %Y")) == NULL)
		err(1, "setup_globals: strdup: failed to allocate memory.");

	if ((syms = xcb_key_symbols_alloc(conn)) == NULL)
		errx(1, "unable to allocate key symbols");

	a_state = get_atom_from_string("WM_STATE");
	a_prot = get_atom_from_string("WM_PROTOCOLS");
	a_delete = get_atom_from_string("WM_DELETE_WINDOW");
	a_net_supported = get_atom_from_string("_NET_SUPPORTED");
	a_net_wm_check = get_atom_from_string("_NET_SUPPORTING_WM_CHECK");
	a_takefocus = get_atom_from_string("WM_TAKE_FOCUS");
	a_utf8_string = get_atom_from_string("UTF8_STRING");
	a_swm_ws = get_atom_from_string("_SWM_WS");
}

void
shutdown_cleanup(void)
{
	int			i, num_screens;

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		err(1, "can't disable alarm");

	bar_extra_stop();
	unmap_all();

	cursors_cleanup();

	teardown_ewmh();

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
		    screens[i].root, XCB_CURRENT_TIME);

		if (screens[i].bar_gc != XCB_NONE)
			xcb_free_gc(conn, screens[i].bar_gc);
		if (!bar_font_legacy) {
			XftColorFree(display, DefaultVisual(display, i),
			    DefaultColormap(display, i), &bar_font_color);
			XftColorFree(display, DefaultVisual(display, i),
			    DefaultColormap(display, i), &search_font_color);
		}
	}

	if (bar_font_legacy)
		XFreeFontSet(display, bar_fs);
	else {
		XftFontClose(display, bar_font);
	}

	xcb_key_symbols_free(syms);
	xcb_flush(conn);
	xcb_disconnect(conn);
}

void
event_error(xcb_generic_error_t *e)
{
	(void)e;

	DNPRINTF(SWM_D_EVENT, "event_error: %s(%u) from %s(%u), sequence: %u, "
	    "resource_id: %u, minor_code: %u\n",
	    xcb_event_get_error_label(e->error_code), e->error_code,
	    xcb_event_get_request_label(e->major_code), e->major_code,
	    e->sequence, e->resource_id, e->minor_code);
}

void
event_handle(xcb_generic_event_t *evt)
{
	uint8_t			type = XCB_EVENT_RESPONSE_TYPE(evt);

	DNPRINTF(SWM_D_EVENT, "XCB Event: %s(%d), seq %u\n",
	    xcb_event_get_label(XCB_EVENT_RESPONSE_TYPE(evt)),
	    XCB_EVENT_RESPONSE_TYPE(evt), evt->sequence);

	switch (type) {
#define EVENT(type, callback) case type: callback((void *)evt); return
	EVENT(0, event_error);
	EVENT(XCB_BUTTON_PRESS, buttonpress);
	/*EVENT(XCB_BUTTON_RELEASE, buttonpress);*/
	/*EVENT(XCB_CIRCULATE_NOTIFY, );*/
	/*EVENT(XCB_CIRCULATE_REQUEST, );*/
	EVENT(XCB_CLIENT_MESSAGE, clientmessage);
	/*EVENT(XCB_COLORMAP_NOTIFY, );*/
	EVENT(XCB_CONFIGURE_NOTIFY, configurenotify);
	EVENT(XCB_CONFIGURE_REQUEST, configurerequest);
	/*EVENT(XCB_CREATE_NOTIFY, );*/
	EVENT(XCB_DESTROY_NOTIFY, destroynotify);
	EVENT(XCB_ENTER_NOTIFY, enternotify);
	EVENT(XCB_EXPOSE, expose);
#ifdef SWM_DEBUG
	EVENT(XCB_FOCUS_IN, focusin);
	EVENT(XCB_FOCUS_OUT, focusout);
#endif
	/*EVENT(XCB_GRAPHICS_EXPOSURE, );*/
	/*EVENT(XCB_GRAVITY_NOTIFY, );*/
	EVENT(XCB_KEY_PRESS, keypress);
	/*EVENT(XCB_KEY_RELEASE, keypress);*/
	/*EVENT(XCB_KEYMAP_NOTIFY, );*/
#ifdef SWM_DEBUG
	EVENT(XCB_LEAVE_NOTIFY, leavenotify);
#endif
	EVENT(XCB_MAP_NOTIFY, mapnotify);
	EVENT(XCB_MAP_REQUEST, maprequest);
	EVENT(XCB_MAPPING_NOTIFY, mappingnotify);
	EVENT(XCB_MOTION_NOTIFY, motionnotify);
	/*EVENT(XCB_NO_EXPOSURE, );*/
	EVENT(XCB_PROPERTY_NOTIFY, propertynotify);
	/*EVENT(XCB_REPARENT_NOTIFY, );*/
	/*EVENT(XCB_RESIZE_REQUEST, );*/
	/*EVENT(XCB_SELECTION_CLEAR, );*/
	/*EVENT(XCB_SELECTION_NOTIFY, );*/
	/*EVENT(XCB_SELECTION_REQUEST, );*/
	EVENT(XCB_UNMAP_NOTIFY, unmapnotify);
	/*EVENT(XCB_VISIBILITY_NOTIFY, );*/
#undef EVENT
	}
	if (type - randr_eventbase == XCB_RANDR_SCREEN_CHANGE_NOTIFY)
		screenchange((void *)evt);
}

int
main(int argc, char *argv[])
{
	struct swm_region	*r;
	char			conf[PATH_MAX], *cfile = NULL;
	struct stat		sb;
	int			xfd, i, num_screens;
	struct sigaction	sact;
	xcb_generic_event_t	*evt;
	int			num_readable;
	struct pollfd		pfd[2];
	bool			stdin_ready = false, startup = true;

	/* suppress unused warning since var is needed */
	(void)argc;

#ifdef SWM_DEBUG
	time_started = time(NULL);
#endif

	start_argv = argv;
	warnx("Welcome to spectrwm V%s Build: %s", SPECTRWM_VERSION, buildstr);
	if (setlocale(LC_CTYPE, "") == NULL || setlocale(LC_TIME, "") == NULL)
		warnx("no locale support");

	/* handle some signals */
	bzero(&sact, sizeof(sact));
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = sighdlr;
	sigaction(SIGINT, &sact, NULL);
	sigaction(SIGQUIT, &sact, NULL);
	sigaction(SIGTERM, &sact, NULL);
	sigaction(SIGHUP, &sact, NULL);

	sact.sa_handler = sighdlr;
	sact.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sact, NULL);

	if ((display = XOpenDisplay(0)) == NULL)
		errx(1, "can not open display");

	conn = XGetXCBConnection(display);
	if (xcb_connection_has_error(conn))
		errx(1, "can not get XCB connection");

	XSetEventQueueOwner(display, XCBOwnsEventQueue);

	xcb_prefetch_extension_data(conn, &xcb_randr_id);
	xfd = xcb_get_file_descriptor(conn);

	/* look for local and global conf file */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user: %d", getuid());

	xcb_grab_server(conn);
	xcb_aux_sync(conn);

	/* flush all events */
	while ((evt = xcb_poll_for_event(conn))) {
		if (XCB_EVENT_RESPONSE_TYPE(evt) == 0)
			event_handle(evt);
		free(evt);
	}

	if (enable_wm())
		errx(1, "another window manager is currently running");

	/* Load Xcursors and/or cursorfont glyph cursors. */
	cursors_load();

	xcb_aux_sync(conn);

	setup_globals();
	setup_screens();
	setup_ewmh();
	setup_keys();
	setup_quirks();
	setup_spawn();

	/* load config */
	for (i = 0; ; i++) {
		conf[0] = '\0';
		switch (i) {
		case 0:
			/* ~ */
			snprintf(conf, sizeof conf, "%s/.%s",
			    pwd->pw_dir, SWM_CONF_FILE);
			break;
		case 1:
			/* global */
			snprintf(conf, sizeof conf, "/etc/%s",
			    SWM_CONF_FILE);
			break;
		case 2:
			/* ~ compat */
			snprintf(conf, sizeof conf, "%s/.%s",
			    pwd->pw_dir, SWM_CONF_FILE_OLD);
			break;
		case 3:
			/* global compat */
			snprintf(conf, sizeof conf, "/etc/%s",
			    SWM_CONF_FILE_OLD);
			break;
		default:
			goto noconfig;
		}

		if (strlen(conf) && stat(conf, &sb) != -1)
			if (S_ISREG(sb.st_mode)) {
				cfile = conf;
				break;
			}
	}
noconfig:

	/* load conf (if any) */
	if (cfile)
		conf_load(cfile, SWM_CONF_DEFAULT);

	validate_spawns();

	if (getenv("SWM_STARTED") == NULL)
		setenv("SWM_STARTED", "YES", 1);

	/* setup all bars */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			bar_setup(r);

	/* Manage existing windows. */
	grab_windows();

	grabkeys();
	stack();
	bar_draw();

	xcb_ungrab_server(conn);
	xcb_flush(conn);

	/* Update state of each newly mapped workspace. */
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			r->ws->state = SWM_WS_STATE_MAPPED;

	memset(&pfd, 0, sizeof(pfd));
	pfd[0].fd = xfd;
	pfd[0].events = POLLIN;
	pfd[1].fd = STDIN_FILENO;
	pfd[1].events = POLLIN;

	while (running) {
		while ((evt = xcb_poll_for_event(conn))) {
			if (!running)
				goto done;
			event_handle(evt);
			free(evt);
		}

		/* If just (re)started, set default focus if needed. */
		if (startup) {
			startup = false;

			if (focus_mode != SWM_FOCUS_FOLLOW) {
				r = TAILQ_FIRST(&screens[0].rl);
				if (r) {
					focus_region(r);
					focus_flush();
				}
				continue;
			}
		}

		num_readable = poll(pfd, bar_extra ? 2 : 1, 1000);
		if (num_readable == -1) {
			DNPRINTF(SWM_D_MISC, "poll failed: %s", strerror(errno));
		} else if (num_readable > 0 && bar_extra && pfd[1].revents & POLLIN) {
			stdin_ready = true;
		}

		if (restart_wm)
			restart(NULL, NULL);

		if (search_resp)
			search_do_resp();

		if (!running)
			goto done;

		if (stdin_ready) {
			stdin_ready = false;
			bar_extra_update();
		}

		bar_draw();
		xcb_flush(conn);
	}
done:
	shutdown_cleanup();

	return (0);
}
