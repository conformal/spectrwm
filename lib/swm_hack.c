/*
 * Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2011-2024 Reginald Kennedy <rk@rejii.com>
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
 * Copyright (C) 2005-2007 Carsten Haitzler
 * Copyright (C) 2006-2007 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Basic hack mechanism (dlopen etc.) taken from e_hack.c in e17.
 */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <xcb/xcb.h>

static xcb_atom_t	swmws = XCB_ATOM_NONE, swmpid = XCB_ATOM_NONE;
static bool		xterm = false;

static xcb_atom_t	get_atom_from_string_xcb(xcb_connection_t *, char *);
static xcb_atom_t	get_atom_from_string_xlib(Display *, char *);
static void		preload_atoms_xcb(xcb_connection_t *);
static void		preload_atoms_xlib(Display *);
static void		prepare_window_xcb(xcb_connection_t *, xcb_window_t);
static void		prepare_window_xlib(Display *, Window);
static void		set_property_xcb(xcb_connection_t *, xcb_window_t,
			    xcb_atom_t, char *);
static void		set_property_xlib(Display *, Window, Atom, char *);
static void		*xcbsym(const char *, char **);
static void		*xlibsym(const char *, char **);
static void		*xtlibsym(const char *, char **);

#define SWM_PROPLEN	(16)

static void *
xcbsym(const char *symbol, char **e)
{
	static void	*lib_xcb = NULL;
	void		*p = NULL;

#ifdef RTLD_NEXT
	p = dlsym(RTLD_NEXT, symbol);
#endif
	if (p == NULL) {
		if (lib_xcb == NULL)
			lib_xcb = dlopen("libxcb.so", RTLD_GLOBAL | RTLD_LAZY);
		if (lib_xcb)
			p = dlsym(lib_xcb, symbol);
		if (p == NULL && e)
			*e = dlerror();
	}

	return (p);
}

static void *
xlibsym(const char *symbol, char **e)
{
	static void	*lib_xlib = NULL;
	void		*p = NULL;

#ifdef RTLD_NEXT
	p = dlsym(RTLD_NEXT, symbol);
#endif
	if (p == NULL) {
		if (lib_xlib == NULL)
			lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
		if (lib_xlib)
			p = dlsym(lib_xlib, symbol);
		if (p == NULL && e)
			*e = dlerror();
	}

	return (p);
}

static void *
xtlibsym(const char *symbol, char **e)
{
	static void	*lib_xtlib = NULL;
	void		*p = NULL;

#ifdef RTLD_NEXT
	p = dlsym(RTLD_NEXT, symbol);
#endif
	if (p == NULL) {
		if (lib_xtlib == NULL)
			lib_xtlib = dlopen("libXt.so", RTLD_GLOBAL | RTLD_LAZY);
		if (lib_xtlib)
			p = dlsym(lib_xtlib, symbol);
		if (p == NULL && e)
			*e = dlerror();
	}

	return (p);
}

/* xcb_intern_atom */
typedef xcb_intern_atom_cookie_t (XIAF)(xcb_connection_t *, uint8_t, uint16_t,
    const char *);

/* xcb_intern_atom_reply */
typedef xcb_intern_atom_reply_t *(XIARF)(xcb_connection_t *,
    xcb_intern_atom_cookie_t, xcb_generic_error_t **);

