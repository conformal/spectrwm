/*
 * Copyright (c) 2009-2019 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009-2011 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 * Copyright (c) 2010 Tuukka Kataja <stuge@xor.fi>
 * Copyright (c) 2011 Jason L. Wright <jason@thought.net>
 * Copyright (c) 2011-2021 Reginald Kennedy <rk@rejii.com>
 * Copyright (c) 2011-2012 Lawrence Teo <lteo@lteo.net>
 * Copyright (c) 2011-2012 Tiago Cunha <tcunha@gmx.com>
 * Copyright (c) 2012-2015 David Hill <dhill@mindcry.org>
 * Copyright (c) 2014-2015 Yuri D'Elia <yuri.delia@eurac.edu>
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

/* kernel includes */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/queue.h>
#if !defined(__OpenBSD__)
#include "queue_compat.h"
#endif
#include <sys/param.h>
#include <sys/select.h>
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
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
#if defined(__FreeBSD__)
#include <libutil.h>
#else
#include <util.h>
#endif
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <xcb/xinput.h>
#define SWM_XCB_HAS_XINPUT
#endif
#include <xcb/xtest.h>
#include <xcb/randr.h>

/* local includes */
#include "version.h"
#ifdef __OSX__
#include <osx.h>
#endif

/* singly-linked tail queue macros appeared in OpenBSD 6.9 */
#ifndef STAILQ_HEAD
#define STAILQ_HEAD			SIMPLEQ_HEAD
#define STAILQ_HEAD_INITIALIZER		SIMPLEQ_HEAD_INITIALIZER
#define STAILQ_ENTRY			SIMPLEQ_ENTRY
#define STAILQ_FIRST			SIMPLEQ_FIRST
#define STAILQ_END			SIMPLEQ_END
#define STAILQ_EMPTY			SIMPLEQ_EMPTY
#define STAILQ_NEXT			SIMPLEQ_NEXT
#define STAILQ_FOREACH			SIMPLEQ_FOREACH
#define STAILQ_FOREACH_SAFE		SIMPLEQ_FOREACH_SAFE
#define STAILQ_INIT			SIMPLEQ_INIT
#define STAILQ_INSERT_HEAD		SIMPLEQ_INSERT_HEAD
#define STAILQ_INSERT_TAIL		SIMPLEQ_INSERT_TAIL
#define STAILQ_INSERT_AFTER		SIMPLEQ_INSERT_AFTER
#define STAILQ_REMOVE_HEAD		SIMPLEQ_REMOVE_HEAD
#define STAILQ_REMOVE_AFTER		SIMPLEQ_REMOVE_AFTER
/*#define STAILQ_REMOVE*/
#define STAILQ_CONCAT			SIMPLEQ_CONCAT
/*#define STAILQ_LAST*/
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
#define DNPRINTF(n, fmt, args...) do {						\
	if (swm_debug & n) 							\
		fprintf(stderr, "%ld %s: " fmt,					\
		    (long)(time(NULL) - time_started), __func__, ## args);	\
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

#define ALLOCSTR(s, x...) do {							\
	if (s && asprintf(s, x) == -1)						\
		err(1, "asprintf");						\
} while (0)

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
#define ANYMOD			XCB_MOD_MASK_ANY
#define CLEANMASK(mask)		((mask) & (XCB_KEY_BUT_MASK_SHIFT |	\
    XCB_KEY_BUT_MASK_CONTROL | XCB_KEY_BUT_MASK_MOD_1 |			\
    XCB_KEY_BUT_MASK_MOD_2 | XCB_KEY_BUT_MASK_MOD_3 |			\
    XCB_KEY_BUT_MASK_MOD_4 | XCB_KEY_BUT_MASK_MOD_5) & ~(numlockmask))
#define BUTTONMASK		(XCB_EVENT_MASK_BUTTON_PRESS |		\
    XCB_EVENT_MASK_BUTTON_RELEASE)
#define MOUSEMASK		(BUTTONMASK|XCB_EVENT_MASK_POINTER_MOTION)
#define SWM_PROPLEN		(16)
#define SWM_FUNCNAME_LEN	(32)
#define SWM_QUIRK_LEN		(64)
#define X(r)			((r)->g.x)
#define Y(r)			((r)->g.y)
#define WIDTH(r)		((r)->g.w)
#define HEIGHT(r)		((r)->g.h)
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
#define ACCEPTS_FOCUS(w)	(!((w)->hints.flags & XCB_ICCCM_WM_HINT_INPUT) \
    || ((w)->hints.input))
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

#define SWM_COUNT_TILED		(1 << 0)
#define SWM_COUNT_FLOATING	(1 << 1)
#define SWM_COUNT_ICONIC	(1 << 2)
#define SWM_COUNT_NORMAL	(SWM_COUNT_TILED | SWM_COUNT_FLOATING)
#define SWM_COUNT_ALL		(SWM_COUNT_NORMAL | SWM_COUNT_ICONIC)

#define SWM_WIN_UNFOCUS		(1 << 0)
#define SWM_WIN_NOUNMAP		(1 << 1)

#define SWM_CK_ALL		(0xf)
#define SWM_CK_FOCUS		(0x1)
#define SWM_CK_POINTER		(0x2)
#define SWM_CK_FALLBACK		(0x4)
#define SWM_CK_REGION		(0x8)

#define SWM_WSI_LISTCURRENT	(0x001)
#define SWM_WSI_LISTACTIVE	(0x002)
#define SWM_WSI_LISTEMPTY	(0x004)
#define SWM_WSI_LISTNAMED	(0x008)
#define SWM_WSI_LISTURGENT	(0x010)
#define SWM_WSI_LISTALL		(0x0ff)
#define SWM_WSI_HIDECURRENT	(0x100)
#define SWM_WSI_MARKCURRENT	(0x200)
#define SWM_WSI_MARKURGENT	(0x400)
#define SWM_WSI_MARKACTIVE	(0x800)
#define SWM_WSI_MARKEMPTY	(0x1000)
#define SWM_WSI_PRINTNAMES	(0x2000)
#define SWM_WSI_NOINDEXES	(0x4000)
#define SWM_WSI_DEFAULT		(SWM_WSI_LISTCURRENT | SWM_WSI_LISTACTIVE |	\
    SWM_WSI_MARKCURRENT | SWM_WSI_PRINTNAMES)

#define SWM_WM_CLASS_INSTANCE	"spectrwm"
#define SWM_WM_CLASS_BAR	"panel"

#define SWM_CONF_DEFAULT	(0)
#define SWM_CONF_KEYMAPPING	(1)

#ifndef SWM_LIB
#define SWM_LIB			"/usr/local/lib/libswmhack.so"
#endif

char			**start_argv;
xcb_atom_t		a_state;
xcb_atom_t		a_prot;
xcb_atom_t		a_delete;
xcb_atom_t		a_net_frame_extents;
xcb_atom_t		a_net_wm_check;
xcb_atom_t		a_net_supported;
xcb_atom_t		a_takefocus;
xcb_atom_t		a_utf8_string;
xcb_atom_t		a_swm_ws;
volatile sig_atomic_t   running = 1;
volatile sig_atomic_t   restart_wm = 0;
xcb_timestamp_t		last_event_time = 0;
int			outputs = 0;
bool			randr_support = false;
int			randr_eventbase;
unsigned int		numlockmask = 0;
bool			xinput2_support = false;

Display			*display;
xcb_connection_t	*conn;
xcb_key_symbols_t	*syms;

int			boundary_width = 50;
bool			cycle_empty = false;
bool			cycle_visible = false;
int			term_width = 0;
int			font_adjusted = 0;
uint16_t		mod_key = MODKEY;
bool			warp_focus = false;
bool			warp_pointer = false;
bool			workspace_clamp = false;

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
TAILQ_HEAD(search_winlist, search_window) search_wl =
    TAILQ_HEAD_INITIALIZER(search_wl);

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
#define SWM_BAR_MAX		(1024)
#define SWM_BAR_JUSTIFY_LEFT	(0)
#define SWM_BAR_JUSTIFY_CENTER	(1)
#define SWM_BAR_JUSTIFY_RIGHT	(2)
#define SWM_BAR_OFFSET		(4)
#define SWM_BAR_FONTS		"-*-terminus-medium-*-*-*-12-*-*-*-*-*-*-*,"	\
				"-*-profont-*-*-*-*-12-*-*-*-*-*-*-*,"		\
				"-*-times-medium-r-*-*-12-*-*-*-*-*-*-*,"	\
				"-misc-fixed-medium-r-*-*-12-*-*-*-*-*-*-*,"	\
				"-*-*-*-r-*-*-*-*-*-*-*-*-*-*"

#define SWM_BAR_MAX_FONTS	(10)
#define SWM_BAR_MAX_COLORS	(10)

#ifdef X_HAVE_UTF8_STRING
#define DRAWSTRING(x...)	Xutf8DrawString(x)
#define TEXTEXTENTS(x...)	Xutf8TextExtents(x)
#else
#define DRAWSTRING(x...)	XmbDrawString(x)
#define TEXTEXTENTS(x...)	XmbTextExtents(x)
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
char		*bar_format = NULL;
bool		 bar_action_expand = false;
bool		 stack_enabled = true;
bool		 clock_enabled = true;
bool		 iconic_enabled = false;
bool		 maximize_hide_bar = false;
bool		 urgent_enabled = false;
bool		 urgent_collapse = false;
char		*clock_format = NULL;
bool		 window_class_enabled = false;
bool		 window_instance_enabled = false;
bool		 window_name_enabled = false;
uint32_t	 workspace_indicator = SWM_WSI_DEFAULT;
int		 focus_mode = SWM_FOCUS_DEFAULT;
int		 focus_close = SWM_STACK_BELOW;
bool		 focus_close_wrap = true;
int		 focus_default = SWM_STACK_TOP;
int		 spawn_position = SWM_STACK_TOP;
bool		 disable_border = false;
bool		 disable_border_always = false;
int		 border_width = 1;
int		 region_padding = 0;
int		 tile_gap = 0;
bool		 verbose_layout = false;
#ifdef SWM_DEBUG
bool		 debug_enabled;
time_t		 time_started;
#endif
pid_t		 bar_pid;
XFontSet	 bar_fs = NULL;
XFontSetExtents	*bar_fs_extents;
char		*bar_fontnames[SWM_BAR_MAX_FONTS];
XftFont		*bar_xftfonts[SWM_BAR_MAX_FONTS + 1];
int		num_xftfonts;
char		*bar_fontname_pua;
int		font_pua_index;
bool		 bar_font_legacy = true;
char		*bar_fonts = NULL;
XftColor	bar_fg_colors[SWM_BAR_MAX_COLORS];
int		num_fg_colors = 1;
int		num_bg_colors = 1;
XftColor	search_font_color;
char		*startup_exception = NULL;
unsigned int	 nr_exceptions = 0;
char		*workspace_mark_current = NULL;
char		*workspace_mark_urgent = NULL;
char		*workspace_mark_active = NULL;
char		*workspace_mark_empty = NULL;
char		*stack_mark_max = NULL;
char		*stack_mark_vertical = NULL;
char		*stack_mark_vertical_flip = NULL;
char		*stack_mark_horizontal = NULL;
char		*stack_mark_horizontal_flip = NULL;

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
	struct swm_region	*r;	/* Associated region. */
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

enum {
	SWM_WIN_STATE_REPARENTING,
	SWM_WIN_STATE_REPARENTED,
	SWM_WIN_STATE_UNPARENTING,
	SWM_WIN_STATE_UNPARENTED,
};

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	TAILQ_ENTRY(ws_win)	stack_entry;
	xcb_window_t		id;
	xcb_window_t		frame;
	xcb_window_t		transient;
	xcb_visualid_t		visual;
	struct ws_win		*focus_redirect;/* focus on transient */
	struct swm_geometry	g;		/* current geometry */
	struct swm_geometry	g_prev;		/* prev configured geometry */
	struct swm_geometry	g_float;	/* region coordinates */
	bool			g_floatvalid;	/* g_float geometry validity */
	bool			mapped;
	uint8_t			state;
	bool			bordered;
	uint32_t		ewmh_flags;
	int			font_size_boundary[SWM_MAX_FONT_STEPS];
	int			font_steps;
	int			last_inc;
	bool			can_delete;
	bool			take_focus;
	uint32_t		quirks;
	struct workspace	*ws;	/* always valid */
	struct swm_screen	*s;	/* always valid, never changes */
	xcb_size_hints_t	sh;
	xcb_icccm_get_wm_class_reply_t	ch;
	xcb_icccm_wm_hints_t	hints;
#ifdef SWM_DEBUG
	xcb_window_t		debug;
#endif
};
TAILQ_HEAD(ws_win_list, ws_win);
TAILQ_HEAD(ws_win_stack, ws_win);

/* pid goo */
struct pid_e {
	TAILQ_ENTRY(pid_e)	entry;
	pid_t			pid;
	int			ws;
};
TAILQ_HEAD(pid_list, pid_e) pidlist = TAILQ_HEAD_INITIALIZER(pidlist);

/* layout handlers */
void	stack(struct swm_region *);
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
#define SWM_L_FOCUSPREV		(1 << 0)
#define SWM_L_MAPONFOCUS	(1 << 1)
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
	struct ws_win		*focus_raise;	/* may be NULL */
	struct swm_region	*r;		/* may be NULL */
	struct swm_region	*old_r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */
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
	SWM_S_COLOR_BAR1,
	SWM_S_COLOR_BAR2,
	SWM_S_COLOR_BAR3,
	SWM_S_COLOR_BAR4,
	SWM_S_COLOR_BAR5,
	SWM_S_COLOR_BAR6,
	SWM_S_COLOR_BAR7,
	SWM_S_COLOR_BAR8,
	SWM_S_COLOR_BAR9,
	SWM_S_COLOR_BAR_SELECTED,
	SWM_S_COLOR_BAR_BORDER,
	SWM_S_COLOR_BAR_BORDER_UNFOCUS,
	SWM_S_COLOR_BAR_FONT,
	SWM_S_COLOR_BAR_FONT1,
	SWM_S_COLOR_BAR_FONT2,
	SWM_S_COLOR_BAR_FONT3,
	SWM_S_COLOR_BAR_FONT4,
	SWM_S_COLOR_BAR_FONT5,
	SWM_S_COLOR_BAR_FONT6,
	SWM_S_COLOR_BAR_FONT7,
	SWM_S_COLOR_BAR_FONT8,
	SWM_S_COLOR_BAR_FONT9,
	SWM_S_COLOR_BAR_FONT_SELECTED,
	SWM_S_COLOR_FOCUS,
	SWM_S_COLOR_FOCUS_MAXIMIZED,
	SWM_S_COLOR_UNFOCUS,
	SWM_S_COLOR_UNFOCUS_MAXIMIZED,
	SWM_S_COLOR_MAX
};

/* physical screen mapping */
#define SWM_WS_MAX		(22)	/* hard limit */
int		workspace_limit = 10;	/* soft limit */

#define SWM_RATE_DEFAULT	(60)	/* Default for swm_screen. */

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

	uint8_t			depth;
	xcb_timestamp_t		rate; /* Max updates/sec for move and resize */
	xcb_visualid_t		visual;
	Visual			*xvisual; /* Needed for Xft. */
	xcb_colormap_t		colormap;
	xcb_gcontext_t		gc;
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
#define SWM_ARG_ID_STACKBALANCE	(32)
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
#define SWM_ARG_ID_CYCLE_LAYOUT	(60)
#define SWM_ARG_ID_LAYOUT_VERTICAL	(61)
#define SWM_ARG_ID_LAYOUT_HORIZONTAL	(62)
#define SWM_ARG_ID_LAYOUT_MAX	(63)
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
#define SWM_ARG_ID_BAR_TOGGLE	(110)
#define SWM_ARG_ID_BAR_TOGGLE_WS	(111)
#define SWM_ARG_ID_CYCLERG_MOVE_UP	(112)
#define SWM_ARG_ID_CYCLERG_MOVE_DOWN	(113)
#define SWM_ARG_ID_WS_EMPTY	(120)
#define SWM_ARG_ID_WS_EMPTY_MOVE	(121)
#define SWM_ARG_ID_RESTARTOFDAY	(130)
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
	int			ws;	 /* Initial workspace. */
#define SWM_Q_FLOAT		(1 << 0) /* Float this window. */
#define SWM_Q_TRANSSZ		(1 << 1) /* Transient window size too small. */
#define SWM_Q_ANYWHERE		(1 << 2) /* Don't position this window */
#define SWM_Q_XTERM_FONTADJ	(1 << 3) /* Adjust xterm fonts when resizing. */
#define SWM_Q_FULLSCREEN	(1 << 4) /* Remove border when fullscreen. */
#define SWM_Q_FOCUSPREV		(1 << 5) /* Focus on caller. */
#define SWM_Q_NOFOCUSONMAP	(1 << 6) /* Don't focus on window when mapped.*/
#define SWM_Q_FOCUSONMAP_SINGLE	(1 << 7) /* Only focus if single win of type. */
#define SWM_Q_OBEYAPPFOCUSREQ	(1 << 8) /* Focus when applications ask. */
#define SWM_Q_IGNOREPID		(1 << 9) /* Ignore PID when determining ws. */
#define SWM_Q_IGNORESPAWNWS	(1 << 10)/* Ignore _SWM_WS when managing win. */
#define SWM_Q_NOFOCUSCYCLE	(1 << 11)/* Remove from normal focus cycle. */
#define SWM_Q_MINIMALBORDER	(1 << 12)/* No border when floating/unfocused.*/
};
TAILQ_HEAD(quirk_list, quirk) quirks = TAILQ_HEAD_INITIALIZER(quirks);

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
	_NET_REQUEST_FRAME_EXTENTS,
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
    {"_NET_REQUEST_FRAME_EXTENTS", XCB_ATOM_NONE},
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

#define SWM_TEXTFRAGS_MAX		(SWM_BAR_MAX/4)
struct text_fragment {
	char 			*text;
	int 			length;
	int 			font;
	int 			width;
	int 			fg;
	int 			bg;
};

/* bar section */
struct bar_section {
	char			fmtrep[SWM_BAR_MAX * 2];
	char			fmtsplit[SWM_BAR_MAX * 2];
	struct text_fragment	frag[SWM_TEXTFRAGS_MAX];

	bool			fit_to_text;
	int			justify;
	int			weight;
	int			start;
	int 			width;
	int			text_start;
	int 			text_width;
	int 			nfrags;

	/* Needed for legacy font */
	int			height;
	int			ypos;
};

struct bar_section	*bsect = NULL;
int			maxsect = 0;
int			numsect;

#define SWM_SPAWN_OPTIONAL		0x1

/* spawn */
struct spawn_prog {
	TAILQ_ENTRY(spawn_prog)	entry;
	char			*name;
	int			argc;
	char			**argv;
	int			flags;
};
TAILQ_HEAD(spawn_list, spawn_prog) spawns = TAILQ_HEAD_INITIALIZER(spawns);

enum {
	FN_F_NOREPLAY = 0x1,
};

/* User callable function IDs. */
enum actionid {
	FN_BAR_TOGGLE,
	FN_BAR_TOGGLE_WS,
	FN_BUTTON2,
	FN_CYCLE_LAYOUT,
	FN_FLIP_LAYOUT,
	FN_FLOAT_TOGGLE,
	FN_FOCUS,
	FN_FOCUS_MAIN,
	FN_FOCUS_NEXT,
	FN_FOCUS_PREV,
	FN_FOCUS_URGENT,
	FN_FULLSCREEN_TOGGLE,
	FN_MAXIMIZE_TOGGLE,
	FN_HEIGHT_GROW,
	FN_HEIGHT_SHRINK,
	FN_ICONIFY,
	FN_LAYOUT_VERTICAL,
	FN_LAYOUT_HORIZONTAL,
	FN_LAYOUT_MAX,
	FN_MASTER_SHRINK,
	FN_MASTER_GROW,
	FN_MASTER_ADD,
	FN_MASTER_DEL,
	FN_MOVE,
	FN_MOVE_DOWN,
	FN_MOVE_LEFT,
	FN_MOVE_RIGHT,
	FN_MOVE_UP,
	FN_MVRG_1,
	FN_MVRG_2,
	FN_MVRG_3,
	FN_MVRG_4,
	FN_MVRG_5,
	FN_MVRG_6,
	FN_MVRG_7,
	FN_MVRG_8,
	FN_MVRG_9,
	KF_MVRG_NEXT,
	KF_MVRG_PREV,
	FN_MVWS_1,
	FN_MVWS_2,
	FN_MVWS_3,
	FN_MVWS_4,
	FN_MVWS_5,
	FN_MVWS_6,
	FN_MVWS_7,
	FN_MVWS_8,
	FN_MVWS_9,
	FN_MVWS_10,
	FN_MVWS_11,
	FN_MVWS_12,
	FN_MVWS_13,
	FN_MVWS_14,
	FN_MVWS_15,
	FN_MVWS_16,
	FN_MVWS_17,
	FN_MVWS_18,
	FN_MVWS_19,
	FN_MVWS_20,
	FN_MVWS_21,
	FN_MVWS_22,
	FN_NAME_WORKSPACE,
	FN_QUIT,
	FN_RAISE,
	FN_RAISE_TOGGLE,
	FN_RESIZE,
	FN_RESIZE_CENTERED,
	FN_RESTART,
	FN_RESTART_OF_DAY,
	FN_RG_1,
	FN_RG_2,
	FN_RG_3,
	FN_RG_4,
	FN_RG_5,
	FN_RG_6,
	FN_RG_7,
	FN_RG_8,
	FN_RG_9,
	FN_RG_MOVE_NEXT,
	FN_RG_MOVE_PREV,
	FN_RG_NEXT,
	FN_RG_PREV,
	FN_SCREEN_NEXT,
	FN_SCREEN_PREV,
	FN_SEARCH_WIN,
	FN_SEARCH_WORKSPACE,
	FN_SPAWN_CUSTOM,
	FN_STACK_BALANCE,
	FN_STACK_INC,
	FN_STACK_DEC,
	FN_STACK_RESET,
	FN_SWAP_MAIN,
	FN_SWAP_NEXT,
	FN_SWAP_PREV,
	FN_UNICONIFY,
	FN_VERSION,
	FN_WIDTH_GROW,
	FN_WIDTH_SHRINK,
	FN_WIND_DEL,
	FN_WIND_KILL,
	FN_WS_1,
	FN_WS_2,
	FN_WS_3,
	FN_WS_4,
	FN_WS_5,
	FN_WS_6,
	FN_WS_7,
	FN_WS_8,
	FN_WS_9,
	FN_WS_10,
	FN_WS_11,
	FN_WS_12,
	FN_WS_13,
	FN_WS_14,
	FN_WS_15,
	FN_WS_16,
	FN_WS_17,
	FN_WS_18,
	FN_WS_19,
	FN_WS_20,
	FN_WS_21,
	FN_WS_22,
	FN_WS_EMPTY,
	FN_WS_EMPTY_MOVE,
	FN_WS_NEXT,
	FN_WS_NEXT_ALL,
	FN_WS_NEXT_MOVE,
	FN_WS_PREV,
	FN_WS_PREV_ALL,
	FN_WS_PREV_MOVE,
	FN_WS_PRIOR,
	/* SWM_DEBUG actions MUST be here: */
	FN_DEBUG_TOGGLE,
	FN_DUMPWINS,
	/* ALWAYS last: */
	FN_INVALID
};

enum binding_type {
	KEYBIND,
	BTNBIND
};

enum {
	BINDING_F_REPLAY = 0x1,
};

struct binding {
	RB_ENTRY(binding)	entry;
	uint16_t		mod;		/* Modifier Mask. */
	enum binding_type	type;		/* Key or Button. */
	uint32_t		value;		/* KeySym or Button Index. */
	enum actionid		action;		/* Action Identifier. */
	uint32_t		flags;
	char			*spawn_name;
};
RB_HEAD(binding_tree, binding) bindings = RB_INITIALIZER(&bindings);

/* function prototypes */
void	 adjust_font(struct ws_win *);
char	*argsep(char **);
void	 bar_cleanup(struct swm_region *);
void	 bar_extra_setup(void);
void	 bar_extra_stop(void);
int	 bar_extra_update(void);
void	 bar_fmt(const char *, char *, struct swm_region *, size_t);
void	 bar_fmt_expand(char *, size_t);
void     bar_parse_markup(struct bar_section *);
void	 bar_draw(struct swm_bar *);
void	 bar_print(struct swm_region *, const char *);
void	 bar_print_legacy(struct swm_region *, const char *);
void	 bar_print_layout(struct swm_region *);
void	 bar_split_format(char *);
void	 bar_strlcat_esc(char *, char *, size_t);
void	 bar_replace(char *, char *, struct swm_region *, size_t);
void	 bar_replace_pad(char *, int *, size_t);
void	 bar_replace_action(char *, char *, struct swm_region *, size_t);
char	*bar_replace_seq(char *, char *, struct swm_region *, size_t *, size_t);
void	 bar_setup(struct swm_region *);
void	 bar_toggle(struct binding *, struct swm_region *, union arg *);
void	 bar_urgent(char *, size_t);
void	 bar_window_class(char *, size_t, struct swm_region *);
void	 bar_window_class_instance(char *, size_t, struct swm_region *);
void	 bar_window_float(char *, size_t, struct swm_region *);
void	 bar_window_instance(char *, size_t, struct swm_region *);
void	 bar_window_name(char *, size_t, struct swm_region *);
void	 bar_window_state(char *, size_t, struct swm_region *);
void	 bar_workspace_indicator(char *, size_t, struct swm_region *);
void	 bar_workspace_name(char *, size_t, struct swm_region *);
int	 binding_cmp(struct binding *, struct binding *);
void	 binding_insert(uint16_t, enum binding_type, uint32_t, enum actionid,
	     uint32_t, const char *);
struct binding	*binding_lookup(uint16_t, enum binding_type, uint32_t);
void	 binding_remove(struct binding *);
void	 buttonpress(xcb_button_press_event_t *);
void	 buttonrelease(xcb_button_release_event_t *);
void	 center_pointer(struct swm_region *);
const struct xcb_setup_t	*get_setup(void);
void	 clear_bindings(void);
void	 clear_keybindings(void);
int	 clear_maximized(struct workspace *);
void	 clear_quirks(void);
void	 clear_spawns(void);
void	 clientmessage(xcb_client_message_event_t *);
void	 client_msg(struct ws_win *, xcb_atom_t, xcb_timestamp_t);
int	 conf_load(const char *, int);
void	 configurenotify(xcb_configure_notify_event_t *);
void	 configurerequest(xcb_configure_request_event_t *);
void	 config_win(struct ws_win *, xcb_configure_request_event_t *);
void	 constrain_window(struct ws_win *, struct swm_geometry *, int *);
int	 count_win(struct workspace *, uint32_t);
void	 cursors_cleanup(void);
void	 cursors_load(void);
void	 cyclerg(struct binding *, struct swm_region *, union arg *);
void	 cyclews(struct binding *, struct swm_region *, union arg *);
#ifdef SWM_DEBUG
void	 debug_refresh(struct ws_win *);
#endif
void	 debug_toggle(struct binding *, struct swm_region *, union arg *);
void	 destroynotify(xcb_destroy_notify_event_t *);
void	 dumpwins(struct binding *, struct swm_region *, union arg *);
void	 emptyws(struct binding *, struct swm_region *, union arg *);
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
struct swm_bar	*find_bar(xcb_window_t);
struct ws_win	*find_main_window(struct ws_win *);
struct pid_e	*find_pid(pid_t);
struct swm_region	*find_region(xcb_window_t);
struct swm_screen	*find_screen(xcb_window_t);
struct ws_win	*find_window(xcb_window_t);
void	 floating_toggle(struct binding *, struct swm_region *, union arg *);
void	 focus(struct binding *, struct swm_region *, union arg *);
void	 focus_flush(void);
void	 focus_pointer(struct binding *, struct swm_region *, union arg *);
void	 focus_region(struct swm_region *);
void	 focus_win(struct ws_win *);
void	 focus_win_input(struct ws_win *, bool);
void	 focusin(xcb_focus_in_event_t *);
#ifdef SWM_DEBUG
void	 focusout(xcb_focus_out_event_t *);
#endif
void	 focusrg(struct binding *, struct swm_region *, union arg *);
void	 fontset_init(void);
void	 free_window(struct ws_win *);
void	 fullscreen_toggle(struct binding *, struct swm_region *, union arg *);
xcb_atom_t get_atom_from_string(const char *);
#ifdef SWM_DEBUG
char	*get_atom_name(xcb_atom_t);
#endif
xcb_keycode_t	 get_binding_keycode(struct binding *);
struct ws_win   *get_focus_magic(struct ws_win *);
struct ws_win   *get_focus_other(struct ws_win *);
#ifdef SWM_DEBUG
char	*get_mapping_notify_label(uint8_t);
#endif
xcb_generic_event_t	*get_next_event(bool);
#ifdef SWM_DEBUG
char	*get_notify_detail_label(uint8_t);
char	*get_notify_mode_label(uint8_t);
#endif
struct swm_region	*get_pointer_region(struct swm_screen *);
struct ws_win	*get_pointer_win(struct swm_screen *);
struct ws_win	*get_region_focus(struct swm_region *);
int	 get_region_index(struct swm_region *);
xcb_screen_t	*get_screen(int);
int	 get_screen_count(void);
#ifdef SWM_DEBUG
char	*get_source_type_label(uint32_t);
char	*get_stack_mode_name(uint8_t);
char	*get_state_mask_label(uint16_t);
#endif
int32_t	 get_swm_ws(xcb_window_t);
bool	 get_urgent(struct ws_win *);
#ifdef SWM_DEBUG
char	*get_win_input_model(struct ws_win *);
#endif
char	*get_win_name(xcb_window_t);
uint8_t	 get_win_state(xcb_window_t);
void	 get_wm_protocols(struct ws_win *);
#ifdef SWM_DEBUG
char	*get_wm_state_label(uint8_t);
#endif
int	 get_ws_idx(struct ws_win *);
void	 grab_windows(void);
void	 grabbuttons(void);
void	 grabkeys(void);
void	 iconify(struct binding *, struct swm_region *, union arg *);
bool	 isxlfd(char *);
bool     is_valid_markup(char *, size_t *);
bool	 keybindreleased(struct binding *, xcb_key_release_event_t *);
void	 keypress(xcb_key_press_event_t *);
void	 keyrelease(xcb_key_release_event_t *);
bool	 keyrepeating(xcb_key_release_event_t *);
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
void	 maximize_toggle(struct binding *, struct swm_region *, union arg *);
void	 motionnotify(xcb_motion_notify_event_t *);
void	 move(struct binding *, struct swm_region *, union arg *);
void	 move_win(struct ws_win *, struct binding *, int);
uint32_t name_to_pixel(struct swm_screen *, const char *);
void	 name_workspace(struct binding *, struct swm_region *, union arg *);
void	 new_region(struct swm_screen *, int, int, int, int);
int	 parse_rgb(const char *, uint16_t *, uint16_t *, uint16_t *);
int	 parsebinding(const char *, uint16_t *, enum binding_type *, uint32_t *,
	     uint32_t *, char **);
int	 parsequirks(const char *, uint32_t *, int *, char **);
int	 parse_workspace_indicator(const char *, uint32_t *, char **);
bool	 pointer_follow(struct swm_screen *);
void	 pressbutton(struct binding *, struct swm_region *, union arg *);
void	 priorws(struct binding *, struct swm_region *, union arg *);
#ifdef SWM_DEBUG
void	 print_win_geom(xcb_window_t);
#endif
void	 propertynotify(xcb_property_notify_event_t *);
void	 put_back_event(xcb_generic_event_t *);
void	 quirk_free(struct quirk *);
void	 quirk_insert(const char *, const char *, const char *, uint32_t, int);
void	 quirk_remove(struct quirk *);
void	 quirk_replace(struct quirk *, const char *, const char *, const char *,
	     uint32_t, int);
void	 quit(struct binding *, struct swm_region *, union arg *);
void	 raise_focus(struct binding *, struct swm_region *, union arg *);
void	 raise_toggle(struct binding *, struct swm_region *, union arg *);
void	 raise_window(struct ws_win *);
void	 raise_window_related(struct ws_win *);
void	 region_containment(struct ws_win *, struct swm_region *, int);
struct swm_region	*region_under(struct swm_screen *, int, int);
void	 regionize(struct ws_win *, int, int);
int	 reparent_window(struct ws_win *);
void	 reparentnotify(xcb_reparent_notify_event_t *);
void	 resize(struct binding *, struct swm_region *, union arg *);
void	 resize_win(struct ws_win *, struct binding *, int);
void	 update_stacking(struct swm_screen *);
void	 restart(struct binding *, struct swm_region *, union arg *);
struct swm_region	*root_to_region(xcb_window_t, int);
void	 screenchange(xcb_randr_screen_change_notify_event_t *);
void	 scan_config(void);
void	 scan_randr(int);
void	 search_do_resp(void);
void	 search_resp_name_workspace(const char *, size_t);
void	 search_resp_search_window(const char *);
void	 search_resp_search_workspace(const char *);
void	 search_resp_uniconify(const char *, size_t);
void	 search_win(struct binding *, struct swm_region *, union arg *);
void	 search_win_cleanup(void);
void	 search_workspace(struct binding *, struct swm_region *, union arg *);
void	 send_to_rg(struct binding *, struct swm_region *, union arg *);
void	 send_to_rg_relative(struct binding *, struct swm_region *, union arg *);
void	 send_to_ws(struct binding *, struct swm_region *, union arg *);
void	 set_region(struct swm_region *);
int	 setautorun(const char *, const char *, int, char **);
void	 setbinding(uint16_t, enum binding_type, uint32_t, enum actionid,
	     uint32_t, const char *);
int	 setconfbinding(const char *, const char *, int, char **);
int	 setconfcolor(const char *, const char *, int, char **);
int	 setconfcolorlist(const char *, const char *, int, char **);
int	 setconfmodkey(const char *, const char *, int, char **);
int	 setconfquirk(const char *, const char *, int, char **);
int	 setconfregion(const char *, const char *, int, char **);
int	 setconfspawn(const char *, const char *, int, char **);
int	 setconfvalue(const char *, const char *, int, char **);
int	 setkeymapping(const char *, const char *, int, char **);
int	 setlayout(const char *, const char *, int, char **);
void	 setquirk(const char *, const char *, const char *, uint32_t, int);
void	 setscreencolor(const char *, int, int);
void	 setspawn(const char *, const char *, int);
void	 setup_btnbindings(void);
void	 setup_ewmh(void);
void	 setup_extensions(void);
void	 setup_globals(void);
void	 setup_keybindings(void);
void	 setup_quirks(void);
void	 setup_screens(void);
void	 setup_spawn(void);
void	 set_focus_redirect(struct ws_win *);
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
void	 stack_config(struct binding *, struct swm_region *, union arg *);
void	 stack_master(struct workspace *, struct swm_geometry *, int, bool);
void	 store_float_geom(struct ws_win *);
char	*strdupsafe(const char *);
void	 swapwin(struct binding *, struct swm_region *, union arg *);
void	 switchlayout(struct binding *, struct swm_region *, union arg *);
void	 switchws(struct binding *, struct swm_region *, union arg *);
void	 teardown_ewmh(void);
void	 unescape_selector(char *);
char	*unescape_value(const char *);
void	 unfocus_win(struct ws_win *);
void	 uniconify(struct binding *, struct swm_region *, union arg *);
void	 unmanage_window(struct ws_win *);
void	 unmap_all(void);
void	 unmap_window(struct ws_win *);
void	 unmap_workspace(struct workspace *);
void	 unmapnotify(xcb_unmap_notify_event_t *);
void	 unparent_window(struct ws_win *);
void	 update_floater(struct ws_win *);
void	 update_modkey(uint16_t);
void	 update_win_stacking(struct ws_win *);
void	 update_window(struct ws_win *);
void	 draw_frame(struct ws_win *);
void	 update_wm_state(struct  ws_win *win);
void	 updatenumlockmask(void);
static void	 usage(void);
void	 validate_spawns(void);
int	 validate_win(struct ws_win *);
int	 validate_ws(struct workspace *);
void	 version(struct binding *, struct swm_region *, union arg *);
void	 win_to_ws(struct ws_win *, int, uint32_t);
pid_t	 window_get_pid(xcb_window_t);
void	 wkill(struct binding *, struct swm_region *, union arg *);
void	 update_ws_stack(struct workspace *);
void	 xft_init(struct swm_region *);
void	 _add_startup_exception(const char *, va_list);
void	 add_startup_exception(const char *, ...);

RB_PROTOTYPE(binding_tree, binding, entry, binding_cmp);
#ifndef __clang_analyzer__ /* Suppress false warnings. */
RB_GENERATE(binding_tree, binding, entry, binding_cmp);
#endif

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

	return (result);
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

const struct xcb_setup_t *
get_setup(void)
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
	return (xcb_get_setup(conn));
}

xcb_screen_t *
get_screen(int screen)
{
	const xcb_setup_t	*r;
	xcb_screen_iterator_t	iter;

	r = get_setup();
	iter = xcb_setup_roots_iterator(r);
	for (; iter.rem; --screen, xcb_screen_next(&iter))
		if (screen == 0)
			return (iter.data);

	return (NULL);
}

int
get_screen_count(void)
{
	return (xcb_setup_roots_length(get_setup()));
}

int
get_region_index(struct swm_region *r)
{
	struct swm_region	*rr;
	int			 ridx = 0;

	if (r == NULL)
		return (-1);

	TAILQ_FOREACH(rr, &r->s->rl, entry) {
		if (rr == r)
			break;
		++ridx;
	}

	if (rr == NULL)
		return (-1);

	return (ridx);
}

void
event_drain(uint8_t rt)
{
	xcb_generic_event_t	*evt;

	/* Ensure all pending requests have been processed before filtering. */
	xcb_aux_sync(conn);
	while ((evt = get_next_event(false))) {
		if (rt == 0 || XCB_EVENT_RESPONSE_TYPE(evt) != rt)
			event_handle(evt);

		free(evt);
	}
}

void
focus_flush(void)
{
	if (focus_mode == SWM_FOCUS_DEFAULT)
		event_drain(XCB_ENTER_NOTIFY);
	else
		event_drain(0);
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
	xcb_atom_t		action[SWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (win == NULL)
		return;

	action[n++] = ewmh[_NET_WM_ACTION_CLOSE].atom;

	if (ABOVE(win)) {
		action[n++] = ewmh[_NET_WM_ACTION_MOVE].atom;
		action[n++] = ewmh[_NET_WM_ACTION_RESIZE].atom;
		action[n++] = ewmh[_NET_WM_ACTION_ABOVE].atom;
	}

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
	    ewmh[_NET_WM_ALLOWED_ACTIONS].atom, XCB_ATOM_ATOM, 32, 1, action);
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
	DNPRINTF(SWM_D_PROP, "win %#x, state: %s, " "action: %ld\n",
	    WINID(win), name, action);
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
	DNPRINTF(SWM_D_PROP, "done\n");
}

