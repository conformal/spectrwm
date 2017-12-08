/*
 * Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2011-2017 Reginald Kennedy <rk@rejii.com>
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

static void		*lib_xcb = NULL;
static void		*lib_xlib = NULL;
static void		*lib_xtlib = NULL;

static xcb_connection_t	*conn = NULL;
static Display		*display = NULL;

static bool		xterm = false;

void	set_property_xcb(xcb_connection_t *, xcb_window_t, char *, char *);
void	set_property_xlib(Display *, Window, char *, char *);

#ifdef _GNU_SOURCE
#define DLOPEN(s)	RTLD_NEXT
#else
#define DLOPEN(s)	dlopen((s), RTLD_GLOBAL | RTLD_LAZY)
#endif

#define SWM_PROPLEN	(16)

/* xcb_intern_atom */
typedef xcb_intern_atom_cookie_t (XIAF)(xcb_connection_t *, uint8_t, uint16_t,
    const char *);

/* xcb_intern_atom_reply */
typedef xcb_intern_atom_reply_t *(XIARF)(xcb_connection_t *,
    xcb_intern_atom_cookie_t, xcb_generic_error_t **);

/* xcb_change_property */
typedef xcb_void_cookie_t (XCPF)(xcb_connection_t *, uint8_t, xcb_window_t,
    xcb_atom_t, xcb_atom_t, uint8_t, uint32_t, const void *);

void
set_property_xcb(xcb_connection_t *c, xcb_window_t wid, char *name, char *val)
{
	xcb_atom_t			atom = XCB_ATOM_NONE;
	xcb_intern_atom_cookie_t	iac;
	xcb_intern_atom_reply_t		*iar;
	char				prop[SWM_PROPLEN];
	static XIAF			*xiaf = NULL;
	static XIARF			*xiarf = NULL;
	static XCPF			*xcpf = NULL;

	/* Obtain function pointers. */
	if (lib_xcb == NULL)
		lib_xcb = DLOPEN("libxcb.so");
	if (lib_xcb) {
		if (xiaf == NULL)
			xiaf = (XIAF *)dlsym(lib_xcb, "xcb_intern_atom");
		if (xiarf == NULL)
			xiarf = (XIARF *)dlsym(lib_xcb, "xcb_intern_atom_reply");
		if (xcpf == NULL)
			xcpf = (XCPF *)dlsym(lib_xcb, "xcb_change_property");
	}
	if (xiaf == NULL || xiarf == NULL || xcpf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return;
	}

	/* Set the window property with the located functions. */
	iac = (*xiaf)(c, 0, strlen(name), name);
	iar = (*xiarf)(c, iac, NULL);
	if (iar) {
		atom = iar->atom;
		free(iar);

		if (snprintf(prop, SWM_PROPLEN, "%s", val) < SWM_PROPLEN)
			(*xcpf)(c, XCB_PROP_MODE_REPLACE, wid, atom,
			    XCB_ATOM_STRING, 8, strlen((char *)prop), prop);
	}
}

/* xcb_create_window/xcb_create_window_checked */
typedef xcb_void_cookie_t (XCWF)(xcb_connection_t *, uint8_t, xcb_window_t,
    xcb_window_t, int16_t, int16_t, uint16_t, uint16_t, uint16_t, uint16_t,
    xcb_visualid_t, uint32_t, const void *);