static xcb_atom_t
get_atom_from_string_xcb(xcb_connection_t *c, char *name)
{
        xcb_atom_t                      atom;
        xcb_intern_atom_cookie_t        iac;
        xcb_intern_atom_reply_t         *iar;
	static XIAF			*xiaf = NULL;
	static XIARF			*xiarf = NULL;
	char				*e;

	if (xiaf == NULL) {
		xiaf = (XIAF *)xcbsym("xcb_intern_atom", &e);
		if (xiaf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}
	if (xiarf == NULL) {
		xiarf = (XIARF *)xcbsym("xcb_intern_atom_reply", &e);
		if (xiarf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	iac = (*xiaf)(c, 0, strlen(name), name);
	iar = (*xiarf)(c, iac, NULL);
        if (iar) {
                atom = iar->atom;
                free(iar);

                return (atom);
        }

        return (XCB_ATOM_NONE);
}

/* XInternAtom */
typedef Atom (IAF)(Display *, char *, Bool);

static xcb_atom_t
get_atom_from_string_xlib(Display *dpy, char *name)
{
	xcb_atom_t		atom = XCB_ATOM_NONE;
	static IAF		*iaf = NULL;
	char			*e;

	if (iaf == NULL) {
		iaf = (IAF *)xlibsym("XInternAtom", &e);
		if (iaf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	atom = (xcb_atom_t)(*iaf)(dpy, name, False);
	return (atom);
}

typedef int (XCHE)(xcb_connection_t *c);

static void
preload_atoms_xcb(xcb_connection_t *c)
{
	static XCHE	*xchef = NULL;
	char		*e;

	if (xchef == NULL) {
		xchef = (XCHE *)xcbsym("xcb_connection_has_error", &e);
		if (xchef == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	if (c == NULL || ((*xchef)(c)))
		return;

	if (swmws == XCB_ATOM_NONE)
		swmws = get_atom_from_string_xcb(c, "_SWM_WS");
	if (swmpid == XCB_ATOM_NONE)
		swmpid = get_atom_from_string_xcb(c, "_SWM_PID");
}

static void
preload_atoms_xlib(Display *dpy)
{
	if (dpy == NULL)
		return;

	if (swmws == XCB_ATOM_NONE)
		swmws = get_atom_from_string_xlib(dpy, "_SWM_WS");
	if (swmpid == XCB_ATOM_NONE)
		swmpid = get_atom_from_string_xlib(dpy, "_SWM_PID");
}

typedef void (XDF)(xcb_connection_t *);
typedef xcb_connection_t *(XCAF)(const char *, xcb_auth_info_t *, int *);
xcb_connection_t *
xcb_connect_to_display_with_auth_info(const char *display,
    xcb_auth_info_t *auth, int *screen)
{
	static XCAF		*xcaf = NULL;
	static XDF		*xdf = NULL;
	xcb_connection_t	*c;
	char			*e;

	if (xcaf == NULL) {
		xcaf =
		    (XCAF *)xcbsym("xcb_connect_to_display_with_auth_info", &e);
		if (xcaf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}
	if (xdf == NULL) {
		xdf = (XDF *)xcbsym("xcb_disconnect", &e);
		if (xdf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	/* Preload without affecting the initial sequence count. */
	c = (*xcaf)(display, auth, screen);
	preload_atoms_xcb(c);
	(*xdf)(c);

	c = (*xcaf)(display, auth, screen);
	return (c);
}

/* xcb_change_property */
typedef xcb_void_cookie_t (XCPF)(xcb_connection_t *, uint8_t, xcb_window_t,
    xcb_atom_t, xcb_atom_t, uint8_t, uint32_t, const void *);

static void
set_property_xcb(xcb_connection_t *c, xcb_window_t wid, xcb_atom_t atom,
    char *val)
{
	char		prop[SWM_PROPLEN];
	static XCPF	*xcpf = NULL;
	char		*e;

	if (xcpf == NULL) {
		xcpf = (XCPF *)xcbsym("xcb_change_property", &e);
		if (xcpf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	if (atom != XCB_ATOM_NONE &&
	    (snprintf(prop, SWM_PROPLEN, "%s", val)) < SWM_PROPLEN)
		(*xcpf)(c, XCB_PROP_MODE_REPLACE, wid, atom, XCB_ATOM_STRING, 8,
		    strlen((char *)prop), prop);
}

/* XChangeProperty */
typedef int (CPF)(Display *, Window, Atom, Atom, int, int, unsigned char *, int);

static void
set_property_xlib(Display *dpy, Window id, Atom atom, char *val)
{
	char		prop[SWM_PROPLEN];
	static CPF	*cpf = NULL;
	char		*e;

	if (cpf == NULL) {
		cpf = (CPF *)xlibsym("XChangeProperty", &e);
		if (cpf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	if (atom != XCB_ATOM_NONE &&
	    (snprintf(prop, SWM_PROPLEN, "%s", val)) < SWM_PROPLEN)
		(*cpf)(dpy, id, atom, XA_STRING, 8, PropModeReplace,
		    (unsigned char *)prop, strlen((char *)prop));
}

static void
prepare_window_xcb(xcb_connection_t *c, xcb_window_t window)
{
	char		*env;

	if (window == XCB_WINDOW_NONE)
		return;

	if ((env = getenv("_SWM_WS")) != NULL)
		set_property_xcb(c, window, swmws, env);
	if ((env = getenv("_SWM_PID")) != NULL)
		set_property_xcb(c, window, swmpid, env);
	if (getenv("_SWM_XTERM_FONTADJ") != NULL) {
		unsetenv("_SWM_XTERM_FONTADJ");
		xterm = true;
	}
}

static void
prepare_window_xlib(Display *dpy, Window window)
{
	char		*env;

	if (window == None)
		return;

	if ((env = getenv("_SWM_WS")) != NULL)
		set_property_xlib(dpy, window, (Atom)swmws, env);
	if ((env = getenv("_SWM_PID")) != NULL)
		set_property_xlib(dpy, window, (Atom)swmpid, env);
	if (getenv("_SWM_XTERM_FONTADJ") != NULL) {
		unsetenv("_SWM_XTERM_FONTADJ");
		xterm = true;
	}
}

/* xcb_create_window/xcb_create_window_checked */
typedef xcb_void_cookie_t (XCWF)(xcb_connection_t *, uint8_t, xcb_window_t,
    xcb_window_t, int16_t, int16_t, uint16_t, uint16_t, uint16_t, uint16_t,
    xcb_visualid_t, uint32_t, const void *);

xcb_void_cookie_t
xcb_create_window_checked(xcb_connection_t *c, uint8_t depth, xcb_window_t wid,
    xcb_window_t parent, int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const void *value_list)
{
	static XCWF		*xcwcf = NULL;
	xcb_void_cookie_t	xcb_ret;
	char			*e;

	if (xcwcf == NULL) {
		xcwcf = (XCWF *)xcbsym("xcb_create_window_checked", &e);
		if (xcwcf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	xcb_ret = (*xcwcf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);
	prepare_window_xcb(c, wid);

	return (xcb_ret);
}

xcb_void_cookie_t
xcb_create_window(xcb_connection_t *c, uint8_t depth, xcb_window_t wid,
    xcb_window_t parent, int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const void *value_list)
{
	static XCWF		*xcwf = NULL;
	xcb_void_cookie_t	xcb_ret;
	char			*e;

	if (xcwf == NULL) {
		xcwf = (XCWF *)xcbsym("xcb_create_window", &e);
		if (xcwf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	xcb_ret = (*xcwf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);
	prepare_window_xcb(c, wid);

	return (xcb_ret);
}

/* xcb_create_window_aux/xcb_create_window_aux_checked */
typedef xcb_void_cookie_t (XCWAF)(xcb_connection_t *, uint8_t, xcb_window_t,
    xcb_window_t, int16_t, int16_t, uint16_t, uint16_t, uint16_t, uint16_t,
    xcb_visualid_t, uint32_t, const xcb_create_window_value_list_t *);

xcb_void_cookie_t
xcb_create_window_aux_checked(xcb_connection_t *c, uint8_t depth,
    xcb_window_t wid, xcb_window_t parent, int16_t x, int16_t y, uint16_t width,
    uint16_t height, uint16_t border_width, uint16_t _class,
    xcb_visualid_t visual, uint32_t value_mask,
    const xcb_create_window_value_list_t *value_list)
{
	static XCWAF		*xcwacf = NULL;
	xcb_void_cookie_t	xcb_ret;
	char			*e;

	if (xcwacf == NULL) {
		xcwacf = (XCWAF *)xcbsym("xcb_create_window_aux_checked", &e);
		if (xcwacf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	xcb_ret = (*xcwacf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);
	prepare_window_xcb(c, wid);

	return (xcb_ret);
}

xcb_void_cookie_t
xcb_create_window_aux(xcb_connection_t *c, uint8_t depth, xcb_window_t wid,
    xcb_window_t parent, int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const xcb_create_window_value_list_t *value_list)
{
	static XCWAF		*xcwaf = NULL;
	xcb_void_cookie_t	xcb_ret;
	char			*e;

	if (xcwaf == NULL) {
		xcwaf = (XCWAF *)xcbsym("xcb_create_window_aux", &e);
		if (xcwaf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	xcb_ret = (*xcwaf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);
	prepare_window_xcb(c, wid);

	return (xcb_ret);
}

typedef Display *(ODF)(_Xconst char *);
Display *
XOpenDisplay(_Xconst char *_display)
{
	static ODF	*odf = NULL;
	Display		*display;
	char		*e;

	if (odf == NULL) {
		odf = (ODF *)xlibsym("XOpenDisplay", &e);
		if (odf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	display = (*odf)(_display);
	preload_atoms_xlib(display);

	return (display);
}

typedef Window (CWF)(Display *, Window, int, int, unsigned int, unsigned int,
    unsigned int, int, unsigned int, Visual *, unsigned long,
    XSetWindowAttributes *);
Window
XCreateWindow(Display *dpy, Window parent, int x, int y, unsigned int width,
    unsigned int height, unsigned int border_width, int depth,
    unsigned int _class, Visual *visual, unsigned long valuemask,
    XSetWindowAttributes * attributes)
{
	static CWF	*cwf = NULL;
	Window		wid;
	char		*e;

	if (cwf == NULL) {
		cwf = (CWF *)xlibsym("XCreateWindow", &e);
		if (cwf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	wid = (*cwf)(dpy, parent, x, y, width, height, border_width, depth,
	    _class, visual, valuemask, attributes);
	prepare_window_xlib(dpy, wid);

	return (wid);
}

typedef Window (CSWF)(Display *, Window, int, int, unsigned int, unsigned int,
    unsigned int, unsigned long, unsigned long);
Window
XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
    unsigned int width, unsigned int height, unsigned int border_width,
    unsigned long border, unsigned long background)
{
	static CSWF	*cswf = NULL;
	Window		wid;
	char		*e;

	if (cswf == NULL) {
		cswf = (CSWF *)xlibsym("XCreateSimpleWindow", &e);
		if (cswf == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	wid = (*cswf)(dpy, parent, x, y, width, height, border_width, border,
	    background);
	prepare_window_xlib(dpy, wid);

	return (wid);
}

typedef KeyCode (KTKF)(Display *, KeySym);

/*
 * XtAppNextEvent Intercept Hack
 * Normally xterm rejects "synthetic" (XSendEvent) events to prevent spoofing.
 * We don't want to disable this completely, it's insecure. But hook here
 * and allow these mostly harmless ones that we use to adjust fonts.
 */
typedef void (ANEF)(XtAppContext, XEvent *);
void
XtAppNextEvent(XtAppContext app_context, XEvent *event_return)
{
	static ANEF	*anef = NULL;
	static KTKF	*xktkf = NULL;
	static KeyCode	kp_add = 0, kp_subtract = 0;
	char		*e;

	if (anef == NULL) {
		anef = (ANEF *)xtlibsym("XtAppNextEvent", &e);
		if (anef == NULL) {
			fprintf(stderr, "libswmhack.so: ERROR: %s\n", e);
			exit(1);
		}
	}

	(*anef)(app_context, event_return);

	/* Return here if it's not an Xterm. */
	if (!xterm || event_return == NULL)
		return;

	if (event_return->type != KeyPress && event_return->type != KeyRelease)
		return;

	if (xktkf == NULL) {
		xktkf = (KTKF *)xlibsym("XKeysymToKeycode", &e);
		if (xktkf == NULL) {
			fprintf(stderr, "libswmhack.so: %s\n", e);
			return;
		}

		kp_add = (*xktkf)(event_return->xany.display, XK_KP_Add);
		kp_subtract = (*xktkf)(event_return->xany.display,
		    XK_KP_Subtract);
	}

	if (kp_add == 0 || kp_subtract == 0)
		return;

	/* Allow spoofing of font change keystrokes. */
	if (event_return->xkey.state == ShiftMask &&
	   (event_return->xkey.keycode == kp_add ||
	   event_return->xkey.keycode == kp_subtract))
		event_return->xkey.send_event = 0;
}