void
ewmh_apply_flags(struct ws_win *win, uint32_t pending)
{
	struct workspace	*ws;
	uint32_t		changed;
	bool			dofocus;

	changed = win->ewmh_flags ^ pending;
	if (changed == 0)
		return;

	DNPRINTF(SWM_D_PROP, "pending: %u\n", pending);

	win->ewmh_flags = pending;
	ws = win->ws;
	dofocus = !(pointer_follow(win->s));

	if (changed & EWMH_F_HIDDEN) {
		if (ICONIC(win)) {
			if (dofocus)
				ws->focus_pending =
				    get_focus_magic(get_focus_other(win));

			unfocus_win(win);
			unmap_window(win);
		} else {
			/* Reload floating geometry in case region changed. */
			if (FLOATING(win))
				load_float_geom(win);

			/* The window is no longer iconic, prepare focus. */
			if (dofocus)
				ws->focus_pending = get_focus_magic(win);
			raise_window(win);
			update_win_stacking(win);
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
			update_win_stacking(win);
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
			if (dofocus &&
			    ws->cur_layout != &layouts[SWM_MAX_STACK]) {
				if (WS_FOCUSED(ws))
					focus_win(win);
				else
					ws->focus_pending = win;
			}
		}

		draw_frame(win);
		raise_window_related(win);
		update_stacking(win->s);
	}

	if (changed & EWMH_F_FULLSCREEN) {
		if (FULLSCREEN(win)) {
			if (dofocus) {
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
		update_win_stacking(win);
	}

	DNPRINTF(SWM_D_PROP, "done\n");
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
dumpwins(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct workspace			*ws;
	struct ws_win				*w;
	uint32_t				state;
	xcb_get_window_attributes_cookie_t	c;
	xcb_get_window_attributes_reply_t	*wa;
	int					i;

	/* suppress unused warning since var is needed */
	(void)bp;
	(void)args;

	if (r->ws == NULL) {
		DPRINTF("dumpwins: invalid workspace\n");
		return;
	}

	DPRINTF("=== focus information ===\n");
	for (i = 0; i < workspace_limit; ++i) {
		ws = &r->s->ws[i];
		DPRINTF("ws %02d focus: 0x%08x, focus_pending: 0x%08x, "
		    "focus_prev: 0x%08x, focus_raise: 0x%08x\n", i,
		    WINID(ws->focus), WINID(ws->focus_pending),
		    WINID(ws->focus_prev), WINID(ws->focus_raise));
	}

	DPRINTF("=== managed window list ws %02d ===\n", r->ws->idx);
	TAILQ_FOREACH(w, &r->ws->winlist, entry) {
		state = get_win_state(w->id);
		c = xcb_get_window_attributes(conn, w->id);
		wa = xcb_get_window_attributes_reply(conn, c, NULL);
		if (wa) {
			DPRINTF("win %#x (%#x), map_state: %d, state: %u, "
			    "transient: %#x\n", w->frame, w->id, wa->map_state,
			    state, w->transient);
			free(wa);
		} else
			DPRINTF("win %#x, failed xcb_get_window_attributes\n",
			    w->id);
	}

	DPRINTF("=== stacking order (top down) === \n");
	TAILQ_FOREACH(w, &r->ws->stack, stack_entry) {
		DPRINTF("win %#x (%#x), fs: %s, maximized: %s, above: %s, "
		    "iconic: %s\n", w->frame, w->id, YESNO(FULLSCREEN(w)),
		    YESNO(MAXIMIZED(w)), YESNO(ABOVE(w)), YESNO(ICONIC(w)));
	}

	DPRINTF("=================================\n");
}

void
debug_toggle(struct binding *b, struct swm_region *r, union arg *s)
{
	struct ws_win		*win;
	int			num_screens, i, j;

	/* Suppress warnings. */
	(void)b;
	(void)r;
	(void)s;

	debug_enabled = !debug_enabled;
	DNPRINTF(SWM_D_MISC, "debug_enabled: %s\n", YESNO(debug_enabled));

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				debug_refresh(win);

	xcb_flush(conn);
}

void
debug_refresh(struct ws_win *win)
{
	struct ws_win		*w;
	XftDraw			*draw;
	XGlyphInfo		info;
	GC			l_draw;
	XGCValues		l_gcv;
	XRectangle		l_ibox, l_lbox = {0, 0, 0, 0};
	xcb_rectangle_t		rect;
	size_t			len;
	uint32_t		wc[4], mask, width, height, gcv[1];
	int			widx, sidx;
	char			*s;

	if (debug_enabled) {
		/* Create debug window if it doesn't exist. */
		if (win->debug == XCB_WINDOW_NONE) {
			win->debug = xcb_generate_id(conn);
			wc[0] = win->s->c[SWM_S_COLOR_BAR].pixel;
			wc[1] = win->s->c[SWM_S_COLOR_BAR_BORDER].pixel;
			wc[2] = win->s->colormap;

			xcb_create_window(conn, win->s->depth, win->debug,
			    win->frame, 0, 0, 10, 10, 1,
			    XCB_WINDOW_CLASS_INPUT_OUTPUT, win->s->visual,
			    XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
			    XCB_CW_COLORMAP, wc);

			xcb_map_window(conn, win->debug);
		}

		/* Determine workspace window list index. */
		widx = 0;
		TAILQ_FOREACH(w, &win->ws->winlist, entry) {
			++widx;
			if (w == win)
				break;
		}

		/* Determine stacking index (top down). */
		sidx = 0;
		TAILQ_FOREACH(w, &win->ws->stack, stack_entry) {
			++sidx;
			if (w == win)
				break;
		}

		if (asprintf(&s, "%#x f:%#x wl:%d s:%d visual: %#x "
		    "colormap: %#x im: %s", win->id, win->frame, widx, sidx,
		    win->s->visual, win->s->colormap,
		    get_win_input_model(win)) == -1)
			return;

		len = strlen(s);

		/* Update window to an appropriate dimension. */
		if (bar_font_legacy) {
			TEXTEXTENTS(bar_fs, s, len, &l_ibox, &l_lbox);
			width = l_lbox.width + 4;
			height = bar_fs_extents->max_logical_extent.height + 4;
		} else {
			XftTextExtentsUtf8(display, bar_xftfonts[0],
			    (FcChar8 *)s, len, &info);
			width = info.xOff + 4;
			height = bar_xftfonts[0]->height + 4;
		}

		mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
		    XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
		if (win->bordered)
			wc[0] = wc[1] = border_width;
		else
			wc[0] = wc[1] = 0;

		wc[2] = width;
		wc[3] = height;

		xcb_configure_window(conn, win->debug, mask, wc);

		/* Draw a filled rectangle to 'clear' window. */
		rect.x = 0;
		rect.y = 0;
		rect.width = width;
		rect.height = height;

		gcv[0] = win->s->c[SWM_S_COLOR_BAR].pixel;
		xcb_change_gc(conn, win->s->gc, XCB_GC_FOREGROUND, gcv);
		xcb_poly_fill_rectangle(conn, win->debug, win->s->gc, 1, &rect);

		/* Draw text. */
		if (bar_font_legacy) {
			l_gcv.graphics_exposures = 0;
			l_draw = XCreateGC(display, win->debug, 0, &l_gcv);

			XSetForeground(display, l_draw,
			    win->s->c[SWM_S_COLOR_BAR_FONT].pixel);

			DRAWSTRING(display, win->debug, bar_fs, l_draw, 2,
			    (bar_fs_extents->max_logical_extent.height -
			    l_lbox.height) / 2 - l_lbox.y, s, len);

			XFreeGC(display, l_draw);
		} else {
			draw = XftDrawCreate(display, win->debug,
			    win->s->xvisual, win->s->colormap);

			XftDrawStringUtf8(draw, &bar_fg_colors[0],
			    bar_xftfonts[0], 2, (bar_height +
			    bar_xftfonts[0]->height) / 2 -
			    bar_xftfonts[0]->descent, (FcChar8 *)s, len);

			XftDrawDestroy(draw);
		}

		free(s);
	} else if (win->debug != XCB_WINDOW_NONE) {
			xcb_destroy_window(conn, win->debug);
			win->debug = XCB_WINDOW_NONE;
	}
}
#else
void
dumpwins(struct binding *b, struct swm_region *r, union arg *s)
{
	(void)b;
	(void)r;
	(void)s;
}

void
debug_toggle(struct binding *b, struct swm_region *r, union arg *s)
{
	(void)b;
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

	DNPRINTF(SWM_D_MISC, "pid: %d\n", pid);

	if (pid == 0)
		return (NULL);

	TAILQ_FOREACH(p, &pidlist, entry) {
		if (p->pid == pid)
			return (p);
	}

	return (NULL);
}

uint32_t
name_to_pixel(struct swm_screen *s, const char *colorname)
{
	uint32_t			result = 0;
	char				cname[32] = "#";
	xcb_alloc_color_reply_t		*cr;
	xcb_alloc_named_color_reply_t	*nr;
	uint16_t			rr, gg, bb;

	/* color is in format rgb://rr/gg/bb */
	if (strncmp(colorname, "rgb:", 4) == 0) {
		if (parse_rgb(colorname, &rr, &gg, &bb) == -1)
			warnx("could not parse rgb %s", colorname);
		else {
			cr = xcb_alloc_color_reply(conn,
			    xcb_alloc_color(conn, s->colormap, rr, gg, bb),
			    NULL);
			if (cr) {
				result = cr->pixel;
				free(cr);
			} else
				warnx("color '%s' not found", colorname);
		}
	} else {
		nr = xcb_alloc_named_color_reply(conn,
		    xcb_alloc_named_color(conn, s->colormap, strlen(colorname),
		    colorname), NULL);
		if (nr == NULL) {
			strlcat(cname, colorname + 2, sizeof cname - 1);
			nr = xcb_alloc_named_color_reply(conn,
			    xcb_alloc_named_color(conn, s->colormap,
			    strlen(cname), cname), NULL);
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

	screens[i].c[c].pixel = name_to_pixel(&screens[i], val);
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
	strlcpy(ws->stacker, stack_mark_max, sizeof ws->stacker);
	if (ws->cur_layout->l_stack == vertical_stack)
		strlcpy(ws->stacker, ws->l_state.vertical_flip ?
		    stack_mark_vertical_flip : stack_mark_vertical,
		    sizeof ws->stacker);
	else if (ws->cur_layout->l_stack == horizontal_stack)
		strlcpy(ws->stacker, ws->l_state.horizontal_flip ?
		    stack_mark_horizontal_flip : stack_mark_horizontal,
		    sizeof ws->stacker);
}

void
socket_setnonblock(int fd)
{
	int			flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		err(1, "fcntl F_GETFL");
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
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
	TEXTEXTENTS(bar_fs, s, len, &ibox, &lbox);

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

	/* clear back buffer */
	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);

	gcv[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1, &rect);

	/* draw back buffer */
	gcvd.graphics_exposures = 0;
	draw = XCreateGC(display, r->bar->buffer, GCGraphicsExposures, &gcvd);
	XSetForeground(display, draw, r->s->c[SWM_S_COLOR_BAR_FONT].pixel);
	DRAWSTRING(display, r->bar->buffer, bar_fs, draw,
	    x, (bar_fs_extents->max_logical_extent.height - lbox.height) / 2 -
	    lbox.y, s, len);
	XFreeGC(display, draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->gc, 0, 0,
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

	XftTextExtentsUtf8(display, bar_xftfonts[0], (FcChar8 *)s, len, &info);

	switch (bar_justify) {
	case SWM_BAR_JUSTIFY_LEFT:
		x = SWM_BAR_OFFSET;
		break;
	case SWM_BAR_JUSTIFY_CENTER:
		x = (WIDTH(r) - info.xOff) / 2;
		break;
	case SWM_BAR_JUSTIFY_RIGHT:
		x = WIDTH(r) - info.xOff - SWM_BAR_OFFSET;
		break;
	}

	if (x < SWM_BAR_OFFSET)
		x = SWM_BAR_OFFSET;

	/* clear back buffer */
	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);

	gcv[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1, &rect);

	/* draw back buffer */
	draw = XftDrawCreate(display, r->bar->buffer, r->s->xvisual,
	    r->s->colormap);

	XftDrawStringUtf8(draw, &bar_fg_colors[0], bar_xftfonts[0], x,
	    (HEIGHT(r->bar) + bar_xftfonts[0]->height) / 2 -
	    bar_xftfonts[0]->descent, (FcChar8 *)s, len);

	XftDrawDestroy(draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->gc, 0, 0,
	    0, 0, WIDTH(r->bar), HEIGHT(r->bar));
}

void
bar_print_layout(struct swm_region *r)
{
	struct text_fragment	*frag;
	xcb_rectangle_t		rect;
	XftDraw			*xft_draw = NULL;
	GC			draw = 0;
	XGCValues		gcvd;
	uint32_t		gcv[1];
	int			xpos, i, j;
	int			bg, fg, fn, section_bg;
	int 			space, usage, weight;

	space =  WIDTH(r) - 2 * SWM_BAR_OFFSET;
	usage = 0;
	weight = 0;

	/* Parse markup sequences in each section  */
	/* For the legacy font, just setup one text fragment  */
	for (i = 0; i < numsect; i++) {
		bar_parse_markup(bsect + i);
		if (bsect[i].fit_to_text) {
			bsect[i].width = bsect[i].text_width + 2 *
			    SWM_BAR_OFFSET;
			usage += bsect[i].width;
		} else
			weight += bsect[i].weight;
	}

	/* Calculate width for each text justified section  */
	space -= usage;
	for (i = 0; i < numsect; i++)
		if (!bsect[i].fit_to_text && weight > 0)
			bsect[i].width = bsect[i].weight * space / weight;

	/* Calculate starting position of each section and text */
	xpos = 0;
	for (i = 0; i < numsect; i++) {
		bsect[i].start = xpos;
		if (bsect[i].fit_to_text)
			bsect[i].text_start = bsect[i].start + SWM_BAR_OFFSET;
		else {
			if (bsect[i].justify == SWM_BAR_JUSTIFY_LEFT)
				bsect[i].text_start = bsect[i].start +
				    SWM_BAR_OFFSET;
			else if (bsect[i].justify ==  SWM_BAR_JUSTIFY_RIGHT)
				bsect[i].text_start = bsect[i].start +
				    bsect[i].width -
				    bsect[i].text_width - SWM_BAR_OFFSET;
			else
				bsect[i].text_start = bsect[i].start +
				    (bsect[i].width - bsect[i].text_width) / 2;
		}

		/* Don't overflow text to the left */
		if (bsect[i].text_start < (bsect[i].start + SWM_BAR_OFFSET))
			bsect[i].text_start = bsect[i].start + SWM_BAR_OFFSET;

		xpos += bsect[i].width;
	}

	/* Create drawing context */
	if (bar_font_legacy) {
		gcvd.graphics_exposures = 0;
		draw = XCreateGC(display, r->bar->buffer, GCGraphicsExposures,
		    &gcvd);
	} else
		xft_draw = XftDrawCreate(display, r->bar->buffer, r->s->xvisual,
		    r->s->colormap);

	/* Paint entire bar with default background color */
	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);
	gcv[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1, &rect);

	/* Display the text for each section */
	for (i = 0; i < numsect; i++) {
		rect.x = bsect[i].start;
		rect.y = 0;
		rect.width = bsect[i].width;
		rect.height = HEIGHT(r->bar);

		/* No space to draw that section */
		if (rect.width < 1)
			continue;

		/* Paint background color of the section  */
		section_bg = bsect[i].frag[0].bg;
		gcv[0] = r->s->c[SWM_S_COLOR_BAR+section_bg].pixel;
		xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
		xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1,
		    &rect);

		/* No space to draw anything else */
		if (rect.width < SWM_BAR_OFFSET)
			continue;

		/* Draw the text fragments in the current section */
		xpos = bsect[i].text_start;
		for (j = 0; j < bsect[i].nfrags; j++) {
			frag = bsect[i].frag + j;
			fn = frag->font;
			fg = frag->fg;
			bg = frag->bg;

			/* Paint background color of the text fragment  */
			if (bg != section_bg) {
				rect.x = xpos;
				rect.width = frag->width;
				gcv[0] = r->s->c[SWM_S_COLOR_BAR+bg].pixel;
				xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND,
				    gcv);
				xcb_poly_fill_rectangle(conn, r->bar->buffer,
				    r->s->gc, 1, &rect);
			}

			/* Draw text  */
			if (bar_font_legacy) {
				XSetForeground(display, draw,
				    r->s->c[SWM_S_COLOR_BAR_FONT+fg].pixel);
				DRAWSTRING(display, r->bar->buffer, bar_fs,
				    draw, xpos,
				    (bar_fs_extents->max_logical_extent.height
				    - bsect[i].height) / 2 - bsect[i].ypos,
				    frag->text, frag->length);
			} else {
				XftDrawStringUtf8(xft_draw, &bar_fg_colors[fg],
				    bar_xftfonts[fn], xpos, (HEIGHT(r->bar) +
				    bar_xftfonts[fn]->height) / 2 -
				    bar_xftfonts[fn]->descent,
				    (FcChar8 *)frag->text, frag->length);
			}

			xpos += frag->width;
		}
	}

	if (bar_font_legacy)
		XFreeGC(display, draw);
	else
		XftDrawDestroy(xft_draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->gc, 0, 0, 0, 0,
	    WIDTH(r->bar), HEIGHT(r->bar));
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
		bar_strlcat_esc(s, r->ws->focus->ch.class_name, sz);
}

void
bar_window_instance(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;
	if (r->ws->focus->ch.instance_name != NULL)
		bar_strlcat_esc(s, r->ws->focus->ch.instance_name, sz);
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
	bar_strlcat_esc(s, title, sz);
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

	return (urgent);
}

void
bar_urgent(char *s, size_t sz)
{
	struct ws_win		*win;
	int			i, j, num_screens, urgent[SWM_WS_MAX];
	char			b[8];

	memset(&urgent, 0, sizeof(urgent));

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (get_urgent(win))
					urgent[j] = 1;

	for (i = 0; i < workspace_limit; i++) {
		if (urgent[i]) {
			snprintf(b, sizeof b, "%d ", i + 1);
			strlcat(s, b, sz);
		} else if (!urgent_collapse) {
			strlcat(s, "- ", sz);
		}
	}
	if (urgent_collapse && s[0])
		s[strlen(s) - 1] = 0;
}

void
bar_workspace_indicator(char *s, size_t sz, struct swm_region *r)
{
	struct ws_win		*w;
	struct workspace	*ws;
	int		 	 i, count = 0;
	char			 tmp[SWM_BAR_MAX], *mark;
	bool			 current, active, named, urgent, collapse;

	if (r == NULL)
		return;

	for (i = 0; i < workspace_limit; i++) {
		ws = &r->s->ws[i];

		current = (ws == r->ws);
		named = (ws->name != NULL);
		urgent = false;
		active = false;
		TAILQ_FOREACH(w, &ws->winlist, entry) {
			active = true;
			/* Only get urgent if needed. */
			if (!(workspace_indicator & SWM_WSI_LISTURGENT ||
			    workspace_indicator & SWM_WSI_MARKURGENT) ||
			    (urgent = get_urgent(w)))
				break;
		}

		collapse = !(workspace_indicator & SWM_WSI_MARKCURRENT ||
		    workspace_indicator & SWM_WSI_MARKURGENT);

		if (!(current && workspace_indicator & SWM_WSI_HIDECURRENT) &&
		    ((current && workspace_indicator & SWM_WSI_LISTCURRENT) ||
		    (active && workspace_indicator & SWM_WSI_LISTACTIVE) ||
		    (!active && workspace_indicator & SWM_WSI_LISTEMPTY) ||
		    (urgent && workspace_indicator & SWM_WSI_LISTURGENT) ||
		    (named && workspace_indicator & SWM_WSI_LISTNAMED))) {
			if (count > 0)
				strlcat(s, " ", sz);

			if (current &&
			    workspace_indicator & SWM_WSI_MARKCURRENT)
				mark = workspace_mark_current;
			else if (urgent &&
			    workspace_indicator & SWM_WSI_MARKURGENT)
				mark = workspace_mark_urgent;
			else if (active &&
			    workspace_indicator & SWM_WSI_MARKACTIVE)
				mark = workspace_mark_active;
			else if (!active &&
			    workspace_indicator & SWM_WSI_MARKEMPTY)
				mark = workspace_mark_empty;
			else if (!collapse)
				mark = " ";
			else
				mark = "";
			strlcat(s, mark, sz);

			if (named && workspace_indicator & SWM_WSI_PRINTNAMES) {
				snprintf(tmp, sizeof tmp, "%d:%s", ws->idx + 1,
				    ws->name);
				if (named &&
				    workspace_indicator & SWM_WSI_NOINDEXES)
					snprintf(tmp, sizeof tmp, "%s",
					    ws->name);
			} else if (workspace_indicator & SWM_WSI_NOINDEXES)
				snprintf(tmp, sizeof tmp, "%s", " ");
			else
				snprintf(tmp, sizeof tmp, "%d", ws->idx + 1);
			strlcat(s, tmp, sz);
			count++;
		}
	}
}

void
bar_workspace_name(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL)
		return;
	if (r->ws->name != NULL)
		bar_strlcat_esc(s, r->ws->name, sz);
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
		strlcat(fmtnew, (urgent_collapse ? "*+U*+4<" : "* +U*+4<"), sz);

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
	char			*ptr, *cur = fmt;
	char			tmp[SWM_BAR_MAX];
	int			limit, size, count;
	size_t			len;
	int			pre_padding = 0;
	int			post_padding = 0;
	int			padding_len = 0;

	/* Reset replace buffer. */
	bzero(tmp, sizeof tmp);

	cur++;
	/* determine if pre-padding is requested */
	if (*cur == '_') {
		pre_padding = 1;
		cur++;
	}

	/* get number, if any */
	size = 0;
	if (sscanf(cur, "%d%n", &limit, &size) != 1)
		limit = sizeof tmp - 1;
	if (limit <= 0 || limit >= (int)sizeof tmp)
		limit = sizeof tmp - 1;

	cur += size;

	/* determine if post padding is requested */
	if (*cur == '_') {
		post_padding = 1;
		cur++;
	}

	/* character sequence */
	switch (*cur) {
	case '+':
		strlcpy(tmp, "+", sizeof tmp);
		break;
	case '<':
		bar_replace_pad(tmp, &limit, sizeof tmp);
		break;
	case 'A':
		if (bar_action_expand)
			snprintf(tmp, sizeof tmp, "%s", bar_ext);
		else
			bar_strlcat_esc(tmp, bar_ext, sizeof tmp);
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
	case 'L':
		bar_workspace_indicator(tmp, sizeof tmp, r);
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
	case 'R':
	        snprintf(tmp, sizeof tmp, "%d", get_region_index(r) + 1);
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
		/* Unknown character sequence or EOL; copy as-is. */
		strlcpy(tmp, fmt, cur - fmt + 2);
		break;
	}

	len = strlen(tmp);

	/* calculate the padding lengths */
	padding_len = limit - (int)len;
	if (padding_len > 0) {
		limit = len;

		if (pre_padding)
			pre_padding = padding_len / (pre_padding +
			    post_padding);
		if (post_padding)
			post_padding = padding_len - pre_padding;
	} else {
		pre_padding = 0;
		post_padding = 0;
	}

	/* do pre padding */
	while (pre_padding-- > 0) {
		if (*offrep >= sz - 1)
			break;
		fmtrep[(*offrep)++] = ' ';
	}

	ptr = tmp;
	while (limit-- > 0) {
		if (*offrep >= sz - 1)
			break;
		fmtrep[(*offrep)++] = *ptr++;
	}

	/* do post padding */
	while (post_padding-- > 0) {
		if (*offrep >= sz - 1)
			break;
		fmtrep[(*offrep)++] = ' ';
	}

	if (*cur != '\0')
		cur++;

	return (cur);
}

void
bar_replace_action(char *fmt, char *fmtact, struct swm_region *r, size_t sz)
{
	size_t		off;
	char		*s;

	off = 0;
	while (*fmt != '\0') {
		if (*fmt != '+') {
			/* skip ordinary characters */
			if (off >= sz - 1)
				break;
			fmtact[off++] = *fmt++;
			continue;
		}

		/* Find the first character after the padding */
		s = fmt + 1;
		while ((*s == '_') || ((*s >= '0') && (*s <= '9')))
			s++;

		if (*s == 'A') {
			/* Replace the action script character sequence */
			fmt = bar_replace_seq(fmt, fmtact, r, &off, sz);
			if (off >= sz - 1)
				break;
		} else {
			/* Copy '+' and the next character */
			fmtact[off++] = *fmt++;
			if (*fmt != '\0')
				fmtact[off++] = *fmt++;
		}
	}

	fmtact[off] = '\0';
}

void
bar_strlcat_esc(char *dst, char *src, size_t sz)
{
	/* Find end of destination string */
	while (*dst != '\0' && sz != 0) {
		dst++;
		sz--;
	}

	/* Concat string and escape every '+' */
	while (*src != '\0' && sz > 1) {
		if ((*src == '+') && (sz > 2)) {
			*dst++ = '+';
			sz--;
		}
		*dst++ = *src++;
		sz--;
	}
	*dst = '\0';
}

