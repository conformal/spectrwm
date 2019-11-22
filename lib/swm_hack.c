/*
 * Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2011-2018 Reginald Kennedy <rk@rejii.com>
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

/* dlopened libs so we can find the symbols in the real one to call them */
static void		*lib_xlib = NULL;
static void		*lib_xtlib = NULL;

static bool		xterm = false;
static Display		*display = NULL;

static Atom		swmws = None, swmpid = None;

void	set_property(Display *, Window, Atom, char *);
Atom	get_atom_from_string(Display *, char *);

#if defined(_GNU_SOURCE) && !defined(__CYGWIN__)
#define DLOPEN(s)	RTLD_NEXT
#else
#define DLOPEN(s)	dlopen((s), RTLD_GLOBAL | RTLD_LAZY)
#endif

typedef Atom (XIA)(Display *_display, char *atom_name, Bool only_if_exists);

Atom
get_atom_from_string(Display *dpy, char *name)
{
	Atom			atom = None;
	static XIA		*xia = NULL;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib) {
		if (xia == NULL)
			xia = (XIA *) dlsym(lib_xlib, "XInternAtom");
	}
	if (xia == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return (atom);
	}

	atom = (*xia)(dpy, name, False);
	return (atom);
}

typedef int (XCP)(Display *_display, Window w, Atom property, Atom type,
    int format, int mode, unsigned char *data, int nelements);

#define SWM_PROPLEN	(16)
void
set_property(Display *dpy, Window id, Atom atom, char *val)
{
	char			prop[SWM_PROPLEN];
	static XCP		*xcp = NULL;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib) {
		if (xcp == NULL)
			xcp = (XCP *) dlsym(lib_xlib, "XChangeProperty");
	}
	if (xcp == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return;
	}

	/* Try to update the window's workspace property */
	if (atom)
		if (snprintf(prop, SWM_PROPLEN, "%s", val) < SWM_PROPLEN)
			(*xcp)(dpy, id, atom, XA_STRING,
			    8, PropModeReplace, (unsigned char *)prop,
			    strlen((char *)prop));
}

typedef Display *(ODF)(register _Xconst char *_display);

/* XOpenDisplay intercept hack */
Display *
XOpenDisplay(register _Xconst char *_display)
{
	static ODF	*func = NULL;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib && func == NULL)
		func = (ODF *) dlsym(lib_xlib, "XOpenDisplay");
	if (func == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return (None);
	}

	display = (*func) (_display);

	/* Preload atoms to prevent deadlock. */
	if (swmws == None)
		swmws = get_atom_from_string(display, "_SWM_WS");
	if (swmpid == None)
		swmpid = get_atom_from_string(display, "_SWM_PID");

	return (display);
}

typedef Window (CWF)(Display * _display, Window _parent, int _x, int _y,
    unsigned int _width, unsigned int _height, unsigned int _border_width,
    int _depth, unsigned int _class, Visual * _visual, unsigned long _valuemask,
    XSetWindowAttributes * _attributes);

/* XCreateWindow intercept hack */
Window
XCreateWindow(Display *dpy, Window parent, int x, int y, unsigned int width,
    unsigned int height, unsigned int border_width, int depth,
    unsigned int clss, Visual * visual, unsigned long valuemask,
    XSetWindowAttributes * attributes)
{
	static CWF	*func = NULL;
	char		*env;
	Window		id;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib && func == NULL)
		func = (CWF *) dlsym(lib_xlib, "XCreateWindow");
	if (func == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return (None);
	}

	id = (*func) (dpy, parent, x, y, width, height, border_width,
	    depth, clss, visual, valuemask, attributes);

	if (id) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property(dpy, id, swmws, env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property(dpy, id, swmpid, env);
		if (getenv("_SWM_XTERM_FONTADJ") != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}
	return (id);
}

typedef Window (CSWF)(Display * _display, Window _parent, int _x, int _y,
    unsigned int _width, unsigned int _height, unsigned int _border_width,
    unsigned long _border, unsigned long _background);

/* XCreateSimpleWindow intercept hack */
Window
XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
    unsigned int width, unsigned int height, unsigned int border_width,
    unsigned long border, unsigned long background)
{
	static CSWF	*func = NULL;
	char		*env;
	Window		id;

	if (lib_xlib == NULL)
		lib_xlib = DLOPEN("libX11.so");
	if (lib_xlib && func == NULL)
		func = (CSWF *) dlsym(lib_xlib, "XCreateSimpleWindow");
	if (func == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return (None);
	}

	id = (*func) (dpy, parent, x, y, width, height,
	    border_width, border, background);

	if (id) {
		if ((env = getenv("_SWM_WS")) != NULL)
			set_property(dpy, id, swmws, env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property(dpy, id, swmpid, env);
		if (getenv("_SWM_XTERM_FONTADJ") != NULL) {
			unsetenv("_SWM_XTERM_FONTADJ");
			xterm = true;
		}
	}
	return (id);
}

typedef void (ANEF)(XtAppContext app_context, XEvent *event_return);

/*
 * XtAppNextEvent Intercept Hack
 * Normally xterm rejects "synthetic" (XSendEvent) events to prevent spoofing.
 * We don't want to disable this completely, it's insecure. But hook here
 * and allow these mostly harmless ones that we use to adjust fonts.
 */
void
XtAppNextEvent(XtAppContext app_context, XEvent *event_return)
{
	static ANEF	*func = NULL;
	static KeyCode	kp_add = 0, kp_subtract = 0;

	if (lib_xtlib == NULL)
		lib_xtlib = DLOPEN("libXt.so");
	if (lib_xtlib && func == NULL) {
		func = (ANEF *) dlsym(lib_xtlib, "XtAppNextEvent");
		if (display) {
			kp_add = XKeysymToKeycode(display, XK_KP_Add);
			kp_subtract = XKeysymToKeycode(display, XK_KP_Subtract);
		}
	}
	if (func == NULL) {
		fprintf(stderr, "libswmhack.so: ERROR: %s\n", dlerror());
		return;
	}

	(*func) (app_context, event_return);

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