xcb_void_cookie_t
xcb_create_window(xcb_connection_t *c, uint8_t depth, xcb_window_t wid,
    xcb_window_t parent, int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const void *value_list)
{
	static XCWF		*xcwf = NULL;
	char			*env;
	xcb_void_cookie_t	xcb_ret;

	if (lib_xcb == NULL)
		lib_xcb = DLOPEN("libxcb.so");
	if (lib_xcb && xcwf == NULL) {
		xcwf = (XCWF *)dlsym(lib_xcb, "xcb_create_window");
		conn = c;
	}
	if (xcwf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return xcb_ret;
	}

	xcb_ret = (*xcwf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);

	if (wid != XCB_WINDOW_NONE) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property_xcb(c, wid, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property_xcb(c, wid, "_SWM_PID", env);
		if ((env = getenv("_SWM_XTERM_FONTADJ")) != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}

	return xcb_ret;
}

xcb_void_cookie_t
xcb_create_window_checked(xcb_connection_t *c, uint8_t depth, xcb_window_t wid,
    xcb_window_t parent, int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const void *value_list)
{
	static XCWF		*xcwf = NULL;
	char			*env;
	xcb_void_cookie_t	xcb_ret;

	if (lib_xcb == NULL)
		lib_xcb = DLOPEN("libxcb.so");
	if (lib_xcb && xcwf == NULL) {
		xcwf = (XCWF *)dlsym(lib_xcb, "xcb_create_window_checked");
		conn = c;
	}
	if (xcwf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return xcb_ret;
	}

	xcb_ret = (*xcwf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);

	if (wid != XCB_WINDOW_NONE) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property_xcb(c, wid, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property_xcb(c, wid, "_SWM_PID", env);
		if ((env = getenv("_SWM_XTERM_FONTADJ")) != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}

	return xcb_ret;
}

/* xcb_create_window_aux/xcb_create_window_aux_checked */
typedef xcb_void_cookie_t (XCWAF)(xcb_connection_t *, uint8_t, xcb_window_t,
    xcb_window_t, int16_t, int16_t, uint16_t, uint16_t, uint16_t, uint16_t,
    xcb_visualid_t, uint32_t, const xcb_create_window_value_list_t *);

xcb_void_cookie_t
xcb_create_window_aux(xcb_connection_t *c, uint8_t depth, xcb_window_t wid,
    xcb_window_t parent, int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const xcb_create_window_value_list_t *value_list)
{
	static XCWAF		*xcwaf = NULL;
	char			*env;
	xcb_void_cookie_t	xcb_ret;

	if (lib_xcb == NULL)
		lib_xcb = DLOPEN("libxcb.so");
	if (lib_xcb && xcwaf == NULL) {
		xcwaf = (XCWAF *)dlsym(lib_xcb, "xcb_create_window_aux");
		conn = c;
	}
	if (xcwaf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return xcb_ret;
	}

	xcb_ret = (*xcwaf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);

	if (wid != XCB_WINDOW_NONE) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property_xcb(c, wid, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property_xcb(c, wid, "_SWM_PID", env);
		if ((env = getenv("_SWM_XTERM_FONTADJ")) != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}

	return xcb_ret;
}

xcb_void_cookie_t
xcb_create_window_aux_checked(xcb_connection_t *c, uint8_t depth,
    xcb_window_t wid, xcb_window_t parent, int16_t x, int16_t y, uint16_t width,
    uint16_t height, uint16_t border_width, uint16_t _class,
    xcb_visualid_t visual, uint32_t value_mask,
    const xcb_create_window_value_list_t *value_list)
{
	static XCWAF		*xcwaf = NULL;
	char			*env;
	xcb_void_cookie_t	xcb_ret;

	if (lib_xcb == NULL)
		lib_xcb = DLOPEN("libxcb.so");
	if (lib_xcb && xcwaf == NULL) {
		xcwaf = (XCWAF *)dlsym(lib_xcb, "xcb_create_window_aux");
		conn = c;
	}
	if (xcwaf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return xcb_ret;
	}

	xcb_ret = (*xcwaf)(c, depth, wid, parent, x, y, width, height,
	    border_width, _class, visual, value_mask, value_list);

	if (wid != XCB_WINDOW_NONE) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property_xcb(c, wid, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property_xcb(c, wid, "_SWM_PID", env);
		if ((env = getenv("_SWM_XTERM_FONTADJ")) != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}

	return xcb_ret;
}

/* XInternAtom */
typedef Atom (IAF)(Display *, char *, Bool);

/* XChangeProperty */
typedef int (CPF)(Display *, Window, Atom, Atom, int, int, unsigned char *, int);

void
set_property_xlib(Display *dpy, Window id, char *name, char *val)
{
	Atom			atom = 0;
	char			prop[SWM_PROPLEN];
	static IAF		*iaf = NULL;
	static CPF		*cpf = NULL;

	/* Obtain function pointers. */
	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib) {
		if (iaf == NULL)
			iaf = (IAF *)dlsym(lib_xlib, "XInternAtom");
		if (cpf == NULL)
			cpf = (CPF *)dlsym(lib_xlib, "XChangeProperty");
	}
	if (iaf == NULL || cpf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return;
	}

	/* Set the window property with the located functions. */
	atom = (*iaf)(dpy, name, False);
	if (atom)
		if (snprintf(prop, SWM_PROPLEN, "%s", val) < SWM_PROPLEN)
			(*cpf)(dpy, id, atom, XA_STRING, 8, PropModeReplace,
			    (unsigned char *)prop, strlen((char *)prop));
}


/* XCreateWindow */
typedef Window (CWF)(Display *, Window, int, int, unsigned int, unsigned int,
    unsigned int, int, unsigned int, Visual *, unsigned long,
    XSetWindowAttributes *);

Window
XCreateWindow(Display *dpy, Window parent, int x, int y, unsigned int width,
    unsigned int height, unsigned int border_width, int depth, unsigned int clss,
    Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes)
{
	static CWF	*cwf = NULL;
	char		*env;
	Window		id;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib && cwf == NULL) {
		cwf = (CWF *)dlsym(lib_xlib, "XCreateWindow");
		display = dpy;
	}
	if (cwf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return None;
	}

	id = (*cwf)(dpy, parent, x, y, width, height, border_width,
	    depth, clss, visual, valuemask, attributes);

	if (id) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property_xlib(dpy, id, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property_xlib(dpy, id, "_SWM_PID", env);
		if ((env = getenv("_SWM_XTERM_FONTADJ")) != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}
	return (id);
}

/* XCreateSimpleWindow */
typedef Window (CSWF)(Display *, Window, int, int, unsigned int, unsigned int,
    unsigned int, unsigned long, unsigned long);

Window
XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
    unsigned int width, unsigned int height, unsigned int border_width,
    unsigned long border, unsigned long background)
{
	static CSWF	*cswf = NULL;
	char		*env;
	Window		id;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib && cswf == NULL) {
		cswf = (CSWF *)dlsym(lib_xlib, "XCreateSimpleWindow");
		display = dpy;
	}
	if (cswf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return None;
	}

	id = (*cswf)(dpy, parent, x, y, width, height,
	    border_width, border, background);

	if (id) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property_xlib(dpy, id, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property_xlib(dpy, id, "_SWM_PID", env);
		if ((env = getenv("_SWM_XTERM_FONTADJ")) != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}
	return (id);
}

/* XtAppNextEvent */
typedef	void (ANEF)(XtAppContext, XEvent *);

/* XKeysymToKeycode */
typedef KeyCode (KTKF)(Display *, KeySym);

/*
 * Normally xterm rejects "synthetic" (XSendEvent) events to prevent spoofing.
 * We don't want to disable this completely, it's insecure. But hook here
 * and allow these mostly harmless ones that we use to adjust fonts.
 */
void
XtAppNextEvent(XtAppContext app_context, XEvent *event_return)
{
	static ANEF	*anef = NULL;
	static KTKF	*ktkf = NULL;
	static KeyCode	kp_add = 0, kp_subtract = 0;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib && ktkf == NULL)
		ktkf = (KTKF *)dlsym(lib_xlib, "XKeysymToKeycode");
	if (lib_xtlib == NULL)
		lib_xtlib = DLOPEN("libXt.so");
	if (lib_xtlib && anef == NULL)
		anef = (ANEF *)dlsym(lib_xtlib, "XtAppNextEvent");
	if (anef == NULL || ktkf == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return;
	}

	if (display && (kp_add == 0 || kp_subtract == 0)) {
		kp_add = (*ktkf)(display, XK_KP_Add);
		kp_subtract = (*ktkf)(display, XK_KP_Subtract);
	}

	(*anef)(app_context, event_return);

	/* Return here if it's not an Xterm. */
	if (!xterm)
		return;

	/* Allow spoofing of font change keystrokes. */
	if ((event_return->type == KeyPress ||
	   event_return->type == KeyRelease) &&
	   event_return->xkey.state == ShiftMask &&
	   (event_return->xkey.keycode == kp_add ||
	   event_return->xkey.keycode == kp_subtract))
		event_return->xkey.send_event = 0;
}