void
bar_replace(char *fmt, char *fmtrep, struct swm_region *r, size_t sz)
{
	size_t		off;
	char		*s;

	off = 0;
	while (*fmt != '\0') {
		if (*fmt != '+') {
			/* skip ordinary characters */
			if (off >= sz - 1)
				break;
			fmtrep[off++] = *fmt++;
			continue;
		}

		/* Find the first character after the padding */
		s = fmt + 1;
		while ((*s == '_') || ((*s >= '0') && (*s <= '9')))
			s++;

		if ((bar_action_expand) && (*s == 'A')) {
			/* skip this character sequence */
			fmt = s + 1;
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
bar_split_format(char *format)
{
	char *src, *dst;
	int i = 0;

	/* Count the number of sections in format */
	numsect = 1;
	src = format;
	while (*src != '\0') {
		if ((*src == '+') && (*(src+1) == '+'))
			src++;
		else if ((*src == '+') && (*(src+1) == '|')) {
			if (src != format) numsect++;
			src++;
		}
		src++;
	}

	/* Allocate the data structures for the bar sections */
	if (numsect > maxsect) {
		free(bsect);
		if ((bsect = calloc(numsect, sizeof(struct bar_section)
		   )) == NULL)
			err(1, "bar_split_format: failed to calloc memory.");
		maxsect = numsect;
	}

	/* Defaults for first section */
	bsect[0].weight = 1;
	bsect[0].justify = bar_justify;

	/* split format into sections */
	src = format;
	dst = bsect[0].fmtsplit;
	while (*src != '\0') {
		if ((*src == '+') && (*(src+1) == '+')) {
			*dst++ = *src++;
			*dst++ = *src++;
		} else if ((*src == '+') && (*(src+1) == '|')) {
			if (src != format)
				i++;
			if (i == numsect)
				break;

			*dst = '\0';
			dst = bsect[i].fmtsplit;
			src += 2;

			/* Set weight and justification */
			bsect[i].weight = atoi(src);
			if (bsect[i].weight == 0)
				bsect[i].weight = 1;

			while ((*src >= '0') && (*src <= '9'))
				src++;

			bsect[i].fit_to_text = false;
			if (*src == 'T') {
				bsect[i].fit_to_text = true;
				src++;
			} else if (*src == 'L') {
				bsect[i].justify = SWM_BAR_JUSTIFY_LEFT;
				src++;
			} else if (*src == 'C') {
				bsect[i].justify = SWM_BAR_JUSTIFY_CENTER;
				src++;
			} else if (*src == 'R') {
				bsect[i].justify = SWM_BAR_JUSTIFY_RIGHT;
				src++;
			} else
				bsect[i].justify = bar_justify;
		} else
			*dst++ = *src++;
	}
	while (*src != '\0')
		*dst++ = *src++;

	*dst = '\0';
}

bool
is_valid_markup(char *f, size_t *size)
{
	char *s = f;
	int n;

	*size = 0;
	if (*s != '+')
		return false;
	s++;
	if (*s != '@')
		return false;
	s++;
	if ((*s == 'b') && (*(s+1) == 'g') && (*(s+2) == '=') && (*(s+3) >= '0')
	    && (*(s+3) <= '9') && (*(s+4) == ';')) {
		*size = 7;
		n = (*(s+3) - '0');
		if (n < num_bg_colors)
			return true;
		else
			return false;
	}

	if ((*s == 'f') && (*(s+1) == 'g') && (*(s+2) == '=') && (*(s+3) >= '0')
	    && (*(s+3) <= '9') && (*(s+4) == ';')) {
		*size = 7;
		n = (*(s+3) - '0');
		if (n < num_fg_colors)
			return true;
		else
			return false;
	}

	if ((*s == 'f') && (*(s+1) == 'n') && (*(s+2) == '=') && (*(s+3) >= '0')
	    && (*(s+3) <= '9') && (*(s+4) == ';')) {
		if (bar_font_legacy)
			return false;
		*size = 7;
		n = (*(s+3) - '0');
		if (n < num_xftfonts)
			return true;
		else
			return false;
	}

	if ((*s == 's') && (*(s+1) == 't') && (*(s+2) == 'p') &&
	    (*(s+3) == ';')) {
		*size = 6;
		return true;
	}

	return false;
}

void
bar_parse_markup(struct bar_section *sect)
{
	XRectangle		ibox, lbox;
	XGlyphInfo		info;
	int 			i = 0, len = 0, stop = 0;
	int			idx, prevfont;
	char			*fmt;
	unsigned char		*u1, *u2;
	struct text_fragment	*frag;
	size_t			size;

	sect->text_width = 0;
	sect->nfrags = 0;
	frag = sect->frag;
	frag[0].text = sect->fmtrep;
	frag[0].length = 0;
	frag[0].font = 0;
	frag[0].width = 0;
	frag[0].bg = 0;
	frag[0].fg = 0;

	if (bar_font_legacy) {
		TEXTEXTENTS(bar_fs, sect->fmtrep, strlen(sect->fmtrep), &ibox,
		    &lbox);
		sect->height = lbox.height;
		sect->ypos = lbox.y;
	}

	fmt = sect->fmtrep;
	while (*fmt != '\0') {
		/* Use special font for Unicode code points U+E000 - U+F8FF */
		u1 = (unsigned char *)fmt;
		u2 = (unsigned char *)(fmt+1);
		if ((font_pua_index) && ((*u1 == 0xEE) || ((*u1 == 0xEF) &&
		    (*u2 < 0xA4)))) {
			if (len) {
				/* Finish processing preceding fragment */
				XftTextExtentsUtf8(display,
				    bar_xftfonts[frag[i].font],
				    (FcChar8 *)frag[i].text, len, &info);

				frag[i].length = len;
				frag[i].width = info.xOff;
				sect->text_width += frag[i].width;
				i++;
				if (i == SWM_TEXTFRAGS_MAX)
					break;
				frag[i].font = frag[i-1].font;
				frag[i].fg = frag[i-1].fg;
				frag[i].bg = frag[i-1].bg;
				len = 0;
			}

			prevfont = frag[i].font;
			frag[i].font = font_pua_index;
			frag[i].length = 3;
			frag[i].text = fmt;

			XftTextExtentsUtf8(display, bar_xftfonts[frag[i].font],
			    (FcChar8 *)frag[i].text, frag[i].length, &info);

			frag[i].width = info.xOff;
			sect->text_width += frag[i].width;

			fmt += frag[i].length;
			i++;
			if (i == SWM_TEXTFRAGS_MAX)
				break;

			frag[i].font = prevfont;
			frag[i].fg = frag[i-1].fg;
			frag[i].bg = frag[i-1].bg;
			frag[i].text = fmt;
			continue;
		}
		/* process markup code */
		if ((*fmt == '+') && (*(fmt+1) == '@') && (!stop) &&
			(is_valid_markup(fmt, &size))) {
			if (len) {
				/* Process preceding text fragment */
				if (bar_font_legacy) {
					TEXTEXTENTS(bar_fs, frag[i].text, len,
					    &ibox, &lbox);
					frag[i].width = lbox.width;
				} else {
					XftTextExtentsUtf8(display,
					    bar_xftfonts[frag[i].font],
					    (FcChar8 *)frag[i].text, len,
					    &info);
					frag[i].width = info.xOff;
				}
				frag[i].length = len;
				sect->text_width += frag[i].width;
				i++;
				if (i == SWM_TEXTFRAGS_MAX)
					break;
				frag[i].font = frag[i-1].font;
				frag[i].fg = frag[i-1].fg;
				frag[i].bg = frag[i-1].bg;
				len = 0;
			}
			idx = *(fmt+5) - '0';
			if ((*(fmt+2) == 'f') && (*(fmt+3) == 'n'))
				frag[i].font = idx;
			else if ((*(fmt+2) == 'f') && (*(fmt+3) == 'g'))
				frag[i].fg = idx;
			else if ((*(fmt+2) == 'b') && (*(fmt+3) == 'g'))
				frag[i].bg = idx;
			else if ((*(fmt+2) == 's') && (*(fmt+3) == 't')
			    && (*(fmt+4) == 'p'))
				stop = 1;

			*fmt = '\0';
			fmt += size;
			frag[i].text = fmt;
			continue;
		}
		/* process escaped '+' */
		if ((*fmt == '+') && (*(fmt+1) == '+') && (!stop)) {
			len++;
			fmt++;
			*fmt = '\0';
			if (bar_font_legacy) {
				TEXTEXTENTS(bar_fs, frag[i].text, len,
				    &ibox, &lbox);
				frag[i].width = lbox.width;
			} else {
				XftTextExtentsUtf8(display,
				    bar_xftfonts[frag[i].font],
				    (FcChar8 *)frag[i].text, len,
				    &info);
				frag[i].width = info.xOff;
			}
			frag[i].length = len;
			sect->text_width += frag[i].width;
			len = 0;
			fmt++;
			i++;
			if (i == SWM_TEXTFRAGS_MAX)
				break;
			frag[i].font = frag[i-1].font;
			frag[i].fg = frag[i-1].fg;
			frag[i].bg = frag[i-1].bg;
			frag[i].text = fmt;
			continue;
		}
		fmt++;
		len++;
	}
	if ((len) && (i < SWM_TEXTFRAGS_MAX)) {
		/* Process last text fragment */
		if (bar_font_legacy) {
			TEXTEXTENTS(bar_fs, frag[i].text, len, &ibox, &lbox);
			frag[i].width = lbox.width;
		} else {
			XftTextExtentsUtf8(display, bar_xftfonts[frag[i].font],
			    (FcChar8 *)frag[i].text, len, &info);
			frag[i].width = info.xOff;
		}
		sect->text_width += frag[i].width;
		frag[i].length = len;
		i++;
	}
	sect->nfrags = i;
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

/* Redraws a region bar; need to follow with xcb_flush() or focus_flush(). */
void
bar_draw(struct swm_bar *bar)
{
	struct swm_region	*r;
	char			fmtexp[SWM_BAR_MAX], fmtnew[SWM_BAR_MAX];
	char			fmtact[SWM_BAR_MAX * 2];
	int			i;

	/* expand the format by first passing it through strftime(3) */
	bar_fmt_expand(fmtexp, sizeof fmtexp);

	if (bar == NULL)
		return;

	r = bar->r;

	if (bar_enabled && r->ws->bar_enabled)
		xcb_map_window(conn, bar->id);
	else {
		xcb_unmap_window(conn, bar->id);
		return;
	}

	if (startup_exception) {
		snprintf(fmtexp, sizeof fmtexp,
		    "total exceptions: %u, first exception: %s",
		    nr_exceptions, startup_exception);
		if (bar_font_legacy)
			bar_print_legacy(r, fmtexp);
		else
			bar_print(r, fmtexp);
		return;
	}

	bar_fmt(fmtexp, fmtnew, r, sizeof fmtnew);
	if (bar_action_expand) {
		bar_replace_action(fmtnew, fmtact, r, sizeof fmtact);
		bar_split_format(fmtact);
	} else
		bar_split_format(fmtnew);

	for (i = 0;  i < numsect; i++)
		bar_replace(bsect[i].fmtsplit, bsect[i].fmtrep, r,
		    sizeof bsect[i].fmtrep);

	bar_print_layout(r);
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
		return (changed);

	while (fgets(b, sizeof(b), stdin) != NULL) {
		if (bar_enabled) {
			len = strlen(b);
			if (len > 0 && b[len - 1] == '\n') {
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

	return (changed);
}

void
bar_toggle(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct swm_region	*tmpr;
	int			i, num_screens;

	/* suppress unused warnings since vars are needed */
	(void)bp;
	(void)r;
	(void)args;

	switch (args->id) {
	case SWM_ARG_ID_BAR_TOGGLE_WS:
		/* Only change if master switch is enabled. */
		if (bar_enabled)
			r->ws->bar_enabled = !r->ws->bar_enabled;
		else
			bar_enabled = r->ws->bar_enabled = true;
		DNPRINTF(SWM_D_BAR, "ws%d->bar_enabled: %s\n", r->ws->idx,
		    YESNO(bar_enabled));
		break;
	case SWM_ARG_ID_BAR_TOGGLE:
		bar_enabled = !bar_enabled;
		break;
	}

	DNPRINTF(SWM_D_BAR, "bar_enabled: %s\n", YESNO(bar_enabled));

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

	/* Restack all regions and redraw bar. */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(tmpr, &screens[i].rl, entry) {
			stack(tmpr);
			bar_draw(tmpr->bar);
		}

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

	DNPRINTF(SWM_D_INIT, "loading bar_fonts: %s\n", bar_fonts);

	bar_fs = XCreateFontSet(display, bar_fonts, &missing_charsets,
	    &num_missing_charsets, &default_string);

	if (num_missing_charsets > 0) {
		warnx("Unable to load charset(s):");

		for (i = 0; i < num_missing_charsets; ++i)
			warnx("%s", missing_charsets[i]);

		XFreeStringList(missing_charsets);

		if(bar_fs && default_string) {
			if (strcmp(default_string, ""))
				warnx("Glyphs from those sets will be replaced "
				    "by '%s'.", default_string);
			else
				warnx("Glyphs from those sets won't be drawn.");
		}
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
	XRenderColor	color;
	int		i;

	for (i = 0; i < num_xftfonts; i++) {
		bar_xftfonts[i] = XftFontOpenName(display, r->s->idx,
		    bar_fontnames[i]);
		if (bar_xftfonts[i] == NULL)
			warnx("unable to load font %s", bar_fontnames[i]);
	}

	font_pua_index = 0;
	if (bar_fontname_pua) {
		bar_xftfonts[num_xftfonts] = XftFontOpenName(display,
		    r->s->idx, bar_fontname_pua);
		if (bar_xftfonts[i] == NULL)
			warnx("unable to load font %s", bar_fontname_pua);
		else
			font_pua_index = num_xftfonts;
	}

	for (i = 0; i < num_fg_colors; i++) {
		PIXEL_TO_XRENDERCOLOR(r->s->c[SWM_S_COLOR_BAR_FONT+i].pixel,
		    color);
		if (!XftColorAllocValue(display, r->s->xvisual,
		    r->s->colormap, &color, &bar_fg_colors[i]))
			warn("Xft error: unable to allocate color.");
	}

	if (bar_xftfonts[0] != NULL)
		bar_height = bar_xftfonts[0]->height + 2 * bar_border_width;

	if (bar_height < 1)
		bar_height = 1;
}

void
bar_setup(struct swm_region *r)
{
	uint32_t	 wa[4];
	size_t		len;
	char		*name;

	DNPRINTF(SWM_D_BAR, "screen %d.\n", r->s->idx);

	if (r->bar != NULL)
		return;

	if ((r->bar = calloc(1, sizeof(struct swm_bar))) == NULL)
		err(1, "bar_setup: calloc: failed to allocate memory.");

	if (bar_font_legacy)
		fontset_init();
	else
		xft_init(r);

	r->bar->r = r;
	X(r->bar) = X(r);
	Y(r->bar) = bar_at_bottom ? (Y(r) + HEIGHT(r) - bar_height) : Y(r);
	WIDTH(r->bar) = WIDTH(r) - 2 * bar_border_width;
	HEIGHT(r->bar) = bar_height - 2 * bar_border_width;

	/* Assume region is unfocused when we create the bar. */
	r->bar->id = xcb_generate_id(conn);
	wa[0] = r->s->c[SWM_S_COLOR_BAR].pixel;
	wa[1] = r->s->c[SWM_S_COLOR_BAR_BORDER_UNFOCUS].pixel;
	wa[2] = XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_LEAVE_WINDOW |
	    XCB_EVENT_MASK_EXPOSURE |
	    XCB_EVENT_MASK_POINTER_MOTION |
	    XCB_EVENT_MASK_POINTER_MOTION_HINT |
	    XCB_EVENT_MASK_FOCUS_CHANGE;
	wa[3] = r->s->colormap;

	xcb_create_window(conn, r->s->depth, r->bar->id, r->s->root,
	    X(r->bar), Y(r->bar), WIDTH(r->bar), HEIGHT(r->bar),
	    bar_border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT,
	    r->s->visual, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
	    | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP, wa);

	/* Set class and title to make the bar window identifiable. */
	xcb_icccm_set_wm_class(conn, r->bar->id,
	    strlen(SWM_WM_CLASS_BAR) + strlen(SWM_WM_CLASS_INSTANCE) + 2,
	    SWM_WM_CLASS_BAR "\0" SWM_WM_CLASS_INSTANCE "\0");

	if (asprintf(&name, "Status Bar - Region %d - " SWM_WM_CLASS_INSTANCE,
	    get_region_index(r) + 1) == -1)
		err(1, "asprintf");
	len = strlen(name);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, r->bar->id,
	    XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, len, name);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, r->bar->id,
	    ewmh[_NET_WM_NAME].atom, a_utf8_string, 8, len, name);
	free(name);

	/* Stack bar window above region window to start. */
	wa[0] = r->id;
	wa[1] = XCB_STACK_MODE_ABOVE;

	xcb_configure_window(conn, r->bar->id, XCB_CONFIG_WINDOW_SIBLING |
	    XCB_CONFIG_WINDOW_STACK_MODE, wa);

	r->bar->buffer = xcb_generate_id(conn);
	xcb_create_pixmap(conn, r->s->depth, r->bar->buffer, r->bar->id,
	    WIDTH(r->bar), HEIGHT(r->bar));

	if (randr_support)
		xcb_randr_select_input(conn, r->bar->id,
		    XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);

	if (bar_enabled)
		xcb_map_window(conn, r->bar->id);

	DNPRINTF(SWM_D_BAR, "win %#x, (x,y) w x h: (%d,%d) %d x %d\n",
	    WINID(r->bar), X(r->bar), Y(r->bar), WIDTH(r->bar), HEIGHT(r->bar));

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
	uint32_t		data[2] = { state, XCB_ATOM_NONE };

	DNPRINTF(SWM_D_EVENT, "win %#x, state: %s(%u)\n", WINID(win),
	    get_wm_state_label(state), state);

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

	DNPRINTF(SWM_D_MISC, "property: win %#x, state: %s(%u)\n",
	    w, get_wm_state_label(result), result);
	return (result);
}

void
version(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct swm_region	*tmpr;
	int			i, num_screens;

	/* suppress unused warnings since vars are needed */
	(void)bp;
	(void)r;
	(void)args;

	bar_version = !bar_version;
	if (bar_version)
		snprintf(bar_vertext, sizeof bar_vertext,
		    "Version: %s Build: %s", SPECTRWM_VERSION, buildstr);
	else
		strlcpy(bar_vertext, "", sizeof bar_vertext);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		TAILQ_FOREACH(tmpr, &screens[i].rl, entry) {
			bar_draw(tmpr->bar);
			xcb_flush(conn);
		}
	}
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
	DNPRINTF(SWM_D_EVENT, "win %#x, atom: %s(%u), time: %#x\n",
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
	ce.border_width = 0;
	ce.override_redirect = 0;

	if (ev == NULL) {
		/* EWMH */
		ce.event = win->id;
		ce.window = win->id;
		ce.above_sibling = XCB_WINDOW_NONE;
	} else {
		/* normal */
		ce.event = ev->window;
		ce.window = ev->window;

		/* make response appear more WM_SIZE_HINTS-compliant */
		if (win->sh.flags) {
			DNPRINTF(SWM_D_MISC, "hints: win %#x, sh.flags: %u, "
			    "min: %d x %d, max: %d x %d, inc: %d x %d\n",
			    win->id, win->sh.flags, SH_MIN_W(win),
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
		ce.x += ev->border_width;
		ce.y += ev->border_width;

		ce.above_sibling = ev->sibling;
	}

	DNPRINTF(SWM_D_MISC, "ewmh: %s, win %#x, (x,y) w x h: (%d,%d) %d x %d, "
	    "border: %d\n", YESNO(ev == NULL), win->id, ce.x, ce.y, ce.width,
	    ce.height, ce.border_width);

	xcb_send_event(conn, 0, win->id, XCB_EVENT_MASK_STRUCTURE_NOTIFY,
	    (char *)&ce);
}

int
count_win(struct workspace *ws, uint32_t flags)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (!(flags & SWM_COUNT_ICONIC) && ICONIC(win))
			continue;
		if (!(flags & SWM_COUNT_TILED) && !FLOATING(win))
			continue;
		if (!(flags & SWM_COUNT_FLOATING) && FLOATING(win))
			continue;
		count++;
	}

	return (count);
}

void
quit(struct binding *bp, struct swm_region *r, union arg *args)
{
	/* suppress unused warnings since vars are needed */
	(void)bp;
	(void)r;
	(void)args;

	DNPRINTF(SWM_D_MISC, "shutting down...\n");
	running = 0;
}

void
lower_window(struct ws_win *win)
{
	struct ws_win		*target = NULL, *mainw;
	struct workspace	*ws;

	DNPRINTF(SWM_D_EVENT, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	ws = win->ws;
	mainw = find_main_window(win);

	TAILQ_FOREACH(target, &ws->stack, stack_entry) {
		if (target == win || ICONIC(target))
			continue;
		if (ws->cur_layout == &layouts[SWM_MAX_STACK])
			break;
		if (TRANS(win)) {
			if (mainw == target || win == target->focus_redirect)
				break;
			if (mainw == find_main_window(target))
				continue;
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

#ifdef SWM_DEBUG
	if (swm_debug & SWM_D_STACK) {
		DPRINTF("=== stacking order (top down) === \n");
		TAILQ_FOREACH(target, &ws->stack, stack_entry) {
			DPRINTF("win %#x, fs: %s, maximized: %s, above: %s, "
			    "iconic: %s\n", target->id,
			    YESNO(FULLSCREEN(target)), YESNO(MAXIMIZED(target)),
			    YESNO(ABOVE(target)), YESNO(ICONIC(target)));
		}
	}
#endif
	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
raise_window_related(struct ws_win *win)
{
	struct ws_win		*w, *tmpw, *mainw;

	/* Raise main window and any transients. */
	mainw = find_main_window(win);
	raise_window(mainw);

	TAILQ_FOREACH_SAFE(w, &win->ws->stack, stack_entry, tmpw) {
		if (w == mainw || ICONIC(w))
			continue;

		if (find_main_window(w) == mainw)
			raise_window(w);
	}
}

void
raise_window(struct ws_win *win)
{
	struct ws_win		*target = NULL, *mainw;
	struct workspace	*ws;

	DNPRINTF(SWM_D_EVENT, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	ws = win->ws;
	mainw = find_main_window(win);

	TAILQ_FOREACH(target, &ws->stack, stack_entry) {
		if (target == win || ICONIC(target))
			continue;
		if (ws->cur_layout == &layouts[SWM_MAX_STACK] ||
		    win == ws->focus_raise)
			break;
		if (TRANS(win) && (mainw == find_main_window(target)))
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
		    (ws->focus == win && ws->always_raise))
			break;
		if (!ABOVE(target) && !TRANS(target))
			break;
	}

	TAILQ_REMOVE(&ws->stack, win, stack_entry);
	if (target)
		TAILQ_INSERT_BEFORE(target, win, stack_entry);
	else
		TAILQ_INSERT_TAIL(&ws->stack, win, stack_entry);

#ifdef SWM_DEBUG
	if (swm_debug & SWM_D_STACK) {
		DPRINTF("=== stacking order (top down) === \n");
		TAILQ_FOREACH(target, &ws->stack, stack_entry) {
			DPRINTF("win %#x, fs: %s, maximized: %s, above: %s, "
			    "iconic: %s\n", target->id,
			    YESNO(FULLSCREEN(target)), YESNO(MAXIMIZED(target)),
			    YESNO(ABOVE(target)), YESNO(ICONIC(target)));
		}
	}
#endif
	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
update_win_stacking(struct ws_win *win)
{
	struct ws_win		*sibling;
#ifdef SWM_DEBUG
	struct ws_win		*w;
#endif
	struct swm_region	*r;
	uint32_t		val[2];
	bool			raised;

	if (win == NULL || (r = win->ws->r) == NULL)
		return;

	if (win->frame == XCB_WINDOW_NONE) {
		DNPRINTF(SWM_D_EVENT, "win %#x not reparented.\n", win->id);
		return;
	}

	raised = (win->ws->focus == win && (win->ws->always_raise ||
	    win == win->ws->focus_raise));

	sibling = win;
	while ((sibling = TAILQ_NEXT(sibling, stack_entry)))
		if (!ICONIC(sibling))
			break;

	if (sibling && (FLOATING(win) == FLOATING(sibling) || raised))
		val[0] = sibling->frame;
	else if (FLOATING(win) || raised)
		val[0] = r->bar->id;
	else
		val[0] = r->id;

	DNPRINTF(SWM_D_EVENT, "win %#x (%#x), sibling %#x\n",
	    win->frame, win->id, val[0]);

	val[1] = XCB_STACK_MODE_ABOVE;

	xcb_configure_window(conn, win->frame, XCB_CONFIG_WINDOW_SIBLING |
	    XCB_CONFIG_WINDOW_STACK_MODE, val);

#ifdef SWM_DEBUG
	TAILQ_FOREACH(w, &win->ws->winlist, entry)
		debug_refresh(w);
#endif
}

void
map_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, mapped: %s\n",
	    win->id, YESNO(win->mapped));

	if (win->mapped)
		return;

	xcb_map_window(conn, win->frame);
	xcb_map_window(conn, win->id);
	set_win_state(win, XCB_ICCCM_WM_STATE_NORMAL);
	win->mapped = true;
}

void
unmap_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, mapped: %s\n", win->id,
	    YESNO(win->mapped));

	if (!win->mapped)
		return;

	xcb_unmap_window(conn, win->id);
	xcb_unmap_window(conn, win->frame);
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

	DNPRINTF(SWM_D_MISC, "win %#x, keycode %u\n", win->id, *keycode);

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
restart(struct binding *bp, struct swm_region *r, union arg *args)
{
	/* suppress unused warning since var is needed */
	(void)bp;
	(void)r;

	DNPRINTF(SWM_D_MISC, "%s\n", start_argv[0]);

	shutdown_cleanup();

	if (args && args->id == SWM_ARG_ID_RESTARTOFDAY)
		unsetenv("SWM_STARTED");

	execvp(start_argv[0], start_argv);
	warn("execvp failed");
	quit(NULL, NULL, NULL);
}

struct swm_region *
get_pointer_region(struct swm_screen *s)
{
	struct swm_region		*r = NULL;
	xcb_query_pointer_reply_t	*qpr;

	qpr = xcb_query_pointer_reply(conn,
	    xcb_query_pointer(conn, s->root), NULL);
	if (qpr) {
		DNPRINTF(SWM_D_MISC, "pointer: (%d,%d)\n", qpr->root_x,
		    qpr->root_y);
		TAILQ_FOREACH(r, &s->rl, entry)
			if (X(r) <= qpr->root_x && qpr->root_x < MAX_X(r) &&
			    Y(r) <= qpr->root_y && qpr->root_y < MAX_Y(r))
				break;
		free(qpr);
	}

	return (r);
}

struct ws_win *
get_pointer_win(struct swm_screen *s)
{
	struct ws_win			*win = NULL;
	xcb_query_pointer_reply_t	*qpr;

	qpr = xcb_query_pointer_reply(conn,
	    xcb_query_pointer(conn, s->root), NULL);
	if (qpr) {
		win = find_window(qpr->child);
		free(qpr);
	}

	return (win);
}

void
center_pointer(struct swm_region *r)
{
	struct ws_win			*win;
	xcb_window_t			dwinid;
	int				dx, dy;
#ifdef SWM_XCB_HAS_XINPUT
	xcb_input_xi_get_client_pointer_reply_t		*gcpr;
#endif

	if (!warp_pointer || r == NULL)
		return;

	win = r->ws->focus;

	DNPRINTF(SWM_D_EVENT, "win %#x\n", WINID(win));

	if (win && win->mapped) {
		dwinid = win->frame;
		dx = WIDTH(win) / 2;
		dy = HEIGHT(win) / 2;
	} else {
		dwinid = r->id;
		dx = WIDTH(r) / 2;
		dy = HEIGHT(r) / 2;
	}

#ifdef SWM_XCB_HAS_XINPUT
	if (xinput2_support) {
		gcpr = xcb_input_xi_get_client_pointer_reply(conn,
		    xcb_input_xi_get_client_pointer(conn, XCB_NONE), NULL);
		if (gcpr)
			/* XIWarpPointer takes FP1616. */
			xcb_input_xi_warp_pointer(conn, XCB_NONE, dwinid, 0, 0,
			    0, 0, dx << 16, dy << 16, gcpr->deviceid);
	} else {
#endif
		xcb_warp_pointer(conn, XCB_NONE, dwinid, 0, 0, 0, 0, dx, dy);
#ifdef SWM_XCB_HAS_XINPUT
	}
#endif
}

struct swm_region *
root_to_region(xcb_window_t root, int check)
{
	struct ws_win			*cfw;
	struct swm_region		*r = NULL;
	int				i, num_screens;
	xcb_get_input_focus_reply_t	*gifr;

	DNPRINTF(SWM_D_MISC, "win %#x\n", root);

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
			if (gifr->focus != XCB_INPUT_FOCUS_POINTER_ROOT) {
				cfw = find_window(gifr->focus);
				if (cfw && cfw->ws->r)
					r = cfw->ws->r;
			}
			free(gifr);
		}
	}

	/* No region with an active focus; try to use pointer. */
	if (r == NULL && check & SWM_CK_POINTER)
		r = get_pointer_region(&screens[i]);

	/* Last resort. */
	if (r == NULL && check & SWM_CK_FALLBACK)
		r = TAILQ_FIRST(&screens[i].rl);

	DNPRINTF(SWM_D_MISC, "idx: %d\n", get_region_index(r));

	return (r);
}

struct swm_region *
find_region(xcb_window_t id)
{
	struct swm_region	*r;
	int			i, num_screens;

	if (id == XCB_WINDOW_NONE)
		return (NULL);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (r->id == id)
				return (r);

	return (NULL);
}

struct swm_screen *
find_screen(xcb_window_t id)
{
	int			i, num_screens;

	if (id == XCB_WINDOW_NONE)
		return (NULL);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == id)
			return (&screens[i]);

	return (NULL);
}

struct swm_bar *
find_bar(xcb_window_t id)
{
	struct swm_region	*r;
	int			i, num_screens;

	if (id == XCB_WINDOW_NONE)
		return (NULL);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (r->bar && r->bar->id == id)
				return (r->bar);

	return (NULL);
}

struct ws_win *
find_window(xcb_window_t id)
{
	struct ws_win		*win = NULL;
	int			i, j, num_screens;
	xcb_query_tree_reply_t	*qtr;

	DNPRINTF(SWM_D_MISC, "id: %#x\n", id);

	if (id == XCB_WINDOW_NONE)
		return (NULL);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (id == win->id || id == win->frame)
					return (win);

	/* If window isn't top-level, try to find managed ancestor. */
	qtr = xcb_query_tree_reply(conn, xcb_query_tree(conn, id), NULL);
	if (qtr) {
		if (qtr->parent != XCB_WINDOW_NONE && qtr->parent != qtr->root)
			win = find_window(qtr->parent);

#ifdef SWM_DEBUG
		if (win)
			DNPRINTF(SWM_D_MISC, "%#x is decendent of %#x.\n",
			    id, qtr->parent);
#endif
		free(qtr);
	}

#ifdef SWM_DEBUG
	if (win == NULL)
		DNPRINTF(SWM_D_MISC, "unmanaged.\n");
#endif
	return (win);
}

void
spawn(int ws_idx, union arg *args, bool close_fd)
{
	int			fd;
	char			*ret = NULL;

	if (args == NULL || args->argv[0] == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "%s\n", args->argv[0]);

	close(xcb_get_file_descriptor(conn));

	if ((ret = getenv("LD_PRELOAD"))) {
		if (asprintf(&ret, "%s:%s", SWM_LIB, ret) == -1) {
			warn("spawn: asprintf LD_PRELOAD");
			_exit(1);
		}
		setenv("LD_PRELOAD", ret, 1);
		free(ret);
	} else {
		setenv("LD_PRELOAD", SWM_LIB, 1);
	}

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

	if (signal(SIGPIPE, SIG_DFL) == SIG_ERR)
		err(1, "could not reset SIGPIPE");

	execvp(args->argv[0], args->argv);

	warn("spawn: execvp");
	_exit(1);
}

void
kill_refs(struct ws_win *win)
{
	struct workspace	*ws;
	struct ws_win		*w;
	int			i, j, num_screens;

	if (win == NULL)
		return;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		for (j = 0; j < workspace_limit; j++) {
			ws = &screens[i].ws[j];

			if (win == ws->focus)
				ws->focus = NULL;
			if (win == ws->focus_prev)
				ws->focus_prev = NULL;
			if (win == ws->focus_pending)
				ws->focus_pending = NULL;
			if (win == ws->focus_raise)
				ws->focus_raise = NULL;

			if (TRANS(win))
				TAILQ_FOREACH(w, &ws->winlist, entry)
					if (win == w->focus_redirect)
						w->focus_redirect = NULL;
		}
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

	DNPRINTF(SWM_D_FOCUS, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	if (win->ws == NULL) {
		DNPRINTF(SWM_D_FOCUS, "NULL ws\n");
		return;
	}

	if (validate_ws(win->ws)) {
		DNPRINTF(SWM_D_FOCUS, "invalid ws\n");
		return;
	}

	if (win->ws->r == NULL) {
		DNPRINTF(SWM_D_FOCUS, "NULL region\n");
		return;
	}

	if (validate_win(win)) {
		DNPRINTF(SWM_D_FOCUS, "invalid win\n");
		kill_refs(win);
		return;
	}

	if (win->ws->focus == win) {
		win->ws->focus = NULL;
		win->ws->focus_prev = win;
		if (win->ws->focus_raise == win && !FLOATING(win)) {
			win->ws->focus_raise = NULL;
			raise_window(win);
			update_win_stacking(win);
		}
	}

	if (validate_win(win->ws->focus)) {
		kill_refs(win->ws->focus);
		win->ws->focus = NULL;
	}

	if (validate_win(win->ws->focus_prev)) {
		kill_refs(win->ws->focus_prev);
		win->ws->focus_prev = NULL;
	}

	draw_frame(win);

	/* Raise window to "top unfocused position." */
	if (win->ws->always_raise) {
		raise_window(win);
		update_win_stacking(win);
	}

	/* Update border width */
	if (win->bordered && (win->quirks & SWM_Q_MINIMALBORDER) &&
	    FLOATING(win)) {
		win->bordered = 0;
		X(win) += border_width;
		Y(win) += border_width;
		update_window(win);
	}

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->s->root,
	    ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1, &none);

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
focus_win_input(struct ws_win *win, bool force_input)
{
	/* Set input focus if no input hint, or indicated by hint. */
	if (ACCEPTS_FOCUS(win)) {
		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
		    "PointerRoot, time: %#x\n", win->id, last_event_time);
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, win->id,
		    (force_input ? XCB_CURRENT_TIME : last_event_time));
	} else if (!win->take_focus) {
		if (win->ws && win->ws->r) {
			DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
			    "PointerRoot, time: CurrentTime\n", win->ws->r->id);
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
			    win->ws->r->id, XCB_CURRENT_TIME);
		} else {
			DNPRINTF(SWM_D_FOCUS, "SetInputFocus: PointerRoot, "
			    "revert-to: PointerRoot, time: CurrentTime\n");
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_INPUT_FOCUS_POINTER_ROOT, XCB_CURRENT_TIME);
		}
	}

	/* Tell app it can adjust focus to a specific window. */
	if (win->take_focus)
		client_msg(win, a_takefocus, last_event_time);
}

void
focus_win(struct ws_win *win)
{
	struct ws_win			*cfw = NULL, *mainw = NULL, *w;
	struct workspace		*ws;
	xcb_get_input_focus_reply_t	*gifr = NULL;
	xcb_get_window_attributes_reply_t	*war = NULL;

	DNPRINTF(SWM_D_FOCUS, "win %#x\n", WINID(win));

	if (win == NULL || win->ws == NULL)
		goto out;

	ws = win->ws;
	if (!win->mapped && !(ws->cur_layout->flags & SWM_L_MAPONFOCUS))
		goto out;

	if (validate_ws(ws))
		goto out;

	if (validate_win(win)) {
		kill_refs(win);
		goto out;
	}

	gifr = xcb_get_input_focus_reply(conn, xcb_get_input_focus(conn), NULL);
	if (gifr && gifr->focus != XCB_INPUT_FOCUS_POINTER_ROOT) {
		DNPRINTF(SWM_D_FOCUS, "cur focus: %#x\n", gifr->focus);

		cfw = find_window(gifr->focus);
		if (cfw) {
			if (cfw != win) {
				if (cfw->ws != ws && cfw->ws->r != NULL &&
				    cfw->frame != XCB_WINDOW_NONE) {
					draw_frame(cfw);
				} else {
					unfocus_win(cfw);
				}
			}
		} else {
			war = xcb_get_window_attributes_reply(conn,
			    xcb_get_window_attributes(conn, gifr->focus), NULL);
			if (war && war->override_redirect && ws->focus == win) {
				DNPRINTF(SWM_D_FOCUS, "skip refocus "
				    "from override_redirect.\n");
				goto out;
			}
		}
	}

	if (ws->focus != win) {
		if (ws->focus && ws->focus != cfw)
			unfocus_win(ws->focus);
		ws->focus = win;
	}

	set_focus_redirect(win); /* Set focus redirect up to main window. */
	win->focus_redirect = NULL; /* Clear any redirect from this window. */

	if (ws->r) {
		if (ws->cur_layout->flags & SWM_L_MAPONFOCUS) {
			/* Only related windows should be mapped. */
			mainw = find_main_window(win);
			TAILQ_FOREACH(w, &ws->stack, stack_entry) {
				if (ICONIC(w))
					continue;

				if (find_main_window(w) == mainw)
					map_window(w);
				else
					unmap_window(w);
			}
			update_stacking(win->s);
		} else if ((tile_gap < 0 && !FLOATING(win)) ||
		    ws->always_raise) {
			/* Focused window needs to be raised. */
			raise_window(win);
			update_win_stacking(win);
			map_window(win);
		}

		if (cfw != win) {
			focus_win_input(win, false);

			set_region(ws->r);

			xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
			    win->s->root, ewmh[_NET_ACTIVE_WINDOW].atom,
			    XCB_ATOM_WINDOW, 32, 1, &win->id);

			bar_draw(ws->r->bar);
		}
	}

	/* Update window border even if workspace is hidden. */
	draw_frame(win);
out:
	free(gifr);
	free(war);
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

/* If a transient window should have focus instead, return it. */
struct ws_win *
get_focus_magic(struct ws_win *win)
{
	struct ws_win	*winfocus = NULL;
	int		i, wincount;

	DNPRINTF(SWM_D_FOCUS, "win %#x\n", WINID(win));
	if (win == NULL)
		return (win);

	winfocus = find_main_window(win);

	if (winfocus->focus_redirect == NULL)
		return (winfocus);

	/* Put limit just in case a redirect loop exists. */
	wincount = count_win(win->ws, SWM_COUNT_NORMAL);
	for (i = 0; i < wincount; ++i) {
		if (winfocus->focus_redirect == NULL)
			break;

		if (ICONIC(winfocus->focus_redirect))
			break;

		if (validate_win(winfocus->focus_redirect))
			break;

		winfocus = winfocus->focus_redirect;
	}

	return (winfocus);
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

	/* Update the focus window frame on the now unfocused region. */
	if (rf && rf->ws->focus)
		draw_frame(rf->ws->focus);

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
		if (old_r) {
			unfocus_win(old_r->ws->focus);
			/* Clear bar since empty. */
			bar_draw(old_r->bar);
		}

		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
		    "PointerRoot,time: CurrentTime\n", r->id);
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, r->id,
		    XCB_CURRENT_TIME);
	}
}

void
switchws(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct swm_screen	*s;
	struct swm_region	*this_r, *other_r;
	struct ws_win		*win;
	struct workspace	*new_ws, *old_ws;
	xcb_window_t		none = XCB_WINDOW_NONE;
	int			wsid = args->id;
	bool			unmap_old = false, dofocus;

	if (!(r && r->s))
		return;

	if (wsid >= workspace_limit)
		return;

	s = r->s;
	this_r = r;
	old_ws = this_r->ws;
	new_ws = &s->ws[wsid];

	DNPRINTF(SWM_D_WS, "screen[%d]:%dx%d+%d+%d: %d -> %d\n", s->idx,
	    WIDTH(r), HEIGHT(r), X(r), Y(r), old_ws->idx, wsid);

	if (new_ws == NULL || old_ws == NULL)
		return;
	if (new_ws == old_ws)
		return;

	other_r = new_ws->r;
	if (other_r && workspace_clamp && (bp == NULL ||
	    (bp->action != FN_RG_MOVE_NEXT && bp->action != FN_RG_MOVE_PREV))) {
		DNPRINTF(SWM_D_WS, "ws clamped.\n");
		if (warp_focus) {
			DNPRINTF(SWM_D_WS, "warping focus to region "
			    "with ws %d\n", wsid);
			focus_region(other_r);
			center_pointer(other_r);
			focus_flush();
		}
		return;
	}

	dofocus = !(pointer_follow(s));

	if ((win = old_ws->focus) != NULL) {
		draw_frame(win);

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
		    ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1,
		    &none);
	}

	if (other_r) {
		/* the other ws is visible in another region, exchange them */
		other_r->ws_prior = new_ws;
		other_r->ws = old_ws;
		old_ws->r = other_r;
	} else {
		/* the other workspace is hidden, hide this one */
		old_ws->r = NULL;
		unmap_old = true;
	}

	this_r->ws_prior = old_ws;
	this_r->ws = new_ws;
	new_ws->r = this_r;

	/* Prepare focus. */
	if (dofocus && (new_ws->focus_pending == NULL ||
	    validate_win(new_ws->focus_pending))) {
		new_ws->focus_pending = get_region_focus(new_ws->r);
		if (new_ws->focus_prev && !ICONIC(new_ws->focus_prev)) {
			new_ws->focus = new_ws->focus_prev;
			new_ws->focus_prev = NULL;
		}
	}

	new_ws->state = SWM_WS_STATE_MAPPING;

	stack(other_r);
	stack(this_r);

	/* unmap old windows */
	if (unmap_old) {
		TAILQ_FOREACH(win, &old_ws->winlist, entry)
			unmap_window(win);
		old_ws->state = SWM_WS_STATE_HIDDEN;
	}

	/* if workspaces were swapped, then don't wait to set focus */
	if (old_ws->r) {
		if (dofocus && new_ws->focus_pending) {
			focus_win(new_ws->focus_pending);
			new_ws->focus_pending = NULL;
		}

		if (old_ws->focus)
			draw_frame(old_ws->focus);
	}

	/* Clear bar and set focus on region input win if new ws is empty. */
	if (new_ws->focus_pending == NULL && new_ws->focus == NULL) {
		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
		    "PointerRoot, time: CurrentTime\n", r->id);
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, r->id,
		    XCB_CURRENT_TIME);
		bar_draw(r->bar);
	}

	ewmh_update_current_desktop();

	center_pointer(r);
	focus_flush();
	new_ws->state = SWM_WS_STATE_MAPPED;

	DNPRINTF(SWM_D_WS, "done\n");
}

void
cyclews(struct binding *bp, struct swm_region *r, union arg *args)
{
	union arg		a;
	struct swm_screen	*s = r->s;
	struct swm_region	*rr;
	struct workspace	*ws, *nws = NULL;
	struct ws_win		*winfocus;
	int			i;
	bool			allowempty = false, mv = false;

	if (r == NULL || r->ws == NULL)
		return;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	ws = r->ws;
	winfocus = ws->focus;

	for (i = 0; i < workspace_limit; ++i) {
		switch (args->id) {
		case SWM_ARG_ID_CYCLEWS_MOVE_UP:
			mv = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP_ALL:
			allowempty = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP:
			nws = &s->ws[(ws->idx + i + 1) % workspace_limit];
			break;
		case SWM_ARG_ID_CYCLEWS_MOVE_DOWN:
			mv = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN_ALL:
			allowempty = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN:
			nws = &s->ws[(workspace_limit + ws->idx - i - 1) %
			    workspace_limit];
			break;
		default:
			return;
		};

		DNPRINTF(SWM_D_WS, "curws: %d, nws: %d, allowempty: %d, mv: %d, "
		    "cycle_visible: %d, cycle_empty: %d\n", ws->idx, nws->idx,
		    allowempty, mv, cycle_visible, cycle_empty);

		if (!allowempty && !cycle_empty && TAILQ_EMPTY(&nws->winlist))
			continue;
		if (!cycle_visible && nws->r)
			continue;
		/* New workspace found. */
		break;
	}

	if (nws && nws != ws) {
		if (mv && winfocus) {
			/* Move window to new ws without unmapping. */
			win_to_ws(winfocus, nws->idx, SWM_WIN_NOUNMAP);
			rr = nws->r;
			if (rr) {
				if (!workspace_clamp) {
					/* Swap workspaces */
					r->ws_prior = ws;
					r->ws = nws;
					nws->r = r;

					rr->ws_prior = nws;
					rr->ws = ws;
					ws->r = rr;
				}
				/* else leave ws on other region. */
			} else {
				nws->r = r;
				r->ws = nws;
				r->ws_prior = ws;
				ws->r = NULL;
			}

			/* Hot set new focus. */
			nws->focus_prev = nws->focus;
			nws->focus = winfocus;
			nws->focus_pending = NULL;
			/* Focus on old ws updated by win_to_ws(). */

			if (nws->state == SWM_WS_STATE_HIDDEN)
				nws->state = SWM_WS_STATE_MAPPING;

			raise_window(winfocus);

			if (rr) {
				stack(rr);

				if (ws->focus == NULL) {
					ws->focus = ws->focus_pending;
					ws->focus_pending = NULL;
					raise_window(ws->focus);
				}
			} else
				unmap_workspace(ws);

			if (nws->r != r) {
				set_region(nws->r);
				draw_frame(nws->focus);
				draw_frame(ws->focus);
			}

			clear_maximized(nws);
			stack(r);

			ewmh_update_current_desktop();
			center_pointer(nws->r);
			focus_flush();
			nws->state = SWM_WS_STATE_MAPPED;
		} else {
			a.id = nws->idx;
			switchws(bp, r, &a);
			focus_flush();
		}
	}

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
unmap_workspace(struct workspace *ws)
{
	struct ws_win	*w;

	if (ws == NULL || ws->state == SWM_WS_STATE_HIDDEN)
		return;

	TAILQ_FOREACH(w, &ws->winlist, entry)
		unmap_window(w);
	ws->state = SWM_WS_STATE_HIDDEN;
}

void
emptyws(struct binding *bp, struct swm_region *r, union arg *args)
{
	int		i, empty = -1;
	union arg	a;
	struct ws_win	*win;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	/* Find first empty ws. */
	for (i = 0; i < workspace_limit; ++i)
		if (TAILQ_EMPTY(&r->s->ws[i].winlist)) {
			empty = i;
			break;
		}

	if (empty == -1) {
		DNPRINTF(SWM_D_FOCUS, "no empty ws.\n");
		return;
	}

	a.id = empty;

	switch (args->id) {
	case SWM_ARG_ID_WS_EMPTY_MOVE:
		/* Only move & switch if there is a focused window. */
		if (r->ws->focus)
			TAILQ_FOREACH(win, &r->ws->winlist, entry)
				if (win != r->ws->focus) {
					send_to_ws(bp, r, &a);
					switchws(bp, r, &a);
					break;
				}
		break;
	case SWM_ARG_ID_WS_EMPTY:
		switchws(bp, r, &a);
		break;
	default:
		DNPRINTF(SWM_D_FOCUS, "invalid id: %d\n", args->id);
	}

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
priorws(struct binding *bp, struct swm_region *r, union arg *args)
{
	union arg		a;

	(void)args;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	if (r->ws_prior == NULL)
		return;

	a.id = r->ws_prior->idx;
	switchws(bp, r, &a);
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
focusrg(struct binding *bp, struct swm_region *r, union arg *args)
{
	int			ridx = args->id, i, num_screens;
	struct swm_region	*rr = NULL;

	(void)bp;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	DNPRINTF(SWM_D_FOCUS, "id: %d\n", ridx);

	rr = TAILQ_FIRST(&r->s->rl);
	for (i = 0; i < ridx && rr != NULL; ++i)
		rr = TAILQ_NEXT(rr, entry);

	if (rr == NULL)
		return;

	focus_region(rr);
	center_pointer(rr);
	focus_flush();
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
cyclerg(struct binding *bp, struct swm_region *r, union arg *args)
{
	union arg		a;
	struct swm_region	*rr = NULL;
	int			i, num_screens;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	i = r->s->idx;
	DNPRINTF(SWM_D_FOCUS, "id: %d, region: %d\n", args->id, i);

	switch (args->id) {
	case SWM_ARG_ID_CYCLERG_UP:
	case SWM_ARG_ID_CYCLERG_MOVE_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&screens[i].rl);
		break;
	case SWM_ARG_ID_CYCLERG_DOWN:
	case SWM_ARG_ID_CYCLERG_MOVE_DOWN:
		rr = TAILQ_PREV(r, swm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&screens[i].rl, swm_region_list);
		break;
	default:
		return;
	};
	if (rr == NULL)
		return;

	switch (args->id) {
	case SWM_ARG_ID_CYCLERG_UP:
	case SWM_ARG_ID_CYCLERG_DOWN:
		focus_region(rr);
		center_pointer(rr);
		focus_flush();
		break;
	case SWM_ARG_ID_CYCLERG_MOVE_UP:
	case SWM_ARG_ID_CYCLERG_MOVE_DOWN:
		a.id = rr->ws->idx;
		switchws(bp, r, &a);
		break;
	default:
		return;
	};

	DNPRINTF(SWM_D_FOCUS, "done\n");
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
swapwin(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*target, *source;
	struct ws_win		*cur_focus;
	struct ws_win_list	*wl;

	(void)bp;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

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
			source = find_main_window(
			    find_window(source->transient));

		target = source;
		while ((target = TAILQ_PREV(target, ws_win_list, entry)))
			if (!ICONIC(target) && !TRANS(target))
				break;

		if (target && target->transient)
			target = find_window(target->transient);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_TAIL(wl, source, entry);
		else
			TAILQ_INSERT_BEFORE(target, source, entry);
		break;
	case SWM_ARG_ID_SWAPNEXT:
		if (TRANS(source))
			source = find_main_window(
			    find_window(source->transient));

		target = source;
		while ((target = TAILQ_NEXT(target, entry)))
			if (!ICONIC(target) && !TRANS(target))
				break;

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
		DNPRINTF(SWM_D_MOVE, "invalid id: %d\n", args->id);
		return;
	}

	sort_windows(wl);
	ewmh_update_client_list();

	stack(r);
	center_pointer(r);
	focus_flush();
out:
	DNPRINTF(SWM_D_MOVE, "done\n");
}

/* Determine focus other than specified window, but on the same workspace. */
struct ws_win *
get_focus_other(struct ws_win *win)
{
	struct ws_win		*w, *winfocus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	if (!(win && win->ws))
		goto done;

	ws = win->ws;
	wl = &ws->winlist;

	DNPRINTF(SWM_D_FOCUS, "win %#x, focus: %#x, focus_prev: %#x, "
	    "focus_close: %d, focus_close_wrap: %d, focus_default: %d\n",
	    WINID(win), WINID(ws->focus), WINID(ws->focus_prev), focus_close,
	    focus_close_wrap, focus_default);

	if (ws->focus == NULL) {
		/* Fallback to default. */
		if (focus_default == SWM_STACK_TOP) {
			TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list, entry)
				if (!ICONIC(winfocus) && winfocus != win)
					break;
		} else {
			TAILQ_FOREACH(winfocus, wl, entry)
				if (!ICONIC(winfocus) && winfocus != win)
					break;
		}

		goto done;
	}

	if (ws->focus != win && !ICONIC(ws->focus)) {
		winfocus = ws->focus;
		goto done;
	}

	/* FOCUSPREV quirk: try previously focused window. */
	if (((win->quirks & SWM_Q_FOCUSPREV) ||
	    (ws->cur_layout->flags & SWM_L_FOCUSPREV)) &&
	    ws->focus_prev && ws->focus_prev != win &&
	    !ICONIC(ws->focus_prev)) {
		winfocus = ws->focus_prev;
		goto done;
	}

	if (ws->focus == win && TRANS(win)) {
		w = find_window(win->transient);
		if (w && w->ws == ws && !ICONIC(w)) {
			winfocus = w;
			goto done;
		}
	}

	switch (focus_close) {
	case SWM_STACK_BOTTOM:
		TAILQ_FOREACH(winfocus, wl, entry)
			if (!ICONIC(winfocus) && winfocus != win)
				break;
		break;
	case SWM_STACK_TOP:
		TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list, entry)
			if (!ICONIC(winfocus) && winfocus != win)
				break;
		break;
	case SWM_STACK_ABOVE:
		winfocus = TAILQ_NEXT(win, entry);
		while (winfocus && ICONIC(winfocus))
			winfocus = TAILQ_NEXT(winfocus, entry);

		if (winfocus == NULL) {
			if (focus_close_wrap) {
				TAILQ_FOREACH(winfocus, wl, entry)
					if (!ICONIC(winfocus) &&
					    winfocus != win)
						break;
			} else {
				TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list,
				    entry)
					if (!ICONIC(winfocus) &&
					    winfocus != win)
						break;
			}
		}
		break;
	case SWM_STACK_BELOW:
		winfocus = TAILQ_PREV(win, ws_win_list, entry);
		while (winfocus && ICONIC(winfocus))
			winfocus = TAILQ_PREV(winfocus, ws_win_list, entry);

		if (winfocus == NULL) {
			if (focus_close_wrap) {
				TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list,
				    entry)
					if (!ICONIC(winfocus) &&
					    winfocus != win)
						break;
			} else {
				TAILQ_FOREACH(winfocus, wl, entry)
					if (!ICONIC(winfocus) &&
					    winfocus != win)
						break;
			}
		}
		break;
	}
