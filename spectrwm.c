/*
 * Copyright (c) 2009-2019 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009-2011 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 * Copyright (c) 2010 Tuukka Kataja <stuge@xor.fi>
 * Copyright (c) 2011 Jason L. Wright <jason@thought.net>
 * Copyright (c) 2011-2024 Reginald Kennedy <rk@rejii.com>
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
#if defined(__NetBSD__)
#include <inttypes.h>
#endif
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#if !defined(SWM_XCB_HAS_XINPUT) && (defined(__linux__) || defined(__FreeBSD__)	\
    || defined(__OpenBSD__) || defined(__NetBSD__))
#define SWM_XCB_HAS_XINPUT
#endif
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#ifdef SWM_XCB_HAS_XINPUT
#include <xcb/xinput.h>
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

/* Enable to turn on debug output by default. */
/*#define SWM_DEBUG*/

#define DPRINTF(x...) do {							\
	if (swm_debug)								\
		fprintf(stderr, x);						\
} while (0)
#define DNPRINTF(n, fmt, args...) do {						\
	if (swm_debug & n) 							\
		fprintf(stderr, "%ld %s: " fmt,					\
		    (long)(time(NULL) - time_started), __func__, ## args);	\
} while (0)

#define YESNO(x)		((x) ? "yes" : "no")

#define SWM_D_MISC		(0x00001)
#define SWM_D_EVENT		(0x00002)
#define SWM_D_WS		(0x00004)
#define SWM_D_FOCUS		(0x00008)
#define SWM_D_MOVE		(0x00010)
#define SWM_D_STACK		(0x00020)
#define SWM_D_MOUSE		(0x00040)
#define SWM_D_PROP		(0x00080)
#define SWM_D_CLASS		(0x00100)
#define SWM_D_KEY		(0x00200)
#define SWM_D_QUIRK		(0x00400)
#define SWM_D_SPAWN		(0x00800)
#define SWM_D_EVENTQ		(0x01000)
#define SWM_D_CONF		(0x02000)
#define SWM_D_BAR		(0x04000)
#define SWM_D_INIT		(0x08000)
#define SWM_D_ATOM		(0x10000)

#define SWM_D_ALL		(0x1ffff)

/* Debug output is disabled by default unless SWM_DEBUG is set. */
uint32_t		swm_debug = 0
#ifdef SWM_DEBUG
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
			    | SWM_D_ATOM
#endif /* SWM_DEBUG */
			    ;

#define ALLOCSTR(s, x...) do {							\
	if (s && asprintf(s, x) == -1)						\
		err(1, "%s: asprintf", __func__);				\
} while (0)

/* _NET_WM_STATE flags */
#define EWMH_F_MAXIMIZED_VERT		(1 << 0)
#define EWMH_F_MAXIMIZED_HORZ		(1 << 1)
#define EWMH_F_SKIP_TASKBAR		(1 << 2)
#define EWMH_F_SKIP_PAGER		(1 << 3)
#define EWMH_F_HIDDEN			(1 << 4)
#define EWMH_F_FULLSCREEN		(1 << 5)
#define EWMH_F_ABOVE			(1 << 6)
#define EWMH_F_BELOW			(1 << 7)
#define EWMH_F_DEMANDS_ATTENTION	(1 << 8)
#define SWM_F_MANUAL			(1 << 9)
#define SWM_EWMH_ACTION_COUNT_MAX	(10)

#define EWMH_F_MAXIMIZED	(EWMH_F_MAXIMIZED_VERT | EWMH_F_MAXIMIZED_HORZ)
#define EWMH_F_UNTILED		(EWMH_F_ABOVE | EWMH_F_FULLSCREEN |	       \
    EWMH_F_MAXIMIZED)

#define EWMH_WINDOW_TYPE_DESKTOP	(1 << 0)
#define EWMH_WINDOW_TYPE_DOCK		(1 << 1)
#define EWMH_WINDOW_TYPE_TOOLBAR	(1 << 2)
#define EWMH_WINDOW_TYPE_MENU		(1 << 3)
#define EWMH_WINDOW_TYPE_UTILITY	(1 << 4)
#define EWMH_WINDOW_TYPE_SPLASH		(1 << 5)
#define EWMH_WINDOW_TYPE_DIALOG		(1 << 6)
#define EWMH_WINDOW_TYPE_DROPDOWN_MENU	(1 << 7)
#define EWMH_WINDOW_TYPE_POPUP_MENU	(1 << 8)
#define EWMH_WINDOW_TYPE_TOOLTIP	(1 << 9)
#define EWMH_WINDOW_TYPE_NOTIFICATION	(1 << 10)
#define EWMH_WINDOW_TYPE_COMBO		(1 << 11)
#define EWMH_WINDOW_TYPE_DND		(1 << 12)
#define EWMH_WINDOW_TYPE_NORMAL		(1 << 13)
#define EWMH_WINDOW_TYPE_COUNT		(14)

#define WINDESKTOP(w)		((w)->type & EWMH_WINDOW_TYPE_DESKTOP)
#define WINDOCK(w)		((w)->type & EWMH_WINDOW_TYPE_DOCK)
#define WINTOOLBAR(w)		((w)->type & EWMH_WINDOW_TYPE_TOOLBAR)
#define WINUTILITY(w)		((w)->type & EWMH_WINDOW_TYPE_UTILITY)
#define WINSPLASH(w)		((w)->type & EWMH_WINDOW_TYPE_SPLASH)
#define WINDIALOG(w)		((w)->type & EWMH_WINDOW_TYPE_DIALOG)
#define WINNOTIFY(w)		((w)->type & EWMH_WINDOW_TYPE_NOTIFICATION)

#define EWMH_ALL_DESKTOPS		(0xffffffff)

/* convert 8-bit to 16-bit */
#define RGB_8_TO_16(col)	(((col) << 8) + (col))

#define SWM_TO_XRENDER_COLOR(sc, xrc) do {				\
	(xrc).red = (sc).r;							\
	(xrc).green = (sc).g;						\
	(xrc).blue = (sc).b;						\
	(xrc).alpha = (sc).a;						\
} while (0);

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
#define CANCELKEY		XK_Escape

#define SWM_PROPLEN		(16)
#define SWM_FUNCNAME_LEN	(32)
#define SWM_QUIRK_LEN		(64)
#define SWM_MAX_FONT_STEPS	(3)	/* For SWM_Q_XTERM_FONTADJ */

/* For ws_win, swm_region and swm_bar. */
#define WINID(w)		((w) ? (w)->id : XCB_WINDOW_NONE)
#define X(r)			((r)->g.x)
#define Y(r)			((r)->g.y)
#define WIDTH(r)		((r)->g.w)
#define HEIGHT(r)		((r)->g.h)
#define ROTATION(rr)		((rr)->g.r)
#define MAX_X(r)		(X(r) + WIDTH(r))
#define MAX_Y(r)		(Y(r) + HEIGHT(r))

#define SH_POS(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_POSITION)
#define SH_UPOS(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
#define SH_MIN(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
#define SH_MIN_W(w)		((w)->sh.min_width)
#define SH_MIN_H(w)		((w)->sh.min_height)
#define SH_MAX(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
#define SH_MAX_W(w)		((w)->sh.max_width)
#define SH_MAX_H(w)		((w)->sh.max_height)
#define SH_INC(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC)
#define SH_INC_W(w)		((w)->sh.width_inc)
#define SH_INC_H(w)		((w)->sh.height_inc)
#define SH_GRAVITY(w)		((w)->sh.flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY)
#define MAXIMIZED_VERT(w)	((w)->ewmh_flags & EWMH_F_MAXIMIZED_VERT)
#define MAXIMIZED_HORZ(w)	((w)->ewmh_flags & EWMH_F_MAXIMIZED_HORZ)
#define SKIP_TASKBAR(w)		((w)->ewmh_flags & EWMH_F_SKIP_TASKBAR)
#define SKIP_PAGER(w)		((w)->ewmh_flags & EWMH_F_SKIP_PAGER)
#define HIDDEN(w)		((w)->ewmh_flags & EWMH_F_HIDDEN)
#define FULLSCREEN(w)		((w)->ewmh_flags & EWMH_F_FULLSCREEN)
#define ABOVE(w)		((w)->ewmh_flags & EWMH_F_ABOVE)
#define BELOW(w)		((w)->ewmh_flags & EWMH_F_BELOW)
#define DEMANDS_ATTENTION(w)	((w)->ewmh_flags & EWMH_F_DEMANDS_ATTENTION)
#define MANUAL(w)		((w)->ewmh_flags & SWM_F_MANUAL)
#define MAXIMIZED(w)		((w)->ewmh_flags & EWMH_F_MAXIMIZED)

/* Constrain Window flags */
#define SWM_CW_RESIZABLE	(0x01)
#define SWM_CW_SOFTBOUNDARY	(0x02)
#define SWM_CW_HARDBOUNDARY	(0x04)
#define SWM_CW_RIGHT		(0x10)
#define SWM_CW_LEFT		(0x20)
#define SWM_CW_BOTTOM		(0x40)
#define SWM_CW_TOP		(0x80)
#define SWM_CW_ALLSIDES		(0xf0)

#define SWM_FOCUS_TYPE_STARTUP		(1 << 0)
#define SWM_FOCUS_TYPE_BORDER		(1 << 1)
#define SWM_FOCUS_TYPE_LAYOUT		(1 << 2)
#define SWM_FOCUS_TYPE_MAP		(1 << 3)
#define SWM_FOCUS_TYPE_UNMAP		(1 << 4)
#define SWM_FOCUS_TYPE_ICONIFY		(1 << 5)
#define SWM_FOCUS_TYPE_UNICONIFY	(1 << 6)
#define SWM_FOCUS_TYPE_CONFIGURE	(1 << 7)
#define SWM_FOCUS_TYPE_MOVE		(1 << 8)
#define SWM_FOCUS_TYPE_WORKSPACE	(1 << 9)
#define SWM_FOCUS_TYPE_ALL		((1 << 10) - 1)

#define SWM_FOCUS_MODE_DEFAULT	(SWM_FOCUS_TYPE_BORDER)
#define SWM_FOCUS_MODE_FOLLOW	(SWM_FOCUS_TYPE_ALL)
#define SWM_FOCUS_MODE_MANUAL	(0)

#define SWM_COUNT_TILED		(1 << 0)
#define SWM_COUNT_FLOATING	(1 << 1)
#define SWM_COUNT_ICONIC	(1 << 2)
#define SWM_COUNT_DESKTOP	(1 << 3)
#define SWM_COUNT_NORMAL	(SWM_COUNT_TILED | SWM_COUNT_FLOATING |	\
    SWM_COUNT_DESKTOP)
#define SWM_COUNT_ALL		(SWM_COUNT_NORMAL | SWM_COUNT_ICONIC)

#define SWM_WIN_UNFOCUS		(1 << 0)
#define SWM_WIN_NOUNMAP		(1 << 1)

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
#define SWM_CONF_DELIMLIST	","
#define SWM_CONF_WHITESPACE	" \t\n"

#ifndef SWM_LIB
#define SWM_LIB			"/usr/local/lib/libswmhack.so"
#endif

char			**start_argv;
xcb_atom_t		a_state;
xcb_atom_t		a_change_state;
xcb_atom_t		a_prot;
xcb_atom_t		a_delete;
xcb_atom_t		a_net_frame_extents;
xcb_atom_t		a_net_wm_check;
xcb_atom_t		a_net_wm_pid;
xcb_atom_t		a_net_supported;
xcb_atom_t		a_takefocus;
xcb_atom_t		a_utf8_string;
xcb_atom_t		a_swm_pid;
xcb_atom_t		a_swm_ws;
volatile sig_atomic_t   running = 1;
volatile sig_atomic_t   restart_wm = 0;
xcb_timestamp_t		event_time = 0;
int			outputs = 0;
xcb_window_t		pointer_window = XCB_WINDOW_NONE;
bool			randr_support = false;
bool			randr_scan = false;
int			randr_eventbase;
unsigned int		numlockmask = 0;
bool			xinput2_support = false;
int			xinput2_opcode;
bool			xinput2_raw = false;

Display			*display;
xcb_connection_t	*conn;
xcb_key_symbols_t	*syms;

int			boundary_width = 50;
int			snap_range = 25;
bool			cycle_empty = false;
bool			cycle_visible = false;
int			term_width = 0;
int			font_adjusted = 0;
uint16_t		mod_key = MODKEY;
xcb_keysym_t		cancel_key = CANCELKEY;
xcb_keycode_t		cancel_keycode = XCB_NO_SYMBOL;
bool			warp_focus = false;
bool			warp_pointer = false;
bool			workspace_autorotate = false;
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
#define SWM_STACK_PRIOR		(4)

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
#define SWM_BAR_FONTS_FALLBACK	"-*-fixed-*-r-*-*-*-*-*-*-*-*-*-*,"		\
				"-*-*-*-*-*-r-*-*-*-*-*-*-*-*,"			\
				"-*-*-*-*-*-*-*-*-*-*-*-*-*-*"

#ifdef X_HAVE_UTF8_STRING
#define DRAWSTRING(x...)	Xutf8DrawString(x)
#define TEXTEXTENTS(x...)	Xutf8TextExtents(x)
#else
#define DRAWSTRING(x...)	XmbDrawString(x)
#define TEXTEXTENTS(x...)	XmbTextExtents(x)
#endif

enum {
	SWM_UNFOCUS_NONE,
	SWM_UNFOCUS_RESTORE,
	SWM_UNFOCUS_ICONIFY,
	SWM_UNFOCUS_FLOAT,
	SWM_UNFOCUS_BELOW,
	SWM_UNFOCUS_QUICK_BELOW,
};

char		*bar_argv[] = { NULL, NULL };
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
int		 bar_workspace_limit = 0;
bool		 stack_enabled = true;
bool		 clock_enabled = true;
bool		 iconic_enabled = false;
int		 fullscreen_unfocus = SWM_UNFOCUS_NONE;
bool		 fullscreen_hide_other = false;
int		 maximized_unfocus = SWM_UNFOCUS_RESTORE;
bool		 maximize_hide_bar = false;
bool		 maximize_hide_other = false;
bool		 max_layout_maximize = true;
bool		 urgent_enabled = false;
bool		 urgent_collapse = false;
char		*clock_format = NULL;
bool		 window_class_enabled = false;
bool		 window_instance_enabled = false;
bool		 window_name_enabled = false;
bool		 click_to_raise = true;
uint32_t	 workspace_indicator = SWM_WSI_DEFAULT;
unsigned int	 focus_mode = SWM_FOCUS_MODE_DEFAULT;
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
bool		 debug_enabled;
time_t		 time_started;
pid_t		 bar_pid;
XFontSet	 bar_fs = NULL;
XFontSetExtents	*bar_fs_extents;
char		**bar_fontnames = NULL;
int		 num_xftfonts = 0;
char		*bar_fontname_pua = NULL;
int		 font_pua_index = 0;
bool		 bar_font_legacy = true;
char		*bar_fonts = NULL;
XftColor	search_font_color;
char		*startup_exception = NULL;
unsigned int	 nr_exceptions = 0;
char		*workspace_mark_current = NULL;
char		*workspace_mark_current_suffix = NULL;
char		*workspace_mark_urgent = NULL;
char		*workspace_mark_urgent_suffix = NULL;
char		*workspace_mark_active = NULL;
char		*workspace_mark_active_suffix = NULL;
char		*workspace_mark_empty = NULL;
char		*workspace_mark_empty_suffix = NULL;
char		*focus_mark_none = NULL;
char		*focus_mark_normal = NULL;
char		*focus_mark_floating = NULL;
char		*focus_mark_free = NULL;
char		*focus_mark_maximized = NULL;
char		*stack_mark_floating = NULL;
char		*stack_mark_max = NULL;
char		*stack_mark_vertical = NULL;
char		*stack_mark_vertical_flip = NULL;
char		*stack_mark_horizontal = NULL;
char		*stack_mark_horizontal_flip = NULL;
size_t		 stack_mark_maxlen = 1;	/* Start with null byte. */

#define ROTATION_DEFAULT	(XCB_RANDR_ROTATION_ROTATE_0)
#define ROTATION_VERT		(XCB_RANDR_ROTATION_ROTATE_0 |		       \
    XCB_RANDR_ROTATION_ROTATE_180)

/* layout manager data */
struct swm_geometry {
	int16_t			x;
	int16_t			y;
	uint16_t		w;
	uint16_t		h;
	uint16_t		r;	/* RandR rotation. */
};

struct swm_screen;
struct workspace;

struct swm_stackable {
	SLIST_ENTRY(swm_stackable)	entry;
	enum stackable {
		STACKABLE_WIN,
		STACKABLE_BAR,
		STACKABLE_REGION,
		STACKABLE_INVALID
	}				type;
	union {
		struct ws_win		*win;
		struct swm_bar		*bar;
		struct swm_region	*region;
	};
	enum swm_layer {
		SWM_LAYER_REGION,
		SWM_LAYER_DESKTOP,
		SWM_LAYER_BELOW,
		SWM_LAYER_TILED,
		SWM_LAYER_DOCK,
		SWM_LAYER_BAR,
		SWM_LAYER_ABOVE,
		SWM_LAYER_MAXIMIZED,
		SWM_LAYER_FULLSCREEN,
		SWM_LAYER_RAISED,
		SWM_LAYER_INVALID
	}				layer;
	struct swm_screen		*s; /* always valid, never changes */
};
SLIST_HEAD(swm_stack_list, swm_stackable);

struct swm_bar {
	struct swm_stackable	*st;	/* Always valid, never changes. */
	xcb_window_t		id;
	struct swm_geometry	g;
	struct swm_region	*r;	/* Associated region. */
	bool			disabled;
	xcb_pixmap_t		buffer;
};

/* virtual "screens" */
struct swm_region {
	TAILQ_ENTRY(swm_region)	entry;
	struct swm_stackable	*st;	/* Always valid, never changes. */
	xcb_window_t		id;
	struct swm_geometry	g;
	struct swm_geometry	g_usable;
	struct workspace	*ws;	/* current workspace on this region */
	struct workspace	*ws_prior; /* prior workspace on this region */
	struct swm_screen	*s;	/* screen idx */
	struct swm_bar		*bar;
};
TAILQ_HEAD(swm_region_list, swm_region);

struct swm_strut {
	SLIST_ENTRY(swm_strut)	entry;
	struct ws_win		*win;
	/* _NET_WM_STRUT_PARTIAL: CARDINAL[12]/32 */
	uint32_t		left;
	uint32_t		right;
	uint32_t		top;
	uint32_t		bottom;
	uint32_t		left_start_y;
	uint32_t		left_end_y;
	uint32_t		right_start_y;
	uint32_t		right_end_y;
	uint32_t		top_start_x;
	uint32_t		top_end_x;
	uint32_t		bottom_start_x;
	uint32_t		bottom_end_x;
};
SLIST_HEAD(swm_strut_list, swm_strut);

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	TAILQ_ENTRY(ws_win)	manage_entry;
	TAILQ_ENTRY(ws_win)	focus_entry;
	TAILQ_ENTRY(ws_win)	priority_entry;
	struct swm_stackable	*st;	/* Always valid, never changes */
	xcb_window_t		id;
	xcb_window_t		frame;
	xcb_window_t		transient_for;	/* WM_TRANSIENT_FOR (WINDOW). */
	xcb_visualid_t		visual;
	struct ws_win		*main;		/* Always valid. */
	struct ws_win		*parent;	/* WM_TRANSIENT_FOR ws_win. */
	struct ws_win		*focus_redirect;/* focus on transient */
	struct swm_geometry	g;		/* current geometry */
	struct swm_geometry	g_grav;		/* win-gravity reference. */
	struct swm_geometry	g_float;	/* root coordinates */
	struct swm_geometry	g_floatref;	/* reference coordinates */
	bool			g_floatref_root;
	bool			g_float_xy_valid;
	uint8_t			gravity;
	bool			mapped;
	uint32_t		mapping;	/* # of pending operations */
	uint32_t		unmapping;	/* # of pending operations */
	uint32_t		state;		/* current ICCCM WM_STATE */
	bool			normalmax;
	bool			maxstackmax;
	bool			bordered;
	uint32_t		type;		/* _NET_WM_WINDOW_TYPE */
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
	struct swm_strut	*strut;
	xcb_window_t		debug;	/* Debug overlay window. */
};
TAILQ_HEAD(ws_win_list, ws_win);
TAILQ_HEAD(ws_win_focus, ws_win);
TAILQ_HEAD(ws_win_managed, ws_win);
TAILQ_HEAD(ws_win_priority, ws_win);

/* pid goo */
struct pid_e {
	TAILQ_ENTRY(pid_e)	entry;
	pid_t			pid;
	int			ws;
};
TAILQ_HEAD(pid_list, pid_e) pidlist = TAILQ_HEAD_INITIALIZER(pidlist);

/* layout handlers */
static void	stack(struct swm_region *);
static void	vertical_config(struct workspace *, int);
static void	vertical_stack(struct workspace *, struct swm_geometry *);
static void	horizontal_config(struct workspace *, int);
static void	horizontal_stack(struct workspace *, struct swm_geometry *);
static void	max_config(struct workspace *, int);
static void	max_stack(struct workspace *, struct swm_geometry *);
static void	floating_stack(struct workspace *, struct swm_geometry *);
static void	plain_stacker(struct workspace *);
static void	fancy_stacker(struct workspace *);

enum {
	SWM_V_STACK,
	SWM_H_STACK,
	SWM_MAX_STACK,
	SWM_FLOATING_STACK,
	SWM_STACK_COUNT
};

struct layout {
	char		*name;
	void		(*l_stack)(struct workspace *, struct swm_geometry *);
	void		(*l_config)(struct workspace *, int);
	uint32_t	flags;
#define SWM_L_FOCUSPREV		(1 << 0)
#define SWM_L_MAPONFOCUS	(1 << 1)
#define SWM_L_NOTILE		(1 << 2)
	void		(*l_string)(struct workspace *);
} layouts[SWM_STACK_COUNT] = {
	{ "vertical",	vertical_stack,		vertical_config,
	    0,						plain_stacker },
	{ "horizontal",	horizontal_stack,	horizontal_config,
	    0,						plain_stacker },
	{ "max",	max_stack,		max_config,
	    SWM_L_MAPONFOCUS | SWM_L_FOCUSPREV,		plain_stacker },
	{ "floating",	floating_stack,		NULL,
	    SWM_L_NOTILE,				plain_stacker },
};

struct layout		*layout_order[SWM_STACK_COUNT];
int			 layout_order_count = 0;

#define SWM_H_SLICE		(32)
#define SWM_V_SLICE		(32)

#define SWM_FANCY_MAXLEN	(8)		/* Includes null byte. */

/* define work spaces */
struct workspace {
	RB_ENTRY(workspace)	entry;
	int			idx;		/* workspace index */
	char			*name;		/* workspace name */
	bool			always_raise;	/* raise windows on focus */
	bool			bar_enabled;	/* bar visibility */
	struct layout		*cur_layout;	/* current layout handlers */
	struct layout		*prev_layout;	/* may be NULL */
	struct ws_win		*focus;		/* may be NULL */
	struct ws_win		*focus_raise;	/* may be NULL */
	struct swm_screen	*s;	/* Always valid, never changes. */
	struct swm_region	*r;		/* may be NULL */
	struct swm_region	*old_r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */
	char			*stacker;	/* stack_mark buffer */
	size_t			stacker_len;
	uint16_t		rotation;	/* Layout reference. */

	/* stacker state */
	struct {
		int		horizontal_msize;
		int		horizontal_mwin;
		int		horizontal_stacks;
		bool		horizontal_flip;
		int		vertical_msize;
		int		vertical_mwin;
		int		vertical_stacks;
		bool		vertical_flip;
	} l_state;
};
RB_HEAD(workspace_tree, workspace);

enum {
	SWM_S_COLOR_BAR,
	SWM_S_COLOR_BAR_UNFOCUS,
	SWM_S_COLOR_BAR_FREE,
	SWM_S_COLOR_BAR_SELECTED,
	SWM_S_COLOR_BAR_BORDER,
	SWM_S_COLOR_BAR_BORDER_UNFOCUS,
	SWM_S_COLOR_BAR_BORDER_FREE,
	SWM_S_COLOR_BAR_FONT,
	SWM_S_COLOR_BAR_FONT_UNFOCUS,
	SWM_S_COLOR_BAR_FONT_FREE,
	SWM_S_COLOR_BAR_FONT_SELECTED,
	SWM_S_COLOR_FOCUS,
	SWM_S_COLOR_FOCUS_MAXIMIZED,
	SWM_S_COLOR_UNFOCUS,
	SWM_S_COLOR_UNFOCUS_MAXIMIZED,
	SWM_S_COLOR_URGENT,
	SWM_S_COLOR_URGENT_MAXIMIZED,
	SWM_S_COLOR_FOCUS_FREE,
	SWM_S_COLOR_FOCUS_MAXIMIZED_FREE,
	SWM_S_COLOR_UNFOCUS_FREE,
	SWM_S_COLOR_UNFOCUS_MAXIMIZED_FREE,
	SWM_S_COLOR_URGENT_FREE,
	SWM_S_COLOR_URGENT_MAXIMIZED_FREE,
	SWM_S_COLOR_MAX
};

/* physical screen mapping */
#define SWM_WS_MAX		(22)	/* hard limit */
int		workspace_limit = 10;	/* soft limit */

#define SWM_RATE_DEFAULT	(60)	/* Default for swm_screen. */

struct swm_color {
	uint16_t	r;
	uint16_t	g;
	uint16_t	b;
	uint16_t	a;
	uint16_t	r_orig;
	uint16_t	g_orig;
	uint16_t	b_orig;
	uint32_t	pixel;
	XftColor	xft_color;
	bool		manual;
};

struct swm_screen {
	int			idx;	/* screen index */
	xcb_window_t		root;

	struct swm_region	*r;	/* Root region is always valid. */
	struct swm_region_list	rl;	/* Additional regions on this screen. */
	struct swm_region_list	orl;	/* Old/unused regions on this screen. */

	struct swm_region	*r_focus;	/* Current active region. */
	xcb_window_t		active_window; /* current _NET_ACTIVE_WINDOW */
	xcb_window_t		swmwin;	/* ewmh wm check/default input */

	struct workspace_tree	workspaces;	/* Dynamic workspaces. */
	struct ws_win_priority	priority;	/* Window floating priority. */
	struct swm_stack_list	stack;		/* Current stacking order. */

	struct ws_win		*focus;	/* Currently focused window. */
	struct ws_win_focus	fl;	/* Previous focus queue. */
	struct ws_win_managed	managed;	/* All client windows. */
	int			managed_count;
	struct swm_strut_list	struts;

	struct swm_color_type {
		struct swm_color	**colors;
		int			count;
	} c[SWM_S_COLOR_MAX];

	uint8_t			depth;
	xcb_timestamp_t		rate; /* Max updates/sec for move and resize */
	xcb_visualid_t		visual;
	Visual			*xvisual; /* Needed for Xft. */
	xcb_colormap_t		colormap;
	xcb_gcontext_t		gc;
	XftFont			**bar_xftfonts;
};
struct swm_screen	*screens;

/* args to functions */
union arg {
	int			id;
#define SWM_ARG_ID_FOCUSNEXT	(0)
#define SWM_ARG_ID_FOCUSPREV	(1)
#define SWM_ARG_ID_FOCUSMAIN	(2)
#define SWM_ARG_ID_FOCUSURGENT	(3)
#define SWM_ARG_ID_FOCUSPRIOR	(4)
#define SWM_ARG_ID_FOCUSFREE	(5)
#define SWM_ARG_ID_SWAPNEXT	(10)
#define SWM_ARG_ID_SWAPPREV	(11)
#define SWM_ARG_ID_SWAPMAIN	(12)
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
#define SWM_ARG_ID_PRIOR_LAYOUT	(64)
#define SWM_ARG_ID_LAYOUT_FLOATING	(65)
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

#define SWM_ASOP_BASIC		(1 << 0)
#define SWM_ASOP_ADD		(1 << 1)
#define SWM_ASOP_SUBTRACT	(1 << 2)

/* quirks */
struct quirk {
	TAILQ_ENTRY(quirk)	entry;
	char			*class;		/* WM_CLASS:class */
	char			*instance;	/* WM_CLASS:instance */
	char			*name;		/* WM_NAME */
	regex_t			regex_class;
	regex_t			regex_instance;
	regex_t			regex_name;
	uint32_t		type;
	uint8_t			mode;	 /* Assignment mode. */
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
#define SWM_Q_MAXIMIZE		(1 << 13)/* Maximize window when mapped. */
#define SWM_Q_BELOW		(1 << 14)/* Put window below when mapped. */
#define SWM_Q_ICONIFY		(1 << 15)/* Put window below when mapped. */
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
	_NET_WM_MOVERESIZE,
	_NET_WM_NAME,
	_NET_WM_STATE,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_BELOW,
	_NET_WM_STATE_DEMANDS_ATTENTION,
	_NET_WM_STRUT,
	_NET_WM_STRUT_PARTIAL,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_MENU,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	_NET_WM_WINDOW_TYPE_POPUP_MENU,
	_NET_WM_WINDOW_TYPE_TOOLTIP,
	_NET_WM_WINDOW_TYPE_NOTIFICATION,
	_NET_WM_WINDOW_TYPE_COMBO,
	_NET_WM_WINDOW_TYPE_DND,
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WORKAREA,
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
    {"_NET_WM_MOVERESIZE", XCB_ATOM_NONE},
    {"_NET_WM_NAME", XCB_ATOM_NONE},
    {"_NET_WM_STATE", XCB_ATOM_NONE},
    {"_NET_WM_STATE_MAXIMIZED_VERT", XCB_ATOM_NONE},
    {"_NET_WM_STATE_MAXIMIZED_HORZ", XCB_ATOM_NONE},
    {"_NET_WM_STATE_SKIP_TASKBAR", XCB_ATOM_NONE},
    {"_NET_WM_STATE_SKIP_PAGER", XCB_ATOM_NONE},
    {"_NET_WM_STATE_HIDDEN", XCB_ATOM_NONE},
    {"_NET_WM_STATE_FULLSCREEN", XCB_ATOM_NONE},
    {"_NET_WM_STATE_ABOVE", XCB_ATOM_NONE},
    {"_NET_WM_STATE_BELOW", XCB_ATOM_NONE},
    {"_NET_WM_STATE_DEMANDS_ATTENTION", XCB_ATOM_NONE},
    {"_NET_WM_STRUT", XCB_ATOM_NONE},
    {"_NET_WM_STRUT_PARTIAL", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DESKTOP", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DOCK", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_TOOLBAR", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_MENU", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_UTILITY", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_SPLASH", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DIALOG", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_POPUP_MENU", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_TOOLTIP", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_NOTIFICATION", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_COMBO", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DND", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_NORMAL", XCB_ATOM_NONE},
    {"_NET_WORKAREA", XCB_ATOM_NONE},
    {"_SWM_WM_STATE_MANUAL", XCB_ATOM_NONE},
};

struct ewmh_window_type {
	char		*name;
	int		id;
	uint32_t	flag;
} ewmh_window_types[EWMH_WINDOW_TYPE_COUNT] = {
	{"DESKTOP", _NET_WM_WINDOW_TYPE_DESKTOP, EWMH_WINDOW_TYPE_DESKTOP},
	{"DOCK", _NET_WM_WINDOW_TYPE_DOCK, EWMH_WINDOW_TYPE_DOCK},
	{"TOOLBAR", _NET_WM_WINDOW_TYPE_TOOLBAR, EWMH_WINDOW_TYPE_TOOLBAR},
	{"MENU", _NET_WM_WINDOW_TYPE_MENU, EWMH_WINDOW_TYPE_MENU},
	{"UTILITY", _NET_WM_WINDOW_TYPE_UTILITY, EWMH_WINDOW_TYPE_UTILITY},
	{"SPLASH", _NET_WM_WINDOW_TYPE_SPLASH, EWMH_WINDOW_TYPE_SPLASH},
	{"DIALOG", _NET_WM_WINDOW_TYPE_DIALOG, EWMH_WINDOW_TYPE_DIALOG},
	{"DROPDOWN_MENU", _NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	    EWMH_WINDOW_TYPE_DROPDOWN_MENU},
	{"POPUP_MENU", _NET_WM_WINDOW_TYPE_POPUP_MENU,
	    EWMH_WINDOW_TYPE_POPUP_MENU},
	{"TOOLTIP", _NET_WM_WINDOW_TYPE_TOOLTIP, EWMH_WINDOW_TYPE_TOOLTIP},
	{"NOTIFICATION", _NET_WM_WINDOW_TYPE_NOTIFICATION,
	    EWMH_WINDOW_TYPE_NOTIFICATION},
	{"COMBO", _NET_WM_WINDOW_TYPE_COMBO, EWMH_WINDOW_TYPE_COMBO},
	{"DND", _NET_WM_WINDOW_TYPE_DND, EWMH_WINDOW_TYPE_DND},
	{"NORMAL", _NET_WM_WINDOW_TYPE_NORMAL, EWMH_WINDOW_TYPE_NORMAL},
};

/* EWMH source type */
enum {
	EWMH_SOURCE_TYPE_NONE = 0,
	EWMH_SOURCE_TYPE_NORMAL = 1,
	EWMH_SOURCE_TYPE_OTHER = 2,
};

enum {
	EWMH_WM_MOVERESIZE_SIZE_TOPLEFT = 0,
	EWMH_WM_MOVERESIZE_SIZE_TOP = 1,
	EWMH_WM_MOVERESIZE_SIZE_TOPRIGHT = 2,
	EWMH_WM_MOVERESIZE_SIZE_RIGHT = 3,
	EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT = 4,
	EWMH_WM_MOVERESIZE_SIZE_BOTTOM = 5,
	EWMH_WM_MOVERESIZE_SIZE_BOTTOMLEFT = 6,
	EWMH_WM_MOVERESIZE_SIZE_LEFT = 7,
	EWMH_WM_MOVERESIZE_MOVE = 8,
	EWMH_WM_MOVERESIZE_SIZE_KEYBOARD = 9,
	EWMH_WM_MOVERESIZE_MOVE_KEYBOARD = 10,
	EWMH_WM_MOVERESIZE_CANCEL = 11,
};

#define SWM_SIZE_VFLIP		(0x1)
#define SWM_SIZE_HFLIP		(0x2)
#define SWM_SIZE_VERT		(0x4)
#define SWM_SIZE_HORZ		(0x8)

#define SWM_SIZE_TOP		(SWM_SIZE_VERT | SWM_SIZE_VFLIP)
#define SWM_SIZE_BOTTOM		(SWM_SIZE_VERT)
#define SWM_SIZE_RIGHT		(SWM_SIZE_HORZ)
#define SWM_SIZE_LEFT		(SWM_SIZE_HORZ | SWM_SIZE_HFLIP)
#define SWM_SIZE_TOPLEFT	(SWM_SIZE_TOP | SWM_SIZE_LEFT)
#define SWM_SIZE_TOPRIGHT	(SWM_SIZE_TOP | SWM_SIZE_RIGHT)
#define SWM_SIZE_BOTTOMLEFT	(SWM_SIZE_BOTTOM | SWM_SIZE_LEFT)
#define SWM_SIZE_BOTTOMRIGHT	(SWM_SIZE_BOTTOM | SWM_SIZE_RIGHT)

/* Cursors */
enum {
	XC_FLEUR,
	XC_BOTTOM_LEFT_CORNER,
	XC_BOTTOM_RIGHT_CORNER,
	XC_BOTTOM_SIDE,
	XC_LEFT_PTR,
	XC_LEFT_SIDE,
	XC_RIGHT_SIDE,
	XC_SIZING,
	XC_TOP_LEFT_CORNER,
	XC_TOP_RIGHT_CORNER,
	XC_TOP_SIDE,
	XC_MAX
};

struct cursors {
	char		*name; /* Name used by Xcursor .*/
	uint8_t		cf_char; /* cursorfont index. */
	xcb_cursor_t	cid;
} cursors[XC_MAX] =	{
	{"fleur", XC_fleur, XCB_CURSOR_NONE},
	{"bottom_left_corner", XC_bottom_left_corner, XCB_CURSOR_NONE},
	{"bottom_right_corner", XC_bottom_right_corner, XCB_CURSOR_NONE},
	{"bottom_side", XC_bottom_side, XCB_CURSOR_NONE},
	{"left_ptr", XC_left_ptr, XCB_CURSOR_NONE},
	{"left_side", XC_left_side, XCB_CURSOR_NONE},
	{"right_side", XC_right_side, XCB_CURSOR_NONE},
	{"sizing", XC_sizing, XCB_CURSOR_NONE},
	{"top_left_corner", XC_top_left_corner, XCB_CURSOR_NONE},
	{"top_right_corner", XC_top_right_corner, XCB_CURSOR_NONE},
	{"top_side", XC_top_side, XCB_CURSOR_NONE},
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

/* spawn */
#define SWM_SPAWN_OPTIONAL		(1 << 0)
#define SWM_SPAWN_CLOSE_FD		(1 << 1)
#define SWM_SPAWN_XTERM_FONTADJ		(1 << 2)
#define SWM_SPAWN_WS			(1 << 3)
#define SWM_SPAWN_PID			(1 << 4)
#define SWM_SPAWN_NOSPAWNWS		(1 << 5)

unsigned int		spawn_flags = 0;

struct spawn_prog {
	TAILQ_ENTRY(spawn_prog)	entry;
	char			*name;
	int			argc;
	char			**argv;
	unsigned int		flags;
};
TAILQ_HEAD(spawn_list, spawn_prog) spawns = TAILQ_HEAD_INITIALIZER(spawns);

/* Action callback flags. */
#define FN_F_NOREPLAY	(0x1)

/* User callable function IDs. */
enum actionid {
	FN_FOCUS_FREE,
	FN_FREE_TOGGLE,
	FN_BAR_TOGGLE,
	FN_BAR_TOGGLE_WS,
	FN_BUTTON2,
	FN_CYCLE_LAYOUT,
	FN_FLIP_LAYOUT,
	FN_FLOAT_TOGGLE,
	FN_BELOW_TOGGLE,
	FN_FOCUS,
	FN_FOCUS_MAIN,
	FN_FOCUS_NEXT,
	FN_FOCUS_PREV,
	FN_FOCUS_PRIOR,
	FN_FOCUS_URGENT,
	FN_FULLSCREEN_TOGGLE,
	FN_MAXIMIZE_TOGGLE,
	FN_HEIGHT_GROW,
	FN_HEIGHT_SHRINK,
	FN_ICONIFY,
	FN_LAYOUT_VERTICAL,
	FN_LAYOUT_HORIZONTAL,
	FN_LAYOUT_MAX,
	FN_LAYOUT_FLOATING,
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
	FN_PRIOR_LAYOUT,
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
	FN_DEBUG_TOGGLE,
	FN_DUMPWINS,
	/* ALWAYS last: */
	FN_INVALID
};

enum binding_type {
	KEYBIND,
	BTNBIND
};

#define BINDING_F_REPLAY	(0x1)

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

struct atom_name {
	RB_ENTRY(atom_name)	entry;
	xcb_atom_t		atom;
	char			*name;
};
RB_HEAD(atom_name_tree, atom_name) atom_names = RB_INITIALIZER(&atom_names);

/* function prototypes */
static void	 adjust_font(struct ws_win *);
static void	 apply_struts(struct swm_screen *, struct swm_geometry *);
static int	 apply_unfocus(struct workspace *, struct ws_win *);
static char	*argsep(char **);
static int	 asopcheck(uint8_t, uint8_t, char **);
static int	 atom_name_cmp(struct atom_name *, struct atom_name *);
static void	 atom_name_insert(xcb_atom_t, char *);
static struct atom_name	*atom_name_lookup(xcb_atom_t);
static void	 atom_name_remove(struct atom_name *);
static void	 bar_cleanup(struct swm_region *);
static void	 bar_draw(struct swm_bar *);
static void	 bar_extra_setup(void);
static void	 bar_extra_stop(void);
static int	 bar_extra_update(void);
static void	 bar_fmt(const char *, char *, struct swm_region *, size_t);
static void	 bar_fmt_expand(char *, size_t);
static void	 bar_parse_markup(struct swm_screen *s, struct bar_section *);
static void	 bar_print(struct swm_region *, const char *);
static void	 bar_print_layout(struct swm_region *);
static void	 bar_print_legacy(struct swm_region *, const char *);
static void	 bar_split_format(char *);
static void	 bar_strlcat_esc(char *, char *, size_t, size_t *);
static void	 bar_replace(char *, char *, struct swm_region *, size_t);
static void	 bar_replace_action(char *, char *, struct swm_region *,
		     size_t);
static void	 bar_replace_pad(char *, size_t *, size_t);
static char	*bar_replace_seq(char *, char *, struct swm_region *, size_t *,
		     size_t);
static void	 bar_setup(struct swm_region *);
static void	 bar_toggle(struct swm_screen *, struct binding *, union arg *);
static void	 bar_urgent(struct swm_screen *, char *, size_t);
static void	 bar_window_class(char *, size_t, struct ws_win *, size_t *);
static void	 bar_window_class_instance(char *, size_t, struct ws_win *,
		     size_t *);
static void	 bar_window_instance(char *, size_t, struct ws_win *, size_t *);
static void	 bar_window_name(char *, size_t, struct ws_win *, size_t *);
static void	 bar_window_state(char *, size_t, struct ws_win *);
static void	 bar_workspace_indicator(char *, size_t, struct swm_region *);
static void	 bar_workspace_name(char *, size_t, struct workspace *,
		     size_t *);
static void	 below_toggle(struct swm_screen *, struct binding *,
		     union arg *);
static int	 binding_cmp(struct binding *, struct binding *);
static void	 binding_insert(uint16_t, enum binding_type, uint32_t,
		     enum actionid, uint32_t, const char *);
static struct binding	*binding_lookup(uint16_t, enum binding_type, uint32_t);
static void	 binding_remove(struct binding *);
static bool	 bounds_intersect(struct swm_geometry *, struct swm_geometry *);
static bool	 button_has_binding(uint32_t);
static void	 buttonpress(xcb_button_press_event_t *);
static void	 buttonrelease(xcb_button_release_event_t *);
static void	 center_pointer(struct swm_region *);
static char	*cleanopt(char *);
static void	 clear_atom_names(void);
static void	 clear_attention(struct ws_win *);
static void	 clear_bindings(void);
static void	 clear_keybindings(void);
static void	 clear_quirks(void);
static void	 clear_spawns(void);
static void	 clear_stack(struct swm_screen *);
static void	 click_focus(struct swm_screen *, xcb_window_t, int, int);
static void	 client_msg(struct ws_win *, xcb_atom_t, xcb_timestamp_t);
static void	 clientmessage(xcb_client_message_event_t *);
static char	*color_to_rgb(struct swm_color *);
static int	 conf_load(const char *, int);
static void	 config_win(struct ws_win *, xcb_configure_request_event_t *);
static void	 configurenotify(xcb_configure_notify_event_t *);
static void	 configurerequest(xcb_configure_request_event_t *);
static void	 constrain_window(struct ws_win *, struct swm_geometry *,
		     uint32_t *);
static bool	 contain_window(struct ws_win *, struct swm_geometry, int,
		     uint32_t);
static int	 count_win(struct workspace *, uint32_t);
static int	 create_search_win(struct ws_win *, int);
static void	 cursors_cleanup(void);
static void	 cursors_load(void);
static void	 cyclerg(struct swm_screen *, struct binding *, union arg *);
static void	 cyclews(struct swm_screen *, struct binding *, union arg *);
static void	 debug_refresh(struct ws_win *);
static void	 debug_toggle(struct swm_screen *, struct binding *,
		     union arg *);
static void	 destroynotify(xcb_destroy_notify_event_t *);
static void	 draw_frame(struct ws_win *);
static void	 dumpwins(struct swm_screen *, struct binding *, union arg *);
static void	 emptyws(struct swm_screen *, struct binding *, union arg *);
static int	 enable_wm(void);
static void	 enternotify(xcb_enter_notify_event_t *);
static void	 event_error(xcb_generic_error_t *);
static void	 event_handle(xcb_generic_event_t *);
static uint32_t	 ewmh_apply_flags(struct ws_win *, uint32_t);
static uint32_t	 ewmh_change_wm_state(struct ws_win *, xcb_atom_t, long);
static void	 ewmh_get_desktop_names(struct swm_screen *);
static void	 ewmh_get_strut(struct ws_win *);
static void	 ewmh_get_window_type(struct ws_win *);
static void	 ewmh_get_wm_state(struct ws_win *);
static void	 ewmh_print_window_type(uint32_t);
static void	 ewmh_update_actions(struct ws_win *);
static void	 ewmh_update_active_window(struct swm_screen *);
static void	 ewmh_update_client_list(struct swm_screen *);
static void	 ewmh_update_current_desktop(struct swm_screen *);
static void	 ewmh_update_desktop_names(struct swm_screen *);
static void	 ewmh_update_number_of_desktops(struct swm_screen *);
static void	 ewmh_update_wm_state(struct ws_win *);
static void	 ewmh_update_workarea(struct swm_screen *);
static void	 ewmh_update_desktop_viewports(struct swm_screen *);
static char	*expand_tilde(const char *);
static void	 expose(xcb_expose_event_t *);
static void	 fake_keypress(struct ws_win *, xcb_keysym_t, uint16_t);
static struct swm_bar	*find_bar(xcb_window_t);
static struct ws_win	*find_main_window(struct ws_win *);
static struct pid_e	*find_pid(pid_t);
static struct swm_region	*find_region(xcb_window_t);
static struct swm_screen	*find_screen(xcb_window_t);
static struct ws_win	*find_window(xcb_window_t);
static void	 floating_toggle(struct swm_screen *, struct binding *,
		     union arg *);
static void	 flush(void);
static void	 focus(struct swm_screen *, struct binding *, union arg *);
static void	 focus_follow(struct swm_screen *, struct swm_region *,
		     struct ws_win *);
static void	 focus_pointer(struct swm_screen *, struct binding *,
		     union arg *);
static void	 focus_region(struct swm_region *);
static void	 focus_win(struct swm_screen *s, struct ws_win *);
static void	 focus_win_input(struct ws_win *, bool);
static void	 focus_window(xcb_window_t);
static void	 focus_window_region(xcb_window_t);
static void	 focusin(xcb_focus_in_event_t *);
static void	 focusout(xcb_focus_out_event_t *);
static void	 focusrg(struct swm_screen *, struct binding *, union arg *);
static bool	 follow_mode(unsigned int);
static bool	 follow_pointer(struct swm_screen *, unsigned int);
static int	 fontset_init(void);
static void	 freecolortype(struct swm_screen *, int);
static void	 free_stackable(struct swm_stackable *);
static void	 free_toggle(struct swm_screen *, struct binding *,
		     union arg *);
static void	 free_window(struct ws_win *);
static void	 fullscreen_toggle(struct swm_screen *, struct binding *,
		     union arg *);
static xcb_atom_t	 get_atom_from_string(const char *);
static const char	*get_atom_label(xcb_atom_t);
static char	*get_atom_name(xcb_atom_t);
static struct swm_geometry	 get_boundary(struct ws_win *);
static int	 get_character_font(struct swm_screen *, FcChar32, int);
static struct swm_region	*get_current_region(struct swm_screen *);
static struct swm_color *getcolor(struct swm_screen *, int, int);
static uint32_t	 getcolorpixel(struct swm_screen *, int, int);
static char	*getcolorrgb(struct swm_screen *, int, int);
static XftColor	*getcolorxft(struct swm_screen *, int, int);
static const char	*get_event_label(xcb_generic_event_t *);
static struct ws_win	*get_focus_magic(struct ws_win *);
static struct ws_win	*get_focus_other(struct ws_win *);
static const char	*get_gravity_label(uint8_t);
#ifdef SWM_XCB_HAS_XINPUT
static const char	*get_input_event_label(xcb_ge_generic_event_t *);
#endif
static xcb_window_t	 get_input_focus(void);
static xcb_atom_t	 get_intern_atom(const char *);
static xcb_keycode_t	 get_keysym_keycode(xcb_keysym_t);
static struct ws_win	*get_main_window(struct workspace *);
static const char	*get_mapping_notify_label(uint8_t);
static const char	*get_moveresize_direction_label(uint32_t);
static xcb_generic_event_t	*get_next_event(bool);
static const char	*get_notify_detail_label(uint8_t);
static const char	*get_notify_mode_label(uint8_t);
static struct swm_region	*get_pointer_region(struct swm_screen *);
static struct ws_win	*get_pointer_win(struct swm_screen *);
static const char	*get_randr_event_label(xcb_generic_event_t *);
static const char	*get_randr_rotation_label(int);
static struct swm_region	*get_region(struct swm_screen *, int);
static int	 get_region_index(struct swm_region *);
static xcb_screen_t	*get_screen(int);
static int	 get_screen_count(void);
static const struct xcb_setup_t	*get_setup(void);
static const char	*get_source_type_label(uint32_t);
static const char	*get_stack_mode_label(uint8_t);
static const char	*get_state_mask_label(uint16_t);
static xcb_keysym_t	 get_string_keysym(const char *);
static int32_t	 get_swm_ws(xcb_window_t);
static const char	*get_win_input_model_label(struct ws_win *);
static char	*get_win_name(xcb_window_t);
static uint32_t	 get_win_state(xcb_window_t);
static void	 get_wm_hints(struct ws_win *);
static void	 get_wm_normal_hints(struct ws_win *);
static void	 get_wm_protocols(struct ws_win *);
static const char	*get_wm_state_label(uint32_t);
static bool	 get_wm_transient_for(struct ws_win *);
static struct workspace	*get_workspace(struct swm_screen *, int);
static struct ws_win	*get_ws_focus(struct workspace *);
static struct ws_win	*get_ws_focus_prev(struct workspace *);
static int	 get_ws_id(struct ws_win *);
static void	 grab_buttons_win(xcb_window_t);
static void	 grab_windows(void);
static void	 grabbuttons(void);
static void	 grabkeys(void);
static void	 iconify(struct swm_screen *, struct binding *, union arg *);
static bool	 isxlfd(char *);
static bool	 keybindreleased(struct binding *, xcb_key_release_event_t *);
static void	 keypress(xcb_key_press_event_t *);
static void	 keyrelease(xcb_key_release_event_t *);
static bool	 keyrepeating(xcb_key_release_event_t *);
static void	 kill_bar_extra_atexit(void);
static void	 kill_refs(struct ws_win *);
static void	 layout_order_reset(void);
static void	 leavenotify(xcb_leave_notify_event_t *);
static void	 load_float_geom(struct ws_win *);
static struct ws_win	*manage_window(xcb_window_t, int, bool);
static void	 map_window(struct ws_win *);
static void	 mapnotify(xcb_map_notify_event_t *);
static void	 mappingnotify(xcb_mapping_notify_event_t *);
static void	 maprequest(xcb_map_request_event_t *);
static void	 maximize_toggle(struct swm_screen *, struct binding *,
		     union arg *);
static void	 motionnotify(xcb_motion_notify_event_t *);
static void	 move(struct swm_screen *, struct binding *, union arg *);
static void	 move_win(struct ws_win *, struct binding *, int);
static void	 move_win_pointer(struct ws_win *, struct binding *, uint32_t,
		     uint32_t);
static void	 moveresize_win(struct ws_win *, xcb_client_message_event_t *);
static void	 name_workspace(struct swm_screen *, struct binding *,
		     union arg *);
static void	 new_region(struct swm_screen *, int16_t, int16_t, uint16_t,
		     uint16_t, uint16_t);
static int	 parse_color(struct swm_screen *, const char *,
		     struct swm_color *);
static int	 parse_focus_types(const char *, uint32_t *, char **);
static int	 parse_rgb(const char *, uint16_t *, uint16_t *, uint16_t *);
static int	 parse_rgba(const char *, uint16_t *, uint16_t *, uint16_t *,
		     uint16_t *);
static int	 parse_window_type(const char *, uint32_t *, char **);
static int	 parse_spawn_flags(const char *, uint32_t *, char **);
static int	 parse_workspace_indicator(const char *, uint32_t *, char **);
static int	 parsebinding(const char *, uint16_t *, enum binding_type *,
		     uint32_t *, uint32_t *, char **);
static int	 parseconfcolor(uint8_t, const char *, const char *, int, bool,
		     char **);
static int	 parsequirks(const char *, uint32_t *, int *, char **);
static void	 pressbutton(struct swm_screen *, struct binding *,
		     union arg *);
static void	 print_stackable(struct swm_stackable *);
static void	 print_stacking(struct swm_screen *);
static void	 print_strut(struct swm_strut *);
static void	 print_win_geom(xcb_window_t);
static void	 prioritize_window(struct ws_win *);
static void	 priorws(struct swm_screen *, struct binding *, union arg *);
static void	 propertynotify(xcb_property_notify_event_t *);
static void	 put_back_event(xcb_generic_event_t *);
static void	 quirk_free(struct quirk *);
static void	 quirk_insert(const char *, const char *, const char *,
		     uint32_t, uint8_t, uint32_t, int);
static void	 quirk_remove(struct quirk *);
static void	 quirk_replace(struct quirk *, const char *, const char *,
		     const char *, uint32_t, uint8_t, uint32_t, int);
static void	 quit(struct swm_screen *, struct binding *, union arg *);
static void	 raise_focus(struct swm_screen *, struct binding *,
		     union arg *);
static void	 raise_toggle(struct swm_screen *, struct binding *,
		     union arg *);
#if defined(SWM_XCB_HAS_XINPUT) && defined(XCB_INPUT_RAW_BUTTON_PRESS)
static void	 rawbuttonpress(xcb_input_raw_button_press_event_t *);
#endif
static void	 refresh_stack(struct swm_screen *);
static int	 refresh_strut(struct swm_screen *);
static int	 regcompopt(regex_t *, const char *);
static struct swm_region	*region_under(struct swm_screen *, int, int);
static void	 regionize(struct ws_win *, int, int);
static int	 reparent_window(struct ws_win *);
static void	 reparentnotify(xcb_reparent_notify_event_t *);
static void	 resize(struct swm_screen *, struct binding *, union arg *);
static void	 resize_win(struct ws_win *, struct binding *, int);
static void	 resize_win_pointer(struct ws_win *, struct binding *, uint32_t,
	     uint32_t, uint32_t, bool);
static void	 restart(struct swm_screen *, struct binding *, union arg *);
static bool	 rg_root(struct swm_region *);
static void	 rotatews(struct workspace *, uint16_t);
static void	 scan_config(void);
static bool	 scan_markup(struct swm_screen *, char *, int *, size_t *);
static void	 scan_randr(struct swm_screen *);
static void	 screenchange(xcb_randr_screen_change_notify_event_t *);
static void	 search_do_resp(void);
static void	 search_resp_name_workspace(const char *, size_t);
static void	 search_resp_search_window(const char *);
static void	 search_resp_search_workspace(const char *);
static void	 search_resp_uniconify(const char *, size_t);
static void	 search_win(struct swm_screen *, struct binding *, union arg *);
static void	 search_win_cleanup(void);
static void	 search_workspace(struct swm_screen *, struct binding *,
		     union arg *);
static void	 send_to_rg(struct swm_screen *, struct binding *, union arg *);
static void	 send_to_rg_relative(struct swm_screen *, struct binding *,
		     union arg *);
static void	 send_to_ws(struct swm_screen *, struct binding *, union arg *);
static void	 set_attention(struct ws_win *);
static void	 set_focus(struct swm_screen *, struct ws_win *);
static void	 set_focus_prev(struct ws_win *);
static void	 set_focus_redirect(struct ws_win *);
static void	 set_input_focus(xcb_window_t, bool);
static void	 set_region(struct swm_region *);
static void	 set_win_state(struct ws_win *, uint32_t);
static int	 setautorun(uint8_t, const char *, const char *, int, char **);
static void	 setbinding(uint16_t, enum binding_type, uint32_t,
		     enum actionid, uint32_t, const char *);
static int	 setconfbinding(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfcancelkey(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfcolor(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfcolorlist(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfmodkey(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfquirk(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfregion(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfspawn(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfspawnflags(uint8_t, const char *, const char *, int,
		     char **);
static int	 setconfvalue(uint8_t, const char *, const char *, int,
		     char **);
static int	 setkeymapping(uint8_t, const char *, const char *, int,
		     char **);
static int	 setlayout(uint8_t, const char *, const char *, int, char **);
static int	 setlayoutorder(const char *, char **);
static void	 setquirk(const char *, const char *, const char *, uint32_t,
		     uint8_t, uint32_t, int);
static void	 setscreencolor(struct swm_screen *, const char *, int, int);
static void	 setspawn(const char *, const char *, unsigned int);
static void	 setup_btnbindings(void);
static void	 setup_ewmh(void);
static void	 setup_extensions(void);
static void	 setup_focus(void);
static void	 setup_fonts(void);
static void	 setup_globals(void);
static void	 setup_keybindings(void);
static void	 setup_marks(void);
static void	 setup_quirks(void);
static void	 setup_screens(void);
static void	 setup_spawn(void);
#if defined(SWM_XCB_HAS_XINPUT) && defined(XCB_INPUT_RAW_BUTTON_PRESS)
static void	 setup_xinput2(struct swm_screen *);
#endif
static void	 shutdown_cleanup(void);
static void	 sighdlr(int);
static void	 socket_setnonblock(int);
static void	 spawn(int, union arg *, unsigned int);
static void	 spawn_custom(struct swm_screen *, union arg *, const char *);
static int	 spawn_expand(struct swm_screen *, struct spawn_prog *, int,
		     char ***);
static struct spawn_prog	*spawn_find(const char *);
static void	 spawn_insert(const char *, const char *, unsigned int);
static void	 spawn_remove(struct spawn_prog *);
static void	 spawn_select(struct swm_region *, union arg *, const char *,
		     int *);
static xcb_window_t	 st_window_id(struct swm_stackable *);
static void	 stack_config(struct swm_screen *, struct binding *,
		     union arg *);
static void	 stack_master(struct workspace *, struct swm_geometry *, int,
		     bool);
static void	 store_float_geom(struct ws_win *);
static char	*strdupsafe(const char *);
static int32_t	 strtoint32(const char *, int32_t, int32_t, int *);
static void	 swapwin(struct swm_screen *, struct binding *, union arg *);
static void	 switch_workspace(struct swm_region *, struct workspace *, bool,
		     bool);
static void	 switchlayout(struct swm_screen *, struct binding *,
		     union arg *);
static void	 switchws(struct swm_screen *, struct binding *, union arg *);
static void	 teardown_ewmh(void);
static void	 transfer_win(struct ws_win *, struct workspace *);
static char	*trimopt(char *);
static void	 update_mapping(struct swm_screen *);
static void	 update_region_mapping(struct swm_region *);
static void	 update_stacking(struct swm_screen *);
static void	 unescape_selector(char *);
static char	*unescape_value(const char *);
static void	 unfocus_win(struct ws_win *);
static void	 uniconify(struct swm_screen *, struct binding *, union arg *);
static void	 unmanage_window(struct ws_win *);
static void	 unmap_window(struct ws_win *);
static void	 unmap_workspace(struct workspace *);
static void	 unmapnotify(xcb_unmap_notify_event_t *);
static void	 unparent_window(struct ws_win *);
static void	 unsnap_win(struct ws_win *, bool);
static void	 update_bars(struct swm_screen *);
static void	 update_debug(struct swm_screen *);
static void	 update_floater(struct ws_win *);
static void	 update_focus(struct swm_screen *);
static void	 update_gravity(struct ws_win *);
static void	 update_keycodes(void);
static void	 update_layout(struct swm_screen *);
static void	 update_modkey(uint16_t);
static void	 update_stackable(struct swm_stackable *,
		     struct swm_stackable *);
static void	 update_win_layer(struct ws_win *);
static void	 update_win_layer_related(struct ws_win *);
static void	 update_window(struct ws_win *);
static void	 updatenumlockmask(void);
static void	 usage(void);
static void	 validate_spawns(void);
static int	 validate_win(struct ws_win *);
static int	 validate_ws(struct workspace *);
static void	 version(struct swm_screen *, struct binding *, union arg *);
static bool	 win_below(struct ws_win *);
static uint16_t	 win_border(struct ws_win *);
static bool	 win_floating(struct ws_win *);
static bool	 win_focused(struct ws_win *);
static bool	 win_free(struct ws_win *);
static uint8_t	 win_gravity(struct ws_win *);
static bool	 win_main(struct ws_win *);
static bool	 win_raised(struct ws_win *);
static bool	 win_related(struct ws_win *, struct ws_win *);
static bool	 win_reparented(struct ws_win *);
static void	 win_to_ws(struct ws_win *, struct workspace *, uint32_t);
static bool	 win_transient(struct ws_win *);
static bool	 win_urgent(struct ws_win *);
static pid_t	 window_get_pid(xcb_window_t);
static void	 wkill(struct swm_screen *, struct binding *, union arg *);
static int	 workspace_cmp(struct workspace *, struct workspace *);
static struct workspace	*workspace_insert(struct swm_screen *, int);
static struct workspace	*workspace_lookup(struct swm_screen *, int);
static void		 workspace_remove(struct workspace *);
static bool	 ws_floating(struct workspace *);
static bool	 ws_focused(struct workspace *);
static bool	 ws_maponfocus(struct workspace *);
static bool	 ws_maxstack(struct workspace *);
static bool	 ws_maxstack_prior(struct workspace *);
static bool	 ws_root(struct workspace *);
static int	 xft_init(struct swm_screen *);
static void	 _add_startup_exception(const char *, va_list);
static void	 add_startup_exception(const char *, ...);

RB_PROTOTYPE_STATIC(binding_tree, binding, entry, binding_cmp);
RB_PROTOTYPE_STATIC(atom_name_tree, atom_name, entry, atom_name_cmp);
RB_PROTOTYPE_STATIC(workspace_tree, workspace, entry, workspace_cmp);

RB_GENERATE_STATIC(binding_tree, binding, entry, binding_cmp);
RB_GENERATE_STATIC(atom_name_tree, atom_name, entry, atom_name_cmp);
RB_GENERATE_STATIC(workspace_tree, workspace, entry, workspace_cmp);

static bool
win_free(struct ws_win *win)
{
	return (win && win->ws == win->s->r->ws);
}

static bool
win_floating(struct ws_win *win)
{
	return (win_transient(win) || win->ewmh_flags & EWMH_F_UNTILED ||
	    ws_floating(win->ws) || WINDOCK(win) || win_below(win));
}

static bool
win_focused(struct ws_win *win)
{
	return (win->s->focus == win);
}

static bool
win_raised(struct ws_win *win)
{
	return (win->ws->focus_raise == win ||
	    (win->ws->always_raise && win->ws->focus == win));
}

static bool
win_below(struct ws_win *win)
{
	return (BELOW(win) || (((win_free(win) && !win_focused(win)) ||
	    !win_related(get_ws_focus(win->ws), win) || (ws_focused(win->ws) &&
	    win_free(win->s->focus))) && ((FULLSCREEN(win) &&
	    fullscreen_unfocus == SWM_UNFOCUS_QUICK_BELOW) || (MAXIMIZED(win) &&
	    maximized_unfocus == SWM_UNFOCUS_QUICK_BELOW))));
}

static bool
win_reparented(struct ws_win *win)
{
	return (win->frame != XCB_WINDOW_NONE);
}

static bool
win_transient(struct ws_win *win)
{
	return (win->transient_for != XCB_WINDOW_NONE);
}

static bool
win_main(struct ws_win *win)
{
	return (win->main == win);
}

static bool
win_related(struct ws_win *w1, struct ws_win *w2)
{
	return (w1 && w2 && w1->main == w2->main);
}

static uint16_t
win_border(struct ws_win *win)
{
	return (win->bordered ? border_width : 0);
}

static bool
win_prioritized(struct ws_win *win)
{
	return (TAILQ_FIRST(&win->s->priority) == win);
}

static bool
ws_focused(struct workspace *ws)
{
	return (ws->r && ws->s->r_focus == ws->r);
}

static bool
ws_maponfocus(struct workspace *ws)
{
	return (ws->cur_layout->flags & SWM_L_MAPONFOCUS);
}

static bool
ws_maxstack(struct workspace *ws)
{
	return (ws->cur_layout == &layouts[SWM_MAX_STACK]);
}

static bool
ws_maxstack_prior(struct workspace *ws)
{
	return (ws->prev_layout == &layouts[SWM_MAX_STACK]);
}

static bool
ws_floating(struct workspace *ws)
{
	return (ws && ws->cur_layout->flags & SWM_L_NOTILE);
}

static bool
rg_root(struct swm_region *r)
{
	return (r && r->s->r == r);
}

static bool
ws_root(struct workspace *ws)
{
	return (ws && ws->s->r->ws == ws);
}

static bool
follow_mode(unsigned int type)
{
	return (focus_mode & type);
}

static xcb_window_t
st_window_id(struct swm_stackable *st)
{
	xcb_window_t	wid;

	switch (st->type) {
	case STACKABLE_WIN:
		wid = (win_reparented(st->win) ? st->win->frame : st->win->id);
		break;
	case STACKABLE_BAR:
		wid = st->bar->id;
		break;
	case STACKABLE_REGION:
		wid = st->region->id;
		break;
	default:
		wid = XCB_WINDOW_NONE;
		break;
	}

	return (wid);
}

static int
workspace_cmp(struct workspace *ws1, struct workspace *ws2)
{
	if (ws1->idx < ws2->idx)
		return (-1);
	if (ws1->idx > ws2->idx)
		return (1);
	return (0);
}

static struct workspace *
workspace_lookup(struct swm_screen *s, int id)
{
	struct workspace	ws;

	ws.idx = id;

	return (RB_FIND(workspace_tree, &s->workspaces, &ws));
}

/* Get/create workspace for given screen and id. */
static struct workspace *
get_workspace(struct swm_screen *s, int id)
{
	struct workspace	*ws;

	/* Hard limit. */
	if (id >= SWM_WS_MAX || id < -1)
		return (NULL);

	if ((ws = workspace_lookup(s, id)) == NULL)
		ws = workspace_insert(s, id);

	return (ws);
}

static struct workspace *
workspace_insert(struct swm_screen *s, int id)
{
	struct workspace	*ws;
	int			i;

	if ((ws = calloc(1, sizeof(struct workspace))) == NULL)
		err(1, "workspace_insert: calloc");

	ws->s = s;
	ws->idx = id;
	ws->name = NULL;
	ws->bar_enabled = true;
	ws->prev_layout = NULL;
	ws->focus = NULL;
	ws->focus_raise = NULL;
	ws->r = NULL;
	ws->old_r = NULL;
	ws->rotation = ROTATION_DEFAULT;
	TAILQ_INIT(&ws->winlist);

	ws->stacker_len = stack_mark_maxlen;
	ws->stacker = calloc(ws->stacker_len, sizeof(char));
	if (ws->stacker == NULL)
		err(1, "workspace_insert: stacker calloc");
	ws->stacker[0] = '\0';

	for (i = 0; i < LENGTH(layouts); i++)
		if (layouts[i].l_config != NULL)
			layouts[i].l_config(ws, SWM_ARG_ID_STACKINIT);
	ws->cur_layout = &layouts[0];

	if (RB_INSERT(workspace_tree, &s->workspaces, ws))
		/* An entry already exists. */
		errx(1, "workspace_insert: RB_INSERT");

	return (ws);
}

static void
workspace_remove(struct workspace *ws)
{
	RB_REMOVE(workspace_tree, &ws->s->workspaces, ws);
	free(ws->name);
	free(ws->stacker);
	free(ws);
}

static void
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

static void
cursors_cleanup(void)
{
	int	i;
	for (i = 0; i < LENGTH(cursors); ++i)
		xcb_free_cursor(conn, cursors[i].cid);
}

static char *
expand_tilde(const char *str)
{
	struct passwd		*ppwd;
	const char		*p, *s = str;
	char			*user, *result = NULL;

	if (s == NULL)
		errx(1, "expand_tilde: NULL string.");

	if (*s == '~') {
		p = ++s;
		while (*p != '\0' && *p != '/')
			p++;

		if (p - s > 0) {
			/* Assume tilde-prefix is a user. */
			if ((user = strndup(s, p - s)) == NULL)
				err(1, "expand_tilde: strndup");
			s = p;

			ppwd = getpwnam(user);
			free(user);
		} else
			ppwd = getpwuid(getuid());

		if (ppwd && asprintf(&result, "%s%s", ppwd->pw_dir, s) == -1)
			err(1, "expand_tilde: asprintf");
	}

	if (result == NULL) {
		if ((result = strdup(str)) == NULL)
			err(1, "expand_tilde: strdup");
	}

	return (result);
}

static int
parse_rgba(const char *rgba, uint16_t *rr, uint16_t *gg, uint16_t *bb,
    uint16_t *aa)
{
	unsigned int	tmpr, tmpg, tmpb, tmpa;

	if (sscanf(rgba, "rgba:%x/%x/%x/%x", &tmpr, &tmpg, &tmpb, &tmpa) != 4)
		return (-1);

	*rr = tmpr;
	*gg = tmpg;
	*bb = tmpb;
	*aa = tmpa;

	return (0);
}

static int
parse_rgb(const char *rgb, uint16_t *rr, uint16_t *gg, uint16_t *bb)
{
	unsigned int	tmpr, tmpg, tmpb;

	if (sscanf(rgb, "rgb:%x/%x/%x", &tmpr, &tmpg, &tmpb) != 3)
		return (-1);

	*rr = tmpr;
	*gg = tmpg;
	*bb = tmpb;

	return (0);
}

static const struct xcb_setup_t *
get_setup(void)
{
	int	 errcode = xcb_connection_has_error(conn);
#ifdef XCB_CONN_ERROR
	/* libxcb >= 1.8 */
	char	*s;
	switch (errcode) {
	case XCB_CONN_ERROR:
		s = "Socket, pipe or other stream error.";
		break;
	case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
		s = "Extension not supported.";
		break;
	case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
		s = "Insufficient memory.";
		break;
	case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
		s = "Request length exceeded.";
		break;
	case XCB_CONN_CLOSED_PARSE_ERR:
		s = "Error parsing display string.";
		break;
#ifdef XCB_CONN_CLOSED_INVALID_SCREEN
	/* libxcb >= 1.9 */
	case XCB_CONN_CLOSED_INVALID_SCREEN:
		s = "Invalid screen.";
		break;
#ifdef XCB_CONN_CLOSED_FDPASSING_FAILED
	/* libxcb >= 1.9.2 */
	case XCB_CONN_CLOSED_FDPASSING_FAILED:
		s = "Failed to pass file descriptor.";
		break;
#endif
#endif
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

static xcb_screen_t *
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

static int
get_screen_count(void)
{
	return (xcb_setup_roots_length(get_setup()));
}

static struct swm_region *
get_region(struct swm_screen *s, int index)
{
	struct swm_region 	*r;
	int			 i;

	DNPRINTF(SWM_D_FOCUS, "id: %d\n", index);

	if (index == 0)
		return (s->r);

	r = TAILQ_FIRST(&s->rl);
	for (i = 0; r && i < index - 1; ++i)
		r = TAILQ_NEXT(r, entry);

	return (r);
}

static int
get_region_index(struct swm_region *r)
{
	struct swm_region	*rr;
	int			 ridx;

	if (r == NULL)
		return (-1);

	if (rg_root(r))
		return (0);

	/* Dynamic regions begin at 1. */
	ridx = 1;
	TAILQ_FOREACH(rr, &r->s->rl, entry) {
		if (rr == r)
			break;
		ridx++;
	}

	if (rr == NULL)
		return (-1);

	return (ridx);
}

static void
flush(void)
{
	xcb_generic_event_t	*e;
	static bool		flushing = false;

	/* Ensure all pending requests have been processed. */
	xcb_aux_sync(conn);

	/* If called recursively via below loop, only sync. */
	if (flushing)
		return;

	flushing = true;
	while ((e = get_next_event(false))) {
		switch (XCB_EVENT_RESPONSE_TYPE(e)) {
		case XCB_ENTER_NOTIFY:
			event_time = ((xcb_enter_notify_event_t *)e)->time;
			pointer_window = ((xcb_enter_notify_event_t *)e)->event;
			DNPRINTF(SWM_D_EVENT, "pointer_window: %#x\n",
			    pointer_window);
			break;
		case XCB_MOTION_NOTIFY:
			event_time = ((xcb_motion_notify_event_t *)e)->time;
			break;
		default:
			event_handle(e);
		}
		free(e);
	}
	flushing = false;
}

static xcb_atom_t
get_intern_atom(const char *str)
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

static char *
get_atom_name(xcb_atom_t atom)
{
	xcb_get_atom_name_reply_t	*r;
	size_t				len;
	char				*name = NULL;

	if (!(swm_debug & SWM_D_ATOM))
		return (NULL);

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

	return (name);
}

static int
atom_name_cmp(struct atom_name *ap1, struct atom_name *ap2)
{
	if (ap1->atom < ap2->atom)
		return (-1);
	if (ap1->atom > ap2->atom)
		return (1);
	return (0);
}

static void
atom_name_insert(xcb_atom_t atom, char *name)
{
	struct atom_name	*ap;

	if ((ap = malloc(sizeof *ap)) == NULL)
		err(1, "atom_name_insert: malloc");

	ap->atom = atom;
	ap->name = name;
	if (RB_INSERT(atom_name_tree, &atom_names, ap))
		errx(1, "atom_name_insert: RB_INSERT");
}

static void
atom_name_remove(struct atom_name *ap)
{
	RB_REMOVE(atom_name_tree, &atom_names, ap);
	free(ap->name);
	free(ap);
}

static void
clear_atom_names(void)
{
	struct atom_name	*ap;

#ifndef __clang_analyzer__ /* Suppress false warnings. */
	while((ap = RB_ROOT(&atom_names)))
		atom_name_remove(ap);
#endif
}

static struct atom_name *
atom_name_lookup(xcb_atom_t atom)
{
	struct atom_name	ap;

	ap.atom = atom;

	return (RB_FIND(atom_name_tree, &atom_names, &ap));
}

static xcb_atom_t
get_atom_from_string(const char *str)
{
	xcb_atom_t		atom;
	char			*name;

	if (str == NULL)
		return (XCB_ATOM_NONE);

	atom = get_intern_atom(str);
	if (atom != XCB_ATOM_NONE && atom_name_lookup(atom) == NULL) {
		if ((name = strdup(str)) == NULL)
			err(1, "get_atom_from_string: strdup");
		atom_name_insert(atom, name);
	}

	return (atom);
}

static const char *
get_atom_label(xcb_atom_t atom)
{
	struct atom_name	*ap;
	char			*name;

	ap = atom_name_lookup(atom);
	if (ap)
		name = ap->name;
	else if (swm_debug & SWM_D_ATOM) {
		name = get_atom_name(atom);
		atom_name_insert(atom, name);
	} else
		name = "";

	return (name);
}

static void
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

static void
setup_ewmh(void)
{
	xcb_window_t			root, swmwin;
	int				i, j, num_screens;

	for (i = 0; i < LENGTH(ewmh); i++)
		ewmh[i].atom = get_atom_from_string(ewmh[i].name);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		root = screens[i].root;
		swmwin = screens[i].swmwin;

		/* Set up _NET_SUPPORTING_WM_CHECK. */
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
		    swmwin, ewmh[_NET_WM_NAME].atom, a_utf8_string,
		    8, strlen("spectrwm"), "spectrwm");
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
		    a_net_wm_check, XCB_ATOM_WINDOW, 32, 1, &swmwin);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, swmwin,
		    a_net_wm_check, XCB_ATOM_WINDOW, 32, 1, &swmwin);

		/* Report supported atoms */
		xcb_delete_property(conn, root, a_net_supported);
		for (j = 0; j < LENGTH(ewmh); j++)
			xcb_change_property(conn, XCB_PROP_MODE_APPEND, root,
			    a_net_supported, XCB_ATOM_ATOM, 32, 1,
			    &ewmh[j].atom);

		ewmh_update_number_of_desktops(&screens[i]);
		ewmh_get_desktop_names(&screens[i]);
		ewmh_update_desktop_viewports(&screens[i]);
	}
}

static void
teardown_ewmh(void)
{
	int				i, num_screens;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		xcb_delete_property(conn, screens[i].swmwin, a_net_wm_check);
		xcb_delete_property(conn, screens[i].root, a_net_wm_check);
		xcb_delete_property(conn, screens[i].root, a_net_supported);
	}
}

static void
ewmh_get_window_type(struct ws_win *win)
{
	xcb_get_property_reply_t	*r;
	xcb_get_property_cookie_t	c;
	xcb_atom_t			*type;
	int				i, j, n;

	c = xcb_get_property(conn, 0, win->id,
	    ewmh[_NET_WM_WINDOW_TYPE].atom, XCB_ATOM_ATOM, 0, UINT32_MAX);
	r = xcb_get_property_reply(conn, c, NULL);
	if (r == NULL)
		return;

	type = xcb_get_property_value(r);
	n = xcb_get_property_value_length(r) / sizeof(xcb_atom_t);

	win->type = 0;
	for (i = 0; i < n; i++)
		for (j = 0; j < EWMH_WINDOW_TYPE_COUNT; j++)
			if (type[i] == ewmh[ewmh_window_types[j].id].atom)
				win->type |= ewmh_window_types[j].flag;
	free(r);
}

static void
ewmh_print_window_type(uint32_t type)
{
	int		i;

	if (type == 0) {
		DPRINTF("None ");
		return;
	}

	for (i = 0; i < EWMH_WINDOW_TYPE_COUNT; i++)
		if (type & ewmh_window_types[i].flag)
			DPRINTF("%s ", ewmh_window_types[i].name);
}

static void
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

static uint32_t
ewmh_change_wm_state(struct ws_win *win, xcb_atom_t state, long action)
{
	uint32_t		flag = 0;
	uint32_t		new_flags;
	uint32_t		ret = 0;

	DNPRINTF(SWM_D_PROP, "win %#x, state: %s(%u), " "action: %ld\n",
	    WINID(win), get_atom_label(state), state, action);

	if (win == NULL)
		goto out;

	if (state == ewmh[_NET_WM_STATE_MAXIMIZED_VERT].atom ||
	    state == ewmh[_NET_WM_STATE_MAXIMIZED_HORZ].atom)
		flag = EWMH_F_MAXIMIZED;
	else if (state == ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom)
		flag = EWMH_F_SKIP_TASKBAR;
	else if (state == ewmh[_NET_WM_STATE_SKIP_PAGER].atom)
		flag = EWMH_F_SKIP_PAGER;
	else if (state == ewmh[_NET_WM_STATE_HIDDEN].atom)
		flag = EWMH_F_HIDDEN;
	else if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		flag = EWMH_F_FULLSCREEN;
	else if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		flag = EWMH_F_ABOVE;
	else if (state == ewmh[_NET_WM_STATE_BELOW].atom)
		flag = EWMH_F_BELOW;
	else if (state == ewmh[_NET_WM_STATE_DEMANDS_ATTENTION].atom)
		flag = EWMH_F_DEMANDS_ATTENTION;
	else if (state == ewmh[_SWM_WM_STATE_MANUAL].atom)
		flag = SWM_F_MANUAL;

	/* Disallow unfloating transients. */
	if (win_transient(win) && flag == EWMH_F_ABOVE)
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

	ret = ewmh_apply_flags(win, new_flags);
out:
	DNPRINTF(SWM_D_PROP, "done\n");
	return (ret);
}

static uint32_t
ewmh_apply_flags(struct ws_win *win, uint32_t pending)
{
	uint32_t		changed;

	changed = win->ewmh_flags ^ pending;
	if (changed == 0)
		return (changed);

	DNPRINTF(SWM_D_PROP, "pending: %u\n", pending);

	win->ewmh_flags = pending;

	if (changed & EWMH_F_HIDDEN) {
		if (HIDDEN(win)) {
			unmap_window(win);
		} else {
			/* Reload floating geometry in case region changed. */
			if (win_floating(win))
				load_float_geom(win);
		}
	}

	if (changed & EWMH_F_ABOVE || changed & EWMH_F_BELOW) {
		if (ABOVE(win) || BELOW(win))
			load_float_geom(win);
		else if (!MAXIMIZED(win))
			store_float_geom(win);
	}

	if (changed & EWMH_F_DEMANDS_ATTENTION)
		draw_frame(win);

	if (changed & EWMH_F_MAXIMIZED) {
		/* VERT and/or HORZ changed. */
		if (ABOVE(win) || BELOW(win)) {
			if (!MAXIMIZED(win))
				load_float_geom(win);
			else
				store_float_geom(win);
		}

		draw_frame(win);
	}

	if (changed & EWMH_F_FULLSCREEN) {
		if (!FULLSCREEN(win))
			load_float_geom(win);

		win->ewmh_flags &= ~EWMH_F_MAXIMIZED;
	}

	DNPRINTF(SWM_D_PROP, "changed: %#x\n", changed);
	return (changed);
}

static void
ewmh_update_wm_state(struct  ws_win *win) {
	xcb_atom_t		vals[SWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (MAXIMIZED_VERT(win))
		vals[n++] = ewmh[_NET_WM_STATE_MAXIMIZED_VERT].atom;
	if (MAXIMIZED_HORZ(win))
		vals[n++] = ewmh[_NET_WM_STATE_MAXIMIZED_HORZ].atom;
	if (SKIP_TASKBAR(win))
		vals[n++] = ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom;
	if (SKIP_PAGER(win))
		vals[n++] = ewmh[_NET_WM_STATE_SKIP_PAGER].atom;
	if (HIDDEN(win))
		vals[n++] = ewmh[_NET_WM_STATE_HIDDEN].atom;
	if (FULLSCREEN(win))
		vals[n++] = ewmh[_NET_WM_STATE_FULLSCREEN].atom;
	if (ABOVE(win))
		vals[n++] = ewmh[_NET_WM_STATE_ABOVE].atom;
	if (BELOW(win))
		vals[n++] = ewmh[_NET_WM_STATE_BELOW].atom;
	if (DEMANDS_ATTENTION(win))
		vals[n++] = ewmh[_NET_WM_STATE_DEMANDS_ATTENTION].atom;
	if (MANUAL(win))
		vals[n++] = ewmh[_SWM_WM_STATE_MANUAL].atom;

	if (n > 0)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
		    ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, n, vals);
	else
		xcb_delete_property(conn, win->id, ewmh[_NET_WM_STATE].atom);
}

static void
print_strut(struct swm_strut *st)
{
	if (st == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "win %#x left:%u right:%u top:%u bottom:%u "
	    "left_start_y:%u left_end_y:%u right_start_y:%u right_end_y:%u "
	    "top_start_x:%u top_end_x:%u bottom_start_x:%u bottom_end_x:%u\n",
	    st->win->id, st->left, st->right, st->top, st->bottom,
	    st->left_start_y, st->left_end_y, st->right_start_y, st->right_end_y,
	    st->top_start_x, st->top_end_x, st->bottom_start_x,
	    st->bottom_end_x);
}

static void
ewmh_get_strut(struct ws_win *win)
{
	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;
	struct swm_strut		*srt = NULL;
	uint32_t			*pv;

	if (win == NULL)
		return;

	if (win->strut) {
		SLIST_REMOVE(&win->s->struts, win->strut, swm_strut, entry);
		free(win->strut);
		win->strut = NULL;
	}

	/* _NET_WM_STRUT_PARTIAL: CARDINAL[12]/32 */
	c = xcb_get_property(conn, 0, win->id, ewmh[_NET_WM_STRUT_PARTIAL].atom,
	    XCB_ATOM_CARDINAL, 0, 12);
	r = xcb_get_property_reply(conn, c, NULL);
	if (r && r->format == 32 && r->length == 12) {
		if ((srt = calloc(1, sizeof(struct swm_strut))) == NULL)
			err(1, "ewmh_get_strut: calloc");

		pv = xcb_get_property_value(r);
		srt->left= pv[0];
		srt->right= pv[1];
		srt->top= pv[2];
		srt->bottom= pv[3];
		srt->left_start_y= pv[4];
		srt->left_end_y= pv[5];
		srt->right_start_y= pv[6];
		srt->right_end_y= pv[7];
		srt->top_start_x= pv[8];
		srt->top_end_x= pv[9];
		srt->bottom_start_x= pv[10];
		srt->bottom_end_x= pv[11];
		srt->win = win;

		if (swm_debug)
			print_strut(srt);
	} else {
		free(r);
		/* _NET_WM_STRUT: CARDINAL[4]/32 */
		c = xcb_get_property(conn, 0, win->id, ewmh[_NET_WM_STRUT].atom,
		    XCB_ATOM_CARDINAL, 0, 4);
		r = xcb_get_property_reply(conn, c, NULL);
		if (r && r->format == 32 && r->length == 4) {
			if ((srt = calloc(1, sizeof(struct swm_strut))) == NULL)
				err(1, "ewmh_get_strut: calloc");

			pv = xcb_get_property_value(r);
			srt->left= pv[0];
			srt->right= pv[1];
			srt->top= pv[2];
			srt->bottom= pv[3];
			srt->win = win;

			if (swm_debug)
				print_strut(srt);
		}
	}

	if (srt) {
		win->strut = srt;
		SLIST_INSERT_HEAD(&win->s->struts, srt, entry);
	}

	free(r);
}

static void
apply_struts(struct swm_screen *s, struct swm_geometry *g)
{
	struct swm_strut	*srt;

	/* Reduce available area for struts. */
	SLIST_FOREACH(srt, &s->struts, entry) {
		if (HIDDEN(srt->win) || srt->win->ws->r == NULL)
			continue;
		if (srt->top && (srt->top > (uint32_t)g->y &&
		    srt->top <= (uint32_t)g->y + g->h) &&
		    ((srt->top_start_x == 0 && srt->top_end_x == 0) ||
		    (srt->top_end_x > (uint32_t)g->x &&
		    srt->top_start_x < (uint32_t)g->x + g->w))) {
			g->h -= srt->top - (uint32_t)g->y;
			g->y = srt->top;
		}
		if (srt->bottom && (HEIGHT(s->r) - srt->bottom <
		    (uint32_t)g->y + g->h) && ((srt->bottom_start_x == 0
		    && srt->bottom_end_x == 0) || (srt->bottom_end_x >
		    (uint32_t)g->x && srt->bottom_start_x <
		    (uint32_t)g->x + g->w))) {
			g->h = HEIGHT(s->r) - srt->bottom - g->y;
		}
		if (srt->left && (srt->left > (uint32_t)g->x &&
		    srt->left <= (uint32_t)g->x + g->w) &&
		    ((srt->left_start_y == 0 && srt->left_end_y == 0) ||
		    (srt->left_end_y > (uint32_t)g->y &&
		    srt->left_start_y < (uint32_t)g->y + g->h))) {
			g->w -= srt->left - (uint32_t)g->x;
			g->x = srt->left;
		}
		if (srt->right && (WIDTH(s->r) - srt->right <
		    (uint32_t)g->x + g->w) && ((srt->right_start_y == 0 &&
		    srt->right_end_y == 0) || (srt->right_end_y >
		    (uint32_t)g->y && srt->right_start_y <
		    (uint32_t)g->y + g->h))) {
			g->w = WIDTH(s->r) - srt->right - g->x;
		}
	}
}

static int
refresh_strut(struct swm_screen *s)
{
	int			changed = 0;
	struct swm_region	*r;
	struct swm_geometry	g;
	uint32_t		wc[4];

	g = s->r->g;
	apply_struts(s, &g);
	s->r->g_usable = (g.w > 0 && g.h > 0) ? g : s->r->g;

	TAILQ_FOREACH(r, &s->rl, entry) {
		g = r->g;
		apply_struts(s, &g);

		if ((g.x == r->g_usable.x && g.y == r->g_usable.y &&
		    g.w == r->g_usable.w && g.h == r->g_usable.h))
			continue;

		r->g_usable = (g.w > 0 && g.h > 0) ? g : r->g;
		changed++;

		DNPRINTF(SWM_D_MISC, "r%d usable:%dx%d+%d+%d\n",
		    get_region_index(r), r->g_usable.w, r->g_usable.h,
		    r->g_usable.x, r->g_usable.y);

		if (r->bar) {
			if (bar_at_bottom)
				g.y += g.h - bar_height;
			g.h = bar_height;
			wc[0] = g.x;
			wc[1] = g.y;
			wc[2] = g.w;
			wc[3] = g.h;
			g.x += bar_border_width;
			g.y += bar_border_width;
			g.w -= 2 * bar_border_width;
			g.h -= 2 * bar_border_width;
			r->bar->g = g;
			xcb_configure_window(conn, r->bar->id,
			    XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
			    XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
			    wc);
		}
	}

	ewmh_update_workarea(s);
	return (changed);
}

static void
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

static void
ewmh_update_active_window(struct swm_screen *s)
{
	xcb_window_t		awid;

	if (s->focus)
		awid = s->focus->id;
	else
		awid = XCB_WINDOW_NONE;

	if (awid != s->active_window) {
		DNPRINTF(SWM_D_FOCUS, "root: %#x win: %#x\n", s->root,
		    s->active_window);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
		    ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1,
		    &awid);
		s->active_window = awid;
	}
}

static void
dumpwins(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct workspace			*ws;
	struct ws_win				*w;
	uint32_t				state;
	xcb_get_window_attributes_cookie_t	c;
	xcb_get_window_attributes_reply_t	*wa;
	int					i;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	if (swm_debug == 0)
		return;

	DPRINTF("=== Screen %d Focus Information ===\n", s->idx);
	DPRINTF("r_focus:%d focus:%#x\n",
	    get_region_index(s->r_focus), WINID(s->focus));
	RB_FOREACH(ws, workspace_tree, &s->workspaces)
		DPRINTF("ws:%2d f:%#9x pf:%#9x ar:%d fr:%#x\n", ws->idx,
		   WINID(ws->focus), WINID(get_ws_focus_prev(ws)),
		   ws->always_raise, WINID(ws->focus_raise));

	DPRINTF("=== Screen %d Managed Windows ===\n", s->idx);
	TAILQ_FOREACH(w, &s->managed, manage_entry) {
		state = get_win_state(w->id);
		c = xcb_get_window_attributes(conn, w->id);
		wa = xcb_get_window_attributes_reply(conn, c, NULL);
		if (wa) {
			DPRINTF("win %#9x (f:%#x) ws:%02d map_state:%d"
			    " state:%.3s transient_for:%#x main:%#9x"
			    " parent:%#x\n", w->id, w->frame, w->ws->idx,
			    wa->map_state, get_wm_state_label(state),
			    w->transient_for, WINID(w->main), WINID(w->parent));
			free(wa);
		} else
			DPRINTF("win %#x GetWindowAttributes failed.\n", w->id);
	}
	DPRINTF("=== Screen %d Window Priority (high to low) ===\n", s->idx);
	i = 0;
	TAILQ_FOREACH(w, &s->priority, priority_entry)
		DPRINTF("%2d) win %#9x (f:%#x) st:%u\n", i++, w->id,
		    w->frame, w->st->layer);
	print_stacking(s);
}

static void
print_stackable(struct swm_stackable *st)
{
	struct ws_win		*w;
	struct swm_bar		*b;
	struct swm_region	*r;

	switch (st->type) {
	case STACKABLE_WIN:
		w = st->win;
		DPRINTF("l:%d win %#9x (f:%#x) mp:%d ws:%2i fs:%d mx:%d "
		    "ab:%d bl:%d ic:%d ra:%d\n", st->layer, w->id, w->frame,
		    w->mapped, w->ws->idx, (FULLSCREEN(w) != 0),
		    (MAXIMIZED(w) != 0), (ABOVE(w) != 0), (BELOW(w) != 0),
		    (HIDDEN(w) != 0), win_raised(w));
		break;
	case STACKABLE_BAR:
		b = st->bar;
		DPRINTF("l:%d bar %#9x region:%d enabled:%d\n",
		    b->st->layer, b->id, get_region_index(b->r),
		    b->r->ws->bar_enabled);
		break;
	case STACKABLE_REGION:
		r = st->region;
		DPRINTF("l:%d rgn %#9x region:%d\n", st->layer, r->id,
		    get_region_index(r));
		break;
	default:
		DPRINTF("invalid type:%d\n", st->type);
		break;
	}
}

static void
print_stacking(struct swm_screen *s)
{
	struct swm_stackable	*st;

	DPRINTF("=== stacking order (bottom up) ===\n");
	SLIST_FOREACH(st, &s->stack, entry)
		print_stackable(st);
	DPRINTF("=================================\n");
}

static void
debug_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	int			num_screens, i;

	/* Suppress warnings. */
	(void)s;
	(void)bp;
	(void)args;

	if (swm_debug == 0)
		return;

	debug_enabled = !debug_enabled;
	DNPRINTF(SWM_D_MISC, "debug_enabled: %s\n", YESNO(debug_enabled));

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		update_debug(&screens[i]);

	xcb_flush(conn);
}

#define DEBUG_MAXROWS		(3)
static void
debug_refresh(struct ws_win *win)
{
	struct swm_screen	*s;
	struct swm_stackable	*st;
	struct ws_win		*w;
	XftDraw			*draw;
	XGlyphInfo		info;
	GC			l_draw;
	XGCValues		l_gcv;
	XRectangle		l_ibox, l_lbox = {0, 0, 0, 0};
	xcb_rectangle_t		rect;
	uint32_t		wc[4], mask, width, height, gcv[1];
	int			widx, sidx, fidx, pidx, i, rows;
	size_t			len[DEBUG_MAXROWS];
	char			*str[DEBUG_MAXROWS];
	char			*buf, *sp, *b;

	if (debug_enabled) {
		s = win->s;

		/* Create debug window if it doesn't exist. */
		if (win->debug == XCB_WINDOW_NONE) {
			win->debug = xcb_generate_id(conn);
			wc[0] = getcolorpixel(s, SWM_S_COLOR_BAR, 0);
			wc[1] = getcolorpixel(s, SWM_S_COLOR_BAR_BORDER, 0);
			wc[2] = s->colormap;

			xcb_create_window(conn, s->depth, win->debug,
			    win->frame, 0, 0, 10, 10, 1,
			    XCB_WINDOW_CLASS_INPUT_OUTPUT, s->visual,
			    XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
			    XCB_CW_COLORMAP, wc);

			if (win->mapped)
				xcb_map_window(conn, win->debug);
		}

		if (!win->mapped)
			return;

		/* Determine workspace window list index. */
		widx = 0;
		TAILQ_FOREACH(w, &win->ws->winlist, entry) {
			++widx;
			if (w == win)
				break;
		}

		/* Determine stacking index (bottom up). */
		sidx = 0;
		SLIST_FOREACH(st, &s->stack, entry) {
			++sidx;
			if (st->type == STACKABLE_WIN && st->win == win)
				break;
		}

		/* Determine recent focus index (most recent first). */
		fidx = 0;
		TAILQ_FOREACH(w, &s->fl, focus_entry) {
			++fidx;
			if (w == win)
				break;
		}

		/* Determine priority index (highest first). */
		pidx = 0;
		TAILQ_FOREACH(w, &s->priority, priority_entry) {
			++pidx;
			if (w == win)
				break;
		}

		if (asprintf(&buf,
		    "%#x f:%#x m:%#x p:%#x\n"
		    "l:%d wl:%d st:%d fl:%d pr:%d\n"
		    "vi:%#x cm:%#x im:%s",
		    win->id, win->frame, win->main->id, WINID(win->parent),
		    st->layer, widx, sidx, fidx, pidx, s->visual,
		    s->colormap, get_win_input_model_label(win)) == -1)
			return;

		/* Determine rows and window dimensions. */
		sp = buf;
		width = 1;
		rows = 0;
		while ((b = strsep(&sp, "\n"))) {
			if (*b == '\0')
				continue;

			str[rows] = b;
			len[rows] = strlen(b);

			if (bar_font_legacy) {
				TEXTEXTENTS(bar_fs, str[rows], len[rows],
				    &l_ibox, &l_lbox);
				if (l_lbox.width > (int)width)
					width = l_lbox.width;
			} else {
				XftTextExtentsUtf8(display,
				    s->bar_xftfonts[0],
				    (FcChar8 *)str[rows], len[rows], &info);
				if (info.xOff > (int)width)
					width = info.xOff;
			}
			rows++;
			if (rows == DEBUG_MAXROWS)
				break;
		}

		if (bar_font_legacy)
			height = bar_fs_extents-> max_logical_extent.height;
		else
			height = s->bar_xftfonts[0]->height;

		/* Add 1px pad. */
		width += 2;
		height += 2;

		mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
		    XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
		wc[0] = wc[1] = win_border(win);

		/* Add 1px pad for window. */
		wc[2] = width + 2;
		wc[3] = height * rows + 2;

		xcb_configure_window(conn, win->debug, mask, wc);

		/* Draw a filled rectangle to 'clear' window. */
		rect.x = 0;
		rect.y = 0;
		rect.width = wc[2];
		rect.height = wc[3];

		gcv[0] = getcolorpixel(s, SWM_S_COLOR_BAR, 0);
		xcb_change_gc(conn, s->gc, XCB_GC_FOREGROUND, gcv);
		xcb_poly_fill_rectangle(conn, win->debug, s->gc, 1, &rect);

		/* Draw text. */
		if (bar_font_legacy) {
			l_gcv.graphics_exposures = 0;
			l_draw = XCreateGC(display, win->debug, 0, &l_gcv);

			XSetForeground(display, l_draw,
			    getcolorpixel(s, SWM_S_COLOR_BAR_FONT, 0));

			for (i = 0; i < rows; i++)
				DRAWSTRING(display,
				    win->debug, bar_fs, l_draw, 2,
				    (height - l_lbox.height) / 2 - l_lbox.y +
				    height * i + 1, str[i], len[i]);

			XFreeGC(display, l_draw);
		} else {
			draw = XftDrawCreate(display, win->debug,
			    s->xvisual, s->colormap);

			for (i = 0; i < rows; i++)
				XftDrawStringUtf8(draw,
				    getcolorxft(s, SWM_S_COLOR_BAR_FONT, 0),
				    s->bar_xftfonts[0], 2, (height +
				    s->bar_xftfonts[0]->height) / 2 -
				    s->bar_xftfonts[0]->descent +
				    height * i + 1, (FcChar8 *)str[i], len[i]);

			XftDrawDestroy(draw);
		}

		free(buf);
	} else if (win->debug != XCB_WINDOW_NONE) {
		xcb_destroy_window(conn, win->debug);
		win->debug = XCB_WINDOW_NONE;
	}
}

static void
update_debug(struct swm_screen *s)
{
	struct ws_win		*w;

	if (swm_debug == 0)
		return;

	TAILQ_FOREACH(w, &s->managed, manage_entry)
		debug_refresh(w);
}

static void
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
				if (errno != ECHILD)
					warn("sighdlr: waitpid");
				break;
			}
			if (pid == searchpid)
				search_resp = 1;

			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0)
					warnx("sighdlr: child exit status: %d",
					    WEXITSTATUS(status));
			} else
				warnx("sighdlr: child is terminated "
				    "abnormally");
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

static struct pid_e *
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

static char *
color_to_rgb(struct swm_color *color)
{
	char *name;

	if (asprintf(&name, "rgb:%04x/%04x/%04x",
	    color->r_orig, color->g_orig, color->b_orig) == -1)
		err(1, "color_to_rgb: asprintf");

	return (name);
}

static int
parse_color(struct swm_screen *s, const char *name, struct swm_color *color)
{
	char				cname[32] = "#";
	xcb_lookup_color_reply_t	*lcr;
	uint16_t			rr, gg, bb, aa;
	bool				valid = false;

	if (s == NULL || name == NULL || color == NULL)
		return (1);
	/*
	 * rgba color is in format rgba://rr/gg/bb/aa
	 * rgb color is in format rgb://rr/gg/bb
	 */
	if (strncmp(name, "rgba:", 5) == 0) {
		if (parse_rgba(name, &rr, &gg, &bb, &aa) != -1)
			valid = true;
		else
			warnx("could not parse rgba %s", name);
	} else if (strncmp(name, "rgb:", 4) == 0) {
		if (parse_rgb(name, &rr, &gg, &bb) != -1) {
			aa = 0xff;
			valid = true;
		} else
			warnx("could not parse rgb %s", name);
	} else {
		lcr = xcb_lookup_color_reply(conn, xcb_lookup_color(conn,
		    s->colormap, strlen(name), name), NULL);
		if (lcr == NULL) {
			strlcat(cname, name + 2, sizeof cname - 1);
			lcr = xcb_lookup_color_reply(conn,
			    xcb_lookup_color(conn, s->colormap, strlen(cname),
			    cname), NULL);
		}
		if (lcr) {
			rr = lcr->visual_red;
			gg = lcr->visual_green;
			bb = lcr->visual_blue;
			aa = 0xff;
			valid = true;
			free(lcr);
		} else
			warnx("color '%s' not found", name);
	}

	if (!valid)
		return (1);

	color->r = color->r_orig = RGB_8_TO_16(rr);
	color->g = color->g_orig = RGB_8_TO_16(gg);
	color->b = color->b_orig = RGB_8_TO_16(bb);
	color->a = RGB_8_TO_16(aa);

	return (0);
}

static void
setscreencolor(struct swm_screen *s, const char *val, int c, int i)
{
	struct swm_color		*color;
	xcb_screen_t			*scr;
	xcb_visualtype_t		*vis;
	xcb_alloc_color_reply_t		*cr;
	uint32_t			mask;
	int				rgbdepth = 0, j;

	if (s == NULL || val == NULL || c < 0 || c >= SWM_S_COLOR_MAX)
		return;

	if ((scr = get_screen(s->idx)) == NULL) {
		DNPRINTF(SWM_D_CONF, "failed to get screen %d\n", s->idx);
		return;
	}

	if ((color = calloc(1, sizeof(struct swm_color))) == NULL)
		err(1, "setscreencolor: calloc");

	if (parse_color(s, val, color)) {
		DNPRINTF(SWM_D_CONF, "failed to parse color: %s\n", val);
		free(color);
		return;
	}

	vis = xcb_aux_find_visual_by_id(scr, s->visual);
	DNPRINTF(SWM_D_CONF, "vis %#x, class:%u, red_mask: %#x, "
	    "green_mask %#x, blue_mask %#x\n", vis->visual_id, vis->_class,
	    vis->red_mask, vis->green_mask, vis->blue_mask);

	/* Count RGB bits. */
	mask = vis->red_mask | vis->blue_mask | vis->green_mask;
	while (mask) {
		if (mask & 0x1)
			rgbdepth++;
		mask >>= 1;
	}

	if (s->depth <= rgbdepth)
		/* No extra bits for alpha. */
		color->a = 0xffff;

	if (vis->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
		/* Roll our own pixel. */

		/* Premultiply alpha. */
		color->r = (uint32_t)(color->r * (double)color->a / 0xffff);
		color->g = (uint32_t)(color->g * (double)color->a / 0xffff);
		color->b = (uint32_t)(color->b * (double)color->a / 0xffff);

		/* Fit color values into pixel masks. */
#define FITMASK(c, m)		((uint32_t)((double)(c)/0xffff * (m)) & (m))
		color->pixel = FITMASK(color->r, vis->red_mask) |
			FITMASK(color->g, vis->green_mask) |
			FITMASK(color->b, vis->blue_mask);

		if (s->depth > rgbdepth) {
			/* Assume extra bits are for alpha. */
			mask = ~(vis->red_mask | vis->blue_mask |
			    vis->green_mask);
			color->pixel |= FITMASK(color->a, mask);
		}
#undef FITMASK
	} else {
		/* Get pixel from server. */
		cr = xcb_alloc_color_reply(conn, xcb_alloc_color(conn,
		    s->colormap, color->r, color->g, color->b), NULL);
		if (cr) {
			color->pixel = cr->pixel;
			free(cr);
		} else {
			warnx("color '%s' not found", val);
			free(color);
			return;
		}
	}

	if (i >= s->c[c].count) {
		s->c[c].colors = reallocarray(s->c[c].colors, i + 1,
		    sizeof(struct swm_color *));
		if (s->c[c].colors == NULL)
			err(1, "setscreencolor: reallocarray");

		/* Init new slots. */
		for (j = s->c[c].count; j < i + 1; j++)
			s->c[c].colors[j] = NULL;
		s->c[c].count = i + 1;
	}

	if (s->c[c].colors[i])
		free(s->c[c].colors[i]);
	s->c[c].colors[i] = color;

	DNPRINTF(SWM_D_CONF, "set c[%d][%d] r:%#x g:%#x b:%#x a:%#x pixel:%#x\n",
	    c, i, color->r, color->g, color->b, color->a, color->pixel);
}

static void
freecolortype(struct swm_screen *s, int c)
{
	int i;

	for (i = 0; i < s->c[c].count; i++)
		free(s->c[c].colors[i]);
	free(s->c[c].colors);

	s->c[c].colors = NULL;
	s->c[c].count = 0;
}

static struct swm_color *
getcolor(struct swm_screen *s, int c, int i)
{
	if (i < s->c[c].count && s->c[c].colors[i])
		return (s->c[c].colors[i]);

	/* Try fallbacks. */
	switch (c) {
	case SWM_S_COLOR_BAR_UNFOCUS:
	case SWM_S_COLOR_BAR_FREE:
	case SWM_S_COLOR_BAR_SELECTED:
		c = SWM_S_COLOR_BAR;
		break;
	case SWM_S_COLOR_BAR_FONT_UNFOCUS:
	case SWM_S_COLOR_BAR_FONT_FREE:
		c = SWM_S_COLOR_BAR_FONT;
		break;
	case SWM_S_COLOR_BAR_FONT_SELECTED:
		c = SWM_S_COLOR_BAR;
		break;
	case SWM_S_COLOR_FOCUS_MAXIMIZED:
		c = SWM_S_COLOR_FOCUS;
		break;
	case SWM_S_COLOR_UNFOCUS_MAXIMIZED:
		c = SWM_S_COLOR_UNFOCUS;
		break;
	case SWM_S_COLOR_URGENT_MAXIMIZED:
		c = SWM_S_COLOR_URGENT;
		if (i >= s->c[c].count || s->c[c].colors[i] == NULL)
			c = SWM_S_COLOR_UNFOCUS;
		break;
	case SWM_S_COLOR_FOCUS_MAXIMIZED_FREE:
		c = SWM_S_COLOR_FOCUS_FREE;
		break;
	case SWM_S_COLOR_UNFOCUS_MAXIMIZED_FREE:
		c = SWM_S_COLOR_UNFOCUS_FREE;
		break;
	case SWM_S_COLOR_URGENT_MAXIMIZED_FREE:
		c = SWM_S_COLOR_URGENT_FREE;
		if (i >= s->c[c].count || s->c[c].colors[i] == NULL)
			c = SWM_S_COLOR_UNFOCUS_FREE;
		break;
	case SWM_S_COLOR_URGENT:
		c = SWM_S_COLOR_UNFOCUS;
		break;
	case SWM_S_COLOR_URGENT_FREE:
		c = SWM_S_COLOR_UNFOCUS_FREE;
		break;
	default:
		DNPRINTF(SWM_D_BAR, "no fallback [%d][%d]\n", c, i);
		return (NULL);
	}

	if (i < s->c[c].count && s->c[c].colors[i])
		return (s->c[c].colors[i]);

	DNPRINTF(SWM_D_BAR, "missing fallback [%d][%d]\n", c, i);
	return (NULL);
}

static uint32_t
getcolorpixel(struct swm_screen *s, int c, int i)
{
	struct swm_color	*color;

	color = getcolor(s, c, i);
	if (color)
		return (color->pixel);

	return (0);
}

static char *
getcolorrgb(struct swm_screen *s, int c, int i)
{
	struct swm_color	*color;

	color = getcolor(s, c, i);
	if (color == NULL)
		errx(1, "getcolorrgb: invalid color index");

	return (color_to_rgb(color));
}

static XftColor *
getcolorxft(struct swm_screen *s, int c, int i)
{
	struct swm_color	*color;

	color = getcolor(s, c, i);
	if (color)
		return (&color->xft_color);

	DNPRINTF(SWM_D_BAR, "invalid color index\n");

	return (NULL);

}

static void
fancy_stacker(struct workspace *ws)
{
	if (ws->cur_layout->l_stack == vertical_stack)
		snprintf(ws->stacker, ws->stacker_len,
		    ws->l_state.vertical_flip ? "[%d>%d]" : "[%d|%d]",
		    ws->l_state.vertical_mwin, ws->l_state.vertical_stacks);
	else if (ws->cur_layout->l_stack == horizontal_stack)
		snprintf(ws->stacker, ws->stacker_len,
		    ws->l_state.horizontal_flip ? "[%dv%d]" : "[%d-%d]",
		    ws->l_state.horizontal_mwin, ws->l_state.horizontal_stacks);
	else if (ws->cur_layout->l_stack == floating_stack)
		strlcpy(ws->stacker, "[ ~ ]", ws->stacker_len);
	else
		strlcpy(ws->stacker, "[   ]", ws->stacker_len);
}

static void
plain_stacker(struct workspace *ws)
{
	if (ws->cur_layout->l_stack == vertical_stack)
		strlcpy(ws->stacker, (ws->l_state.vertical_flip ?
		    stack_mark_vertical_flip : stack_mark_vertical),
		    ws->stacker_len);
	else if (ws->cur_layout->l_stack == horizontal_stack)
		strlcpy(ws->stacker, (ws->l_state.horizontal_flip ?
		    stack_mark_horizontal_flip : stack_mark_horizontal),
		    ws->stacker_len);
	else if (ws->cur_layout->l_stack == floating_stack)
		strlcpy(ws->stacker, stack_mark_floating, ws->stacker_len);
	else
		strlcpy(ws->stacker, stack_mark_max, ws->stacker_len);
}

static void
socket_setnonblock(int fd)
{
	int			flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		err(1, "fcntl F_GETFL");
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
		err(1, "fcntl F_SETFL");
}

static void
bar_print_legacy(struct swm_region *r, const char *s)
{
	xcb_rectangle_t		rect;
	uint32_t		gcv[1];
	XGCValues		gcvd;
	int			x = 0, fg_type, bg_type;
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

	/* Setup default fg/bg index type */
	if (win_free(r->s->focus) && r->s->r_focus == r) {
		fg_type = SWM_S_COLOR_BAR_FONT_FREE;
		bg_type = SWM_S_COLOR_BAR_FREE;
	} else if (ws_focused(r->ws)) {
		fg_type = SWM_S_COLOR_BAR_FONT;
		bg_type = SWM_S_COLOR_BAR;
	} else {
		fg_type = SWM_S_COLOR_BAR_FONT_UNFOCUS;
		bg_type = SWM_S_COLOR_BAR_UNFOCUS;
	}

	/* clear back buffer */
	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);

	gcv[0] = getcolorpixel(r->s, bg_type, 0);
	xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1, &rect);

	/* draw back buffer */
	gcvd.graphics_exposures = 0;
	draw = XCreateGC(display, r->bar->buffer, GCGraphicsExposures, &gcvd);
	XSetForeground(display, draw, getcolorpixel(r->s, fg_type, 0));
	DRAWSTRING(display, r->bar->buffer, bar_fs, draw,
	    x, (bar_fs_extents->max_logical_extent.height - lbox.height) / 2 -
	    lbox.y, s, len);
	XFreeGC(display, draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->gc, 0, 0,
	    0, 0, WIDTH(r->bar), HEIGHT(r->bar));
}

static void
bar_print(struct swm_region *r, const char *s)
{
	size_t				len;
	xcb_rectangle_t			rect;
	uint32_t			gcv[1];
	int32_t				x = 0;
	XGlyphInfo			info;
	XftDraw				*draw;
	XftFont				*xf;

	len = strlen(s);
	xf = r->s->bar_xftfonts[0];

	XftTextExtentsUtf8(display, xf, (FcChar8 *)s, len, &info);

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
	rect.width = WIDTH(r->bar) + 2 * bar_border_width;
	rect.height = HEIGHT(r->bar) + 2 * bar_border_width;

	gcv[0] = getcolorpixel(r->s, SWM_S_COLOR_BAR, 0);
	xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1, &rect);

	/* draw back buffer */
	draw = XftDrawCreate(display, r->bar->buffer, r->s->xvisual,
	    r->s->colormap);

	XftDrawStringUtf8(draw, getcolorxft(r->s, SWM_S_COLOR_BAR_FONT, 0), xf,
	    x, (HEIGHT(r->bar) + xf->height) / 2 - xf->descent, (FcChar8 *)s,
	    len);

	XftDrawDestroy(draw);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->gc, 0, 0, 0, 0,
	    WIDTH(r->bar) + 2 * bar_border_width,
	    HEIGHT(r->bar) + 2 * bar_border_width);
}

static void
bar_print_layout(struct swm_region *r)
{
	struct text_fragment	*frag;
	xcb_rectangle_t		rect;
	xcb_point_t		points[5];
	XftDraw			*xft_draw = NULL;
	XRectangle		x_rect;
	XftFont			*xf;
	GC			draw = 0;
	XGCValues		gcvd;
	uint32_t		gcv[2];
	int			xpos, i, j;
	int			bd_type, bg, bg_type, fg, fg_type, fn;
	int 			space, remain, weight;

	space =  WIDTH(r) - 2 * bar_border_width;
	weight = 0;

	/* Parse markup sequences in each section  */
	/* For the legacy font, just setup one text fragment  */
	for (i = 0; i < numsect; i++) {
		bar_parse_markup(r->s, bsect + i);
		if (bsect[i].fit_to_text) {
			bsect[i].width = bsect[i].text_width + 2 *
			    SWM_BAR_OFFSET;
			space -= bsect[i].width;
		} else
			weight += bsect[i].weight;
	}

	/* Calculate width for each text justified section  */
	remain = space;
	j = -1;
	for (i = 0; i < numsect; i++)
		if (!bsect[i].fit_to_text && weight > 0) {
			bsect[i].width = bsect[i].weight * space / weight;
			remain -= bsect[i].width;
			j = i;
		}

	/* Add any space that was rounded off to the last section. */
	if (j != -1)
		bsect[j].width += remain;

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

	/* Setup default fg/bg index type */
	if (win_free(r->s->focus) && r->s->r_focus == r) {
		fg_type = SWM_S_COLOR_BAR_FONT_FREE;
		bg_type = SWM_S_COLOR_BAR_FREE;
		bd_type = SWM_S_COLOR_BAR_BORDER_FREE;
	} else if (ws_focused(r->ws)) {
		fg_type = SWM_S_COLOR_BAR_FONT;
		bg_type = SWM_S_COLOR_BAR;
		bd_type = SWM_S_COLOR_BAR_BORDER;
	} else {
		fg_type = SWM_S_COLOR_BAR_FONT_UNFOCUS;
		bg_type = SWM_S_COLOR_BAR_UNFOCUS;
		bd_type = SWM_S_COLOR_BAR_BORDER_UNFOCUS;
	}

	/* Paint entire bar with default background color */
	rect.x = bar_border_width;
	rect.y = bar_border_width;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);
	gcv[0] = getcolorpixel(r->s, bg_type, 0);
	xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->gc, 1, &rect);

	/* Draw border. */
	if (bar_border_width > 0) {
	        points[0].x = points[0].y = bar_border_width / 2;
	        points[1].x = bar_border_width + WIDTH(r->bar) + points[0].x;
	        points[1].y = points[0].y;
	        points[2].x = points[1].x;
	        points[2].y = bar_border_width + HEIGHT(r->bar) + points[0].y;
	        points[3].x = points[0].x;
	        points[3].y = points[2].y;
	        points[4] = points[0];
	        gcv[0] = getcolorpixel(r->s, bd_type, 0);
	        gcv[1] = bar_border_width;
	        xcb_change_gc(conn, r->s->gc,
		    XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH, gcv);
	        xcb_poly_line(conn, XCB_COORD_MODE_ORIGIN, r->bar->buffer,
		    r->s->gc, 5, points);
	}

	/* Display the text for each section */
	for (i = 0; i < numsect; i++) {
		rect.x = bar_border_width + bsect[i].start;
		rect.y = bar_border_width;
		rect.width = bsect[i].width;
		rect.height = HEIGHT(r->bar);

		/* No space to draw that section */
		if (rect.width < 1)
			continue;

		/* No space to draw anything else */
		if (rect.width < SWM_BAR_OFFSET)
			continue;

		/* Set the clip rectangle to avoid text overflow */
		x_rect.x = rect.x;
		x_rect.y = rect.y;
		x_rect.width = rect.width;
		x_rect.height = rect.height;
		if (bar_font_legacy)
			XSetClipRectangles(display, draw, 0, 0, &x_rect, 1,
			    YXBanded);
		else
			XftDrawSetClipRectangles(xft_draw, 0, 0, &x_rect, 1);

		/* Draw the text fragments in the current section */
		xpos = bar_border_width + bsect[i].text_start;
		for (j = 0; j < bsect[i].nfrags; j++) {
			frag = bsect[i].frag + j;
			fn = frag->font;
			fg = frag->fg;
			bg = frag->bg;

			/* Paint background color of the text fragment  */
			if (bg != 0) {
				rect.x = xpos;
				rect.width = frag->width;
				gcv[0] = getcolorpixel(r->s, bg_type, bg);
				xcb_change_gc(conn, r->s->gc, XCB_GC_FOREGROUND,
				    gcv);
				xcb_poly_fill_rectangle(conn, r->bar->buffer,
				    r->s->gc, 1, &rect);
			}

			/* Draw text  */
			if (bar_font_legacy) {
				XSetForeground(display, draw,
				    getcolorpixel(r->s, fg_type, fg));
				DRAWSTRING(display, r->bar->buffer, bar_fs,
				    draw, xpos, bar_border_width +
				    (bar_fs_extents->max_logical_extent.height
				    - bsect[i].height) / 2 - bsect[i].ypos,
				    frag->text, frag->length);
			} else {
				xf = r->s->bar_xftfonts[fn];
				XftDrawStringUtf8(xft_draw, getcolorxft(r->s,
				    fg_type, fg), xf, xpos, bar_border_width +
				    (HEIGHT(r->bar) + xf->height) / 2
				    - xf->descent, (FcChar8 *)frag->text,
				    frag->length);
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
	    WIDTH(r->bar) + 2 * bar_border_width,
	    HEIGHT(r->bar) + 2 * bar_border_width);
}

static void
bar_extra_stop(void)
{
	if (bar_pid) {
		kill(bar_pid, SIGTERM);
		bar_pid = 0;
	}
	strlcpy(bar_ext, "", sizeof bar_ext);
	bar_extra = false;
}

static void
bar_window_class(char *s, size_t sz, struct ws_win *win, size_t *n)
{
	if (win && win->ch.class_name)
		bar_strlcat_esc(s, win->ch.class_name, sz, n);
}

static void
bar_window_instance(char *s, size_t sz, struct ws_win *win, size_t *n)
{
	if (win && win->ch.instance_name)
		bar_strlcat_esc(s, win->ch.instance_name, sz, n);
}

static void
bar_window_class_instance(char *s, size_t sz, struct ws_win *win, size_t *n)
{
	if (win) {
		bar_window_class(s, sz, win, n);
		strlcat(s, ":", sz);
		bar_window_instance(s, sz, win, n);
	}
}

static void
bar_window_state(char *s, size_t sz, struct ws_win *win)
{
	if (win) {
		if (MAXIMIZED(win))
			strlcpy(s, focus_mark_maximized, sz);
		else if (win_free(win))
			strlcpy(s, focus_mark_free, sz);
		else if (win_floating(win) && !FULLSCREEN(win))
			strlcpy(s, focus_mark_floating, sz);
		else
			strlcpy(s, focus_mark_normal, sz);
	} else
		strlcpy(s, focus_mark_none, sz);
}

static void
bar_window_name(char *s, size_t sz, struct ws_win *win, size_t *n)
{
	char		*title;

	if (win && win->mapped) {
		title = get_win_name(win->id);
		bar_strlcat_esc(s, title, sz, n);
		free(title);
	}
}

static void
get_wm_normal_hints(struct ws_win *win)
{
	xcb_icccm_get_wm_normal_hints_reply(conn,
	    xcb_icccm_get_wm_normal_hints(conn, win->id),
	    &win->sh, NULL);
}

/* Get/refresh current WM_HINTS on a window. */
static void
get_wm_hints(struct ws_win *win)
{
	xcb_icccm_get_wm_hints_reply(conn,
	    xcb_icccm_get_wm_hints(conn, win->id),
	    &win->hints, NULL);
}

/* Get/refresh WM_TRANSIENT_FOR on a window. */
static bool
get_wm_transient_for(struct ws_win *win)
{
	xcb_window_t		trans;

	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));
	if (xcb_icccm_get_wm_transient_for_reply(conn,
	    xcb_icccm_get_wm_transient_for(conn, win->id), &trans, NULL)) {
		if (win->transient_for != trans) {
			win->transient_for = trans;
			win->parent = find_window(win->transient_for);
			if (win->parent == win)
				win->parent = NULL;
			DNPRINTF(SWM_D_PROP, "transient_for: %#x, "
			    "parent: %#x\n", trans, WINID(win->parent));
			return (true);
		}
	}

	return (false);
}

static void
clear_attention(struct ws_win *win)
{
	if (!DEMANDS_ATTENTION(win))
		return;

	win->ewmh_flags &= ~EWMH_F_DEMANDS_ATTENTION;
	ewmh_update_wm_state(win);
}

static void
set_attention(struct ws_win *win)
{
	if (DEMANDS_ATTENTION(win))
		return;

	win->ewmh_flags |= EWMH_F_DEMANDS_ATTENTION;
	ewmh_update_wm_state(win);
}

static bool
win_urgent(struct ws_win *win)
{
	return (xcb_icccm_wm_hints_get_urgency(&win->hints) != 0 ||
	    DEMANDS_ATTENTION(win));
}

static void
bar_urgent(struct swm_screen *s, char *str, size_t sz)
{
	struct workspace	*ws;
	struct ws_win		*win;
	int			i;
	bool			urgent;
	char			b[13];

	ws = RB_MIN(workspace_tree, &s->workspaces);
	for (i = 0; i < workspace_limit; i++) {
		if (bar_workspace_limit > 0 && i >= bar_workspace_limit)
			break;

		while (ws && ws->idx < i)
			ws = RB_NEXT(workspace_tree, &s->workspaces, ws);

		urgent = false;
		if (ws && ws->idx == i) {
			TAILQ_FOREACH(win, &ws->winlist, entry)
				if (win_urgent(win)) {
					urgent = true;
					break;
				}
		}

		if (urgent) {
			snprintf(b, sizeof b, "%d ", i + 1);
			strlcat(str, b, sz);
		} else if (!urgent_collapse)
			strlcat(str, "- ", sz);
	}
	if (urgent_collapse && str[0])
		str[strlen(str) - 1] = '\0';
}

static void
bar_workspace_indicator(char *s, size_t sz, struct swm_region *r)
{
	struct ws_win		*w;
	struct workspace	*ws;
	int		 	 count = 0, i;
	char			 tmp[SWM_BAR_MAX], *mark, *suffix, *name;
	bool			 current, active, urgent, collapse;

	if (r == NULL)
		return;

	ws = RB_MIN(workspace_tree, &r->s->workspaces);
	for (i = 0; i < workspace_limit; i++) {
		if (bar_workspace_limit > 0 && i >= bar_workspace_limit)
			break;

		while (ws && ws->idx < i)
			ws = RB_NEXT(workspace_tree, &s->workspaces, ws);

		urgent = false;

		if (ws && ws->idx == i) {
			current = (ws == r->ws);
			active = (TAILQ_FIRST(&ws->winlist) != NULL);
			name = ws->name;

			/* Get urgency status if needed. */
			if (workspace_indicator & SWM_WSI_LISTURGENT ||
			    workspace_indicator & SWM_WSI_MARKURGENT)
				TAILQ_FOREACH(w, &ws->winlist, entry)
					if ((urgent = win_urgent(w)))
						break;
		} else {
			current = false;
			active = false;
			name = NULL;
		}

		collapse = !(workspace_indicator & SWM_WSI_MARKCURRENT ||
		    workspace_indicator & SWM_WSI_MARKURGENT);

		if (!(current && workspace_indicator & SWM_WSI_HIDECURRENT) &&
		    ((current && workspace_indicator & SWM_WSI_LISTCURRENT) ||
		    (active && workspace_indicator & SWM_WSI_LISTACTIVE) ||
		    (!active && workspace_indicator & SWM_WSI_LISTEMPTY) ||
		    (urgent && workspace_indicator & SWM_WSI_LISTURGENT) ||
		    (name && workspace_indicator & SWM_WSI_LISTNAMED))) {
			if (count > 0)
				strlcat(s, " ", sz);

			if (current &&
			    workspace_indicator & SWM_WSI_MARKCURRENT) {
				mark = workspace_mark_current;
				suffix = workspace_mark_current_suffix;
			} else if (urgent &&
			    workspace_indicator & SWM_WSI_MARKURGENT) {
				mark = workspace_mark_urgent;
				suffix = workspace_mark_urgent_suffix;
			} else if (active &&
			    workspace_indicator & SWM_WSI_MARKACTIVE) {
				mark = workspace_mark_active;
				suffix = workspace_mark_active_suffix;
			} else if (!active &&
			    workspace_indicator & SWM_WSI_MARKEMPTY) {
				mark = workspace_mark_empty;
				suffix = workspace_mark_empty_suffix;
			} else if (!collapse) {
				mark = " ";
				suffix = NULL;
			} else {
				mark = NULL;
				suffix = NULL;
			}

			if (mark)
				strlcat(s, mark, sz);

			*tmp = '\0';
			if (name && workspace_indicator & SWM_WSI_PRINTNAMES) {
				if (workspace_indicator & SWM_WSI_NOINDEXES)
					snprintf(tmp, sizeof tmp, "%s", name);
				else
					snprintf(tmp, sizeof tmp, "%d:%s",
					    i + 1, name);
			} else if (workspace_indicator & SWM_WSI_NOINDEXES)
				snprintf(tmp, sizeof tmp, "%s", " ");
			else
				snprintf(tmp, sizeof tmp, "%d", i + 1);
			strlcat(s, tmp, sz);
			if (suffix)
				strlcat(s, suffix, sz);
			count++;
		}
	}
}

static void
bar_workspace_name(char *s, size_t sz, struct workspace *ws, size_t *n)
{
	if (ws && ws->name)
		bar_strlcat_esc(s, ws->name, sz, n);
}

/* build the default bar format according to the defined enabled options */
static void
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
			if (HIDDEN(w)) {
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

static void
bar_replace_pad(char *tmp, size_t *limit, size_t sz)
{
	/* special case; no limit given, pad one space, instead */
	if (*limit == sz - 1)
		*limit = 1;
	snprintf(tmp, sz, "%*s", *(int *)limit, " ");
}

/* replaces the bar format character sequences (like in tmux(1)) */
static char *
bar_replace_seq(char *fmt, char *fmtrep, struct swm_region *r, size_t *offrep,
    size_t sz)
{
	struct ws_win		*w, *cfw;
	char			*ptr, *cur = fmt;
	char			tmp[SWM_BAR_MAX];
	int			size, num;
	size_t			len, limit;
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
	if (sscanf(cur, "%d%n", &num, &size) == 1 && num > 0 &&
	    (size_t)num < sizeof tmp)
		limit = num;
	else
		limit = sizeof tmp - 1;

	cur += size;

	/* determine if post padding is requested */
	if (*cur == '_') {
		post_padding = 1;
		cur++;
	}

	if (r->s->focus && win_free(r->s->focus) && r->s->r_focus == r)
		cfw = r->s->focus;
	else
		cfw = r->ws->focus;

	/* character sequence */
	switch (*cur) {
	case '+':
		strlcpy(tmp, "++", sizeof tmp);
		limit++;
		break;
	case '<':
		bar_replace_pad(tmp, &limit, sizeof tmp);
		break;
	case 'A':
		if (bar_action_expand)
			snprintf(tmp, sizeof tmp, "%s", bar_ext);
		else
			bar_strlcat_esc(tmp, bar_ext, sizeof tmp, &limit);
		break;
	case 'C':
		bar_window_class(tmp, sizeof tmp, cfw, &limit);
		break;
	case 'D':
		bar_workspace_name(tmp, sizeof tmp, r->ws, &limit);
		break;
	case 'F':
		bar_window_state(tmp, sizeof tmp, cfw);
		break;
	case 'I':
		snprintf(tmp, sizeof tmp, "%d", r->ws->idx + 1);
		break;
	case 'L':
		bar_workspace_indicator(tmp, sizeof tmp, r);
		break;
	case 'M':
		num = 0;
		TAILQ_FOREACH(w, &r->ws->winlist, entry)
			if (HIDDEN(w))
				++num;
		snprintf(tmp, sizeof tmp, "%d", num);
		break;
	case 'N':
		snprintf(tmp, sizeof tmp, "%d", r->s->idx + 1);
		break;
	case 'P':
		bar_window_class_instance(tmp, sizeof tmp, cfw, &limit);
		break;
	case 'R':
		snprintf(tmp, sizeof tmp, "%d", get_region_index(r));
		break;
	case 'S':
		snprintf(tmp, sizeof tmp, "%s", r->ws->stacker);
		break;
	case 'T':
		bar_window_instance(tmp, sizeof tmp, cfw, &limit);
		break;
	case 'U':
		bar_urgent(r->s, tmp, sizeof tmp);
		break;
	case 'V':
		snprintf(tmp, sizeof tmp, "%s", bar_vertext);
		break;
	case 'w':
		num = 0;
		TAILQ_FOREACH(w, &r->ws->winlist, entry)
			++num;
		snprintf(tmp, sizeof tmp, "%d", num);
		break;
	case 'W':
		bar_window_name(tmp, sizeof tmp, cfw, &limit);
		break;
	default:
		/* Unknown character sequence or EOL; copy as-is. */
		strlcpy(tmp, fmt, cur - fmt + 2);
		break;
	}

	len = strlen(tmp);

	/* calculate the padding lengths */
	padding_len = limit - len;
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

static void
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

static void
bar_strlcat_esc(char *dst, char *src, size_t sz, size_t *n)
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
			(*n)++;
			sz--;
		}
		*dst++ = *src++;
		sz--;
	}
	*dst = '\0';
}

static void
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

static void
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
		bsect = calloc(numsect, sizeof(struct bar_section));
		if (bsect == NULL)
			err(1, "bar_split_format: calloc");
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

static bool
scan_markup(struct swm_screen *s, char *f, int *n, size_t *size)
{
	char	*c = f, *t;

	*size = 0;
	if (*c != '+')
		return false;
	c++;
	if (*c != '@')
		return false;
	c++;
	if ((*c == 'b') && (*(c + 1) == 'g') && (*(c + 2) == '=')) {
		*n = strtol((c + 3), &t, 10);
		*size = t - f + 1;
		if (*t == ';' && *n >= 0 && *n < s->c[SWM_S_COLOR_BAR].count)
			return true;
		else
			return false;
	}

	if ((*c == 'f') && (*(c + 1) == 'g') && (*(c + 2) == '=')) {
		*n = strtol((c + 3), &t, 10);
		*size = t - f + 1;
		if (*t == ';' && *n >= 0 &&
		    *n < s->c[SWM_S_COLOR_BAR_FONT].count)
			return true;
		else
			return false;
	}

	if ((*c == 'f') && (*(c + 1) == 'n') && (*(c + 2) == '=')) {
		if (bar_font_legacy)
			return false;
		*n = strtol((c + 3), &t, 10);
		*size = t - f + 1;
		if (*t == ';' && *n >= 0 && *n < num_xftfonts)
			return true;
		else
			return false;
	}

	if ((*c == 's') && (*(c + 1) == 't') && (*(c + 2) == 'p') &&
	    (*(c + 3) == ';')) {
		*size = 6;
		return true;
	}

	return false;
}

static int
get_character_font(struct swm_screen *s, FcChar32 c, int pref)
{
	int			i;

	if (bar_font_legacy)
		return (0);

	/* Try special font for PUA codepoints. */
	if (font_pua_index && s->bar_xftfonts[font_pua_index] &&
	    ((0xe000 <= c && c <= 0xf8ff) || (0xf0000 <= c && c <= 0xffffd) ||
	    (0x100000 <= c && c <= 0x10fffd)) &&
	    XftCharExists(display, s->bar_xftfonts[font_pua_index], c))
		return (font_pua_index);

	if (pref >= num_xftfonts)
		pref = -1;

	/* Try specified font. */
	if (pref >= 0 && s->bar_xftfonts[pref] &&
	    XftCharExists(display, s->bar_xftfonts[pref], c))
		return (pref);

	/* Search the rest, from the top. */
	for (i = 0; i < num_xftfonts; i++)
		if (i != pref && s->bar_xftfonts[i] &&
		    XftCharExists(display, s->bar_xftfonts[i], c))
			return (i);

	/* Fallback to the specified font, if valid. */
	if (pref >= 0)
		return (pref);

	return (0);
}

static void
bar_parse_markup(struct swm_screen *s, struct bar_section *sect)
{
	XRectangle		ibox, lbox;
	XGlyphInfo		info;
	struct text_fragment	*frag;
	int 			i = 0, len = 0, stop = 0, fmtlen, termfrag = 0;
	int			idx, fn, fg, bg;
	FcChar32		c;
	char			*fmt;
	size_t			sz;

	sect->text_width = 0;
	sect->nfrags = 0;
	frag = sect->frag;
	frag[0].text = sect->fmtrep;
	frag[0].length = 0;
	frag[0].font = fn = 0;
	frag[0].width = 0;
	frag[0].bg = bg = 0;
	frag[0].fg = fg = 0;

	fmt = sect->fmtrep;
	fmtlen = strlen(fmt);

	if (bar_font_legacy) {
		TEXTEXTENTS(bar_fs, fmt, fmtlen, &ibox, &lbox);
		sect->height = lbox.height;
		sect->ypos = lbox.y;
	}

	while (*fmt != '\0') {
		/* Handle markup sequences. */
		if (*fmt == '+' && !stop) {
			if (*(fmt+1) == '@' && scan_markup(s, fmt, &idx, &sz)) {
				if ((*(fmt+2) == 'f') && (*(fmt+3) == 'n'))
					fn = idx;
				else if ((*(fmt+2) == 'f') && (*(fmt+3) == 'g'))
					fg = idx;
				else if ((*(fmt+2) == 'b') && (*(fmt+3) == 'g'))
					bg = idx;
				else if ((*(fmt+2) == 's') && (*(fmt+3) == 't')
				    && (*(fmt+4) == 'p'))
					stop = 1;

				/* Eat markup. */
				fmt += sz;
				fmtlen -= sz;
				termfrag = 1;
				continue;
			} else if (*(fmt+1) == '+') {
				/* Eat escape. */
				fmt++;
				fmtlen--;
				termfrag = 1;
			}
		}

		/* Decode current character. */
		len = FcUtf8ToUcs4((FcChar8 *)fmt, &c, fmtlen);
		if (len <= 0)
			break;

		idx = get_character_font(s, c, fn);
		if (idx != frag[i].font || termfrag) {
			/* Terminate current fragment. */
			if (frag[i].length > 0) {
				if (bar_font_legacy) {
					TEXTEXTENTS(bar_fs, frag[i].text,
					    frag[i].length, &ibox, &lbox);
					frag[i].width = lbox.width;
				} else {
					XftTextExtentsUtf8(display,
					    s->bar_xftfonts[frag[i].font],
					    (FcChar8 *)frag[i].text,
					    frag[i].length, &info);
					frag[i].width = info.xOff;
				}
				sect->text_width += frag[i].width;
				i++;
				if (i == SWM_TEXTFRAGS_MAX)
					break;
			}

			/* Begin new fragment. */
			frag[i].text = fmt;
			frag[i].length = 0;
			frag[i].font = idx;
			frag[i].fg = fg;
			frag[i].bg = bg;
			termfrag = 0;
		}

		frag[i].length += len;
		fmt += len;
		fmtlen -= len;
	}

	if ((frag[i].length > 0) && (i < SWM_TEXTFRAGS_MAX)) {
		/* Process last text fragment */
		if (bar_font_legacy) {
			TEXTEXTENTS(bar_fs, frag[i].text, frag[i].length, &ibox,
			    &lbox);
			frag[i].width = lbox.width;
		} else {
			XftTextExtentsUtf8(display,
			    s->bar_xftfonts[frag[i].font],
			    (FcChar8 *)frag[i].text, frag[i].length, &info);
			frag[i].width = info.xOff;
		}
		sect->text_width += frag[i].width;
		i++;
	}

	sect->nfrags = i;
}

static void
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

static void
update_bars(struct swm_screen *s)
{
	struct swm_region	*r;

	TAILQ_FOREACH(r, &s->rl, entry)
		bar_draw(r->bar);
}

static void
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

	if (!bar_enabled || !r->ws->bar_enabled || r->bar->disabled) {
		xcb_unmap_window(conn, bar->id);
		return;
	}

	xcb_map_window(conn, bar->id);

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
static int
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

static void
bar_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	int			i, num_screens;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	switch (args->id) {
	case SWM_ARG_ID_BAR_TOGGLE_WS:
		if ((r = get_current_region(s))) {
			/* Only change if master switch is enabled. */
			if (bar_enabled)
				r->ws->bar_enabled = !r->ws->bar_enabled;
			else
				bar_enabled = r->ws->bar_enabled = true;
			DNPRINTF(SWM_D_BAR, "ws%d->bar_enabled: %s\n",
			    r->ws->idx, YESNO(bar_enabled));
		}
		break;
	case SWM_ARG_ID_BAR_TOGGLE:
		bar_enabled = !bar_enabled;
		break;
	}

	DNPRINTF(SWM_D_BAR, "bar_enabled: %s\n", YESNO(bar_enabled));

	/* update bars as necessary */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (r->bar) {
				if (bar_enabled && r->ws->bar_enabled) {
					xcb_map_window(conn, r->bar->id);
					bar_draw(r->bar);
				} else
					xcb_unmap_window(conn, r->bar->id);
			}
			stack(r);
			update_mapping(&screens[i]);
		}

	flush();
}

static void
bar_extra_setup(void)
{
	int		fd, bar_pipe[2];

	/* do this here because the conf file is in memory */
	if (!bar_extra && bar_argv[0]) {
		/* launch external status app */
		bar_extra = true;
		if (pipe(bar_pipe) == -1)
			err(1, "pipe error");
		/* Read must not block or spectrwm will hang. */
		socket_setnonblock(bar_pipe[0]);

		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			err(1, "could not disable SIGPIPE");

		switch (bar_pid = fork()) {
		case -1:
			err(1, "cannot fork");
			break;
		case 0: /* child */
			close(bar_pipe[0]);

			/* Isolate stdin. */
			if ((fd = open(_PATH_DEVNULL, O_RDONLY, 0)) == -1) {
				warn("open /dev/null");
				_exit(1);
			}
			if (dup2(fd, STDIN_FILENO) == -1) {
				warn("dup2 stdin");
				_exit(1);
			}
			if (fd > STDERR_FILENO)
				close(fd);

			/* Reassign stdout to write end of pipe. */
			if (dup2(bar_pipe[1], STDOUT_FILENO) == -1) {
				warn("dup2 stdout");
				_exit(1);
			}
			if (bar_pipe[1] > STDERR_FILENO)
				close(bar_pipe[1]);

			execvp(bar_argv[0], bar_argv);
			warn("%s external app failed", bar_argv[0]);
			_exit(1);
			break;
		default: /* parent */
			close(bar_pipe[1]);

			/* Reassign stdin to read end of pipe. */
			if (dup2(bar_pipe[0], STDIN_FILENO) == -1)
				err(1, "dup2");
			if (bar_pipe[0] > STDERR_FILENO)
				close(bar_pipe[0]);
			break;
		}

		atexit(kill_bar_extra_atexit);
	}
}

static void
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

static int
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

	if (bar_fs == NULL) {
		warnx("Error creating font set structure.");
		return (1);
	}

	bar_fs_extents = XExtentsOfFontSet(bar_fs);

	bar_height = bar_fs_extents->max_logical_extent.height +
	    2 * bar_border_width;

	if (bar_height < 1)
		bar_height = 1;

	return (0);
}

static int
xft_init(struct swm_screen *s)
{
	struct swm_color	*c;
	XRenderColor		color;
	int			i;

	DNPRINTF(SWM_D_INIT, "loading bar_fonts: %s\n", bar_fonts);

	if (num_xftfonts == 0 && bar_fontname_pua == NULL)
		return (1);

	if ((s->bar_xftfonts = calloc(num_xftfonts + 1,
	    sizeof(XftFont *))) == NULL)
		err(1, "xft_init: calloc");

	for (i = 0; i < num_xftfonts; i++) {
		s->bar_xftfonts[i] = XftFontOpenName(display, s->idx,
		    bar_fontnames[i]);
		if (s->bar_xftfonts[i] == NULL)
			warnx("unable to load font %s", bar_fontnames[i]);
	}

	font_pua_index = 0;
	if (bar_fontname_pua) {
		s->bar_xftfonts[num_xftfonts] = XftFontOpenName(display, s->idx,
		    bar_fontname_pua);
		if (s->bar_xftfonts[num_xftfonts] == NULL)
			warnx("unable to load font %s", bar_fontname_pua);
		else
			font_pua_index = num_xftfonts;
	}

	for (i = 0; i < s->c[SWM_S_COLOR_BAR_FONT].count; i++) {
		c = s->c[SWM_S_COLOR_BAR_FONT].colors[i];
		SWM_TO_XRENDER_COLOR(*c, color);
		if (!XftColorAllocValue(display, s->xvisual, s->colormap,
		    &color, &c->xft_color))
			warnx("Xft error: unable to allocate color.");
	}

	for (i = 0; i < s->c[SWM_S_COLOR_BAR_FONT_FREE].count; i++) {
		c = s->c[SWM_S_COLOR_BAR_FONT_FREE].colors[i];
		SWM_TO_XRENDER_COLOR(*c, color);
		if (!XftColorAllocValue(display, s->xvisual, s->colormap,
		    &color, &c->xft_color))
			warnx("Xft error: unable to allocate color.");
	}

	for (i = 0; i < s->c[SWM_S_COLOR_BAR_FONT_UNFOCUS].count; i++) {
		c = s->c[SWM_S_COLOR_BAR_FONT_UNFOCUS].colors[i];
		SWM_TO_XRENDER_COLOR(*c, color);
		if (!XftColorAllocValue(display, s->xvisual, s->colormap,
		    &color, &c->xft_color))
			warnx("Xft error: unable to allocate color.");
	}

	if (s->c[SWM_S_COLOR_BAR].count > 0) {
		c = s->c[SWM_S_COLOR_BAR].colors[0];
		SWM_TO_XRENDER_COLOR(*c, color);
		if (!XftColorAllocValue(display, s->xvisual, s->colormap,
		    &color, &c->xft_color))
			warnx("Xft error: unable to allocate color.");
	}

	if (s->bar_xftfonts[0] == NULL)
		return (1);

	bar_height = s->bar_xftfonts[0]->height + 2 * bar_border_width;
	if (bar_height < 1)
		bar_height = 1;

	return (0);
}

static void
setup_fonts(void)
{
	int	i, num_screens;
	int	fail = 0;
	bool	custom = false;
	size_t	len;
	char	*buf, *b, *sp;

	/* Process bar_fonts. */
	if (bar_fonts && bar_fonts[0] != '\0')
		custom = true;
	else {
		/* Use defaults. */
		free(bar_fonts);
		if ((bar_fonts = strdup(SWM_BAR_FONTS)) == NULL)
			err(1, "setup_fonts: strdup");
	}

	DNPRINTF(SWM_D_CONF, "custom: %s\n", YESNO(custom));

	len = strlen(bar_fonts) + 1;
	if ((buf = malloc(len)) == NULL)
		err(1, "setup_fonts: calloc");

	/* If all entries are XLFD, use legacy mode. */
	memcpy(buf, bar_fonts, len);
	bar_font_legacy = true;
	sp = buf;
	while ((b = strsep(&sp, SWM_CONF_DELIMLIST)) != NULL) {
		if (*b == '\0')
			continue;
		if (!isxlfd(b)) {
			bar_font_legacy = false;
			break;
		}
	}

	/* Otherwise, use Xft mode and parse list into array. */
	if (!bar_font_legacy) {
		/* Get count. */
		memcpy(buf, bar_fonts, len);
		sp = buf;
		while ((b = strsep(&sp, SWM_CONF_DELIMLIST)) != NULL) {
			if (*b == '\0')
				continue;
			num_xftfonts++;
		}

		DNPRINTF(SWM_D_CONF, "num_xftfonts: %d\n", num_xftfonts);

		/* Extra slot is for PUA font. */
		bar_fontnames = calloc(num_xftfonts + 1, sizeof(char *));
		if (bar_fontnames == NULL)
			err(1, "setup_fonts: calloc");

		memcpy(buf, bar_fonts, len);
		sp = buf;
		i = 0;
		while ((b = strsep(&sp, SWM_CONF_DELIMLIST)) != NULL) {
			if (*b == '\0')
				continue;
			if ((bar_fontnames[i++] = strdup(b)) == NULL)
				err(1, "setup_fonts: strdup");
		}
	}
	free(buf);

	DNPRINTF(SWM_D_CONF, "legacy: %s, bar_fonts: %s\n",
	    YESNO(bar_font_legacy), bar_fonts);

	if (bar_font_legacy)
		fail = fontset_init();
	else {
		num_screens = get_screen_count();
		for (i = 0; i < num_screens; i++)
			if ((fail = xft_init(&screens[i])))
				break;
	}

	if (fail) {
		warnx("Failed to load bar_font. Switching to fallback.");
		if (custom)
			add_startup_exception("Error loading bar_font: %s",
			    bar_fonts);
		free(bar_fonts);
		if ((bar_fonts = strdup(SWM_BAR_FONTS_FALLBACK)) == NULL)
			err(1, "setup_fonts: strdup");

		bar_font_legacy = true;
		if (fontset_init())
			errx(1, "Failed to load a font.");
	}
}

static void
bar_setup(struct swm_region *r)
{
	struct swm_screen	*s;
	struct swm_region	*rfirst;
	uint32_t	 	wa[4];
	size_t			len;
	char			*name;

	if (r == NULL || r->bar)
		return;

	s = r->s;
	DNPRINTF(SWM_D_BAR, "screen %d, region %d\n", s->idx,
	    get_region_index(r));

	if ((r->bar = calloc(1, sizeof(struct swm_bar))) == NULL)
		err(1, "bar_setup: bar calloc");

	if ((r->bar->st = calloc(1, sizeof(struct swm_stackable))) == NULL)
		err(1, "bar_setup: st calloc");

	r->bar->st->type = STACKABLE_BAR;
	r->bar->st->bar = r->bar;
	r->bar->st->layer = SWM_LAYER_BAR;
	r->bar->st->s = s;

	r->bar->r = r;
	X(r->bar) = X(r) + bar_border_width;
	Y(r->bar) = bar_at_bottom ? (Y(r) + HEIGHT(r) - bar_height +
	    bar_border_width) : Y(r) + bar_border_width;
	WIDTH(r->bar) = WIDTH(r) - 2 * bar_border_width;
	HEIGHT(r->bar) = bar_height - 2 * bar_border_width;
	r->bar->disabled = false;

	/* Assume region is unfocused when we create the bar. */
	r->bar->id = xcb_generate_id(conn);
	wa[0] = getcolorpixel(s, SWM_S_COLOR_BAR_UNFOCUS, 0);
	wa[1] = getcolorpixel(s, SWM_S_COLOR_BAR_BORDER_UNFOCUS, 0);
	wa[2] = XCB_EVENT_MASK_BUTTON_PRESS |
	    XCB_EVENT_MASK_BUTTON_RELEASE |
	    XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_LEAVE_WINDOW |
	    XCB_EVENT_MASK_POINTER_MOTION |
	    XCB_EVENT_MASK_POINTER_MOTION_HINT |
	    XCB_EVENT_MASK_EXPOSURE |
	    XCB_EVENT_MASK_FOCUS_CHANGE;
	wa[3] = s->colormap;

	xcb_create_window(conn, s->depth, r->bar->id, s->root,
	    X(r->bar) - bar_border_width, Y(r->bar) - bar_border_width,
	    WIDTH(r->bar) + 2 * bar_border_width,
	    HEIGHT(r->bar) + 2 * bar_border_width, 0,
	    XCB_WINDOW_CLASS_INPUT_OUTPUT, s->visual, XCB_CW_BACK_PIXEL |
	    XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP, wa);

	/* Set class and title to make the bar window identifiable. */
	xcb_icccm_set_wm_class(conn, r->bar->id,
	    strlen(SWM_WM_CLASS_BAR) + strlen(SWM_WM_CLASS_INSTANCE) + 2,
	    SWM_WM_CLASS_BAR "\0" SWM_WM_CLASS_INSTANCE "\0");

	if (asprintf(&name, "Status Bar - Region %d - " SWM_WM_CLASS_INSTANCE,
	    get_region_index(r)) == -1)
		err(1, "bar_setup: asprintf");
	len = strlen(name);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, r->bar->id,
	    XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, len, name);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, r->bar->id,
	    ewmh[_NET_WM_NAME].atom, a_utf8_string, 8, len, name);
	free(name);

	rfirst = TAILQ_FIRST(&s->rl);
	if (rfirst == r) {
		wa[0] = r->id;
		wa[1] = XCB_STACK_MODE_ABOVE;
	} else {
		wa[0] = rfirst->bar->id;
		wa[1] = XCB_STACK_MODE_BELOW;
	}
	xcb_configure_window(conn, r->bar->id, XCB_CONFIG_WINDOW_SIBLING |
	    XCB_CONFIG_WINDOW_STACK_MODE, wa);

	r->bar->buffer = xcb_generate_id(conn);
	xcb_create_pixmap(conn, s->depth, r->bar->buffer, r->bar->id,
	    WIDTH(r->bar) + 2 * bar_border_width,
	    HEIGHT(r->bar) + 2 * bar_border_width);

	if (bar_enabled)
		xcb_map_window(conn, r->bar->id);

	DNPRINTF(SWM_D_BAR, "win %#x, (x,y) w x h: (%d,%d) %d x %d\n",
	    WINID(r->bar), X(r->bar), Y(r->bar), WIDTH(r->bar), HEIGHT(r->bar));

	bar_extra_setup();
}

static void
free_stackable(struct swm_stackable *st) {
	struct swm_stackable	*sst;

	if (st == NULL)
		return;

	SLIST_FOREACH(sst, &st->s->stack, entry)
		if (sst == st)
			SLIST_REMOVE(&st->s->stack, st, swm_stackable, entry);
	free(st);
}

static void
bar_cleanup(struct swm_region *r)
{
	if (r->bar == NULL)
		return;

	xcb_destroy_window(conn, r->bar->id);
	xcb_free_pixmap(conn, r->bar->buffer);
	free_stackable(r->bar->st);
	free(r->bar);
	r->bar = NULL;
}

static void
setup_marks(void)
{
	struct workspace	*ws;
	int			i, num_screens;
	size_t			mlen, len;

	/* Allocate stacking indicator buffers for longest mark. */
	if (verbose_layout)
		mlen = SWM_FANCY_MAXLEN;
	else {
		mlen = strlen(stack_mark_max);
		if ((len = strlen(stack_mark_vertical)) > mlen)
			mlen = len;
		if ((len = strlen(stack_mark_vertical_flip)) > mlen)
			mlen = len;
		if ((len = strlen(stack_mark_horizontal)) > mlen)
			mlen = len;
		if ((len = strlen(stack_mark_horizontal_flip)) > mlen)
			mlen = len;
		if ((len = strlen(stack_mark_floating)) > mlen)
			mlen = len;
		mlen++; /* null byte. */
	}

	stack_mark_maxlen = mlen;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		RB_FOREACH(ws, workspace_tree, &screens[i].workspaces) {
			if (ws_root(ws))
				continue;

			free(ws->stacker);
			if ((ws->stacker = calloc(mlen, sizeof(char))) == NULL)
				err(1, "setup_marks: calloc");
			ws->stacker[0] = '\0';
			ws->stacker_len = mlen;
		}
}

static void
set_win_state(struct ws_win *win, uint32_t state)
{
	uint32_t		data[2] = { state, XCB_WINDOW_NONE };

	DNPRINTF(SWM_D_EVENT, "win %#x, state: %s(%u)\n", WINID(win),
	    get_wm_state_label(state), state);

	if (win == NULL)
		return;

	win->state = state;
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id, a_state,
	    a_state, 32, 2, data);
}

static uint32_t
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

	return (result);
}

static void
version(struct swm_screen *s, struct binding *bp, union arg *args)
{
	int		i, num_screens;

	/* Suppress warning. */
	(void)s;
	(void)bp;
	(void)args;

	bar_version = !bar_version;
	if (bar_version)
		snprintf(bar_vertext, sizeof bar_vertext,
		    "Version: %s Build: %s", SPECTRWM_VERSION, buildstr);
	else
		strlcpy(bar_vertext, "", sizeof bar_vertext);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		update_bars(&screens[i]);
	xcb_flush(conn);
}

static void
client_msg(struct ws_win *win, xcb_atom_t a, xcb_timestamp_t t)
{
	xcb_client_message_event_t	ev;

	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, atom: %s(%u), time: %#x\n",
	    win->id, get_atom_label(a), a, t);

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
static void
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

static int
count_win(struct workspace *ws, uint32_t flags)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (!(flags & SWM_COUNT_ICONIC) && HIDDEN(win))
			continue;
		if (!(flags & SWM_COUNT_DESKTOP) && WINDESKTOP(win))
			continue;
		if (!(flags & SWM_COUNT_TILED) && !win_floating(win))
			continue;
		if (!(flags & SWM_COUNT_FLOATING) && win_floating(win))
			continue;
		count++;
	}

	return (count);
}

static void
quit(struct swm_screen *s, struct binding *bp, union arg *args)
{
	/* Suppress warning. */
	(void)s;
	(void)bp;
	(void)args;

	DNPRINTF(SWM_D_MISC, "shutting down...\n");
	running = 0;
}

static void
prioritize_window(struct ws_win *win)
{

	DNPRINTF(SWM_D_STACK, "win %#x\n", win->id);
	TAILQ_REMOVE(&win->s->priority, win, priority_entry);
	TAILQ_INSERT_HEAD(&win->s->priority, win, priority_entry);
}

static void
clear_stack(struct swm_screen *s)
{
	while (!SLIST_EMPTY(&s->stack))
		SLIST_REMOVE_HEAD(&s->stack, entry);
}

/* Updates current stacking order with all rules/priorities/etc. */
static void
refresh_stack(struct swm_screen *s)
{
	struct swm_region	*r;
	struct ws_win		*w;
	enum swm_layer		layer;

	DNPRINTF(SWM_D_STACK, "rebuilding stack on screen %d\n", s->idx);

	/* Rebuild from scratch for now. */
	clear_stack(s);

	/* Start from the top. */
	for (layer = SWM_LAYER_INVALID; layer; layer--) {
		switch (layer - 1) {
		case SWM_LAYER_REGION:
			/* Region input windows. */
			TAILQ_FOREACH(r, &s->rl, entry)
				SLIST_INSERT_HEAD(&s->stack, r->st, entry);
			break;
		case SWM_LAYER_BAR:
			/* Status bar windows. */
			TAILQ_FOREACH(r, &s->rl, entry)
				SLIST_INSERT_HEAD(&s->stack, r->bar->st, entry);
			break;
		case SWM_LAYER_TILED:
			/* Windows by recent focus. */
			TAILQ_FOREACH(w, &s->fl, focus_entry)
				if (w->st->layer == layer - 1)
					SLIST_INSERT_HEAD(&s->stack, w->st,
					    entry);
			break;
		default:
			/* Windows by priority. */
			TAILQ_FOREACH(w, &s->priority, priority_entry)
				if (w->st->layer == layer - 1)
					SLIST_INSERT_HEAD(&s->stack,
					    w->st, entry);
			break;
		}
	}
}

static void
update_win_layer_related(struct ws_win *win)
{
	struct ws_win		*w;

	/* Update main window first. */
	update_win_layer(win->main);
	TAILQ_FOREACH(w, &win->ws->winlist, entry)
		if (win_related(w, win))
			update_win_layer(w);
}

/* Caution: update main windows first before descendents. */
static void
update_win_layer(struct ws_win *win)
{
	enum swm_layer	layer;

	if (win_raised(win))
		layer = SWM_LAYER_RAISED;
	else if (WINDESKTOP(win))
		layer = SWM_LAYER_DESKTOP;
	else if (win_below(win))
		layer = SWM_LAYER_BELOW;
	else if (FULLSCREEN(win))
		layer = SWM_LAYER_FULLSCREEN;
	else if (MAXIMIZED(win))
		layer = SWM_LAYER_MAXIMIZED;
	else if (WINDOCK(win))
		layer = SWM_LAYER_DOCK;
	else if (win_floating(win))
		layer = SWM_LAYER_ABOVE;
	else
		layer = SWM_LAYER_TILED;

	/* Keep descendents above main. */
	if (!win_main(win) && layer < win->main->st->layer)
		layer = win->main->st->layer;

	DNPRINTF(SWM_D_MISC, "win 0x%08x, layer %d->%d\n", win->id,
	    win->st->layer, layer);
	win->st->layer = layer;
}

static void
update_stackable(struct swm_stackable *st, struct swm_stackable *st_sib)
{
	uint32_t		val[2];

	if (st == NULL)
		return;

	val[0] = st_sib ? st_window_id(st_sib) : st->s->swmwin;
	val[1] = XCB_STACK_MODE_ABOVE;

	DNPRINTF(SWM_D_STACK, "win:%#x sibling:%#x\n",
	    st_window_id(st), val[0]);

	xcb_configure_window(conn, st_window_id(st),
	    XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE, val);
}

static void
map_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, mapped: %s\n",
	    win->id, YESNO(win->mapped));

	if (!win_reparented(win)) {
		DNPRINTF(SWM_D_EVENT, "skip win %#x; not reparented\n",
		    win->id);
		return;
	}

	if (win->mapped)
		return;

	xcb_map_window(conn, win->frame);
	xcb_map_window(conn, win->id);
	if (win->debug != XCB_WINDOW_NONE)
		xcb_map_window(conn, win->debug);
	win->mapping += 2;
	win->mapped = true;
	set_win_state(win, XCB_ICCCM_WM_STATE_NORMAL);
}

static void
unmap_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, mapped: %s\n", win->id,
	    YESNO(win->mapped));

	if (!win_reparented(win)) {
		DNPRINTF(SWM_D_EVENT, "skip win %#x; not reparented\n",
		    win->id);
		return;
	}

	if (!win->mapped)
		return;

	if (win->debug != XCB_WINDOW_NONE)
		xcb_unmap_window(conn, win->debug);
	xcb_unmap_window(conn, win->id);
	xcb_unmap_window(conn, win->frame);
	win->unmapping += 2;
	win->mapped = false;
	set_win_state(win, XCB_ICCCM_WM_STATE_ICONIC);
}

static void
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

static void
restart(struct swm_screen *s, struct binding *bp, union arg *args)
{
	/* Suppress warning. */
	(void)s;
	(void)bp;

	DNPRINTF(SWM_D_MISC, "%s\n", start_argv[0]);

	shutdown_cleanup();

	if (args && args->id == SWM_ARG_ID_RESTARTOFDAY)
		unsetenv("SWM_STARTED");

	execvp(start_argv[0], start_argv);
	warn("execvp failed");
	quit(NULL, NULL, NULL);
}

static bool
follow_pointer(struct swm_screen *s, unsigned int type)
{
	bool	result;

	result = (follow_mode(type) && s->r_focus &&
	    !ws_maponfocus(s->r_focus->ws) &&
	    s->r_focus == get_pointer_region(s));
	DNPRINTF(SWM_D_FOCUS, "%s\n", YESNO(result));

	return (result);
}

static struct swm_region *
get_pointer_region(struct swm_screen *s)
{
	struct swm_region		*r = NULL;
	struct ws_win			*w;
	xcb_query_pointer_reply_t	*qpr;

	if (s == NULL)
		return (NULL);

	qpr = xcb_query_pointer_reply(conn,
	    xcb_query_pointer(conn, s->root), NULL);
	if (qpr) {
		w = find_window(qpr->child);
		if (w && !win_free(w) && w->ws->r) {
			pointer_window = qpr->child;
			r = w->ws->r;
		} else {
			DNPRINTF(SWM_D_MISC, "pointer: (%d,%d)\n", qpr->root_x,
			    qpr->root_y);
			r = region_under(s, qpr->root_x, qpr->root_y);
		}
		free(qpr);
	}

	return (r);
}

static struct ws_win *
get_pointer_win(struct swm_screen *s)
{
	struct ws_win			*win = NULL;
	xcb_query_pointer_reply_t	*qpr;

	qpr = xcb_query_pointer_reply(conn,
	    xcb_query_pointer(conn, s->root), NULL);
	if (qpr) {
		win = find_window(qpr->child);
		if (win)
			pointer_window = qpr->child;
		free(qpr);
	}

	return (win);
}

static void
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
		if (gcpr) {
			/* XIWarpPointer takes FP1616. */
			xcb_input_xi_warp_pointer(conn, XCB_NONE, dwinid, 0, 0,
			    0, 0, dx << 16, dy << 16, gcpr->deviceid);
			free(gcpr);
		}
	} else {
#endif
		xcb_warp_pointer(conn, XCB_NONE, dwinid, 0, 0, 0, 0, dx, dy);
#ifdef SWM_XCB_HAS_XINPUT
	}
#endif
}

static xcb_window_t
get_input_focus(void)
{
	xcb_window_t			id = XCB_WINDOW_NONE;
	xcb_get_input_focus_reply_t	*gifr;

	gifr = xcb_get_input_focus_reply(conn,
	    xcb_get_input_focus(conn), NULL);
	if (gifr) {
		if (gifr->focus != XCB_INPUT_FOCUS_POINTER_ROOT)
			id = gifr->focus;
		free(gifr);
	}

	return (id);
}

static struct swm_region *
get_current_region(struct swm_screen *s)
{
	struct swm_region		*r;
	struct ws_win			*w;

	if (s == NULL)
		errx(1, "missing screen.");

	/* 1. Focused region */
	r = s->r_focus;
	if (r == NULL) {
		/* 2. Region of window with input focus. */
		w = find_window(get_input_focus());
		if (w && w->ws)
			r = w->ws->r;

		if (r == NULL) {
			/* 3. Region with pointer. */
			r = get_pointer_region(s);
			if (r == NULL)
				/* 4. Default. */
				r = TAILQ_FIRST(&s->rl);
		}
	}

	DNPRINTF(SWM_D_MISC, "idx: %d\n", get_region_index(r));

	return (r);
}

static struct swm_region *
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

static struct swm_screen *
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

static struct swm_bar *
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

static struct ws_win *
find_window(xcb_window_t id)
{
	struct ws_win		*win = NULL;
	int			i, num_screens;
	xcb_query_tree_reply_t	*qtr;

	DNPRINTF(SWM_D_MISC, "id: %#x\n", id);

	if (id == XCB_WINDOW_NONE)
		return (NULL);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(win, &screens[i].managed, manage_entry)
			if (id == win->id || id == win->frame)
				return (win);

	/* If window isn't top-level, try to find managed ancestor. */
	qtr = xcb_query_tree_reply(conn, xcb_query_tree(conn, id), NULL);
	if (qtr) {
		if (qtr->parent != XCB_WINDOW_NONE && qtr->parent != qtr->root)
			win = find_window(qtr->parent);

		if (win)
			DNPRINTF(SWM_D_MISC, "%#x is descendent of %#x.\n",
			    id, qtr->parent);

		free(qtr);
	}

	if (win == NULL)
		DNPRINTF(SWM_D_MISC, "unmanaged.\n");

	return (win);
}

static void
spawn(int ws_idx, union arg *args, unsigned int flags)
{
	int			fd;
	char			*ret = NULL;

	if (args == NULL || args->argv[0] == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "ws:%d f:%#x %s\n", ws_idx, flags, args->argv[0]);

	close(xcb_get_file_descriptor(conn));

	if (flags & (SWM_SPAWN_WS | SWM_SPAWN_PID | SWM_SPAWN_XTERM_FONTADJ)) {
		if ((ret = getenv("LD_PRELOAD"))) {
			if (asprintf(&ret, "%s:%s", SWM_LIB, ret) == -1) {
				warn("spawn: asprintf LD_PRELOAD");
				_exit(1);
			}
			setenv("LD_PRELOAD", ret, 1);
			free(ret);
			ret = NULL;
		} else {
			setenv("LD_PRELOAD", SWM_LIB, 1);
		}

		if (flags & SWM_SPAWN_WS) {
			if (asprintf(&ret, "%d", ws_idx) == -1) {
				warn("spawn: asprintf SWM_WS");
				_exit(1);
			}
			setenv("_SWM_WS", ret, 1);
			free(ret);
			ret = NULL;
		}

		if (flags & SWM_SPAWN_PID) {
			if (asprintf(&ret, "%d", getpid()) == -1) {
				warn("spawn: asprintf _SWM_PID");
				_exit(1);
			}
			setenv("_SWM_PID", ret, 1);
			free(ret);
			ret = NULL;
		}

		if (flags & SWM_SPAWN_XTERM_FONTADJ)
			setenv("_SWM_XTERM_FONTADJ", "", 1);
	}

	if (setsid() == -1) {
		warn("spawn: setsid");
		_exit(1);
	}

	if (flags & SWM_SPAWN_CLOSE_FD) {
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

/* Cleanup all traces of a (possibly invalid) window pointer. */
static void
kill_refs(struct ws_win *win)
{
	struct workspace	*ws;
	struct ws_win		*w;
	int			i, num_screens;

	if (win == NULL)
		return;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		if (screens[i].focus == win)
			screens[i].focus = NULL;

		RB_FOREACH(ws, workspace_tree, &screens[i].workspaces) {
			if (win == ws->focus)
				ws->focus = NULL;
			if (win == ws->focus_raise)
				ws->focus_raise = NULL;
		}
		TAILQ_FOREACH(w, &screens[i].managed, manage_entry) {
			if (win == w->focus_redirect)
				w->focus_redirect = NULL;
			if (win == w->parent) {
				w->parent = NULL;
				w->main = w;
			}
			if (win == w->main)
				w->main = find_main_window(w);
		}
	}
}

/* Check if window pointer is still valid. */
static int
validate_win(struct ws_win *testwin)
{
	struct ws_win		*win;
	int			i, num_screens;

	if (testwin == NULL)
		return (0);

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(win, &screens[i].managed, manage_entry)
			if (win == testwin)
				return (0);
	return (1);
}

/* Check if workspace pointer is still valid. */
static int
validate_ws(struct workspace *testws)
{
	struct workspace	*ws;
	int			i, num_screens;

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++)
		RB_FOREACH(ws, workspace_tree, &screens[i].workspaces)
			if (ws == testws)
				return (0);
	return (1);
}

static void
unfocus_win(struct ws_win *win)
{
	bool			raise = false;

	DNPRINTF(SWM_D_FOCUS, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	if (!win->mapped) {
		DNPRINTF(SWM_D_FOCUS, "unmapped\n");
		return;
	}

	if (validate_win(win)) {
		DNPRINTF(SWM_D_FOCUS, "invalid win\n");
		kill_refs(win);
		return;
	}

	if (win->s->focus == win)
		win->s->focus = NULL;

	if (win->ws) {
		if (validate_ws(win->ws)) {
			DNPRINTF(SWM_D_FOCUS, "invalid ws\n");
			return;
		}

		raise = win->ws->always_raise;

		if (win->ws->focus == win) {
			win->ws->focus = NULL;
			if (win->ws->focus_raise == win) {
				win->ws->focus_raise = NULL;
				raise = true;
			}
		}

		if (validate_win(win->ws->focus)) {
			kill_refs(win->ws->focus);
			win->ws->focus = NULL;
		}

		if (raise) {
			update_win_layer(win);
			refresh_stack(win->s);
			update_stacking(win->s);
		}
	}

	/* Update border width */
	if (win->bordered && (win->quirks & SWM_Q_MINIMALBORDER) &&
	    win_floating(win)) {
		win->bordered = false;
		update_gravity(win);
		X(win) += border_width;
		Y(win) += border_width;
		update_window(win);
	}

	draw_frame(win);
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static bool
accepts_focus(struct ws_win *win)
{
	return (!(win->hints.flags & XCB_ICCCM_WM_HINT_INPUT) ||
	    win->hints.input);
}

static bool
win_noinput(struct ws_win *win)
{
	return (!accepts_focus(win) && !win->take_focus);
}

static void
focus_win_input(struct ws_win *win, bool force_input)
{
	/* Set input focus if no input hint, or indicated by hint. */
	if (accepts_focus(win))
		set_input_focus(win->id, force_input);
	else
		set_input_focus(win->frame, false);

	/* Tell app it can adjust focus to a specific window. */
	if (win->take_focus)
		client_msg(win, a_takefocus, event_time);
}

static void
set_input_focus(xcb_window_t winid, bool force)
{
	if (force) {
		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
		    "PointerRoot, time: CurrentTime\n", winid);
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, winid,
		    XCB_CURRENT_TIME);
	} else {
		DNPRINTF(SWM_D_FOCUS, "SetInputFocus: %#x, revert-to: "
		    "PointerRoot, time: %#x\n", winid, event_time);
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, winid,
		    event_time);
	}
}

/* Focus a window all in one go. */
static void
focus_win(struct swm_screen *s, struct ws_win *win)
{
	struct ws_win		*w;

	DNPRINTF(SWM_D_FOCUS, "win %#x\n", WINID(win));

	if (validate_win(win)) {
		kill_refs(win);
		win = NULL;
	}

	set_focus(s, win);
	if (win)
		update_win_layer_related(win);

	refresh_stack(s);
	update_stacking(s);
	update_mapping(s);
	update_focus(s);

	/* Redraw all frames for now. */
	TAILQ_FOREACH(w, &s->managed, manage_entry)
		if (w->mapped)
			draw_frame(w);

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

/* Apply follow mode focus policy. */
static void
focus_follow(struct swm_screen *s, struct swm_region *r, struct ws_win *win)
{
	struct swm_region	*rr;

	/* Try to focus pointer, otherwise window, otherwise region. */
	if (win && (validate_win(win) || win->ws->r != r))
		win = NULL;

	if (r && !ws_maponfocus(r->ws) && (rr = get_pointer_region(s)) &&
	    (rr == r || rr == s->r)) {
		if (pointer_window != XCB_WINDOW_NONE)
			focus_window(pointer_window);
		else if (r->ws->focus == NULL)
			focus_win(s, win ? get_focus_magic(win) :
			    get_focus_magic(get_ws_focus(r->ws)));
	} else if (win) {
		focus_win(s, get_focus_magic(win));
	} else if (r)
		focus_win(s, get_focus_magic(get_ws_focus(r->ws)));

	xcb_flush(conn);
}

static void
grab_buttons_win(xcb_window_t win)
{
	struct binding		*bp;
	int			i;
	uint16_t		modifiers[4];

	if (win == XCB_WINDOW_NONE)
		return;

	modifiers[0] = 0;
	modifiers[1] = numlockmask;
	modifiers[2] = XCB_MOD_MASK_LOCK;
	modifiers[3] = numlockmask | XCB_MOD_MASK_LOCK;

	xcb_ungrab_button(conn, XCB_BUTTON_INDEX_ANY, win, XCB_MOD_MASK_ANY);
	RB_FOREACH(bp, binding_tree, &bindings) {
		if (bp->type != BTNBIND)
			continue;

		if (xinput2_raw && bp->flags & BINDING_F_REPLAY)
			continue;

		/* When binding ws actions, skip unused workspaces. */
		if ((int)bp->action > FN_WS_1 + workspace_limit - 1 &&
		    bp->action <= FN_WS_22)
			continue;

		if ((int)bp->action > FN_MVWS_1 + workspace_limit - 1 &&
		    bp->action <= FN_MVWS_22)
			continue;

		if (bp->mod == XCB_MOD_MASK_ANY) {
			/* Grab ANYMOD case. */
			DNPRINTF(SWM_D_MOUSE, "grab btn: %u, modmask: %d, "
			    "win: %#x\n", bp->value, bp->mod, win);
			xcb_grab_button(conn, 0, win,
			    BUTTONMASK, XCB_GRAB_MODE_SYNC,
			    XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			    XCB_CURSOR_NONE, bp->value, bp->mod);
		} else {
			/* Need to grab each modifier permutation. */
			for (i = 0; i < LENGTH(modifiers); ++i) {
				DNPRINTF(SWM_D_MOUSE, "grab btn: %u, "
				    "modmask: %u\n", bp->value,
				    bp->mod | modifiers[i]);
				xcb_grab_button(conn, 0, win, BUTTONMASK,
				    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC,
				    XCB_WINDOW_NONE, XCB_CURSOR_NONE,
				    bp->value, bp->mod | modifiers[i]);
			}
		}
	}
}

/* If a transient window should have focus instead, return it. */
static struct ws_win *
get_focus_magic(struct ws_win *win)
{
	struct ws_win	*winfocus = NULL;
	int		i, wincount;

	DNPRINTF(SWM_D_FOCUS, "win %#x\n", WINID(win));
	if (win == NULL)
		return (win);

	winfocus = win->main;

	if (winfocus->focus_redirect == NULL)
		return (winfocus);

	/* Put limit just in case a redirect loop exists. */
	wincount = count_win(win->ws, SWM_COUNT_NORMAL);
	for (i = 0; i < wincount; ++i) {
		if (winfocus->focus_redirect == NULL)
			break;

		if (HIDDEN(winfocus->focus_redirect))
			break;

		if (validate_win(winfocus->focus_redirect))
			break;

		winfocus = winfocus->focus_redirect;
	}

	return (winfocus);
}

static void
set_focus(struct swm_screen *s, struct ws_win *win)
{
	struct ws_win		*w;

	DNPRINTF(SWM_D_FOCUS, "screen %d, win: %#x\n", s->idx, WINID(win));

	if (win == NULL) {
		s->focus = NULL;
		return;
	}

	if (win->s != s)
		return;

	TAILQ_REMOVE(&s->fl, win, focus_entry);
	TAILQ_INSERT_HEAD(&s->fl, win, focus_entry);

	if ((w = win->ws->focus) != win) {
		win->ws->focus = win;
		win->ws->focus_raise = NULL;
		if (w)
			update_win_layer(w);
		if (win->ws->always_raise)
			update_win_layer(win);
	}
	s->focus = win;
	set_focus_redirect(win);
}

static void
set_focus_prev(struct ws_win *win)
{
	struct ws_win		*w;

	DNPRINTF(SWM_D_FOCUS, "win: %#x\n", WINID(win));
	if (win == NULL || win->ws == NULL)
		return;

	if (win->ws->focus) {
		w = TAILQ_FIRST(&win->s->fl);
		if (win != w) {
			TAILQ_REMOVE(&win->s->fl, win, focus_entry);
			TAILQ_INSERT_AFTER(&win->s->fl, w, win, focus_entry);
		}
	} else {
		TAILQ_REMOVE(&win->s->fl, win, focus_entry);
		TAILQ_INSERT_HEAD(&win->s->fl, win, focus_entry);
	}
}

static void
update_focus(struct swm_screen *s)
{
	struct ws_win				*win, *cfw = NULL;
	xcb_get_window_attributes_reply_t	*war = NULL;
	xcb_window_t				cfid;

	if (s == NULL)
		return;

	/* 1. Check if a managed window has focus right now. */
	cfid = get_input_focus();
	cfw = find_window(cfid);

	/* 2. Get currently set focus, if any. */
	if (s->focus)
		win = s->focus;
	else if (s->r_focus)
		win = get_focus_magic(get_ws_focus(s->r_focus->ws));
	else
		win = NULL;

	DNPRINTF(SWM_D_FOCUS, "win: %#x, cfw: %#x, sfocus: %#x\n",
	    WINID(win), WINID(cfw), WINID(s->focus));

	/* Skip reenter from override-redirect. */
	if (cfw == NULL && cfid != XCB_WINDOW_NONE) {
		war = xcb_get_window_attributes_reply(conn,
		    xcb_get_window_attributes(conn, cfid), NULL);
		if (war && war->override_redirect && (win == NULL ||
		    win->id == s->active_window)) {
			DNPRINTF(SWM_D_FOCUS, "skip refocus from "
			    "override_redirect.\n");
			free(war);
			return;
		}
		free(war);
	}

	/* 3. Handle current focus. */
	if (cfw && cfw != win) {
		if (cfw->mapped && win_reparented(cfw) && (win == NULL ||
		    (cfw->ws != win->ws && !ws_root(cfw->ws) &&
		     !ws_root(win->ws))))
			draw_frame(cfw);
		else
			unfocus_win(cfw);
	}

	/* 4. Set current focus and unfocus existing focus. */
	if (s->focus != win) {
		if (s->focus && s->focus != cfw)
			unfocus_win(s->focus);
		set_focus(s, win);
	}

	if (win && DEMANDS_ATTENTION(win))
		clear_attention(win);

	/* 5. Set input focus. */
	if (win && cfw == win)
		return; /* Already has input focus. */

	if (win && win->mapped) {
		focus_win_input(((win_noinput(win) && win_transient(win)) ?
		    win->main : win), false);
		set_region(win->ws->r);
	} else
		set_input_focus((s->r_focus ? s->r_focus->id : s->swmwin), true);

	draw_frame(win);
	update_bars(s);
	ewmh_update_active_window(s);
}

static void
set_region(struct swm_region *r)
{
	struct swm_region	*rf;
	int			vals[2];

	if (r == NULL)
		return;

	if (rg_root(r))
		return;

	rf = r->s->r_focus;
	if (rf && rf == r)
		return;

	if (rf != NULL && rf != r && (X(rf) != X(r) || Y(rf) != Y(r) ||
	    WIDTH(rf) != WIDTH(r) || HEIGHT(rf) != HEIGHT(r))) {
		/* Set _NET_DESKTOP_GEOMETRY. */
		vals[0] = WIDTH(r);
		vals[1] = HEIGHT(r);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, r->s->root,
		    ewmh[_NET_DESKTOP_GEOMETRY].atom, XCB_ATOM_CARDINAL, 32, 2,
		    &vals);
	}

	r->s->r_focus = r;

	/* Update the focus window frame on the now unfocused region. */
	if (rf && rf->ws->focus)
		draw_frame(rf->ws->focus);

	ewmh_update_current_desktop(r->s);
}

static void
focus_region(struct swm_region *r)
{
	struct ws_win		*nfw;
	struct swm_region	*old_r;

	if (r == NULL)
		return;

	old_r = r->s->r_focus;
	set_region(r);

	nfw = get_focus_magic(get_ws_focus(r->ws));
	if (nfw) {
		focus_win(nfw->s, nfw);
	} else {
		/* New region is empty; need to manually unfocus win. */
		if (old_r && old_r != r) {
			unfocus_win(old_r->ws->focus);
			/* Clear bar since empty. */
			bar_draw(old_r->bar);
		}
		bar_draw(r->bar);

		set_input_focus(r->id, true);
		ewmh_update_active_window(r->s);
	}

	if (apply_unfocus(r->s->r->ws, NULL)) {
		refresh_stack(r->s);
		update_stacking(r->s);
		stack(r->s->r);
		update_mapping(r->s);
	}

}

#define SWM_MODE_VFLIP		(1 << 0)
#define SWM_MODE_HFLIP		(1 << 1)

#define SWM_MODE_NORMAL		(0)
#define SWM_MODE_LEFT		(SWM_MODE_VFLIP)
#define SWM_MODE_INVERTED	(SWM_MODE_VFLIP | SWM_MODE_HFLIP)
#define SWM_MODE_RIGHT		(SWM_MODE_HFLIP)

static struct rotation_mode {
	uint16_t	r;
	uint8_t		mode;
} rotation_map[] = {
	{ XCB_RANDR_ROTATION_ROTATE_0,		SWM_MODE_NORMAL },
	{ XCB_RANDR_ROTATION_ROTATE_90,		SWM_MODE_LEFT },
	{ XCB_RANDR_ROTATION_ROTATE_180,	SWM_MODE_INVERTED },
	{ XCB_RANDR_ROTATION_ROTATE_270,	SWM_MODE_RIGHT },
};

static void
rotatews(struct workspace *ws, uint16_t rot)
{
	int		i, j, d;
	uint8_t		mode = 0;

	if (ws->cur_layout != &layouts[SWM_V_STACK] &&
	    ws->cur_layout != &layouts[SWM_H_STACK])
		return;

	/* Get current mode. */
	mode = (ws->l_state.vertical_flip ? SWM_MODE_VFLIP : 0) |
	    (ws->l_state.horizontal_flip ? SWM_MODE_HFLIP : 0);

	/* Get offset to new rotation. */
	d = 0;
	for (i = 0; i < LENGTH(rotation_map); i++)
		if (rotation_map[i].r == ws->rotation) {
			for (j = 0; j < LENGTH(rotation_map); j++) {
				if (rotation_map[(i + j) %
				    LENGTH(rotation_map)].r == rot) {
					d = j;
					break;
				}
			}
			break;
		}

	/* Apply offset to current flip. */
	for (i = 0; i < LENGTH(rotation_map); i++)
		if (rotation_map[i].mode == mode) {
			mode = rotation_map[(i + d) %
			    LENGTH(rotation_map)].mode;
			break;
		}

	/* Swap layout if rotation axis changed. */
	if ((ws->rotation & ROTATION_VERT) != (rot & ROTATION_VERT))
		ws->cur_layout = (ws->cur_layout == &layouts[SWM_V_STACK] ?
			&layouts[SWM_H_STACK] : &layouts[SWM_V_STACK]);

	/* Set new mode. */
	ws->l_state.vertical_flip = (mode & SWM_MODE_VFLIP);
	ws->l_state.horizontal_flip = (mode & SWM_MODE_HFLIP);
	ws->rotation = rot;
}

static void
switch_workspace(struct swm_region *r, struct workspace *ws, bool noclamp,
    bool follow)
{
	struct swm_screen	*s;
	struct swm_region	*other_r;
	struct workspace	*old_ws;
	struct ws_win		*nfw;

	if (r == NULL || ws == NULL)
		return;

	if (rg_root(r) || ws_root(ws))
		return;

	s = r->s;
	old_ws = r->ws;
	if (old_ws == ws) {
		if (win_free(s->focus)) {
			set_focus(s, get_focus_magic(get_ws_focus(ws)));
			apply_unfocus(s->r->ws, NULL);
			refresh_stack(s);
			update_stacking(s);
			update_mapping(s);
			update_focus(s);
			center_pointer(r);
			flush();
		}
		return;
	}

	DNPRINTF(SWM_D_WS, "screen[%d]:%dx%d+%d+%d: %d -> %d\n", s->idx,
	    WIDTH(r), HEIGHT(r), X(r), Y(r), (old_ws ? old_ws->idx : -2),
	    ws->idx);

	other_r = ws->r;
	if (other_r && workspace_clamp && !noclamp) {
		DNPRINTF(SWM_D_WS, "ws clamped.\n");
		if (warp_focus) {
			DNPRINTF(SWM_D_WS, "warping focus to region "
			    "with ws %d\n", ws->idx);
			focus_region(other_r);
			center_pointer(other_r);
			flush();
		}
		return;
	}

	if (other_r) {
		/* The other ws is visible in another region, exchange them. */
		other_r->ws_prior = ws;
		other_r->ws = old_ws;
		old_ws->r = other_r;

		if (workspace_autorotate)
			rotatews(old_ws, ROTATION(other_r));
	} else
		/* The other workspace is hidden, hide this one. */
		old_ws->r = NULL;

	if (workspace_autorotate)
		rotatews(ws, ROTATION(r));

	/* Attach new ws. */
	r->ws_prior = old_ws;
	r->ws = ws;
	ws->r = r;

	/* Prepare focus. */
	nfw = get_focus_magic(get_ws_focus(ws));
	set_focus(s, nfw);
	if (apply_unfocus(s->r->ws, NULL))
		stack(s->r);
	apply_unfocus(ws, nfw);
	refresh_stack(s);
	update_stacking(s);
	if (refresh_strut(s))
		update_layout(s);
	else {
		stack(other_r);
		stack(r);
	}
	update_mapping(s);

	/* Unmap old windows. */
	if (old_ws->r == NULL)
		unmap_workspace(old_ws);

	bar_draw(r->bar);
	if (other_r)
		bar_draw(other_r->bar);

	ewmh_update_current_desktop(s);
	ewmh_update_number_of_desktops(s);

	if (!follow)
		update_focus(s);

	center_pointer(r);
	flush();
	if (follow)
		focus_follow(s, r, NULL);

	DNPRINTF(SWM_D_WS, "done\n");
}

static void
switchws(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct workspace	*ws;

	/* Suppress warning. */
	(void)bp;

	ws = get_workspace(s, args->id);
	if (ws == NULL)
		return;

	r = get_current_region(s);
	switch_workspace(r, ws, false, follow_mode(SWM_FOCUS_TYPE_WORKSPACE));
	DNPRINTF(SWM_D_WS, "done\n");
}

static void
cyclews(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r, *rr;
	struct workspace	*ws, *nws = NULL;
	struct ws_win		*winfocus;
	int			i;
	bool			allowempty = false, mv = false;

	/* Suppress warning. */
	(void)bp;

	if ((r = get_current_region(s)) == NULL)
		return;
	ws = r->ws;
	winfocus = ws->focus;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), ws->idx);

	for (i = 0; i < workspace_limit; ++i) {
		switch (args->id) {
		case SWM_ARG_ID_CYCLEWS_MOVE_UP:
			mv = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP_ALL:
			allowempty = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP:
			nws = get_workspace(s, (ws->idx + i + 1) %
			    workspace_limit);
			break;
		case SWM_ARG_ID_CYCLEWS_MOVE_DOWN:
			mv = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN_ALL:
			allowempty = true;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN:
			nws = get_workspace(s, (workspace_limit +
			    ws->idx - i - 1) % workspace_limit);
			break;
		default:
			return;
		};

		if (nws == NULL)
			return; /* Shouldn't happen. */

		DNPRINTF(SWM_D_WS, "curws: %d, nws: %d, allowempty: %d, mv: %d,"
		    " cycle_visible: %d, cycle_empty: %d\n", ws->idx, nws->idx,
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
			win_to_ws(winfocus, nws, SWM_WIN_NOUNMAP);
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
			prioritize_window(winfocus);
			draw_frame(get_ws_focus_prev(nws));

			if (nws->r != r) {
				set_region(nws->r);
				draw_frame(ws->focus);
			}

			apply_unfocus(nws, NULL);
			refresh_stack(r->s);
			update_stacking(s);

			if (rr)
				stack(rr);
			else
				unmap_workspace(ws);
			stack(r);
			update_mapping(s);

			bar_draw(r->bar);
			if (rr)
				bar_draw(rr->bar);

			ewmh_update_current_desktop(r->s);
			center_pointer(nws->r);
			flush();
		} else {
			switch_workspace(r, nws, false,
			    follow_mode(SWM_FOCUS_TYPE_WORKSPACE));
		}
	}

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
unmap_workspace(struct workspace *ws)
{
	struct ws_win	*w;

	if (ws == NULL)
		return;

	TAILQ_FOREACH(w, &ws->winlist, entry)
		unmap_window(w);
}

static void
emptyws(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct workspace	*ws = NULL;
	struct ws_win		*win;
	int			i;

	/* Suppress warning. */
	(void)bp;

	if ((r = get_current_region(s)) == NULL)
		return;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n", args->id,
	    s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	/* Find first empty ws. */
	for (i = 0; i < workspace_limit; ++i) {
		ws = get_workspace(s, i);
		if (TAILQ_EMPTY(&ws->winlist))
			break;
	}
	if (ws == NULL) {
		DNPRINTF(SWM_D_FOCUS, "no empty ws.\n");
		return;
	}

	switch (args->id) {
	case SWM_ARG_ID_WS_EMPTY_MOVE:
		win = s->focus;
		if (win == NULL || win_free(win))
			return;
		transfer_win(win, ws);
		/* FALLTHROUGH */
	case SWM_ARG_ID_WS_EMPTY:
		switch_workspace(r, ws, false,
		    follow_mode(SWM_FOCUS_TYPE_WORKSPACE));
		break;
	default:
		DNPRINTF(SWM_D_FOCUS, "invalid id: %d\n", args->id);
	}

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
priorws(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	if ((r = get_current_region(s)) == NULL)
		return;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	if (r->ws_prior == NULL)
		return;

	switch_workspace(r, r->ws_prior, false,
	    follow_mode(SWM_FOCUS_TYPE_WORKSPACE));
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
focusrg(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;

	/* Suppress warning. */
	(void)bp;

	r = get_region(s, args->id);
	if (r == NULL || s->r_focus == r)
		return;

	focus_region(r);
	center_pointer(r);
	flush();
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
cyclerg(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r, *rr = NULL;
	int			num_screens;

	/* Suppress warning. */
	(void)bp;

	if ((r = get_current_region(s)) == NULL)
		return;

	num_screens = get_screen_count();
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	DNPRINTF(SWM_D_FOCUS, "id: %d, region: %d\n", args->id,
	    get_region_index(r));

	switch (args->id) {
	case SWM_ARG_ID_CYCLERG_UP:
	case SWM_ARG_ID_CYCLERG_MOVE_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&s->rl);
		break;
	case SWM_ARG_ID_CYCLERG_DOWN:
	case SWM_ARG_ID_CYCLERG_MOVE_DOWN:
		rr = TAILQ_PREV(r, swm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&s->rl, swm_region_list);
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
		flush();
		break;
	case SWM_ARG_ID_CYCLERG_MOVE_UP:
	case SWM_ARG_ID_CYCLERG_MOVE_DOWN:
		switch_workspace(r, rr->ws, true,
		    follow_mode(SWM_FOCUS_TYPE_WORKSPACE));
		break;
	default:
		return;
	};

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
swapwin(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*w, *sw, *pw;
	struct ws_win_list	*wl;

	/* Suppress warning. */
	(void)bp;

	if (s->focus && win_free(s->focus))
		return;

	if ((r = get_current_region(s)) == NULL)
		return;

	DNPRINTF(SWM_D_WS, "id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n", args->id,
	    s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	sw = r->ws->focus;
	if (sw == NULL || FULLSCREEN(sw))
		return;

	if (ws_maxstack(r->ws))
		return;

	if (apply_unfocus(r->ws, NULL) > 0)
		refresh_stack(r->s);

	wl = &sw->ws->winlist;

	switch (args->id) {
	case SWM_ARG_ID_SWAPPREV:
		w = sw = sw->main;
		/* Find prev 'main' */
		while ((w = TAILQ_PREV(w, ws_win_list, entry)))
			if (!HIDDEN(w) && win_main(w))
				break;

		TAILQ_REMOVE(wl, sw, entry);
		if (w == NULL)
			TAILQ_INSERT_TAIL(wl, sw, entry);
		else
			TAILQ_INSERT_BEFORE(w, sw, entry);
		break;
	case SWM_ARG_ID_SWAPNEXT:
		w = sw = sw->main;
		/* Find next 'main' */
		while ((w = TAILQ_NEXT(w, entry)))
			if (!HIDDEN(w) && win_main(w))
				break;

		TAILQ_REMOVE(wl, sw, entry);
		if (w == NULL)
			TAILQ_INSERT_HEAD(wl, sw, entry);
		else
			TAILQ_INSERT_AFTER(wl, w, sw, entry);
		break;
	case SWM_ARG_ID_SWAPMAIN:
		sw = sw->main;
		w = get_main_window(r->ws);
		if (w) {
			w = w->main;
			if (w == sw) {
				sw = get_ws_focus_prev(r->ws);
				if (sw)
					sw = sw->main;
			}
		}
		if (w == NULL || sw == NULL || w->ws != sw->ws)
			return;
		pw = w;
		while ((pw = TAILQ_PREV(pw, ws_win_list, entry)))
			if (pw != sw)
				break;
		set_focus_prev(w);
		TAILQ_REMOVE(wl, w, entry);
		TAILQ_INSERT_BEFORE(sw, w, entry);
		TAILQ_REMOVE(wl, sw, entry);
		if (pw)
			TAILQ_INSERT_AFTER(wl, pw, sw, entry);
		else
			TAILQ_INSERT_HEAD(wl, sw, entry);
		break;
	default:
		DNPRINTF(SWM_D_MOVE, "invalid id: %d\n", args->id);
		return;
	}

	ewmh_update_client_list(s);

	update_stacking(s);
	stack(r);
	update_mapping(s);
	center_pointer(r);
	flush();

	DNPRINTF(SWM_D_MOVE, "done\n");
}

/* Determine focus other than specified window, but on the same workspace. */
static struct ws_win *
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
	    WINID(win), WINID(ws->focus), WINID(get_ws_focus_prev(ws)),
	    focus_close, focus_close_wrap, focus_default);

	if (ws->focus == NULL) {
		/* Fallback to default. */
		if (focus_default == SWM_STACK_TOP) {
			TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list, entry)
				if (!HIDDEN(winfocus) && winfocus != win)
					break;
		} else {
			TAILQ_FOREACH(winfocus, wl, entry)
				if (!HIDDEN(winfocus) && winfocus != win)
					break;
		}

		goto done;
	}

	if (ws->focus != win && !HIDDEN(ws->focus)) {
		winfocus = ws->focus;
		goto done;
	}

	/* FOCUSPREV quirk: try previously focused window. */
	if (win->quirks & SWM_Q_FOCUSPREV) {
		winfocus = get_ws_focus_prev(ws);
		if (winfocus)
			goto done;
	}

	if (ws->focus == win && win_transient(win)) {
		w = win->parent;
		if (w && w != win && w->ws == ws && !HIDDEN(w)) {
			winfocus = w;
			goto done;
		}
	}

	if (ws->cur_layout->flags & SWM_L_FOCUSPREV) {
		winfocus = get_ws_focus_prev(ws);
		if (winfocus)
			goto done;
	}

	switch (focus_close) {
	case SWM_STACK_BOTTOM:
		TAILQ_FOREACH(winfocus, wl, entry)
			if (!HIDDEN(winfocus) && winfocus != win)
				break;
		break;
	case SWM_STACK_TOP:
		TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list, entry)
			if (!HIDDEN(winfocus) && winfocus != win)
				break;
		break;
	case SWM_STACK_ABOVE:
		winfocus = TAILQ_NEXT(win, entry);
		while (winfocus && HIDDEN(winfocus))
			winfocus = TAILQ_NEXT(winfocus, entry);

		if (winfocus == NULL) {
			if (focus_close_wrap) {
				TAILQ_FOREACH(winfocus, wl, entry)
					if (!HIDDEN(winfocus) &&
					    winfocus != win)
						break;
			} else {
				TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list,
				    entry)
					if (!HIDDEN(winfocus) &&
					    winfocus != win)
						break;
			}
		}
		break;
	case SWM_STACK_PRIOR:
		winfocus = get_ws_focus_prev(ws);
		break;
	case SWM_STACK_BELOW:
		winfocus = TAILQ_PREV(win, ws_win_list, entry);
		while (winfocus && HIDDEN(winfocus))
			winfocus = TAILQ_PREV(winfocus, ws_win_list, entry);

		if (winfocus == NULL) {
			if (focus_close_wrap) {
				TAILQ_FOREACH_REVERSE(winfocus, wl, ws_win_list,
				    entry)
					if (!HIDDEN(winfocus) &&
					    winfocus != win)
						break;
			} else {
				TAILQ_FOREACH(winfocus, wl, entry)
					if (!HIDDEN(winfocus) &&
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

static struct ws_win *
get_ws_focus_prev(struct workspace *ws)
{
	struct ws_win	*w;

	/* Find last available focus that is unrelated to the current focus. */
	TAILQ_FOREACH(w, &ws->s->fl, focus_entry)
		if (w->ws == ws && !HIDDEN(w) && w != ws->focus &&
		    !win_related(w, ws->focus))
			break;
	return (w);
}

static struct ws_win *
get_ws_focus(struct workspace *ws)
{
	struct ws_win		*winfocus = NULL;

	if (ws == NULL)
		return (NULL);

	if (ws->focus && !HIDDEN(ws->focus))
		winfocus = ws->focus;
	else
		winfocus = get_ws_focus_prev(ws);

	return (winfocus);
}

/* Return the 'main' window in specified workspace. */
static struct ws_win *
get_main_window(struct workspace *ws)
{
	struct ws_win		*mwin = NULL;

	if (ws == NULL)
		return (NULL);

	/* Use the first tiled window, unless layout is max. */
	if (!ws_maxstack(ws)) {
		TAILQ_FOREACH(mwin, &ws->winlist, entry) {
			if (HIDDEN(mwin))
				continue;

			if (!win_floating(mwin))
				break;
		}
	}

	/* Fallback to the first 'main' window. */
	if (mwin == NULL)
		TAILQ_FOREACH(mwin, &ws->winlist, entry)
			if (!HIDDEN(mwin) && win_main(mwin))
				break;

	return (mwin);
}

static void
focus(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*head, *cur_focus = NULL, *winfocus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws, *cws, *wws;
	int			i, d, wincount;

	/* Suppress warning. */
	(void)bp;

	if ((r = get_current_region(s)) == NULL)
		return;

	ws = r->ws;
	wl = &ws->winlist;

	cur_focus = s->focus;
	if (cur_focus == NULL)
		cur_focus = get_focus_magic(get_ws_focus(ws));

	DNPRINTF(SWM_D_FOCUS, "id: %d, cur_focus: %#x\n", args->id,
	    WINID(cur_focus));

	/* Make sure an uniconified window has focus, if one exists. */
	if (cur_focus == NULL) {
		cur_focus = TAILQ_FIRST(wl);
		while (cur_focus != NULL && HIDDEN(cur_focus))
			cur_focus = TAILQ_NEXT(cur_focus, entry);

		DNPRINTF(SWM_D_FOCUS, "new cur_focus: %#x\n", WINID(cur_focus));
	}

	cws = cur_focus ? cur_focus->ws : NULL;

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		if (cur_focus == NULL)
			goto out;

		wincount = count_win(ws, SWM_COUNT_ALL) +
		    count_win(s->r->ws, SWM_COUNT_ALL);
		winfocus = cur_focus->main;
		for (i = 0; i < wincount; ++i) {
			winfocus = TAILQ_PREV(winfocus, ws_win_list, entry);
			if (winfocus == NULL) {
				winfocus = TAILQ_LAST(wl, ws_win_list);
				if (winfocus == NULL)
					break;
			}
			if (HIDDEN(winfocus) || !win_main(winfocus))
				continue;
			if (winfocus->quirks & SWM_Q_NOFOCUSCYCLE)
				continue;
			break;
		}
		break;
	case SWM_ARG_ID_FOCUSNEXT:
		if (cur_focus == NULL)
			goto out;

		wincount = count_win(ws, SWM_COUNT_ALL) +
		    count_win(s->r->ws, SWM_COUNT_ALL);
		winfocus = cur_focus->main;
		for (i = 0; i < wincount; ++i) {
			winfocus = TAILQ_NEXT(winfocus, entry);
			if (winfocus == NULL) {
				winfocus = TAILQ_FIRST(wl);
				if (winfocus == NULL)
					break;
			}
			if (HIDDEN(winfocus) || !win_main(winfocus))
				continue;
			if (winfocus->quirks & SWM_Q_NOFOCUSCYCLE)
				continue;
			break;
		}
		break;
	case SWM_ARG_ID_FOCUSMAIN:
		if (cur_focus == NULL)
			goto out;

		winfocus = get_main_window(ws);
		if (winfocus == cur_focus)
			winfocus = get_ws_focus_prev(ws);
		break;
	case SWM_ARG_ID_FOCUSPRIOR:
		if (cur_focus == NULL)
			goto out;

		winfocus = get_ws_focus_prev(ws);
		break;
	case SWM_ARG_ID_FOCUSURGENT:
		/* Search forward for the next urgent window. */
		winfocus = NULL;
		head = cur_focus;
		d = cws ? cws->idx : ws->idx;

		for (i = 0; i <= workspace_limit + 1; ++i) {
			if (head == NULL) {
				wws = workspace_lookup(s,
				    (d + i + 1) % (workspace_limit + 1) - 1);
				if (wws)
					head = TAILQ_FIRST(&wws->winlist);
			}

			while (head) {
				if (head == cur_focus) {
					if (i > 0) {
						winfocus = NULL;
						break;
					}
				} else if (win_urgent(head)) {
					winfocus = head;
					break;
				}

				head = TAILQ_NEXT(head, entry);
			}

			if (winfocus)
				break;
		}

		/* Switch ws if new focus is on a different ws. */
		if (winfocus && winfocus->ws != ws && !ws_root(winfocus->ws))
			switch_workspace(r, winfocus->ws, false, false);
		break;
	case SWM_ARG_ID_FOCUSFREE:
		if (s->focus && win_free(s->focus)) {
			winfocus = get_focus_magic(get_ws_focus(ws));
		} else {
			winfocus = get_focus_magic(get_ws_focus(s->r->ws));
			if (winfocus == NULL)
				winfocus = cur_focus;
		}
		break;
	default:
		goto out;
	}

	set_focus(s, get_focus_magic(winfocus));

	if (winfocus && winfocus->ws != cws)
		apply_unfocus(winfocus->ws, NULL);
	else if (winfocus == NULL) {
		apply_unfocus(s->r->ws, NULL);
	}
	apply_unfocus(cws, NULL);

	if (winfocus)
		update_win_layer_related(winfocus);
	refresh_stack(s);
	update_stacking(s);
	stack(r);
	stack(s->r);
	update_mapping(s);

	update_focus(s);
	center_pointer(r);
	flush();
out:
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
focus_pointer(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win		*win;

	/* Suppress warning. */
	(void)args;

	/* Not needed for buttons since this is already done in buttonpress. */
	if (bp->type == KEYBIND) {
		win = get_pointer_win(s);
		if (click_to_raise && !win_prioritized(win))
			prioritize_window(win);
		focus_win(s, get_pointer_win(s));
		flush();
	}
}

static void
switchlayout(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct layout		*new_layout = NULL;
	struct swm_region	*r;
	struct workspace	*ws;
	struct ws_win		*w;
	uint32_t		changed = 0;
	int			i;

	/* Suppress warning. */
	(void)bp;

	if ((r = get_current_region(s)) == NULL)
		return;
	ws = r->ws;

	DNPRINTF(SWM_D_EVENT, "workspace: %d\n", ws->idx);

	switch (args->id) {
	case SWM_ARG_ID_CYCLE_LAYOUT:
		for (i = 0; i < layout_order_count; i++)
			if (layout_order[i] == ws->cur_layout) {
				new_layout =
				    layout_order[(i + 1) % layout_order_count];
				break;
			}

		if (new_layout == NULL)
			new_layout = layout_order[0];
		break;
	case SWM_ARG_ID_LAYOUT_VERTICAL:
		new_layout = &layouts[SWM_V_STACK];
		break;
	case SWM_ARG_ID_LAYOUT_HORIZONTAL:
		new_layout = &layouts[SWM_H_STACK];
		break;
	case SWM_ARG_ID_LAYOUT_MAX:
		new_layout = &layouts[SWM_MAX_STACK];
		break;
	case SWM_ARG_ID_LAYOUT_FLOATING:
		new_layout = &layouts[SWM_FLOATING_STACK];
		break;
	case SWM_ARG_ID_PRIOR_LAYOUT:
		if (ws->prev_layout)
			new_layout = ws->prev_layout;
		break;
	default:
		goto out;
	}

	if (new_layout == NULL || new_layout == ws->cur_layout)
		goto out;

	ws->prev_layout = ws->cur_layout;
	ws->cur_layout = new_layout;

	if (max_layout_maximize) {
		if (!ws_maxstack_prior(ws) && ws_maxstack(ws)) {
			/* Enter max layout. */
			TAILQ_FOREACH(w, &ws->winlist, entry) {
				if (win_transient(w) || WINDOCK(w) ||
				    WINDESKTOP(w))
					continue;
				w->normalmax = MAXIMIZED(w);
				if (!MAXIMIZED(w) && w->maxstackmax) {
					changed |= ewmh_apply_flags(w,
					    w->ewmh_flags | EWMH_F_MAXIMIZED);
					ewmh_update_wm_state(w);
				}
			}
		} else if (ws_maxstack_prior(ws) && !ws_maxstack(ws)) {
			/* Leave max layout. */
			TAILQ_FOREACH(w, &ws->winlist, entry) {
				if (win_transient(w) || WINDOCK(w) ||
				    WINDESKTOP(w))
					continue;
				w->maxstackmax = MAXIMIZED(w);
				if (MAXIMIZED(w) && !w->normalmax) {
					changed |= ewmh_apply_flags(w,
					    w->ewmh_flags & ~EWMH_F_MAXIMIZED);
					ewmh_update_wm_state(w);
				}
			}
		}
	}

	changed |= apply_unfocus(ws, NULL);
	if (changed || ws->prev_layout->flags & SWM_L_NOTILE ||
	    ws->cur_layout->flags & SWM_L_NOTILE) {
		TAILQ_FOREACH(w, &ws->winlist, entry)
			if (win_main(w))
				update_win_layer_related(w);
		refresh_stack(s);
		update_stacking(s);
	}

	stack(r);
	update_mapping(s);
	bar_draw(r->bar);

	focus_win(s, get_focus_magic(get_ws_focus(ws)));
	center_pointer(r);

	flush();
	if (follow_mode(SWM_FOCUS_TYPE_LAYOUT))
		focus_follow(s, r, NULL);
out:
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
stack_config(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct workspace	*ws;

	/* Suppress warning. */
	(void)bp;

	if ((r = get_current_region(s)) == NULL)
		return;
	ws = r->ws;

	DNPRINTF(SWM_D_STACK, "id: %d workspace: %d\n", args->id, ws->idx);

	if (apply_unfocus(ws, NULL) > 0) {
		refresh_stack(s);
		update_stacking(s);
		stack(r);
	}

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != SWM_ARG_ID_STACKINIT)
		stack(r);

	update_mapping(s);
	bar_draw(r->bar);

	center_pointer(r);
	flush();
}

static void
stack(struct swm_region *r) {
	struct swm_geometry	g;

	if (r == NULL)
		return;

	DNPRINTF(SWM_D_STACK, "begin\n");

	/* Adjust stack area for region bar and padding. */
	g = r->g_usable;
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

	r->ws->cur_layout->l_stack(r->ws, &g);
	r->ws->cur_layout->l_string(r->ws);
	/* save r so we can track region changes */
	r->ws->old_r = r;

	if (font_adjusted)
		font_adjusted--;

	DNPRINTF(SWM_D_STACK, "end\n");
}

static void
store_float_geom(struct ws_win *win)
{
	struct swm_region	*r;

	if (win == NULL)
		return;

	/* Exclude fullscreen/maximized/tiled. */
	if (FULLSCREEN(win) || MAXIMIZED(win) || (!ABOVE(win) &&
	    !win_transient(win) && !ws_floating(win->ws)))
		return;

	/* Retain window geometry and update reference coordinates. */
	win->g_float = win->g;
	win->g_float.x -= win->g_grav.x;
	win->g_float.y -= win->g_grav.y;

	r = (win_free(win) && win->s->r_focus) ? win->s->r_focus : win->ws->r;
	if (r) {
		win->g_floatref = r->g;
		win->g_floatref_root = false;
	}

	DNPRINTF(SWM_D_MISC, "win %#x, g_float: (%d,%d) %d x %d, "
	    "g_floatref: (%d,%d) %d x %d\n", win->id, win->g_float.x,
	    win->g_float.y, win->g_float.w, win->g_float.h, win->g_floatref.x,
	    win->g_floatref.y, win->g_floatref.w, win->g_floatref.h);
}

static void
load_float_geom(struct ws_win *win)
{
	if (win == NULL)
		return;

	win->g = win->g_float;
	win->g.x += win->g_grav.x;
	win->g.y += win->g_grav.y;

	if (!win_free(win) && win->ws->r && !win->g_floatref_root) {
		/* Adjust position to current region. */
		X(win) += X(win->ws->r) - win->g_floatref.x;
		Y(win) += Y(win->ws->r) - win->g_floatref.y;
	}
	DNPRINTF(SWM_D_MISC, "win %#x, g: (%d,%d) %d x %d, ref_root:%d\n",
	    win->id, X(win), Y(win), WIDTH(win), HEIGHT(win),
	    win->g_floatref_root);
}

static void
update_floater(struct ws_win *win)
{
	struct workspace	*ws;
	struct swm_region	*r, *rf;
	bool			bordered, fs;

	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	ws = win->ws;

	if ((r = ws->r) == NULL)
		return;

	if (win_free(win)) {
		rf = region_under(win->s, X(win) + WIDTH(win) / 2,
		    Y(win) + HEIGHT(win) / 2);
		if (rf == NULL)
			rf = r->s->r_focus ? r->s->r_focus : r->s->r;
	} else
		rf = r;

	bordered = !WINDOCK(win) && !WINDESKTOP(win);

	if (FULLSCREEN(win)) {
		/* _NET_WM_FULLSCREEN: fullscreen without border. */
		win->g = rf->g;
		if (win->bordered) {
			win->bordered = false;
			update_gravity(win);
		}
	} else if (MAXIMIZED(win)) {
		/* Maximize: like a single stacked window. */
		win->g = rf->g_usable;

		if (bar_enabled && ws->bar_enabled && !maximize_hide_bar) {
			if (!bar_at_bottom)
				Y(win) += bar_height;
			HEIGHT(win) -= bar_height;
		} else if (disable_border) {
			bordered = false;
		}

		if (disable_border_always)
			bordered = false;

		if (bordered) {
			/* Window geometry excludes frame. */
			X(win) += border_width;
			Y(win) += border_width;
			HEIGHT(win) -= 2 * border_width;
			WIDTH(win) -= 2 * border_width;
		}

		if (win->bordered != bordered) {
			win->bordered = bordered;
			update_gravity(win);
		}
	} else {
		/* Normal floating window. */
		if (rf != ws->old_r || ws_floating(ws) ||
		    win->quirks & SWM_Q_ANYWHERE)
			load_float_geom(win);

		fs = ((win->quirks & SWM_Q_FULLSCREEN) &&
		    WIDTH(win) >= WIDTH(rf) && HEIGHT(win) >= HEIGHT(rf));
		if (fs || ((!ws_focused(win->ws) || win->ws->focus != win) &&
		    (win->quirks & SWM_Q_MINIMALBORDER)))
			bordered = false;

		if (win->bordered != bordered) {
			win->bordered = bordered;
			update_gravity(win);
			load_float_geom(win);
		}

		/* Invalidate client position if out of region. */
		if (win->g_float_xy_valid && !bounds_intersect(&win->g, &r->g))
			win->g_float_xy_valid = false;

		if (!fs && !MANUAL(win) && !win->g_float_xy_valid) {
			if (win_transient(win) &&
			    (win->quirks & SWM_Q_TRANSSZ)) {
				/* Adjust size on TRANSSZ quirk. */
				WIDTH(win) = (double)WIDTH(rf) * dialog_ratio;
				HEIGHT(win) = (double)HEIGHT(rf) * dialog_ratio;
			}

			if (!(win->quirks & SWM_Q_ANYWHERE) && !WINDOCK(win)
			    && !WINDESKTOP(win)) {
				/*
				 * Floaters and transients are auto-centred
				 * unless manually moved, resized or ANYWHERE
				 * quirk is set.
				 */
				X(win) = X(rf) + (WIDTH(rf) - WIDTH(win)) / 2;
				Y(win) = Y(rf) + (HEIGHT(rf) - HEIGHT(win)) / 2;

				store_float_geom(win);
			}
		}
	}

	/* Ensure at least 1 pixel of the window is in the region. */
	contain_window(win, r->g, boundary_width, SWM_CW_ALLSIDES);
	update_window(win);
}

/*
 * Send keystrokes to terminal to decrease/increase the font size as the
 * window size changes.
 */
static void
adjust_font(struct ws_win *win)
{
	if (!(win->quirks & SWM_Q_XTERM_FONTADJ) ||
	    ABOVE(win) || win_transient(win))
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
static void
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
			if (!win_floating(win) && !HIDDEN(win))
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
			msize = -2 * border_width;
			colno = split = winno / stacks;
			cell.w = ((r_g.w - (stacks * 2 * border_width) +
			    2 * border_width) / stacks);
			if (flip)
				cell.x += r_g.w - cell.w;
			s = stacks - 1;
		}

		hrh = r_g.h / colno;
		extra = r_g.h - (colno * hrh);
		cell.h = hrh - 2 * border_width;
		i = j = 0;
	}

	/* Update window geometry. */
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (HIDDEN(win))
			continue;

		if (win_floating(win) || WINDESKTOP(win)) {
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
				cell.x -= cell.w + 2 * border_width + tile_gap;
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

		bordered = (winno > 1 || !disable_border || (bar_enabled &&
		    ws->bar_enabled && !disable_border_always));

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
			reconfigure = true;
			X(win) -= border_width;
			Y(win) -= border_width;
			WIDTH(win) += 2 * border_width;
			HEIGHT(win) += 2 * border_width;
		}

		if (bordered != win->bordered) {
			reconfigure = true;
			win->bordered = bordered;
			update_gravity(win);
		}

		if (reconfigure) {
			adjust_font(win);
			update_window(win);
		}

		last_h = cell.h;
		i++;
		j++;
	}

	DNPRINTF(SWM_D_STACK, "done\n");
}

static void
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

static void
vertical_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, ws->l_state.vertical_flip);
}

static void
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

static void
horizontal_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, ws->l_state.horizontal_flip);
}

static void
floating_stack(struct workspace *ws, struct swm_geometry *g)
{
	struct ws_win		*w;

	/* Suppress warning. */
	(void)g;

	/* Update window geometry. */
	TAILQ_FOREACH(w, &ws->winlist, entry) {
		if (HIDDEN(w))
			continue;

		update_floater(w);
	}
}

static void
max_config(struct workspace *ws, int id)
{
	struct swm_screen	*s = NULL;
	struct ws_win		*w;
	uint32_t		changed = 0;

	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	if (id == SWM_ARG_ID_STACKRESET) {
		TAILQ_FOREACH(w, &ws->winlist, entry) {
			w->maxstackmax = max_layout_maximize;
			if (max_layout_maximize)
				changed |= ewmh_apply_flags(w,
				    w->ewmh_flags | EWMH_F_MAXIMIZED);
			else
				changed |= ewmh_apply_flags(w,
				    w->ewmh_flags & ~EWMH_F_MAXIMIZED);

			if (changed) {
				changed = 0;
				s = w->s;
				ewmh_update_wm_state(w);
			}
		}

		if (s)
			update_stacking(s);
	}
}

/* Single-tiled layout. */
static void
max_stack(struct workspace *ws, struct swm_geometry *g)
{
	struct ws_win		*w;
	bool			bordered;

	DNPRINTF(SWM_D_STACK, "workspace: %d\n", ws->idx);

	/* Update window geometry. */
	TAILQ_FOREACH(w, &ws->winlist, entry) {
		if (HIDDEN(w))
			continue;

		if (win_floating(w)) {
			update_floater(w);
			continue;
		}

		/* Single tile. */
		bordered = (!disable_border || (bar_enabled &&
		    ws->bar_enabled && !disable_border_always));
		if (bordered != w->bordered || X(w) != g->x || Y(w) != g->y ||
		    WIDTH(w) != g->w || HEIGHT(w) != g->h) {
			if (w->bordered != bordered) {
				w->bordered = bordered;
				update_gravity(w);
			}
			w->g = *g;
			if (bordered) {
				X(w) += border_width;
				Y(w) += border_width;
			} else {
				WIDTH(w) += 2 * border_width;
				HEIGHT(w) += 2 * border_width;
			}

			adjust_font(w);
			update_window(w);
		}
	}
}

static void
update_layout(struct swm_screen *s)
{
	struct swm_region	*r;

	/* Update root and dynamic regions. */
	stack(s->r);
	TAILQ_FOREACH(r, &s->rl, entry)
		stack(r);
}

static void
update_stacking(struct swm_screen *s)
{
	struct swm_stackable	*st, *st_prev;

	/* Stack windows from bottom up. */
	st_prev = NULL;
	SLIST_FOREACH(st, &s->stack, entry) {
		update_stackable(st, st_prev);
		st_prev = st;
	}
	update_debug(s);
}

static void
update_region_mapping(struct swm_region *r)
{
	struct ws_win		*w, *wf;
	bool			mof;

	if (r == NULL || r->ws == NULL)
		return;

	wf = get_focus_magic(get_ws_focus(r->ws));
	if (r->bar)
		r->bar->disabled = (wf && ((MAXIMIZED(wf) && maximize_hide_bar
		    && maximize_hide_other) || (FULLSCREEN(wf) &&
		    fullscreen_hide_other)) && !win_below(wf));
	mof = (ws_maponfocus(r->ws) || (wf && ((maximize_hide_other &&
	    MAXIMIZED(wf)) || (fullscreen_hide_other && FULLSCREEN(wf))) &&
	    !win_below(wf)));

	DNPRINTF(SWM_D_MISC, "r:%d ws:%d wf:%#x mof:%#x\n", get_region_index(r),
	    r->ws->idx, WINID(wf), mof);
	/* Map first, then unmap. */
	TAILQ_FOREACH(w, &r->ws->winlist, entry)
		if (!HIDDEN(w) && (!mof || win_related(w, wf) || WINDOCK(w)))
			map_window(w);
	TAILQ_FOREACH(w, &r->ws->winlist, entry)
		if (HIDDEN(w) || (mof && !win_related(w, wf) && !WINDOCK(w)))
			unmap_window(w);
}

static void
update_mapping(struct swm_screen *s)
{
	struct swm_region	*r;

	/* Update root and dynamic regions. */
	update_region_mapping(s->r);
	TAILQ_FOREACH(r, &s->rl, entry)
		update_region_mapping(r);
}

static void
transfer_win(struct ws_win *win, struct workspace *ws)
{
	struct swm_screen	*s;
	struct workspace	*ows;
	bool			follow;

	s = win->s;
	ows = win->ws;

	DNPRINTF(SWM_D_MOVE, "win %#x, id: %d\n", win->id, ws->idx);

	follow = follow_pointer(s, SWM_FOCUS_TYPE_MOVE);

	if (win_free(win))
		win->ewmh_flags |= EWMH_F_ABOVE;

	win_to_ws(win, ws, SWM_WIN_UNFOCUS);
	apply_unfocus(ws, NULL);

	/* Set new focus on target ws. */
	if (!follow) {
		ws->focus = win;
		set_focus(s, ows->focus);
		draw_frame(get_ws_focus_prev(ws));
	}

	DNPRINTF(SWM_D_STACK, "focus: %#x, focus_prev: %#x, first: %#x, "
	    "win: %#x\n", WINID(ows->focus), WINID(get_ws_focus_prev(ows)),
	    WINID(TAILQ_FIRST(&ows->winlist)), win->id);

	update_win_layer_related(win);
	refresh_stack(s);
	update_stacking(s);

	stack(ows->r);
	if (ws->r) {
		if (win_floating(win))
			load_float_geom(win);

		stack(ws->r);
	} else
		store_float_geom(win);

	update_mapping(s);

	if (!follow) {
		update_focus(s);
		center_pointer(ows->r);
	}

	flush();
	if (follow) {
		if (pointer_window != XCB_WINDOW_NONE)
			focus_window(pointer_window);
		else if (ows->focus == NULL)
			focus_win(s, get_focus_magic(get_ws_focus(ows)));
		xcb_flush(conn);
	}
}

static void
send_to_rg(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win;

	/* Suppress warning. */
	(void)bp;

	DNPRINTF(SWM_D_FOCUS, "id: %d\n", args->id);

	win = s->focus;
	if (win == NULL)
		return;

	r = get_region(s, args->id);
	if (r == NULL)
		return;

	transfer_win(win, r->ws);
}

static struct swm_region *
region_under(struct swm_screen *s, int x, int y)
{
	struct swm_region	*r;

	if (s == NULL)
		return (NULL);

	TAILQ_FOREACH(r, &s->rl, entry)
		if (X(r) <= x && x < MAX_X(r) && Y(r) <= y && y < MAX_Y(r))
			break;

	return (r);
}

/* Transfer focused window to target workspace and focus. */
static void
send_to_ws(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct workspace	*ws;
	struct ws_win		*win;

	/* Suppress warning. */
	(void)bp;

	win = s->focus;
	if (win == NULL)
		return;

	ws = get_workspace(s, args->id);
	if (ws == NULL || win->ws == ws)
		return;

	transfer_win(win, ws);
	DNPRINTF(SWM_D_MISC, "done\n");
}

/* Transfer focused window to region-relative workspace and focus. */
static void
send_to_rg_relative(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win		*win;
	struct swm_region	*r, *r_other;

	/* Suppress warning. */
	(void)bp;

	win = s->focus;
	if (win == NULL)
		return;

	if ((r = get_current_region(s)) == NULL)
		return;

	win = r->ws->focus;
	if (win == NULL)
		return;

	if (args->id == 1) {
		r_other = TAILQ_NEXT(r, entry);
		if (r_other == NULL)
			r_other = TAILQ_FIRST(&s->rl);
	} else {
		r_other = TAILQ_PREV(r, swm_region_list, entry);
		if (r_other == NULL)
			r_other = TAILQ_LAST(&s->rl, swm_region_list);
	}

	if (r_other == r)
		return;

	transfer_win(win, r_other->ws);
}

static void
update_win_refs(struct ws_win *win)
{
	struct workspace	*ws;
	struct ws_win		*w;

	RB_FOREACH(ws, workspace_tree, &win->s->workspaces)
		TAILQ_FOREACH(w, &ws->winlist, entry)
			if (w->transient_for == win->id)
				w->parent = win;

	RB_FOREACH(ws, workspace_tree, &win->s->workspaces)
		TAILQ_FOREACH(w, &ws->winlist, entry)
			w->main = find_main_window(w);
}

/* Determine a window to consider 'main' for specified window. */
static struct ws_win *
find_main_window(struct ws_win *win)
{
	struct ws_win	*w;
	int		i;

	if (win == NULL || win->parent == NULL)
		return (win);

	/* Resolve TRANSIENT_FOR as far as possible. */
	w = win;
	for (i = 0; w && w->parent && i < win->s->managed_count; i++) {
		w = w->parent;
		if (w == win)
			/* Transient loop shouldn't occur. */
			break;
	}

	if (w == NULL)
		w = win;

	DNPRINTF(SWM_D_MISC, "win %#x, count: %d, main: %#x\n", WINID(win),
	    win->s->managed_count, WINID(w));

	return (w);
}

static void
set_focus_redirect(struct ws_win *win)
{
	struct ws_win	*w;
	int		i;

	if (win == NULL || win->parent == NULL || win_noinput(win))
		return;

	/* Set focus_redirect along transient chain. */
	w = win;
	for (i = 0; w && w->parent && i < win->s->managed_count; i++) {
		/* Transient loop shouldn't occur. */
		if (w->parent == win)
			break;
		w->parent->focus_redirect = w;
		w = w->parent;
	}

	win->focus_redirect = NULL; /* Clear any redirect from this window. */
}

static void
win_to_ws(struct ws_win *win, struct workspace *nws, uint32_t flags)
{
	struct ws_win		*w, *tmpw;
	struct workspace	*ws;
	uint32_t		wsid;
	bool			focused;

	if (win == NULL || nws == NULL)
		return;

	ws = win->ws;
	if (ws == nws)
		return;

	DNPRINTF(SWM_D_MOVE, "win %#x, ws %d -> %d, focus: %#x, main: %#x\n",
	    win->id, ws->idx, nws->idx, WINID(ws->focus), WINID(win->main));

	wsid = (nws->idx >= 0 ? (uint32_t)(nws->idx) : EWMH_ALL_DESKTOPS);

	/* Transfer main window and any related transients. */
	TAILQ_FOREACH_SAFE(w, &ws->winlist, entry, tmpw) {
		if (win_related(w, win)) {
			focused = (ws->focus == w);
			if (focused) {
				ws->focus = get_focus_other(w);

				if (flags & SWM_WIN_UNFOCUS)
					unfocus_win(w);
			}

			/* Unmap if new ws is hidden. */
			if (!(flags & SWM_WIN_NOUNMAP) && nws->r == NULL)
				unmap_window(w);

			/* Transfer */
			TAILQ_REMOVE(&ws->winlist, w, entry);
			TAILQ_INSERT_TAIL(&nws->winlist, w, entry);
			w->ws = nws;

			if (focused)
				nws->focus = w;

			/* Cleanup references. */
			if (ws->focus == w)
				ws->focus = NULL;
			if (ws->focus_raise == w)
				ws->focus_raise = NULL;

			DNPRINTF(SWM_D_PROP, "win %#x, set property: "
			    "_NET_WM_DESKTOP: %d\n", w->id, wsid);
			xcb_change_property(conn, XCB_PROP_MODE_REPLACE,
			    w->id, ewmh[_NET_WM_DESKTOP].atom,
			    XCB_ATOM_CARDINAL, 32, 1, &wsid);
		}
	}

	ewmh_update_client_list(win->s);

	DNPRINTF(SWM_D_MOVE, "done\n");
}

static void
pressbutton(struct swm_screen *s, struct binding *bp, union arg *args)
{
	/* Suppress warning. */
	(void)s;
	(void)bp;

	xcb_test_fake_input(conn, XCB_BUTTON_PRESS, args->id,
	    XCB_CURRENT_TIME, XCB_WINDOW_NONE, 0, 0, 0);
	xcb_test_fake_input(conn, XCB_BUTTON_RELEASE, args->id,
	    XCB_CURRENT_TIME, XCB_WINDOW_NONE, 0, 0, 0);
}

static void
raise_focus(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win	*win;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	win = s->focus;
	if (win == NULL || win_raised(win))
		return;

	win->ws->focus_raise = win;
	update_win_layer(win);
	prioritize_window(win);
	refresh_stack(s);
	update_stacking(s);

	flush();
}

static void
raise_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	if ((r = get_current_region(s)) == NULL)
		return;

	if (r->ws->focus && MAXIMIZED(r->ws->focus))
		return;

	r->ws->always_raise = !r->ws->always_raise;

	/* Update focused win stacking order based on new always_raise value. */
	if (r->ws->focus) {
		update_win_layer(r->ws->focus);
		refresh_stack(s);
		update_stacking(s);
	}

	flush();
}

static void
iconify(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win, *nfw;
	bool			follow;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	win = s->focus;
	if (win == NULL)
		return;

	nfw = get_focus_other(win);
	unfocus_win(win);
	ewmh_apply_flags(win, win->ewmh_flags | EWMH_F_HIDDEN);
	ewmh_update_wm_state(win);

	if (nfw == NULL && win_free(win)) {
		if ((r = get_current_region(s)) == NULL)
			return;
		nfw = get_focus_magic(get_ws_focus(r->ws));
	}

	follow = follow_mode(SWM_FOCUS_TYPE_ICONIFY);
	if (!follow && nfw)
		set_focus(s, get_focus_magic(nfw));

	refresh_strut(s);
	stack(win->ws->r);
	update_mapping(s);
	if (!follow) {
		update_focus(win->s);
		center_pointer(win->ws->r);
	}

	flush(); /* win can be freed. */
	if (follow)
		focus_follow(s, s->r_focus, nfw);
}

static char *
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
		err(1, "get_win_name: strdup");

	free(r);

	return (name);
}

static void
uniconify(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win;
	FILE			*lfile;
	char			*name;
	int			count = 0;

	/* Suppress warnings. */
	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

	if ((r = get_current_region(s)) == NULL)
		return;

	/* make sure we have anything to uniconify */
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (win->ws == NULL)
			continue; /* should never happen */
		if (!HIDDEN(win))
			continue;
		count++;
	}

	/* Tack on 'free' wins. */
	TAILQ_FOREACH(win, &s->r->ws->winlist, entry) {
		if (win->ws == NULL)
			continue; /* should never happen */
		if (!HIDDEN(win))
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
		if (!HIDDEN(win))
			continue;

		name = get_win_name(win->id);
		fprintf(lfile, "%s.%u\n", name, win->id);
		free(name);
	}

	/* Tack on 'free' wins. */
	TAILQ_FOREACH(win, &s->r->ws->winlist, entry) {
		if (!HIDDEN(win))
			continue;

		name = get_win_name(win->id);
		fprintf(lfile, "%s.%u\n", name, win->id);
		free(name);
	}

	fclose(lfile);
}

static void
name_workspace(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	FILE			*lfile;

	/* Suppress warning. */
	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

	if ((r = get_current_region(s)) == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_NAME_WORKSPACE;

	spawn_select(r, args, "name_workspace", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	fprintf(lfile, "%s", "");
	fclose(lfile);
}

static void
search_workspace(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct workspace	*ws;
	int			i;
	FILE			*lfile;

	/* Suppress warning. */
	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

	if ((r = get_current_region(s)) == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WORKSPACE;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	for (i = 0; i < workspace_limit; ++i) {
		ws = workspace_lookup(s, i);

		fprintf(lfile, "%d%s%s\n", i + 1, ((ws && ws->name) ? ":" : ""),
		    ((ws && ws->name) ? ws->name : ""));
	}

	fclose(lfile);
}

static void
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

static int
create_search_win(struct ws_win *win, int index)
{
	struct search_window	*sw = NULL;
	xcb_window_t		w;
	uint32_t		wa[3];
	uint32_t		offset;
	int			width, height;
	char			str[11];
	size_t			len;
	XftDraw			*draw;
	XGlyphInfo		info;
	GC			l_draw;
	XGCValues		l_gcv;
	XRectangle		l_ibox, l_lbox = {0, 0, 0, 0};

	if ((sw = calloc(1, sizeof(struct search_window))) == NULL) {
		warn("search_win: calloc");
		return (1);
	}
	sw->idx = index;
	sw->win = win;

	snprintf(str, sizeof str, "%d", index);
	len = strlen(str);

	w = xcb_generate_id(conn);
	wa[0] = getcolorpixel(win->s, SWM_S_COLOR_FOCUS, 0);
	wa[1] = getcolorpixel(win->s, SWM_S_COLOR_UNFOCUS, 0);
	wa[2] = win->s->colormap;

	if (bar_font_legacy) {
		TEXTEXTENTS(bar_fs, str, len, &l_ibox, &l_lbox);
		width = l_lbox.width + 4;
		height = bar_fs_extents->max_logical_extent.height + 4;
	} else {
		XftTextExtentsUtf8(display, win->s->bar_xftfonts[0],
		    (FcChar8 *)str, len, &info);
		width = info.xOff + 4;
		height = win->s->bar_xftfonts[0]->height + 4;
	}

	offset = win_border(win);

	xcb_create_window(conn, win->s->depth, w, win->frame, offset,
	    offset, width, height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
	    win->s->visual, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
	    XCB_CW_COLORMAP, wa);

	xcb_map_window(conn, w);

	sw->indicator = w;
	TAILQ_INSERT_TAIL(&search_wl, sw, entry);

	if (bar_font_legacy) {
		l_gcv.graphics_exposures = 0;
		l_draw = XCreateGC(display, w, 0, &l_gcv);

		XSetForeground(display, l_draw,
			getcolorpixel(win->s, SWM_S_COLOR_BAR, 0));

		DRAWSTRING(display, w, bar_fs, l_draw, 2,
		    (bar_fs_extents->max_logical_extent.height -
		    l_lbox.height) / 2 - l_lbox.y + 2, str, len);

		XFreeGC(display, l_draw);
	} else {
		draw = XftDrawCreate(display, w, win->s->xvisual,
		    win->s->colormap);

		XftDrawStringUtf8(draw, getcolorxft(win->s, SWM_S_COLOR_BAR, 0),
		    win->s->bar_xftfonts[0], 2, height - 2 -
		    win->s->bar_xftfonts[0]->descent,
		    (FcChar8 *)str, len);

		XftDrawDestroy(draw);
	}

	DNPRINTF(SWM_D_MISC, "mapped win %#x\n", w);

	return (0);
}

static void
search_win(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win = NULL;
	int			i;
	FILE			*lfile;
	char			*title;

	/* Suppress warning. */
	(void)bp;

	DNPRINTF(SWM_D_MISC, "begin\n");

	if ((r = get_current_region(s)) == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WINDOW;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	i = 1;
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (HIDDEN(win))
			continue;

		if (create_search_win(win, i)) {
			fclose(lfile);
			search_win_cleanup();
			return;
		}

		title = get_win_name(win->id);
		fprintf(lfile, "%d%s%s\n", i, (title ? ":" : ""),
		    (title ? title : ""));
		free(title);
		i++;
	}
	/* Tack on 'free' wins. */
	TAILQ_FOREACH(win, &s->r->ws->winlist, entry) {
		if (HIDDEN(win))
			continue;

		if (create_search_win(win, i)) {
			fclose(lfile);
			search_win_cleanup();
			return;
		}

		title = get_win_name(win->id);
		fprintf(lfile, "%d%s%s\n", i, (title ? ":" : ""),
		    (title ? title : ""));
		free(title);
		i++;
	}

	fclose(lfile);

	xcb_flush(conn);
}

static void
search_resp_uniconify(const char *resp, size_t len)
{
	char			*name;
	struct ws_win		*win;
	struct swm_screen	*s;
	char			*str;
	bool			follow;

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);
	if (search_r == NULL)
		return;
	s = search_r->s;

	TAILQ_FOREACH(win, &search_r->ws->winlist, entry) {
		if (!HIDDEN(win))
			continue;
		name = get_win_name(win->id);
		if (asprintf(&str, "%s.%u", name, win->id) == -1) {
			free(name);
			continue;
		}
		free(name);
		if (strncmp(str, resp, len) == 0) {
			free(str);
			break;
		}
		free(str);
	}
	/* Try root ws. */
	if (win == NULL)
		TAILQ_FOREACH(win, &s->r->ws->winlist, entry) {
			if (!HIDDEN(win))
				continue;
			name = get_win_name(win->id);
			if (asprintf(&str, "%s.%u", name, win->id) == -1) {
				free(name);
				continue;
			}
			free(name);
			if (strncmp(str, resp, len) == 0) {
				free(str);
				break;
			}
			free(str);
		}

	if (win) {
		/* XXX this should be a callback to generalize */
		ewmh_apply_flags(win, win->ewmh_flags & ~EWMH_F_HIDDEN);
		ewmh_update_wm_state(win);

		set_focus_redirect(win);
		follow = follow_mode(SWM_FOCUS_TYPE_UNICONIFY);
		if (!follow)
			set_focus(s, get_focus_magic(win));

		apply_unfocus(win->ws, win);
		refresh_stack(s);
		update_stacking(s);
		refresh_strut(s);
		stack(win->ws->r);
		update_mapping(s);

		if (!follow) {
			update_focus(s);
			center_pointer(win->ws->r);
		}

		flush(); /* win can be freed. */
		if (follow)
			focus_follow(s, s->r_focus, win);

		if (validate_win(win) == 0) {
			draw_frame(win);
			debug_refresh(win);
		}
	}
}

static void
search_resp_name_workspace(const char *resp, size_t len)
{
	struct workspace	*ws;

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

	if (search_r->ws == NULL)
		return;
	ws = search_r->ws;

	if (ws->name) {
		free(ws->name);
		ws->name = NULL;
	}

	if (len) {
		ws->name = strdup(resp);
		if (ws->name == NULL) {
			DNPRINTF(SWM_D_MISC, "strdup: %s", strerror(errno));
			return;
		}
	}

	ewmh_update_desktop_names(search_r->s);
	ewmh_get_desktop_names(search_r->s);
	update_bars(search_r->s);
}

static void
ewmh_update_desktop_names(struct swm_screen *s)
{
	struct workspace	*ws;
	char			*name_list = NULL, *p;
	int			i;
	size_t			len = 0, tot = 0;

	for (i = 0; i < workspace_limit; ++i) {
		if ((ws = workspace_lookup(s, i)))
			if (ws->name)
				len += strlen(ws->name);
		++len;
	}

	if ((name_list = calloc(len, sizeof(char))) == NULL)
		err(1, "update_desktop_names: calloc");

	p = name_list;
	for (i = 0; i < workspace_limit; ++i) {
		if ((ws = workspace_lookup(s, i)) && ws->name) {
			len = strlen(ws->name);
			memcpy(p, ws->name, len);
		} else
			len = 0;

		p += len + 1;
		tot += len + 1;
	}

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
	    ewmh[_NET_DESKTOP_NAMES].atom, a_utf8_string, 8, tot, name_list);

	free(name_list);
}

static void
ewmh_get_desktop_names(struct swm_screen *s)
{
	struct workspace		*ws;
	xcb_get_property_reply_t	*gpr;
	xcb_get_property_cookie_t	gpc;
	int				i, n, k;
	char				*names = NULL;

	for (i = 0; i < workspace_limit; ++i) {
		if ((ws = workspace_lookup(s, i))) {
			free(ws->name);
			ws->name = NULL;
		}
	}

	gpc = xcb_get_property(conn, 0, s->root, ewmh[_NET_DESKTOP_NAMES].atom,
	    a_utf8_string, 0, UINT32_MAX);
	gpr = xcb_get_property_reply(conn, gpc, NULL);
	if (gpr == NULL)
		return;

	names = xcb_get_property_value(gpr);
	n = xcb_get_property_value_length(gpr);

	for (i = 0, k = 0; i < n; ++i) {
		if (*(names + i) != '\0') {
			if ((ws = get_workspace(s, k)))
				ws->name = strdup(names + i);
			i += strlen(names + i);
		}
		++k;
	}
	free(gpr);
}

static void
ewmh_update_client_list(struct swm_screen *s)
{
	struct ws_win		*w;
	struct workspace	*ws;
	xcb_window_t		*wins;
	int			i;

	DNPRINTF(SWM_D_PROP, "win count: %d\n", s->managed_count);

	if (s->managed_count == 0)
		return;

	wins = calloc(s->managed_count, sizeof(xcb_window_t));
	if (wins == NULL)
		err(1, "ewmh_update_client_list: calloc");

	/* Save workspace window order. */
	i = 0;
	RB_FOREACH(ws, workspace_tree, &s->workspaces)
		TAILQ_FOREACH(w, &ws->winlist, entry)
			wins[i++] = w->id;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
	    ewmh[_NET_CLIENT_LIST].atom, XCB_ATOM_WINDOW, 32, s->managed_count,
	    wins);
	free(wins);
}

static void
ewmh_update_current_desktop(struct swm_screen *s)
{
	struct swm_region	*r;
	uint32_t		val;

	if ((r = get_current_region(s)) == NULL)
		return;

	val = r->ws->idx;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
	    ewmh[_NET_CURRENT_DESKTOP].atom, XCB_ATOM_CARDINAL, 32, 1, &val);
}

static void
ewmh_update_number_of_desktops(struct swm_screen *s)
{
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
	    ewmh[_NET_NUMBER_OF_DESKTOPS].atom, XCB_ATOM_CARDINAL, 32, 1,
	    &workspace_limit);
}

static void
ewmh_update_desktop_viewports(struct swm_screen *s)
{
	uint32_t	vals[2];

	/* Always (0,0) since regions are never larger than root. */
	vals[0] = 0;
	vals[1] = 0;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
	    ewmh[_NET_DESKTOP_VIEWPORT].atom, XCB_ATOM_CARDINAL, 32, 2, &vals);
}

static void
ewmh_update_workarea(struct swm_screen *s)
{
	int			i;
	uint32_t		*vals;

	if ((vals = calloc(workspace_limit * 4, sizeof(uint32_t))) == NULL)
		err(1, "ewmh_update_workarea: calloc");

	DNPRINTF(SWM_D_MISC, "usable: x:%u y:%u w:%d h:%d\n", s->r->g_usable.x,
	    s->r->g_usable.y, s->r->g_usable.w, s->r->g_usable.h);

	/* The usable area of root applies to all desktops. */
	for (i = 0; i < workspace_limit; ++i) {
		vals[i * 4] = s->r->g_usable.x;
		vals[i * 4 + 1] = s->r->g_usable.y;
		vals[i * 4 + 2] = s->r->g_usable.w;
		vals[i * 4 + 3] = s->r->g_usable.h;
	}

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, s->root,
	    ewmh[_NET_WORKAREA].atom, XCB_ATOM_CARDINAL, 32,
	    workspace_limit * 4, vals);

	free(vals);
}

static void
search_resp_search_workspace(const char *resp)
{
	struct workspace	*ws;
	char			*p, *q;
	int			ws_idx, fail;

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

	q = strdup(resp);
	if (q == NULL) {
		DNPRINTF(SWM_D_MISC, "strdup: %s", strerror(errno));
		return;
	}
	p = strchr(q, ':');
	if (p != NULL)
		*p = '\0';
	ws_idx = strtoint32(q, 1, workspace_limit, &fail) - 1;
	if (fail) {
		DNPRINTF(SWM_D_MISC, "integer conversion failed for %s\n", q);
		free(q);
		return;
	}
	free(q);

	ws = get_workspace(search_r->s, ws_idx);
	if (ws)
		switch_workspace(search_r, ws, false,
		    follow_mode(SWM_FOCUS_TYPE_WORKSPACE));
}

static void
search_resp_search_window(const char *resp)
{
	char			*s, *p;
	int			idx, fail;
	struct search_window	*sw;

	DNPRINTF(SWM_D_MISC, "resp: %s\n", resp);

	s = strdup(resp);
	if (s == NULL) {
		DNPRINTF(SWM_D_MISC, "strdup: %s", strerror(errno));
		return;
	}
	p = strchr(s, ':');
	if (p != NULL)
		*p = '\0';
	idx = strtoint32(s, 1, INT_MAX, &fail);
	if (fail) {
		DNPRINTF(SWM_D_MISC, "integer conversion failed for %s\n", s);
		free(s);
		return;
	}
	free(s);

	TAILQ_FOREACH(sw, &search_wl, entry)
		if (idx == sw->idx) {
			focus_win(sw->win->s, sw->win);
			break;
		}
}

#define MAX_RESP_LEN	1024

static void
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
	DNPRINTF(SWM_D_MISC, "done\n");
}

static void
wkill(struct swm_screen *s, struct binding *bp, union arg *args)
{
	(void)bp;

	DNPRINTF(SWM_D_MISC, "win %#x, id: %d\n", WINID(s->focus), args->id);

	if (s->focus == NULL)
		return;

	if (args->id == SWM_ARG_ID_KILLWINDOW)
		xcb_kill_client(conn, s->focus->id);
	else
		if (s->focus->can_delete)
			client_msg(s->focus, a_delete, 0);

	xcb_flush(conn);
}

/* Apply unfocus conditions on windows in workspace unrelated to win. */
static int
apply_unfocus(struct workspace *ws, struct ws_win *win)
{
	struct ws_win		*w;
	int			count = 0;
	uint32_t		changed = 0;

	if (ws == NULL)
		goto out;

	DNPRINTF(SWM_D_MISC, "ws: %d\n", ws->idx);

	if (ws_maxstack(ws))
		goto out;

	if (maximized_unfocus == SWM_UNFOCUS_NONE &&
	    fullscreen_unfocus == SWM_UNFOCUS_NONE)
		goto out;

	TAILQ_FOREACH(w, &ws->winlist, entry) {
		if (win_related(w, win))
			continue;

		if (MAXIMIZED(w))
			switch (maximized_unfocus) {
			case SWM_UNFOCUS_RESTORE:
				changed |= ewmh_apply_flags(w,
				    w->ewmh_flags & ~EWMH_F_MAXIMIZED);
				break;
			case SWM_UNFOCUS_ICONIFY:
				changed |= ewmh_apply_flags(w,
				    w->ewmh_flags | EWMH_F_HIDDEN);
				break;
			case SWM_UNFOCUS_FLOAT:
				changed |= ewmh_apply_flags(w,
				    (w->ewmh_flags | EWMH_F_ABOVE) &
				    ~EWMH_F_MAXIMIZED);
				break;
			case SWM_UNFOCUS_BELOW:
				changed |= ewmh_apply_flags(w,
				    (w->ewmh_flags | EWMH_F_BELOW));
				break;
			case SWM_UNFOCUS_QUICK_BELOW:
				changed = 1;
				break;
			default:
				break;
			}

		if (FULLSCREEN(w))
			switch (fullscreen_unfocus) {
			case SWM_UNFOCUS_RESTORE:
				changed |= ewmh_apply_flags(w,
				    w->ewmh_flags & ~EWMH_F_FULLSCREEN);
				break;
			case SWM_UNFOCUS_ICONIFY:
				changed |= ewmh_apply_flags(w,
				    w->ewmh_flags | EWMH_F_HIDDEN);
				break;
			case SWM_UNFOCUS_FLOAT:
				changed |= ewmh_apply_flags(w,
				    (w->ewmh_flags | EWMH_F_ABOVE) &
				    ~EWMH_F_FULLSCREEN);
				break;
			case SWM_UNFOCUS_BELOW:
				changed |= ewmh_apply_flags(w,
				    (w->ewmh_flags | EWMH_F_BELOW));
				break;
			case SWM_UNFOCUS_QUICK_BELOW:
				changed = 1;
				break;
			default:
				break;
			}

		if (changed) {
			update_win_layer_related(w);
			ewmh_update_wm_state(w);
			changed = 0;
			++count;
		}
	}
out:
	return (count);
}

static void
free_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win;
	struct workspace	*nws;

	(void)bp;
	(void)args;

	win = s->focus;
	if (win == NULL)
		return;

	r = get_current_region(s);
	nws = (win_free(win) ? r->ws : s->r->ws);

	apply_unfocus(win->ws, win);

	if (!win_free(win)) {
		if (win_floating(win) && !FULLSCREEN(win) && !MAXIMIZED(win)) {
			win->g_float = win->g;
			update_gravity(win);
			/* Maintain original position. */
			win->g_float.x -= win->g_grav.x;
			win->g_float.y -= win->g_grav.y;
			win->g_floatref = nws->r->g;
			win->g_floatref_root = false;
		}
	}

	win_to_ws(win, nws, SWM_WIN_NOUNMAP);
	update_win_layer_related(win);
	refresh_stack(s);
	update_stacking(s);

	stack(r);
	if (nws->r != r)
		stack(nws->r);

	if (win_free(win))
		store_float_geom(win);

	update_mapping(s);
	draw_frame(win);
	bar_draw(r->bar);

	flush();
}

static void
maximize_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win;
	bool			follow;

	/* Suppress warnings. */
	(void)bp;
	(void)args;

	win = s->focus;
	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	if (FULLSCREEN(win) || WINDOCK(win) || WINDESKTOP(win))
		return;

	r = win->ws->r;

	ewmh_apply_flags(win, win->ewmh_flags ^ EWMH_F_MAXIMIZED);
	ewmh_update_wm_state(win);

	if (ws_maxstack(win->ws))
		win->maxstackmax = MAXIMIZED(win);

	apply_unfocus(win->ws, win);
	update_win_layer_related(win);

	refresh_stack(s);
	update_stacking(s);
	stack(win->ws->r);
	update_mapping(s);

	follow = follow_pointer(s, SWM_FOCUS_TYPE_CONFIGURE);
	if (!follow && win_focused(win)) {
		focus_win(s, win);
		center_pointer(win->ws->r);
	}

	flush();
	if (follow_mode(SWM_FOCUS_TYPE_CONFIGURE))
		focus_follow(s, r, NULL);

	DNPRINTF(SWM_D_MISC, "done\n");
}

static void
floating_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct swm_region	*r;
	struct ws_win		*win;
	uint32_t		newf;
	bool			follow;

	/* Suppress warnings. */
	(void)bp;
	(void)args;

	win = s->focus;
	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	r = win->ws->r;

	if ((ws_floating(win->ws) && !ws_root(win->ws) && !BELOW(win)) ||
	    FULLSCREEN(win) || win_transient(win))
		return;

	if (MAXIMIZED(win) || BELOW(win))
		newf = (win->ewmh_flags & ~EWMH_F_ABOVE);
	else
		newf = (win->ewmh_flags ^ EWMH_F_ABOVE);
	newf &= ~(EWMH_F_MAXIMIZED | EWMH_F_BELOW);

	if (ws_maxstack(win->ws))
		win->maxstackmax = MAXIMIZED(win);
	else if (ws_root(win->ws)) {
		if ((r = get_current_region(s))) {
			if (!ws_floating(r->ws))
				newf &= ~EWMH_F_ABOVE;
			win_to_ws(win, r->ws, SWM_WIN_NOUNMAP);
		}
	}

	ewmh_apply_flags(win, newf);
	ewmh_update_wm_state(win);
	update_win_layer_related(win);
	refresh_stack(s);
	update_stacking(s);
	stack(r);
	update_mapping(s);

	follow = follow_mode(SWM_FOCUS_TYPE_CONFIGURE);
	if (!follow && win_focused(win))
		focus_win(s, win);

	center_pointer(r);
	flush();
	if (follow)
		focus_follow(s, r, NULL);

	DNPRINTF(SWM_D_MISC, "done\n");
}

static void
fullscreen_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win		*win;

	/* Suppress warnings. */
	(void)bp;
	(void)args;

	win = s->focus;
	DNPRINTF(SWM_D_MISC, "win %#x\n", WINID(win));

	if (win == NULL)
		return;

	if (WINDOCK(win) || WINDESKTOP(win))
		return;

	ewmh_apply_flags(win, win->ewmh_flags ^ EWMH_F_FULLSCREEN);
	ewmh_update_wm_state(win);
	update_win_layer_related(win);

	refresh_stack(s);
	update_stacking(s);
	stack(win->ws->r);
	update_mapping(s);

	if (win == win->ws->focus)
		focus_win(s, win);

	center_pointer(win->ws->r);
	flush();
	DNPRINTF(SWM_D_MISC, "done\n");
}

static void
below_toggle(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win		*win;

	/* Suppress warning. */
	(void)bp;
	(void)args;

	win = s->focus;
	if (win == NULL)
		return;

	ewmh_apply_flags(win, win->ewmh_flags ^ EWMH_F_BELOW);
	ewmh_update_wm_state(win);
	update_win_layer_related(win);

	refresh_stack(s);
	update_stacking(s);
	stack(win->ws->r);
	update_mapping(s);

	if (win->ws->focus == win)
		focus_win(s, win);

	center_pointer(win->ws->r);
	flush();
	DNPRINTF(SWM_D_MISC, "done\n");
}

static bool
bounds_intersect(struct swm_geometry *b1, struct swm_geometry *b2)
{
	return (!(b1->x + b1->w < b2->x || b1->x > b2->x + b2->w ||
	    b1->y + b1->h < b2->y || b1->y > b2->y + b2->h));
}

static struct swm_geometry
get_boundary(struct ws_win *win)
{
	if (win->ws->r) {
		DNPRINTF(SWM_D_MISC, "r:%d\n", get_region_index(win->ws->r));
		return (win->ws->r->g);
	}
	if (win->ws->old_r) {
		DNPRINTF(SWM_D_MISC, "old r:%d\n",
		    get_region_index(win->ws->old_r));
		return (win->ws->old_r->g);
	}

	/* Workspace not mapped, use screen geometry. */
	DNPRINTF(SWM_D_MISC, "root r:%d\n", get_region_index(win->s->r));
	return (win->s->r->g);
}

/* Try to keep window within a boundary. Return true if window was contained, */
static bool
contain_window(struct ws_win *win, struct swm_geometry g, int bw, uint32_t opts)
{
	int				rt, lt, tp, bm;
	bool				contained = true;

	if (win == NULL)
		return (contained);

	if (!(opts & SWM_CW_SOFTBOUNDARY))
		bw = 0;

	/*
	 * Perpendicular distance of each side of the window to the respective
	 * side of the region boundary.  Positive values indicate the side of
	 * the window has passed beyond the region boundary.
	 */
	rt = (opts & SWM_CW_RIGHT) ? MAX_X(win) - (g.x + g.w) : bw;
	lt = (opts & SWM_CW_LEFT) ? g.x - X(win) : bw;
	bm = (opts & SWM_CW_BOTTOM) ? MAX_Y(win) - (g.y + g.h) : bw;
	tp = (opts & SWM_CW_TOP) ? g.y - Y(win) : bw;

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
		contained = false;
	}

	constrain_window(win, &g, &opts);

	return (contained);
}

/* Move or resize a window so that flagged side(s) fit into the supplied box. */
static void
constrain_window(struct ws_win *win, struct swm_geometry *b, uint32_t *opts)
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

static void
draw_frame(struct ws_win *win)
{
	xcb_point_t		points[5];
	uint32_t		gcv[2];

	if (win == NULL || border_width == 0)
		return;

	if (!win_reparented(win)) {
		DNPRINTF(SWM_D_EVENT, "win %#x not reparented\n", win->id);
		return;
	}

	if (!win->bordered) {
		DNPRINTF(SWM_D_EVENT, "win %#x frame disabled\n", win->id);
	}

	if (win_focused(win)) {
		if (win_free(win))
			gcv[0] = getcolorpixel(win->s, (MAXIMIZED(win) ?
			    SWM_S_COLOR_FOCUS_MAXIMIZED_FREE :
			    SWM_S_COLOR_FOCUS_FREE), 0);
		else
			gcv[0] = getcolorpixel(win->s, (MAXIMIZED(win) ?
			    SWM_S_COLOR_FOCUS_MAXIMIZED :
			    SWM_S_COLOR_FOCUS), 0);
	} else if (win_urgent(win)) {
		if (win_free(win))
			gcv[0] = getcolorpixel(win->s, (MAXIMIZED(win) ?
			    SWM_S_COLOR_URGENT_MAXIMIZED_FREE :
			    SWM_S_COLOR_URGENT_FREE), 0);
		else
			gcv[0] = getcolorpixel(win->s, (MAXIMIZED(win) ?
			    SWM_S_COLOR_URGENT_MAXIMIZED :
			    SWM_S_COLOR_URGENT), 0);
	} else {
		if (win_free(win))
			gcv[0] = getcolorpixel(win->s, (MAXIMIZED(win) ?
			    SWM_S_COLOR_UNFOCUS_MAXIMIZED_FREE :
			    SWM_S_COLOR_UNFOCUS_FREE), 0);
		else
			gcv[0] = getcolorpixel(win->s, (MAXIMIZED(win) ?
			    SWM_S_COLOR_UNFOCUS_MAXIMIZED :
			    SWM_S_COLOR_UNFOCUS), 0);
	}

	points[0].x = points[0].y = border_width / 2;
	points[1].x = border_width + WIDTH(win) + points[0].x;
	points[1].y = points[0].y;
	points[2].x = points[1].x;
	points[2].y = border_width + HEIGHT(win) + points[0].y;
	points[3].x = points[0].x;
	points[3].y = points[2].y;
	points[4] = points[0];
	gcv[1] = border_width;

	xcb_change_gc(conn, win->s->gc, XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH,
	    gcv);
	xcb_poly_line(conn, XCB_COORD_MODE_ORIGIN, win->frame, win->s->gc, 5,
	    points);
}

static void
update_window(struct ws_win *win)
{
	uint16_t	mask;
	uint32_t	wc[5];

	if (!win_reparented(win)) {
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

	DNPRINTF(SWM_D_EVENT, "win %#x (f:%#x), (x,y) w x h: (%d,%d) %d x %d,"
	    " bordered: %s\n", win->id, win->frame, wc[0], wc[1], wc[2], wc[3],
	    YESNO(win->bordered));

	xcb_configure_window(conn, win->frame, mask, wc);

	/* Reconfigure client window. */
	wc[0] = wc[1] = win_border(win);
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
}

struct event {
	STAILQ_ENTRY(event)	entry;
	xcb_generic_event_t	*ev;
};
STAILQ_HEAD(event_queue, event) events = STAILQ_HEAD_INITIALIZER(events);

static xcb_generic_event_t *
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

static void
put_back_event(xcb_generic_event_t *evt)
{
	struct event	*ep;
	if ((ep = malloc(sizeof (struct event))) == NULL)
		err(1, "put_back_event: malloc");
	ep->ev = evt;
	STAILQ_INSERT_HEAD(&events, ep, entry);
}

/* Peeks at next event to detect auto-repeat. */
static bool
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

static bool
keybindreleased(struct binding *bp, xcb_key_release_event_t *kre)
{
	if (bp->type == KEYBIND && !keyrepeating(kre) &&
		bp->value == xcb_key_press_lookup_keysym(syms, kre, 0))
		return (true);

	return (false);
}

#define SWM_RESIZE_STEPS	(50)

static void
resize_win(struct ws_win *win, struct binding *bp, int opt)
{
	struct swm_geometry		b;
	xcb_query_pointer_reply_t	*xpr = NULL;
	uint32_t			dir;
	bool				inplace = false, step = false;

	if (win == NULL)
		return;

	if (FULLSCREEN(win) || WINDESKTOP(win) || WINDOCK(win))
		return;

	b = get_boundary(win);

	DNPRINTF(SWM_D_EVENT, "win %#x, floating: %s, transient: %#x\n",
	    win->id, YESNO(ABOVE(win)), win->transient_for);

	/* Override floating geometry when resizing maximized windows. */
	if (MAXIMIZED(win) || ws_floating(win->ws)) {
		inplace = true;
	} else if (!(win_transient(win) || ABOVE(win)))
		return;

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
		unsnap_win(win, inplace);
		flush();

		/* It's possible for win to have been freed during flush(). */
		if (validate_win(win)) {
			DNPRINTF(SWM_D_EVENT, "invalid win\n");
			return;
		}

		contain_window(win, b, boundary_width,
		    SWM_CW_ALLSIDES | SWM_CW_RESIZABLE | SWM_CW_HARDBOUNDARY);
		update_window(win);
		store_float_geom(win);
		return;
	}

	contain_window(win, b, boundary_width,
	    SWM_CW_ALLSIDES | SWM_CW_RESIZABLE | SWM_CW_SOFTBOUNDARY);
	update_window(win);

	/* get cursor offset from window root */
	xpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win->id),
	    NULL);
	if (xpr == NULL)
		return;

	dir = SWM_SIZE_HORZ | SWM_SIZE_VERT;
	if (xpr->win_x < WIDTH(win) / 2)
		dir |= SWM_SIZE_HFLIP;
	if (xpr->win_y < HEIGHT(win) / 2)
		dir |= SWM_SIZE_VFLIP;

	resize_win_pointer(win, bp, xpr->root_x, xpr->root_y, dir,
	    (opt == SWM_ARG_ID_CENTER));
	free(xpr);
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
resize_win_pointer(struct ws_win *win, struct binding *bp,
    uint32_t x_root, uint32_t y_root, uint32_t dir, bool center)
{
	struct swm_geometry		g, b;
	xcb_cursor_t			cursor;
	xcb_generic_event_t		*evt;
	xcb_motion_notify_event_t	*mne;
	xcb_button_press_event_t	*bpe;
	xcb_key_press_event_t		*kpe;
	xcb_client_message_event_t	*cme;
	xcb_timestamp_t			timestamp = 0, mintime;
	int				dx, dy;
	bool				resizing;
	bool				inplace = false;

	if (MAXIMIZED(win) || ws_floating(win->ws))
		inplace = true;
	else if (!(win_transient(win) || ABOVE(win)))
		return;

	if (center)
		cursor = cursors[XC_SIZING].cid;
	else
		switch (dir) {
		case SWM_SIZE_TOPLEFT:
			cursor = cursors[XC_TOP_LEFT_CORNER].cid;
			break;
		case SWM_SIZE_TOP:
			cursor = cursors[XC_TOP_SIDE].cid;
			break;
		case SWM_SIZE_TOPRIGHT:
			cursor = cursors[XC_TOP_RIGHT_CORNER].cid;
			break;
		case SWM_SIZE_RIGHT:
			cursor = cursors[XC_RIGHT_SIDE].cid;
			break;
		case SWM_SIZE_BOTTOMRIGHT:
			cursor = cursors[XC_BOTTOM_RIGHT_CORNER].cid;
			break;
		case SWM_SIZE_BOTTOM:
			cursor = cursors[XC_BOTTOM_SIDE].cid;
			break;
		case SWM_SIZE_BOTTOMLEFT:
			cursor = cursors[XC_BOTTOM_LEFT_CORNER].cid;
			break;
		case SWM_SIZE_LEFT:
			cursor = cursors[XC_LEFT_SIDE].cid;
			break;
		default:
			cursor = cursors[XC_SIZING].cid;
			break;
		}

	xcb_grab_pointer(conn, 0, win->id, MOUSEMASK,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE, cursor,
	    XCB_CURRENT_TIME);

	/* Release keyboard freeze if called via keybind. */
	if (bp->type == KEYBIND)
		xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
		    XCB_CURRENT_TIME);

	/* Offer another means of termination, as recommended by the spec. */
	xcb_grab_key(conn, 0, win->id, XCB_MOD_MASK_ANY, cancel_keycode,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);

	unsnap_win(win, inplace);
	xcb_flush(conn);

	mintime = 1000 / win->s->rate;
	g = win->g;
	resizing = true;
	while (resizing && (evt = get_next_event(true))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_BUTTON_RELEASE:
			bpe = (xcb_button_press_event_t *)evt;
			event_time = bpe->time;
			if (bp->type == BTNBIND && (bp->value == bpe->detail ||
			    bp->value == XCB_BUTTON_INDEX_ANY))
				resizing = false;
			break;
		case XCB_KEY_RELEASE:
			kpe = (xcb_key_press_event_t *)evt;
			event_time = kpe->time;
			if (keybindreleased(bp, kpe))
				resizing = false;
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)evt;
			event_time = mne->time;
			DNPRINTF(SWM_D_EVENT, "MOTION_NOTIFY: root: %#x\n",
			    mne->root);

			/* cursor offset/delta from start of the operation */
			dx = mne->root_x - x_root;
			dy = mne->root_y - y_root;

			if (dir & SWM_SIZE_VERT) {
				if (dir & SWM_SIZE_VFLIP)
					dy = -dy;
				if (center) {
					if (g.h / 2 + dy < 1)
						dy = 1 - g.h / 2;
					Y(win) = g.y - dy;
					HEIGHT(win) = g.h + 2 * dy;
				} else {
					if (g.h + dy < 1)
						dy = 1 - g.h;
					if (dir & SWM_SIZE_VFLIP)
						Y(win) = g.y - dy;
					HEIGHT(win) = g.h + dy;
				}
			}
			if (dir & SWM_SIZE_HORZ) {
				if (dir & SWM_SIZE_HFLIP)
					dx = -dx;
				if (center) {
					if (g.w / 2 + dx < 1)
						dx = 1 - g.w / 2;
					X(win) = g.x - dx;
					WIDTH(win) = g.w + 2 * dx;
				} else {
					if (g.w + dx < 1)
						dx = 1 - g.w;
					if (dir & SWM_SIZE_HFLIP)
						X(win) = g.x - dx;
					WIDTH(win) = g.w + dx;
				}
			}
			update_gravity(win);

			/* Don't sync faster than the current rate limit. */
			if ((mne->time - timestamp) > mintime) {
				store_float_geom(win);
				timestamp = mne->time;
				regionize(win, mne->root_x, mne->root_y);

				b = get_boundary(win);
				contain_window(win, b, boundary_width,
				    SWM_CW_ALLSIDES | SWM_CW_RESIZABLE |
				    SWM_CW_HARDBOUNDARY | SWM_CW_SOFTBOUNDARY);
				update_window(win);
				xcb_flush(conn);
			}
			break;
		case XCB_BUTTON_PRESS:
			bpe = (xcb_button_press_event_t *)evt;
			event_time = bpe->time;
			/* Ignore. */
			DNPRINTF(SWM_D_EVENT, "BUTTON_PRESS ignored\n");
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_POINTER,
			    bpe->time);
			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
			kpe = (xcb_key_press_event_t *)evt;
			event_time = kpe->time;
			/* Handle cancel_key. */
			if ((xcb_key_press_lookup_keysym(syms, kpe, 0)) ==
			    cancel_key)
				resizing = false;

			/* Ignore. */
			DNPRINTF(SWM_D_EVENT, "KEY_PRESS ignored\n");
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    kpe->time);
			xcb_flush(conn);
			break;
		case XCB_CLIENT_MESSAGE:
			cme = (xcb_client_message_event_t *)evt;
			if (cme->type == ewmh[_NET_WM_MOVERESIZE].atom) {
				DNPRINTF(SWM_D_EVENT, "_NET_WM_MOVERESIZE\n");
				if (cme->data.data32[2] ==
				    EWMH_WM_MOVERESIZE_CANCEL)
					resizing = false;
			} else
				clientmessage(cme);
			break;
		default:
			/* Window can be freed or lose focus here. */
			event_handle(evt);

			if (validate_win(win)) {
				DNPRINTF(SWM_D_EVENT, "invalid win\n");
				goto out;
			}
			if (!win_focused(win)) {
				DNPRINTF(SWM_D_EVENT, "win lost focus\n");
				goto out;
			}
			break;
		}
		free(evt);
	}
	if (timestamp) {
		contain_window(win, b, boundary_width, SWM_CW_ALLSIDES |
		    SWM_CW_RESIZABLE | SWM_CW_HARDBOUNDARY |
		    SWM_CW_SOFTBOUNDARY);
		update_window(win);
	}
	store_float_geom(win);
out:
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	xcb_flush(conn);
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
resize(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win		*win = NULL;

	if (args->id != SWM_ARG_ID_DONTCENTER && args->id != SWM_ARG_ID_CENTER)
		/* keyboard resize uses the focus window. */
		win = s->focus;
	else
		/* mouse resize uses pointer window. */
		win = get_pointer_win(s);

	if (win == NULL)
		return;

	resize_win(win, bp, args->id);

	if (args->id && bp->type == KEYBIND)
		center_pointer(win->ws->r);

	flush();
}

/* Try to set window region based on supplied coordinates or window center. */
static void
regionize(struct ws_win *win, int x, int y)
{
	struct swm_region *r, *r_orig;

	if (win == NULL)
		return;

	r = region_under(win->s, x, y);
	if (r == NULL) {
		r = region_under(win->s, X(win) + WIDTH(win) / 2,
		    Y(win) + HEIGHT(win) / 2);
		if (r == NULL)
			return;
	}

	/* Only change focused region with ws-free windows. */
	if (win_free(win)) {
		set_region(r);
		update_bars(r->s);
		return;
	}

	if (r != win->ws->r) {
		apply_unfocus(r->ws, NULL);
		win->g_float = win->g;
		update_gravity(win);
		/* Maintain original position. */
		win->g_float.x -= win->g_grav.x;
		win->g_float.y -= win->g_grav.y;
		win->g_floatref = r->g;
		win->g_floatref_root = false;

		if (!ws_floating(r->ws))
			win->ewmh_flags |= EWMH_F_ABOVE;

		r_orig = win->ws->r;

		win_to_ws(win, r->ws, 0);
		set_region(r);

		update_win_layer_related(win);
		refresh_stack(win->s);
		update_stacking(win->s);

		/* Need to restack both regions. */
		stack(r_orig);
		stack(r);
		update_mapping(win->s);

		update_bars(r->s);

	}
}

static void
unsnap_win(struct ws_win *win, bool inplace)
{
	uint32_t	newf;

	DNPRINTF(SWM_D_MISC, "win %#x inplace: %s\n", WINID(win),
	    YESNO(inplace));

	if (inplace) {
		win->g_float = win->g;
		update_gravity(win);
		/* Maintain original position. */
		win->g_float.x -= win->g_grav.x;
		win->g_float.y -= win->g_grav.y;
		win->g_floatref = get_boundary(win);
		win->g_floatref_root = false;
	}

	newf = (win->ewmh_flags | SWM_F_MANUAL) & ~EWMH_F_MAXIMIZED;
	if (!ws_floating(win->ws) || win_free(win))
		newf |= EWMH_F_ABOVE;
	ewmh_apply_flags(win, newf);
	ewmh_update_wm_state(win);
	update_win_layer_related(win);

	refresh_stack(win->s);
	update_stacking(win->s);
	if (inplace) {
		stack(win->ws->r);
		update_mapping(win->s);
	}
}

#define SWM_MOVE_STEPS	(50)

static void
move_win(struct ws_win *win, struct binding *bp, int opt)
{
	xcb_query_pointer_reply_t	*qpr = NULL;
	bool				step = false, inplace;

	if (win == NULL)
		return;

	if (FULLSCREEN(win) || WINDESKTOP(win) || WINDOCK(win))
		return;

	DNPRINTF(SWM_D_EVENT, "win %#x, floating: %s, transient: %#x\n",
	    win->id, YESNO(ABOVE(win)), win->transient_for);

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
		inplace = (!win_floating(win) || MAXIMIZED(win));
		unsnap_win(win, inplace);
		flush();

		/* It's possible for win to have been freed during flush(). */
		if (validate_win(win)) {
			DNPRINTF(SWM_D_EVENT, "invalid win.\n");
			goto out;
		}

		regionize(win, -1, -1);
		contain_window(win, get_boundary(win), boundary_width,
		    SWM_CW_ALLSIDES);
		update_window(win);
		store_float_geom(win);
		return;
	}

	/* get cursor offset from window root */
	qpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win->id),
		NULL);
	if (qpr == NULL)
		return;

	move_win_pointer(win, bp, qpr->root_x, qpr->root_y);
	free(qpr);
out:
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
move_win_pointer(struct ws_win *win, struct binding *bp, uint32_t x_root,
    uint32_t y_root)
{
	struct swm_geometry		g_orig, b;
	xcb_generic_event_t		*evt;
	xcb_motion_notify_event_t	*mne;
	xcb_button_press_event_t	*bpe;
	xcb_key_press_event_t		*kpe;
	xcb_client_message_event_t	*cme;
	xcb_timestamp_t			timestamp = 0, mintime;
	int				dx, dy;
	bool				moving, snapped, inplace;

	xcb_grab_pointer(conn, 0, win->id, MOUSEMASK,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
	    XCB_WINDOW_NONE, cursors[XC_FLEUR].cid, XCB_CURRENT_TIME);

	/* Release keyboard freeze if called via keybind. */
	if (bp->type == KEYBIND)
		xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
		     XCB_CURRENT_TIME);

	/* Offer another means of termination, as recommended by the spec. */
	xcb_grab_key(conn, 0, win->id, XCB_MOD_MASK_ANY, cancel_keycode,
	    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);

	g_orig = win->g;
	b = get_boundary(win);
	inplace = (!win_floating(win) || MAXIMIZED(win));
	snapped = (snap_range && inplace);
	if (!snapped) {
		unsnap_win(win, inplace);
		regionize(win, x_root, y_root);
		contain_window(win, b, boundary_width,
		    SWM_CW_ALLSIDES | SWM_CW_SOFTBOUNDARY);
		update_window(win);
	}
	xcb_flush(conn);
	dx = x_root - X(win);
	dy = y_root - Y(win);
	mintime = 1000 / win->s->rate;
	moving = true;
	while (moving && (evt = get_next_event(true))) {
		switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_BUTTON_RELEASE:
			bpe = (xcb_button_press_event_t *)evt;
			event_time = bpe->time;
			if (bp->type == BTNBIND && (bp->value == bpe->detail ||
			    bp->value == XCB_BUTTON_INDEX_ANY))
				moving = false;
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_POINTER,
			    bpe->time);
			xcb_flush(conn);
			break;
		case XCB_KEY_RELEASE:
			kpe = (xcb_key_press_event_t *)evt;
			event_time = kpe->time;
			if (keybindreleased(bp, kpe))
				moving = false;
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    kpe->time);
			xcb_flush(conn);
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)evt;
			event_time = mne->time;
			DNPRINTF(SWM_D_EVENT, "MOTION_NOTIFY: root: %#x time: "
			    "%#x, root_x: %d, root_y: %d\n", mne->root,
			    mne->time, mne->root_x, mne->root_y);
			X(win) = mne->root_x - dx;
			Y(win) = mne->root_y - dy;

			/* Don't sync faster than the current rate limit. */
			if ((mne->time - timestamp) > mintime) {
				timestamp = mne->time;

				if (snapped &&
				    !contain_window(win, g_orig, snap_range,
				    SWM_CW_ALLSIDES | SWM_CW_SOFTBOUNDARY)) {
					unsnap_win(win, inplace);
					snapped = false;
				} else {
					regionize(win, mne->root_x,
							mne->root_y);
					b = get_boundary(win);
					contain_window(win, b, boundary_width,
					    SWM_CW_ALLSIDES |
					    SWM_CW_SOFTBOUNDARY);
				}

				update_window(win);
				xcb_flush(conn);
			}
			break;
		case XCB_BUTTON_PRESS:
			bpe = (xcb_button_press_event_t *)evt;
			event_time = bpe->time;
			/* Thaw and ignore. */
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_POINTER,
			    bpe->time);
			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
			kpe = (xcb_key_press_event_t *)evt;
			event_time = kpe->time;
			/* Handle cancel_key. */
			if ((xcb_key_press_lookup_keysym(syms, kpe, 0)) ==
			    cancel_key)
				moving = false;

			/* Ignore. */
			xcb_allow_events(conn, XCB_ALLOW_ASYNC_KEYBOARD,
			    kpe->time);
			xcb_flush(conn);
			break;
		case XCB_CLIENT_MESSAGE:
			cme = (xcb_client_message_event_t *)evt;
			if (cme->type == ewmh[_NET_WM_MOVERESIZE].atom) {
				DNPRINTF(SWM_D_EVENT, "_NET_WM_MOVERESIZE\n");
				if (cme->data.data32[2] ==
				    EWMH_WM_MOVERESIZE_CANCEL)
					moving = false;
			} else
				clientmessage(cme);
			break;
		default:
			/* Window can be freed or lose focus here. */
			event_handle(evt);

			if (validate_win(win)) {
				DNPRINTF(SWM_D_EVENT, "invalid win\n");
				goto out;
			}
			if (!win_focused(win)) {
				DNPRINTF(SWM_D_EVENT, "win lost focus\n");
				goto out;
			}
			break;
		}
		free(evt);
	}
	if (snapped) {
		contain_window(win, g_orig, snap_range, SWM_CW_ALLSIDES |
		    SWM_CW_SOFTBOUNDARY);
		update_window(win);
	} else if (timestamp) {
		b = get_boundary(win);
		contain_window(win, b, boundary_width,
		    SWM_CW_ALLSIDES | SWM_CW_SOFTBOUNDARY);
		update_window(win);
	}
	store_float_geom(win);
out:
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	xcb_flush(conn);
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
move(struct swm_screen *s, struct binding *bp, union arg *args)
{
	struct ws_win			*win = NULL;

	if (args->id) {
		/* Keyboard move uses the focus window. */
		win = s->focus;

		/* Disallow move_ on tiled. */
		if (win && !win_transient(win) && !ABOVE(win) &&
		    !ws_floating(win->ws))
			return;
	} else
		/* Mouse move uses the pointer window. */
		win = get_pointer_win(s);

	if (win == NULL)
		return;

	move_win(win, bp, args->id);

	if (args->id && bp->type == KEYBIND)
		center_pointer(win->ws->r);

	flush();
}

/* action definitions */
static struct action {
	char			name[SWM_FUNCNAME_LEN];
	void			(*func)(struct swm_screen *, struct binding *,
				    union arg *);
	uint32_t		flags;
	union arg		args;
} actions[FN_INVALID + 2] = {
	/* name			function	argument */
	{ "focus_free",		focus,		0, {.id = SWM_ARG_ID_FOCUSFREE} },
	{ "free_toggle",	free_toggle,	0, {0} },
	{ "bar_toggle",		bar_toggle,	0, {.id = SWM_ARG_ID_BAR_TOGGLE} },
	{ "bar_toggle_ws",	bar_toggle,	0, {.id = SWM_ARG_ID_BAR_TOGGLE_WS} },
	{ "button2",		pressbutton,	0, {.id = 2} },
	{ "cycle_layout",	switchlayout,	0, {.id = SWM_ARG_ID_CYCLE_LAYOUT} },
	{ "flip_layout",	stack_config,	0, {.id = SWM_ARG_ID_FLIPLAYOUT} },
	{ "float_toggle",	floating_toggle,0, {0} },
	{ "below_toggle",	below_toggle,	0, {0} },
	{ "focus",		focus_pointer,	0, {0} },
	{ "focus_main",		focus,		0, {.id = SWM_ARG_ID_FOCUSMAIN} },
	{ "focus_next",		focus,		0, {.id = SWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		0, {.id = SWM_ARG_ID_FOCUSPREV} },
	{ "focus_prior",	focus,		0, {.id = SWM_ARG_ID_FOCUSPRIOR} },
	{ "focus_urgent",	focus,		0, {.id = SWM_ARG_ID_FOCUSURGENT} },
	{ "fullscreen_toggle",	fullscreen_toggle, 0, {0} },
	{ "maximize_toggle",	maximize_toggle,0, {0} },
	{ "height_grow",	resize,		0, {.id = SWM_ARG_ID_HEIGHTGROW} },
	{ "height_shrink",	resize,		0, {.id = SWM_ARG_ID_HEIGHTSHRINK} },
	{ "iconify",		iconify,	0, {0} },
	{ "layout_vertical",	switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_VERTICAL} },
	{ "layout_horizontal",	switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_HORIZONTAL} },
	{ "layout_max",		switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_MAX} },
	{ "layout_floating",	switchlayout,	0, {.id = SWM_ARG_ID_LAYOUT_FLOATING} },
	{ "master_shrink",	stack_config,	0, {.id = SWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	0, {.id = SWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	0, {.id = SWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	0, {.id = SWM_ARG_ID_MASTERDEL} },
	{ "move",		move,		FN_F_NOREPLAY, {0} },
	{ "move_down",		move,		0, {.id = SWM_ARG_ID_MOVEDOWN} },
	{ "move_left",		move,		0, {.id = SWM_ARG_ID_MOVELEFT} },
	{ "move_right",		move,		0, {.id = SWM_ARG_ID_MOVERIGHT} },
	{ "move_up",		move,		0, {.id = SWM_ARG_ID_MOVEUP} },
	{ "mvrg_1",		send_to_rg,	0, {.id = 1} },
	{ "mvrg_2",		send_to_rg,	0, {.id = 2} },
	{ "mvrg_3",		send_to_rg,	0, {.id = 3} },
	{ "mvrg_4",		send_to_rg,	0, {.id = 4} },
	{ "mvrg_5",		send_to_rg,	0, {.id = 5} },
	{ "mvrg_6",		send_to_rg,	0, {.id = 6} },
	{ "mvrg_7",		send_to_rg,	0, {.id = 7} },
	{ "mvrg_8",		send_to_rg,	0, {.id = 8} },
	{ "mvrg_9",		send_to_rg,	0, {.id = 9} },
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
	{ "prior_layout",	switchlayout,	0, {.id = SWM_ARG_ID_PRIOR_LAYOUT} },
	{ "quit",		quit,		0, {0} },
	{ "raise",		raise_focus,	0, {0} },
	{ "raise_toggle",	raise_toggle,	0, {0} },
	{ "resize",		resize, FN_F_NOREPLAY, {.id = SWM_ARG_ID_DONTCENTER} },
	{ "resize_centered",	resize, FN_F_NOREPLAY, {.id = SWM_ARG_ID_CENTER} },
	{ "restart",		restart,	0, {0} },
	{ "restart_of_day",	restart,	0, {SWM_ARG_ID_RESTARTOFDAY} },
	{ "rg_1",		focusrg,	0, {.id = 1} },
	{ "rg_2",		focusrg,	0, {.id = 2} },
	{ "rg_3",		focusrg,	0, {.id = 3} },
	{ "rg_4",		focusrg,	0, {.id = 4} },
	{ "rg_5",		focusrg,	0, {.id = 5} },
	{ "rg_6",		focusrg,	0, {.id = 6} },
	{ "rg_7",		focusrg,	0, {.id = 7} },
	{ "rg_8",		focusrg,	0, {.id = 8} },
	{ "rg_9",		focusrg,	0, {.id = 9} },
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
	{ "debug_toggle",	debug_toggle,	0, {0} },
	{ "dumpwins",		dumpwins,	0, {0} },
	/* ALWAYS last: */
	{ "invalid action",	NULL,		0, {0} },
};

static void
update_modkey(uint16_t mod)
{
	struct binding		*bp;

	/* Replace all instances of the old mod key. */
	RB_FOREACH(bp, binding_tree, &bindings)
		if (bp->mod & mod_key)
			bp->mod = (bp->mod & ~mod_key) | mod;
	mod_key = mod;
}

static void
update_keycodes(void)
{
	if ((cancel_keycode = get_keysym_keycode(cancel_key)) == XCB_NO_SYMBOL)
		cancel_keycode = get_keysym_keycode(CANCELKEY);
}

static int
spawn_expand(struct swm_screen *s, struct spawn_prog *prog, int wsid,
    char ***ret_args)
{
	struct swm_region	*r;
	int			i, c;
	char			*ap, **real_args;

	r = get_current_region(s);

	DNPRINTF(SWM_D_SPAWN, "%s\n", prog->name);

	/* make room for expanded args */
	if ((real_args = calloc(prog->argc + 1, sizeof(char *))) == NULL)
		err(1, "spawn_custom: calloc real_args");

	/* Expand spawn args into real_args. */
	for (i = c = 0; i < prog->argc; i++) {
		ap = prog->argv[i];
		DNPRINTF(SWM_D_SPAWN, "raw arg: %s\n", ap);
		if (strcasecmp(ap, "$bar_border") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_BAR_BORDER, 0);
		} else if (strcasecmp(ap, "$bar_color") == 0) {
			real_args[c] = getcolorrgb(s, SWM_S_COLOR_BAR, 0);
		} else if (strcasecmp(ap, "$bar_color_selected") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_BAR_SELECTED, 0);
		} else if (strcasecmp(ap, "$bar_font") == 0) {
			if ((real_args[c] = strdup(bar_fonts)) == NULL)
				err(1, "spawn_custom: bar_fonts strdup");
		} else if (strcasecmp(ap, "$bar_font_color") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_BAR_FONT, 0);
		} else if (strcasecmp(ap, "$bar_font_color_selected") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_BAR_FONT_SELECTED, 0);
		} else if (strcasecmp(ap, "$color_focus_free") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_FOCUS_FREE, 0);
		} else if (strcasecmp(ap, "$color_focus_maximized_free") == 0) {
			real_args[c] = getcolorrgb(s,
			    SWM_S_COLOR_FOCUS_MAXIMIZED_FREE, 0);
		} else if (strcasecmp(ap, "$color_unfocus_free") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_UNFOCUS_FREE, 0);
		} else if (strcasecmp(ap,
		    "$color_unfocus_maximized_free") == 0) {
			real_args[c] = getcolorrgb(s,
			    SWM_S_COLOR_UNFOCUS_MAXIMIZED_FREE, 0);
		} else if (strcasecmp(ap, "$color_urgent_free") == 0) {
			real_args[c] =
			    getcolorrgb(r->s, SWM_S_COLOR_URGENT_FREE, 0);
		} else if (strcasecmp(ap,
		    "$color_urgent_maximized_free") == 0) {
			real_args[c] = getcolorrgb(r->s,
			    SWM_S_COLOR_URGENT_MAXIMIZED_FREE, 0);
		} else if (strcasecmp(ap, "$color_focus") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_FOCUS, 0);
		} else if (strcasecmp(ap, "$color_focus_maximized") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_FOCUS_MAXIMIZED, 0);
		} else if (strcasecmp(ap, "$color_unfocus") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_UNFOCUS, 0);
		} else if (strcasecmp(ap, "$color_unfocus_maximized") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_UNFOCUS_MAXIMIZED, 0);
		} else if (strcasecmp(ap, "$color_urgent") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_URGENT, 0);
		} else if (strcasecmp(ap, "$color_urgent_maximized") == 0) {
			real_args[c] =
			    getcolorrgb(s, SWM_S_COLOR_URGENT_MAXIMIZED, 0);
		} else if (strcasecmp(ap, "$region_index") == 0) {
			if (asprintf(&real_args[c], "%d",
			    get_region_index(r)) < 1)
				err(1, "spawn_custom: region_index asprintf");
		} else if (strcasecmp(ap, "$workspace_index") == 0) {
			if (asprintf(&real_args[c], "%d", wsid + 1) < 1)
				err(1, "spawn_custom: workspace_index "
				    "asprintf");
		} else if (strcasecmp(ap, "$dmenu_bottom") == 0) {
			if (!bar_at_bottom)
				continue;
			if ((real_args[c] = strdup("-b")) == NULL)
				err(1, "spawn_custom: dmenu_bottom strdup");
		} else {
			/* no match --> copy as is */
			if ((real_args[c] = strdup(ap)) == NULL)
				err(1, "spawn_custom: arg strdup");
		}
		DNPRINTF(SWM_D_SPAWN, "cooked arg: %s\n", real_args[c]);
		++c;
	}

	if (swm_debug & SWM_D_SPAWN) {
		DPRINTF("result: ");
		for (i = 0; i < c; ++i)
			DPRINTF("\"%s\" ", real_args[i]);
		DPRINTF("\n");
	}

	*ret_args = real_args;
	return (c);
}

static void
spawn_custom(struct swm_screen *s, union arg *args, const char *spawn_name)
{
	struct spawn_prog	*prog;
	struct swm_region	*r;
	union arg		a;
	char			**real_args;
	unsigned int		flags;
	int			spawn_argc, i, wsid;

	(void)args;

	if (s == NULL)
		return;

	if ((prog = spawn_find(spawn_name)) == NULL) {
		warnx("spawn_custom: program %s not found", spawn_name);
		return;
	}

	r = get_current_region(s);
	wsid = r->ws->idx;

	if ((spawn_argc = spawn_expand(s, prog, wsid, &real_args)) < 0)
		return;

	a.argv = real_args;

	flags = prog->flags | SWM_SPAWN_CLOSE_FD;
	if (!(flags & SWM_SPAWN_NOSPAWNWS))
		flags |= SWM_SPAWN_WS;

	if (fork() == 0)
		spawn(wsid, &a, flags);

	for (i = 0; i < spawn_argc; i++)
		free(real_args[i]);
	free(real_args);
}

static void
spawn_select(struct swm_region *r, union arg *args, const char *spawn_name,
    int *pid)
{
	struct spawn_prog	*prog;
	union arg		a;
	char			**real_args;
	int			i, spawn_argc, wsid;

	(void)args;

	wsid = r->ws->idx;

	if ((prog = spawn_find(spawn_name)) == NULL) {
		warnx("spawn_select: program %s not found", spawn_name);
		return;
	}

	if ((spawn_argc = spawn_expand(r->s, prog, wsid, &real_args)) < 0)
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
		if (dup2(select_list_pipe[0], STDIN_FILENO) == -1) {
			warn("spawn_select: dup2");
			_exit(1);
		}
		if (dup2(select_resp_pipe[1], STDOUT_FILENO) == -1) {
			warn("spawn_select: dup2");
			_exit(1);
		}
		close(select_list_pipe[1]);
		close(select_resp_pipe[0]);
		spawn(wsid, &a, 0);
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

static void
spawn_insert(const char *name, const char *args, unsigned int flags)
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

	if (sp->argv != NULL) {
		DNPRINTF(SWM_D_SPAWN, "arg %d: [%s]\n", sp->argc,
		    sp->argv[sp->argc-1]);
	}

	TAILQ_INSERT_TAIL(&spawns, sp, entry);
	DNPRINTF(SWM_D_SPAWN, "leave\n");
}

static void
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

static void
clear_spawns(void)
{
	struct spawn_prog	*sp;
#ifndef __clang_analyzer__ /* Suppress false warnings. */
	while ((sp = TAILQ_FIRST(&spawns)) != NULL) {
		spawn_remove(sp);
	}
#endif
}

static struct spawn_prog *
spawn_find(const char *name)
{
	struct spawn_prog	*sp;

	TAILQ_FOREACH(sp, &spawns, entry)
		if (strcasecmp(sp->name, name) == 0)
			return (sp);

	return (NULL);
}

static void
setspawn(const char *name, const char *args, unsigned int flags)
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

static int
asopcheck(uint8_t asop, uint8_t allowed, char **emsg)
{
	if (!(asop & allowed)) {
		switch (asop) {
		case SWM_ASOP_ADD:
			ALLOCSTR(emsg, "'+=' cannot be used with this option");
			break;
		case SWM_ASOP_SUBTRACT:
			ALLOCSTR(emsg, "'-=' cannot be used with this option");
			break;
		case SWM_ASOP_BASIC:
			ALLOCSTR(emsg, "'=' cannot be used with this option");
			break;
		default:
			ALLOCSTR(emsg, "invalid assignment operator");
			break;
		}
		return (1);
	}
	return (0);
}

static int
regcompopt(regex_t *preg, const char *regex) {
	int	ret;
	char	*str;

	if (asprintf(&str, "^%s$", regex) == -1)
		err(1, "regcompopt: asprintf");
	ret = regcomp(preg, str, REG_EXTENDED | REG_NOSUB);
	free(str);

	return (ret);
}

static char *
cleanopt(char *str)
{
	char	*p;

	/* Trim leading/trailing whitespace. */
	str += strspn(str, SWM_CONF_WHITESPACE);
	p = str + strlen(str) - 1;
	while (p > str && strchr(SWM_CONF_WHITESPACE, *p))
		*p-- = '\0';

	return (str);
}

static struct spawn_flag {
	char		*name;
	unsigned int	mask;
} spawnflags[] = {
	{"none",		0},
	{"optional",		SWM_SPAWN_OPTIONAL},
	{"nospawnws",		SWM_SPAWN_NOSPAWNWS},
	{"xterm_fontadj",	SWM_SPAWN_XTERM_FONTADJ},
};

static int
parse_spawn_flags(const char *str, uint32_t *flags, char **emsg)
{
	char			*tmp, *cp, *name;
	int			i, count;

	if (str == NULL || flags == NULL)
		return (1);

	if ((cp = tmp = strdup(str)) == NULL)
		err(1, "parse_spawn_flags: strdup");

	*flags = 0;
	count = 0;
	while ((name = strsep(&cp, SWM_CONF_DELIMLIST)) != NULL) {
		name = cleanopt(name);
		if (*name == '\0')
			continue;

		for (i = 0; i < LENGTH(spawnflags); i++) {
			if (strcmp(name, spawnflags[i].name) == 0) {
				DNPRINTF(SWM_D_CONF, "flag: [%s]\n", name);
				*flags |= spawnflags[i].mask;
				break;
			}
		}
		if (i >= LENGTH(spawnflags)) {
			ALLOCSTR(emsg, "invalid spawn flag: %s", name);
			DNPRINTF(SWM_D_CONF, "invalid spawn flag: [%s]\n",
			    name);
			free(tmp);
			return (1);
		}
		count++;
	}

	if (count == 0) {
		ALLOCSTR(emsg, "missing spawn flag");
		free(tmp);
		return(1);
	}

	free(tmp);
	return (0);
}

static int
setconfspawnflags(uint8_t asop, const char *selector, const char *value,
    int flags, char **emsg)
{
	struct spawn_prog	*sp = NULL;
	unsigned int		sflags;
	regex_t			regex_name;
	int			count;

	/* Suppress warning. */
	(void)flags;

	if (asopcheck(asop, SWM_ASOP_BASIC | SWM_ASOP_ADD | SWM_ASOP_SUBTRACT,
	    emsg))
		return (1);

	/* If no selector, set default spawn flags. */
	if (selector == NULL || strlen(selector) == 0) {
		if (parse_spawn_flags(value, &sflags, emsg))
			return (1);

		switch (asop) {
		case SWM_ASOP_ADD:
			spawn_flags |= sflags;
			break;
		case SWM_ASOP_SUBTRACT:
			spawn_flags &= ~sflags;
			break;
		case SWM_ASOP_BASIC:
		default:
			spawn_flags = sflags;
			break;
		}

		DNPRINTF(SWM_D_KEY, "set spawn_flags: %#x\n", spawn_flags);
		return (0);
	}

	/* Otherwise, search for spawn entries and set their spawn flags. */
	if (regcompopt(&regex_name, selector)) {
		ALLOCSTR(emsg, "invalid regex: %s", selector);
		return (1);
	}

	if (parse_spawn_flags(value, &sflags, emsg))
		return (1);

	count = 0;
	TAILQ_FOREACH(sp, &spawns, entry) {
		if (regexec(&regex_name, sp->name, 0, NULL, 0) == 0) {
			switch (asop) {
			case SWM_ASOP_ADD:
				sp->flags |= sflags;
				break;
			case SWM_ASOP_SUBTRACT:
				sp->flags &= ~sflags;
				break;
			case SWM_ASOP_BASIC:
			default:
				sp->flags = sflags;
				break;
			}

			DNPRINTF(SWM_D_KEY, "set %s flags: %#x\n", sp->name,
			    sp->flags);
			count++;
		}
	}
	regfree(&regex_name);

	if (count == 0) {
		ALLOCSTR(emsg, "program entry not found: %s", selector);
		return (1);
	}

	return (0);
}

static int
setconfspawn(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	char		*name, *args, *str;

	/* Suppress warning. */
	(void)flags;

	if (selector == NULL || strlen(selector) == 0) {
		ALLOCSTR(emsg, "missing selector");
		return (1);
	}

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	if ((str = strdup(selector)) == NULL)
		err(1, "setconfspawn: strdup");

	name = str;
	unescape_selector(name);

	args = expand_tilde(value);

	DNPRINTF(SWM_D_SPAWN, "[%s] flags:%#x [%s]\n", name, spawn_flags, args);

	setspawn(name, args, spawn_flags);
	free(str);
	free(args);

	DNPRINTF(SWM_D_SPAWN, "done\n");
	return (0);
}

static void
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

static void
setup_spawn(void)
{
	setconfspawn(SWM_ASOP_BASIC, "lock", "xlock", 0, NULL);
	setconfspawn(SWM_ASOP_BASIC, "term", "xterm", 0, NULL);
	setconfspawnflags(SWM_ASOP_BASIC, "term", "xterm_fontadj", 0, NULL);
	setconfspawn(SWM_ASOP_BASIC, "spawn_term", "xterm", 0, NULL);
	setconfspawnflags(SWM_ASOP_BASIC, "spawn_term", "xterm_fontadj", 0,
	    NULL);

	setconfspawn(SWM_ASOP_BASIC, "menu", "dmenu_run $dmenu_bottom "
	    "-fn $bar_font -nb $bar_color -nf $bar_font_color "
	    "-sb $bar_color_selected -sf $bar_font_color_selected", 0, NULL);

	setconfspawn(SWM_ASOP_BASIC, "search", "dmenu $dmenu_bottom -i "
	    "-fn $bar_font -nb $bar_color -nf $bar_font_color "
	    "-sb $bar_color_selected -sf $bar_font_color_selected", 0, NULL);

	setconfspawn(SWM_ASOP_BASIC, "name_workspace", "dmenu $dmenu_bottom "
	    "-p Workspace -fn $bar_font -nb $bar_color -nf $bar_font_color "
	    "-sb $bar_color_selected -sf $bar_font_color_selected", 0, NULL);

	 /* These are not verified for existence, even with a binding set. */
	setconfspawn(SWM_ASOP_BASIC, "screenshot_all", "screenshot.sh full", 0,
	    NULL);
	setconfspawnflags(SWM_ASOP_BASIC, "screenshot_all", "optional", 0,
	    NULL);
	setconfspawn(SWM_ASOP_BASIC, "screenshot_wind", "screenshot.sh window",
	    0, NULL);
	setconfspawnflags(SWM_ASOP_BASIC, "screenshot_wind", "optional", 0,
	    NULL);
	setconfspawn(SWM_ASOP_BASIC, "initscr", "initscreen.sh", 0, NULL);
	setconfspawnflags(SWM_ASOP_BASIC, "initscr", "optional", 0, NULL);
}

static char *
trimopt(char *str)
{
	char	*p;

	/* Trim leading/trailing whitespace. */
	str += strspn(str, SWM_CONF_WHITESPACE);
	p = str + strlen(str) - 1;
	while (p > str && strchr(SWM_CONF_WHITESPACE, *p))
		*p-- = '\0';

	return (str);
}

/* bindings */
#define SWM_MODNAME_SIZE	32
#define SWM_KEY_WS		"\n+ \t"
static int
parsebinding(const char *bindstr, uint16_t *mod, enum binding_type *type,
    uint32_t *val, uint32_t *flags, char **emsg)
{
	char			*str, *cp, *name;
	xcb_keysym_t		ks;

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
			if ((ks = get_string_keysym(name)) == XCB_NO_SYMBOL) {
				ALLOCSTR(emsg, "invalid key: %s", name);
				free(str);
				return (1);
			}

			*val = ks;
		}
	}

	/* If ANYMOD was specified, ignore the rest. */
	if (*mod & XCB_MOD_MASK_ANY)
		*mod = XCB_MOD_MASK_ANY;

	free(str);
	DNPRINTF(SWM_D_KEY, "leave\n");
	return (0);
}

static char *
strdupsafe(const char *str)
{
	if (str == NULL)
		return (NULL);
	else
		return (strdup(str));
}

static int
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

static void
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
	if (RB_INSERT(binding_tree, &bindings, bp))
		errx(1, "binding_insert: RB_INSERT");

	DNPRINTF(SWM_D_KEY, "leave\n");
}

static struct binding *
binding_lookup(uint16_t mod, enum binding_type type, uint32_t val)
{
	struct binding		bp;

	bp.mod = mod;
	bp.type = type;
	bp.value = val;

	return (RB_FIND(binding_tree, &bindings, &bp));
}

static void
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

static void
setbinding(uint16_t mod, enum binding_type type, uint32_t val,
    enum actionid aid, uint32_t flags, const char *spawn_name)
{
	struct binding		*bp;

	if (spawn_name != NULL) {
		DNPRINTF(SWM_D_KEY, "enter %s [%s]\n", actions[aid].name,
		    spawn_name);
	}

	/* Unbind any existing. Loop is to handle MOD_MASK_ANY. */
	while ((bp = binding_lookup(mod, type, val)))
		binding_remove(bp);

	if (aid != FN_INVALID)
		binding_insert(mod, type, val, aid, flags, spawn_name);

	DNPRINTF(SWM_D_KEY, "leave\n");
}

static int
setconfbinding(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	struct spawn_prog	*sp;
	uint32_t		keybtn, opts;
	uint16_t		mod;
	enum actionid		aid;
	enum binding_type	type;

	/* Suppress warning. */
	(void)flags;

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

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
static void
setup_keybindings(void)
{
#define BINDKEY(m, k, a)	setbinding(m, KEYBIND, k, a, 0, NULL)
#define BINDKEYSPAWN(m, k, s)	setbinding(m, KEYBIND, k, FN_SPAWN_CUSTOM, 0, s)
	BINDKEY(MOD,		XK_grave,		FN_FOCUS_FREE);
	BINDKEY(MODSHIFT,	XK_grave,		FN_FREE_TOGGLE);
	BINDKEY(MOD,		XK_b,			FN_BAR_TOGGLE);
	BINDKEY(MODSHIFT,	XK_b,			FN_BAR_TOGGLE_WS);
	BINDKEY(MOD,		XK_v,			FN_BUTTON2);
	BINDKEY(MOD,		XK_space,		FN_CYCLE_LAYOUT);
	BINDKEY(MODSHIFT,	XK_backslash,		FN_FLIP_LAYOUT);
	BINDKEY(MOD,		XK_t,			FN_FLOAT_TOGGLE);
	BINDKEY(MODSHIFT,	XK_t,			FN_BELOW_TOGGLE);
	BINDKEY(MOD,		XK_m,			FN_FOCUS_MAIN);
	BINDKEY(MOD,		XK_j,			FN_FOCUS_NEXT);
	BINDKEY(MOD,		XK_Tab,			FN_FOCUS_NEXT);
	BINDKEY(MOD,		XK_k,			FN_FOCUS_PREV);
	BINDKEY(MODSHIFT,	XK_Tab,			FN_FOCUS_PREV);
	BINDKEY(MODSHIFT,	XK_a,			FN_FOCUS_PRIOR);
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
	if (swm_debug) {
		BINDKEY(MOD,		XK_d,		FN_DEBUG_TOGGLE);
		BINDKEY(MODSHIFT,	XK_d,		FN_DUMPWINS);
	}
#undef BINDKEY
#undef BINDKEYSPAWN
}

static void
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

static void
clear_bindings(void)
{
	struct binding		*bp;

	while ((bp = RB_ROOT(&bindings)))
		binding_remove(bp);
}

static void
clear_keybindings(void)
{
	struct binding		*bp, *bptmp;

	RB_FOREACH_SAFE(bp, binding_tree, &bindings, bptmp) {
		if (bp->type != KEYBIND)
			continue;
		binding_remove(bp);
	}
}

static bool
button_has_binding(uint32_t button)
{
	struct binding		b, *bp;

	b.type = BTNBIND;
	b.value = button;
	b.mod = 0;

	bp = RB_NFIND(binding_tree, &bindings, &b);

	return (bp && bp->type == BTNBIND && bp->value == button);
}

static int
setkeymapping(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	char			*keymapping_file;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	DNPRINTF(SWM_D_KEY, "enter\n");

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

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

static void
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

static xcb_keysym_t
get_string_keysym(const char *name)
{
	KeySym					ks, lks, uks;

	/* TODO: do this without Xlib. */
	ks = XStringToKeysym(name);
	if (ks == NoSymbol) {
		DNPRINTF(SWM_D_KEY, "invalid key %s\n", name);
		return (XCB_NO_SYMBOL);
	}

	XConvertCase(ks, &lks, &uks);

	return ((xcb_keysym_t)lks);
}

static xcb_keycode_t
get_keysym_keycode(xcb_keysym_t ks)
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
			if (xcb_key_symbols_get_keysym(syms, kc, col) == ks) {
				free(kmr);
				return (kc);
			}
		}
	}
	free(kmr);

	return (XCB_NO_SYMBOL);
}

static void
grabkeys(void)
{
	struct binding		*bp;
	int			num_screens, i, j;
	uint16_t		modifiers[4];
	xcb_keycode_t		keycode;

	DNPRINTF(SWM_D_MISC, "begin\n");
	updatenumlockmask();
	update_keycodes();

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
			keycode = get_keysym_keycode(bp->value);
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

#ifdef SWM_XCB_HAS_XINPUT
static const char *
get_input_event_label(xcb_ge_generic_event_t *ev)
{
	char *label;

	switch (ev->event_type) {
	case XCB_INPUT_DEVICE_CHANGED:
		label = "DeviceChanged";
		break;
	case XCB_INPUT_KEY_PRESS:
		label = "KeyPress";
		break;
	case XCB_INPUT_KEY_RELEASE:
		label = "KeyRelease";
		break;
	case XCB_INPUT_BUTTON_PRESS:
		label = "ButtonPress";
		break;
	case XCB_INPUT_BUTTON_RELEASE:
		label = "ButtonRelease";
		break;
	case XCB_INPUT_MOTION:
		label = "Motion";
		break;
	case XCB_INPUT_ENTER:
		label = "Enter";
		break;
	case XCB_INPUT_LEAVE:
		label = "Leave";
		break;
	case XCB_INPUT_FOCUS_IN:
		label = "FocusIn";
		break;
	case XCB_INPUT_FOCUS_OUT:
		label = "FocusOut";
		break;
	case XCB_INPUT_HIERARCHY:
		label = "Hierarchy";
		break;
	case XCB_INPUT_PROPERTY:
		label = "Property";
		break;
#ifdef XCB_INPUT_RAW_KEY_PRESS
	/* XI >= 2.1 */
	case XCB_INPUT_RAW_KEY_PRESS:
		label = "RawKeyPress";
		break;
	case XCB_INPUT_RAW_KEY_RELEASE:
		label = "RawKeyRelease";
		break;
	case XCB_INPUT_RAW_BUTTON_PRESS:
		label = "RawButtonPress";
		break;
	case XCB_INPUT_RAW_BUTTON_RELEASE:
		label = "RawButtonRelease";
		break;
	case XCB_INPUT_RAW_MOTION:
		label = "RawMotion";
		break;
#ifdef XCB_INPUT_TOUCH_BEGIN
	/* XI >= 2.2 */
	case XCB_INPUT_TOUCH_BEGIN:
		label = "TouchBegin";
		break;
	case XCB_INPUT_TOUCH_UPDATE:
		label = "TouchUpdate";
		break;
	case XCB_INPUT_TOUCH_END:
		label = "TouchEnd";
		break;
	case XCB_INPUT_TOUCH_OWNERSHIP:
		label = "TouchOwnership";
		break;
	case XCB_INPUT_RAW_TOUCH_BEGIN:
		label = "RawTouchBegin";
		break;
	case XCB_INPUT_RAW_TOUCH_UPDATE:
		label = "RawTouchUpdate";
		break;
	case XCB_INPUT_RAW_TOUCH_END:
		label = "RawTouchEnd";
		break;
#ifdef XCB_INPUT_BARRIER_HIT
	/* XI >= 2.3 */
	case XCB_INPUT_BARRIER_HIT:
		label = "BarrierHit";
		break;
	case XCB_INPUT_BARRIER_LEAVE:
		label = "BarrierLeave";
		break;
#ifdef XCB_INPUT_GESTURE_PINCH_BEGIN
	/* XI >= 2.4 */
	case XCB_INPUT_GESTURE_PINCH_BEGIN:
		label = "GesturePinchBegin";
		break;
	case XCB_INPUT_GESTURE_PINCH_UPDATE:
		label = "GesturePinchUpdate";
		break;
	case XCB_INPUT_GESTURE_PINCH_END:
		label = "GesturePinchEnd";
		break;
	case XCB_INPUT_GESTURE_SWIPE_BEGIN:
		label = "GestureSwipeBegin";
		break;
	case XCB_INPUT_GESTURE_SWIPE_UPDATE:
		label = "GestureSwipeUpdate";
		break;
	case XCB_INPUT_GESTURE_SWIPE_END:
		label = "GestureSwipeEnd";
		break;
#endif /* XCB_INPUT_GESTURE_PINCH_BEGIN */
#endif /* XCB_INPUT_BARRIER_HIT */
#endif /* XCB_INPUT_TOUCH_BEGIN */
#endif /* XCB_INPUT_RAW_KEY_PRESS */
	default:
		label = "Unknown";
	}

	return (label);
}
#endif /* SWM_XCB_HAS_XINPUT */

#if defined(SWM_XCB_HAS_XINPUT) && defined(XCB_INPUT_RAW_BUTTON_PRESS)
static void
setup_xinput2(struct swm_screen *s)
{
	xcb_void_cookie_t	ck;
	xcb_generic_error_t	*error;

	if (!xinput2_support || !xinput2_raw)
		return;

	struct {
		xcb_input_event_mask_t	head;
		uint32_t		val;
	} masks;

	masks.head.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
	masks.head.mask_len = 1;
	masks.val = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS |
		XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_PRESS;

	ck = xcb_input_xi_select_events_checked(conn, s->root, 1, &masks.head);
	if ((error = xcb_request_check(conn, ck))) {
		DNPRINTF(SWM_D_INIT, "xi2 error_code: %u\n",
		    error->error_code);
		free(error);
	}
}

static void
rawbuttonpress(xcb_input_raw_button_press_event_t *e)
{
	struct swm_screen		*s;
	struct binding			*bp;
	struct action			*ap;
	xcb_query_pointer_reply_t	*qpr;

	DPRINTF("length: %u, deviceid: %u, time: %#x, detail: %#x, "
	    "sourceid: %u, valuators_len: %u, flags: %#x\n", e->length,
	    e->deviceid, e->time, e->detail, e->sourceid,
	    e->valuators_len, e->flags);

	if (!button_has_binding(e->detail))
		goto done;

	/* Try to find binding with the current modifier state. */
	qpr = xcb_query_pointer_reply(conn,
	    xcb_query_pointer(conn, screens[0].root), NULL);
	if (qpr == NULL) {
		DNPRINTF(SWM_D_MISC, "failed to query pointer.\n");
		goto done;
	}

	bp = binding_lookup(CLEANMASK(qpr->mask), BTNBIND, e->detail);
	if (bp == NULL) {
		bp = binding_lookup(ANYMOD, BTNBIND, e->detail);
		if (bp == NULL)
			goto out;
	}
	DNPRINTF(SWM_D_MISC, "mask: %u, bound: %s\n", qpr->mask, YESNO(bp));

	if (!(bp->flags & BINDING_F_REPLAY)) {
		DNPRINTF(SWM_D_FOCUS, "skip; binding grabbed.\n");
		goto out;
	}

	/* Handle ungrabbed binding. */

	/* Click to focus. */
	s = find_screen(qpr->root);
	click_focus(s, qpr->child, qpr->root_x, qpr->root_y);

	if ((ap = &actions[bp->action])) {
		if (bp->action == FN_SPAWN_CUSTOM)
			spawn_custom(s, &ap->args, bp->spawn_name);
		else if (ap->func)
			ap->func(s, bp, &ap->args);
	}
	flush();
out:
	free(qpr);
done:
	DNPRINTF(SWM_D_FOCUS, "done\n");
}
#endif

static void
grabbuttons(void)
{
	struct ws_win	*w;
	int		num_screens, i;

	DNPRINTF(SWM_D_MOUSE, "begin\n");
	updatenumlockmask();

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		if (xinput2_raw)
			grab_buttons_win(screens[i].root);
		else
			TAILQ_FOREACH(w, &screens[i].managed, manage_entry)
				grab_buttons_win(w->id);
	}
	DNPRINTF(SWM_D_MOUSE, "done\n");
}

static struct wsi_flag {
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

static int
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
	while ((name = strsep(&cp, SWM_CONF_DELIMLIST)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, SWM_CONF_WHITESPACE);
		name += strspn(name, SWM_CONF_WHITESPACE);
		len = strcspn(name, SWM_CONF_WHITESPACE);

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

static int
setlayoutorder(const char *str, char **emsg)
{
	struct layout	*new_layout;
	int		i;
	char		*s, *cp, *name;

	if (str == NULL)
		return (1);

	if ((cp = s = strdup(str)) == NULL)
		err(1, "setlayoutorder: strdup");

	layout_order_count = 0;

	while ((name = strsep(&cp, SWM_CONF_DELIMLIST)) != NULL) {
		name = trimopt(name);
		if (*name == '\0')
			continue;

		new_layout = NULL;
		for (i = 0; i < LENGTH(layouts); i++)
			if (strcasecmp(name, layouts[i].name) == 0) {
				new_layout = &layouts[i];
				break;
			}

		if (new_layout == NULL) {
			ALLOCSTR(emsg, "invalid layout: %s", name);
			free(s);
			return (1);
		}

		for (i = 0; i < layout_order_count; i++)
			if (layout_order[i] == new_layout) {
				ALLOCSTR(emsg, "duplicate layout: %s", name);
				free(s);
				return (1);
			}

		layout_order[layout_order_count++] = new_layout;
	}
	free(s);

	if (layout_order_count == 0) {
		ALLOCSTR(emsg, "missing layout");
		return (1);
	}

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
	"MAXIMIZE",
	"BELOW",
	"ICONIFY",
};

/* SWM_Q_DELIM: retain '|' for back compat for now (2009-08-11) */
#define SWM_Q_DELIM		"\n|+ \t"
static int
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
			if (*ws < -1)
				*ws = -1;
			continue;
		}

		/* When workspace index is unspecified, reset to none. */
		if (strncmp(name, "WS", SWM_QUIRK_LEN) == 0 ||
		    strncmp(name, "WS[]", SWM_QUIRK_LEN) == 0) {
			DNPRINTF(SWM_D_QUIRK, "%s\n", name);
			*ws = -3;
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

static void
quirk_insert(const char *class, const char *instance, const char *name,
    uint32_t type, uint8_t mode, uint32_t quirk, int ws)
{
	struct quirk		*qp;
	int			retc, reti, retn;

	DNPRINTF(SWM_D_QUIRK, "class: %s, instance: %s, name: %s, type: %u, "
	    "mode:%u, value: %u, ws: %d\n", class, instance, name, type, mode,
	    quirk, ws);

	if ((qp = malloc(sizeof *qp)) == NULL)
		err(1, "quirk_insert: malloc");

	if ((qp->class = strdup(class)) == NULL)
		err(1, "quirk_insert: strdup");
	if ((qp->instance = strdup(instance)) == NULL)
		err(1, "quirk_insert: strdup");
	if ((qp->name = strdup(name)) == NULL)
		err(1, "quirk_insert: strdup");

	if ((retc = regcompopt(&qp->regex_class, class)))
		add_startup_exception("invalid regex for 'class' field: %s",
		    class);
	if ((reti = regcompopt(&qp->regex_instance, instance)))
		add_startup_exception("invalid regex for 'instance' field: %s",
		    instance);
	if ((retn = regcompopt(&qp->regex_name, name)))
		add_startup_exception("invalid regex for 'name' field: %s",
		    name);

	if (retc || reti || retn) {
		DNPRINTF(SWM_D_QUIRK, "regex error\n");
		if (retc == 0)
			regfree(&qp->regex_class);
		if (reti == 0)
			regfree(&qp->regex_instance);
		if (retn == 0)
			regfree(&qp->regex_name);
		free(qp->class);
		free(qp->instance);
		free(qp->name);
		free(qp);
	} else {
		qp->type = type;
		qp->quirk = quirk;
		qp->mode = mode;
		qp->ws = ws;
		TAILQ_INSERT_TAIL(&quirks, qp, entry);
	}
	DNPRINTF(SWM_D_QUIRK, "leave\n");
}

static void
quirk_remove(struct quirk *qp)
{
	DNPRINTF(SWM_D_QUIRK, "%s:%s [%u]\n", qp->class, qp->name, qp->quirk);

	TAILQ_REMOVE(&quirks, qp, entry);
	quirk_free(qp);

	DNPRINTF(SWM_D_QUIRK, "leave\n");
}

static void
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

static void
clear_quirks(void)
{
	struct quirk		*qp;
#ifndef __clang_analyzer__ /* Suppress false warnings. */
	while ((qp = TAILQ_FIRST(&quirks)) != NULL) {
		quirk_remove(qp);
	}
#endif
}

static void
quirk_replace(struct quirk *qp, const char *class, const char *instance,
    const char *name, uint32_t type, uint8_t mode, uint32_t quirk, int ws)
{
	DNPRINTF(SWM_D_QUIRK, "%s:%s:%s:%u %u [%u], ws: %d\n", qp->class,
	    qp->instance, qp->name, qp->type, qp->mode, qp->quirk, qp->ws);

	quirk_remove(qp);
	quirk_insert(class, instance, name, type, mode, quirk, ws);

	DNPRINTF(SWM_D_QUIRK, "leave\n");
}

static void
setquirk(const char *class, const char *instance, const char *name,
    uint32_t type, uint8_t mode, uint32_t quirk, int ws)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "%s:%s:%s:%u %u [%u] ws: %d\n", class, instance,
	    name, type, mode, quirk, ws);

#ifndef __clang_analyzer__ /* Suppress false warnings. */
	/* Remove/replace existing quirk. */
	TAILQ_FOREACH(qp, &quirks, entry) {
		if (strcmp(qp->class, class) == 0 &&
		    strcmp(qp->instance, instance) == 0 &&
		    strcmp(qp->name, name) == 0 &&
		    qp->type == type) {
			if (quirk == 0 && ws == -2)
				quirk_remove(qp);
			else
				quirk_replace(qp, class, instance, name, type,
				    mode, quirk, ws);
			goto out;
		}
	}
#endif

	/* Only insert if quirk is not NONE or forced ws is set. */
	if (quirk || ws != -2)
		quirk_insert(class, instance, name, type, mode, quirk, ws);
out:
	DNPRINTF(SWM_D_QUIRK, "leave\n");
}

/* Eat '\' in str used to escape square brackets and colon. */
static void
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

static int
parse_window_type(const char *str, uint32_t *flags, char **emsg)
{
	char			*tmp, *cp, *name;
	int			i;

	if (str == NULL || flags == NULL)
		return (1);

	if ((cp = tmp = strdup(str)) == NULL)
		err(1, "parse_window_type: strdup");

	*flags = 0;
	while ((name = strsep(&cp, SWM_CONF_DELIMLIST)) != NULL) {
		name = cleanopt(name);
		if (*name == '\0')
			continue;

		for (i = 0; i < LENGTH(ewmh_window_types); i++) {
			if (strcasecmp(name, ewmh_window_types[i].name) == 0) {
				DNPRINTF(SWM_D_CONF, "type: [%s]\n", name);
				*flags |= ewmh_window_types[i].flag;
				break;
			}
		}
		if (i >= LENGTH(ewmh_window_types)) {
			ALLOCSTR(emsg, "invalid window type: %s", name);
			DNPRINTF(SWM_D_CONF, "invalid window type: [%s]\n",
			    name);
			free(tmp);
			return (1);
		}
	}

	free(tmp);
	return (0);
}

static struct focus_type {
	char		*name;
	unsigned int	mask;
} focustypes[] = {
	{"startup",		SWM_FOCUS_TYPE_STARTUP},
	{"border",		SWM_FOCUS_TYPE_BORDER},
	{"layout",		SWM_FOCUS_TYPE_LAYOUT},
	{"map",			SWM_FOCUS_TYPE_MAP},
	{"unmap",		SWM_FOCUS_TYPE_UNMAP},
	{"iconify",		SWM_FOCUS_TYPE_ICONIFY},
	{"uniconify",		SWM_FOCUS_TYPE_UNICONIFY},
	{"configure",		SWM_FOCUS_TYPE_CONFIGURE},
	{"move",		SWM_FOCUS_TYPE_MOVE},
	{"workspace",		SWM_FOCUS_TYPE_WORKSPACE},
};

static int
parse_focus_types(const char *str, uint32_t *ftypes, char **emsg)
{
	char			*tmp, *cp, *name;
	int			i, count;

	if (str == NULL || ftypes == NULL)
		return (1);

	if ((cp = tmp = strdup(str)) == NULL)
		err(1, "parse_focus_types: strdup");

	*ftypes = 0;
	count = 0;
	while ((name = strsep(&cp, SWM_CONF_DELIMLIST)) != NULL) {
		name = cleanopt(name);
		if (*name == '\0')
			continue;

		for (i = 0; i < LENGTH(focustypes); i++) {
			if (strcmp(name, focustypes[i].name) == 0) {
				DNPRINTF(SWM_D_CONF, "flag: [%s]\n", name);
				*ftypes |= focustypes[i].mask;
				break;
			}
		}
		if (i >= LENGTH(focustypes)) {
			ALLOCSTR(emsg, "invalid focus type: %s", name);
			DNPRINTF(SWM_D_CONF, "invalid focus type: [%s]\n",
			    name);
			free(tmp);
			return (1);
		}
		count++;
	}

	if (count == 0) {
		ALLOCSTR(emsg, "missing focus type");
		free(tmp);
		return(1);
	}

	free(tmp);
	return (0);
}

static int
setconffocusmode(uint8_t asop, const char *selector, const char *value,
    int flags, char **emsg)
{
	unsigned int		ftypes, fmode;

	/* Suppress warning. */
	(void)flags;

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	if (selector && strlen(selector)) {
		ftypes = 0;
		if (parse_focus_types(selector, &ftypes, emsg))
			return (1);
	} else
		ftypes = SWM_FOCUS_TYPE_ALL;

	if (strcmp(value, "default") == 0)
		fmode = SWM_FOCUS_MODE_DEFAULT;
	else if (strcmp(value, "follow") == 0 ||
			strcmp(value, "follow_cursor") == 0)
		fmode = SWM_FOCUS_MODE_FOLLOW;
	else if (strcmp(value, "manual") == 0)
		fmode = SWM_FOCUS_MODE_MANUAL;
	else {
		ALLOCSTR(emsg, "invalid focus mode value: %s", value);
		return (1);
	}

	focus_mode = (focus_mode & ~ftypes) | (fmode & ftypes);
	DNPRINTF(SWM_D_MISC, "focus_mode = %d\n", focus_mode);

	return (0);
}

static int
setconfquirk(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	char			*str, *cp, *class;
	char			*instance = NULL, *name = NULL, *type = NULL;
	int			retval, count = 0, ws = -2;
	uint32_t		qrks, wintype = 0;

	/* Suppress warning. */
	(void)flags;

	if (asopcheck(asop, SWM_ASOP_BASIC | SWM_ASOP_ADD | SWM_ASOP_SUBTRACT,
	    emsg))
		return (1);

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

	/* Class */
	unescape_selector(class);

	/* Instance */
	if (count) {
		instance = class + strlen(class) + 1;
		unescape_selector(instance);
	} else {
		instance = ".*";
	}

	/* Name */
	if (count > 1) {
		name = instance + strlen(instance) + 1;
		unescape_selector(name);
	} else {
		name = ".*";
	}

	/* Type */
	if (count > 2) {
		type = name + strlen(name) + 1;
		unescape_selector(type);
		if (parse_window_type(type, &wintype, emsg))
			return (1);
	}

	DNPRINTF(SWM_D_CONF, "class: %s, instance: %s, name: %s, type: %u\n",
	    class, instance, name, wintype);

	if ((retval = parsequirks(value, &qrks, &ws, emsg)) == 0)
		setquirk(class, instance, name, wintype, asop, qrks, ws);

	free(str);
	return (retval);
}

static void
setup_quirks(void)
{
	setquirk(".*", ".*", ".*", EWMH_WINDOW_TYPE_SPLASH |
	    EWMH_WINDOW_TYPE_DIALOG, SWM_ASOP_BASIC, SWM_Q_FLOAT, -2);
	setquirk(".*", ".*", ".*", EWMH_WINDOW_TYPE_TOOLBAR |
	    EWMH_WINDOW_TYPE_UTILITY, SWM_ASOP_BASIC, SWM_Q_FLOAT |
	    SWM_Q_ANYWHERE, -2);
	setquirk(".*", ".*", ".*", EWMH_WINDOW_TYPE_NOTIFICATION,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_ANYWHERE | SWM_Q_MINIMALBORDER |
	    SWM_Q_NOFOCUSONMAP, -2);
	setquirk("MPlayer", "xv", ".*", 0, SWM_ASOP_BASIC,
	    SWM_Q_FLOAT | SWM_Q_FULLSCREEN | SWM_Q_FOCUSPREV, -2);
	setquirk("OpenOffice.org 3.2", "VCLSalFrame", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT, -2);
	setquirk("Firefox-bin", "firefox-bin", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_TRANSSZ, -2);
	setquirk("Firefox", "Dialog", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT, -2);
	setquirk("Gimp", "gimp", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_ANYWHERE, -2);
	setquirk("XTerm", "xterm", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_XTERM_FONTADJ, -2);
	setquirk("xine", "Xine Window", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_ANYWHERE, -2);
	setquirk("Xitk", "Xitk Combo", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_ANYWHERE, -2);
	setquirk("xine", "xine Panel", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_ANYWHERE, -2);
	setquirk("Xitk", "Xine Window", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_ANYWHERE, -2);
	setquirk("xine", "xine Video Fullscreen Window", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FULLSCREEN | SWM_Q_FLOAT, -2);
	setquirk("pcb", "pcb", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT, -2);
	setquirk("SDL_App", "SDL_App", ".*", 0,
	    SWM_ASOP_BASIC, SWM_Q_FLOAT | SWM_Q_FULLSCREEN, -2);
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
	SWM_S_BAR_WORKSPACE_LIMIT,
	SWM_S_BORDER_WIDTH,
	SWM_S_BOUNDARY_WIDTH,
	SWM_S_CLICK_TO_RAISE,
	SWM_S_CLOCK_ENABLED,
	SWM_S_CLOCK_FORMAT,
	SWM_S_CYCLE_EMPTY,
	SWM_S_CYCLE_VISIBLE,
	SWM_S_DIALOG_RATIO,
	SWM_S_DISABLE_BORDER,
	SWM_S_FOCUS_CLOSE,
	SWM_S_FOCUS_CLOSE_WRAP,
	SWM_S_FOCUS_DEFAULT,
	SWM_S_FULLSCREEN_HIDE_OTHER,
	SWM_S_FULLSCREEN_UNFOCUS,
	SWM_S_ICONIC_ENABLED,
	SWM_S_LAYOUT_ORDER,
	SWM_S_MAX_LAYOUT_MAXIMIZE,
	SWM_S_MAXIMIZE_HIDE_BAR,
	SWM_S_MAXIMIZE_HIDE_OTHER,
	SWM_S_MAXIMIZED_UNFOCUS,
	SWM_S_REGION_PADDING,
	SWM_S_SNAP_RANGE,
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
	SWM_S_WORKSPACE_AUTOROTATE,
	SWM_S_WORKSPACE_CLAMP,
	SWM_S_WORKSPACE_LIMIT,
	SWM_S_WORKSPACE_INDICATOR,
	SWM_S_WORKSPACE_NAME,
	SWM_S_FOCUS_MARK_NONE,
	SWM_S_FOCUS_MARK_NORMAL,
	SWM_S_FOCUS_MARK_FLOATING,
	SWM_S_FOCUS_MARK_FREE,
	SWM_S_FOCUS_MARK_MAXIMIZED,
	SWM_S_WORKSPACE_MARK_CURRENT,
	SWM_S_WORKSPACE_MARK_CURRENT_SUFFIX,
	SWM_S_WORKSPACE_MARK_URGENT,
	SWM_S_WORKSPACE_MARK_URGENT_SUFFIX,
	SWM_S_WORKSPACE_MARK_ACTIVE,
	SWM_S_WORKSPACE_MARK_ACTIVE_SUFFIX,
	SWM_S_WORKSPACE_MARK_EMPTY,
	SWM_S_WORKSPACE_MARK_EMPTY_SUFFIX,
	SWM_S_STACK_MARK_FLOATING,
	SWM_S_STACK_MARK_MAX,
	SWM_S_STACK_MARK_VERTICAL,
	SWM_S_STACK_MARK_VERTICAL_FLIP,
	SWM_S_STACK_MARK_HORIZONTAL,
	SWM_S_STACK_MARK_HORIZONTAL_FLIP,
};

static int
setconfvalue(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	struct swm_region	*r;
	struct workspace	*ws;
	int			i, ws_id, num_screens, n;

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	switch (flags) {
	case SWM_S_BAR_ACTION:
		free(bar_argv[0]);
		bar_argv[0] = expand_tilde(value);
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
			if ((ws = get_workspace(&screens[i], ws_id)))
				ws->bar_enabled = (atoi(value) != 0);
		}
		break;
	case SWM_S_BAR_FONT:
		free(bar_fonts);
		if ((bar_fonts = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_font strdup");
		break;
	case SWM_S_BAR_FONT_PUA:
		free(bar_fontname_pua);
		if ((bar_fontname_pua = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_font_pua strdup");
		break;
	case SWM_S_BAR_FORMAT:
		free(bar_format);
		if ((bar_format = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_format strdup");
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
	case SWM_S_BAR_WORKSPACE_LIMIT:
		bar_workspace_limit = atoi(value);
		if (bar_workspace_limit > SWM_WS_MAX)
			bar_workspace_limit = SWM_WS_MAX;
		else if (bar_workspace_limit < 0)
			bar_workspace_limit = 0;
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
	case SWM_S_CLICK_TO_RAISE:
		click_to_raise = (atoi(value) != 0);
		break;
	case SWM_S_CLOCK_ENABLED:
		clock_enabled = (atoi(value) != 0);
		break;
	case SWM_S_CLOCK_FORMAT:
#ifndef SWM_DENY_CLOCK_FORMAT
		free(clock_format);
		if ((clock_format = strdup(value)) == NULL)
			err(1, "setconfvalue: clock_format strdup");
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
		else if (!strcmp(value, "prior"))
			focus_close = SWM_STACK_PRIOR;
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
	case SWM_S_FULLSCREEN_HIDE_OTHER:
		fullscreen_hide_other = atoi(value);
		break;
	case SWM_S_FULLSCREEN_UNFOCUS:
		if (strcmp(value, "none") == 0 || strcmp(value, "default") == 0)
			fullscreen_unfocus = SWM_UNFOCUS_NONE;
		else if (strcmp(value, "restore") == 0)
			fullscreen_unfocus = SWM_UNFOCUS_RESTORE;
		else if (strcmp(value, "iconify") == 0)
			fullscreen_unfocus = SWM_UNFOCUS_ICONIFY;
		else if (strcmp(value, "float") == 0)
			fullscreen_unfocus = SWM_UNFOCUS_FLOAT;
		else if (strcmp(value, "below") == 0)
			fullscreen_unfocus = SWM_UNFOCUS_BELOW;
		else if (strcmp(value, "quick_below") == 0)
			fullscreen_unfocus = SWM_UNFOCUS_QUICK_BELOW;
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}
		break;
	case SWM_S_ICONIC_ENABLED:
		iconic_enabled = (atoi(value) != 0);
		break;
	case SWM_S_LAYOUT_ORDER:
		if (setlayoutorder(value, emsg)) {
			layout_order_reset();
			return (1);
		}
		break;
	case SWM_S_MAX_LAYOUT_MAXIMIZE:
		max_layout_maximize = atoi(value);
		break;
	case SWM_S_MAXIMIZE_HIDE_BAR:
		maximize_hide_bar = atoi(value);
		break;
	case SWM_S_MAXIMIZE_HIDE_OTHER:
		maximize_hide_other = atoi(value);
		break;
	case SWM_S_MAXIMIZED_UNFOCUS:
		if (strcmp(value, "none") == 0)
			maximized_unfocus = SWM_UNFOCUS_NONE;
		else if (strcmp(value, "restore") == 0 ||
		    strcmp(value, "default") == 0)
			maximized_unfocus = SWM_UNFOCUS_RESTORE;
		else if (strcmp(value, "iconify") == 0)
			maximized_unfocus = SWM_UNFOCUS_ICONIFY;
		else if (strcmp(value, "float") == 0)
			maximized_unfocus = SWM_UNFOCUS_FLOAT;
		else if (strcmp(value, "below") == 0)
			maximized_unfocus = SWM_UNFOCUS_BELOW;
		else if (strcmp(value, "quick_below") == 0)
			maximized_unfocus = SWM_UNFOCUS_QUICK_BELOW;
		else {
			ALLOCSTR(emsg, "invalid value: %s", value);
			return (1);
		}
		break;
	case SWM_S_REGION_PADDING:
		region_padding = atoi(value);
		if (region_padding < 0)
			region_padding = 0;
		break;
	case SWM_S_SNAP_RANGE:
		snap_range = atoi(value);
		if (snap_range < 0)
			snap_range = 0;
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
		setconfspawn(asop, "term", value, 0, emsg);
		setconfspawn(asop, "spawn_term", value, 0, emsg);
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
		for (i = 0; i < LENGTH(layouts); i++) {
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
	case SWM_S_WORKSPACE_AUTOROTATE:
		workspace_autorotate = (atoi(value) != 0);
		/* Update existing region(s). */
		num_screens = get_screen_count();
		for (i = 0; i < num_screens; i++)
			TAILQ_FOREACH(r, &screens[i].rl, entry)
				rotatews(r->ws, (workspace_autorotate ?
				    ROTATION(r) : ROTATION_DEFAULT));
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

		num_screens = get_screen_count();
		for (i = 0; i < num_screens; ++i)
			ewmh_update_number_of_desktops(&screens[i]);
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
			ws = get_workspace(&screens[i], ws_id);
			if (ws) {
				free(ws->name);
				if ((ws->name = strdup(value)) == NULL)
					err(1, "setconfvalue: name strdup");

				ewmh_update_desktop_names(&screens[i]);
				ewmh_get_desktop_names(&screens[i]);
			}
		}
		break;
	case SWM_S_FOCUS_MARK_NONE:
		free(focus_mark_none);
		focus_mark_none = unescape_value(value);
		break;
	case SWM_S_FOCUS_MARK_NORMAL:
		free(focus_mark_normal);
		focus_mark_normal = unescape_value(value);
		break;
	case SWM_S_FOCUS_MARK_FLOATING:
		free(focus_mark_floating);
		focus_mark_floating = unescape_value(value);
		break;
	case SWM_S_FOCUS_MARK_FREE:
		free(focus_mark_free);
		focus_mark_free = unescape_value(value);
		break;
	case SWM_S_FOCUS_MARK_MAXIMIZED:
		free(focus_mark_maximized);
		focus_mark_maximized = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_CURRENT:
		free(workspace_mark_current);
		workspace_mark_current = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_CURRENT_SUFFIX:
		free(workspace_mark_current_suffix);
		workspace_mark_current_suffix = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_URGENT:
		free(workspace_mark_urgent);
		workspace_mark_urgent = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_URGENT_SUFFIX:
		free(workspace_mark_urgent_suffix);
		workspace_mark_urgent_suffix = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_ACTIVE:
		free(workspace_mark_active);
		workspace_mark_active = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_ACTIVE_SUFFIX:
		free(workspace_mark_active_suffix);
		workspace_mark_active_suffix = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_EMPTY:
		free(workspace_mark_empty);
		workspace_mark_empty = unescape_value(value);
		break;
	case SWM_S_WORKSPACE_MARK_EMPTY_SUFFIX:
		free(workspace_mark_empty_suffix);
		workspace_mark_empty_suffix = unescape_value(value);
		break;
	case SWM_S_STACK_MARK_FLOATING:
		free(stack_mark_floating);
		stack_mark_floating = unescape_value(value);
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

static int
setconfmodkey(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	/* Suppress warning. */
	(void)selector;
	(void)flags;

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

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

static int
setconfcancelkey(uint8_t asop, const char *selector, const char *value,
    int flags, char **emsg)
{
	xcb_keysym_t	ks;
	char		*name, *cp;

	/* Suppress warning. */
	(void)selector;
	(void)flags;

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	if ((cp = name = strdup(value)) == NULL)
		err(1, "setconfcancelkey: strdup");

	cp += strcspn(cp, SWM_CONF_WHITESPACE) + 1;
	*cp = '\0';

	if ((ks = get_string_keysym(name)) == XCB_NO_SYMBOL) {
		ALLOCSTR(emsg, "invalid key");
		free(name);
		return (1);
	}
	free(name);

	cancel_key = ks;
	DNPRINTF(SWM_D_KEY, "cancel_key = %u\n", cancel_key);

	return (0);
}

static int
parseconfcolor(uint8_t asop, const char *selector, const char *value, int flags,
    bool multi, char **emsg)
{
	struct swm_screen	*s;
	int			first, last, i, n, num_screens;
	char			*b, *str, *sp;

	num_screens = get_screen_count();

	first = 0;
	last = num_screens - 1;

	/* Screen indices begin at 1; handle values <= 0 as 'all screens.' */
	if (selector && strlen(selector)) {
		i = atoi(selector);
		if (i > num_screens) {
			ALLOCSTR(emsg, "invalid screen index: %d", i);
			return (1);
		}
		if (i > 0)
			first = last = i - 1;
	}

	if ((sp = str = strdup(value)) == NULL)
		err(1, "parseconfcolor: strdup");

	if (asop == SWM_ASOP_BASIC)
		for (i = first; i <= last; ++i)
			freecolortype(&screens[i], flags);

	n = 0;
	while ((b = strsep(&sp, SWM_CONF_DELIMLIST)) != NULL) {
		while (isblank((unsigned char)*b)) b++;
		if (*b == '\0')
			continue;

		if (n > 0 && !multi) {
			ALLOCSTR(emsg, "option value must be a single color");
			free(str);
			return (1);
		}
		/* Append color. */
		for (i = first; i <= last; ++i) {
			s = &screens[i];
			setscreencolor(s, b, flags, s->c[flags].count);
		}
		n++;
	}
	free(str);

	return (0);
}

static int
setconfcolor(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	return (parseconfcolor(asop, selector, value, flags, false, emsg));
}

static int
setconfcolorlist(uint8_t asop, const char *selector, const char *value,
    int flags, char **emsg)
{
	if (asopcheck(asop, SWM_ASOP_BASIC | SWM_ASOP_ADD, emsg))
		return (1);

	return (parseconfcolor(asop, selector, value, flags, true, emsg));
}

static int
setconfregion(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	unsigned int			x, y, w, h;
	int				sidx, num_screens, rot;
	char				r[9];
	xcb_screen_t			*screen;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	DNPRINTF(SWM_D_CONF, "%s\n", value);

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	num_screens = get_screen_count();
	if (sscanf(value, "screen[%d]:%ux%u+%u+%u,%8s",
	    &sidx, &w, &h, &x, &y, r) == 6) {
		if (strcasecmp(r, "normal") == 0)
			rot = XCB_RANDR_ROTATION_ROTATE_0;
		else if (strcasecmp(r, "left") == 0)
			rot = XCB_RANDR_ROTATION_ROTATE_90;
		else if (strcasecmp(r, "inverted") == 0)
			rot = XCB_RANDR_ROTATION_ROTATE_180;
		else if (strcasecmp(r, "right") == 0)
			rot = XCB_RANDR_ROTATION_ROTATE_270;
		else {
			ALLOCSTR(emsg, "invalid rotation: %s", value);
			return (1);
		}
	} else if (sscanf(value, "screen[%d]:%ux%u+%u+%u",
	    &sidx, &w, &h, &x, &y) == 5) {
		rot = ROTATION_DEFAULT;
	} else {
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

	new_region(&screens[sidx], x, y, w, h, rot);

	return (0);
}

static int
setautorun(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	union arg		a;
	struct pid_e		*p;
	int			ws_id, argc = 0, n;
	unsigned int		sf;
	pid_t			pid;
	char			*ap, *sp, *str;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (getenv("SWM_STARTED"))
		return (0);

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);

	n = 0;
	if (sscanf(value, "ws[%d]:%n", &ws_id, &n) != 1 || n == 0 ||
	    value[n] == '\0') {
		ALLOCSTR(emsg, "invalid syntax: %s", value);
		return (1);
	}
	value += n;
	if (ws_id > 0)
		ws_id--;
	if (ws_id < -1 || ws_id >= workspace_limit) {
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

	sf = spawn_flags | SWM_SPAWN_CLOSE_FD;
	if (!(sf & SWM_SPAWN_NOSPAWNWS))
		sf |= SWM_SPAWN_WS | SWM_SPAWN_PID;

	if ((pid = fork()) == 0) {
		spawn(ws_id, &a, sf);
		/* NOTREACHED */
		_exit(1);
	}
	free(a.argv);
	free(str);

	if (sf & SWM_SPAWN_PID) {
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
	}

	return (0);
}

static int
setlayout(uint8_t asop, const char *selector, const char *value, int flags,
    char **emsg)
{
	struct workspace	*ws;
	int			ws_id, i, x, mg, ma, si, ar, n;
	int			st = SWM_V_STACK, num_screens;
	uint16_t		rot;
	bool			f = false;

	/* suppress unused warnings since vars are needed */
	(void)selector;
	(void)flags;

	if (getenv("SWM_STARTED"))
		return (0);

	if (asopcheck(asop, SWM_ASOP_BASIC, emsg))
		return (1);
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
	else if (strcasecmp(value, "floating") == 0)
		st = SWM_FLOATING_STACK;
	else {
		ALLOCSTR(emsg, "invalid layout: %s", value);
		return (1);
	}

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		ws = get_workspace(&screens[i], ws_id);
		if (ws == NULL)
			continue;

		/* Set layout relative to default rotation. */
		rot = ws->rotation;
		rotatews(ws, ROTATION_DEFAULT);
		ws->cur_layout = &layouts[st];
		ws->always_raise = (ar != 0);

		if (ws->cur_layout->l_config == NULL || st == SWM_MAX_STACK)
			continue;

		ws->cur_layout->l_config(ws, SWM_ARG_ID_STACKINIT);

		/* master grow */
		for (x = 0; x < abs(mg); x++) {
			ws->cur_layout->l_config(ws, mg >= 0 ?
			    SWM_ARG_ID_MASTERGROW : SWM_ARG_ID_MASTERSHRINK);
		}
		/* master add */
		for (x = 0; x < abs(ma); x++) {
			ws->cur_layout->l_config(ws, ma >= 0 ?
			    SWM_ARG_ID_MASTERADD : SWM_ARG_ID_MASTERDEL);
		}
		/* stack inc */
		for (x = 0; x < abs(si); x++) {
			ws->cur_layout->l_config(ws, si >= 0 ?
			    SWM_ARG_ID_STACKINC : SWM_ARG_ID_STACKDEC);
		}
		/* Apply flip */
		if (f)
			ws->cur_layout->l_config(ws, SWM_ARG_ID_FLIPLAYOUT);

		/* Reapply rotation. */
		rotatews(ws, rot);
	}

	return (0);
}

/* config options */
struct config_option {
	char	*name;
	int	(*func)(uint8_t, const char *, const char *, int, char **);
	int	flags;
};
struct config_option configopt[] = {
	{ "autorun",			setautorun,	0 },
	{ "bar_action",			setconfvalue,	SWM_S_BAR_ACTION },
	{ "bar_action_expand",		setconfvalue,	SWM_S_BAR_ACTION_EXPAND },
	{ "bar_at_bottom",		setconfvalue,	SWM_S_BAR_AT_BOTTOM },
	{ "bar_border",			setconfcolor,	SWM_S_COLOR_BAR_BORDER },
	{ "bar_border_unfocus",		setconfcolor,	SWM_S_COLOR_BAR_BORDER_UNFOCUS },
	{ "bar_border_free",		setconfcolor,	SWM_S_COLOR_BAR_BORDER_FREE },
	{ "bar_border_width",		setconfvalue,	SWM_S_BAR_BORDER_WIDTH },
	{ "bar_color",			setconfcolorlist,SWM_S_COLOR_BAR },
	{ "bar_color_unfocus",		setconfcolorlist,SWM_S_COLOR_BAR_UNFOCUS },
	{ "bar_color_free",		setconfcolorlist,SWM_S_COLOR_BAR_FREE },
	{ "bar_color_selected",		setconfcolor,	SWM_S_COLOR_BAR_SELECTED },
	{ "bar_delay",			NULL,		0 },	/* dummy */
	{ "bar_enabled",		setconfvalue,	SWM_S_BAR_ENABLED },
	{ "bar_enabled_ws",		setconfvalue,	SWM_S_BAR_ENABLED_WS },
	{ "bar_font",			setconfvalue,	SWM_S_BAR_FONT },
	{ "bar_font_color",		setconfcolorlist,SWM_S_COLOR_BAR_FONT },
	{ "bar_font_color_unfocus",	setconfcolorlist,SWM_S_COLOR_BAR_FONT_UNFOCUS },
	{ "bar_font_color_free",	setconfcolorlist,SWM_S_COLOR_BAR_FONT_FREE },
	{ "bar_font_color_selected",	setconfcolor,	SWM_S_COLOR_BAR_FONT_SELECTED },
	{ "bar_font_pua",		setconfvalue,	SWM_S_BAR_FONT_PUA },
	{ "bar_format",			setconfvalue,	SWM_S_BAR_FORMAT },
	{ "bar_justify",		setconfvalue,	SWM_S_BAR_JUSTIFY },
	{ "bar_workspace_limit",	setconfvalue,	SWM_S_BAR_WORKSPACE_LIMIT },
	{ "bind",			setconfbinding,	0 },
	{ "border_width",		setconfvalue,	SWM_S_BORDER_WIDTH },
	{ "boundary_width",		setconfvalue,	SWM_S_BOUNDARY_WIDTH },
	{ "cancelkey",			setconfcancelkey,0 },
	{ "click_to_raise",		setconfvalue,	SWM_S_CLICK_TO_RAISE },
	{ "clock_enabled",		setconfvalue,	SWM_S_CLOCK_ENABLED },
	{ "clock_format",		setconfvalue,	SWM_S_CLOCK_FORMAT },
	{ "color_focus",		setconfcolor,	SWM_S_COLOR_FOCUS },
	{ "color_focus_free",		setconfcolor,	SWM_S_COLOR_FOCUS_FREE },
	{ "color_focus_maximized",	setconfcolor,	SWM_S_COLOR_FOCUS_MAXIMIZED },
	{ "color_focus_maximized_free",	setconfcolor,	SWM_S_COLOR_FOCUS_MAXIMIZED_FREE },
	{ "color_unfocus",		setconfcolor,	SWM_S_COLOR_UNFOCUS },
	{ "color_unfocus_free",		setconfcolor,	SWM_S_COLOR_UNFOCUS_FREE },
	{ "color_unfocus_maximized",	setconfcolor,	SWM_S_COLOR_UNFOCUS_MAXIMIZED },
	{ "color_unfocus_maximized_free",setconfcolor,	SWM_S_COLOR_UNFOCUS_MAXIMIZED_FREE },
	{ "color_urgent",		setconfcolor,	SWM_S_COLOR_URGENT },
	{ "color_urgent_free",		setconfcolor,	SWM_S_COLOR_URGENT_FREE },
	{ "color_urgent_maximized",	setconfcolor,	SWM_S_COLOR_URGENT_MAXIMIZED },
	{ "color_urgent_maximized_free",setconfcolor,	SWM_S_COLOR_URGENT_MAXIMIZED_FREE },
	{ "cycle_empty",		setconfvalue,	SWM_S_CYCLE_EMPTY },
	{ "cycle_visible",		setconfvalue,	SWM_S_CYCLE_VISIBLE },
	{ "dialog_ratio",		setconfvalue,	SWM_S_DIALOG_RATIO },
	{ "disable_border",		setconfvalue,	SWM_S_DISABLE_BORDER },
	{ "focus_close",		setconfvalue,	SWM_S_FOCUS_CLOSE },
	{ "focus_close_wrap",		setconfvalue,	SWM_S_FOCUS_CLOSE_WRAP },
	{ "focus_default",		setconfvalue,	SWM_S_FOCUS_DEFAULT },
	{ "focus_mode",			setconffocusmode,0 },
	{ "fullscreen_hide_other",	setconfvalue,	SWM_S_FULLSCREEN_HIDE_OTHER },
	{ "fullscreen_unfocus",		setconfvalue,	SWM_S_FULLSCREEN_UNFOCUS },
	{ "iconic_enabled",		setconfvalue,	SWM_S_ICONIC_ENABLED },
	{ "java_workaround",		NULL,		0 },	/* dummy */
	{ "keyboard_mapping",		setkeymapping,	0 },
	{ "layout",			setlayout,	0 },
	{ "layout_order",		setconfvalue,	SWM_S_LAYOUT_ORDER },
	{ "max_layout_maximize",	setconfvalue,	SWM_S_MAX_LAYOUT_MAXIMIZE },
	{ "maximize_hide_bar",		setconfvalue,	SWM_S_MAXIMIZE_HIDE_BAR },
	{ "maximize_hide_other",	setconfvalue,	SWM_S_MAXIMIZE_HIDE_OTHER },
	{ "maximized_unfocus",		setconfvalue,	SWM_S_MAXIMIZED_UNFOCUS },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "region_padding",		setconfvalue,	SWM_S_REGION_PADDING },
	{ "screenshot_app",		NULL,		0 },	/* dummy */
	{ "screenshot_enabled",		NULL,		0 },	/* dummy */
	{ "snap_range",			setconfvalue,	SWM_S_SNAP_RANGE },
	{ "spawn_flags",		setconfspawnflags,0 },
	{ "spawn_position",		setconfvalue,	SWM_S_SPAWN_ORDER },
	{ "spawn_term",			setconfvalue,	SWM_S_SPAWN_TERM },
	{ "stack_enabled",		setconfvalue,	SWM_S_STACK_ENABLED },
	{ "term_width",			setconfvalue,	SWM_S_TERM_WIDTH },
	{ "tile_gap",			setconfvalue,	SWM_S_TILE_GAP },
	{ "title_class_enabled",	setconfvalue,	SWM_S_WINDOW_CLASS_ENABLED },	/* For backwards compat. */
	{ "title_name_enabled",		setconfvalue,	SWM_S_WINDOW_INSTANCE_ENABLED },/* For backwards compat. */
	{ "urgent_collapse",		setconfvalue,	SWM_S_URGENT_COLLAPSE },
	{ "urgent_enabled",		setconfvalue,	SWM_S_URGENT_ENABLED },
	{ "verbose_layout",		setconfvalue,	SWM_S_VERBOSE_LAYOUT },
	{ "warp_focus",			setconfvalue,	SWM_S_WARP_FOCUS },
	{ "warp_pointer",		setconfvalue,	SWM_S_WARP_POINTER },
	{ "window_class_enabled",	setconfvalue,	SWM_S_WINDOW_CLASS_ENABLED },
	{ "window_instance_enabled",	setconfvalue,	SWM_S_WINDOW_INSTANCE_ENABLED },
	{ "window_name_enabled",	setconfvalue,	SWM_S_WINDOW_NAME_ENABLED },
	{ "workspace_autorotate",	setconfvalue,	SWM_S_WORKSPACE_AUTOROTATE },
	{ "workspace_clamp",		setconfvalue,	SWM_S_WORKSPACE_CLAMP },
	{ "workspace_limit",		setconfvalue,	SWM_S_WORKSPACE_LIMIT },
	{ "workspace_indicator",	setconfvalue,	SWM_S_WORKSPACE_INDICATOR },
	{ "name",			setconfvalue,	SWM_S_WORKSPACE_NAME },
	{ "focus_mark_none",		setconfvalue,	SWM_S_FOCUS_MARK_NONE },
	{ "focus_mark_normal",		setconfvalue,	SWM_S_FOCUS_MARK_NORMAL },
	{ "focus_mark_floating",	setconfvalue,	SWM_S_FOCUS_MARK_FLOATING },
	{ "focus_mark_free",		setconfvalue,	SWM_S_FOCUS_MARK_FREE },
	{ "focus_mark_maximized",	setconfvalue,	SWM_S_FOCUS_MARK_MAXIMIZED },
	{ "workspace_mark_current",	setconfvalue,	SWM_S_WORKSPACE_MARK_CURRENT },
	{ "workspace_mark_current_suffix",setconfvalue,	SWM_S_WORKSPACE_MARK_CURRENT_SUFFIX },
	{ "workspace_mark_urgent",	setconfvalue,	SWM_S_WORKSPACE_MARK_URGENT },
	{ "workspace_mark_urgent_suffix",setconfvalue,	SWM_S_WORKSPACE_MARK_URGENT_SUFFIX },
	{ "workspace_mark_active",	setconfvalue,	SWM_S_WORKSPACE_MARK_ACTIVE },
	{ "workspace_mark_active_suffix",setconfvalue,	SWM_S_WORKSPACE_MARK_ACTIVE_SUFFIX },
	{ "workspace_mark_empty",	setconfvalue,	SWM_S_WORKSPACE_MARK_EMPTY },
	{ "workspace_mark_empty_suffix",setconfvalue,	SWM_S_WORKSPACE_MARK_EMPTY_SUFFIX },
	{ "stack_mark_floating",	setconfvalue,	SWM_S_STACK_MARK_FLOATING },
	{ "stack_mark_max",		setconfvalue,	SWM_S_STACK_MARK_MAX },
	{ "stack_mark_vertical",	setconfvalue,	SWM_S_STACK_MARK_VERTICAL },
	{ "stack_mark_vertical_flip",	setconfvalue,	SWM_S_STACK_MARK_VERTICAL_FLIP },
	{ "stack_mark_horizontal",	setconfvalue,	SWM_S_STACK_MARK_HORIZONTAL },
	{ "stack_mark_horizontal_flip",	setconfvalue,	SWM_S_STACK_MARK_HORIZONTAL_FLIP },
};

static void
_add_startup_exception(const char *fmt, va_list ap)
{
	if (vasprintf(&startup_exception, fmt, ap) == -1)
		warn("%s: asprintf", __func__);
}

static void
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

static int
conf_load(const char *filename, int keymapping)
{
	struct config_option	*opt = NULL;
	FILE			*config;
	char			*line = NULL, *cp, *ce, *optsub, *optval = NULL;
	char			*emsg = NULL;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optidx, count;
	uint8_t			asop = 0;

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
		wordlen = strcspn(cp, "=+-[ \t\n");
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
		cp += strspn(cp, " \t\n"); /* eat whitespace */

		/* Get assignment operator. */
		wordlen = strspn(cp, "=+-");
		if (wordlen >= 1 && strncmp(cp, "=", wordlen) == 0)
			asop = SWM_ASOP_BASIC;
		else if (wordlen >= 2 && strncmp(cp, "+=", wordlen) == 0)
			asop = SWM_ASOP_ADD;
		else if (wordlen >= 2 && strncmp(cp, "-=", wordlen) == 0)
			asop = SWM_ASOP_SUBTRACT;
		else {
			add_startup_exception("%s: line %zd: syntax error: "
			    "invalid/missing assignment operator", filename,
			    lineno);
			continue;
		}
		cp += wordlen;
		cp += strspn(cp, " \t\n"); /* eat whitespace */

		/* get RHS value */
		optval = cp;
		if (strlen(optval) == 0) {
			add_startup_exception("%s: line %zd: must supply value "
			    "to %s", filename, lineno, opt->name);
			continue;
		}
		/* trim trailing spaces */
		ce = optval + strlen(optval) - 1;
		while (ce > optval && isspace((unsigned char)*ce))
			--ce;
		*(ce + 1) = '\0';
		/* call function to deal with it all */
		if (opt->func &&
		    opt->func(asop, optsub, optval, opt->flags, &emsg)) {
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

static int32_t
strtoint32(const char *str, int32_t min, int32_t max, int *fail)
{
	int32_t		ret;
#if defined(__NetBSD__)
	int		e;

	ret = strtoi(str, NULL, 10, min, max, &e);
	*fail = (e != 0);
#else
	const char	*errstr;

	ret = strtonum(str, min, max, &errstr);
	*fail = (errstr != NULL);
#endif
	return (ret);
}

static pid_t
window_get_pid(xcb_window_t win)
{
	pid_t				ret = 0;
	int				fail;
	xcb_get_property_reply_t	*pr;

	pr = xcb_get_property_reply(conn, xcb_get_property(conn, 0, win,
	    a_net_wm_pid, XCB_ATOM_CARDINAL, 0, 1), NULL);
	if (pr && pr->type == XCB_ATOM_CARDINAL && pr->format == 32)
		ret = *((pid_t *)xcb_get_property_value(pr));
	else { /* tryharder */
		free(pr);
		pr = xcb_get_property_reply(conn, xcb_get_property(conn, 0, win,
		    a_swm_pid, XCB_ATOM_STRING, 0, SWM_PROPLEN), NULL);
		if (pr && pr->type == XCB_ATOM_STRING && pr->format == 8) {
			ret = (pid_t)strtoint32(xcb_get_property_value(pr), 0,
			    INT32_MAX, &fail);
			if (fail)
				ret = 0;
		}
	}
	free(pr);

	DNPRINTF(SWM_D_PROP, "pid: %d\n", ret);
	return (ret);
}

static int
get_swm_ws(xcb_window_t id)
{
	int				ws_idx = -2, fail;
	xcb_get_property_reply_t	*gpr;

	gpr = xcb_get_property_reply(conn, xcb_get_property(conn, 0, id,
	    a_swm_ws, XCB_ATOM_STRING, 0, SWM_PROPLEN), NULL);
	if (gpr && gpr->type == XCB_ATOM_STRING && gpr->format == 8) {
		ws_idx = strtoint32(xcb_get_property_value(gpr), -1,
		    workspace_limit - 1, &fail);
		if (fail)
			ws_idx = -2;
	}
	free(gpr);

	DNPRINTF(SWM_D_PROP, "_SWM_WS: %d\n", ws_idx);
	return (ws_idx);
}

static int
get_ws_id(struct ws_win *win)
{
	xcb_get_property_reply_t	*gpr;
	int				wsid = -2;
	uint32_t			val;

	if (win == NULL)
		return (-2);

	gpr = xcb_get_property_reply(conn, xcb_get_property(conn, 0, win->id,
	    ewmh[_NET_WM_DESKTOP].atom, XCB_ATOM_CARDINAL, 0, 1), NULL);
	if (gpr && gpr->type == XCB_ATOM_CARDINAL && gpr->format == 32) {
		val = *((uint32_t *)xcb_get_property_value(gpr));
		DNPRINTF(SWM_D_PROP, "get _NET_WM_DESKTOP: %#x\n", val);
		wsid = (val == EWMH_ALL_DESKTOPS ? -1 : (int)val);
	}
	free(gpr);

	if (wsid == -2 && !(win->quirks & SWM_Q_IGNORESPAWNWS))
		wsid = get_swm_ws(win->id);

	if (wsid >= workspace_limit || wsid < -2)
		wsid = -2;

	DNPRINTF(SWM_D_PROP, "win %#x, wsid: %d\n", win->id, wsid);

	return (wsid);
}

static int
reparent_window(struct ws_win *win)
{
	xcb_screen_t		*s;
	xcb_void_cookie_t	c;
	xcb_generic_error_t	*error;
	uint32_t		wa[3];

	if (win_reparented(win)) {
		DNPRINTF(SWM_D_MISC, "win %#x already reparented.\n", win->id);
		return (0);
	}

	win->frame = xcb_generate_id(conn);

	DNPRINTF(SWM_D_MISC, "win %#x (f:%#x)\n", win->id, win->frame);

	if ((s = get_screen(win->s->idx)) == NULL)
		errx(1, "ERROR: unable to get screen %d.", win->s->idx);
	wa[0] = s->black_pixel;
	wa[1] = XCB_EVENT_MASK_BUTTON_PRESS |
	    XCB_EVENT_MASK_BUTTON_RELEASE |
	    XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_LEAVE_WINDOW |
	    XCB_EVENT_MASK_EXPOSURE |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
	    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	    XCB_EVENT_MASK_FOCUS_CHANGE;
	wa[2] = win->s->colormap;

	xcb_create_window(conn, win->s->depth, win->frame, win->s->root, X(win),
	    Y(win), WIDTH(win), HEIGHT(win), 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
	    win->s->visual, XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK |
	    XCB_CW_COLORMAP, wa);

	c = xcb_reparent_window_checked(conn, win->id, win->frame, 0, 0);
	if ((error = xcb_request_check(conn, c))) {
		DNPRINTF(SWM_D_MISC, "error:\n");
		event_error(error);
		free(error);

		/* Abort. */
		xcb_destroy_window(conn, win->frame);
		win->frame = XCB_WINDOW_NONE;
		unmanage_window(win);
		return (1);
	} else {
		if (win->mapped) {
			xcb_map_window(conn, win->frame);
			/* Remap will occur. */
			win->unmapping++;
			win->mapping++;
		}

		xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win->id);
		if (!xinput2_raw)
			grab_buttons_win(win->id);
	}

	return (0);
}

static void
unparent_window(struct ws_win *win)
{
	if (!win_reparented(win))
		return;

	if (win->id != XCB_WINDOW_NONE) {
		xcb_reparent_window(conn, win->id, win->s->root, X(win), Y(win));

		if (win->mapped) {
			/* Remap will occur. */
			win->unmapping++;
			win->mapping++;
		}

		xcb_change_save_set(conn, XCB_SET_MODE_DELETE, win->id);
	}

	if (win->debug != XCB_WINDOW_NONE) {
		xcb_destroy_window(conn, win->debug);
		win->debug = XCB_WINDOW_NONE;
	}

	xcb_destroy_window(conn, win->frame);
	win->frame = XCB_WINDOW_NONE;
}

static struct ws_win *
manage_window(xcb_window_t id, int spawn_pos, bool mapping)
{
	struct ws_win				*win = NULL, *w;
	struct swm_screen			*s;
	struct swm_region			*r;
	struct pid_e				*p;
	struct quirk				*qp;
	xcb_get_geometry_reply_t		*gr;
	xcb_get_window_attributes_reply_t	*war = NULL;
	uint32_t				i, wa[1], new_flags;
	int					ws_idx, force_ws = -2;
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
		DNPRINTF(SWM_D_MISC, "skip; win %#x (f:%#x) already managed\n",
		    win->id, win->frame);
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
		err(1, "manage_window: win calloc");

	if ((win->st = calloc(1, sizeof(struct swm_stackable))) == NULL)
		err(1, "manage_window: st calloc");

	s = find_screen(gr->root);
	win->st->s = win->s = s; /* this never changes */
	win->st->type = STACKABLE_WIN;
	win->st->win = win;

	win->id = id;
	/* Ignore window border if there is one. */
	WIDTH(win) = gr->width;
	HEIGHT(win) = gr->height;
	X(win) = gr->x + gr->border_width;
	Y(win) = gr->y + gr->border_width;
	win->g_float = win->g; /* Window is initially floating. */
	win->g_floatref_root = true;
	win->g_float_xy_valid = false;
	win->bordered = false;
	win->maxstackmax = max_layout_maximize;
	win->mapped = (war->map_state != XCB_MAP_STATE_UNMAPPED);
	win->mapping = 0;
	win->unmapping = 0;
	win->strut = NULL;
	win->main = win;
	win->parent = NULL;

	free(gr);

	/* Select which X events to monitor and set border pixel color. */
	wa[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_PROPERTY_CHANGE |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	xcb_change_window_attributes(conn, win->id, XCB_CW_EVENT_MASK, wa);

	/* Get WM_SIZE_HINTS. */
	get_wm_normal_hints(win);
	win->gravity = win_gravity(win);
	update_gravity(win);

	/* Get WM_HINTS. */
	get_wm_hints(win);

	/* Get WM_TRANSIENT_FOR/update parent. */
	if (get_wm_transient_for(win))
		win->main = find_main_window(win);

	/* Only updates other wins (not in list yet.) */
	update_win_refs(win);

	/* Redirect focus from any parent windows. */
	set_focus_redirect(win);

	get_wm_protocols(win);
	ewmh_get_window_type(win);
	ewmh_get_strut(win);

	if (swm_debug & SWM_D_MISC) {
		DNPRINTF(SWM_D_MISC, "window type: ");
		ewmh_print_window_type(win->type);
		DPRINTF("\n");
	}

	/* Must be after getting WM_HINTS and WM_PROTOCOLS. */
	DNPRINTF(SWM_D_FOCUS, "input_model: %s\n",
	    get_win_input_model_label(win));

	/* Determine initial quirks. */
	xcb_icccm_get_wm_class_reply(conn,
	    xcb_icccm_get_wm_class(conn, win->id),
	    &win->ch, NULL);

	class = win->ch.class_name ? win->ch.class_name : "";
	instance = win->ch.instance_name ? win->ch.instance_name : "";
	name = get_win_name(win->id);

	DNPRINTF(SWM_D_CLASS, "class: %s, instance: %s, name: %s, type: %u\n",
	    class, instance, name, win->type);

	TAILQ_FOREACH(qp, &quirks, entry) {
		if (regexec(&qp->regex_class, class, 0, NULL, 0) == 0 &&
		    regexec(&qp->regex_instance, instance, 0, NULL, 0) == 0 &&
		    regexec(&qp->regex_name, name, 0, NULL, 0) == 0 &&
		    (qp->type == 0 || win->type & qp->type)) {
			DNPRINTF(SWM_D_CLASS, "matched quirk: %s:%s:%s:%u "
			    "mode: %u, mask: %#x, ws: %d\n", qp->class,
			    qp->instance, qp->name, qp->type, qp->mode,
			    qp->quirk, qp->ws);
			switch (qp->mode) {
			case SWM_ASOP_ADD:
				win->quirks |= qp->quirk;
				break;
			case SWM_ASOP_SUBTRACT:
				win->quirks &= ~qp->quirk;
				break;
			case SWM_ASOP_BASIC:
			default:
				win->quirks = qp->quirk;
				break;
			}

			if (qp->ws == -2)
				continue;

			if (qp->ws >= -1 && qp->ws < workspace_limit) {
				if (qp->mode == SWM_ASOP_SUBTRACT) {
					if (qp->ws == force_ws)
						force_ws = -2;
				} else
					force_ws = qp->ws;
			} else if (qp->ws == -3)
				force_ws = -2;
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
	if (!win_main(win)) {
		win->ws = win->main->ws;
	} else if (!(win->quirks & SWM_Q_IGNOREPID) &&
	    (p = find_pid(window_get_pid(win->id))) != NULL) {
		win->ws = get_workspace(s, p->ws);
		TAILQ_REMOVE(&pidlist, p, entry);
		free(p);
		p = NULL;
	} else if ((ws_idx = get_ws_id(win)) != -2 && !win_transient(win)) {
		/* _SWM_WS is set; use that. */
		win->ws = get_workspace(s, ws_idx);
	} else if ((r = get_current_region(s)) && r->ws)
		win->ws = r->ws;
	else
		win->ws = get_workspace(s, 0);

	if (force_ws != -2)
		win->ws = get_workspace(s, force_ws);

	if (win->ws == NULL)
		win->ws = s->r->ws; /* Failsafe. */

	/* WS must be valid before adding to managed list. */
	TAILQ_INSERT_TAIL(&s->managed, win, manage_entry);
	s->managed_count++;

	/* Set the _NET_WM_DESKTOP atom. */
	DNPRINTF(SWM_D_PROP, "set _NET_WM_DESKTOP: %d\n", win->ws->idx);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
	    ewmh[_NET_WM_DESKTOP].atom, XCB_ATOM_CARDINAL, 32, 1,
	    &win->ws->idx);

	/* Remove any _SWM_WS now that we set _NET_WM_DESKTOP. */
	xcb_delete_property(conn, win->id, a_swm_ws);

	/* Initial position was specified by the program/user. */
	if ((SH_POS(win) && (win->quirks & SWM_Q_ANYWHERE || WINDOCK(win) ||
	    WINDESKTOP(win) || ws_floating(win->ws))) || SH_UPOS(win))
		win->g_float_xy_valid = true;

	if (win->ws->r) {
		/* On MapRequest, update the reference point. */
		if (mapping) {
			win->g_floatref = win->ws->r->g;
			win->g_floatref_root = false;
		}
		/* Make sure window has at least 1px inside its region. */
		contain_window(win, get_boundary(win), boundary_width,
		    SWM_CW_ALLSIDES | SWM_CW_HARDBOUNDARY);
		update_window(win);
	}

	/* Figure out where to insert the window in the workspace list. */
	if (win->parent && win->ws == win->parent->ws)
		TAILQ_INSERT_AFTER(&win->parent->ws->winlist, win->parent, win,
		    entry);
	else if (!win_main(win) && win->ws == win->main->ws)
		TAILQ_INSERT_AFTER(&win->main->ws->winlist, win->main, win,
		    entry);
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

	/* The most recent focus, after current focus. */
	if (s->focus && (w = TAILQ_FIRST(&s->fl)))
		TAILQ_INSERT_AFTER(&s->fl, w, win, focus_entry);
	else
		TAILQ_INSERT_HEAD(&s->fl, win, focus_entry);

	/* Same for the priority queue. */
	if (s->focus && (w = TAILQ_FIRST(&s->priority)))
		TAILQ_INSERT_AFTER(&s->priority, w, win, priority_entry);
	else
		TAILQ_INSERT_HEAD(&s->priority, win, priority_entry);

	ewmh_update_client_list(s);

	/* Get/apply initial _NET_WM_STATE */
	ewmh_get_wm_state(win);

	/* Apply quirks. */
	new_flags = win->ewmh_flags;

	if (win->quirks & SWM_Q_FLOAT)
		new_flags |= EWMH_F_ABOVE;

	if (win->quirks & SWM_Q_BELOW)
		new_flags |= EWMH_F_BELOW;

	if (win->quirks & SWM_Q_ICONIFY)
		new_flags |= EWMH_F_HIDDEN;

	if (win->quirks & SWM_Q_ANYWHERE)
		new_flags |= SWM_F_MANUAL;

	if (win->quirks & SWM_Q_MAXIMIZE ||
	    (win->maxstackmax && ws_maxstack(win->ws)))
		new_flags |= EWMH_F_MAXIMIZED;

	ewmh_apply_flags(win, new_flags);
	ewmh_update_wm_state(win);

	/* Set initial _NET_WM_ALLOWED_ACTIONS */
	ewmh_update_actions(win);

	update_win_layer_related(win);

	if (reparent_window(win) == 0) {
		refresh_stack(s);
		update_stacking(s);

		if (win->ws->r && !HIDDEN(win))
			map_window(win);
		else
			unmap_window(win);

		update_window(win);

		DNPRINTF(SWM_D_MISC, "done. win %#x, (x,y) w x h: (%d,%d) "
		    "%d x %d, ws: %d, iconic: %s, transient: %#x\n", win->id,
		    X(win), Y(win), WIDTH(win), HEIGHT(win), win->ws->idx,
		    YESNO(HIDDEN(win)), win->transient_for);
	} else {
		/* Failed to manage. */
		win = NULL;
	}
out:
	free(war);
	return (win);
}

static const char *
get_gravity_label(uint8_t val)
{
	const char	*label = NULL;

	switch(val) {
	case XCB_GRAVITY_NORTH_WEST:
		label = "NorthWest";
		break;
	case XCB_GRAVITY_NORTH:
		label = "North";
		break;
	case XCB_GRAVITY_NORTH_EAST:
		label = "NorthEast";
		break;
	case XCB_GRAVITY_WEST:
		label = "West";
		break;
	case XCB_GRAVITY_CENTER:
		label = "Center";
		break;
	case XCB_GRAVITY_EAST:
		label = "East";
		break;
	case XCB_GRAVITY_SOUTH_WEST:
		label = "SouthWest";
		break;
	case XCB_GRAVITY_SOUTH:
		label = "South";
		break;
	case XCB_GRAVITY_SOUTH_EAST:
		label = "SouthEast";
		break;
	case XCB_GRAVITY_STATIC:
		label = "Static";
		break;
	default:
		label = "Unknown";
		break;
	}

	return (label);
}

static void
update_gravity(struct ws_win *win)
{
	switch (win->gravity) {
	case XCB_GRAVITY_NORTH_WEST:
		win->g_grav.x = win->g_grav.y = win_border(win);
		break;
	case XCB_GRAVITY_NORTH:
		win->g_grav.x = -win->g_float.w / 2;
		win->g_grav.y = win_border(win);
		break;
	case XCB_GRAVITY_NORTH_EAST:
		win->g_grav.x = -win->g_float.w - win_border(win);
		win->g_grav.y = win_border(win);
		break;
	case XCB_GRAVITY_WEST:
		win->g_grav.x = win_border(win);
		win->g_grav.y = -win->g_float.h / 2;
		break;
	case XCB_GRAVITY_CENTER:
		win->g_grav.x = -win->g_float.w / 2;
		win->g_grav.y = -win->g_float.h / 2;
		break;
	case XCB_GRAVITY_EAST:
		win->g_grav.x = -win->g_float.w - win_border(win);
		win->g_grav.y = -win->g_float.h / 2;
		break;
	case XCB_GRAVITY_SOUTH_WEST:
		win->g_grav.x = win_border(win);
		win->g_grav.y = -win->g_float.h - win_border(win);
		break;
	case XCB_GRAVITY_SOUTH:
		win->g_grav.x = -win->g_float.w / 2;
		win->g_grav.y = -win->g_float.h - win_border(win);
		break;
	case XCB_GRAVITY_SOUTH_EAST:
		win->g_grav.x = -win->g_float.w - win_border(win);
		win->g_grav.y = -win->g_float.h - win_border(win);
		break;
	case XCB_GRAVITY_STATIC:
	default:
		win->g_grav.x = win->g_grav.y = 0;
		break;
	}

	DNPRINTF(SWM_D_MISC, "win %#x, g_grav (x,y): (%d,%d) gravity: %s\n",
	    win->id, win->g_grav.x, win->g_grav.y,
	    get_gravity_label(win->gravity));
}

static void
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

static void
unmanage_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "win %#x (f:%#x)\n", win->id, win->frame);

	kill_refs(win);
	unparent_window(win);

	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	TAILQ_REMOVE(&win->s->fl, win, focus_entry);
	TAILQ_REMOVE(&win->s->priority, win, priority_entry);
	TAILQ_REMOVE(&win->s->managed, win, manage_entry);
	win->s->managed_count--;

	if (win->strut) {
		SLIST_REMOVE(&win->s->struts, win->strut, swm_strut, entry);
		free(win->strut);
		win->strut = NULL;
	}

	free_stackable(win->st);
	ewmh_update_client_list(win->s);
	free_window(win);
}

static const char *
get_randr_event_label(xcb_generic_event_t *e)
{
	const char		*label = NULL;
	uint8_t			type = XCB_EVENT_RESPONSE_TYPE(e);

	if ((type - randr_eventbase) == XCB_RANDR_SCREEN_CHANGE_NOTIFY)
		label = "RRScreenChangeNotify";
#ifdef XCB_RANDR_NOTIFY /* RandR 1.2 */
	else if ((type - randr_eventbase) == XCB_RANDR_NOTIFY) {
		switch (((xcb_randr_notify_event_t *)e)->subCode) {
		case XCB_RANDR_NOTIFY_CRTC_CHANGE:
			label = "RRCrtcChangeNotify";
			break;
		case XCB_RANDR_NOTIFY_OUTPUT_CHANGE:
			label = "RROutputChangeNotify";
			break;
		case XCB_RANDR_NOTIFY_OUTPUT_PROPERTY:
			label = "RROutputPropertyNotify";
			break;
#ifdef XCB_RANDR_NOTIFY_PROVIDER_CHANGE /* RandR 1.4 */
		case XCB_RANDR_NOTIFY_PROVIDER_CHANGE:
			label = "RRProviderChangeNotify";
			break;
		case XCB_RANDR_NOTIFY_PROVIDER_PROPERTY:
			label = "RRProviderPropertyNotify";
			break;
		case XCB_RANDR_NOTIFY_RESOURCE_CHANGE:
			label = "RResourceChangeNotify";
			break;
#ifdef XCB_RANDR_NOTIFY_LEASE /* RandR 1.6 */
		case XCB_RANDR_NOTIFY_LEASE:
			label = "RRLeaseNotify";
			break;
#endif /* XCB_RANDR_NOTIFY_LEASE */
#endif /* XCB_RANDR_NOTIFY_PROVIDER_CHANGE */
		default:
			label = "RRNotifyUnknown";
		}
	}
#endif /* XCB_RANDR_NOTIFY */

	return (label);
}

static const char *
get_event_label(xcb_generic_event_t *e)
{
	const char	*label = NULL;
	uint8_t		type = XCB_EVENT_RESPONSE_TYPE(e);

	if (type <= XCB_MAPPING_NOTIFY)
		label = xcb_event_get_label(type);
	else if (type == XCB_GE_GENERIC) {
#ifdef SWM_XCB_HAS_XINPUT
		if (xinput2_support &&
		    ((xcb_ge_generic_event_t *)e)->extension == xinput2_opcode)
			label = get_input_event_label(
			    (xcb_ge_generic_event_t *)e);
		else
#endif
			label = "Unknown GenericEvent";
	} else if (randr_support &&
	    ((type - randr_eventbase) == XCB_RANDR_SCREEN_CHANGE_NOTIFY
#ifdef XCB_RANDR_NOTIFY /* RandR 1.2 */
	    || (type - randr_eventbase) == XCB_RANDR_NOTIFY
#endif
	    ))
		label = get_randr_event_label(e);
	else
		label = "Unknown Event";

	return (label);
}

/* events */
static void
expose(xcb_expose_event_t *e)
{
	struct ws_win		*w;
	struct swm_bar		*b;

	DNPRINTF(SWM_D_EVENT, "win %#x, count: %d\n", e->window, e->count);

	if (e->count > 0)
		return;

	if ((b = find_bar(e->window))) {
		bar_draw(b);
		xcb_flush(conn);
	} else if ((w = find_window(e->window)) && w->frame == e->window) {
		draw_frame(w);
		debug_refresh(w);
		xcb_flush(conn);
	}

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
focusin(xcb_focus_in_event_t *e)
{
	struct swm_region	*r;
	struct ws_win		*win;
	struct swm_screen	*s = NULL;

	DNPRINTF(SWM_D_EVENT, "win %#x, mode: %s(%u), detail: %s(%u)\n",
	    e->event, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail);

	if (e->detail == XCB_NOTIFY_DETAIL_POINTER ||
	    e->detail == XCB_NOTIFY_DETAIL_VIRTUAL)
		return;

	if (e->mode == XCB_NOTIFY_MODE_GRAB &&
	    e->detail == XCB_NOTIFY_DETAIL_INFERIOR)
		return;

	if (e->mode == XCB_NOTIFY_MODE_WHILE_GRABBED &&
	    e->detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL)
		return;

	/* Managed window. */
	if ((win = find_window(e->event))) {
		/* Ignore if there is a current focus and its unrelated. */
		if (win->mapped && win != win->ws->focus &&
		    (win->ws->focus == NULL ||
		    win_related(win, win->ws->focus))) {
			set_focus(win->s, win);
			draw_frame(get_ws_focus_prev(win->ws));
			draw_frame(win);
			update_stacking(win->s);
			flush();
		}
	} else {
		/* Redirect focus if needed. */
		r = find_region(e->event);
		if (r)
			s = r->s;
		else
			s = find_screen(e->event);

		if (s && !follow_pointer(s, SWM_FOCUS_TYPE_BORDER)) {
			win = s->focus;
			if (win == NULL && s->r_focus)
				win = get_focus_magic(
				    get_ws_focus(s->r_focus->ws));
			if (win) {
				focus_win_input(win, false);
				set_focus(s, win);
				draw_frame(win);
				draw_frame(get_ws_focus_prev(win->ws));
				flush();
			}
		}
	}
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
focusout(xcb_focus_out_event_t *e)
{
	DNPRINTF(SWM_D_EVENT, "win %#x, mode: %s(%u), detail: %s(%u)\n",
	    e->event, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail);
}

static void
keypress(xcb_key_press_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;
	xcb_keysym_t		keysym;
	bool			replay = true;

	event_time = e->time;

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
		spawn_custom(find_screen(e->root), &ap->args, bp->spawn_name);
	else if (ap->func)
		ap->func(find_screen(e->root), bp, &ap->args);

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

static void
keyrelease(xcb_key_release_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;
	xcb_keysym_t		keysym;

	event_time = e->time;

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

static void
buttonpress(xcb_button_press_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;
	struct ws_win		*w;
	xcb_window_t		winid;
	bool			replay = true;

	event_time = e->time;

	DNPRINTF(SWM_D_EVENT, "win (x,y): %#x (%d,%d), detail: %u, time: %#x, "
	    "root (x,y): %#x (%d,%d), child: %#x, state: %u, same_screen: %s\n",
	    e->event, e->event_x, e->event_y, e->detail, e->time, e->root,
	    e->root_x, e->root_y, e->child, e->state, YESNO(e->same_screen));

	if (e->event == e->root)
		winid = e->child;
	else
		winid = e->event;

	/* Try to find bound action with current modifier state. */
	bp = binding_lookup(CLEANMASK(e->state), BTNBIND, e->detail);
	if (bp == NULL) {
		/* Look for catch-all. */
		bp = binding_lookup(ANYMOD, BTNBIND, e->detail);
		if (bp == NULL)
			goto out;
	}

	if (xinput2_raw && bp->flags & BINDING_F_REPLAY)
		goto out;

	/* Button binding is not pass-through */

	click_focus(find_screen(e->root), winid, e->root_x, e->root_y);

	replay = bp->flags & BINDING_F_REPLAY;

	if ((ap = &actions[bp->action]) == NULL)
		goto out;

	if (bp->action == FN_SPAWN_CUSTOM)
		spawn_custom(find_screen(e->root), &ap->args, bp->spawn_name);
	else if (ap->func)
		ap->func(find_screen(e->root), bp, &ap->args);

	replay = replay && !(ap->flags & FN_F_NOREPLAY);
out:
	/* Only release grabs on windows with grabs. */
	if ((xinput2_raw && e->event == e->root) ||
	    ((w = find_window(winid)) && w->id == e->event)) {
		if (replay) {
			DNPRINTF(SWM_D_EVENT, "replaying\n");
			xcb_allow_events(conn, XCB_ALLOW_REPLAY_POINTER,
			    e->time);
		} else {
			DNPRINTF(SWM_D_EVENT, "unfreezing\n");
			xcb_allow_events(conn, XCB_ALLOW_SYNC_POINTER, e->time);
		}
	}

	flush();
	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static void
buttonrelease(xcb_button_release_event_t *e)
{
	struct action		*ap;
	struct binding		*bp;
	struct ws_win		*w;

	event_time = e->time;

	DNPRINTF(SWM_D_EVENT, "win (x,y): %#x (%d,%d), detail: %u, time: %#x, "
	    "root (x,y): %#x (%d,%d), child: %#x, state: %u, same_screen: %s\n",
	    e->event, e->event_x, e->event_y, e->detail, e->time, e->root,
	    e->root_x, e->root_y, e->child, e->state, YESNO(e->same_screen));

	/* Only release grabs on windows with grabs. */
	if ((xinput2_raw && e->event == e->root) ||
	    ((w = find_window(e->event)) && w->id == e->event)) {
		bp = binding_lookup(CLEANMASK(e->state), BTNBIND, e->detail);
		if (bp == NULL)
			/* Look for catch-all. */
			bp = binding_lookup(ANYMOD, BTNBIND, e->detail);

		if (bp && bp->flags & BINDING_F_REPLAY &&
		    (((ap = &actions[bp->action]) == NULL) ||
		    !(ap->flags & FN_F_NOREPLAY))) {
			DNPRINTF(SWM_D_EVENT, "replaying\n");
			xcb_allow_events(conn, XCB_ALLOW_REPLAY_POINTER,
			    e->time);
		} else {
			DNPRINTF(SWM_D_EVENT, "unfreezing\n");
			xcb_allow_events(conn, XCB_ALLOW_SYNC_POINTER, e->time);
		}

		xcb_flush(conn);
	}

	DNPRINTF(SWM_D_FOCUS, "done\n");
}

static const char *
get_win_input_model_label(struct ws_win *win)
{
	const char	*inputmodel;
	/*
	 *	Input Model		Input Field	WM_TAKE_FOCUS
	 *	No Input		False		Absent
	 *	Passive			True		Absent
	 *	Locally Active		True		Present
	 *	Globally Active		False		Present
	 */

	if (accepts_focus(win))
		inputmodel = (win->take_focus) ? "Locally Active" : "Passive";
	else
		inputmodel = (win->take_focus) ? "Globally Active" : "No Input";

	return (inputmodel);
}

static void
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

static const char *
get_stack_mode_label(uint8_t mode)
{
	const char	*label;

	switch (mode) {
	case XCB_STACK_MODE_ABOVE:
		label = "Above";
		break;
	case XCB_STACK_MODE_BELOW:
		label = "Below";
		break;
	case XCB_STACK_MODE_TOP_IF:
		label = "TopIf";
		break;
	case XCB_STACK_MODE_BOTTOM_IF:
		label = "BottomIf";
		break;
	case XCB_STACK_MODE_OPPOSITE:
		label = "Opposite";
		break;
	default:
		label = "Unknown";
	}

	return (label);
}

static void
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
			    get_stack_mode_label(e->stack_mode), e->stack_mode);
		DPRINTF("}\n");
	}

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
	} else if ((!MANUAL(win) || win->quirks & SWM_Q_ANYWHERE ||
	    ws_floating(win->ws)) && !FULLSCREEN(win) && !MAXIMIZED(win)) {
		/* Update floating geometry. */
		if (win->ws->r)
			r = win->ws->r;
		else if (win->ws->old_r)
			r = win->ws->old_r;

		/* Assume position is in reference to latest region. */
		if (r) {
			win->g_floatref = r->g;
			win->g_floatref_root = false;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_X) {
			win->g_float.x = e->x;
			win->g_float_xy_valid = true;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
			win->g_float.y = e->y;
			win->g_float_xy_valid = true;
		}
		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
			win->g_float.w = e->width;
		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
			win->g_float.h = e->height;

		if (e->value_mask &
		    (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT))
			update_gravity(win);

		if (!MAXIMIZED(win) && !FULLSCREEN(win) && win_floating(win)) {
			/* Immediately apply new geometry if mapped. */
			if (win->mapped) {
				xcb_window_t wid = pointer_window;
				update_floater(win);
				flush();
				if (follow_mode(SWM_FOCUS_TYPE_CONFIGURE) &&
				    pointer_window != wid) {
					focus_window(pointer_window);
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
	} else {
		config_win(win, e);
		xcb_flush(conn);
	}

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
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
			update_mapping(win->s);
			xcb_flush(conn);
		}
	}
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
destroynotify(xcb_destroy_notify_event_t *e)
{
	struct ws_win		*win;
	struct workspace	*ws;
	struct swm_screen	*s;
	bool			follow;

	DNPRINTF(SWM_D_EVENT, "win %#x\n", e->window);

	if ((win = find_window(e->window)) == NULL) {
		DNPRINTF(SWM_D_EVENT, "ignore; unmanaged.\n");
		return;
	}

	if (win->frame == e->window) {
		DNPRINTF(SWM_D_EVENT, "ignore; frame for win %#x\n", win->id);
		win->frame = XCB_WINDOW_NONE;
		return;
	}

	s = win->s;
	ws = win->ws;
	win->id = XCB_WINDOW_NONE;

	follow = follow_pointer(s, SWM_FOCUS_TYPE_UNMAP);

	if (win == ws->focus)
		ws->focus = get_focus_magic(get_focus_other(win));

	unmanage_window(win);

	if (ws->r) {
		if (refresh_strut(s))
			update_layout(s);
		else
			stack(ws->r);
		update_mapping(s);

		if (!follow) {
			update_focus(s);
			center_pointer(ws->r);
		}
		bar_draw(ws->r->bar);

		flush(); /* win can be freed. */
		if (follow) {
			if (pointer_window != XCB_WINDOW_NONE)
				focus_window(pointer_window);
			else if (ws->focus == NULL)
				focus_win(s, get_focus_magic(get_ws_focus(ws)));
		}
	}
	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static const char *
get_notify_detail_label(uint8_t detail)
{
	const char *label;

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

static const char *
get_notify_mode_label(uint8_t mode)
{
	const char *label;

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

static const char *
get_state_mask_label(uint16_t state)
{
	const char *label;

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

static const char *
get_wm_state_label(uint32_t state)
{
	const char *label;
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

static void
enternotify(xcb_enter_notify_event_t *e)
{
	struct ws_win		*win = NULL;

	event_time = e->time;
	pointer_window = e->event;

	DNPRINTF(SWM_D_FOCUS, "time: %#x, win (x,y): %#x (%d,%d), mode: %s(%d),"
	    " detail: %s(%d), root (x,y): %#x (%d,%d), child: %#x, "
	    "same_screen_focus: %s, state: %s(%d)\n", e->time, e->event,
	    e->event_x, e->event_y, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail, e->root, e->root_x,
	    e->root_y, e->child, YESNO(e->same_screen_focus),
	    get_state_mask_label(e->state), e->state);

	if (!follow_mode(SWM_FOCUS_TYPE_BORDER)) {
		DNPRINTF(SWM_D_EVENT, "ignore; manual focus\n");
		return;
	}

	if (e->event == e->root) {
		if (e->child == XCB_WINDOW_NONE &&
		    e->mode == XCB_NOTIFY_MODE_GRAB &&
		    e->detail == XCB_NOTIFY_DETAIL_INFERIOR) {
			DNPRINTF(SWM_D_EVENT, "ignore; grab inferior\n");
			return;
		}
	} else {
		/* Ignore enternotify within managed windows. */
		if (e->detail == XCB_NOTIFY_DETAIL_INFERIOR) {
			DNPRINTF(SWM_D_EVENT, "ignore; enter from inferior\n");
			return;
		}

		win = find_window(e->event);

		/* Only handle enternotify on frame. */
		if (win && e->event != win->frame &&
		    e->detail == XCB_NOTIFY_DETAIL_ANCESTOR) {
			DNPRINTF(SWM_D_EVENT, "ignore; enter client from "
			    "ancestor\n");
			return;
		}
	}

	if (win && win_free(win)) {
		set_region(get_pointer_region(win->s));
		focus_win(win->s, get_focus_magic(win));
	} else
		focus_window(e->event);
	xcb_flush(conn);
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
leavenotify(xcb_leave_notify_event_t *e)
{
	event_time = e->time;

	DNPRINTF(SWM_D_FOCUS, "time: %#x, win (x,y): %#x (%d,%d), mode: %s(%d),"
	    " detail: %s(%d), root (x,y): %#x (%d,%d), child: %#x, "
	    "same_screen_focus: %s, state: %s(%d)\n", e->time, e->event,
	    e->event_x, e->event_y, get_notify_mode_label(e->mode), e->mode,
	    get_notify_detail_label(e->detail), e->detail, e->root, e->root_x,
	    e->root_y, e->child, YESNO(e->same_screen_focus),
	    get_state_mask_label(e->state), e->state);
}

static void
mapnotify(xcb_map_notify_event_t *e)
{
	struct ws_win		*win;

	if (!(swm_debug & SWM_D_EVENT))
		return;

	DNPRINTF(SWM_D_EVENT, "event: %#x, win %#x, override_redirect: %u\n",
	    e->event, e->window, e->override_redirect);

	if (e->event != e->window) {
		DNPRINTF(SWM_D_EVENT, "not event window.\n");
		return;
	}

	if ((win = manage_window(e->window, spawn_position, false)) == NULL) {
		DNPRINTF(SWM_D_EVENT, "unmanaged.\n");
		return;
	}

	if (win->id != e->window && win->frame != e->window) {
		DNPRINTF(SWM_D_EVENT, "unknown window.\n");
		return;
	}

	if (win->mapping > 0) {
		DNPRINTF(SWM_D_EVENT, "mapping %d\n", win->mapping);
		win->mapping--;
		return;
	}
}

static const char *
get_mapping_notify_label(uint8_t request)
{
	const char *label;

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

static void
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
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
click_focus(struct swm_screen *s, xcb_window_t id, int x, int y)
{
	struct swm_region	*r, *rr;
	struct swm_bar		*bar;
	struct ws_win		*win, *w;

	win = find_window(id);
	if (win) {
		if (win_free(win))
			set_region(region_under(s, x, y));
		else if (apply_unfocus(s->r->ws, NULL)) {
			refresh_stack(s);
			update_stacking(s);
			stack(s->r);
			update_mapping(s);
		}

		w = get_focus_magic(win);
		if (w != win && w == win->ws->focus &&
		    win->focus_redirect == win)
			win->focus_redirect = NULL;
		focus_win(s, win);
		if (click_to_raise && !win_prioritized(win)) {
			prioritize_window(win);
			refresh_stack(win->s);
			update_stacking(win->s);
		}
	} else {
		/* Not a managed window, figure out region. */
		r = find_region(id);
		if (r == NULL) {
			bar = find_bar(id);
			if (bar)
				r = bar->r;

			/* Fallback to pointer location. */
			if (r == NULL)
				r = region_under(s, x, y);
		}

		/* If region is empty, set default focus. */
		if (r && TAILQ_EMPTY(&r->ws->winlist)) {
			w = find_window(get_input_focus());
			if (w) {
				rr = w->ws->r;
				if (rr && rr != r)
					unfocus_win(rr->ws->focus);
			}

			set_input_focus(r->id, false);
			bar_draw(r->bar);
		}

		focus_region(r);
	}
}

static void
focus_window(xcb_window_t id)
{
	struct ws_win			*w;

	DNPRINTF(SWM_D_FOCUS, "id: %#x\n", id);

	if (id == XCB_WINDOW_NONE)
		return;

	if ((w = find_window(id)))
		focus_win(w->s, get_focus_magic(w));
	else
		focus_window_region(id);
}

static void
focus_window_region(xcb_window_t id)
{
	struct swm_region		*r;
	struct swm_bar			*b;
	xcb_get_geometry_reply_t	*gr;

	DNPRINTF(SWM_D_FOCUS, "id: %#x\n", id);

	r = find_region(id);
	if (r == NULL && (b = find_bar(id)))
		r = b->r;
	if (r == NULL) {
		gr = xcb_get_geometry_reply(conn,
		    xcb_get_geometry(conn, id), NULL);
		if (gr == NULL) {
			DNPRINTF(SWM_D_MISC, "get geometry failed\n");
			return;
		}
		r = get_pointer_region(find_screen(gr->root));
		free(gr);
	}
	focus_region(r);
}

static void
maprequest(xcb_map_request_event_t *e)
{
	struct swm_screen	*s;
	struct workspace	*ws;
	struct ws_win		*win, *w;
	bool			follow, setfocus = false;

	DNPRINTF(SWM_D_EVENT, "win %#x, parent: %#x\n", e->window, e->parent);

	if ((win = manage_window(e->window, spawn_position, true)) == NULL)
		return;

	s = win->s;
	ws = win->ws;
	win->mapped = false;

	follow = follow_pointer(s, SWM_FOCUS_TYPE_MAP);
	/* Set new focus unless a quirk says otherwise. */
	if (!follow && !(win->quirks & SWM_Q_NOFOCUSONMAP)) {
		w = NULL;
		if (win->quirks & SWM_Q_FOCUSONMAP_SINGLE) {
			/* See if other wins of same type are already mapped. */
			TAILQ_FOREACH(w, &ws->winlist, entry) {
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
			setfocus = true;
	}

	if (ewmh_apply_flags(win, win->ewmh_flags & ~EWMH_F_HIDDEN))
		ewmh_update_wm_state(win);

	if (setfocus) {
		set_focus(s, get_focus_magic(win));
		draw_frame(get_ws_focus_prev(ws));
	}

	prioritize_window(win);
	update_win_layer_related(win);

	apply_unfocus(ws, win);
	refresh_stack(s);
	update_stacking(s);

	if (ws->r) {
		if (refresh_strut(s))
			update_layout(s);
		else
			stack(ws->r);
		update_mapping(s);

		if (!follow) {
			update_focus(s);
			center_pointer(ws->r);
		}

		flush(); /* win can be freed. */
		if (follow) {
			if (pointer_window != XCB_WINDOW_NONE)
				focus_window(pointer_window);
			else if (ws->focus == NULL && validate_win(win) == 0)
				focus_win(s, get_focus_magic(win));
		}
	}
	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
motionnotify(xcb_motion_notify_event_t *e)
{
	struct swm_screen	*s;
	struct swm_region	*r = NULL;
	struct ws_win		*win = NULL;

	event_time = e->time;

	DNPRINTF(SWM_D_FOCUS, "time: %#x, win (x,y): %#x (%d,%d), "
	    "detail: %s(%d), root (x,y): %#x (%d,%d), child: %#x, "
	    "same_screen_focus: %s, state: %d\n", e->time, e->event, e->event_x,
	    e->event_y, get_notify_detail_label(e->detail), e->detail, e->root,
	    e->root_x, e->root_y, e->child, YESNO(e->same_screen), e->state);

	if (follow_mode(SWM_FOCUS_TYPE_BORDER)) {
		if (pointer_window != XCB_WINDOW_NONE)
			win = get_focus_magic(find_window(pointer_window));
		if (win == NULL && e->event == e->root)
			win = find_window(e->child);
	}

	if (win)
		focus_win(win->s, win);
	else if (follow_mode(SWM_FOCUS_TYPE_BORDER)) {
		if (pointer_window != XCB_WINDOW_NONE)
			focus_window_region(pointer_window);
		else if ((s = find_screen(e->root)) != NULL) {
			TAILQ_FOREACH(r, &s->rl, entry)
				if (X(r) <= e->root_x && e->root_x < MAX_X(r) &&
				    Y(r) <= e->root_y && e->root_y < MAX_Y(r))
					break;
			focus_region(r);
		}
	}
	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
propertynotify(xcb_property_notify_event_t *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "win %#x, atom: %s(%u), time: %#x, state: %u\n",
	    e->window, get_atom_label(e->atom), e->atom, e->time, e->state);

	event_time = e->time;

	win = find_window(e->window);
	if (win == NULL)
		return;

	if (e->atom == XCB_ATOM_WM_CLASS ||
	    e->atom == XCB_ATOM_WM_NAME) {
		if (win->ws->r)
			bar_draw(win->ws->r->bar);
	} else if (e->atom == XCB_ATOM_WM_HINTS) {
		get_wm_hints(win);
		draw_frame(win);
		update_bars(win->s);
	} else if (e->atom == XCB_ATOM_WM_NORMAL_HINTS) {
		get_wm_normal_hints(win);
		win->gravity = win_gravity(win);
	} else if (e->atom == XCB_ATOM_WM_TRANSIENT_FOR) {
		if (get_wm_transient_for(win))
			update_win_refs(win);
		if (win->main->ws != win->ws)
			win_to_ws(win, win->main->ws, SWM_WIN_UNFOCUS);
		update_win_layer_related(win);
		refresh_stack(win->s);
		update_stacking(win->s);
	} else if (e->atom == ewmh[_NET_WM_STRUT_PARTIAL].atom ||
	    e->atom == ewmh[_NET_WM_STRUT].atom) {
		ewmh_get_strut(win);
		refresh_strut(win->s);
		update_layout(win->s);
	} else if (e->atom == a_prot) {
		get_wm_protocols(win);
	}

	xcb_flush(conn);
	DNPRINTF(SWM_D_EVENT, "done\n");
}

static void
reparentnotify(xcb_reparent_notify_event_t *e)
{
	DNPRINTF(SWM_D_EVENT, "event: %#x, win %#x, parent: %#x, "
	    "(x,y): (%u,%u), override_redirect: %u\n", e->event, e->window,
	    e->parent, e->x, e->y, e->override_redirect);
}

static void
unmapnotify(xcb_unmap_notify_event_t *e)
{
	struct swm_screen	*s;
	struct ws_win		*win, *nfw = NULL;
	struct workspace	*ws;
	bool			follow;

	DNPRINTF(SWM_D_EVENT, "event: %#x, win %#x, from_configure: %u\n",
	    e->event, e->window, e->from_configure);

	if (e->event != e->window) {
		DNPRINTF(SWM_D_EVENT, "ignore; not event window.\n");
		return;
	}

	win = find_window(e->window);
	if (win == NULL || (win->id != e->window && win->frame != e->window)) {
		DNPRINTF(SWM_D_EVENT, "ignore; unmanaged.\n");
		return;
	}

	if (win->unmapping > 0) {
		DNPRINTF(SWM_D_EVENT, "ignore; unmapping %d\n", win->unmapping);
		win->unmapping--;
		return;
	}

	s = win->s;
	ws = win->ws;

	follow = follow_pointer(s, SWM_FOCUS_TYPE_UNMAP);

	if (win->frame == e->window) {
		DNPRINTF(SWM_D_EVENT, "unexpected unmap of frame; fixing.\n");

		if (win == ws->focus)
			nfw = win;

		xcb_map_window(conn, win->frame);
	} else {
		DNPRINTF(SWM_D_EVENT, "unexpected unmap; unmanaging.\n");

		if (win == ws->focus)
			nfw = get_focus_other(win);

		win->mapped = false;

		/* EWMH advises removal of _NET_WM_DESKTOP and _NET_WM_STATE. */
		xcb_delete_property(conn, win->id, ewmh[_NET_WM_DESKTOP].atom);
		xcb_delete_property(conn, win->id, ewmh[_NET_WM_STATE].atom);
		set_win_state(win, XCB_ICCCM_WM_STATE_WITHDRAWN);
		unmanage_window(win);
	}

	if (!follow && nfw)
		set_focus(s, get_focus_magic(nfw));

	refresh_stack(s);
	update_stacking(s);

	if (ws->r) {
		if (refresh_strut(s))
			update_layout(s);
		else
			stack(ws->r);
		update_mapping(s);

		if (!follow) {
			update_focus(s);
			center_pointer(ws->r);
		}
		bar_draw(ws->r->bar);

		flush(); /* win can be freed. */
		if (follow) {
			if (pointer_window != XCB_WINDOW_NONE)
				focus_window(pointer_window);
			else if (ws->focus == NULL && validate_win(nfw) == 0)
				focus_win(s, get_focus_magic(nfw));
		}
	}
	xcb_flush(conn);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static const char *
get_source_type_label(uint32_t type)
{
	const char *label;

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

static uint8_t
win_gravity(struct ws_win *win)
{
	if (SH_GRAVITY(win))
		return (win->sh.win_gravity);
	else
		return (XCB_GRAVITY_NORTH_WEST);
}

static void
clientmessage(xcb_client_message_event_t *e)
{
	struct swm_screen	*s;
	struct swm_region	*r = NULL;
	struct workspace	*ws;
	struct ws_win		*win, *cfw;
	uint32_t		vals[4];
	xcb_map_request_event_t	mre;
	bool			follow = false;

	DNPRINTF(SWM_D_EVENT, "win %#x, atom: %s(%u)\n", e->window,
	    get_atom_label(e->type), e->type);

	if (e->type == ewmh[_NET_CURRENT_DESKTOP].atom) {
		s = find_screen(e->window);
		if (s) {
			r = get_current_region(s);
			if (r && (int)e->data.data32[0] < workspace_limit) {
				follow = follow_mode(SWM_FOCUS_TYPE_WORKSPACE);
				ws = get_workspace(s, e->data.data32[0]);
				if (ws)
					switch_workspace(r, ws, false, follow);
			}
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
			mre.parent = XCB_WINDOW_NONE;
			maprequest(&mre);
		}
		return;
	}

	s = win->s;
	cfw = s->focus;

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
			/* Uniconify. */
			if (ewmh_apply_flags(win,
			    win->ewmh_flags & ~EWMH_F_HIDDEN))
				ewmh_update_wm_state(win);

			set_focus(s, win);
			apply_unfocus(win->ws, win);
			update_win_layer_related(win);
			prioritize_window(win);
			refresh_stack(s);
			update_stacking(s);
			if (refresh_strut(s))
				update_layout(s);
			else
				stack(win->ws->r);
			update_mapping(s);

			if (win->mapped)
				update_focus(s);
			else {
				set_attention(win);
				update_bars(s);
				xcb_flush(conn);
				return;
			}
		} else {
			if (s->focus != win) {
				set_attention(win);
				draw_frame(win);
				update_bars(s);
				xcb_flush(conn);
			}
			return;
		}
	} else if (e->type == ewmh[_NET_CLOSE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_CLOSE_WINDOW\n");
		if (win->can_delete)
			client_msg(win, a_delete, 0);
		else
			xcb_kill_client(conn, win->id);
		xcb_flush(conn);
		return;
	} else if (e->type == ewmh[_NET_MOVERESIZE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_MOVERESIZE_WINDOW\n");
		if ((!MANUAL(win) || win->quirks & SWM_Q_ANYWHERE ||
		    ws_floating(win->ws)) && !FULLSCREEN(win) &&
		    !MAXIMIZED(win)) {
			uint8_t grav;

			if (win->mapped)
				follow = follow_pointer(s,
				    SWM_FOCUS_TYPE_CONFIGURE);

			/* Update floating geometry. */
			if (e->data.data32[0] & (1 << 8)) { /* x */
				win->g_float.x = e->data.data32[1];
				win->g_float_xy_valid = true;
			}
			if (e->data.data32[0] & (1 << 9)) {/* y */
				win->g_float.y = e->data.data32[2];
				win->g_float_xy_valid = true;
			}
			if (e->data.data32[0] & (1 << 10)) /* width */
				win->g_float.w = e->data.data32[3];
			if (e->data.data32[0] & (1 << 11)) /* height */
				win->g_float.h = e->data.data32[4];

			/* Use specified gravity just this once. */
			grav = win->gravity;
			if (e->data.data32[0] & 0xff)
				win->gravity = e->data.data32[0] & 0xff;
			update_gravity(win);

			if (win_floating(win)) {
				load_float_geom(win);
				update_floater(win);
			} else {
				config_win(win, NULL);
				return;
			}

			win->gravity = grav;
		} else {
			config_win(win, NULL);
			return;
		}
	} else if (e->type == ewmh[_NET_RESTACK_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_RESTACK_WINDOW\n");
		vals[0] = e->data.data32[1]; /* Sibling window. */
		vals[1] = e->data.data32[2]; /* Stack mode detail. */

		if (win->frame == XCB_WINDOW_NONE)
			return;

		xcb_configure_window(conn, win->frame, XCB_CONFIG_WINDOW_SIBLING
		    | XCB_CONFIG_WINDOW_STACK_MODE, vals);
		xcb_flush(conn);
		return;
	} else 	if (e->type == ewmh[_NET_WM_STATE].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_WM_STATE\n");
		uint32_t changed;

		changed = ewmh_change_wm_state(win, e->data.data32[1],
		    e->data.data32[0]);
		if (e->data.data32[2])
			changed |= ewmh_change_wm_state(win, e->data.data32[2],
			    e->data.data32[0]);

		if (changed & EWMH_F_HIDDEN)
			follow = follow_pointer(s, HIDDEN(win) ?
			    SWM_FOCUS_TYPE_ICONIFY : SWM_FOCUS_TYPE_UNICONIFY);
		else if (changed & (EWMH_F_FULLSCREEN | EWMH_F_MAXIMIZED))
			follow = follow_pointer(s, SWM_FOCUS_TYPE_CONFIGURE);
		else if (changed & (EWMH_F_BELOW | EWMH_F_ABOVE))
			follow = follow_pointer(s, SWM_FOCUS_TYPE_BORDER);

		if ((s->focus == win) && HIDDEN(win) && !follow)
			set_focus(s, get_focus_other(win));

		if (ws_maxstack(win->ws) && changed & EWMH_F_MAXIMIZED)
			win->maxstackmax = MAXIMIZED(win);

		ewmh_update_wm_state(win);
		update_win_layer_related(win);
		refresh_stack(s);
		update_stacking(s);
		if (refresh_strut(s))
			update_layout(s);
		else
			stack(win->ws->r);
		update_mapping(s);

		r = get_current_region(s);
		if (r != win->ws->r)
			return;
	} else if (e->type == ewmh[_NET_WM_DESKTOP].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_WM_DESKTOP, new_desktop: %d, "
		    "source_type: %s(%d)\n", e->data.data32[0],
		    get_source_type_label(e->data.data32[1]),
		    e->data.data32[1]);

		if ((int)e->data.data32[0] >= workspace_limit)
			return;

		r = win->ws->r;
		win_to_ws(win, get_workspace(s, e->data.data32[0]),
		    SWM_WIN_UNFOCUS);

		follow = follow_pointer(s, SWM_FOCUS_TYPE_MOVE);

		/* Stack source and destination ws, if mapped. */
		if (r != win->ws->r) {
			if (refresh_strut(s))
				update_layout(s);
			else if (r) {
				stack(r);
				bar_draw(r->bar);
			}

			if (win->ws->r) {
				if (win_floating(win))
					load_float_geom(win);

				stack(win->ws->r);
				bar_draw(win->ws->r->bar);
			}
			update_mapping(s);
		}

		if (r == NULL && win->ws->r == NULL)
			return;
	} else if (e->type == a_change_state) {
		DNPRINTF(SWM_D_EVENT, "WM_CHANGE_STATE state: %s\n",
		    get_wm_state_label(e->data.data32[0]));
		if (e->data.data32[0] != XCB_ICCCM_WM_STATE_ICONIC)
			return;
		/* Iconify. */
		follow = follow_mode(SWM_FOCUS_TYPE_ICONIFY);
		ewmh_apply_flags(win, win->ewmh_flags | EWMH_F_HIDDEN);
		ewmh_update_wm_state(win);
		update_stacking(s);
		if (s->focus == win && !follow)
			set_focus(s, get_focus_other(win));
		if (refresh_strut(s))
			update_layout(s);
		else
			stack(win->ws->r);
		update_mapping(s);

		r = get_current_region(s);
		if (r != win->ws->r)
			return;
	} else if (e->type == ewmh[_NET_WM_MOVERESIZE].atom) {
		DNPRINTF(SWM_D_EVENT, "_NET_WM_MOVERESIZE\n");
		moveresize_win(win, e);
		return;
	}

	if (!follow) {
		update_focus(s);
		if (cfw != s->focus)
			center_pointer(s->r_focus);
	}

	flush(); /* win can be freed. */
	if (follow)
		focus_follow(s, s->r_focus, win);

	DNPRINTF(SWM_D_EVENT, "done\n");
}

static const char *
get_moveresize_direction_label(uint32_t type)
{
	switch (type) {
#define RETSTR(c,l) case c: return l
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_TOPLEFT,		"SIZE_TOPLEFT");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_TOP,		"SIZE_TOP");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_TOPRIGHT,	"SIZE_TOPRIGHT");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_RIGHT,		"SIZE_RIGHT");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT,	"SIZE_BOTTOMRIGHT");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_BOTTOM,		"SIZE_BOTTOM");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_BOTTOMLEFT,	"SIZE_BOTTOMLEFT");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_LEFT,		"SIZE_LEFT");
	RETSTR(EWMH_WM_MOVERESIZE_MOVE,			"MOVE");
	RETSTR(EWMH_WM_MOVERESIZE_SIZE_KEYBOARD,	"SIZE_KEYBOARD");
	RETSTR(EWMH_WM_MOVERESIZE_MOVE_KEYBOARD,	"MOVE_KEYBOARD");
	RETSTR(EWMH_WM_MOVERESIZE_CANCEL,		"CANCEL");
#undef RETSTR
	}

	return "Invalid";
}

static void
moveresize_win(struct ws_win *win, xcb_client_message_event_t *e)
{
	struct binding		b;
	uint32_t		dir = 0;
	bool			move;

	/*
	 * _NET_WM_MOVERESIZE
	 * window = window to be moved or resized
	 * message_type = _NET_WM_MOVERESIZE
	 * format = 32
	 * data.l[0] = x_root
	 * data.l[1] = y_root
	 * data.l[2] = direction
	 * data.l[3] = button
	 * data.l[4] = source indication
	 */
	DNPRINTF(SWM_D_EVENT, "win %#x, event win: %#x, x_root: %d, y_root: %d,"
	    " direction: %s (%u), button: %#x, source type: %s\n",
	    win->id, e->window, e->data.data32[0], e->data.data32[1],
	    get_moveresize_direction_label(e->data.data32[2]),
	    e->data.data32[2], e->data.data32[3],
	    get_source_type_label(e->data.data32[4]));

	move = false;
	switch (e->data.data32[2]) {
	case EWMH_WM_MOVERESIZE_SIZE_TOPLEFT:
		dir = SWM_SIZE_TOPLEFT;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_TOP:
		dir = SWM_SIZE_TOP;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_TOPRIGHT:
		dir = SWM_SIZE_TOPRIGHT;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_RIGHT:
		dir = SWM_SIZE_RIGHT;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
		dir = SWM_SIZE_BOTTOMRIGHT;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_BOTTOM:
		dir = SWM_SIZE_BOTTOM;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
		dir = SWM_SIZE_BOTTOMLEFT;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_LEFT:
		dir = SWM_SIZE_LEFT;
		break;
	case EWMH_WM_MOVERESIZE_MOVE:
		move = true;
		break;
	case EWMH_WM_MOVERESIZE_SIZE_KEYBOARD:
	case EWMH_WM_MOVERESIZE_MOVE_KEYBOARD:
		DNPRINTF(SWM_D_EVENT, "ignore; keyboard move/resize.\n");
		return;
	case EWMH_WM_MOVERESIZE_CANCEL:
		DNPRINTF(SWM_D_EVENT, "ignore; cancel.\n");
		return;
	default:
		DNPRINTF(SWM_D_EVENT, "invalid direction.\n");
		return;
	};

	b.type = BTNBIND;
	b.value = e->data.data32[3];

	if (move)
		move_win_pointer(win, &b, e->data.data32[0], e->data.data32[1]);
	else
		resize_win_pointer(win, &b, e->data.data32[0],
		    e->data.data32[1], dir, false);
	DNPRINTF(SWM_D_EVENT, "done.\n");
}

static int
enable_wm(void)
{
	int			num_screens, i;
	const uint32_t		val =
	    XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_LEAVE_WINDOW |
	    XCB_EVENT_MASK_POINTER_MOTION |
	    XCB_EVENT_MASK_POINTER_MOTION_HINT |
	    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	    XCB_EVENT_MASK_FOCUS_CHANGE |
	    XCB_EVENT_MASK_PROPERTY_CHANGE;
	xcb_screen_t		*sc;
	xcb_void_cookie_t	ck;
	xcb_generic_error_t	*error;

	/* this causes an error if some other window manager is running */
	num_screens = get_screen_count();
	for (i = 0; i < num_screens; i++) {
		if ((sc = get_screen(i)) == NULL)
			errx(1, "ERROR: unable to get screen %d.", i);
		DNPRINTF(SWM_D_INIT, "screen %d, root: %#x\n", i, sc->root);
		ck = xcb_change_window_attributes_checked(conn, sc->root,
		    XCB_CW_EVENT_MASK, &val);
		if ((error = xcb_request_check(conn, ck))) {
			DNPRINTF(SWM_D_INIT, "error_code: %u\n",
			    error->error_code);
			free(error);
			return (1);
		}
	}

	return (0);
}

static const char *
get_randr_rotation_label(int rot)
{
	const char *label;

	switch (rot) {
	case XCB_RANDR_ROTATION_ROTATE_0:
		label = "normal";
		break;
	case XCB_RANDR_ROTATION_ROTATE_90:
		label = "left";
		break;
	case XCB_RANDR_ROTATION_ROTATE_180:
		label = "inverted";
		break;
	case XCB_RANDR_ROTATION_ROTATE_270:
		label = "right";
		break;
	default:
		label = "invalid";
	}

	return (label);
}

static void
new_region(struct swm_screen *s, int16_t x, int16_t y, uint16_t w, uint16_t h,
    uint16_t rot)
{
	struct swm_region	*r = NULL, *n;
	struct workspace	*ws = NULL;
	int			i;
	uint32_t		wa[1];

	DNPRINTF(SWM_D_MISC, "screen[%d]:%dx%d+%d+%d,%s\n", s->idx, w, h, x, y,
	    get_randr_rotation_label(rot));

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
			free_stackable(r->st);
			r->st = NULL;
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
		if (X(r) == x && Y(r) == y && HEIGHT(r) == h && WIDTH(r) == w)
			break;

	/* size match (same axis) */
	if (r == NULL)
		TAILQ_FOREACH(r, &s->orl, entry)
			if ((r->g.r & ROTATION_VERT) == (rot & ROTATION_VERT) &&
			    HEIGHT(r) == h && WIDTH(r) == w)
				break;

	/* size match (different axis) */
	if (r == NULL)
		TAILQ_FOREACH(r, &s->orl, entry)
			if ((r->g.r & ROTATION_VERT) != (rot & ROTATION_VERT) &&
			    HEIGHT(r) == w && WIDTH(r) == h)
				break;

	if (r != NULL) {
		TAILQ_REMOVE(&s->orl, r, entry);
		/* try to use old region's workspace */
		if (r->ws->r == NULL)
			ws = r->ws;
	} else if ((r = calloc(1, sizeof(struct swm_region))) == NULL)
		err(1, "new_region: r calloc");

	if ((r->st = calloc(1, sizeof(struct swm_stackable))) == NULL)
		err(1, "new_region: st calloc");

	/* if we don't have a workspace already, find one */
	if (ws == NULL) {
		for (i = 0; i < workspace_limit; i++) {
			ws = get_workspace(s, i);
			if (ws && ws->r == NULL)
				break;
		}
	}

	if (ws == NULL)
		errx(1, "new_region: no free workspaces");

	r->st->type = STACKABLE_REGION;
	r->st->region = r;
	r->st->layer = SWM_LAYER_REGION;
	r->st->s = s;
	X(r) = x;
	Y(r) = y;
	WIDTH(r) = w;
	HEIGHT(r) = h;
	ROTATION(r) = rot;
	r->g_usable = r->g;
	r->bar = NULL;
	r->s = s;
	r->ws = ws;
	r->ws_prior = NULL;
	ws->r = r;
	outputs++;
	TAILQ_INSERT_TAIL(&s->rl, r, entry);

	if (workspace_autorotate)
		rotatews(ws, rot);

	/* Invisible region window to detect pointer events on empty regions. */
	r->id = xcb_generate_id(conn);
	wa[0] = XCB_EVENT_MASK_BUTTON_PRESS |
	    XCB_EVENT_MASK_BUTTON_RELEASE |
	    XCB_EVENT_MASK_ENTER_WINDOW |
	    XCB_EVENT_MASK_LEAVE_WINDOW |
	    XCB_EVENT_MASK_POINTER_MOTION |
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

static void
scan_randr(struct swm_screen *s)
{
#ifdef XCB_RANDR_GET_SCREEN_RESOURCES_CURRENT /* RandR 1.3 */
	int						i, j;
	int						ncrtc = 0, nmodes = 0;
	xcb_randr_get_screen_resources_current_reply_t	*srr;
	xcb_randr_get_crtc_info_reply_t			*cir = NULL;
	xcb_randr_crtc_t				*crtc;
	xcb_randr_mode_info_t				*mode;
	int						minrate, currate;
#endif
	struct swm_region				*r;
	struct workspace				*ws;
	xcb_screen_t					*screen;

	if (s == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "screen: %d\n", s->idx);

	if ((screen = get_screen(s->idx)) == NULL)
		errx(1, "ERROR: unable to get screen %d.", s->idx);

	/* Cleanup references. */
	clear_stack(s);

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&s->rl)) != NULL) {
		r->ws->old_r = r->ws->r = NULL;
		free_stackable(r->st);
		r->st = NULL;
		bar_cleanup(r);
		xcb_destroy_window(conn, r->id);
		r->id = XCB_WINDOW_NONE;
		TAILQ_REMOVE(&s->rl, r, entry);
		TAILQ_INSERT_TAIL(&s->orl, r, entry);
	}
	outputs = 0;

	/* Update root region geometry. */
	s->r->g.w = screen->width_in_pixels;
	s->r->g.h = screen->height_in_pixels;

#ifdef XCB_RANDR_GET_SCREEN_RESOURCES_CURRENT
	/* Try to automatically detect regions based on RandR CRTC info. */
	if (randr_scan) {
		srr = xcb_randr_get_screen_resources_current_reply(conn,
		    xcb_randr_get_screen_resources_current(conn, s->root),
		    NULL);
		if (srr == NULL) {
			new_region(s, 0, 0, screen->width_in_pixels,
			    screen->height_in_pixels, ROTATION_DEFAULT);
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
			cir = xcb_randr_get_crtc_info_reply(conn,
			    xcb_randr_get_crtc_info(conn, crtc[i],
			    XCB_CURRENT_TIME), NULL);
			if (cir == NULL)
				continue;
			if (cir->num_outputs == 0) {
				free(cir);
				continue;
			}

			if (cir->mode == 0) {
				new_region(s, 0, 0, screen->width_in_pixels,
				    screen->height_in_pixels, ROTATION_DEFAULT);
			} else {
				new_region(s, cir->x, cir->y, cir->width,
				    cir->height, cir->rotation & 0xf);

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

		s->rate = (minrate > 0) ? minrate : SWM_RATE_DEFAULT;
		DNPRINTF(SWM_D_MISC, "Screen %d update rate: %d\n", s->idx,
		    s->rate);
	}
#endif /* XCB_RANDR_GET_SCREEN_RESOURCES_CURRENT */

	/* If detection failed, create a single region that spans the screen. */
	if (TAILQ_EMPTY(&s->rl))
		new_region(s, 0, 0, screen->width_in_pixels,
		    screen->height_in_pixels, ROTATION_DEFAULT);

#ifdef XCB_RANDR_GET_SCREEN_RESOURCES_CURRENT
out:
#endif
	/* Cleanup references to unused regions. */
	TAILQ_FOREACH(r, &s->orl, entry) {
		if (s->r_focus == r)
			s->r_focus = NULL;
		for (i = 0; i < workspace_limit; i++) {
			ws = workspace_lookup(s, i);
			if (ws && ws->old_r == r)
				ws->old_r = NULL;
		}
	}

	DNPRINTF(SWM_D_MISC, "done.\n");
}

static void
screenchange(xcb_randr_screen_change_notify_event_t *e)
{
	struct swm_screen		*s;
	struct swm_region		*r;
	struct workspace		*ws;

	DNPRINTF(SWM_D_EVENT, "root: %#x\n", e->root);

	/* silly event doesn't include the screen index */
	s = find_screen(e->root);
	if (s == NULL)
		errx(1, "screenchange: screen not found.");

	/* brute force for now, just re-enumerate the regions */
	scan_randr(s);

	if (swm_debug & SWM_D_EVENT)
		print_win_geom(e->root);

	/* add bars to all regions */
	TAILQ_FOREACH(r, &s->rl, entry)
		bar_setup(r);

	/* Stack all regions. */
	refresh_stack(s);
	update_stacking(s);
	refresh_strut(s);
	update_layout(s);
	update_mapping(s);

	/* Focus region. */
	if (s->r_focus) {
		/* Update bar colors for existing focus. */
		r = s->r_focus;
		s->r_focus = NULL;
		set_region(r);
	} else {
		/* Focus on the first region. */
		r = TAILQ_FIRST(&s->rl);
		if (r)
			focus_region(r);
	}

	/* Unmap workspaces that are no longer visible. */
	RB_FOREACH(ws, workspace_tree, &s->workspaces)
		if (ws->r == NULL)
			unmap_workspace(ws);

	ewmh_update_number_of_desktops(s);
	update_bars(s);

	flush();
	DNPRINTF(SWM_D_EVENT, "done.\n");
}

static void
grab_windows(void)
{
	struct workspace		*ws;
	struct ws_win_list		*wl;
	struct ws_win			*w;
	xcb_query_tree_cookie_t		qtc;
	xcb_query_tree_reply_t		*qtr;
	xcb_get_property_reply_t	*pr;
	xcb_window_t			*wins = NULL, *cwins = NULL;
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

		/* Manage windows from bottom to top. */
		for (j = 0; j < no; j++)
			manage_window(wins[j], SWM_STACK_TOP, false);

		free(qtr);

		/* Set initial focus state based on manage order and EWMH. */
		DNPRINTF(SWM_D_INIT, "set initial focus state\n");
		RB_FOREACH(ws, workspace_tree, &screens[i].workspaces) {
			wl = &ws->winlist;

			/* Prepare previous focus queue. */
			TAILQ_FOREACH(w, wl, entry)
				set_focus(w->s, w);

			/* 1. Fullscreen */
			TAILQ_FOREACH_REVERSE(w, wl, ws_win_list, entry)
				if (!HIDDEN(w) && FULLSCREEN(w))
					break;
			/* 2. Maximized */
			if (w == NULL)
				TAILQ_FOREACH_REVERSE(w, wl, ws_win_list, entry)
					if (!HIDDEN(w) && MAXIMIZED(w))
						break;
			/* 3. Default */
			if (w == NULL)
				w = get_main_window(ws);

			if (w == NULL)
				continue;

			set_focus(w->s, w);

			DNPRINTF(SWM_D_INIT, "ws %d: focus: %#x\n",
			    j, WINID(ws->focus));
		}
	}
	DNPRINTF(SWM_D_INIT, "done\n");
}

static void
setup_focus(void)
{
	struct swm_region	*r;
	struct ws_win		*w;
	bool			follow;

	follow = follow_mode(SWM_FOCUS_TYPE_STARTUP);
	if (follow) {
		DNPRINTF(SWM_D_INIT, "focus pointer window\n");
		r = get_pointer_region(&screens[0]);
	} else {
		DNPRINTF(SWM_D_INIT, "focus first region\n");
		r = TAILQ_FIRST(&screens[0].rl);
	}

	if (r) {
		if (!follow || ws_maponfocus(r->ws)) {
			w = r->ws->focus;
			if (w == NULL || !(FULLSCREEN(w) || MAXIMIZED(w)))
				w = get_main_window(r->ws);
			if (w) {
				set_focus(w->s, w);
				if (apply_unfocus(w->ws, w) > 0) {
					update_stacking(&screens[0]);
					refresh_strut(&screens[0]);
					update_layout(&screens[0]);
					update_mapping(&screens[0]);
				}
			}
		}
		focus_region(r);
		flush();
		if (follow && pointer_window != XCB_WINDOW_NONE) {
			focus_window(pointer_window);
			xcb_flush(conn);
		}
	}
	DNPRINTF(SWM_D_INIT, "done\n");
}

static void
layout_order_reset(void)
{
	int		i;

	for (i = 0; i < LENGTH(layouts); i++)
		layout_order[i] = &layouts[i];
	layout_order_count = i;
}

static void
setup_screens(void)
{
	struct swm_screen		*s;
	xcb_pixmap_t			pmap;
	xcb_depth_iterator_t		diter;
	xcb_visualtype_iterator_t	viter;
	xcb_void_cookie_t		cmc;
	xcb_generic_error_t		*error;
	XVisualInfo			vtmpl, *vlist;
	int				i, num_screens, vcount;
	uint32_t			wa[3];
	xcb_screen_t			*screen;

	num_screens = get_screen_count();
	if ((screens = calloc(num_screens, sizeof(struct swm_screen))) == NULL)
		err(1, "setup_screens: calloc");

	/* map physical screens */
	for (i = 0; i < num_screens; i++) {
		DNPRINTF(SWM_D_INIT, "init screen: %d\n", i);
		s = &screens[i];
		s->idx = i;

		if ((screen = get_screen(i)) == NULL)
			errx(1, "ERROR: unable to get screen %d.", i);

		/* Create region for root. */
		if ((s->r = calloc(1, sizeof(struct swm_region))) == NULL)
			err(1, "setup_screens: calloc");

		TAILQ_INIT(&s->rl);
		TAILQ_INIT(&s->orl);
		RB_INIT(&s->workspaces);
		TAILQ_INIT(&s->priority);
		SLIST_INIT(&s->stack);
		SLIST_INIT(&s->struts);
		TAILQ_INIT(&s->fl);
		TAILQ_INIT(&s->managed);

		s->r->id = XCB_WINDOW_NONE; /* Not needed for root region. */
		X(s->r) = 0;
		Y(s->r) = 0;
		WIDTH(s->r) = screen->width_in_pixels;
		HEIGHT(s->r) = screen->height_in_pixels;
		s->r->g_usable = s->r->g;
		s->r->s = s;
		s->r->ws = get_workspace(s, -1);
		s->r->ws->r = s->r;
		s->r->ws->cur_layout = &layouts[SWM_FLOATING_STACK];
		s->focus = NULL;
		s->r_focus = NULL;
		s->rate = SWM_RATE_DEFAULT;
		s->root = screen->root;
		s->active_window = screen->root;
		s->depth = screen->root_depth;
		s->visual = screen->root_visual;
		s->colormap = screen->default_colormap;
		s->xvisual = DefaultVisual(display, i);
		s->bar_xftfonts = NULL;
		s->managed_count = 0;

		DNPRINTF(SWM_D_INIT, "root_depth: %d, screen_depth: %d\n",
		    screen->root_depth, xcb_aux_get_depth(conn, screen));

		/* Use the maximum supported screen depth. */
		for (diter = xcb_screen_allowed_depths_iterator(screen);
		    diter.rem; xcb_depth_next(&diter)) {
			if (diter.data->depth > s->depth) {
				viter = xcb_depth_visuals_iterator(diter.data);
				if (viter.rem && viter.data->_class ==
				    XCB_VISUAL_CLASS_TRUE_COLOR) {
					s->depth = diter.data->depth;
					s->visual = viter.data->visual_id;
					DNPRINTF(SWM_D_INIT, "Found TrueColor "
					    "visual %#x, depth: %d.\n",
					    s->visual, s->depth);
				}
			}
		}

		/* Create a new colormap if not using the root visual */
		if (s->visual != screen->root_visual) {
			/* Find the corresponding Xlib visual struct for Xft. */
			vtmpl.visualid = s->visual;
			vlist = XGetVisualInfo(display, VisualIDMask, &vtmpl,
			    &vcount);
			if (vlist) {
				if (vcount > 0)
					s->xvisual = vlist[0].visual;
				XFree(vlist);
			}

			DNPRINTF(SWM_D_INIT, "Creating new colormap.\n");
			s->colormap = xcb_generate_id(conn);
			cmc = xcb_create_colormap_checked(conn,
			    XCB_COLORMAP_ALLOC_NONE, s->colormap, s->root,
			    s->visual);
			if ((error = xcb_request_check(conn, cmc))) {
				DNPRINTF(SWM_D_MISC, "error:\n");
				event_error(error);
				free(error);
			}
		}

		DNPRINTF(SWM_D_INIT, "Using visual %#x, depth: %d.\n",
		    s->visual, xcb_aux_get_depth_of_visual(screen, s->visual));

		/* Set default colors. */
		setscreencolor(s, "red", SWM_S_COLOR_FOCUS, 0);
		setscreencolor(s, "rgb:88/88/88", SWM_S_COLOR_UNFOCUS, 0);
		setscreencolor(s, "yellow", SWM_S_COLOR_FOCUS_FREE, 0);
		setscreencolor(s, "rgb:88/88/00", SWM_S_COLOR_UNFOCUS_FREE, 0);
		setscreencolor(s, "rgb:00/80/80", SWM_S_COLOR_BAR_BORDER, 0);
		setscreencolor(s, "rgb:80/80/00",
		    SWM_S_COLOR_BAR_BORDER_FREE, 0);
		setscreencolor(s, "rgb:00/40/40",
		    SWM_S_COLOR_BAR_BORDER_UNFOCUS, 0);
		setscreencolor(s, "black", SWM_S_COLOR_BAR, 0);
		setscreencolor(s, "rgb:40/40/00", SWM_S_COLOR_BAR_FREE, 0);
		setscreencolor(s, "rgb:a0/a0/a0", SWM_S_COLOR_BAR_FONT, 0);
		setscreencolor(s, "rgb:ff/ff/ff", SWM_S_COLOR_BAR_FONT_FREE, 0);

		/* set default cursor */
		wa[0] = cursors[XC_LEFT_PTR].cid;
		xcb_change_window_attributes(conn, s->root, XCB_CW_CURSOR, wa);

		/* Create graphics context. */

		/* xcb_create_gc determines root/depth from a drawable. */
		pmap = xcb_generate_id(conn);
		xcb_create_pixmap(conn, s->depth, pmap, s->root, 10, 10);

		s->gc = xcb_generate_id(conn);
		wa[0] = wa[1] = screen->black_pixel;
		wa[2] = 0;
		xcb_create_gc(conn, s->gc, pmap, XCB_GC_FOREGROUND |
		    XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES, wa);
		xcb_free_pixmap(conn, pmap);

		/* Create window for input fallback as well as EWMH support. */
		s->swmwin = xcb_generate_id(conn);
		wa[0] = 1;
		wa[1] = XCB_EVENT_MASK_PROPERTY_CHANGE;
		xcb_create_window(conn, XCB_COPY_FROM_PARENT, s->swmwin,
		    s->root, -1, -1, 1, 1, 0,
		    XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT,
		    XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK, wa);
		/* Keep any existing override-redirect windows on top. */
		wa[0] = XCB_STACK_MODE_BELOW;
		xcb_configure_window(conn, s->swmwin,
		    XCB_CONFIG_WINDOW_STACK_MODE, wa);
		xcb_map_window(conn, s->swmwin);

#if defined(SWM_XCB_HAS_XINPUT) && defined(XCB_INPUT_RAW_BUTTON_PRESS)
		setup_xinput2(s);
#endif

		scan_randr(s);
		if (randr_support)
			xcb_randr_select_input(conn, s->root,
			    XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	}
}

static void
setup_extensions(void)
{
	const xcb_query_extension_reply_t	*qep;
	xcb_randr_query_version_reply_t		*rqvr;
#ifdef SWM_XCB_HAS_XINPUT
	xcb_input_xi_query_version_reply_t	*xiqvr;
#endif

	randr_support = false;
	randr_scan = false;
	qep = xcb_get_extension_data(conn, &xcb_randr_id);
	if (qep->present) {
		DNPRINTF(SWM_D_INIT, "XCB RandR version: %u.%u.\n",
		    XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
		rqvr = xcb_randr_query_version_reply(conn,
		    xcb_randr_query_version(conn, XCB_RANDR_MAJOR_VERSION,
		    XCB_RANDR_MINOR_VERSION), NULL);
		if (rqvr) {
			DNPRINTF(SWM_D_INIT, "X server RandR version: %u.%u, "
			    "major_opcode: %u, first_event: %u\n",
			    rqvr->major_version, rqvr->minor_version,
			    qep->major_opcode, qep->first_event);
			if (rqvr->major_version >= 1) {
				randr_support = true;
				randr_eventbase = qep->first_event;

				if (rqvr->minor_version >= 3 ||
				    rqvr->major_version > 1)
					randr_scan = true;
			}
			free(rqvr);
		}
	}
	DNPRINTF(SWM_D_INIT, "randr_support: %s, randr_scan: %s\n",
	    YESNO(randr_support), YESNO(randr_scan));

#ifdef SWM_XCB_HAS_XINPUT
	xinput2_support = false;
	xinput2_raw = false;
	qep = xcb_get_extension_data(conn, &xcb_input_id);
	if (qep->present) {
		DNPRINTF(SWM_D_INIT, "XCB XInput version: %u.%u.\n",
		    XCB_INPUT_MAJOR_VERSION, XCB_INPUT_MINOR_VERSION);
		xiqvr = xcb_input_xi_query_version_reply(conn,
		    xcb_input_xi_query_version(conn, XCB_INPUT_MAJOR_VERSION,
		    XCB_INPUT_MINOR_VERSION), NULL);
		if (xiqvr) {
			DNPRINTF(SWM_D_INIT, "X server XInput version: %u.%u, "
			    "major_opcode: %u\n", xiqvr->major_version,
			    xiqvr->minor_version, qep->major_opcode);
			if (xiqvr->major_version >= 2) {
				xinput2_support = true;
				xinput2_opcode = qep->major_opcode;
#ifdef XCB_INPUT_RAW_BUTTON_PRESS
				if (xiqvr->minor_version >= 1 ||
				    xiqvr->major_version > 2)
					xinput2_raw = true;
#endif
			}
			free(xiqvr);
		}
	}
#endif /* SWM_XCB_HAS_XINPUT */
	DNPRINTF(SWM_D_INIT, "xinput2_support: %s, xinput2_raw: %s\n",
	    YESNO(xinput2_support), YESNO(xinput2_raw));
}

static void
setup_globals(void)
{
	if ((clock_format = strdup("%a %b %d %R %Z %Y")) == NULL)
		err(1, "clock_format: strdup");

	if ((focus_mark_none = strdup("")) == NULL)
		err(1, "focus_mark_none: strdup");

	if ((focus_mark_normal = strdup("")) == NULL)
		err(1, "focus_mark_normal: strdup");

	if ((focus_mark_floating = strdup("(f)")) == NULL)
		err(1, "focus_mark_floating: strdup");

	if ((focus_mark_free = strdup("(*)")) == NULL)
		err(1, "focus_mark_free: strdup");

	if ((focus_mark_maximized = strdup("(m)")) == NULL)
		err(1, "focus_mark_maximized: strdup");

	if ((stack_mark_floating = strdup("[~]")) == NULL)
		err(1, "stack_mark_floating: strdup");

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
	a_change_state = get_atom_from_string("WM_CHANGE_STATE");
	a_prot = get_atom_from_string("WM_PROTOCOLS");
	a_delete = get_atom_from_string("WM_DELETE_WINDOW");
	a_net_frame_extents = get_atom_from_string("_NET_FRAME_EXTENTS");
	a_net_supported = get_atom_from_string("_NET_SUPPORTED");
	a_net_wm_check = get_atom_from_string("_NET_SUPPORTING_WM_CHECK");
	a_net_wm_pid = get_atom_from_string("_NET_WM_PID");
	a_takefocus = get_atom_from_string("WM_TAKE_FOCUS");
	a_utf8_string = get_atom_from_string("UTF8_STRING");
	a_swm_pid = get_atom_from_string("_SWM_PID");
	a_swm_ws = get_atom_from_string("_SWM_WS");

	layout_order_reset();
}

static void
scan_config(void)
{
	struct stat		sb;
	struct passwd		*pwd;
	char			conf[PATH_MAX];
	char			*cfile = NULL, *str = NULL, *ret, *s, *sp;
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
			sp = str;
			while ((s = strsep(&sp, ":")) != NULL) {
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

static void
shutdown_cleanup(void)
{
	struct swm_screen	*s;
	struct swm_region	*r;
	struct ws_win		*w;
	struct workspace	*ws;
	int			i, j, num_screens;

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		err(1, "can't disable alarm");

	bar_extra_stop();

	cursors_cleanup();

	clear_quirks();
	clear_spawns();
	clear_bindings();
	clear_atom_names();

	teardown_ewmh();

	num_screens = get_screen_count();
	for (i = 0; i < num_screens; ++i) {
		s = &screens[i];

		set_input_focus(XCB_INPUT_FOCUS_POINTER_ROOT, true);

		xcb_destroy_window(conn, s->swmwin);

		if (s->gc != XCB_NONE)
			xcb_free_gc(conn, s->gc);

		for (j = 0; j < LENGTH(s->c); j++)
			freecolortype(s, j);

		if (!bar_font_legacy) {
			for (j = 0; j < num_xftfonts; j++)
				if (s->bar_xftfonts[j])
					XftFontClose(display,
					    s->bar_xftfonts[j]);

			if (font_pua_index)
				XftFontClose(display,
				    s->bar_xftfonts[font_pua_index]);

			free(s->bar_xftfonts);
		}

		clear_stack(s);

#ifndef __clang_analyzer__ /* Suppress false warnings. */
		/* Cleanup window state and memory. */
		while ((w = TAILQ_FIRST(&s->managed))) {
			unmap_window(w);
			/* Load floating geometry and adjust to root. */
			w->g = w->g_float;
			w->g.x -= w->g_floatref.x;
			w->g.y -= w->g_floatref.y;
			update_window(w);

			if (ws_maxstack(w->ws)) {
				w->ewmh_flags &= ~EWMH_F_MAXIMIZED;
				ewmh_update_wm_state(w);
			}

			if (w->strut) {
				SLIST_REMOVE(&w->s->struts, w->strut, swm_strut,
				    entry);
				free(w->strut);
				w->strut = NULL;
			}

			TAILQ_REMOVE(&s->managed, w, manage_entry);
			free(w->st);
			free_window(w);
		}

		while ((ws = RB_ROOT(&s->workspaces)))
			workspace_remove(ws);
#endif
		/* Free region memory. */
		while ((r = TAILQ_FIRST(&s->rl)) != NULL) {
			TAILQ_REMOVE(&s->rl, r, entry);
			if (r->bar) {
				free(r->bar->st);
				free(r->bar);
			}
			free(r->st);
			free(r);
		}

		while ((r = TAILQ_FIRST(&s->orl)) != NULL) {
			TAILQ_REMOVE(&s->orl, r, entry);
			free(r);
		}

		free(s->r);
	}
	free(screens);

	if (bar_fontnames) {
		for (i = 0; i < num_xftfonts; i++)
			free(bar_fontnames[i]);
		free(bar_fontnames);
	}

	free(bsect);
	free(bar_fontname_pua);
	free(bar_format);
	free(bar_fonts);
	free(clock_format);
	free(startup_exception);
	free(focus_mark_none);
	free(focus_mark_normal);
	free(focus_mark_floating);
	free(focus_mark_free);
	free(focus_mark_maximized);
	free(stack_mark_floating);
	free(stack_mark_max);
	free(stack_mark_vertical);
	free(stack_mark_vertical_flip);
	free(stack_mark_horizontal);
	free(stack_mark_horizontal_flip);
	free(workspace_mark_current);
	free(workspace_mark_current_suffix);
	free(workspace_mark_urgent);
	free(workspace_mark_urgent_suffix);
	free(workspace_mark_active);
	free(workspace_mark_active_suffix);
	free(workspace_mark_empty);
	free(workspace_mark_empty_suffix);

	if (bar_fs)
		XFreeFontSet(display, bar_fs);

	free(bar_argv[0]);

	xcb_key_symbols_free(syms);
	xcb_flush(conn);
	xcb_aux_sync(conn);
	XCloseDisplay(display);
}

static void
event_error(xcb_generic_error_t *e)
{
	(void)e;

	DNPRINTF(SWM_D_EVENT, "%s(%u) from %s(%u), sequence: %u, "
	    "resource_id: %u, minor_code: %u\n",
	    xcb_event_get_error_label(e->error_code), e->error_code,
	    xcb_event_get_request_label(e->major_code), e->major_code,
	    e->sequence, e->resource_id, e->minor_code);
}

static void
event_handle(xcb_generic_event_t *evt)
{
	uint8_t			type = XCB_EVENT_RESPONSE_TYPE(evt);

	DNPRINTF(SWM_D_EVENT, "%s(%d), seq %u, sent: %s\n", get_event_label(evt),
	    type, evt->sequence, YESNO(XCB_EVENT_SENT(evt)));

	if (type <= XCB_MAPPING_NOTIFY) {
		switch (type) {
#define EVENT(type, callback) case type: callback((void *)evt); return
		EVENT(0, event_error);
		EVENT(XCB_KEY_PRESS, keypress);
		EVENT(XCB_KEY_RELEASE, keyrelease);
		EVENT(XCB_BUTTON_PRESS, buttonpress);
		EVENT(XCB_BUTTON_RELEASE, buttonrelease);
		EVENT(XCB_MOTION_NOTIFY, motionnotify);
		EVENT(XCB_ENTER_NOTIFY, enternotify);
		EVENT(XCB_LEAVE_NOTIFY, leavenotify);
		EVENT(XCB_FOCUS_IN, focusin);
		EVENT(XCB_FOCUS_OUT, focusout);
		/*EVENT(XCB_KEYMAP_NOTIFY, );*/
		EVENT(XCB_EXPOSE, expose);
		/*EVENT(XCB_GRAPHICS_EXPOSURE, );*/
		/*EVENT(XCB_NO_EXPOSURE, );*/
		/*EVENT(XCB_VISIBILITY_NOTIFY, );*/
		/*EVENT(XCB_CREATE_NOTIFY, );*/
		EVENT(XCB_DESTROY_NOTIFY, destroynotify);
		EVENT(XCB_UNMAP_NOTIFY, unmapnotify);
		EVENT(XCB_MAP_NOTIFY, mapnotify);
		EVENT(XCB_MAP_REQUEST, maprequest);
		EVENT(XCB_REPARENT_NOTIFY, reparentnotify);
		EVENT(XCB_CONFIGURE_NOTIFY, configurenotify);
		EVENT(XCB_CONFIGURE_REQUEST, configurerequest);
		/*EVENT(XCB_GRAVITY_NOTIFY, );*/
		/*EVENT(XCB_RESIZE_REQUEST, );*/
		/*EVENT(XCB_CIRCULATE_NOTIFY, );*/
		/*EVENT(XCB_CIRCULATE_REQUEST, );*/
		EVENT(XCB_PROPERTY_NOTIFY, propertynotify);
		/*EVENT(XCB_SELECTION_CLEAR, );*/
		/*EVENT(XCB_SELECTION_REQUEST, );*/
		/*EVENT(XCB_SELECTION_NOTIFY, );*/
		/*EVENT(XCB_COLORMAP_NOTIFY, );*/
		EVENT(XCB_CLIENT_MESSAGE, clientmessage);
		EVENT(XCB_MAPPING_NOTIFY, mappingnotify);
#undef EVENT
		}
#if defined(SWM_XCB_HAS_XINPUT) && defined(XCB_INPUT_RAW_BUTTON_PRESS)
	} else if (type == XCB_GE_GENERIC) {
		xcb_ge_generic_event_t *ge;
		ge = (xcb_ge_generic_event_t *)evt;
		if (xinput2_support && ge->extension == xinput2_opcode) {
			if (ge->event_type == XCB_INPUT_RAW_BUTTON_PRESS)
				rawbuttonpress((void *)evt);
		}
#endif
	} else if (randr_support &&
	    (type - randr_eventbase) == XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
		screenchange((void *)evt);
	}
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: spectrwm [-c file] [-v]\n"
	    "        -c FILE        load configuration file\n"
	    "        -d             enable debug mode and logging to stderr\n"
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
	int			ch, i, num_screens, num_readable;
	bool			stdin_ready = false;

	while ((ch = getopt(argc, argv, "c:dhv")) != -1) {
		switch (ch) {
		case 'c':
			cfile = optarg;
			break;
		case 'd':
			swm_debug = SWM_D_ALL;
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

	time_started = time(NULL);

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
		case 0:
			/* Display errors. */
			if (swm_debug & SWM_D_EVENT)
				event_handle(evt);
			break;
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

	setup_marks();
	setup_fonts();
	validate_spawns();

	if (getenv("SWM_STARTED") == NULL)
		setenv("SWM_STARTED", "YES", 1);

	/* Setup bars on all regions. */
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
	for (i = 0; i < num_screens; i++) {
		refresh_stack(&screens[i]);
		update_stacking(&screens[i]);
		refresh_strut(&screens[i]);
		update_layout(&screens[i]);
		update_mapping(&screens[i]);
	}

	xcb_ungrab_server(conn);
	flush();

	setup_focus();

	memset(&pfd, 0, sizeof(pfd));
	pfd[0].fd = xcb_get_file_descriptor(conn);
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

		if (search_resp) {
			search_do_resp();
			continue;
		}

		num_readable = poll(pfd, bar_extra ? 2 : 1, 1000);
		if (num_readable > 0) {
			if (pfd[0].revents & POLLHUP)
				goto done;

			if (bar_extra) {
				if (pfd[1].revents & POLLHUP)
					bar_extra = false;
				else if (pfd[1].revents & POLLIN)
					stdin_ready = true;
			}
		} else if (num_readable == -1) {
			DNPRINTF(SWM_D_MISC, "poll: %s\n", strerror(errno));
		}

		if (restart_wm)
			restart(NULL, NULL, NULL);

		if (!running)
			goto done;

		if (stdin_ready) {
			stdin_ready = false;
			if (bar_extra_update() == 0)
				continue;
		}

		/* Need to ensure the bar(s) are always updated. */
		for (i = 0; i < num_screens; i++)
			update_bars(&screens[i]);

		xcb_flush(conn);
	}
done:
	shutdown_cleanup();

	return (0);
}