done:
	DNPRINTF(SWM_D_FOCUS, "winfocus: %#x\n", WINID(winfocus));
	return (winfocus);
}

struct ws_win *
get_region_focus(struct swm_region *r)
{
	struct ws_win		*winfocus = NULL;

	if (!(r && r->ws))
		return (NULL);

	if (r->ws->focus && !ICONIC(r->ws->focus))
		winfocus = r->ws->focus;
	else if (r->ws->focus_prev && !ICONIC(r->ws->focus_prev))
		winfocus = r->ws->focus_prev;
	else
		TAILQ_FOREACH(winfocus, &r->ws->winlist, entry)
			if (!ICONIC(winfocus))
				break;

	return (get_focus_magic(winfocus));
}

void
focus(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*head, *cur_focus = NULL, *winfocus = NULL;
	struct ws_win		*mainw;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;
	union arg		a;
	int			i, wincount;

	if (!(r && r->ws))
		goto out;

	cur_focus = r->ws->focus;
	ws = r->ws;
	wl = &ws->winlist;

	DNPRINTF(SWM_D_FOCUS, "id: %d, cur_focus: %#x\n", args->id,
	    WINID(cur_focus));

	/* Make sure an uniconified window has focus, if one exists. */
	if (cur_focus == NULL) {
		cur_focus = TAILQ_FIRST(wl);
		while (cur_focus != NULL && ICONIC(cur_focus))
			cur_focus = TAILQ_NEXT(cur_focus, entry);

		DNPRINTF(SWM_D_FOCUS, "new cur_focus: %#x\n", WINID(cur_focus));
	}

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		if (cur_focus == NULL)
			goto out;

		mainw = find_main_window(cur_focus);
		wincount = count_win(ws, SWM_COUNT_ALL);
		winfocus = cur_focus;
		for (i = 0; i < wincount; ++i) {
			winfocus = TAILQ_PREV(winfocus, ws_win_list, entry);
			if (winfocus == NULL)
				winfocus = TAILQ_LAST(wl, ws_win_list);

			if (ICONIC(winfocus))
				continue;

			if (winfocus == mainw ||
			    find_main_window(winfocus) == mainw)
				continue;

			if (winfocus->quirks & SWM_Q_NOFOCUSCYCLE)
				continue;

			break;
		}
		break;
	case SWM_ARG_ID_FOCUSNEXT:
		if (cur_focus == NULL)
			goto out;

		mainw = find_main_window(cur_focus);
		wincount = count_win(ws, SWM_COUNT_ALL);

		winfocus = cur_focus;
		for (i = 0; i < wincount; ++i) {
			winfocus = TAILQ_NEXT(winfocus, entry);
			if (winfocus == NULL)
				winfocus = TAILQ_FIRST(wl);

			if (ICONIC(winfocus))
				continue;

			if (winfocus == mainw ||
			    find_main_window(winfocus) == mainw)
				continue;

			if (winfocus->quirks & SWM_Q_NOFOCUSCYCLE)
				continue;
			break;
		}
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
			switchws(bp, r, &a);
		}
		break;
	default:
		goto out;
	}

	if (clear_maximized(ws) > 0)
		stack(r);

	focus_win(get_focus_magic(winfocus));
	center_pointer(r);
	focus_flush();

out:
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
focus_pointer(struct binding *bp, struct swm_region *r, union arg *args)
{
	(void)args;

	/* Not needed for buttons since this is already done in buttonpress. */
	if (bp->type == KEYBIND) {
		focus_win(get_pointer_win(r->s));
		focus_flush();
	}
}

void
switchlayout(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	/* suppress unused warning since var is needed */
	(void)bp;

	DNPRINTF(SWM_D_EVENT, "workspace: %d\n", ws->idx);

	switch (args->id) {
	case SWM_ARG_ID_CYCLE_LAYOUT:
		ws->cur_layout++;
		if (ws->cur_layout->l_stack == NULL)
			ws->cur_layout = &layouts[0];
		break;
	case SWM_ARG_ID_LAYOUT_VERTICAL:
		ws->cur_layout = &layouts[SWM_V_STACK];
		break;
	case SWM_ARG_ID_LAYOUT_HORIZONTAL:
		ws->cur_layout = &layouts[SWM_H_STACK];
		break;
	case SWM_ARG_ID_LAYOUT_MAX:
		ws->cur_layout = &layouts[SWM_MAX_STACK];
		break;
	default:
		goto out;
	}

	clear_maximized(ws);

	stack(r);
	bar_draw(r->bar);

	focus_win(get_region_focus(r));

	center_pointer(r);
	focus_flush();
out:
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

void
stack_config(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	(void)bp;

	DNPRINTF(SWM_D_STACK, "id: %d workspace: %d\n", args->id, ws->idx);

	if (clear_maximized(ws) > 0)
		stack(r);

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != SWM_ARG_ID_STACKINIT)
		stack(r);
	bar_draw(r->bar);

	center_pointer(r);
	focus_flush();
}

void
stack(struct swm_region *r) {
	struct swm_geometry	g;
	struct swm_region	*r_prev;
	uint32_t		val[2];

	if (r == NULL)
		return;

	DNPRINTF(SWM_D_STACK, "begin\n");

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

	DNPRINTF(SWM_D_STACK, "workspace: %d (screen: %d, region: %d), (x,y) "
	    "WxH: (%d,%d) %d x %d\n", r->ws->idx, r->s->idx,
	    get_region_index(r), g.x, g.y, g.w, g.h);

	r_prev = TAILQ_PREV(r, swm_region_list, entry);
	if (r_prev) {
		/* Stack bar/input relative to prev. region. */
		val[1] = XCB_STACK_MODE_ABOVE;

		val[0] = r_prev->id;
		DNPRINTF(SWM_D_STACK, "region input %#x relative to %#x.\n",
		    r->id, val[0]);
		xcb_configure_window(conn, r->id,
		    XCB_CONFIG_WINDOW_SIBLING |
		    XCB_CONFIG_WINDOW_STACK_MODE, val);

		if (r->bar) {
			val[0] = r_prev->bar->id;
			DNPRINTF(SWM_D_STACK, "region bar %#x relative to "
			    "%#x.\n", r->bar->id, val[0]);
			xcb_configure_window(conn, r->bar->id,
			    XCB_CONFIG_WINDOW_SIBLING |
			    XCB_CONFIG_WINDOW_STACK_MODE, val);
		}
	}

	r->ws->cur_layout->l_stack(r->ws, &g);
	r->ws->cur_layout->l_string(r->ws);
	/* save r so we can track region changes */
	r->ws->old_r = r;

	if (font_adjusted)
		font_adjusted--;

	DNPRINTF(SWM_D_STACK, "end\n");
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
	DNPRINTF(SWM_D_MISC, "win %#x, g: (%d,%d) %d x %d, g_float: (%d,%d) "
	    "%d x %d\n", win->id, X(win), Y(win), WIDTH(win), HEIGHT(win),
	    win->g_float.x, win->g_float.y, win->g_float.w, win->g_float.h);
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
		DNPRINTF(SWM_D_MISC, "win %#x, g: (%d,%d) %d x %d\n", win->id,
		    X(win), Y(win), WIDTH(win), HEIGHT(win));
	} else {
		DNPRINTF(SWM_D_MISC, "win %#x, g_float is not set.\n", win->id);
	}
}

void
update_floater(struct ws_win *win)
{
	struct workspace	*ws;
	struct swm_region	*r;

	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	ws = win->ws;

	if ((r = ws->r) == NULL)
		return;

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

		if (bar_enabled && ws->bar_enabled && !maximize_hide_bar) {
			if (!bar_at_bottom)
				Y(win) += bar_height;
			HEIGHT(win) -= bar_height;
		} else if (disable_border) {
			win->bordered = false;
		}

		if (disable_border_always) {
			win->bordered = false;
		}

		if (win->bordered) {
			/* Window geometry excludes frame. */
			X(win) += border_width;
			Y(win) += border_width;
			HEIGHT(win) -= 2 * border_width;
			WIDTH(win) -= 2 * border_width;
		}
	} else {
		/* Normal floating window. */
		/* Update geometry if new region. */
		if (r != ws->old_r)
			load_float_geom(win);

		if (((win->quirks & SWM_Q_FULLSCREEN) &&
		     WIDTH(win) >= WIDTH(r) && HEIGHT(win) >= HEIGHT(r)) ||
		    ((!WS_FOCUSED(win->ws) || win->ws->focus != win) &&
		     (win->quirks & SWM_Q_MINIMALBORDER))) {
			/* Remove border */
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
				X(win) = X(r) + (WIDTH(r) - WIDTH(win)) / 2;
				Y(win) = Y(r) + (HEIGHT(r) - HEIGHT(win)) / 2;
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
	struct swm_geometry	cell, r_g = *g;
	struct ws_win		*win;
	int			i = 0, j = 0, s = 0, stacks = 0;
	int			w_inc = 1, h_inc, w_base = 1, h_base;
	int			hrh = 0, extra = 0, h_slice = 0, last_h = 0;
	int			split = 0, colno = 0;
	int			winno, mwin = 0, msize = 0;
	int			remain, missing, v_slice, mscale;
	bool			bordered = true, reconfigure = false;

	/*
	 * cell: geometry for window, including frame.
	 * mwin: # of windows in master area.
	 * mscale: size increment of master area.
	 * stacks: # of stack columns
	 */

	DNPRINTF(SWM_D_STACK, "workspace: %d, rot: %s, flip: %s\n", ws->idx,
	    YESNO(rot), YESNO(flip));

	memset(&cell, 0, sizeof(cell));

	/* Prepare tiling variables, if needed. */
	if ((winno = count_win(ws, SWM_COUNT_TILED)) > 0) {
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

		cell = r_g;
		cell.x += border_width;
		cell.y += border_width;

		if (stacks > winno - mwin)
			stacks = winno - mwin;
		if (stacks < 1)
			stacks = 1;

		h_slice = r_g.h / SWM_H_SLICE;
		if (mwin && winno > mwin) {
			v_slice = r_g.w / SWM_V_SLICE;

			split = mwin;
			colno = split;
			cell.w = v_slice * mscale;

			if (w_inc > 1 && w_inc < v_slice) {
				/* Adjust for requested size increment. */
				remain = (cell.w - w_base) % w_inc;
				cell.w -= remain;
			}

			msize = cell.w;
			if (flip)
				cell.x += r_g.w - msize;
			s = stacks;
		} else {
			msize = - 2 * border_width;
			colno = split = winno / stacks;
			cell.w = ((r_g.w - (stacks * 2 * border_width) +
			    2 * border_width) / stacks);
			s = stacks - 1;
		}

		hrh = r_g.h / colno;
		extra = r_g.h - (colno * hrh);
		cell.h = hrh - 2 * border_width;
		i = j = 0;
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
			if (colno > 0)
				hrh = r_g.h / colno;
			extra = r_g.h - (colno * hrh);

			if (!flip)
				cell.x += cell.w + 2 * border_width + tile_gap;

			cell.w = (r_g.w - msize -
			    (stacks * (2 * border_width + tile_gap))) / stacks;
			if (s == 1)
				cell.w += (r_g.w - msize -
				    (stacks * (2 * border_width + tile_gap))) %
				    stacks;

			if (flip)
				cell.x -= cell.w + 2 * border_width +
				    tile_gap;
			s--;
			j = 0;
		}

		cell.h = hrh - 2 * border_width - tile_gap;

		if (rot) {
			h_inc = win->sh.width_inc;
			h_base = win->sh.base_width;
		} else {
			h_inc =	win->sh.height_inc;
			h_base = win->sh.base_height;
		}

		if (j == colno - 1) {
			cell.h = hrh + extra;
		} else if (h_inc > 1 && h_inc < h_slice) {
			/* adjust for window's requested size increment */
			remain = (cell.h - h_base) % h_inc;
			missing = h_inc - remain;

			if (missing <= extra || j == 0) {
				extra -= missing;
				cell.h += missing;
			} else {
				cell.h -= remain;
				extra += remain;
			}
		}

		if (j == 0)
			cell.y = r_g.y + border_width;
		else
			cell.y += last_h + 2 * border_width + tile_gap;

		/* Window coordinates exclude frame. */

		if (winno > 1 || !disable_border ||
		    (bar_enabled && ws->bar_enabled &&
		    !disable_border_always)) {
			bordered = true;
		} else {
			bordered = false;
		}

		if (rot) {
			if (X(win) != cell.y || Y(win) != cell.x ||
			    WIDTH(win) != cell.h || HEIGHT(win) != cell.w) {
				reconfigure = true;
				X(win) = cell.y;
				Y(win) = cell.x;
				WIDTH(win) = cell.h;
				HEIGHT(win) = cell.w;
			}
		} else {
			if (X(win) != cell.x || Y(win) != cell.y ||
			    WIDTH(win) != cell.w || HEIGHT(win) != cell.h) {
				reconfigure = true;
				X(win) = cell.x;
				Y(win) = cell.y;
				WIDTH(win) = cell.w;
				HEIGHT(win) = cell.h;
			}
		}

		if (!bordered) {
			X(win) -= border_width;
			Y(win) -= border_width;
			WIDTH(win) += 2 * border_width;
			HEIGHT(win) += 2 * border_width;
		}

		if (bordered != win->bordered) {
			reconfigure = true;
			win->bordered = bordered;
		}

		if (reconfigure) {
			adjust_font(win);
			update_window(win);
		}

		last_h = cell.h;
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

	DNPRINTF(SWM_D_STACK, "done\n");
}

void
vertical_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "id: %d, workspace: %d\n", id, ws->idx);

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
	case SWM_ARG_ID_STACKBALANCE:
		ws->l_state.vertical_msize = SWM_V_SLICE /
		    (ws->l_state.vertical_stacks + 1);
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
	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, ws->l_state.vertical_flip);
}

void
horizontal_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

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
	case SWM_ARG_ID_STACKBALANCE:
		ws->l_state.horizontal_msize = SWM_H_SLICE /
		    (ws->l_state.horizontal_stacks + 1);
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
	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, ws->l_state.horizontal_flip);
}

void
max_stack(struct workspace *ws, struct swm_geometry *g)
{
	struct swm_geometry	gg = *g;
	struct ws_win		*w, *win = NULL, *mainw;

	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	if (ws == NULL)
		return;

	if (count_win(ws, SWM_COUNT_NORMAL) == 0)
		return;

	/* Figure out which window to put on top. */
	if (ws->focus_pending)
		win = ws->focus_pending;
	else if (ws->focus)
		win = ws->focus;
	else if (ws->focus_prev)
		win = ws->focus_prev;
	else
		win = TAILQ_FIRST(&ws->winlist);

	mainw = find_main_window(win);

	DNPRINTF(SWM_D_STACK, "focus_pending: %#x, focus: %#x, focus_prev: %#x,"
	    " first: %#x, win: %#x, mainw: %#x\n", WINID(ws->focus_pending),
	    WINID(ws->focus), WINID(ws->focus_prev),
	    WINID(TAILQ_FIRST(&ws->winlist)), WINID(win), WINID(mainw));

	/* Unmap unrelated windows. */
	TAILQ_FOREACH(w, &ws->winlist, entry)
		if (find_main_window(w) != mainw)
			unmap_window(w);

	/* Update window geometry. */
	TAILQ_FOREACH(w, &ws->winlist, entry) {
		if (ICONIC(w))
			continue;

		if (TRANS(w) || FULLSCREEN(w)) {
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

			if (disable_border_always ||
			    (disable_border &&
			    !(bar_enabled && ws->bar_enabled))) {
				w->bordered = false;
				WIDTH(w) += 2 * border_width;
				HEIGHT(w) += 2 * border_width;
			} else {
				w->bordered = true;
				X(w) += border_width;
				Y(w) += border_width;
			}

			update_window(w);
		}

		if (find_main_window(w) == mainw)
			map_window(w);
	}

	update_stacking(win->s);
}

void
update_stacking(struct swm_screen *s)
{
	struct swm_region	*r;
	struct ws_win		*w;

	/* Stack all windows from bottom up. */
	TAILQ_FOREACH(r, &s->rl, entry)
		TAILQ_FOREACH_REVERSE(w, &r->ws->stack, ws_win_stack,
		    stack_entry)
			if (!ICONIC(w))
				update_win_stacking(w);
}

void
send_to_rg(struct binding *bp, struct swm_region *r, union arg *args)
{
	int			ridx = args->id, i, num_screens;
	struct swm_region	*rr = NULL;
	union arg		a;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	DNPRINTF(SWM_D_FOCUS, "id: %d\n", ridx);

	rr = TAILQ_FIRST(&r->s->rl);
	for (i = 0; i < ridx && rr != NULL; ++i)
		rr = TAILQ_NEXT(rr, entry);

	if (rr == NULL)
		return;

	a.id = rr->ws->idx;

	send_to_ws(bp, r, &a);
}

struct swm_region *
region_under(struct swm_screen *s, int x, int y)
{
	struct swm_region	*r = NULL;

	TAILQ_FOREACH(r, &s->rl, entry) {
		DNPRINTF(SWM_D_MISC, "ws: %d, region g: (%d,%d) %d x %d, "
		    "coords: (%d,%d)\n", r->ws->idx, X(r), Y(r), WIDTH(r),
		    HEIGHT(r), x, y);
		if (X(r) <= x && x < MAX_X(r))
			if (Y(r) <= y && y < MAX_Y(r))
				return (r);
	}

	return (NULL);
}

/* Transfer focused window to target workspace and focus. */
void
send_to_ws(struct binding *bp, struct swm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win = NULL;
	bool			dofocus;

	(void)bp;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	DNPRINTF(SWM_D_MOVE, "win %#x, ws %d\n", win->id, wsid);

	if (wsid < 0 || wsid >= workspace_limit)
		return;

	if (win->ws->idx == wsid)
		return;

	dofocus = !(pointer_follow(win->s));

	win_to_ws(win, wsid, SWM_WIN_UNFOCUS);

	/* Set new focus on target ws. */
	if (dofocus) {
		win->ws->focus_prev = win->ws->focus;
		win->ws->focus = win;
		win->ws->focus_pending = NULL;

		if (win->ws->focus_prev)
			draw_frame(win->ws->focus_prev);
	}

	DNPRINTF(SWM_D_STACK, "focus_pending: %#x, focus: %#x, focus_prev: %#x,"
	    " first: %#x, win: %#x\n", WINID(r->ws->focus_pending),
	    WINID(r->ws->focus), WINID(r->ws->focus_prev),
	    WINID(TAILQ_FIRST(&r->ws->winlist)), win->id);

	ewmh_apply_flags(win, win->ewmh_flags & ~EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(win);

	/* Restack focused region. */
	stack(r);

	/* If destination ws has a region, restack. */
	if (win->ws->r) {
		if (FLOATING(win))
			load_float_geom(win);

		stack(win->ws->r);
	}

	/* Set new focus on current ws. */
	if (dofocus) {
		if (r->ws->focus == NULL)
			r->ws->focus = r->ws->focus_pending;

		if (r->ws->focus) {
			focus_win(r->ws->focus);
		} else {
			DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
			    "PointerRoot, time: CurrentTime\n", r->id);
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
			    r->id, XCB_CURRENT_TIME);
			bar_draw(r->bar);
		}
	}

	center_pointer(r);
	focus_flush();
}

/* Transfer focused window to region-relative workspace and focus. */
void
send_to_rg_relative(struct binding *bp, struct swm_region *r, union arg *args)
{
	union arg		args_abs;
	struct swm_region	*r_other;

	if (args->id == 1) {
		r_other = TAILQ_NEXT(r, entry);
		if (r_other == NULL)
			r_other = TAILQ_FIRST(&r->s->rl);
	} else {
		r_other = TAILQ_PREV(r, swm_region_list, entry);
		if (r_other == NULL)
			r_other = TAILQ_LAST(&r->s->rl, swm_region_list);
	}

	/* Map relative to absolute */
	args_abs = *args;
	args_abs.id = r_other->ws->idx;

	send_to_ws(bp, r, &args_abs);
}

/* Determine a window to consider 'main' for specified window. */
struct ws_win *
find_main_window(struct ws_win *win)
{
	struct ws_win	*w;
	int		i, wincount;

	if (win == NULL || !TRANS(win))
		return (win);

	/* Limit search to prevent infinite loop. */
	wincount = 0;
	for (i = 0; i < workspace_limit; i++)
		wincount += count_win(&win->s->ws[i], SWM_COUNT_ALL);

	/* Resolve TRANSIENT_FOR as far as possible. */
	w = win;
	for (i = 0; w && TRANS(w) && i < wincount; i++) {
		w = find_window(w->transient);
		if (w == win)
			/* Transient loop shouldn't occur. */
			break;
	}

	if (w == NULL)
		w = win;

	return (w);
}

void
set_focus_redirect(struct ws_win *win)
{
	struct ws_win	*w, *tmpw;
	int		i, wincount;

	if (win == NULL || !TRANS(win))
		return;

	/* Limit search to prevent infinite loop. */
	wincount = 0;
	for (i = 0; i < workspace_limit; i++)
		wincount += count_win(&win->s->ws[i], SWM_COUNT_ALL);

	/* Set focus_redirect along transient chain. */
	w = win;
	for (i = 0; w && TRANS(w) && i < wincount; i++) {
		tmpw = find_window(w->transient);
		if (tmpw == NULL || tmpw == win)
			/* Transient loop shouldn't occur. */
			break;
		tmpw->focus_redirect = w;
		w = tmpw;
	}
}

void
win_to_ws(struct ws_win *win, int wsid, uint32_t flags)
{
	struct ws_win		*mainw, *w, *tmpw;
	struct workspace	*ws, *nws;

	if (win == NULL || wsid < 0 || wsid >= workspace_limit)
		return;

	if (win->ws->idx == wsid) {
		DNPRINTF(SWM_D_MOVE, "win %#x already on ws %d\n",
		    win->id, wsid);
		return;
	}

	ws = win->ws;
	nws = &win->s->ws[wsid];

	DNPRINTF(SWM_D_MOVE, "win %#x, ws %d -> %d\n", win->id, ws->idx, wsid);

	mainw = find_main_window(win);

	DNPRINTF(SWM_D_MOVE, "focus %#x, mainw: %#x\n", WINID(ws->focus), WINID(mainw));

	/* Transfer main window and any related transients. */
	TAILQ_FOREACH_SAFE(w, &ws->winlist, entry, tmpw) {
		if (find_main_window(w) == mainw) {
			if (ws->focus == w) {
				ws->focus_pending = get_focus_other(w);

				if (flags & SWM_WIN_UNFOCUS)
					unfocus_win(w);
			}

			/* Unmap if new ws is hidden. */
			if (!(flags & SWM_WIN_NOUNMAP) && nws->r == NULL)
				unmap_window(w);

			/* Transfer */
			TAILQ_REMOVE(&ws->winlist, w, entry);
			TAILQ_REMOVE(&ws->stack, w, stack_entry);
			TAILQ_INSERT_TAIL(&nws->winlist, w, entry);
			TAILQ_INSERT_TAIL(&nws->stack, w, stack_entry);
			w->ws = nws;

			/* Cleanup references. */
			if (ws->focus == w)
				ws->focus = NULL;
			if (ws->focus_prev == w)
				ws->focus_prev = NULL;
			if (ws->focus_pending == w)
				ws->focus_pending = NULL;
			if (ws->focus_raise == w)
				ws->focus_raise = NULL;

			DNPRINTF(SWM_D_PROP, "win %#x, set property: "
			    "_NET_WM_DESKTOP: %d\n", w->id, wsid);
			xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
			    w->id, ewmh[_NET_WM_DESKTOP].atom,
			    XCB_ATOM_CARDINAL, 32, 1, &wsid);
		}
	}

	ewmh_update_client_list();

	DNPRINTF(SWM_D_MOVE, "done\n");
}

void
pressbutton(struct binding *bp, struct swm_region *r, union arg *args)
{
	/* suppress unused warning since var is needed */
	(void)bp;
	(void)r;

	xcb_test_fake_input(conn, XCB_BUTTON_PRESS, args->id,
	    XCB_CURRENT_TIME, XCB_WINDOW_NONE, 0, 0, 0);
	xcb_test_fake_input(conn, XCB_BUTTON_RELEASE, args->id,
	    XCB_CURRENT_TIME, XCB_WINDOW_NONE, 0, 0, 0);
}

void
raise_focus(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win	*win;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;

	win = r->ws->focus;
	r->ws->focus_raise = win;
	raise_window(win);
	update_win_stacking(win);

	focus_flush();
}

void
raise_toggle(struct binding *bp, struct swm_region *r, union arg *args)
{
	/* Suppress warning. */
	(void)bp;
	(void)args;

	if (r == NULL || r->ws == NULL)
		return;

	if (r->ws->focus && MAXIMIZED(r->ws->focus))
		return;

	r->ws->always_raise = !r->ws->always_raise;

	/* Update focused win stacking order based on new always_raise value. */
	raise_window(r->ws->focus);
	update_win_stacking(r->ws->focus);

	focus_flush();
}

void
iconify(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*w;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	if ((w = r->ws->focus) == NULL)
		return;

	ewmh_apply_flags(w, w->ewmh_flags | EWMH_F_HIDDEN);
	ewmh_update_wm_state(w);

	stack(r);

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
uniconify(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*win;
	FILE			*lfile;
	char			*name;
	int			count = 0;

	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

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

	DNPRINTF(SWM_D_MISC, "count: %d\n", count);

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
name_workspace(struct binding *bp, struct swm_region *r, union arg *args)
{
	FILE			*lfile;

	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

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
search_workspace(struct binding *bp, struct swm_region *r, union arg *args)
{
	int			i;
	struct workspace	*ws;
	FILE			*lfile;

	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

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
#ifndef __clang_analyzer__ /* Suppress false warnings. */
	while ((sw = TAILQ_FIRST(&search_wl)) != NULL) {
		xcb_destroy_window(conn, sw->indicator);
		TAILQ_REMOVE(&search_wl, sw, entry);
		free(sw);
	}
#endif
}

void
search_win(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;
	struct search_window	*sw = NULL;
	xcb_window_t		w;
	uint32_t		wa[3];
	int			i, width, height;
	char			s[11];
	FILE			*lfile;
	size_t			len;
	XftDraw			*draw;
	XGlyphInfo		info;
	GC			l_draw;
	XGCValues		l_gcv;
	XRectangle		l_ibox, l_lbox = {0, 0, 0, 0};

	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WINDOW;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

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
		wa[2] = r->s->colormap;

		if (bar_font_legacy) {
			TEXTEXTENTS(bar_fs, s, len, &l_ibox, &l_lbox);
			width = l_lbox.width + 4;
			height = bar_fs_extents->max_logical_extent.height + 4;
		} else {
			XftTextExtentsUtf8(display, bar_xftfonts[0],
			    (FcChar8 *)s, len, &info);
			width = info.xOff + 4;
			height = bar_xftfonts[0]->height + 4;
		}

		xcb_create_window(conn, win->s->depth, w, win->frame, 0, 0,
		    width, height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		    win->s->visual, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
		    XCB_CW_COLORMAP, wa);

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
			draw = XftDrawCreate(display, w, r->s->xvisual,
			    r->s->colormap);

			XftDrawStringUtf8(draw, &search_font_color,
			    bar_xftfonts[0], 2, (HEIGHT(r->bar) +
			    bar_xftfonts[0]->height) / 2 -
			    bar_xftfonts[0]->descent, (FcChar8 *)s, len);

			XftDrawDestroy(draw);
		}

		DNPRINTF(SWM_D_MISC, "mapped win %#x\n", w);

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

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

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
			stack(search_r);
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

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

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
			DNPRINTF(SWM_D_MISC, "strdup: %s", strerror(errno));
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

		if ((name_list = calloc(len, sizeof(char))) == NULL)
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

		DNPRINTF(SWM_D_PROP, "win count: %d\n", count);

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
	for (i = 0; i < num_screens; ++i) {
		if (screens[i].r_focus)
			xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
			    screens[i].root, ewmh[_NET_CURRENT_DESKTOP].atom,
			    XCB_ATOM_CARDINAL, 32, 1,
			    &screens[i].r_focus->ws->idx);
	}
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

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

	q = strdup(resp);
	if (q == NULL) {
		DNPRINTF(SWM_D_MISC, "strdup: %s", strerror(errno));
		return;
	}
	p = strchr(q, ':');
	if (p != NULL)
		*p = '\0';
	ws_idx = (int)strtonum(q, 1, workspace_limit, &errstr);
	if (errstr) {
		DNPRINTF(SWM_D_MISC, "workspace idx is %s: %s", errstr, q);
		free(q);
		return;
	}
	free(q);
	a.id = ws_idx - 1;
	switchws(NULL, search_r, &a);
}

void
search_resp_search_window(const char *resp)
{
	char			*s;
	int			idx;
	const char		*errstr;
	struct search_window	*sw;

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

	s = strdup(resp);
	if (s == NULL) {
		DNPRINTF(SWM_D_MISC, "strdup: %s", strerror(errno));
		return;
	}

	idx = (int)strtonum(s, 1, INT_MAX, &errstr);
	if (errstr) {
		DNPRINTF(SWM_D_MISC, "window idx is %s: %s", errstr, s);
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

	DNPRINTF(SWM_D_MISC, "begin\n");

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
wkill(struct binding *bp, struct swm_region *r, union arg *args)
{
	(void)bp;

	DNPRINTF(SWM_D_MISC, "win %#x, id: %d\n", WINID(r->ws->focus),
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

	if (ws == NULL)
		goto out;

	DNPRINTF(SWM_D_MISC, "ws: %d\n", ws->idx);

	if (ws->cur_layout == &layouts[SWM_MAX_STACK])
		goto out;

	/* Clear any maximized win(s) on ws, from bottom up. */
	TAILQ_FOREACH_REVERSE(w, &ws->stack, ws_win_stack, stack_entry)
		if (MAXIMIZED(w)) {
			ewmh_apply_flags(w, w->ewmh_flags & ~EWMH_F_MAXIMIZED);
			ewmh_update_wm_state(w);
			++count;
		}
out:
	return (count);
}

void
maximize_toggle(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*w = r->ws->focus;

	/* suppress unused warning since var is needed */
	(void)bp;
	(void)args;

	if (w == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "win %#x\n", w->id);

	if (FULLSCREEN(w))
		return;

	if (w->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return;

	ewmh_apply_flags(w, w->ewmh_flags ^ EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(w);

	stack(r);

	if (w == w->ws->focus)
		focus_win(w);

	center_pointer(r);
	focus_flush();
	DNPRINTF(SWM_D_MISC, "done\n");
}

void
floating_toggle(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*w = r->ws->focus;

	/* suppress unused warning since var is needed */
	(void)bp;
	(void)args;

	if (w == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "win %#x\n", w->id);

	if (FULLSCREEN(w) || TRANS(w))
		return;

	if (w->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return;

	ewmh_apply_flags(w, w->ewmh_flags ^ EWMH_F_ABOVE);
	ewmh_update_wm_state(w);

	stack(r);

	if (w == w->ws->focus)
		focus_win(w);

	center_pointer(r);
	focus_flush();
	DNPRINTF(SWM_D_MISC, "done\n");
}

void
fullscreen_toggle(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*w = r->ws->focus;

	/* suppress unused warning since var is needed */
	(void)bp;
	(void)args;

	if (w == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "win %#x\n", w->id);

	ewmh_apply_flags(w, w->ewmh_flags ^ EWMH_F_FULLSCREEN);
	ewmh_update_wm_state(w);

	stack(r);

	if (w == w->ws->focus)
		focus_win(w);

	center_pointer(r);
	focus_flush();
	DNPRINTF(SWM_D_MISC, "done\n");
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
	rt = (opts & SWM_CW_RIGHT) ? MAX_X(win) - MAX_X(r) : bw;
	lt = (opts & SWM_CW_LEFT) ? X(r) - X(win) : bw;
	bm = (opts & SWM_CW_BOTTOM) ? MAX_Y(win) - MAX_Y(r) : bw;
	tp = (opts & SWM_CW_TOP) ? Y(r) - Y(win) : bw;

	DNPRINTF(SWM_D_MISC, "win %#x, rt: %d, lt: %d, bm: %d, tp: %d, "
	    "SOFTBOUNDARY: %s, HARDBOUNDARY: %s\n", win->id, rt, lt, bm, tp,
	    YESNO(opts & SWM_CW_SOFTBOUNDARY),
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
	DNPRINTF(SWM_D_MISC, "win %#x, (x,y) w x h: (%d,%d) %d x %d, "
	    "box: (x,y) w x h: (%d,%d) %d x %d, rt: %s, lt: %s, bt: %s, "
	    "tp: %s, allow resize: %s\n", win->id, X(win), Y(win), WIDTH(win),
	    HEIGHT(win), b->x, b->y, b->w, b->h, YESNO(*opts & SWM_CW_RIGHT),
	    YESNO(*opts & SWM_CW_LEFT), YESNO(*opts & SWM_CW_BOTTOM),
	    YESNO(*opts & SWM_CW_TOP), YESNO(*opts & SWM_CW_RESIZABLE));

	if ((*opts & SWM_CW_RIGHT) && MAX_X(win) > b->x + b->w) {
		if (*opts & SWM_CW_RESIZABLE)
			WIDTH(win) = b->x + b->w - X(win);
		else
			X(win) = b->x + b->w - WIDTH(win);
	}

	if ((*opts & SWM_CW_LEFT) && X(win) < b->x) {
		if (*opts & SWM_CW_RESIZABLE)
			WIDTH(win) -= b->x - X(win);

		X(win) = b->x;
	}

	if ((*opts & SWM_CW_BOTTOM) && MAX_Y(win) > b->y + b->h) {
		if (*opts & SWM_CW_RESIZABLE)
			HEIGHT(win) = b->y + b->h - Y(win);
		else
			Y(win) = b->y + b->h - HEIGHT(win);
	}

	if ((*opts & SWM_CW_TOP) && Y(win) < b->y) {
		if (*opts & SWM_CW_RESIZABLE)
			HEIGHT(win) -= b->y - Y(win);

		Y(win) = b->y;
	}

	if (*opts & SWM_CW_RESIZABLE) {
		if (WIDTH(win) < 1)
			WIDTH(win) = 1;
		if (HEIGHT(win) < 1)
			HEIGHT(win) = 1;
	}
}

void
draw_frame(struct ws_win *win)
{
	xcb_rectangle_t		rect[4];
	uint32_t		*pixel;
	int			n = 0;

	if (win == NULL)
		return;

	if (win->frame == XCB_WINDOW_NONE) {
		DNPRINTF(SWM_D_EVENT, "win %#x not reparented\n", win->id);
		return;
	}

	if (!win->bordered) {
		DNPRINTF(SWM_D_EVENT, "win %#x frame disabled\n", win->id);
	}

	if (WS_FOCUSED(win->ws) && win->ws->focus == win)
		pixel = MAXIMIZED(win) ?
		    &win->s->c[SWM_S_COLOR_FOCUS_MAXIMIZED].pixel :
		    &win->s->c[SWM_S_COLOR_FOCUS].pixel;
	else
		pixel = MAXIMIZED(win) ?
		    &win->s->c[SWM_S_COLOR_UNFOCUS_MAXIMIZED].pixel :
		    &win->s->c[SWM_S_COLOR_UNFOCUS].pixel;

	/* Top (with corners) */
	rect[n].x = 0;
	rect[n].y = 0;
	rect[n].width = WIDTH(win) + 2 * border_width;
	rect[n].height = border_width;
	/* Left (without corners) */
	rect[++n].x = 0;
	rect[n].y = border_width;
	rect[n].width = border_width;
	rect[n].height = HEIGHT(win);
	/* Right (without corners) */
	rect[++n].x = border_width + WIDTH(win);
	rect[n].y = border_width;
	rect[n].width = border_width;
	rect[n].height = HEIGHT(win);
	/* Bottom (with corners)*/
	rect[++n].x = 0;
	rect[n].y = border_width + HEIGHT(win);
	rect[n].width = WIDTH(win) + 2 * border_width;
	rect[n].height = border_width;

	xcb_change_gc(conn, win->s->gc, XCB_GC_FOREGROUND, pixel);
	xcb_poly_fill_rectangle(conn, win->frame, win->s->gc, 4, rect);
}

void
update_window(struct ws_win *win)
{
	uint16_t	mask;
	uint32_t	wc[5];

	if (win->frame == XCB_WINDOW_NONE) {
		DNPRINTF(SWM_D_EVENT, "skip win %#x; not reparented\n",
		    win->id);
		return;
	}

	mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
	    XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
	    XCB_CONFIG_WINDOW_BORDER_WIDTH;

	/* Reconfigure frame. */
	if (win->bordered) {
		wc[0] = X(win) - border_width;
		wc[1] = Y(win) - border_width;
		wc[2] = WIDTH(win) + 2 * border_width;
		wc[3] = HEIGHT(win) + 2 * border_width;
	} else {
		wc[0] = X(win);
		wc[1] = Y(win);
		wc[2] = WIDTH(win);
		wc[3] = HEIGHT(win);
	}

	wc[4] = 0;

	DNPRINTF(SWM_D_EVENT, "win %#x frame %#x, (x,y) w x h: (%d,%d) %d x %d,"
	    " bordered: %s\n", win->id, win->frame, wc[0], wc[1], wc[2], wc[3],
	    YESNO(win->bordered));

	xcb_configure_window(conn, win->frame, mask, wc);

	/* Reconfigure client window. */
	wc[0] = wc[1] = win->bordered ? border_width : 0;
	wc[2] = WIDTH(win);
	wc[3] = HEIGHT(win);

	DNPRINTF(SWM_D_EVENT, "win %#x, (x,y) w x h: (%d,%d) %d x %d, "
	    "bordered: %s\n", win->id, wc[0], wc[1], wc[2], wc[3],
	    YESNO(win->bordered));
	xcb_configure_window(conn, win->id, mask, wc);

	/*
	 * ICCCM 4.2.3 send a synthetic ConfigureNotify to the window with its
	 * geometry in root coordinates. It's redundant when a window is
	 * resized, but Java has special needs...
	 */
	config_win(win, NULL);

	win->g_prev = win->g;
}

struct event {
	STAILQ_ENTRY(event)	entry;
	xcb_generic_event_t	*ev;
};
STAILQ_HEAD(event_queue, event) events = STAILQ_HEAD_INITIALIZER(events);

xcb_generic_event_t *
get_next_event(bool dowait)
{
	struct event		*ep;
	xcb_generic_event_t	*evt;

	/* Try queue first. */
	if ((ep = STAILQ_FIRST(&events))) {
		evt = ep->ev;
		STAILQ_REMOVE_HEAD(&events, entry);
		free(ep);
	} else if (dowait)
		evt = xcb_wait_for_event(conn);
	else
		evt = xcb_poll_for_event(conn);

	return (evt);
}

void
put_back_event(xcb_generic_event_t *evt)
{
	struct event	*ep;
	if ((ep = malloc(sizeof (struct event))) == NULL)
		err(1, "put_back_event: failed to allocate memory.");
	ep->ev = evt;
	STAILQ_INSERT_HEAD(&events, ep, entry);
}

/* Peeks at next event to detect auto-repeat. */
bool
keyrepeating(xcb_key_release_event_t *kre)
{
	xcb_generic_event_t	*evt;

	/* Ensure repeating keypress is finished processing. */
	xcb_aux_sync(conn);

	if ((evt = get_next_event(false))) {
		put_back_event(evt);

		if (XCB_EVENT_RESPONSE_TYPE(evt) == XCB_KEY_PRESS &&
		   kre->sequence == evt->sequence &&
		   kre->detail == ((xcb_key_press_event_t *)evt)->detail)
			return (true);
	}

	return (false);
}

bool
keybindreleased(struct binding *bp, xcb_key_release_event_t *kre)
{
	if (bp->type == KEYBIND && !keyrepeating(kre) &&
		bp->value == xcb_key_press_lookup_keysym(syms, kre, 0))
		return (true);

	return (false);
}

#define SWM_RESIZE_STEPS	(50)

void
resize_win(struct ws_win *win, struct binding *bp, int opt)
{
	xcb_timestamp_t		timestamp = 0, mintime;
	struct swm_region	*r = NULL;
	struct swm_geometry	g;
	int			top = 0, left = 0;
	int			dx, dy;
	xcb_cursor_t			cursor;
	xcb_query_pointer_reply_t	*xpr = NULL;
	xcb_generic_event_t		*evt;
	xcb_motion_notify_event_t	*mne;
	bool			resizing, step = false;

	if (win == NULL)
		return;
	r = win->ws->r;

	if (FULLSCREEN(win))
		return;

	/* In max_stack mode, should only resize transients. */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK] && !TRANS(win))
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, floating: %s, transient: %#x\n",
	    win->id, YESNO(ABOVE(win)), win->transient);

	if (MAXIMIZED(win))
		store_float_geom(win);
	else if (!(TRANS(win) || ABOVE(win)))
		return;

	ewmh_apply_flags(win, (win->ewmh_flags | SWM_F_MANUAL | EWMH_F_ABOVE) &
	    ~EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(win);

	stack(r);

	focus_flush();

	/* It's possible for win to have been freed during focus_flush(). */
	if (validate_win(win)) {
		DNPRINTF(SWM_D_EVENT, "invalid win\n");
		goto out;
	}

	switch (opt) {
	case SWM_ARG_ID_WIDTHSHRINK:
		WIDTH(win) -= SWM_RESIZE_STEPS;
		step = true;
		break;
	case SWM_ARG_ID_WIDTHGROW:
		WIDTH(win) += SWM_RESIZE_STEPS;
		step = true;
		break;
	case SWM_ARG_ID_HEIGHTSHRINK:
		HEIGHT(win) -= SWM_RESIZE_STEPS;
		step = true;
		break;
	case SWM_ARG_ID_HEIGHTGROW:
		HEIGHT(win) += SWM_RESIZE_STEPS;
		step = true;
		break;
	default:
		break;
	}
	if (step) {
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

	mintime = 1000 / r->s->rate;

	g = win->g;

	if (xpr->win_x < WIDTH(win) / 2)
		left = 1;

	if (xpr->win_y < HEIGHT(win) / 2)
		top = 1;

	if (opt == SWM_ARG_ID_CENTER)
		cursor = cursors[XC_SIZING].cid;
	else if (top)
		cursor = cursors[left ? XC_TOP_LEFT_CORNER :
		    XC_TOP_RIGHT_CORNER].cid;
	else
		cursor = cursors[left ? XC_BOTTOM_LEFT_CORNER :
		    XC_BOTTOM_RIGHT_CORNER].cid;

	xcb_grab_pointer(conn, 0, win->id, MOUSEMASK,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE, cursor,
	    XCB_CURRENT_TIME);

	/* Release keyboard freeze if called via keybind. */
	if (bp->type == KEYBIND)
		xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
		    XCB_CURRENT_TIME);

	xcb_flush(conn);
	resizing = true;
	while (resizing && (evt = get_next_event(true))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_BUTTON_RELEASE:
			if (bp->type == BTNBIND && bp->value ==
			    ((xcb_button_release_event_t *)evt)->detail)
				resizing = false;
			break;
		case XCB_KEY_RELEASE:
			if (keybindreleased(bp, (xcb_key_release_event_t *)evt))
				resizing = false;
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)evt;
			DNPRINTF(SWM_D_EVENT, "MOTION_NOTIFY: root: %#x\n",
			    mne->root);

			/* cursor offset/delta from start of the operation */
			dx = mne->root_x - xpr->root_x;
			dy = mne->root_y - xpr->root_y;

			/* vertical */
			if (top)
				dy = -dy;

			if (opt == SWM_ARG_ID_CENTER) {
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

			if (opt == SWM_ARG_ID_CENTER) {
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

			/* Don't sync faster than the current rate limit. */
			if ((mne->time - timestamp) > mintime) {
				timestamp = mne->time;
				regionize(win, mne->root_x, mne->root_y);
				region_containment(win, r, SWM_CW_ALLSIDES |
				    SWM_CW_RESIZABLE | SWM_CW_HARDBOUNDARY |
				    SWM_CW_SOFTBOUNDARY);
				update_window(win);
				xcb_flush(conn);
			}
			break;
		case XCB_BUTTON_PRESS:
			/* Ignore. */
			DNPRINTF(SWM_D_EVENT, "BUTTON_PRESS ignored\n");
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_POINTER,
			    ((xcb_button_press_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
			/* Ignore. */
			DNPRINTF(SWM_D_EVENT, "KEY_PRESS ignored\n");
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    ((xcb_key_press_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		default:
			event_handle(evt);

			/* It's possible for win to have been freed above. */
			if (validate_win(win)) {
				DNPRINTF(SWM_D_EVENT, "invalid win\n");
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
	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
resize(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r == NULL)
		return;

	if (args->id != SWM_ARG_ID_DONTCENTER &&
	    args->id != SWM_ARG_ID_CENTER) {
		/* keyboard resize uses the focus window. */
		if (r->ws)
			win = r->ws->focus;
	} else
		/* mouse resize uses pointer window. */
		win = get_pointer_win(r->s);

	if (win == NULL)
		return;

	resize_win(win, bp, args->id);

	if (args->id && bp->type == KEYBIND)
		center_pointer(r);

	focus_flush();
}

/* Try to set window region based on supplied coordinates or window center. */
void
regionize(struct ws_win *win, int x, int y)
{
	struct swm_region *r, *r_orig;

	if (win == NULL)
		return;

	r_orig = win->ws->r;

	r = region_under(win->s, x, y);
	if (r == NULL)
		r = region_under(win->s, X(win) + WIDTH(win) / 2,
		    Y(win) + HEIGHT(win) / 2);

	if (r && r != r_orig) {
		clear_maximized(r->ws);

		win_to_ws(win, r->ws->idx, 0);

		/* Need to restack both regions. */
		stack(r_orig);
		stack(r);

		/* Set focus on new ws. */
		unfocus_win(r->ws->focus);
		r->ws->focus = win;

		set_region(r);
		raise_window(win);
		update_win_stacking(win);
	}
}

#define SWM_MOVE_STEPS	(50)

void
move_win(struct ws_win *win, struct binding *bp, int opt)
{
	struct swm_region		*r;
	xcb_timestamp_t			timestamp = 0, mintime;
	xcb_query_pointer_reply_t	*qpr = NULL;
	xcb_generic_event_t		*evt;
	xcb_motion_notify_event_t	*mne;
	bool				moving, restack = false, step = false;

	if (win == NULL)
		return;

	if ((r = win->ws->r) == NULL)
		return;

	if (FULLSCREEN(win))
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, floating: %s, transient: %#x\n",
	    win->id, YESNO(ABOVE(win)), win->transient);

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
		stack(r);

	focus_flush();

	/* It's possible for win to have been freed during focus_flush(). */
	if (validate_win(win)) {
		DNPRINTF(SWM_D_EVENT, "invalid win.\n");
		goto out;
	}

	switch (opt) {
	case SWM_ARG_ID_MOVELEFT:
		X(win) -= (SWM_MOVE_STEPS - border_width);
		step = true;
		break;
	case SWM_ARG_ID_MOVERIGHT:
		X(win) += (SWM_MOVE_STEPS - border_width);
		step = true;
		break;
	case SWM_ARG_ID_MOVEUP:
		Y(win) -= (SWM_MOVE_STEPS - border_width);
		step = true;
		break;
	case SWM_ARG_ID_MOVEDOWN:
		Y(win) += (SWM_MOVE_STEPS - border_width);
		step = true;
		break;
	default:
		break;
	}
	if (step) {
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

	/* Release keyboard freeze if called via keybind. */
	if (bp->type == KEYBIND)
		xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
		     XCB_CURRENT_TIME);

	mintime = 1000 / r->s->rate;

	regionize(win, qpr->root_x, qpr->root_y);
	region_containment(win, win->ws->r, SWM_CW_ALLSIDES |
	    SWM_CW_SOFTBOUNDARY);
	update_window(win);
	xcb_flush(conn);
	moving = true;
	while (moving && (evt = get_next_event(true))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_BUTTON_RELEASE:
			if (bp->type == BTNBIND && bp->value ==
			    ((xcb_button_release_event_t *)evt)->detail)
				moving = false;

			xcb_allow_events(conn, XCB_ALLOW_ASYNC_POINTER,
			    ((xcb_button_release_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		case XCB_KEY_RELEASE:
			if (keybindreleased(bp, (xcb_key_release_event_t *)evt))
				moving = false;

			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    ((xcb_key_release_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)evt;
			DNPRINTF(SWM_D_EVENT, "MOTION_NOTIFY: root: %#x time: "
			    "%#x\n", mne->root, mne->time);
			X(win) = mne->root_x - qpr->win_x;
			Y(win) = mne->root_y - qpr->win_y;

			/* Don't sync faster than the current rate limit. */
			if ((mne->time - timestamp) > mintime) {
				timestamp = mne->time;
				regionize(win, mne->root_x, mne->root_y);
				region_containment(win, win->ws->r,
				    SWM_CW_ALLSIDES | SWM_CW_SOFTBOUNDARY);
				update_window(win);
				xcb_flush(conn);
			}
			break;
		case XCB_BUTTON_PRESS:
			/* Thaw and ignore. */
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_POINTER,
			    ((xcb_button_press_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
			/* Ignore. */
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    ((xcb_key_press_event_t *)evt)->time);
			xcb_flush(conn);
			break;
		default:
			event_handle(evt);

			/* It's possible for win to have been freed. */
			if (validate_win(win)) {
				DNPRINTF(SWM_D_EVENT, "invalid win\n");
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

	/* New region set to max layout. */
	if (win->ws->r && win->ws->cur_layout == &layouts[SWM_MAX_STACK]) {
		stack(win->ws->r);
		focus_flush();
	}

out:
	free(qpr);
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
move(struct binding *bp, struct swm_region *r, union arg *args)
{
	struct ws_win			*win = NULL;

	if (r == NULL)
		return;

	if (args->id) {
		/* Keyboard move uses the focus window. */
		if (r->ws)
			win = r->ws->focus;

		/* Disallow move_ on tiled. */
		if (win && !(TRANS(win) || ABOVE(win)))
			return;
	} else
		/* Mouse move uses the pointer window. */
		win = get_pointer_win(r->s);

	if (win == NULL)
		return;

	move_win(win, bp, args->id);

	if (args->id && bp->type == KEYBIND)
		center_pointer(r);

	focus_flush();
}

/* action definitions */
struct action {
	char			name[SWM_FUNCNAME_LEN];
	void			(*func)(struct binding *, struct swm_region *,
				    union arg *);
	uint32_t		flags;
	union arg		args;
} actions[FN_INVALID + 2] = {
	/* name			function	argument */
	{ "bar_toggle",		bar_toggle,	0, {.id = SWM_ARG_ID_BAR_TOGGLE} },
	{ "bar_toggle_ws",	bar_toggle,	0, {.id = SWM_ARG_ID_BAR_TOGGLE_WS} },
	{ "button2",		pressbutton,	0, {.id = 2} },
	{ "cycle_layout",	switchlayout,	0, {.id = SWM_ARG_ID_CYCLE_LAYOUT} },
	{ "flip_layout",	stack_config,	0, {.id = SWM_ARG_ID_FLIPLAYOUT} },
	{ "float_toggle",	floating_toggle,0, {0} },
	{ "focus",		focus_pointer,	0, {0} },
	{ "focus_main",		focus,		0, {.id = SWM_ARG_ID_FOCUSMAIN} },
	{ "focus_next",		focus,		0, {.id = SWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		0, {.id = SWM_ARG_ID_FOCUSPREV} },
	{ "focus_urgent",	focus,		0, {.id = SWM_ARG_ID_FOCUSURGENT} },
	{ "fullscreen_toggle",	fullscreen_toggle, 0, {0} },
	{ "maximize_toggle",	maximize_toggle,0, {0} },
	{ "height_grow",	resize,		0, {.id = SWM_ARG_ID_HEIGHTGROW} },
	{ "height_shrink",	resize,		0, {.id = SWM_ARG_ID_HEIGHTSHRINK} },
	{ "iconify",		iconify,	0, {0} },
	{ "layout_vertical",	switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_VERTICAL} },
	{ "layout_horizontal",	switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_HORIZONTAL} },
	{ "layout_max"	,	switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_MAX} },
	{ "master_shrink",	stack_config,	0, {.id = SWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	0, {.id = SWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	0, {.id = SWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	0, {.id = SWM_ARG_ID_MASTERDEL} },
	{ "move",		move,		FN_F_NOREPLAY, {0} },
	{ "move_down",		move,		0, {.id = SWM_ARG_ID_MOVEDOWN} },
	{ "move_left",		move,		0, {.id = SWM_ARG_ID_MOVELEFT} },
	{ "move_right",		move,		0, {.id = SWM_ARG_ID_MOVERIGHT} },
	{ "move_up",		move,		0, {.id = SWM_ARG_ID_MOVEUP} },
	{ "mvrg_1",		send_to_rg,	0, {.id = 0} },
	{ "mvrg_2",		send_to_rg,	0, {.id = 1} },
	{ "mvrg_3",		send_to_rg,	0, {.id = 2} },
	{ "mvrg_4",		send_to_rg,	0, {.id = 3} },
	{ "mvrg_5",		send_to_rg,	0, {.id = 4} },
	{ "mvrg_6",		send_to_rg,	0, {.id = 5} },
	{ "mvrg_7",		send_to_rg,	0, {.id = 6} },
	{ "mvrg_8",		send_to_rg,	0, {.id = 7} },
	{ "mvrg_9",		send_to_rg,	0, {.id = 8} },
	{ "mvrg_next",		send_to_rg_relative,	0, {.id = 1} },
	{ "mvrg_prev",		send_to_rg_relative,	0, {.id = -1} },
	{ "mvws_1",		send_to_ws,	0, {.id = 0} },
	{ "mvws_2",		send_to_ws,	0, {.id = 1} },
	{ "mvws_3",		send_to_ws,	0, {.id = 2} },
	{ "mvws_4",		send_to_ws,	0, {.id = 3} },
	{ "mvws_5",		send_to_ws,	0, {.id = 4} },
	{ "mvws_6",		send_to_ws,	0, {.id = 5} },
	{ "mvws_7",		send_to_ws,	0, {.id = 6} },
	{ "mvws_8",		send_to_ws,	0, {.id = 7} },
	{ "mvws_9",		send_to_ws,	0, {.id = 8} },
	{ "mvws_10",		send_to_ws,	0, {.id = 9} },
	{ "mvws_11",		send_to_ws,	0, {.id = 10} },
	{ "mvws_12",		send_to_ws,	0, {.id = 11} },
	{ "mvws_13",		send_to_ws,	0, {.id = 12} },
	{ "mvws_14",		send_to_ws,	0, {.id = 13} },
	{ "mvws_15",		send_to_ws,	0, {.id = 14} },
	{ "mvws_16",		send_to_ws,	0, {.id = 15} },
	{ "mvws_17",		send_to_ws,	0, {.id = 16} },
	{ "mvws_18",		send_to_ws,	0, {.id = 17} },
	{ "mvws_19",		send_to_ws,	0, {.id = 18} },
	{ "mvws_20",		send_to_ws,	0, {.id = 19} },
	{ "mvws_21",		send_to_ws,	0, {.id = 20} },
	{ "mvws_22",		send_to_ws,	0, {.id = 21} },
	{ "name_workspace",	name_workspace,	0, {0} },
	{ "quit",		quit,		0, {0} },
	{ "raise",		raise_focus,	0, {0} },
	{ "raise_toggle",	raise_toggle,	0, {0} },
	{ "resize",		resize, FN_F_NOREPLAY, {.id = SWM_ARG_ID_DONTCENTER} },
	{ "resize_centered",	resize, FN_F_NOREPLAY, {.id = SWM_ARG_ID_CENTER} },
	{ "restart",		restart,	0, {0} },
	{ "restart_of_day",	restart,	0, {SWM_ARG_ID_RESTARTOFDAY} },
	{ "rg_1",		focusrg,	0, {.id = 0} },
	{ "rg_2",		focusrg,	0, {.id = 1} },
	{ "rg_3",		focusrg,	0, {.id = 2} },
	{ "rg_4",		focusrg,	0, {.id = 3} },
	{ "rg_5",		focusrg,	0, {.id = 4} },
	{ "rg_6",		focusrg,	0, {.id = 5} },
	{ "rg_7",		focusrg,	0, {.id = 6} },
	{ "rg_8",		focusrg,	0, {.id = 7} },
	{ "rg_9",		focusrg,	0, {.id = 8} },
	{ "rg_move_next",	cyclerg,	0, {.id = SWM_ARG_ID_CYCLERG_MOVE_UP} },
	{ "rg_move_prev",	cyclerg,	0, {.id = SWM_ARG_ID_CYCLERG_MOVE_DOWN} },
	{ "rg_next",		cyclerg,	0, {.id = SWM_ARG_ID_CYCLERG_UP} },
	{ "rg_prev",		cyclerg,	0, {.id = SWM_ARG_ID_CYCLERG_DOWN} },
	{ "screen_next",	cyclerg,	0, {.id = SWM_ARG_ID_CYCLERG_UP} },
	{ "screen_prev",	cyclerg,	0, {.id = SWM_ARG_ID_CYCLERG_DOWN} },
	{ "search_win",		search_win,	0, {0} },
	{ "search_workspace",	search_workspace,	0, {0} },
	{ "spawn_custom",	NULL,		0, {0} },
	{ "stack_balance",	stack_config,	0, {.id = SWM_ARG_ID_STACKBALANCE} },
	{ "stack_inc",		stack_config,	0, {.id = SWM_ARG_ID_STACKINC} },
	{ "stack_dec",		stack_config,	0, {.id = SWM_ARG_ID_STACKDEC} },
	{ "stack_reset",	stack_config,	0, {.id = SWM_ARG_ID_STACKRESET} },
	{ "swap_main",		swapwin,	0, {.id = SWM_ARG_ID_SWAPMAIN} },
	{ "swap_next",		swapwin,	0, {.id = SWM_ARG_ID_SWAPNEXT} },
	{ "swap_prev",		swapwin,	0, {.id = SWM_ARG_ID_SWAPPREV} },
	{ "uniconify",		uniconify,	0, {0} },
	{ "version",		version,	0, {0} },
	{ "width_grow",		resize,		0, {.id = SWM_ARG_ID_WIDTHGROW} },
	{ "width_shrink",	resize,		0, {.id = SWM_ARG_ID_WIDTHSHRINK} },
	{ "wind_del",		wkill,		0, {.id = SWM_ARG_ID_DELETEWINDOW} },
	{ "wind_kill",		wkill,		0, {.id = SWM_ARG_ID_KILLWINDOW} },
	{ "ws_1",		switchws,	0, {.id = 0} },
	{ "ws_2",		switchws,	0, {.id = 1} },
	{ "ws_3",		switchws,	0, {.id = 2} },
	{ "ws_4",		switchws,	0, {.id = 3} },
	{ "ws_5",		switchws,	0, {.id = 4} },
	{ "ws_6",		switchws,	0, {.id = 5} },
	{ "ws_7",		switchws,	0, {.id = 6} },
	{ "ws_8",		switchws,	0, {.id = 7} },
	{ "ws_9",		switchws,	0, {.id = 8} },
	{ "ws_10",		switchws,	0, {.id = 9} },
	{ "ws_11",		switchws,	0, {.id = 10} },
	{ "ws_12",		switchws,	0, {.id = 11} },
	{ "ws_13",		switchws,	0, {.id = 12} },
	{ "ws_14",		switchws,	0, {.id = 13} },
	{ "ws_15",		switchws,	0, {.id = 14} },
	{ "ws_16",		switchws,	0, {.id = 15} },
	{ "ws_17",		switchws,	0, {.id = 16} },
	{ "ws_18",		switchws,	0, {.id = 17} },
	{ "ws_19",		switchws,	0, {.id = 18} },
	{ "ws_20",		switchws,	0, {.id = 19} },
	{ "ws_21",		switchws,	0, {.id = 20} },
	{ "ws_22",		switchws,	0, {.id = 21} },
	{ "ws_empty",		emptyws,	0, {.id = SWM_ARG_ID_WS_EMPTY} },
	{ "ws_empty_move",	emptyws,	0, {.id = SWM_ARG_ID_WS_EMPTY_MOVE} },
	{ "ws_next",		cyclews,	0, {.id = SWM_ARG_ID_CYCLEWS_UP} },
	{ "ws_next_all",	cyclews,	0, {.id = SWM_ARG_ID_CYCLEWS_UP_ALL} },
	{ "ws_next_move",	cyclews,	0, {.id = SWM_ARG_ID_CYCLEWS_MOVE_UP} },
	{ "ws_prev",		cyclews,	0, {.id = SWM_ARG_ID_CYCLEWS_DOWN} },
	{ "ws_prev_all",	cyclews,	0, {.id = SWM_ARG_ID_CYCLEWS_DOWN_ALL} },
	{ "ws_prev_move",	cyclews,	0, {.id = SWM_ARG_ID_CYCLEWS_MOVE_DOWN} },
	{ "ws_prior",		priorws,	0, {0} },
	/* SWM_DEBUG actions MUST be here: */
	{ "debug_toggle",	debug_toggle,	0, {0} },
	{ "dumpwins",		dumpwins,	0, {0} },
	/* ALWAYS last: */
	{ "invalid action",	NULL,		0, {0} },
};

void
update_modkey(uint16_t mod)
{
	struct binding		*bp;

	/* Replace all instances of the old mod key. */
	RB_FOREACH(bp, binding_tree, &bindings)
		if (bp->mod & mod_key)
			bp->mod = (bp->mod & ~mod_key) | mod;
	mod_key = mod;
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

	DNPRINTF(SWM_D_SPAWN, "%s\n", spawn_name);

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
		DNPRINTF(SWM_D_SPAWN, "raw arg: %s\n", ap);
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
		} else if (strcasecmp(ap, "$bar_color_selected") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_SELECTED].name))
			    == NULL)
				err(1, "spawn_custom bar color selected");
		} else if (strcasecmp(ap, "$bar_font") == 0) {
			if ((real_args[c] = strdup(bar_fonts))
			    == NULL)
				err(1, "spawn_custom bar fonts");
		} else if (strcasecmp(ap, "$bar_font_color") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_FONT].name))
			    == NULL)
				err(1, "spawn_custom color font");
		} else if (strcasecmp(ap, "$bar_font_color_selected") == 0) {
			if ((real_args[c] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_FONT_SELECTED].name))
			    == NULL)
				err(1, "spawn_custom bar font color selected");
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
		DNPRINTF(SWM_D_SPAWN, "cooked arg: %s\n", real_args[c]);
		++c;
	}

#ifdef SWM_DEBUG
	DNPRINTF(SWM_D_SPAWN, "result: ");
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
		return (NULL);

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

	return (arg);
}

/* Process escape chars in string and return allocated result. */
char *
unescape_value(const char *value) {
	const char		*vp;
	char			*result, *rp;
	bool			single_quoted = false, double_quoted = false;

	if (value == NULL)
		return (NULL);

	result = malloc(strlen(value) + 1);
	if (result == NULL)
		err(1, "unescape_value: malloc");

	for (rp = result, vp = value; *vp != '\0'; ++vp) {
		if (*vp == '\'' && !double_quoted)
			single_quoted = !single_quoted;
		else if (*vp == '\"' && !single_quoted)
			double_quoted = !double_quoted;
		else if (*vp == '\\' && ((single_quoted && *(vp + 1) == '\'') ||
		    (double_quoted && *(vp + 1) == '\"') ||
		    (!single_quoted && !double_quoted)))
			*rp++ = *(++vp);
		else
			*rp++ = *vp;
	}

	/* Ensure result is terminated. */
	*rp = '\0';

	return (result);
}

void
spawn_insert(const char *name, const char *args, int flags)
{
	struct spawn_prog	*sp;
	char			*arg, *cp, *ptr;

	DNPRINTF(SWM_D_SPAWN, "%s[%s]\n", name, args);

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

#ifdef SWM_DEBUG
	if (sp->argv != NULL) {
		DNPRINTF(SWM_D_SPAWN, "arg %d: [%s]\n", sp->argc,
		    sp->argv[sp->argc-1]);
	}
#endif
	TAILQ_INSERT_TAIL(&spawns, sp, entry);
	DNPRINTF(SWM_D_SPAWN, "leave\n");
}

void
spawn_remove(struct spawn_prog *sp)
{
	int			i;

	DNPRINTF(SWM_D_SPAWN, "name: %s\n", sp->name);

	TAILQ_REMOVE(&spawns, sp, entry);
	for (i = 0; i < sp->argc; i++)
		free(sp->argv[i]);
	free(sp->argv);
	free(sp->name);
	free(sp);

	DNPRINTF(SWM_D_SPAWN, "leave\n");
}

void
clear_spawns(void)
{
	struct spawn_prog	*sp;
#ifndef __clang_analyzer__ /* Suppress false warnings. */
	while ((sp = TAILQ_FIRST(&spawns)) != NULL) {
		spawn_remove(sp);
	}
#endif
}

struct spawn_prog*
spawn_find(const char *name)
{
	struct spawn_prog	*sp;

	TAILQ_FOREACH(sp, &spawns, entry)
		if (strcasecmp(sp->name, name) == 0)
			return (sp);

	return (NULL);
}

void
setspawn(const char *name, const char *args, int flags)
{
	struct spawn_prog	*sp;

	DNPRINTF(SWM_D_SPAWN, "name: %s\n", name);

	if (name == NULL)
		return;

#ifndef __clang_analyzer__ /* Suppress false warnings. */
	/* Remove any old spawn under the same name. */
	if ((sp = spawn_find(name)) != NULL)
		spawn_remove(sp);
#endif

	if (*args != '\0')
		spawn_insert(name, args, flags);
	else
		warnx("error: setspawn: cannot find program: %s", name);

	DNPRINTF(SWM_D_SPAWN, "leave\n");
}

int
setconfspawn(const char *selector, const char *value, int flags, char **emsg)
{
	char		*args;

	if (selector == NULL || strlen(selector) == 0) {
		ALLOCSTR(emsg, "missing selector");
		return (1);
	}

	args = expand_tilde(value);

	DNPRINTF(SWM_D_SPAWN, "[%s] [%s]\n", selector, args);

	setspawn(selector, args, flags);
	free(args);

	DNPRINTF(SWM_D_SPAWN, "done\n");
	return (0);
}

void
validate_spawns(void)
{
	struct binding		*bp;
	struct spawn_prog	*sp;
	char			which[PATH_MAX];
	size_t			i;

	RB_FOREACH(bp, binding_tree, &bindings) {
		if (bp->action != FN_SPAWN_CUSTOM)
			continue;

		/* find program */
		sp = spawn_find(bp->spawn_name);
		if (sp == NULL || sp->flags & SWM_SPAWN_OPTIONAL)
			continue;

		/* verify we have the goods */
		snprintf(which, sizeof which, "which %s", sp->argv[0]);
		DNPRINTF(SWM_D_CONF, "which %s\n", sp->argv[0]);
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
	setconfspawn("lock", "xlock", 0, NULL);

	setconfspawn("term", "xterm", 0, NULL);
	setconfspawn("spawn_term", "xterm", 0, NULL);

	setconfspawn("menu", "dmenu_run $dmenu_bottom -fn $bar_font "
	    "-nb $bar_color -nf $bar_font_color -sb $bar_color_selected "
	    "-sf $bar_font_color_selected", 0, NULL);

	setconfspawn("search", "dmenu $dmenu_bottom -i -fn $bar_font "
	    "-nb $bar_color -nf $bar_font_color -sb $bar_color_selected "
	    "-sf $bar_font_color_selected", 0, NULL);

	setconfspawn("name_workspace", "dmenu $dmenu_bottom -p Workspace "
	    "-fn $bar_font -nb $bar_color -nf $bar_font_color "
	    "-sb $bar_color_selected -sf $bar_font_color_selected", 0, NULL);

	 /* These are not verified for existence, even with a binding set. */
	setconfspawn("screenshot_all", "screenshot.sh full",
	    SWM_SPAWN_OPTIONAL, NULL);
	setconfspawn("screenshot_wind", "screenshot.sh window",
	    SWM_SPAWN_OPTIONAL, NULL);
	setconfspawn("initscr", "initscreen.sh", SWM_SPAWN_OPTIONAL, NULL);
}

/* bindings */
#define SWM_MODNAME_SIZE	32
#define SWM_KEY_WS		"\n+ \t"
int
parsebinding(const char *bindstr, uint16_t *mod, enum binding_type *type,
    uint32_t *val, uint32_t *flags, char **emsg)
{
	char			*str, *cp, *name;
	KeySym			ks, lks, uks;

	DNPRINTF(SWM_D_KEY, "enter [%s]\n", bindstr);
	if (mod == NULL || val == NULL) {
		DNPRINTF(SWM_D_KEY, "no mod or key vars\n");
		return (1);
	}
	if (bindstr == NULL || strlen(bindstr) == 0) {
		DNPRINTF(SWM_D_KEY, "no bindstr\n");
		return (1);
	}

	if ((cp = str = strdup(bindstr)) == NULL)
		err(1, "parsebinding: strdup");

	*val = XCB_NO_SYMBOL;
	*mod = 0;
	*flags = 0;
	*type = KEYBIND;
	while ((name = strsep(&cp, SWM_KEY_WS)) != NULL) {
		DNPRINTF(SWM_D_KEY, "entry [%s]\n", name);
		if (cp)
			cp += (long)strspn(cp, SWM_KEY_WS);
		if (strncasecmp(name, "MOD", SWM_MODNAME_SIZE) == 0)
			*mod |= mod_key;
		else if (strncasecmp(name, "Mod1", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_1;
		else if (strncasecmp(name, "Mod2", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_2;
		else if (strncmp(name, "Mod3", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_3;
		else if (strncmp(name, "Mod4", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_4;
		else if (strncmp(name, "Mod5", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_5;
		else if (strncasecmp(name, "SHIFT", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_SHIFT;
		else if (strncasecmp(name, "CONTROL", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_CONTROL;
		else if (strncasecmp(name, "ANYMOD", SWM_MODNAME_SIZE) == 0)
			*mod |= XCB_MOD_MASK_ANY;
		else if (strncasecmp(name, "REPLAY", SWM_MODNAME_SIZE) == 0)
			*flags |= BINDING_F_REPLAY;
		else if (sscanf(name, "Button%u", val) == 1) {
			DNPRINTF(SWM_D_KEY, "button %u\n", *val);
			*type = BTNBIND;
			if (*val > 255 || *val == 0) {
				DNPRINTF(SWM_D_KEY, "invalid btn %u\n", *val);
				ALLOCSTR(emsg, "invalid button: %s", name);
				free(str);
				return (1);
			}
		} else {
			/* TODO: do this without Xlib. */
			ks = XStringToKeysym(name);
			if (ks == NoSymbol) {
				DNPRINTF(SWM_D_KEY, "invalid key %s\n", name);
				ALLOCSTR(emsg, "invalid key: %s", name);
				free(str);
				return (1);
			}

			XConvertCase(ks, &lks, &uks);
			*val = (uint32_t)lks;
		}
	}

	/* If ANYMOD was specified, ignore the rest. */
	if (*mod & XCB_MOD_MASK_ANY)
		*mod = XCB_MOD_MASK_ANY;

	free(str);
	DNPRINTF(SWM_D_KEY, "leave\n");
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

int
binding_cmp(struct binding *bp1, struct binding *bp2)
{
	if (bp1->type < bp2->type)
		return (-1);
	if (bp1->type > bp2->type)
		return (1);

	if (bp1->value < bp2->value)
		return (-1);
	if (bp1->value > bp2->value)
		return (1);

	if (bp1->mod < bp2->mod)
		return (-1);
	if (bp1->mod > bp2->mod)
		return (1);

	return (0);
}

void
binding_insert(uint16_t mod, enum binding_type type, uint32_t val,
    enum actionid aid, uint32_t flags, const char *spawn_name)
{
	struct binding		*bp;

	DNPRINTF(SWM_D_KEY, "mod: %u, type: %d, val: %u, action: %s(%d), "
	    "spawn_name: %s\n", mod, type, val, actions[aid].name, aid,
	    spawn_name);

	if ((bp = malloc(sizeof *bp)) == NULL)
		err(1, "binding_insert: malloc");

	bp->mod = mod;
	bp->type = type;
	bp->value = val;
	bp->action = aid;
	bp->flags = flags;
	bp->spawn_name = strdupsafe(spawn_name);
	RB_INSERT(binding_tree, &bindings, bp);

	DNPRINTF(SWM_D_KEY, "leave\n");
}

struct binding *
binding_lookup(uint16_t mod, enum binding_type type, uint32_t val)
{
	struct binding		bp;

	bp.mod = mod;
	bp.type = type;
	bp.value = val;

	return (RB_FIND(binding_tree, &bindings, &bp));
}

void
binding_remove(struct binding *bp)
{
	DNPRINTF(SWM_D_KEY, "mod: %u, type: %d, val: %u, action: %s(%d), "
	    "spawn_name: %s\n", bp->mod, bp->type, bp->value,
	    actions[bp->action].name, bp->action, bp->spawn_name);

	RB_REMOVE(binding_tree, &bindings, bp);
	free(bp->spawn_name);
	free(bp);

	DNPRINTF(SWM_D_KEY, "leave\n");
}

void
setbinding(uint16_t mod, enum binding_type type, uint32_t val,
    enum actionid aid, uint32_t flags, const char *spawn_name)
{
	struct binding		*bp;

#ifdef SWM_DEBUG
	if (spawn_name != NULL) {
		DNPRINTF(SWM_D_KEY, "enter %s [%s]\n", actions[aid].name,
		    spawn_name);
	}
#endif

	/* Unbind any existing. Loop is to handle MOD_MASK_ANY. */
	while ((bp = binding_lookup(mod, type, val)))
		binding_remove(bp);

	if (aid != FN_INVALID)
		binding_insert(mod, type, val, aid, flags, spawn_name);

	DNPRINTF(SWM_D_KEY, "leave\n");
}

int
setconfbinding(const char *selector, const char *value, int flags, char **emsg)
{
	struct spawn_prog	*sp;
	uint32_t		keybtn, opts;
	uint16_t		mod;
	enum actionid		aid;
	enum binding_type	type;

	/* suppress unused warning since var is needed */
	(void)flags;

	DNPRINTF(SWM_D_KEY, "selector: [%s], value: [%s]\n", selector, value);
	if (selector == NULL || strlen(selector) == 0) {
		DNPRINTF(SWM_D_KEY, "unbind %s\n", value);
		if (parsebinding(value, &mod, &type, &keybtn, &opts,
		    emsg) == 0) {
			setbinding(mod, type, keybtn, FN_INVALID, opts, NULL);
			return (0);
		} else
			return (1);
	}
	/* search by key function name */
	for (aid = 0; aid < FN_INVALID; aid++) {
		if (strncasecmp(selector, actions[aid].name,
		    SWM_FUNCNAME_LEN) == 0) {
			DNPRINTF(SWM_D_KEY, "%s: match action\n", selector);
			if (parsebinding(value, &mod, &type, &keybtn, &opts,
			    emsg) == 0) {
				setbinding(mod, type, keybtn, aid, opts, NULL);
				return (0);
			} else
				return (1);
		}
	}
	/* search by custom spawn name */
	if ((sp = spawn_find(selector)) != NULL) {
		DNPRINTF(SWM_D_KEY, "%s: match spawn\n", selector);
		if (parsebinding(value, &mod, &type, &keybtn, &opts,
		    emsg) == 0) {
			setbinding(mod, type, keybtn, FN_SPAWN_CUSTOM, opts,
			    sp->name);
			return (0);
		} else
			return (1);
	}
	DNPRINTF(SWM_D_KEY, "no match\n");
	ALLOCSTR(emsg, "invalid action: %s", selector);
	return (1);
}

#define MOD		mod_key
#define MODSHIFT	mod_key | XCB_MOD_MASK_SHIFT
void
setup_keybindings(void)
{
#define BINDKEY(m, k, a)	setbinding(m, KEYBIND, k, a, 0, NULL)
#define BINDKEYSPAWN(m, k, s)	setbinding(m, KEYBIND, k, FN_SPAWN_CUSTOM, 0, s)
	BINDKEY(MOD,		XK_b,			FN_BAR_TOGGLE);
	BINDKEY(MODSHIFT,	XK_b,			FN_BAR_TOGGLE_WS);
	BINDKEY(MOD,		XK_b,			FN_BAR_TOGGLE);
	BINDKEY(MODSHIFT,	XK_b,			FN_BAR_TOGGLE_WS);
	BINDKEY(MOD,		XK_v,			FN_BUTTON2);
	BINDKEY(MOD,		XK_space,		FN_CYCLE_LAYOUT);
	BINDKEY(MODSHIFT,	XK_backslash,		FN_FLIP_LAYOUT);
	BINDKEY(MOD,		XK_t,			FN_FLOAT_TOGGLE);
	BINDKEY(MOD,		XK_m,			FN_FOCUS_MAIN);
	BINDKEY(MOD,		XK_j,			FN_FOCUS_NEXT);
	BINDKEY(MOD,		XK_Tab,			FN_FOCUS_NEXT);
	BINDKEY(MOD,		XK_k,			FN_FOCUS_PREV);
	BINDKEY(MODSHIFT,	XK_Tab,			FN_FOCUS_PREV);
	BINDKEY(MOD,		XK_u,			FN_FOCUS_URGENT);
	BINDKEY(MODSHIFT,	XK_e,			FN_FULLSCREEN_TOGGLE);
	BINDKEY(MOD,		XK_e,			FN_MAXIMIZE_TOGGLE);
	BINDKEY(MODSHIFT,	XK_equal,		FN_HEIGHT_GROW);
	BINDKEY(MODSHIFT,	XK_minus,		FN_HEIGHT_SHRINK);
	BINDKEY(MOD,		XK_w,			FN_ICONIFY);
	BINDKEY(MOD,		XK_h,			FN_MASTER_SHRINK);
	BINDKEY(MOD,		XK_l,			FN_MASTER_GROW);
	BINDKEY(MOD,		XK_comma,		FN_MASTER_ADD);
	BINDKEY(MOD,		XK_period,		FN_MASTER_DEL);
	BINDKEY(MODSHIFT,	XK_bracketright,	FN_MOVE_DOWN);
	BINDKEY(MOD,		XK_bracketleft,		FN_MOVE_LEFT);
	BINDKEY(MOD,		XK_bracketright,	FN_MOVE_RIGHT);
	BINDKEY(MODSHIFT,	XK_bracketleft,		FN_MOVE_UP);
	BINDKEY(MODSHIFT,	XK_KP_End,		FN_MVRG_1);
	BINDKEY(MODSHIFT,	XK_KP_Down,		FN_MVRG_2);
	BINDKEY(MODSHIFT,	XK_KP_Next,		FN_MVRG_3);
	BINDKEY(MODSHIFT,	XK_KP_Left,		FN_MVRG_4);
	BINDKEY(MODSHIFT,	XK_KP_Begin,		FN_MVRG_5);
	BINDKEY(MODSHIFT,	XK_KP_Right,		FN_MVRG_6);
	BINDKEY(MODSHIFT,	XK_KP_Home,		FN_MVRG_7);
	BINDKEY(MODSHIFT,	XK_KP_Up,		FN_MVRG_8);
	BINDKEY(MODSHIFT,	XK_KP_Prior,		FN_MVRG_9);
	BINDKEY(MODSHIFT,	XK_1,			FN_MVWS_1);
	BINDKEY(MODSHIFT,	XK_2,			FN_MVWS_2);
	BINDKEY(MODSHIFT,	XK_3,			FN_MVWS_3);
	BINDKEY(MODSHIFT,	XK_4,			FN_MVWS_4);
	BINDKEY(MODSHIFT,	XK_5,			FN_MVWS_5);
	BINDKEY(MODSHIFT,	XK_6,			FN_MVWS_6);
	BINDKEY(MODSHIFT,	XK_7,			FN_MVWS_7);
	BINDKEY(MODSHIFT,	XK_8,			FN_MVWS_8);
	BINDKEY(MODSHIFT,	XK_9,			FN_MVWS_9);
	BINDKEY(MODSHIFT,	XK_0,			FN_MVWS_10);
	BINDKEY(MODSHIFT,	XK_F1,			FN_MVWS_11);
	BINDKEY(MODSHIFT,	XK_F2,			FN_MVWS_12);
	BINDKEY(MODSHIFT,	XK_F3,			FN_MVWS_13);
	BINDKEY(MODSHIFT,	XK_F4,			FN_MVWS_14);
	BINDKEY(MODSHIFT,	XK_F5,			FN_MVWS_15);
	BINDKEY(MODSHIFT,	XK_F6,			FN_MVWS_16);
	BINDKEY(MODSHIFT,	XK_F7,			FN_MVWS_17);
	BINDKEY(MODSHIFT,	XK_F8,			FN_MVWS_18);
	BINDKEY(MODSHIFT,	XK_F9,			FN_MVWS_19);
	BINDKEY(MODSHIFT,	XK_F10,			FN_MVWS_20);
	BINDKEY(MODSHIFT,	XK_F11,			FN_MVWS_21);
	BINDKEY(MODSHIFT,	XK_F12,			FN_MVWS_22);
	BINDKEY(MODSHIFT,	XK_slash,		FN_NAME_WORKSPACE);
	BINDKEY(MODSHIFT,	XK_q,			FN_QUIT);
	BINDKEY(MOD,		XK_r,			FN_RAISE);
	BINDKEY(MODSHIFT,	XK_r,			FN_RAISE_TOGGLE);
	BINDKEY(MOD,		XK_q,			FN_RESTART);
	BINDKEY(MOD,		XK_KP_End,		FN_RG_1);
	BINDKEY(MOD,		XK_KP_Down,		FN_RG_2);
	BINDKEY(MOD,		XK_KP_Next,		FN_RG_3);
	BINDKEY(MOD,		XK_KP_Left,		FN_RG_4);
	BINDKEY(MOD,		XK_KP_Begin,		FN_RG_5);
	BINDKEY(MOD,		XK_KP_Right,		FN_RG_6);
	BINDKEY(MOD,		XK_KP_Home,		FN_RG_7);
	BINDKEY(MOD,		XK_KP_Up,		FN_RG_8);
	BINDKEY(MOD,		XK_KP_Prior,		FN_RG_9);
	BINDKEY(MODSHIFT,	XK_Right,		FN_RG_NEXT);
	BINDKEY(MODSHIFT,	XK_Left,		FN_RG_PREV);
	BINDKEY(MOD,		XK_f,			FN_SEARCH_WIN);
	BINDKEY(MOD,		XK_slash,		FN_SEARCH_WORKSPACE);
	BINDKEYSPAWN(MODSHIFT,	XK_i,			"initscr");
	BINDKEYSPAWN(MODSHIFT,	XK_Delete,		"lock");
	BINDKEYSPAWN(MOD,	XK_p,			"menu");
	BINDKEYSPAWN(MOD,	XK_s,			"screenshot_all");
	BINDKEYSPAWN(MODSHIFT,	XK_s,			"screenshot_wind");
	BINDKEYSPAWN(MODSHIFT,	XK_Return,		"term");
	BINDKEY(MODSHIFT,	XK_comma,		FN_STACK_INC);
	BINDKEY(MODSHIFT,	XK_period,		FN_STACK_DEC);
	BINDKEY(MODSHIFT,	XK_space,		FN_STACK_RESET);
	BINDKEY(MOD,		XK_Return,		FN_SWAP_MAIN);
	BINDKEY(MODSHIFT,	XK_j,			FN_SWAP_NEXT);
	BINDKEY(MODSHIFT,	XK_k,			FN_SWAP_PREV);
	BINDKEY(MODSHIFT,	XK_w,			FN_UNICONIFY);
	BINDKEY(MODSHIFT,	XK_v,			FN_VERSION);
	BINDKEY(MOD,		XK_equal,		FN_WIDTH_GROW);
	BINDKEY(MOD,		XK_minus,		FN_WIDTH_SHRINK);
	BINDKEY(MOD,		XK_x,			FN_WIND_DEL);
	BINDKEY(MODSHIFT,	XK_x,			FN_WIND_KILL);
	BINDKEY(MOD,		XK_1,			FN_WS_1);
	BINDKEY(MOD,		XK_2,			FN_WS_2);
	BINDKEY(MOD,		XK_3,			FN_WS_3);
	BINDKEY(MOD,		XK_4,			FN_WS_4);
	BINDKEY(MOD,		XK_5,			FN_WS_5);
	BINDKEY(MOD,		XK_6,			FN_WS_6);
	BINDKEY(MOD,		XK_7,			FN_WS_7);
	BINDKEY(MOD,		XK_8,			FN_WS_8);
	BINDKEY(MOD,		XK_9,			FN_WS_9);
	BINDKEY(MOD,		XK_0,			FN_WS_10);
	BINDKEY(MOD,		XK_F1,			FN_WS_11);
	BINDKEY(MOD,		XK_F2,			FN_WS_12);
	BINDKEY(MOD,		XK_F3,			FN_WS_13);
	BINDKEY(MOD,		XK_F4,			FN_WS_14);
	BINDKEY(MOD,		XK_F5,			FN_WS_15);
	BINDKEY(MOD,		XK_F6,			FN_WS_16);
	BINDKEY(MOD,		XK_F7,			FN_WS_17);
	BINDKEY(MOD,		XK_F8,			FN_WS_18);
	BINDKEY(MOD,		XK_F9,			FN_WS_19);
	BINDKEY(MOD,		XK_F10,			FN_WS_20);
	BINDKEY(MOD,		XK_F11,			FN_WS_21);
	BINDKEY(MOD,		XK_F12,			FN_WS_22);
	BINDKEY(MOD,		XK_Right,		FN_WS_NEXT);
	BINDKEY(MOD,		XK_Left,		FN_WS_PREV);
	BINDKEY(MOD,		XK_Up,			FN_WS_NEXT_ALL);
	BINDKEY(MOD,		XK_Down,		FN_WS_PREV_ALL);
	BINDKEY(MODSHIFT,	XK_Up,			FN_WS_NEXT_MOVE);
	BINDKEY(MODSHIFT,	XK_Down,		FN_WS_PREV_MOVE);
	BINDKEY(MOD,		XK_a,			FN_WS_PRIOR);
#ifdef SWM_DEBUG
	BINDKEY(MOD,		XK_d,			FN_DEBUG_TOGGLE);
	BINDKEY(MODSHIFT,	XK_d,			FN_DUMPWINS);
#endif
#undef BINDKEY
#undef BINDKEYSPAWN
}

void
setup_btnbindings(void)
{
	setbinding(ANYMOD, BTNBIND, XCB_BUTTON_INDEX_1, FN_FOCUS,
	    BINDING_F_REPLAY, NULL);
	setbinding(MOD, BTNBIND, XCB_BUTTON_INDEX_3, FN_RESIZE, 0, NULL);
	setbinding(MODSHIFT, BTNBIND, XCB_BUTTON_INDEX_3, FN_RESIZE_CENTERED, 0,
	    NULL);
	setbinding(MOD, BTNBIND, XCB_BUTTON_INDEX_1, FN_MOVE, 0, NULL);
}
#undef MODSHIFT
#undef MOD

void
clear_bindings(void)
{
	struct binding		*bp;

	while ((bp = RB_ROOT(&bindings)))
		binding_remove(bp);
}

void
clear_keybindings(void)
{
	struct binding		*bp, *bptmp;

	RB_FOREACH_SAFE(bp, binding_tree, &bindings, bptmp) {
		if (bp->type != KEYBIND)
			continue;
		binding_remove(bp);
	}
}

int
setkeymapping(const char *selector, const char *value, int flags, char **emsg)
{
	char			*keymapping_file;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	DNPRINTF(SWM_D_KEY, "enter\n");

	keymapping_file = expand_tilde(value);

	clear_keybindings();
	/* load new key bindings; if it fails, revert to default bindings */
	if (conf_load(keymapping_file, SWM_CONF_KEYMAPPING)) {
		ALLOCSTR(emsg, "failed to load '%s'", keymapping_file);
		free(keymapping_file);
		clear_keybindings();
		setup_keybindings();
		return (1);
	}

	free(keymapping_file);

	DNPRINTF(SWM_D_KEY, "leave\n");
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
	DNPRINTF(SWM_D_MISC, "numlockmask: %#x\n", numlockmask);
}

xcb_keycode_t
get_binding_keycode(struct binding *bp)
{
	const xcb_setup_t			*s;
	xcb_get_keyboard_mapping_reply_t	*kmr;
	int					col;
	xcb_keycode_t				kc, min, max;

	s = get_setup();
	min = s->min_keycode;
	max = s->max_keycode;

	kmr = xcb_get_keyboard_mapping_reply(conn,
	    xcb_get_keyboard_mapping(conn, min, max - min + 1), NULL);
	if (kmr == NULL)
		return (XCB_NO_SYMBOL);

	/* Search for keycode by keysym column. */
	for (col = 0; col < kmr->keysyms_per_keycode; col++) {
		/* Keycodes are unsigned, bail if kc++ is reduced to 0. */
		for (kc = min; kc > 0 && kc <= max; kc++) {
			if (xcb_key_symbols_get_keysym(syms, kc, col) ==
			    bp->value)
				return (kc);
		}
	}

	return (XCB_NO_SYMBOL);
}

void
grabkeys(void)
{
	struct binding		*bp;
	int			num_screens, i, j;
	uint16_t		modifiers[4];
	xcb_keycode_t		keycode;

	DNPRINTF(SWM_D_MISC, "begin\n");
	updatenumlockmask();

	modifiers[0] = 0;
	modifiers[1] = numlockmask;
	modifiers[2] = XCB_MOD_MASK_LOCK;
	modifiers[3] = numlockmask | XCB_MOD_MASK_LOCK;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		if (TAILQ_EMPTY(&screens[i].rl))
			continue;
		xcb_ungrab_key(conn, XCB_GRAB_ANY, screens[i].root,
			XCB_MOD_MASK_ANY);
		RB_FOREACH(bp, binding_tree, &bindings) {
			if (bp->type != KEYBIND)
				continue;

			/* If there is a catch-all, only bind that. */
			if ((binding_lookup(ANYMOD, KEYBIND, bp->value)) &&
			    bp->mod != ANYMOD)
				continue;

			/* Skip unused ws binds. */
			if ((int)bp->action > FN_WS_1 + workspace_limit - 1 &&
			    bp->action <= FN_WS_22)
				continue;

			/* Skip unused mvws binds. */
			if ((int)bp->action > FN_MVWS_1 + workspace_limit - 1 &&
			    bp->action <= FN_MVWS_22)
				continue;

			/* Try to get keycode for the grab. */
			keycode = get_binding_keycode(bp);
			if (keycode == XCB_NO_SYMBOL)
				continue;

			if (bp->mod == XCB_MOD_MASK_ANY) {
				/* All modifiers are grabbed in one pass. */
				DNPRINTF(SWM_D_KEY, "key: %u, modmask: %d\n",
				    bp->value, bp->mod);
				xcb_grab_key(conn, 1, screens[i].root, bp->mod,
				    keycode, XCB_GRAB_MODE_ASYNC,
				    XCB_GRAB_MODE_SYNC);
			} else {
				/* Need to grab each modifier permutation. */
				for (j = 0; j < LENGTH(modifiers); j++) {
					DNPRINTF(SWM_D_KEY, "key: %u, "
					    "modmask: %d\n", bp->value,
					    bp->mod | modifiers[j]);
					xcb_grab_key(conn, 1, screens[i].root,
					    bp->mod | modifiers[j], keycode,
					    XCB_GRAB_MODE_ASYNC,
					    XCB_GRAB_MODE_SYNC);
				}
			}
		}
	}
	DNPRINTF(SWM_D_MISC, "done\n");
}

void
grabbuttons(void)
{
	struct binding	*bp;
	int		num_screens, i, k;
	uint16_t	modifiers[4];

	DNPRINTF(SWM_D_MOUSE, "begin\n");
	updatenumlockmask();

	modifiers[0] = 0;
	modifiers[1] = numlockmask;
	modifiers[2] = XCB_MOD_MASK_LOCK;
	modifiers[3] = numlockmask | XCB_MOD_MASK_LOCK;

	num_screens = get_screen_count();
	for (k = 0; k < num_screens; k++) {
		if (TAILQ_EMPTY(&screens[k].rl))
			continue;
		xcb_ungrab_button(conn, XCB_BUTTON_INDEX_ANY, screens[k].root,
			XCB_MOD_MASK_ANY);
		RB_FOREACH(bp, binding_tree, &bindings) {
			if (bp->type != BTNBIND)
				continue;

			/* If there is a catch-all, only bind that. */
			if ((binding_lookup(ANYMOD, BTNBIND, bp->value)) &&
			    bp->mod != ANYMOD)
				continue;

			/* Skip unused ws binds. */
			if ((int)bp->action > FN_WS_1 + workspace_limit - 1 &&
			    bp->action <= FN_WS_22)
				continue;

			/* Skip unused mvws binds. */
			if ((int)bp->action > FN_MVWS_1 + workspace_limit - 1 &&
			    bp->action <= FN_MVWS_22)
				continue;

			if (bp->mod == XCB_MOD_MASK_ANY) {
				/* All modifiers are grabbed in one pass. */
				DNPRINTF(SWM_D_MOUSE, "grab btn: %u, "
				    "modmask: %d\n", bp->value, bp->mod);
				xcb_grab_button(conn, 0, screens[k].root,
				    BUTTONMASK, XCB_GRAB_MODE_SYNC,
				    XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
				    XCB_CURSOR_NONE, bp->value, bp->mod);
			} else {
				/* Need to grab each modifier permutation. */
				for (i = 0; i < LENGTH(modifiers); ++i) {
					DNPRINTF(SWM_D_MOUSE, "grab btn: %d, "
					    "modmask: %u\n", bp->value,
					    bp->mod | modifiers[i]);
					xcb_grab_button(conn, 0,
					    screens[k].root, BUTTONMASK,
					    XCB_GRAB_MODE_SYNC,
					    XCB_GRAB_MODE_ASYNC,
					    XCB_WINDOW_NONE,
					    XCB_CURSOR_NONE, bp->value,
					    bp->mod | modifiers[i]);
				}
			}
		}
	}
	DNPRINTF(SWM_D_MOUSE, "done\n");
}

struct wsi_flag {
	char *name;
	uint32_t mask;
} wsiflags[] = {
	{"listcurrent", SWM_WSI_LISTCURRENT},
	{"listactive", SWM_WSI_LISTACTIVE},
	{"listempty", SWM_WSI_LISTEMPTY},
	{"listnamed", SWM_WSI_LISTNAMED},
	{"listurgent", SWM_WSI_LISTURGENT},
	{"listall", SWM_WSI_LISTALL},
	{"hidecurrent", SWM_WSI_HIDECURRENT},
	{"markcurrent", SWM_WSI_MARKCURRENT},
	{"markurgent", SWM_WSI_MARKURGENT},
	{"markactive", SWM_WSI_MARKACTIVE},
	{"markempty", SWM_WSI_MARKEMPTY},
	{"printnames", SWM_WSI_PRINTNAMES},
	{"noindexes", SWM_WSI_NOINDEXES},
};

#define SWM_FLAGS_DELIM		","
#define SWM_FLAGS_WHITESPACE	" \t\n"
int
parse_workspace_indicator(const char *str, uint32_t *mode, char **emsg)
{
	char			*tmp, *cp, *name;
	size_t			len;
	int			i;

	if (str == NULL || mode == NULL)
		return (1);

	if ((cp = tmp = strdup(str)) == NULL)
		err(1, "parse_workspace_indicator: strdup");

	*mode = 0;
	while ((name = strsep(&cp, SWM_FLAGS_DELIM)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, SWM_FLAGS_WHITESPACE);
		name += strspn(name, SWM_FLAGS_WHITESPACE);
		len = strcspn(name, SWM_FLAGS_WHITESPACE);

		for (i = 0; i < LENGTH(wsiflags); i++) {
			if (strncasecmp(name, wsiflags[i].name, len) == 0) {
				DNPRINTF(SWM_D_CONF, "flag: [%s]\n", name);
				*mode |= wsiflags[i].mask;
				break;
			}
		}
		if (i >= LENGTH(wsiflags)) {
			ALLOCSTR(emsg, "invalid flag: %s", name);
			DNPRINTF(SWM_D_CONF, "invalid flag: [%s]\n", name);
			free(tmp);
			return (1);
		}
	}

	free(tmp);
	return (0);
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
	"NOFOCUSCYCLE",
	"MINIMALBORDER",
};

/* SWM_Q_DELIM: retain '|' for back compat for now (2009-08-11) */
#define SWM_Q_DELIM		"\n|+ \t"
int
parsequirks(const char *qstr, uint32_t *quirk, int *ws, char **emsg)
{
	char			*str, *cp, *name;
	int			i;

	if (quirk == NULL || qstr == NULL)
		return (1);

	if ((cp = str = strdup(qstr)) == NULL)
		err(1, "parsequirks: strdup");

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
				DNPRINTF(SWM_D_QUIRK, "%s\n", name);
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
			DNPRINTF(SWM_D_QUIRK, "invalid quirk [%s]\n", name);
			ALLOCSTR(emsg, "invalid quirk: %s", name);
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

	DNPRINTF(SWM_D_QUIRK, "class: %s, instance: %s, name: %s, value: %u, "
	    "ws: %d\n", class, instance, name, quirk, ws);

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
	DNPRINTF(SWM_D_QUIRK, "compiled: %s\n", str);
	free(str);

	if (asprintf(&str, "^%s$", instance) == -1)
		err(1, "quirk_insert: asprintf");
	if (regcomp(&qp->regex_instance, str, REG_EXTENDED | REG_NOSUB)) {
		add_startup_exception("regex failed to compile quirk 'instance'"
		    " field: %s", instance);
		failed = true;
	}
	DNPRINTF(SWM_D_QUIRK, "compiled: %s\n", str);
	free(str);

	if (asprintf(&str, "^%s$", name) == -1)
		err(1, "quirk_insert: asprintf");
	if (regcomp(&qp->regex_name, str, REG_EXTENDED | REG_NOSUB)) {
		add_startup_exception("regex failed to compile quirk 'name' "
		    "field: %s", name);
		failed = true;
	}
	DNPRINTF(SWM_D_QUIRK, "compiled: %s\n", str);
	free(str);

	if (failed) {
		DNPRINTF(SWM_D_QUIRK, "regex error\n");
		quirk_free(qp);
	} else {
		qp->quirk = quirk;
		qp->ws = ws;
		TAILQ_INSERT_TAIL(&quirks, qp, entry);
	}
	DNPRINTF(SWM_D_QUIRK, "leave\n");
}

void
quirk_remove(struct quirk *qp)
{
	DNPRINTF(SWM_D_QUIRK, "%s:%s [%u]\n", qp->class, qp->name, qp->quirk);

	TAILQ_REMOVE(&quirks, qp, entry);
	quirk_free(qp);

	DNPRINTF(SWM_D_QUIRK, "leave\n");
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
clear_quirks(void)
{
	struct quirk		*qp;
#ifndef __clang_analyzer__ /* Suppress false warnings. */
	while ((qp = TAILQ_FIRST(&quirks)) != NULL) {
		quirk_remove(qp);
	}
#endif
}

void
quirk_replace(struct quirk *qp, const char *class, const char *instance,
    const char *name, uint32_t quirk, int ws)
{
	DNPRINTF(SWM_D_QUIRK, "%s:%s:%s [%u], ws: %d\n", qp->class,
	    qp->instance, qp->name, qp->quirk, qp->ws);

	quirk_remove(qp);
	quirk_insert(class, instance, name, quirk, ws);

	DNPRINTF(SWM_D_QUIRK, "leave\n");
}

void
setquirk(const char *class, const char *instance, const char *name,
    uint32_t quirk, int ws)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "enter %s:%s:%s [%u], ws: %d\n", class, instance,
	    name, quirk, ws);

#ifndef __clang_analyzer__ /* Suppress false warnings. */
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
			goto out;
		}
	}
#endif

	/* Only insert if quirk is not NONE or forced ws is set. */
	if (quirk || ws != -1)
		quirk_insert(class, instance, name, quirk, ws);
out:
	DNPRINTF(SWM_D_QUIRK, "leave\n");
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
setconfquirk(const char *selector, const char *value, int flags, char **emsg)
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

	DNPRINTF(SWM_D_CONF, "class: %s, instance: %s, name: %s\n", class,
	    instance, name);

	if ((retval = parsequirks(value, &qrks, &ws, emsg)) == 0)
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
	SWM_S_BAR_ACTION_EXPAND,
	SWM_S_BAR_AT_BOTTOM,
	SWM_S_BAR_BORDER_WIDTH,
	SWM_S_BAR_ENABLED,
	SWM_S_BAR_ENABLED_WS,
	SWM_S_BAR_FONT,
	SWM_S_BAR_FONT_PUA,
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
	SWM_S_MAXIMIZE_HIDE_BAR,
	SWM_S_REGION_PADDING,
	SWM_S_SPAWN_ORDER,
	SWM_S_SPAWN_TERM,
	SWM_S_STACK_ENABLED,
	SWM_S_TERM_WIDTH,
	SWM_S_TILE_GAP,
	SWM_S_URGENT_COLLAPSE,
	SWM_S_URGENT_ENABLED,
	SWM_S_VERBOSE_LAYOUT,
	SWM_S_WARP_FOCUS,
	SWM_S_WARP_POINTER,
	SWM_S_WINDOW_CLASS_ENABLED,
	SWM_S_WINDOW_INSTANCE_ENABLED,
	SWM_S_WINDOW_NAME_ENABLED,
	SWM_S_WORKSPACE_CLAMP,
	SWM_S_WORKSPACE_LIMIT,
	SWM_S_WORKSPACE_INDICATOR,
	SWM_S_WORKSPACE_NAME,
	SWM_S_WORKSPACE_MARK_CURRENT,
	SWM_S_WORKSPACE_MARK_URGENT,
	SWM_S_WORKSPACE_MARK_ACTIVE,
	SWM_S_WORKSPACE_MARK_EMPTY,
	SWM_S_STACK_MARK_MAX,
	SWM_S_STACK_MARK_VERTICAL,
	SWM_S_STACK_MARK_VERTICAL_FLIP,
	SWM_S_STACK_MARK_HORIZONTAL,
	SWM_S_STACK_MARK_HORIZONTAL_FLIP,
};

int
setconfvalue(const char *selector, const char *value, int flags, char **emsg)
{
	struct workspace	*ws;
	int			i, ws_id, num_screens, n;
	char			*b, *str, *sp;

	switch (flags) {
	case SWM_S_BAR_ACTION:
		free(bar_argv[0]);
		if ((bar_argv[0] = expand_tilde(value)) == NULL)
			err(1, "setconfvalue: bar_action");
		break;
	case SWM_S_BAR_ACTION_EXPAND:
		bar_action_expand = (atoi(value) != 0);
		break;
	case SWM_S_BAR_AT_BOTTOM:
		bar_at_bottom = (atoi(value) != 0);
		break;
	case SWM_S_BAR_BORDER_WIDTH:
		bar_border_width = atoi(value);
		if (bar_border_width < 0)
			bar_border_width = 0;
		break;
	case SWM_S_BAR_ENABLED:
		bar_enabled = (atoi(value) != 0);
		break;
	case SWM_S_BAR_ENABLED_WS:
		ws_id = atoi(selector) - 1;
		if (ws_id < 0 || ws_id >= workspace_limit) {
			ALLOCSTR(emsg, "invalid workspace: %d", ws_id + 1);
			return (1);
		}

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

		if ((sp = str = strdup(value)) == NULL)
			err(1, "setconfvalue: strdup");

		/* If there are any non-XLFD entries, switch to Xft mode. */
		while ((b = strsep(&sp, ",")) != NULL) {
			if (*b == '\0')
				continue;
			if (!isxlfd(b)) {
				bar_font_legacy = false;
				break;
			}
		}
		free(str);
		if (bar_font_legacy)
			break;

		/* Non-legacy fonts: read list of Xft fontname */
		if ((sp = str = strdup(value)) == NULL)
			err(1, "setconfvalue: strdup");

		num_xftfonts = 0;
		while ((b = strsep(&sp, ",")) != NULL) {
			if (*b == '\0')
				continue;
			free(bar_fontnames[num_xftfonts]);
			if ((bar_fontnames[num_xftfonts] = strdup(b))
			    == NULL)
				err(1, "setconfvalue: bar_font");
			num_xftfonts++;
			if (num_xftfonts == SWM_BAR_MAX_FONTS)
				break;
		}
		free(str);
		break;
	case SWM_S_BAR_FONT_PUA:
		free(bar_fontname_pua);
		if ((bar_fontname_pua = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_font_pua");
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
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}

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
		disable_border_always = (strcmp(value, "always") == 0);
		disable_border = (atoi(value) != 0) || disable_border_always;
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
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}
		break;
	case SWM_S_FOCUS_CLOSE_WRAP:
		focus_close_wrap = (atoi(value) != 0);
		break;
	case SWM_S_FOCUS_DEFAULT:
		if (strcmp(value, "last") == 0)
			focus_default = SWM_STACK_TOP;
		else if (strcmp(value, "first") == 0)
			focus_default = SWM_STACK_BOTTOM;
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}
		break;
	case SWM_S_FOCUS_MODE:
		if (strcmp(value, "default") == 0)
			focus_mode = SWM_FOCUS_DEFAULT;
		else if (strcmp(value, "follow") == 0 ||
		    strcmp(value, "follow_cursor") == 0)
			focus_mode = SWM_FOCUS_FOLLOW;
		else if (strcmp(value, "manual") == 0)
			focus_mode = SWM_FOCUS_MANUAL;
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}
		break;
	case SWM_S_ICONIC_ENABLED:
		iconic_enabled = (atoi(value) != 0);
		break;
	case SWM_S_MAXIMIZE_HIDE_BAR:
		maximize_hide_bar = atoi(value);
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
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}
		break;
	case SWM_S_SPAWN_TERM:
		setconfspawn("term", value, 0, emsg);
		setconfspawn("spawn_term", value, 0, emsg);
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
	case SWM_S_WARP_FOCUS:
		warp_focus = (atoi(value) != 0);
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
	case SWM_S_WORKSPACE_CLAMP:
		workspace_clamp = (atoi(value) != 0);
		break;
	case SWM_S_WORKSPACE_LIMIT:
		workspace_limit = atoi(value);
		if (workspace_limit > SWM_WS_MAX)
			workspace_limit = SWM_WS_MAX;
		else if (workspace_limit < 1)
			workspace_limit = 1;

		ewmh_update_desktops();
		break;
	case SWM_S_WORKSPACE_INDICATOR:
		if (parse_workspace_indicator(value, &workspace_indicator,
		    emsg))
			return (1);
		break;
	case SWM_S_WORKSPACE_NAME:
		if (getenv("SWM_STARTED") != NULL)
			return (0);

		n = 0;
		if (sscanf(value, "ws[%d]:%n", &ws_id, &n) != 1 || n == 0 ||
		    value[n] == '\0') {
			ALLOCSTR(emsg, "invalid syntax: %s", value);
			return (1);
		}
		value += n;
		ws_id--;
		if (ws_id < 0 || ws_id >= workspace_limit) {
			ALLOCSTR(emsg, "invalid workspace: %d", ws_id + 1);
			return (1);
		}

		num_screens = get_screen_count();
		for (i = 0; i < num_screens; ++i) {
			ws = (struct workspace *)&screens[i].ws;

			free(ws[ws_id].name);
			if ((ws[ws_id].name = strdup(value)) == NULL)
				err(1, "name: strdup");

			ewmh_update_desktop_names();
			ewmh_get_desktop_names();
		}
		break;
	case SWM_S_WORKSPACE_MARK_CURRENT:
		free(workspace_mark_current);
		workspace_mark_current = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_URGENT:
		free(workspace_mark_urgent);
		workspace_mark_urgent = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_ACTIVE:
		free(workspace_mark_active);
		workspace_mark_active = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_EMPTY:
		free(workspace_mark_empty);
		workspace_mark_empty = unescape_value(value);
		break;
	case SWM_S_STACK_MARK_MAX:
		free(stack_mark_max);
		stack_mark_max = unescape_value(value);
		break;
	case SWM_S_STACK_MARK_VERTICAL:
		free(stack_mark_vertical);
		stack_mark_vertical = unescape_value(value);
		break;
	case SWM_S_STACK_MARK_VERTICAL_FLIP:
		free(stack_mark_vertical_flip);
		stack_mark_vertical_flip = unescape_value(value);
		break;
	case SWM_S_STACK_MARK_HORIZONTAL:
		free(stack_mark_horizontal);
		stack_mark_horizontal = unescape_value(value);
		break;
	case SWM_S_STACK_MARK_HORIZONTAL_FLIP:
		free(stack_mark_horizontal_flip);
		stack_mark_horizontal_flip = unescape_value(value);
		break;
	default:
		ALLOCSTR(emsg, "invalid option");
		return (1);
	}
	return (0);
}

int
setconfmodkey(const char *selector, const char *value, int flags, char **emsg)
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
	else if (strncasecmp(value, "Mod5", strlen("Mod5")) == 0)
		update_modkey(XCB_MOD_MASK_5);
	else {
		ALLOCSTR(emsg, "invalid value: %s", value);
		return (1);
	}
	return (0);
}

int
setconfcolor(const char *selector, const char *value, int flags, char **emsg)
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
		ALLOCSTR(emsg, "invalid screen index: %d", last + 1);
		return (1);
	}

	for (i = first; i <= last; ++i) {
		setscreencolor(value, i, flags);

		/*
		 * Need to sync 'maximized' and 'selected' colors with their
		 * respective colors if they haven't been customized.
		 */
		switch (flags) {
		case SWM_S_COLOR_FOCUS:
			if (!screens[i].c[SWM_S_COLOR_FOCUS_MAXIMIZED].manual)
				setscreencolor(value, i,
				    SWM_S_COLOR_FOCUS_MAXIMIZED);
			break;
		case SWM_S_COLOR_UNFOCUS:
			if (!screens[i].c[SWM_S_COLOR_UNFOCUS_MAXIMIZED].manual)
				setscreencolor(value, i,
				    SWM_S_COLOR_UNFOCUS_MAXIMIZED);
			break;
		case SWM_S_COLOR_BAR:
			if (!screens[i].c[SWM_S_COLOR_BAR_FONT_SELECTED].manual)
				setscreencolor(value, i,
				    SWM_S_COLOR_BAR_FONT_SELECTED);
			break;
		case SWM_S_COLOR_BAR_BORDER:
			if (!screens[i].c[SWM_S_COLOR_BAR_SELECTED].manual)
				setscreencolor(value, i,
				    SWM_S_COLOR_BAR_SELECTED);
			break;
		}

		screens[i].c[flags].manual = 1;
	}

	return (0);
}

int
setconfcolorlist(const char *selector, const char *value, int flags, char **emsg)
{
	char			*b, *str, *sp;

	switch (flags) {
	case SWM_S_COLOR_BAR:
		/* Set list of background colors */
		if ((sp = str = strdup(value)) == NULL)
			err(1, "setconfvalue: strdup");

		num_bg_colors = 0;
		while ((b = strsep(&sp, ",")) != NULL) {
			while (isblank(*b)) b++;
			if (*b == '\0')
				continue;
			setconfcolor(selector, b, SWM_S_COLOR_BAR +
			    num_bg_colors, emsg);
			num_bg_colors++;
			if (num_bg_colors == SWM_BAR_MAX_COLORS)
				break;
		}
		free(str);
		break;
	case SWM_S_COLOR_BAR_FONT:
		/* Set list of foreground colors */
		if ((sp = str = strdup(value)) == NULL)
			err(1, "setconfvalue: strdup");

		num_fg_colors = 0;
		while ((b = strsep(&sp, ",")) != NULL) {
			while (isblank(*b)) b++;
			if (*b == '\0')
				continue;
			setconfcolor(selector, b, SWM_S_COLOR_BAR_FONT +
			    num_fg_colors, emsg);
			num_fg_colors++;
			if (num_fg_colors == SWM_BAR_MAX_COLORS)
				break;
		}
		free(str);
		break;
	}
	return (0);
}

int
setconfregion(const char *selector, const char *value, int flags, char **emsg)
{
	unsigned int			x, y, w, h;
	int				sidx, num_screens;
	xcb_screen_t			*screen;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	DNPRINTF(SWM_D_CONF, "%s\n", value);

	num_screens = get_screen_count();
	if (sscanf(value, "screen[%d]:%ux%u+%u+%u",
	    &sidx, &w, &h, &x, &y) != 5) {
		ALLOCSTR(emsg, "invalid syntax: %s", value);
		return (1);
	}
	if (sidx < 1 || sidx > num_screens) {
		ALLOCSTR(emsg, "invalid screen index: %d", sidx);
		return (1);
	}
	sidx--;

	if ((screen = get_screen(sidx)) == NULL)
		errx(1, "ERROR: unable to get screen %d.", sidx);

	if (w < 1 || h < 1) {
		ALLOCSTR(emsg, "invalid size: %ux%u", w, h);
		return (1);
	}

	if (x > screen->width_in_pixels ||
	    y > screen->height_in_pixels ||
	    w + x > screen->width_in_pixels ||
	    h + y > screen->height_in_pixels) {
		ALLOCSTR(emsg, "geometry exceeds screen boundary: %ux%u+%u+%u",
		    w, h, x, y);
		return (1);
	}

	new_region(&screens[sidx], x, y, w, h);

	return (0);
}

int
setautorun(const char *selector, const char *value, int flags, char **emsg)
{
	int			ws_id;
	char			*ap, *sp, *str;
	union arg		a;
	int			argc = 0, n;
	pid_t			pid;
	struct pid_e		*p;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (getenv("SWM_STARTED"))
		return (0);

	n = 0;
	if (sscanf(value, "ws[%d]:%n", &ws_id, &n) != 1 || n == 0 ||
	    value[n] == '\0') {
		ALLOCSTR(emsg, "invalid syntax: %s", value);
		return (1);
	}
	value += n;
	ws_id--;
	if (ws_id < 0 || ws_id >= workspace_limit) {
		ALLOCSTR(emsg, "invalid workspace: %d", ws_id + 1);
		return (1);
	}

	sp = str = expand_tilde(value);

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
		DNPRINTF(SWM_D_SPAWN, "arg [%s]\n", ap);
		argc++;
		if ((a.argv = realloc(a.argv, argc * sizeof(char *))) == NULL)
			err(1, "setautorun: realloc");
		a.argv[argc - 1] = ap;
	}

	if ((a.argv = realloc(a.argv, (argc + 1) * sizeof(char *))) == NULL)
		err(1, "setautorun: realloc");
	a.argv[argc] = NULL;

	if ((pid = fork()) == 0) {
		spawn(ws_id, &a, true);
		/* NOTREACHED */
		_exit(1);
	}
	free(a.argv);
	free(str);

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
setlayout(const char *selector, const char *value, int flags, char **emsg)
{
	struct workspace	*ws;
	int			ws_id, i, x, mg, ma, si, ar, n;
	int			st = SWM_V_STACK, num_screens;
	bool			f = false;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (getenv("SWM_STARTED"))
		return (0);

	n = 0;
	if (sscanf(value, "ws[%d]:%d:%d:%d:%d:%n",
	    &ws_id, &mg, &ma, &si, &ar, &n) != 5 || n == 0 ||
	    value[n] == '\0') {
		ALLOCSTR(emsg, "invalid syntax: %s", value);
		return (1);
	}
	value += n;
	ws_id--;
	if (ws_id < 0 || ws_id >= workspace_limit) {
		ALLOCSTR(emsg, "invalid workspace: %d", ws_id + 1);
		return (1);
	}

	if (strcasecmp(value, "vertical") == 0)
		st = SWM_V_STACK;
	else if (strcasecmp(value, "vertical_flip") == 0) {
		st = SWM_V_STACK;
		f = true;
	} else if (strcasecmp(value, "horizontal") == 0)
		st = SWM_H_STACK;
	else if (strcasecmp(value, "horizontal_flip") == 0) {
		st = SWM_H_STACK;
		f = true;
	} else if (strcasecmp(value, "max") == 0 ||
	    strcasecmp(value, "fullscreen") == 0)
		/* Keep "fullscreen" for backwards compatibility. */
		st = SWM_MAX_STACK;
	else {
		ALLOCSTR(emsg, "invalid layout: %s", value);
		return (1);
	}

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
	char			*name;
	int			(*func)(const char*, const char*, int, char **);
	int			flags;
};
struct config_option configopt[] = {
	{ "autorun",			setautorun,	0 },
	{ "bar_action",			setconfvalue,	SWM_S_BAR_ACTION },
	{ "bar_action_expand",		setconfvalue,	SWM_S_BAR_ACTION_EXPAND },
	{ "bar_at_bottom",		setconfvalue,	SWM_S_BAR_AT_BOTTOM },
	{ "bar_border",			setconfcolor,	SWM_S_COLOR_BAR_BORDER },
	{ "bar_border_unfocus",		setconfcolor,	SWM_S_COLOR_BAR_BORDER_UNFOCUS },
	{ "bar_border_width",		setconfvalue,	SWM_S_BAR_BORDER_WIDTH },
	{ "bar_color",			setconfcolorlist,	SWM_S_COLOR_BAR },
	{ "bar_color_selected",		setconfcolor,	SWM_S_COLOR_BAR_SELECTED },
	{ "bar_delay",			NULL,		0 }, /* dummy */
	{ "bar_enabled",		setconfvalue,	SWM_S_BAR_ENABLED },
	{ "bar_enabled_ws",		setconfvalue,	SWM_S_BAR_ENABLED_WS },
	{ "bar_font",			setconfvalue,	SWM_S_BAR_FONT },
	{ "bar_font_color",		setconfcolorlist,	SWM_S_COLOR_BAR_FONT },
	{ "bar_font_color_selected",	setconfcolor,	SWM_S_COLOR_BAR_FONT_SELECTED },
	{ "bar_font_pua",		setconfvalue,	SWM_S_BAR_FONT_PUA },
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
	{ "java_workaround",		NULL,		0 }, /* dummy */
	{ "keyboard_mapping",		setkeymapping,	0 },
	{ "layout",			setlayout,	0 },
	{ "maximize_hide_bar",		setconfvalue,	SWM_S_MAXIMIZE_HIDE_BAR },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "region_padding",		setconfvalue,	SWM_S_REGION_PADDING },
	{ "screenshot_app",		NULL,		0 }, /* dummy */
	{ "screenshot_enabled",		NULL,		0 }, /* dummy */
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
	{ "warp_focus",			setconfvalue,	SWM_S_WARP_FOCUS },
	{ "warp_pointer",		setconfvalue,	SWM_S_WARP_POINTER },
	{ "window_class_enabled",	setconfvalue,	SWM_S_WINDOW_CLASS_ENABLED },
	{ "window_instance_enabled",	setconfvalue,	SWM_S_WINDOW_INSTANCE_ENABLED },
	{ "window_name_enabled",	setconfvalue,	SWM_S_WINDOW_NAME_ENABLED },
	{ "workspace_clamp",		setconfvalue,	SWM_S_WORKSPACE_CLAMP },
	{ "workspace_limit",		setconfvalue,	SWM_S_WORKSPACE_LIMIT },
	{ "workspace_indicator",	setconfvalue,	SWM_S_WORKSPACE_INDICATOR },
	{ "name",			setconfvalue,	SWM_S_WORKSPACE_NAME },
	{ "workspace_mark_current",	setconfvalue,	SWM_S_WORKSPACE_MARK_CURRENT },
	{ "workspace_mark_urgent",	setconfvalue,	SWM_S_WORKSPACE_MARK_URGENT },
	{ "workspace_mark_active",	setconfvalue,	SWM_S_WORKSPACE_MARK_ACTIVE },
	{ "workspace_mark_empty",	setconfvalue,	SWM_S_WORKSPACE_MARK_EMPTY },
	{ "stack_mark_max",		setconfvalue,	SWM_S_STACK_MARK_MAX },
	{ "stack_mark_vertical",	setconfvalue,	SWM_S_STACK_MARK_VERTICAL },
	{ "stack_mark_vertical_flip",	setconfvalue,	SWM_S_STACK_MARK_VERTICAL_FLIP },
	{ "stack_mark_horizontal",	setconfvalue,	SWM_S_STACK_MARK_HORIZONTAL },
	{ "stack_mark_horizontal_flip",	setconfvalue,	SWM_S_STACK_MARK_HORIZONTAL_FLIP },
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
	char			*emsg = NULL;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optidx, count;
	struct config_option	*opt = NULL;

	DNPRINTF(SWM_D_CONF, "filename: %s, keymapping: %d\n", filename,
	    keymapping);

	if (filename == NULL) {
		warnx("conf_load: no filename");
		return (1);
	}

	DNPRINTF(SWM_D_CONF, "open %s\n", filename);

	if ((config = fopen(filename, "r")) == NULL) {
		warn("%s", filename);
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
			if (strncasecmp(cp, opt->name, wordlen) == 0 &&
			    (int)strlen(opt->name) == wordlen) {
				optidx = i;
				break;
			}
		}
		if (optidx == -1) {
			add_startup_exception("%s: line %zd: unknown option "
			    "%.*s", filename, lineno, wordlen, cp);
			continue;
		}
		if (keymapping && opt && strcmp(opt->name, "bind")) {
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
			    "to %s", filename, lineno, opt->name);
			continue;
		}
		/* trim trailing spaces */
		ce = optval + strlen(optval) - 1;
		while (ce > optval && isspace(*ce))
			--ce;
		*(ce + 1) = '\0';
		/* call function to deal with it all */
		if (opt->func && opt->func(optsub, optval, opt->flags, &emsg)) {
			if (emsg) {
				add_startup_exception("%s: line %zd: %s: %s",
				    filename, lineno, opt->name, emsg);
				free(emsg);
				emsg = NULL;
			} else
				add_startup_exception("%s: line %zd: %s",
				    filename, lineno, opt->name);
		}
	}

	if (line)
		free(line);
	fclose(config);

	DNPRINTF(SWM_D_CONF, "end\n");

	return (0);
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
		DNPRINTF(SWM_D_PROP, "_SWM_WS: %s\n", prop);
		ws_idx = (int)strtonum(prop, 0, workspace_limit - 1, &errstr);
		if (errstr) {
			DNPRINTF(SWM_D_PROP, "win #%s: %s", errstr, prop);
		}
		free(prop);
	}

	return (ws_idx);
}

int
get_ws_idx(struct ws_win *win)
{
	xcb_get_property_reply_t	*gpr;
	int			ws_idx = -1;

	if (win == NULL)
		return (-1);

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

	DNPRINTF(SWM_D_PROP, "win %#x, ws_idx: %d\n", win->id, ws_idx);

	return (ws_idx);
}

int
reparent_window(struct ws_win *win)
{
	xcb_screen_t		*s;
	xcb_void_cookie_t	c;
	xcb_generic_error_t	*error;
	uint32_t		wa[3];

	win->frame = xcb_generate_id(conn);

	DNPRINTF(SWM_D_MISC, "win %#x, frame: %#x\n", win->id, win->frame);

	if ((s = get_screen(win->s->idx)) == NULL)
		errx(1, "ERROR: unable to get screen %d.", win->s->idx);
	wa[0] = s->black_pixel;
	wa[1] =
	    XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
	    XCB_EVENT_MASK_EXPOSURE;
#ifdef SWM_DEBUG
	wa[1] |= XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
#endif
	wa[2] = win->s->colormap;

	xcb_create_window(conn, win->s->depth, win->frame, win->s->root, X(win),
	    Y(win), WIDTH(win), HEIGHT(win), 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
	    win->s->visual, XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK |
	    XCB_CW_COLORMAP, wa);

	win->state = SWM_WIN_STATE_REPARENTING;
	c = xcb_reparent_window_checked(conn, win->id, win->frame, 0, 0);
	if ((error = xcb_request_check(conn, c))) {
		DNPRINTF(SWM_D_MISC, "error:\n");
		event_error(error);
		free(error);

		/* Abort. */
		xcb_destroy_window(conn, win->frame);
		win->frame = XCB_WINDOW_NONE;
		win->state = SWM_WIN_STATE_UNPARENTED;
		unmanage_window(win);
		return (1);
	} else
		xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win->id);

	return (0);
}

void
unparent_window(struct ws_win *win)
{
	xcb_change_save_set(conn, XCB_SET_MODE_DELETE, win->id);
	xcb_reparent_window(conn, win->id, win->s->root, X(win), Y(win));
	xcb_destroy_window(conn, win->frame);
	win->frame = XCB_WINDOW_NONE;
	win->state = SWM_WIN_STATE_UNPARENTING;
}

struct ws_win *
manage_window(xcb_window_t id, int spawn_pos, bool mapping)
{
	struct ws_win				*win = NULL, *ww;
	struct swm_region			*r;
	struct pid_e				*p;
	struct quirk				*qp;
	xcb_get_geometry_reply_t		*gr;
	xcb_get_window_attributes_reply_t	*war = NULL;
	xcb_window_t				trans = XCB_WINDOW_NONE;
	uint32_t				i, wa[1], new_flags;
	int					ws_idx, force_ws = -1;
	char					*class, *instance, *name;

	if (find_bar(id)) {
		DNPRINTF(SWM_D_MISC, "skip; win %#x is region bar\n", id);
		goto out;
	}

	if (find_region(id)) {
		DNPRINTF(SWM_D_MISC, "skip; win %#x is region window\n", id);
		goto out;
	}

	if ((win = find_window(id)) != NULL) {
		DNPRINTF(SWM_D_MISC, "skip; win %#x (%#x) already managed\n",
		    win->frame, win->id);
		goto out;
	}

	war = xcb_get_window_attributes_reply(conn,
	    xcb_get_window_attributes(conn, id), NULL);
	if (war == NULL) {
		DNPRINTF(SWM_D_EVENT, "skip; window lost\n");
		goto out;
	}

	if (war->override_redirect) {
		DNPRINTF(SWM_D_EVENT, "skip; override_redirect\n");
		goto out;
	}

	if (!mapping && war->map_state == XCB_MAP_STATE_UNMAPPED &&
	    get_win_state(id) == XCB_ICCCM_WM_STATE_WITHDRAWN) {
		DNPRINTF(SWM_D_EVENT, "skip; window withdrawn\n");
		goto out;
	}

	/* Try to get initial window geometry. */
	gr = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, id), NULL);
	if (gr == NULL) {
		DNPRINTF(SWM_D_MISC, "get geometry failed\n");
		goto out;
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
	X(win) = gr->x + gr->border_width;
	Y(win) = gr->y + gr->border_width;
	win->bordered = false;
	win->mapped = (war->map_state != XCB_MAP_STATE_UNMAPPED);
	win->s = r->s;	/* this never changes */

	free(gr);

	/* Select which X events to monitor and set border pixel color. */
	wa[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_PROPERTY_CHANGE |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	xcb_change_window_attributes(conn, win->id, XCB_CW_EVENT_MASK, wa);

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
		set_focus_redirect(win);
	}

	/* Get WM_PROTOCOLS. */
	get_wm_protocols(win);

#ifdef SWM_DEBUG
	/* Must be after getting WM_HINTS and WM_PROTOCOLS. */
	DNPRINTF(SWM_D_FOCUS, "input_model: %s\n", get_win_input_model(win));
#endif

	/* Set initial quirks based on EWMH. */
	ewmh_autoquirk(win);

	/* Determine initial quirks. */
	xcb_icccm_get_wm_class_reply(conn,
	    xcb_icccm_get_wm_class(conn, win->id),
	    &win->ch, NULL);

	class = win->ch.class_name ? win->ch.class_name : "";
	instance = win->ch.instance_name ? win->ch.instance_name : "";
	name = get_win_name(win->id);

	DNPRINTF(SWM_D_CLASS, "class: %s, instance: %s, name: %s\n", class,
	    instance, name);

	TAILQ_FOREACH(qp, &quirks, entry) {
		if (regexec(&qp->regex_class, class, 0, NULL, 0) == 0 &&
		    regexec(&qp->regex_instance, instance, 0, NULL, 0) == 0 &&
		    regexec(&qp->regex_name, name, 0, NULL, 0) == 0) {
			DNPRINTF(SWM_D_CLASS, "matched quirk: %s:%s:%s "
			    "mask: %#x, ws: %d\n", qp->class, qp->instance,
			    qp->name, qp->quirk, qp->ws);
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
	DNPRINTF(SWM_D_PROP, "set _NET_WM_DESKTOP: %d\n", win->ws->idx);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
	    ewmh[_NET_WM_DESKTOP].atom, XCB_ATOM_CARDINAL, 32, 1,
	    &win->ws->idx);

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

	if (reparent_window(win))
		win = NULL;

	if (win) {
		DNPRINTF(SWM_D_MISC, "done. win %#x, (x,y) w x h: (%d,%d) %d x %d, "
		    "ws: %d, iconic: %s, transient: %#x\n", win->id, X(win), Y(win),
		    WIDTH(win), HEIGHT(win), win->ws->idx, YESNO(ICONIC(win)),
		    win->transient);
	}
out:
	free(war);
	return (win);
}

void
free_window(struct ws_win *win)
{
	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	xcb_icccm_get_wm_class_reply_wipe(&win->ch);

	/* paint memory */
	memset(win, 0xff, sizeof *win);	/* XXX kill later */

	free(win);
	DNPRINTF(SWM_D_MISC, "done\n");
}

void
unmanage_window(struct ws_win *win)
{
	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	kill_refs(win);

	if (win->state != SWM_WIN_STATE_UNPARENTED &&
	    win->state != SWM_WIN_STATE_UNPARENTING)
		unparent_window(win);

	TAILQ_REMOVE(&win->ws->stack, win, stack_entry);
	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	free_window(win);

	ewmh_update_client_list();
}

void
expose(xcb_expose_event_t *e)
{
	struct ws_win		*w;
	struct swm_bar		*b;
#ifdef SWM_DEBUG
	struct workspace	*ws;
#endif

	DNPRINTF(SWM_D_EVENT, "win %#x, count: %d\n", e->window, e->count);

	if (e->count > 0)
		return;

	if ((b = find_bar(e->window))) {
		bar_draw(b);
		xcb_flush(conn);
	} else if ((w = find_window(e->window)) && w->frame == e->window) {
		draw_frame(w);
#ifdef SWM_DEBUG
		ws = w->ws;
		TAILQ_FOREACH(w, &ws->winlist, entry)
			debug_refresh(w);
#endif
		xcb_flush(conn);
	}

	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
focusin(xcb_focus_in_event_t *e)
{
	struct swm_region	*r;
	struct ws_win		*win;
	struct swm_screen	*s = NULL;

	DNPRINTF(SWM_D_EVENT, "win %#x, mode: %s(%u), detail: %s(%u)\n",
	    e->event, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail);

	if (e->mode != XCB_NOTIFY_MODE_NORMAL)
		return;

	/* Managed window. */
	if ((win = find_window(e->event))) {
		if (win != win->ws->focus && win != win->ws->focus_pending) {
			win->ws->focus_prev = win->ws->focus;
			win->ws->focus = win;
			win->ws->focus_pending = NULL;

			if (win->ws->focus_prev)
				draw_frame(win->ws->focus_prev);

			draw_frame(win);
			raise_window(win);
			update_win_stacking(win);
			focus_flush();
		}
	} else {
		/* Redirect focus if needed. */
		r = find_region(e->event);
		if (r)
			s = r->s;
		else
			s = find_screen(e->event);

		if (s && (win = get_region_focus(s->r_focus))) {
			focus_win_input(win, true);

			if (win->ws->focus == NULL)
				win->ws->focus = win;

			draw_frame(win);
			focus_flush();
		}
	}
}

#ifdef SWM_DEBUG
void
focusout(xcb_focus_out_event_t *e)
{
	DNPRINTF(SWM_D_EVENT, "win %#x, mode: %s(%u), detail: %s(%u)\n",
	    e->event, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail);
}
#endif

void
keypress(xcb_key_press_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;
	xcb_keysym_t		keysym;
	bool			replay = true;

	last_event_time = e->time;

	keysym = xcb_key_press_lookup_keysym(syms, e, 0);

	DNPRINTF(SWM_D_EVENT, "keysym: %u, win (x,y): %#x (%d,%d), detail: %u, "
	    "time: %#x, root (x,y): %#x (%d,%d), child: %#x, state: %u, "
	    "cleaned: %u, same_screen: %s\n", keysym, e->event, e->event_x,
	    e->event_y, e->detail, e->time, e->root, e->root_x, e->root_y,
	    e->child, e->state, CLEANMASK(e->state), YESNO(e->same_screen));

	bp = binding_lookup(CLEANMASK(e->state), KEYBIND, keysym);
	if (bp == NULL) {
		/* Look for catch-all. */
		if ((bp = binding_lookup(ANYMOD, KEYBIND, keysym)) == NULL)
			goto out;
	}

	replay = bp->flags & BINDING_F_REPLAY;

	if ((ap = &actions[bp->action]) == NULL)
		goto out;

	if (bp->action == FN_SPAWN_CUSTOM)
		spawn_custom(root_to_region(e->root, SWM_CK_ALL), &ap->args,
		    bp->spawn_name);
	else if (ap->func)
		ap->func(bp, root_to_region(e->root, SWM_CK_ALL), &ap->args);

	replay = replay && !(ap->flags & FN_F_NOREPLAY);

out:
	if (replay) {
		/* Replay event to event window */
		DNPRINTF(SWM_D_EVENT, "replaying\n");
		xcb_allow_events(conn, XCB_ALLOW_REPLAY_KEYBOARD, e->time);
	} else {
		/* Unfreeze grab events. */
		DNPRINTF(SWM_D_EVENT, "unfreezing\n");
		xcb_allow_events(conn, XCB_ALLOW_SYNC_KEYBOARD, e->time);
	}
	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
keyrelease(xcb_key_release_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;
	xcb_keysym_t		keysym;

	last_event_time = e->time;

	keysym = xcb_key_release_lookup_keysym(syms, e, 0);

	DNPRINTF(SWM_D_EVENT, "keysym: %u, win (x,y): %#x (%d,%d), detail: %u, "
	    "time: %#x, root (x,y): %#x (%d,%d), child: %#x, state: %u, "
	    "same_screen: %s\n", keysym, e->event, e->event_x, e->event_y,
	    e->detail, e->time, e->root, e->root_x, e->root_y, e->child,
	    e->state, YESNO(e->same_screen));

	bp = binding_lookup(CLEANMASK(e->state), KEYBIND, keysym);
	if (bp == NULL)
		/* Look for catch-all. */
		bp = binding_lookup(ANYMOD, KEYBIND, keysym);

	if (bp && (ap = &actions[bp->action]) && !(ap->flags & FN_F_NOREPLAY) &&
	    bp->flags & BINDING_F_REPLAY) {
		/* Replay event to event window */
		DNPRINTF(SWM_D_EVENT, "replaying\n");
		xcb_allow_events(conn, XCB_ALLOW_REPLAY_KEYBOARD, e->time);
	} else {
		/* Unfreeze grab events. */
		DNPRINTF(SWM_D_EVENT, "unfreezing\n");
		xcb_allow_events(conn, XCB_ALLOW_SYNC_KEYBOARD, e->time);
	}

	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
buttonpress(xcb_button_press_event_t *e)
{
	struct ws_win		*win = NULL, *newf;
	struct swm_region	*r, *old_r;
	struct action		*ap;
	struct binding		*bp;
	bool			replay = true;

	last_event_time = e->time;

	DNPRINTF(SWM_D_EVENT, "win (x,y): %#x (%d,%d), detail: %u, time: %#x, "
	    "root (x,y): %#x (%d,%d), child: %#x, state: %u, same_screen: %s\n",
	    e->event, e->event_x, e->event_y, e->detail, e->time, e->root,
	    e->root_x, e->root_y, e->child, e->state, YESNO(e->same_screen));

	if (e->event == e->root) {
		if (e->child) {
			win = find_window(e->child);
			if (win == NULL) {
				r = find_region(e->child);
				if (r)
					focus_region(r);
				replay = false;
			}
		} else {
			/* Focus on region if it's empty. */
			r = root_to_region(e->root, SWM_CK_POINTER);
			if (r && TAILQ_EMPTY(&r->ws->winlist)) {
				old_r = root_to_region(e->root, SWM_CK_FOCUS);
				if (old_r && old_r != r)
					unfocus_win(old_r->ws->focus);

				/* Clear bar since empty. */
				bar_draw(r->bar);
				DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, "
				    "revert-to: PointerRoot, time: %#x\n",
				    r->id, last_event_time);
				xcb_set_input_focus(conn,
				    XCB_INPUT_FOCUS_POINTER_ROOT, r->id,
				    XCB_CURRENT_TIME);
				bar_draw(r->bar);

				/* Don't replay root window events. */
				replay = false;
			}
		}
	} else
		win = find_window(e->event);

	if (win) {
		newf = get_focus_magic(win);
		if (win->ws->focus == newf && newf != win) {
			if (win->focus_redirect == win)
				win->focus_redirect = NULL;
			newf = win;
		}
		focus_win(newf);
	}

	/* Handle any bound action. */
	bp = binding_lookup(CLEANMASK(e->state), BTNBIND, e->detail);
	if (bp == NULL) {
		/* Look for catch-all. */
		if ((bp = binding_lookup(ANYMOD, BTNBIND, e->detail)) == NULL)
			goto out;
	}

	replay = bp->flags & BINDING_F_REPLAY;

	if ((ap = &actions[bp->action]) == NULL)
		goto out;

	if (bp->action == FN_SPAWN_CUSTOM)
		spawn_custom(root_to_region(e->root, SWM_CK_ALL), &ap->args,
		    bp->spawn_name);
	else if (ap->func)
		ap->func(bp, root_to_region(e->root, SWM_CK_ALL), &ap->args);

	replay = replay && !(ap->flags & FN_F_NOREPLAY);

out:
	if (replay) {
		/* Replay event to event window */
		DNPRINTF(SWM_D_EVENT, "replaying\n");
		xcb_allow_events(conn, XCB_ALLOW_REPLAY_POINTER, e->time);
	} else {
		/* Unfreeze grab events. */
		DNPRINTF(SWM_D_EVENT, "unfreezing\n");
		xcb_allow_events(conn, XCB_ALLOW_SYNC_POINTER, e->time);
	}

	focus_flush();
}

void
buttonrelease(xcb_button_release_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;

	last_event_time = e->time;

	DNPRINTF(SWM_D_EVENT, "win (x,y): %#x (%d,%d), detail: %u, time: %#x, "
	    "root (x,y): %#x (%d,%d), child: %#x, state: %u, same_screen: %s\n",
	    e->event, e->event_x, e->event_y, e->detail, e->time, e->root,
	    e->root_x, e->root_y, e->child, e->state, YESNO(e->same_screen));

	bp = binding_lookup(CLEANMASK(e->state), BTNBIND, e->detail);
	if (bp == NULL)
		/* Look for catch-all. */
		bp = binding_lookup(ANYMOD, BTNBIND, e->detail);

	if (bp && (ap = &actions[bp->action]) && !(ap->flags & FN_F_NOREPLAY) &&
	    bp->flags & BINDING_F_REPLAY) {
		/* Replay event to event window */
		DNPRINTF(SWM_D_EVENT, "replaying\n");
		xcb_allow_events(conn, XCB_ALLOW_REPLAY_POINTER, e->time);
	} else {
		/* Unfreeze grab events. */
		DNPRINTF(SWM_D_EVENT, "unfreezing\n");
		xcb_allow_events(conn, XCB_ALLOW_SYNC_POINTER, e->time);
	}

	xcb_flush(conn);
}

#ifdef SWM_DEBUG
char *
get_win_input_model(struct ws_win *win)
{
	char		*inputmodel;
	/*
	 *	Input Model		Input Field	WM_TAKE_FOCUS
	 *	No Input		False		Absent
	 *	Passive			True		Absent
	 *	Locally Active		True		Present
	 *	Globally Active		False		Present
	 */

	if (ACCEPTS_FOCUS(win))
		inputmodel = (win->take_focus) ? "Locally Active" : "Passive";
	else
		inputmodel = (win->take_focus) ? "Globally Active" : "No Input";

	return (inputmodel);
}

void
print_win_geom(xcb_window_t w)
{
	xcb_get_geometry_reply_t	*wa;

	wa = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, w), NULL);
	if (wa == NULL) {
		DNPRINTF(SWM_D_MISC, "win %#x not found\n", w);
		return;
	}

	DNPRINTF(SWM_D_MISC, "win %#x, root: %#x, depth: %u, (x,y) w x h: "
	    "(%d,%d) %d x %d, border: %d\n", w, wa->root, wa->depth, wa->x,
	    wa->y, wa->width, wa->height, wa->border_width);

	free(wa);
}
#endif

#ifdef SWM_DEBUG
char *
get_stack_mode_name(uint8_t mode)
{
	char	*name;

	switch (mode) {
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

	return (name);
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
		new = true;

#ifdef SWM_DEBUG
	if (swm_debug & SWM_D_EVENT) {
		print_win_geom(e->window);

		DNPRINTF(SWM_D_EVENT, "win %#x, parent: %#x, new: %s, "
		    "value_mask: %u { ", e->window, e->parent, YESNO(new),
		    e->value_mask);
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

	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
configurenotify(xcb_configure_notify_event_t *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "win %#x, event win: %#x, (x,y) WxH: (%d,%d) "
	    "%ux%u, border: %u, above_sibling: %#x, override_redirect: %s\n",
	    e->window, e->event, e->x, e->y, e->width, e->height,
	    e->border_width, e->above_sibling, YESNO(e->override_redirect));

	win = find_window(e->window);
	if (win) {
		adjust_font(win);
		if (font_adjusted && win->ws->r) {
			stack(win->ws->r);
			xcb_flush(conn);
		}
	}
}

void
destroynotify(xcb_destroy_notify_event_t *e)
{
	struct ws_win		*win;
	struct workspace	*ws;
	bool			dofocus;

	DNPRINTF(SWM_D_EVENT, "win %#x\n", e->window);

	if ((win = find_window(e->window)) == NULL)
		goto out;

	ws = win->ws;

	if (win->frame == e->window) {
		DNPRINTF(SWM_D_EVENT, "frame for win %#x\n", win->id);
		win->frame = XCB_WINDOW_NONE;
		goto out;
	}

	dofocus = !(pointer_follow(win->s));
	if (dofocus && win == ws->focus)
		ws->focus_pending = get_focus_other(win);

	unmanage_window(win);
	stack(ws->r);

	if (dofocus && WS_FOCUSED(ws)) {
		if (ws->focus_pending) {
			focus_win(ws->focus_pending);
			ws->focus_pending = NULL;
		} else if (ws->focus == NULL) {
			DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
			    "PointerRoot, time: CurrentTime\n", ws->r->id);
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
			    ws->r->id, XCB_CURRENT_TIME);
		}

		focus_flush();
	}

out:
	DNPRINTF(SWM_D_EVENT, "done\n");
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

	return (label);
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

	return (label);
}

char *
get_state_mask_label(uint16_t state)
{
	char *label;

	switch (state) {
	case XCB_KEY_BUT_MASK_SHIFT:
		label = "ShiftMask";
		break;
	case XCB_KEY_BUT_MASK_LOCK:
		label = "LockMask";
		break;
	case XCB_KEY_BUT_MASK_CONTROL:
		label = "ControlMask";
		break;
	case XCB_KEY_BUT_MASK_MOD_1:
		label = "Mod1Mask";
		break;
	case XCB_KEY_BUT_MASK_MOD_2:
		label = "Mod2Mask";
		break;
	case XCB_KEY_BUT_MASK_MOD_3:
		label = "Mod3Mask";
		break;
	case XCB_KEY_BUT_MASK_MOD_4:
		label = "Mod4Mask";
		break;
	case XCB_KEY_BUT_MASK_MOD_5:
		label = "Mod5Mask";
		break;
	case XCB_KEY_BUT_MASK_BUTTON_1:
		label = "Button1Mask";
		break;
	case XCB_KEY_BUT_MASK_BUTTON_2:
		label = "Button2Mask";
		break;
	case XCB_KEY_BUT_MASK_BUTTON_3:
		label = "Button3Mask";
		break;
	case XCB_KEY_BUT_MASK_BUTTON_4:
		label = "Button4Mask";
		break;
	case XCB_KEY_BUT_MASK_BUTTON_5:
		label = "Button5Mask";
		break;
	case 0:
		label = "None";
		break;
	default:
		label = "Unknown";
	}

	return (label);
}

char *
get_wm_state_label(uint8_t state)
{
	char *label;
	switch (state) {
	case XCB_ICCCM_WM_STATE_WITHDRAWN:
		label = "Withdrawn";
		break;
	case XCB_ICCCM_WM_STATE_NORMAL:
		label = "Normal";
		break;
	case XCB_ICCCM_WM_STATE_ICONIC:
		label = "Iconic";
		break;
	default:
		label = "Unknown";
	}

	return (label);
}
#endif

void
enternotify(xcb_enter_notify_event_t *e)
{
	struct ws_win		*win;
	struct swm_region	*r;

	last_event_time = e->time;

	DNPRINTF(SWM_D_FOCUS, "time: %#x, win (x,y): %#x (%d,%d), mode: %s(%d), "
	    "detail: %s(%d), root (x,y): %#x (%d,%d), child: %#x, "
	    "same_screen_focus: %s, state: %s(%d)\n", e->time, e->event,
	    e->event_x, e->event_y, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail, e->root, e->root_x,
	    e->root_y, e->child, YESNO(e->same_screen_focus),
	    get_state_mask_label(e->state), e->state);

	if (e->event == e->root && e->child == XCB_WINDOW_NONE &&
	    e->mode == XCB_NOTIFY_MODE_GRAB &&
	    e->detail == XCB_NOTIFY_DETAIL_INFERIOR) {
		DNPRINTF(SWM_D_EVENT, "ignore; grab inferior\n");
		return;
	}

	if (focus_mode == SWM_FOCUS_MANUAL) {
		DNPRINTF(SWM_D_EVENT, "ignore; manual focus\n");
		return;
	}

	if (focus_mode != SWM_FOCUS_FOLLOW &&
	    e->mode == XCB_NOTIFY_MODE_UNGRAB &&
	    e->detail != XCB_NOTIFY_DETAIL_ANCESTOR) {
		DNPRINTF(SWM_D_EVENT, "ignore; ungrab\n");
		return;
	}

	if ((win = find_window(e->event)) == NULL) {
		if (e->event == e->root) {
			/* If no windows on pointer region, then focus root. */
			r = root_to_region(e->root, SWM_CK_POINTER);
			if (r == NULL) {
				DNPRINTF(SWM_D_EVENT, "ignore; NULL region\n");
				return;
			}

			focus_region(r);
		} else {
			DNPRINTF(SWM_D_EVENT, "ignore; window is NULL\n");
			return;
		}
	} else {
		if (e->mode == XCB_NOTIFY_MODE_NORMAL &&
		    e->detail == XCB_NOTIFY_DETAIL_INFERIOR) {
			DNPRINTF(SWM_D_EVENT, "ignore; enter from inferior\n");
			return;
		}

		focus_win(get_focus_magic(win));
		focus_flush();
	}

	DNPRINTF(SWM_D_EVENT, "done\n");

	xcb_flush(conn);
}

#ifdef SWM_DEBUG
void
leavenotify(xcb_leave_notify_event_t *e)
{
	last_event_time = e->time;

	DNPRINTF(SWM_D_FOCUS, "time: %#x, win (x,y): %#x (%d,%d), mode: %s(%d),"
	    " detail: %s(%d), root (x,y): %#x (%d,%d), child: %#x, "
	    "same_screen_focus: %s, state: %s(%d)\n", e->time, e->event,
	    e->event_x, e->event_y, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail, e->root, e->root_x,
	    e->root_y, e->child, YESNO(e->same_screen_focus),
	    get_state_mask_label(e->state), e->state);
}
#endif

/* Determine if focus should be left to the pointer. */
bool
pointer_follow(struct swm_screen *s) {
	struct swm_region		*r;
	xcb_query_pointer_reply_t	*qpr;
	bool				result;

	if (focus_mode != SWM_FOCUS_FOLLOW)
		return (false);

	result = true;
	/* Check if pointer is on one of our non-frame windows, or root. */
	qpr = xcb_query_pointer_reply(conn,
	    xcb_query_pointer(conn, s->root), NULL);
	if (qpr) {
		if (qpr->child == s->root)
			result = false;
		else
			TAILQ_FOREACH(r, &s->rl, entry) {
				/* Pointer on a (not frame) window we own. */
				if ((r->bar && r->bar->id == qpr->child) ||
				    r->id == qpr->child) {
					result = false;
					break;
				}
#if 0
				/* Pointer outside focused region. */
				if (r != s->r_focus && (X(r) <= qpr->root_x &&
				    qpr->root_x < MAX_X(r) &&
				    Y(r) <= qpr->root_y &&
				    qpr->root_y < MAX_Y(r))) {
					result = false;
					break;
				}
#endif
			}

		free(qpr);
	}

	return (result);
}

void
mapnotify(xcb_map_notify_event_t *e)
{
	struct ws_win		*win, *mainw = NULL;
	struct workspace	*ws;
	bool			setstate = true, dofocus;

	DNPRINTF(SWM_D_EVENT, "event: %#x, win %#x, override_redirect: %u\n",
	    e->event, e->window, e->override_redirect);

	if ((win = manage_window(e->window, spawn_position, false)) == NULL)
		goto out;
	ws = win->ws;

	if (win->state == SWM_WIN_STATE_REPARENTING)
		goto out;

	if (ws->r == NULL) {
		unmap_window(win);
		goto flush;
	}

	dofocus = !(pointer_follow(win->s));

	if (ws->state == SWM_WS_STATE_MAPPED) {
		if (ws->focus_pending && TRANS(ws->focus_pending))
			mainw = find_main_window(win);

		/* If main window is maximized, don't clear it. */
		if ((mainw == NULL) || !MAXIMIZED(mainw))
			if (clear_maximized(ws) > 0) {
				stack(ws->r);
				setstate = false;
			}
	}

	if (setstate) {
		win->mapped = true;
		set_win_state(win, XCB_ICCCM_WM_STATE_NORMAL);
	}

	if (dofocus && WS_FOCUSED(win->ws)) {
		if (ws->focus_pending == win) {
			focus_win(win);
			ws->focus_pending = NULL;
			center_pointer(ws->r);
		}
	}
flush:
	focus_flush();
out:
	DNPRINTF(SWM_D_EVENT, "done\n");
}

#ifdef SWM_DEBUG
char *
get_mapping_notify_label(uint8_t request)
{
	char *label;

	switch(request) {
	case XCB_MAPPING_MODIFIER:
		label = "MappingModifier";
		break;
	case XCB_MAPPING_KEYBOARD:
		label = "MappingKeyboard";
		break;
	case XCB_MAPPING_POINTER:
		label = "MappingPointer";
		break;
	default:
		label = "Unknown";
	}

	return (label);
}
#endif

void
mappingnotify(xcb_mapping_notify_event_t *e)
{
	DNPRINTF(SWM_D_EVENT, "request: %s (%u), first_keycode: %u, "
	    "count: %u\n", get_mapping_notify_label(e->request), e->request,
	    e->first_keycode, e->count);

	if (e->request != XCB_MAPPING_POINTER) {
		xcb_refresh_keyboard_mapping(syms, e);
		grabkeys();
	}

	grabbuttons();
}

void
maprequest(xcb_map_request_event_t *e)
{
	struct ws_win		*win, *w = NULL;
	bool			dofocus;

	DNPRINTF(SWM_D_EVENT, "win %#x\n", e->window);

	win = manage_window(e->window, spawn_position, true);
	if (win == NULL)
		goto out;

	dofocus = !(pointer_follow(win->s));
	/* The new window should get focus; prepare. */
	if ((dofocus && !(win->quirks & SWM_Q_NOFOCUSONMAP)) ||
	    (win->ws->cur_layout == &layouts[SWM_MAX_STACK])) {
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
	stack(win->ws->r);

	focus_flush();
out:
	DNPRINTF(SWM_D_EVENT, "done\n");
}

void
motionnotify(xcb_motion_notify_event_t *e)
{
	struct swm_region	*r = NULL;
	int			i, num_screens;

	last_event_time = e->time;

	DNPRINTF(SWM_D_FOCUS, "time: %#x, win (x,y): %#x (%d,%d), "
	    "detail: %s(%d), root (x,y): %#x (%d,%d), child: %#x, "
	    "same_screen_focus: %s, state: %d\n", e->time, e->event, e->event_x,
	    e->event_y, get_notify_detail_label(e->detail), e->detail, e->root,
	    e->root_x, e->root_y, e->child, YESNO(e->same_screen), e->state);

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
	DNPRINTF(SWM_D_EVENT, "win %#x, atom: %s(%u), time: %#x, state: %u\n",
	    e->window, name, e->atom, e->time, e->state);
	free(name);
#endif
	last_event_time = e->time;

	win = find_window(e->window);
	if (win == NULL)
		return;

	ws = win->ws;

	if (e->atom == a_state) {
		/* State just changed, make sure it gets focused if mapped. */
		if (e->state == XCB_PROPERTY_NEW_VALUE) {
			if (focus_mode != SWM_FOCUS_FOLLOW && WS_FOCUSED(ws)) {
				if (win->mapped &&
				    win->state == SWM_WIN_STATE_REPARENTED &&
				    ws->focus_pending == win) {
					focus_win(ws->focus_pending);
					ws->focus_pending = NULL;
				}
			}
		}
	} else if (e->atom == XCB_ATOM_WM_CLASS ||
	    e->atom == XCB_ATOM_WM_NAME) {
		if (ws->r)
			bar_draw(ws->r->bar);
	} else if (e->atom == a_prot) {
		get_wm_protocols(win);
	} else if (e->atom == XCB_ATOM_WM_NORMAL_HINTS) {
		xcb_icccm_get_wm_normal_hints_reply(conn,
		    xcb_icccm_get_wm_normal_hints(conn, win->id),
		    &win->sh, NULL);
	}

	xcb_flush(conn);
}

void
reparentnotify(xcb_reparent_notify_event_t *e)
{
	struct ws_win	*win;

	DNPRINTF(SWM_D_EVENT, "event: %#x, win %#x, parent: %#x, "
	    "(x,y): (%u,%u), override_redirect: %u\n", e->event, e->window,
	    e->parent, e->x, e->y, e->override_redirect);

	win = find_window(e->window);
	if (win) {
		if (win->state == SWM_WIN_STATE_REPARENTING) {
			win->state = SWM_WIN_STATE_REPARENTED;

			if (win->ws->r && !ICONIC(win))
				map_window(win);
			else
				unmap_window(win);

			update_window(win);
			update_win_stacking(win);
		} else if (win->state == SWM_WIN_STATE_UNPARENTING) {
			win->state = SWM_WIN_STATE_UNPARENTED;
		}
	}
}

void
unmapnotify(xcb_unmap_notify_event_t *e)
{
	struct ws_win		*win;
	struct workspace	*ws;
	bool			dofocus;

	DNPRINTF(SWM_D_EVENT, "event: %#x, win %#x, from_configure: %u\n",
	    e->event, e->window, e->from_configure);

	/* If we aren't managing the window, then ignore. */
	win = find_window(e->window);
	if (win == NULL || win->id != e->window) {
		DNPRINTF(SWM_D_EVENT, "ignore; unmanaged.\n");
		return;
	}

	/* Do nothing if already withdrawn. */
	if (!win->mapped && !ICONIC(win)) {
		DNPRINTF(SWM_D_EVENT, "ignore; withdrawn.\n");
		return;
	}

	ws = win->ws;
	win->mapped = false;

	/* Ignore if reparenting-related. */
	if (win->state != SWM_WIN_STATE_REPARENTED) {
		DNPRINTF(SWM_D_EVENT, "ignore; not reparented\n");
		return;
	}

	dofocus = !(pointer_follow(win->s));

	/* If win was focused, make sure to focus on something else. */
	if (win == ws->focus) {
		if (dofocus)
			ws->focus_pending =
			    get_focus_magic(get_focus_other(win));
		else if (WS_FOCUSED(ws))
			ws->focus_pending =
			    get_focus_magic(get_pointer_win(win->s));

		unfocus_win(win);
	}

	if (ICONIC(win)) {
		/* Iconify. */
		DNPRINTF(SWM_D_EVENT, "iconify\n");
		set_win_state(win, XCB_ICCCM_WM_STATE_ICONIC);
	} else {
		/* Withdraw. */
		DNPRINTF(SWM_D_EVENT, "withdraw\n");
		/* EWMH advises removal of _NET_WM_DESKTOP and _NET_WM_STATE. */
		xcb_delete_property(conn, win->id, ewmh[_NET_WM_DESKTOP].atom);
		xcb_delete_property(conn, win->id, ewmh[_NET_WM_STATE].atom);
		set_win_state(win, XCB_ICCCM_WM_STATE_WITHDRAWN);
		unmanage_window(win);
	}

	stack(ws->r);

	if (WS_FOCUSED(ws) && ws->focus_pending) {
		focus_win(ws->focus_pending);
		ws->focus_pending = NULL;
	}

	if ((dofocus && ws->focus == NULL) || TAILQ_EMPTY(&ws->winlist)) {
		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
		    "PointerRoot, time: CurrentTime\n", ws->r->id);
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
		    ws->r->id, XCB_CURRENT_TIME);
		bar_draw(ws->r->bar);
	}

	center_pointer(ws->r);
	focus_flush();
	DNPRINTF(SWM_D_EVENT, "done\n");
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

	return (label);
}
#endif

void
clientmessage(xcb_client_message_event_t *e)
{
	struct ws_win		*win;
	struct swm_region	*r = NULL;
	union arg		a;
	uint32_t		vals[4];
	int			num_screens, i;
	xcb_map_request_event_t	mre;
#ifdef SWM_DEBUG
	char			*name;

	name = get_atom_name(e->type);
	DNPRINTF(SWM_D_EVENT, "win %#x, atom: %s(%u)\n", e->window, name,
	    e->type);
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
			switchws(NULL, r, &a);
			focus_flush();
		}

		return;
	} else if (e->type == ewmh[_NET_REQUEST_FRAME_EXTENTS].atom) {
		DNPRINTF(SWM_D_EVENT,"set _NET_FRAME_EXTENTS on window\n");
		vals[0] = vals[1] = vals[2] = vals[3] = border_width;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, e->window,
		    a_net_frame_extents, XCB_ATOM_CARDINAL, 32, 4, vals);
		xcb_flush(conn);
		return;
	}

	win = find_window(e->window);
	if (win == NULL) {
		if (e->type == ewmh[_NET_ACTIVE_WINDOW].atom) {
			/* Manage the window with maprequest. */
			DNPRINTF(SWM_D_EVENT, "focus unmanaged; mapping.\n");
			mre.window = e->window;
			maprequest(&mre);
		}
		return;
	}

	if (e->type == ewmh[_NET_ACTIVE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_ACTIVE_WINDOW, source_type: "
		    "%s(%d), timestamp: %#x, active_window: %#x\n",
		    get_source_type_label(e->data.data32[0]), e->data.data32[0],
		    e->data.data32[1], e->data.data32[2]);

		/*
		 * Allow focus changes that are a result of direct user
		 * action and from applications that use the old EWMH spec.
		 */
		if (e->data.data32[0] != EWMH_SOURCE_TYPE_NORMAL ||
		    win->quirks & SWM_Q_OBEYAPPFOCUSREQ) {
			if (WS_FOCUSED(win->ws)) {
				if (win->mapped) {
					focus_win(win);
					win->ws->focus_pending = NULL;
				} else
					win->ws->focus_pending = win;
			} else {
				win->ws->focus_prev = win->ws->focus;
				win->ws->focus = win;
				win->ws->focus_pending = NULL;
			}

			if (ICONIC(win)) {
				/* Uniconify */
				ewmh_apply_flags(win,
				    win->ewmh_flags & ~EWMH_F_HIDDEN);
				ewmh_update_wm_state(win);
				stack(win->ws->r);
			}
		}
	} else if (e->type == ewmh[_NET_CLOSE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_CLOSE_WINDOW\n");
		if (win->can_delete)
			client_msg(win, a_delete, 0);
		else
			xcb_kill_client(conn, win->id);
	} else if (e->type == ewmh[_NET_MOVERESIZE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_MOVERESIZE_WINDOW\n");
		if (ABOVE(win)) {
			if (e->data.data32[0] & (1 << 8)) /* x */
				X(win) = e->data.data32[1];
			if (e->data.data32[0] & (1 << 9)) /* y */
				Y(win) = e->data.data32[2];
			if (e->data.data32[0] & (1 << 10)) /* width */
				WIDTH(win) = e->data.data32[3];
			if (e->data.data32[0] & (1 << 11)) /* height */
				HEIGHT(win) = e->data.data32[4];

			update_window(win);
		} else {
			/* Notify no change was made. */
			config_win(win, NULL);
			/* TODO: Change stack sizes */
		}
	} else if (e->type == ewmh[_NET_RESTACK_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_RESTACK_WINDOW\n");
		vals[0] = e->data.data32[1]; /* Sibling window. */
		vals[1] = e->data.data32[2]; /* Stack mode detail. */

		if (win->frame != XCB_WINDOW_NONE)
			xcb_configure_window(conn, win->frame,
			    XCB_CONFIG_WINDOW_SIBLING |
			    XCB_CONFIG_WINDOW_STACK_MODE, vals);
	} else 	if (e->type == ewmh[_NET_WM_STATE].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_WM_STATE\n");
		ewmh_change_wm_state(win, e->data.data32[1], e->data.data32[0]);
		if (e->data.data32[2])
			ewmh_change_wm_state(win, e->data.data32[2],
			    e->data.data32[0]);

		ewmh_update_wm_state(win);
		stack(win->ws->r);
	} else if (e->type == ewmh[_NET_WM_DESKTOP].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_WM_DESKTOP, new_desktop: %d, "
		    "source_type: %s(%d)\n", e->data.data32[0],
		    get_source_type_label(e->data.data32[1]),
		    e->data.data32[1]);
		DNPRINTF(SWM_D_EVENT, "_NET_WM_DESKTOP\n");
		r = win->ws->r;

		win_to_ws(win, e->data.data32[0], SWM_WIN_UNFOCUS);

		/* Stack source and destination ws, if mapped. */
		if (r != win->ws->r) {
			if (r)
				stack(r);

			if (win->ws->r) {
				if (FLOATING(win))
					load_float_geom(win);

				stack(win->ws->r);
			}
		}
	}

	focus_flush();
}

int
enable_wm(void)
{
	int			num_screens, i;
	const uint32_t		val =
	    XCB_EVENT_MASK_KEY_PRESS |
	    XCB_EVENT_MASK_KEY_RELEASE |
	    XCB_EVENT_MASK_BUTTON_PRESS |
	    XCB_EVENT_MASK_BUTTON_RELEASE |
	    XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_LEAVE_WINDOW |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	    XCB_EVENT_MASK_FOCUS_CHANGE |
	    XCB_EVENT_MASK_PROPERTY_CHANGE |
	    XCB_EVENT_MASK_OWNER_GRAB_BUTTON;
	xcb_screen_t		*sc;
	xcb_void_cookie_t	wac;
	xcb_generic_error_t	*error;

	/* this causes an error if some other window manager is running */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		if ((sc = get_screen(i)) == NULL)
			errx(1, "ERROR: unable to get screen %d.", i);
		DNPRINTF(SWM_D_INIT, "screen %d, root: %#x\n", i, sc->root);
		wac = xcb_change_window_attributes_checked(conn, sc->root,
		    XCB_CW_EVENT_MASK, &val);
		if ((error = xcb_request_check(conn, wac))) {
			DNPRINTF(SWM_D_INIT, "error_code: %u\n",
			    error->error_code);
			free(error);
			return (1);
		}
	}

	return (0);
}

void
new_region(struct swm_screen *s, int x, int y, int w, int h)
{
	struct swm_region	*r = NULL, *n;
	struct workspace	*ws = NULL;
	int			i;
	uint32_t		wa[1];

	DNPRINTF(SWM_D_MISC, "screen[%d]:%dx%d+%d+%d\n", s->idx, w, h, x, y);

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
			r->id = XCB_WINDOW_NONE;
			TAILQ_REMOVE(&s->rl, r, entry);
			TAILQ_INSERT_TAIL(&s->orl, r, entry);
		}
	}

	/* search old regions for one to reuse */

	/* size + location match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (r != NULL && X(r) == x && Y(r) == y &&
		    HEIGHT(r) == h && WIDTH(r) == w)
			break;

	/* size match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (r != NULL && HEIGHT(r) == h && WIDTH(r) == w)
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
	r->bar = NULL;
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

	/* Depth of InputOnly always 0. */
	xcb_create_window(conn, 0, r->id, r->s->root, X(r), Y(r), WIDTH(r),
	    HEIGHT(r), 0, XCB_WINDOW_CLASS_INPUT_ONLY, r->s->visual,
	    XCB_CW_EVENT_MASK, wa);

	/* Make sure region input is at the bottom. */
	wa[0] = XCB_STACK_MODE_BELOW;
	xcb_configure_window(conn, r->id, XCB_CONFIG_WINDOW_STACK_MODE, wa);

	xcb_map_window(conn, r->id);
}

void
scan_randr(int idx)
{
#ifdef SWM_XRR_HAS_CRTC
	int						i, j;
	int						ncrtc = 0, nmodes = 0;
	xcb_randr_get_screen_resources_current_cookie_t	src;
	xcb_randr_get_screen_resources_current_reply_t	*srr;
	xcb_randr_get_crtc_info_cookie_t		cic;
	xcb_randr_get_crtc_info_reply_t			*cir = NULL;
	xcb_randr_crtc_t				*crtc;
	xcb_randr_mode_info_t				*mode;
	int						minrate, currate;
#endif /* SWM_XRR_HAS_CRTC */
	struct swm_region				*r;
	int						num_screens;
	xcb_screen_t					*screen;

	DNPRINTF(SWM_D_MISC, "screen: %d\n", idx);

	if ((screen = get_screen(idx)) == NULL)
		errx(1, "ERROR: unable to get screen %d.", idx);

	num_screens = get_screen_count();
	if (idx >= num_screens)
		errx(1, "scan_randr: invalid screen");

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&screens[idx].rl)) != NULL) {
		r->ws->old_r = r->ws->r = NULL;
		bar_cleanup(r);
		xcb_destroy_window(conn, r->id);
		r->id = XCB_WINDOW_NONE;
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
		} else {
			ncrtc = srr->num_crtcs;
			nmodes = srr->num_modes;
		}

		minrate = -1;
		mode = xcb_randr_get_screen_resources_current_modes(srr);
		crtc = xcb_randr_get_screen_resources_current_crtcs(srr);
		for (i = 0; i < ncrtc; i++) {
			currate = SWM_RATE_DEFAULT;
			cic = xcb_randr_get_crtc_info(conn, crtc[i],
			    XCB_CURRENT_TIME);
			cir = xcb_randr_get_crtc_info_reply(conn, cic, NULL);
			if (cir == NULL)
				continue;
			if (cir->num_outputs == 0) {
				free(cir);
				continue;
			}

			if (cir->mode == 0) {
				new_region(&screens[idx], 0, 0,
				    screen->width_in_pixels,
				    screen->height_in_pixels);
			} else {
				new_region(&screens[idx],
				    cir->x, cir->y, cir->width, cir->height);

				/* Determine the crtc refresh rate. */
				for (j = 0; j < nmodes; j++) {
					if (mode[j].id == cir->mode) {
						if (mode[j].htotal &&
						    mode[j].vtotal)
							currate =
							    mode[j].dot_clock /
							    (mode[j].htotal *
							     mode[j].vtotal);
						break;
					}
				}

				if (minrate == -1 || currate < minrate)
					minrate = currate;
			}
			free(cir);
		}
		free(srr);

		screens[idx].rate = (minrate > 0) ? minrate : SWM_RATE_DEFAULT;
		DNPRINTF(SWM_D_MISC, "Screen %d update rate: %d\n",
		    idx, screens[idx].rate);
	}
#endif /* SWM_XRR_HAS_CRTC */

	/* If detection failed, create a single region that spans the screen. */
	if (TAILQ_EMPTY(&screens[idx].rl))
		new_region(&screens[idx], 0, 0, screen->width_in_pixels,
		    screen->height_in_pixels);

#ifdef SWM_XRR_HAS_CRTC
out:
#endif
	/* The screen shouldn't focus on unused regions. */
	TAILQ_FOREACH(r, &screens[idx].orl, entry) {
		if (screens[idx].r_focus == r)
			screens[idx].r_focus = NULL;
	}

	DNPRINTF(SWM_D_MISC, "done.\n");
}

void
screenchange(xcb_randr_screen_change_notify_event_t *e)
{
	struct swm_region		*r;
	struct workspace		*ws;
	struct ws_win			*win;
	int				i, j, num_screens;

	DNPRINTF(SWM_D_EVENT, "root: %#x\n", e->root);

	num_screens = get_screen_count();
	/* silly event doesn't include the screen index */
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == e->root)
			break;
	if (i >= num_screens)
		errx(1, "screenchange: screen not found.");

	/* brute force for now, just re-enumerate the regions */
	scan_randr(i);

#ifdef SWM_DEBUG
	print_win_geom(e->root);
#endif
	/* add bars to all regions */
	TAILQ_FOREACH(r, &screens[i].rl, entry)
		bar_setup(r);

	/* Stack all regions. */
	TAILQ_FOREACH(r, &screens[i].rl, entry)
		stack(r);

	/* Focus region. */
	if (screens[i].r_focus) {
		/* Update bar colors for existing focus. */
		r = screens[i].r_focus;
		screens[i].r_focus = NULL;
		set_region(r);
	} else {
		/* Focus on the first region. */
		r = TAILQ_FIRST(&screens[i].rl);
		if (r)
			focus_region(r);
	}

	/* Cleanup unused previously visible workspaces. */
	for (j = 0; j < workspace_limit; j++) {
		ws = &screens[i].ws[j];
		if (ws->r == NULL && ws->state != SWM_WS_STATE_HIDDEN) {
			TAILQ_FOREACH(win, &ws->winlist, entry)
				unmap_window(win);
			ws->state = SWM_WS_STATE_HIDDEN;
		}
	}

	focus_flush();

	/* Update workspace state and bar on all regions. */
	TAILQ_FOREACH(r, &screens[i].rl, entry) {
		r->ws->state = SWM_WS_STATE_MAPPED;
		bar_draw(r->bar);
	}
}

void
grab_windows(void)
{
	struct swm_region		*r = NULL;
	xcb_query_tree_cookie_t		qtc;
	xcb_query_tree_reply_t		*qtr;
	xcb_get_property_cookie_t	pc;
	xcb_get_property_reply_t	*pr;
	xcb_window_t			*wins = NULL, trans, *cwins = NULL;
	int				i, j, k, n, no, num_screens;

	DNPRINTF(SWM_D_INIT, "begin\n");
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

		/* Manage top-level windows first, then transients. */
		/* TODO: allow transients to be managed before leader. */
		DNPRINTF(SWM_D_INIT, "grab top-level windows.\n");
		for (j = 0; j < no; j++) {
			TAILQ_FOREACH(r, &screens[i].rl, entry) {
				if (r->id == wins[j]) {
					DNPRINTF(SWM_D_INIT, "skip %#x; region "
					    "input window.\n", wins[j]);
					break;
				} else if (r->bar->id == wins[j]) {
					DNPRINTF(SWM_D_INIT, "skip %#x; "
					    "region bar.\n", wins[j]);
					break;
				}
			}

			if (r)
				continue;

			pc = xcb_icccm_get_wm_transient_for(conn, wins[j]);
			if (xcb_icccm_get_wm_transient_for_reply(conn, pc,
			    &trans, NULL)) {
				DNPRINTF(SWM_D_INIT, "skip %#x; transient for "
				    "%#x\n", wins[j], trans);
				continue;
			}

			manage_window(wins[j], SWM_STACK_TOP, false);
		}

		DNPRINTF(SWM_D_INIT, "grab transient windows\n");
		for (j = 0; j < no; j++) {
			pc = xcb_icccm_get_wm_transient_for(conn, wins[j]);
			if (xcb_icccm_get_wm_transient_for_reply(conn, pc,
			    &trans, NULL))
				manage_window(wins[j], SWM_STACK_TOP, false);
		}
		free(qtr);
	}
	DNPRINTF(SWM_D_INIT, "done\n");
}

void
setup_screens(void)
{
	struct workspace		*ws;
	xcb_pixmap_t			pxmap;
	xcb_depth_iterator_t		diter;
	xcb_visualtype_iterator_t	viter;
	xcb_void_cookie_t		cmc;
	xcb_generic_error_t		*error;
	XVisualInfo			vtmpl, *vlist;
	int				i, j, k, num_screens, vcount;
	uint32_t			wa[1];
	xcb_screen_t			*screen;

	num_screens = get_screen_count();
	if ((screens = calloc(num_screens, sizeof(struct swm_screen))) == NULL)
		err(1, "setup_screens: calloc: failed to allocate memory for "
		    "screens");

	wa[0] = cursors[XC_LEFT_PTR].cid;

	/* map physical screens */
	for (i = 0; i < num_screens; i++) {
		DNPRINTF(SWM_D_WS, "init screen: %d\n", i);
		screens[i].idx = i;
		screens[i].r_focus = NULL;

		TAILQ_INIT(&screens[i].rl);
		TAILQ_INIT(&screens[i].orl);
		if ((screen = get_screen(i)) == NULL)
			errx(1, "ERROR: unable to get screen %d.", i);
		screens[i].rate = SWM_RATE_DEFAULT;
		screens[i].root = screen->root;
		screens[i].depth = screen->root_depth;
		screens[i].visual = screen->root_visual;
		screens[i].colormap = screen->default_colormap;
		screens[i].xvisual = DefaultVisual(display, i);

		DNPRINTF(SWM_D_INIT, "root_depth: %d, screen_depth: %d\n",
		    screen->root_depth, xcb_aux_get_depth(conn, screen));

		/* Use the maximum supported screen depth. */
		for (diter = xcb_screen_allowed_depths_iterator(screen);
		    diter.rem; xcb_depth_next(&diter)) {
			if (diter.data->depth > screens[i].depth) {
				viter = xcb_depth_visuals_iterator(diter.data);
				if (viter.rem) {
					screens[i].depth = diter.data->depth;
					screens[i].visual =
					    viter.data->visual_id;
					DNPRINTF(SWM_D_INIT, "Found visual %#x,"
					    " depth: %d.\n", screens[i].visual,
					    screens[i].depth);
				}
			}
		}

		/* Create a new colormap if not using the root visual */
		if (screens[i].visual != screen->root_visual) {
			/* Find the corresponding Xlib visual struct for Xft. */
			vtmpl.visualid = screens[i].visual;
			vlist = XGetVisualInfo(display, VisualIDMask, &vtmpl,
			    &vcount);
			if (vcount > 0)
				screens[i].xvisual = vlist[0].visual;

			DNPRINTF(SWM_D_INIT, "Creating new colormap.\n");
			screens[i].colormap = xcb_generate_id(conn);
			cmc = xcb_create_colormap_checked(conn,
			    XCB_COLORMAP_ALLOC_NONE, screens[i].colormap,
			    screens[i].root, screens[i].visual);
			if ((error = xcb_request_check(conn, cmc))) {
				DNPRINTF(SWM_D_MISC, "error:\n");
				event_error(error);
				free(error);
			}
		}

		DNPRINTF(SWM_D_INIT, "Using visual %#x, depth: %d.\n",
		    screens[i].visual, xcb_aux_get_depth_of_visual(screen,
		    screens[i].visual));

		/* set default colors */
		setscreencolor("red", i, SWM_S_COLOR_FOCUS);
		setscreencolor("red", i, SWM_S_COLOR_FOCUS_MAXIMIZED);
		setscreencolor("rgb:88/88/88", i, SWM_S_COLOR_UNFOCUS);
		setscreencolor("rgb:88/88/88", i,
		    SWM_S_COLOR_UNFOCUS_MAXIMIZED);
		setscreencolor("rgb:00/80/80", i, SWM_S_COLOR_BAR_BORDER);
		setscreencolor("rgb:00/40/40", i,
		    SWM_S_COLOR_BAR_BORDER_UNFOCUS);
		setscreencolor("black", i, SWM_S_COLOR_BAR);
		setscreencolor("rgb:00/80/80", i, SWM_S_COLOR_BAR_SELECTED);
		setscreencolor("rgb:a0/a0/a0", i, SWM_S_COLOR_BAR_FONT);
		setscreencolor("black", i, SWM_S_COLOR_BAR_FONT_SELECTED);

		/* set default cursor */
		xcb_change_window_attributes(conn, screens[i].root,
		    XCB_CW_CURSOR, wa);

		/* Create graphics context. */

		/* xcb_create_gc determines root/depth from a drawable. */
		pxmap = xcb_generate_id(conn);
		xcb_create_pixmap(conn, screens[i].depth, pxmap,
		    screens[i].root, 10, 10);

		screens[i].gc = xcb_generate_id(conn);
		wa[0] = 0;
		xcb_create_gc(conn, screens[i].gc, pxmap,
		    XCB_GC_GRAPHICS_EXPOSURES, wa);
		xcb_free_pixmap(conn, pxmap);

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
			ws->focus_raise = NULL;
			ws->r = NULL;
			ws->old_r = NULL;
			ws->state = SWM_WS_STATE_HIDDEN;
			TAILQ_INIT(&ws->stack);
			TAILQ_INIT(&ws->winlist);

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
setup_extensions(void)
{
	const xcb_query_extension_reply_t	*qep;
	xcb_randr_query_version_reply_t		*rqvr;
#ifdef SWM_XCB_HAS_XINPUT
	xcb_input_xi_query_version_reply_t	*xiqvr;
#endif

	randr_support = false;
	qep = xcb_get_extension_data(conn, &xcb_randr_id);
	if (qep->present) {
		rqvr = xcb_randr_query_version_reply(conn,
		    xcb_randr_query_version(conn, 1, 1), NULL);
		if (rqvr) {
			if (rqvr->major_version >= 1) {
				randr_support = true;
				randr_eventbase = qep->first_event;
			}
			free(rqvr);
		}
	}

#ifdef SWM_XCB_HAS_XINPUT
	xinput2_support = false;
	qep = xcb_get_extension_data(conn, &xcb_input_id);
	if (qep->present) {
		xiqvr = xcb_input_xi_query_version_reply(conn,
		    xcb_input_xi_query_version(conn, 2, 0), NULL);
		if (xiqvr) {
			if (xiqvr->major_version >= 2)
				xinput2_support = true;
			free(xiqvr);
		}
	}
#endif
}

void
setup_globals(void)
{
	if ((bar_fonts = strdup(SWM_BAR_FONTS)) == NULL)
		err(1, "bar_fonts: strdup");

	if ((clock_format = strdup("%a %b %d %R %Z %Y")) == NULL)
		err(1, "clock_format: strdup");

	if ((stack_mark_max = strdup("[ ]")) == NULL)
		err(1, "stack_mark_max: strdup");

	if ((stack_mark_vertical = strdup("[|]")) == NULL)
		err(1, "stack_mark_vertical: strdup");

	if ((stack_mark_vertical_flip = strdup("[>]")) == NULL)
		err(1, "stack_mark_vertical_flip: strdup");

	if ((stack_mark_horizontal = strdup("[-]")) == NULL)
		err(1, "stack_mark_horizontal: strdup");

	if ((stack_mark_horizontal_flip = strdup("[v]")) == NULL)
		err(1, "stack_mark_horizontal_flip: strdup");

	if ((syms = xcb_key_symbols_alloc(conn)) == NULL)
		errx(1, "unable to allocate key symbols.");

	if ((workspace_mark_current = strdup("*")) == NULL)
		err(1, "workspace_mark_current: strdup");

	if ((workspace_mark_urgent = strdup("!")) == NULL)
		err(1, "workspace_mark_urgent: strdup");

	if ((workspace_mark_active = strdup("^")) == NULL)
		err(1, "workspace_mark_active: strdup");

	if ((workspace_mark_empty = strdup("-")) == NULL)
		err(1, "workspace_mark_empty: strdup");

	a_state = get_atom_from_string("WM_STATE");
	a_prot = get_atom_from_string("WM_PROTOCOLS");
	a_delete = get_atom_from_string("WM_DELETE_WINDOW");
	a_net_frame_extents = get_atom_from_string("_NET_FRAME_EXTENTS");
	a_net_supported = get_atom_from_string("_NET_SUPPORTED");
	a_net_wm_check = get_atom_from_string("_NET_SUPPORTING_WM_CHECK");
	a_takefocus = get_atom_from_string("WM_TAKE_FOCUS");
	a_utf8_string = get_atom_from_string("UTF8_STRING");
	a_swm_ws = get_atom_from_string("_SWM_WS");
}

void
scan_config(void)
{
	struct stat		sb;
	struct passwd		*pwd;
	char			conf[PATH_MAX];
	char			*cfile = NULL, *str = NULL, *ret, *s;
	int			i;

	/* To get $HOME */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user: %d", getuid());

	/* XDG search with backwards compatibility. */
	for (i = 0; ; i++) {
		conf[0] = '\0';
		switch (i) {
		case 0:
			/* 1) $XDG_CONFIG_HOME/spectrwm/spectrwm.conf */
			ret = getenv("XDG_CONFIG_HOME");
			if (ret && ret[0])
				snprintf(conf, sizeof conf, "%s/spectrwm/%s",
				    ret, SWM_CONF_FILE);
			else
				/* 2) Default is $HOME/.config */
				snprintf(conf, sizeof conf,
				    "%s/.config/spectrwm/%s", pwd->pw_dir,
				    SWM_CONF_FILE);
			break;
		case 1:
			/* 3) $HOME/.spectrwm.conf */
			snprintf(conf, sizeof conf, "%s/.%s", pwd->pw_dir,
			    SWM_CONF_FILE);
			break;
		case 2:
			/* 4) $XDG_CONFIG_DIRS (colon-separated set of dirs) */
			ret = getenv("XDG_CONFIG_DIRS");
			if (ret && ret[0]) {
				if ((str = strdup(ret)) == NULL)
					err(1, "xdg strdup");
			} else {
				/* 5) Fallback to default: /etc/xdg */
				if (asprintf(&str, "/etc/xdg") == -1)
					err(1, "xdg asprintf");
			}

			/* Try ./spectrwm/spectrwm.conf under each dir. */
			while ((s = strsep(&str, ":")) != NULL) {
				if (*s == '\0')
					continue;
				snprintf(conf, sizeof conf, "%s/spectrwm/%s", s,
				    SWM_CONF_FILE);
				if (stat(conf, &sb) != -1 &&
				    S_ISREG(sb.st_mode)) {
					/* Found a file. */
					cfile = conf;
					break;
				}
				conf[0] = '\0';
			}
			free(str);
			break;
		case 3:
			/* 6) /etc/spectrwm.conf */
			snprintf(conf, sizeof conf, "/etc/%s", SWM_CONF_FILE);
			break;
		case 4:
			/* 7) $HOME/.scrotwm.conf */
			snprintf(conf, sizeof conf, "%s/.%s", pwd->pw_dir,
			    SWM_CONF_FILE_OLD);
			break;
		case 5:
			/* 8) /etc/scrotwm.conf */
			snprintf(conf, sizeof conf, "/etc/%s",
			    SWM_CONF_FILE_OLD);
			break;
		default:
			goto done;
		}

		if (cfile == NULL && conf[0] && stat(conf, &sb) != -1 &&
		    S_ISREG(sb.st_mode))
			cfile = conf;

		if (cfile) {
			conf_load(cfile, SWM_CONF_DEFAULT);
			break;
		}
	}

done:
	DNPRINTF(SWM_D_INIT, "done\n");
}

void
shutdown_cleanup(void)
{
	struct swm_region	*r;
	struct ws_win		*w;
	struct workspace	*ws;
	int			i, num_screens;

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		err(1, "can't disable alarm");

	bar_extra_stop();
	unmap_all();

	cursors_cleanup();

	clear_quirks();
	clear_spawns();
	clear_bindings();

	teardown_ewmh();

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i) {
		int j;

		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: PointerRoot, revert-to: "
		    "PointerRoot, time: CurrentTime\n");
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
		    XCB_INPUT_FOCUS_POINTER_ROOT, XCB_CURRENT_TIME);

		if (screens[i].gc != XCB_NONE)
			xcb_free_gc(conn, screens[i].gc);
		if (!bar_font_legacy) {
			XftColorFree(display, screens[i].xvisual,
			    screens[i].colormap, &search_font_color);
			for (j = 0; j < num_fg_colors; j++)
				XftColorFree(display, screens[i].xvisual,
				    screens[i].colormap, &bar_fg_colors[j]);
		}

		for (j = 0; j < SWM_S_COLOR_MAX; ++j) {
			free(screens[i].c[j].name);
		}

		/* Free window memory. */
		for (j = 0; j < SWM_WS_MAX; ++j) {
			ws = &screens[i].ws[j];
			free(ws->name);

			while ((w = TAILQ_FIRST(&ws->winlist)) != NULL) {
				TAILQ_REMOVE(&ws->winlist, w, entry);
				free_window(w);
			}
		}

		/* Free region memory. */
		while ((r = TAILQ_FIRST(&screens[i].rl)) != NULL) {
			TAILQ_REMOVE(&screens[i].rl, r, entry);
			free(r->bar);
			free(r);
		}

		while ((r = TAILQ_FIRST(&screens[i].orl)) != NULL) {
			TAILQ_REMOVE(&screens[i].rl, r, entry);
			free(r->bar);
			free(r);
		}
	}
	free(screens);

	free(bar_format);
	free(bar_fonts);
	free(clock_format);
	free(startup_exception);
	free(stack_mark_max);
	free(stack_mark_vertical);
	free(stack_mark_vertical_flip);
	free(stack_mark_horizontal);
	free(stack_mark_horizontal_flip);
	free(workspace_mark_current);
	free(workspace_mark_urgent);
	free(workspace_mark_active);
	free(workspace_mark_empty);

	if (bar_fs)
		XFreeFontSet(display, bar_fs);

	for (i = 0; i < num_xftfonts; i++)
		if (bar_xftfonts[i])
			XftFontClose(display, bar_xftfonts[i]);

	if (font_pua_index)
		 XftFontClose(display, bar_xftfonts[font_pua_index]);

	xcb_key_symbols_free(syms);
	xcb_flush(conn);
	xcb_aux_sync(conn);
	xcb_disconnect(conn);
}

void
event_error(xcb_generic_error_t *e)
{
	(void)e;

	DNPRINTF(SWM_D_EVENT, "%s(%u) from %s(%u), sequence: %u, "
	    "resource_id: %u, minor_code: %u\n",
	    xcb_event_get_error_label(e->error_code), e->error_code,
	    xcb_event_get_request_label(e->major_code), e->major_code,
	    e->sequence, e->resource_id, e->minor_code);
}

void
event_handle(xcb_generic_event_t *evt)
{
	uint8_t			type = XCB_EVENT_RESPONSE_TYPE(evt);

	DNPRINTF(SWM_D_EVENT, "%s(%d), seq %u, sent: %s\n",
	    xcb_event_get_label(XCB_EVENT_RESPONSE_TYPE(evt)),
	    XCB_EVENT_RESPONSE_TYPE(evt), evt->sequence,
	    YESNO(XCB_EVENT_SENT(evt)));

	switch (type) {
#define EVENT(type, callback) case type: callback((void *)evt); return
	EVENT(0, event_error);
	EVENT(XCB_BUTTON_PRESS, buttonpress);
	EVENT(XCB_BUTTON_RELEASE, buttonrelease);
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
	EVENT(XCB_FOCUS_IN, focusin);
#ifdef SWM_DEBUG
	EVENT(XCB_FOCUS_OUT, focusout);
#endif
	/*EVENT(XCB_GRAPHICS_EXPOSURE, );*/
	/*EVENT(XCB_GRAVITY_NOTIFY, );*/
	EVENT(XCB_KEY_PRESS, keypress);
	EVENT(XCB_KEY_RELEASE, keyrelease);
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
	EVENT(XCB_REPARENT_NOTIFY, reparentnotify);
	/*EVENT(XCB_RESIZE_REQUEST, );*/
	/*EVENT(XCB_SELECTION_CLEAR, );*/
	/*EVENT(XCB_SELECTION_NOTIFY, );*/
	/*EVENT(XCB_SELECTION_REQUEST, );*/
	EVENT(XCB_UNMAP_NOTIFY, unmapnotify);
	/*EVENT(XCB_VISIBILITY_NOTIFY, );*/
#undef EVENT
	}
	if (randr_support &&
	    (type - randr_eventbase) == XCB_RANDR_SCREEN_CHANGE_NOTIFY)
		screenchange((void *)evt);
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: spectrwm [-c file] [-v]\n"
	    "        -c FILE        load configuration file\n"
	    "        -v             display version information and exit\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct pollfd		pfd[2];
	struct sigaction	sact;
	struct swm_region	*r;
	xcb_generic_event_t	*evt;
	xcb_mapping_notify_event_t *mne;
	char			*cfile = NULL;
	int			ch, xfd, i, num_screens, num_readable;
	bool			stdin_ready = false, startup = true;

	while ((ch = getopt(argc, argv, "c:hv")) != -1) {
		switch (ch) {
		case 'c':
			cfile = optarg;
			break;
		case 'v':
			fprintf(stderr, "spectrwm %s Build: %s\n",
			    SPECTRWM_VERSION, buildstr);
			exit(1);
			break;
		default:
			usage();
		}
	}

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
		errx(1, "unable to open display");

	conn = XGetXCBConnection(display);
	if (xcb_connection_has_error(conn))
		errx(1, "unable to get XCB connection");

	XSetEventQueueOwner(display, XCBOwnsEventQueue);

	xcb_prefetch_extension_data(conn, &xcb_randr_id);
	xfd = xcb_get_file_descriptor(conn);

	xcb_grab_server(conn);
	xcb_aux_sync(conn);

	/* Flush the event queue. */
	while ((evt = get_next_event(false))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_MAPPING_NOTIFY:
			/* Need to handle mapping changes during startup. */
			mne = (xcb_mapping_notify_event_t *)evt;
			if (mne->request == XCB_MAPPING_KEYBOARD)
				xcb_refresh_keyboard_mapping(syms, mne);
			break;
#ifdef SWM_DEBUG
		case 0:
			/* Display errors. */
			event_handle(evt);
			break;
#endif
		}
		free(evt);
	}

	if (enable_wm())
		errx(1, "another window manager is currently running");

	cursors_load();

	xcb_aux_sync(conn);

	setup_globals();
	setup_extensions();
	setup_screens();
	setup_ewmh();
	setup_keybindings();
	setup_btnbindings();
	setup_quirks();
	setup_spawn();

	if (cfile)
		conf_load(cfile, SWM_CONF_DEFAULT);
	else
		scan_config();

	validate_spawns();

	if (getenv("SWM_STARTED") == NULL)
		setenv("SWM_STARTED", "YES", 1);

	/* setup all bars */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			bar_setup(r);

#ifdef __OpenBSD__
	if (pledge("stdio proc exec", NULL) == -1)
		err(1, "pledge");
#endif

	/* Manage existing windows. */
	grab_windows();

	grabkeys();
	grabbuttons();

	/* Stack all regions to trigger mapping. */
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			stack(r);

	xcb_ungrab_server(conn);
	xcb_flush(conn);

	/* Update state and bar of each newly mapped workspace. */
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			r->ws->state = SWM_WS_STATE_MAPPED;
			bar_draw(r->bar);
		}

	memset(&pfd, 0, sizeof(pfd));
	pfd[0].fd = xfd;
	pfd[0].events = POLLIN;
	pfd[1].fd = STDIN_FILENO;
	pfd[1].events = POLLIN;

	while (running) {
		while ((evt = get_next_event(false))) {
			if (!running)
				goto done;
			event_handle(evt);
			free(evt);
		}

		/* If just (re)started, set default focus if needed. */
		if (startup) {
			startup = false;

			if (focus_mode == SWM_FOCUS_FOLLOW)
				r = root_to_region(screens[0].root,
				    SWM_CK_POINTER);
			else
				r = TAILQ_FIRST(&screens[0].rl);

			if (r) {
				focus_region(r);
				focus_flush();
				continue;
			}
		}

		if (search_resp)
			search_do_resp();

		num_readable = poll(pfd, bar_extra ? 2 : 1, 1000);
		if (num_readable == -1) {
			DNPRINTF(SWM_D_MISC, "poll failed: %s",
			    strerror(errno));
		} else if (num_readable > 0 && bar_extra &&
		    pfd[1].revents & POLLIN) {
			stdin_ready = true;
		}

		if (restart_wm)
			restart(NULL, NULL, NULL);

		if (!running)
			goto done;

		if (stdin_ready) {
			stdin_ready = false;
			bar_extra_update();
		}

		/* Need to ensure the bar(s) are always updated. */
		for (i = 0; i < num_screens; i++)
			TAILQ_FOREACH(r, &screens[i].rl, entry)
				bar_draw(r->bar);

		xcb_flush(conn);
	}
done:
	shutdown_cleanup();

	return (0);
}
