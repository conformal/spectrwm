/* $scrotwm$ */
/*
 * Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
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

#define	SWM_VERSION	"0.5"

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <locale.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/queue.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* #define SWM_DEBUG */
#ifdef SWM_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while(0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while(0)
#define	SWM_D_EVENT		0x0001
#define	SWM_D_WS		0x0002

uint32_t		swm_debug = 0
			    | SWM_D_EVENT
			    | SWM_D_WS
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define LENGTH(x)               (sizeof x / sizeof x[0])
#define MODKEY			Mod1Mask
#define CLEANMASK(mask)         (mask & ~(numlockmask | LockMask))

int			(*xerrorxlib)(Display *, XErrorEvent *);
int			other_wm;
int			screen;
int			width, height;
int			running = 1;
int			ignore_enter = 0;
unsigned int		numlockmask = 0;
unsigned long		col_focus = 0xff0000;
unsigned long		col_unfocus = 0x888888;
Display			*display;
Window			root;

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	Window			id;
};

TAILQ_HEAD(ws_win_list, ws_win);

/* define work spaces */
#define SWM_WS_MAX		(10)
struct workspace {
	int			 visible;	/* workspace visible */
	struct ws_win		*focus;		/* which win has focus */
	int			 winno;		/* total nr of windows */
	struct ws_win_list	 winlist;
} ws[SWM_WS_MAX];
int			current_ws = 0;

/* args to functions */
union arg {
	int		id;
#define SWM_ARG_ID_FOCUSNEXT	(0)
#define SWM_ARG_ID_FOCUSPREV	(1)
#define SWM_ARG_ID_FOCUSMAIN	(2)
	char		**argv;
};

void
quit(union arg *args)
{
	running = 0;
}

void
spawn(union arg *args)
{
	/*
	 * The double-fork construct avoids zombie processes and keeps the code
	 * clean from stupid signal handlers.
	 */
	if(fork() == 0) {
		if(fork() == 0) {
			if(display)
				close(ConnectionNumber(display));
			setsid();
			execvp(args->argv[0], args->argv);
			fprintf(stderr, "execvp failed\n");
			perror(" failed");
		}
		exit(0);
	}
	wait(0);
}

void
focus_win(struct ws_win *win)
{
	XSetWindowBorder(display, win->id, col_focus);
	XSetInputFocus(display, win->id, RevertToPointerRoot, CurrentTime);
	ws[current_ws].focus = win;
}

void
unfocus_win(struct ws_win *win)
{
	XSetWindowBorder(display, win->id, col_unfocus);
	if (ws[current_ws].focus == win)
		ws[current_ws].focus = NULL;
}

void
switchws(union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win;
	Window			winfocus;

	DNPRINTF(SWM_D_WS, "switchws: %d\n", wsid + 1);

	if (wsid == current_ws)
		return;

	/* map new window first to prevent ugly blinking */
	TAILQ_FOREACH (win, &ws[wsid].winlist, entry)
		XMapWindow(display, win->id);
	ws[wsid].visible = 1;

	TAILQ_FOREACH (win, &ws[current_ws].winlist, entry)
		XUnmapWindow(display, win->id);
	ws[current_ws].visible = 0;

	current_ws = wsid;

	ignore_enter = 1;
	if (ws[wsid].focus != NULL)
		focus_win(ws[wsid].focus);
	XSync(display, False);
}

void
focus(union arg *args)
{
	struct ws_win		*winfocus, *winlostfocus;

	if (ws[current_ws].focus == NULL || ws[current_ws].winno == 0)
		return;

	winlostfocus = ws[current_ws].focus;

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		if (ws[current_ws].focus ==
		    TAILQ_FIRST(&ws[current_ws].winlist))
			ws[current_ws].focus =
			    TAILQ_LAST(&ws[current_ws].winlist, ws_win_list);
		else
			ws[current_ws].focus =TAILQ_PREV(ws[current_ws].focus,
			    ws_win_list, entry);
		break;

	case SWM_ARG_ID_FOCUSNEXT:
		if (ws[current_ws].focus == TAILQ_LAST(&ws[current_ws].winlist,
		    ws_win_list))
			ws[current_ws].focus =
			    TAILQ_FIRST(&ws[current_ws].winlist);
		else
			ws[current_ws].focus =
			    TAILQ_NEXT(ws[current_ws].focus, entry);
		break;

	case SWM_ARG_ID_FOCUSMAIN:
		ws[current_ws].focus = TAILQ_FIRST(&ws[current_ws].winlist);;
		break;

	default:
		return;
	}

	winfocus = ws[current_ws].focus;
	unfocus_win(winlostfocus);
	focus_win(winfocus);
	XSync(display, False);
}

/* I know this sucks but it works well enough */
void
stack(void)
{
	XWindowChanges		wc;
	struct ws_win		wf, *win, *winfocus = &wf;
	int			i, h, w, x, y, winno, hrh;

	DNPRINTF(SWM_D_EVENT, "stack: workspace: %d\n", current_ws);

	winfocus->id = root;

	if (ws[current_ws].winno == 0)
		return;

	if (ws[current_ws].winno > 1)
		w = width / 2;
	else
		w = width;

	if (ws[current_ws].winno > 2)
		hrh = height / (ws[current_ws].winno - 1);
	else
		hrh = 0;

	x = 0;
	y = 0;
	h = height;
	i = 0;
	TAILQ_FOREACH (win, &ws[current_ws].winlist, entry) {
		if (i == 1) {
			x += w + 2;
			w -= 2;
		}
		if (i != 0 && hrh != 0) {
			/* correct the last window for lost pixels */
			if (win == TAILQ_LAST(&ws[current_ws].winlist,
			    ws_win_list)) {
				h = height - (i * hrh);
				if (h == 0)
					h = hrh;
				else
					h += hrh;
				y += hrh;
			} else {
				h = hrh - 2;
				/* leave first right hand window at y = 0 */
				if (i > 1)
					y += h + 2;
			}
		}

		bzero(&wc, sizeof wc);
		wc.x = x;
		wc.y = y;
		wc.width = w;
		wc.height = h;
		wc.border_width = 1;
		XConfigureWindow(display, win->id, CWX | CWY | CWWidth |
		    CWHeight | CWBorderWidth, &wc);
		if (win == ws[current_ws].focus)
			winfocus = win;
		else
			unfocus_win(win);
		XMapWindow(display, win->id);
		i++;
	}

	focus_win(winfocus); /* this has to be done outside of the loop */
	XSync(display, False);
}

void
swap_to_main(union arg *args)
{
	struct ws_win 		*tmpwin = TAILQ_FIRST(&ws[current_ws].winlist);

	TAILQ_REMOVE(&ws[current_ws].winlist, tmpwin, entry);
	TAILQ_INSERT_AFTER(&ws[current_ws].winlist, ws[current_ws].focus,
	    tmpwin, entry);
	TAILQ_REMOVE(&ws[current_ws].winlist, ws[current_ws].focus, entry);
	TAILQ_INSERT_HEAD(&ws[current_ws].winlist, ws[current_ws].focus, entry);
	ws[current_ws].focus = TAILQ_FIRST(&ws[current_ws].winlist);
	ignore_enter = 2;
	stack();
}

/* terminal + args */
char				*term[] = { "xterm", NULL };

/* key definitions */
struct key {
	unsigned int		mod;
	KeySym			keysym;
	void			(*func)(union arg *);
	union arg		args;
} keys[] = {
	/* modifier		key	function		argument */
	{ MODKEY,		XK_Return,	swap_to_main,	{0} },
	{ MODKEY | ShiftMask,	XK_Return,	spawn,		{.argv = term } },
	{ MODKEY | ShiftMask,	XK_q,		quit,		{0} },
	{ MODKEY,		XK_m,		focus,		{.id = SWM_ARG_ID_FOCUSMAIN} },
	{ MODKEY,		XK_1,		switchws,	{.id = 0} },
	{ MODKEY,		XK_2,		switchws,	{.id = 1} },
	{ MODKEY,		XK_3,		switchws,	{.id = 2} },
	{ MODKEY,		XK_4,		switchws,	{.id = 3} },
	{ MODKEY,		XK_5,		switchws,	{.id = 4} },
	{ MODKEY,		XK_6,		switchws,	{.id = 5} },
	{ MODKEY,		XK_7,		switchws,	{.id = 6} },
	{ MODKEY,		XK_8,		switchws,	{.id = 7} },
	{ MODKEY,		XK_9,		switchws,	{.id = 8} },
	{ MODKEY,		XK_0,		switchws,	{.id = 9} },
	{ MODKEY,		XK_Tab,		focus,		{.id = SWM_ARG_ID_FOCUSNEXT} },
	{ MODKEY | ShiftMask,	XK_Tab,		focus,		{.id = SWM_ARG_ID_FOCUSPREV} },
};

void
updatenumlockmask(void)
{
	unsigned int		i, j;
	XModifierKeymap		*modmap;

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
	unsigned int		i, j;
	KeyCode			code;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask | LockMask };

	updatenumlockmask();

	XUngrabKey(display, AnyKey, AnyModifier, root);
	for(i = 0; i < LENGTH(keys); i++) {
		if((code = XKeysymToKeycode(display, keys[i].keysym)))
			for(j = 0; j < LENGTH(modifiers); j++)
				XGrabKey(display, code,
				    keys[i].mod | modifiers[j], root,
				    True, GrabModeAsync, GrabModeAsync);
	}
}
void
expose(XEvent *e)
{
	XExposeEvent		*ev = &e->xexpose;

	DNPRINTF(SWM_D_EVENT, "expose: window: %d\n", ev->window);
}

void
keypress(XEvent *e)
{
	unsigned int		i;
	KeySym			keysym;
	XKeyEvent		*ev = &e->xkey;

	DNPRINTF(SWM_D_EVENT, "keypress: window: %d\n", ev->window);

	keysym = XKeycodeToKeysym(display, (KeyCode)ev->keycode, 0);
	for(i = 0; i < LENGTH(keys); i++)
		if(keysym == keys[i].keysym
		   && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		   && keys[i].func)
			keys[i].func(&(keys[i].args));
}

void
buttonpress(XEvent *e)
{
	XButtonPressedEvent	*ev = &e->xbutton;
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "buttonpress: window: %d\n", ev->window);

	if (ev->window == root)
		return;
	if (ev->window == ws[current_ws].focus->id)
		return;
#if 0
	TAILQ_FOREACH(win, &ws[current_ws].winlist, entry)
		if (win->id == ev->window) {
			/* focus in the clicked window */
			XSetWindowBorder(display, ev->window, 0xff0000);
			XSetWindowBorder(display,
			    ws[current_ws].focus->id, 0x888888);
			XSetInputFocus(display, ev->window, RevertToPointerRoot,
			    CurrentTime);
			ws[current_ws].focus = win;
			XSync(display, False);
			break;
	}
#endif
}

void
configurerequest(XEvent *e)
{
	XConfigureRequestEvent	*ev = &e->xconfigurerequest;
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "configurerequest: window: %d\n", ev->window);

	XSelectInput(display, ev->window, ButtonPressMask | EnterWindowMask |
	    FocusChangeMask);

	if ((win = calloc(1, sizeof(struct ws_win))) == NULL)
		errx(1, "calloc: failed to allocate memory for new window");

	win->id = ev->window;
	TAILQ_INSERT_TAIL(&ws[current_ws].winlist, win, entry);
	ws[current_ws].focus = win; /* make new win focused */
	ws[current_ws].winno++;
	stack();
}

void
configurenotify(XEvent *e)
{
	XConfigureEvent		*ev = &e->xconfigure;

	DNPRINTF(SWM_D_EVENT, "configurenotify: window: %d\n", ev->window);
}

void
destroynotify(XEvent *e)
{
	size_t			 sz;
	struct ws_win		*win;
	XDestroyWindowEvent	*ev = &e->xdestroywindow;

	DNPRINTF(SWM_D_EVENT, "destroynotify: window %d\n", ev->window);

	TAILQ_FOREACH (win, &ws[current_ws].winlist, entry) {
		DNPRINTF(SWM_D_EVENT, "trying: %x\n", win->id);
		if (ev->window == win->id) {
			/* find a window to focus */
			ws[current_ws].focus =
			    TAILQ_PREV(win, ws_win_list,entry);
			if (ws[current_ws].focus == NULL)
				ws[current_ws].focus =
				    TAILQ_FIRST(&ws[current_ws].winlist);
	
			TAILQ_REMOVE(&ws[current_ws].winlist, win, entry);
			free(win);
			ws[current_ws].winno--;
			break;
		}
	}

	stack();
}

void
enternotify(XEvent *e)
{
	XCrossingEvent		*ev = &e->xcrossing;
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "enternotify: window: %d\n", ev->window);

	if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) &&
	    ev->window != root)
		return;
	if (ignore_enter) {
		/* eat event(s) to prevent autofocus */
		ignore_enter--;
		return;
	}
	TAILQ_FOREACH (win, &ws[current_ws].winlist, entry) {
		if (win->id == ev->window)
			focus_win(win);
		else
			unfocus_win(win);
	}
}

void
focusin(XEvent *e)
{
	XFocusChangeEvent	*ev = &e->xfocus;

	DNPRINTF(SWM_D_EVENT, "focusin: window: %d\n", ev->window);

	if (ev->window == root)
		return;

	/*
	 * kill grab for now so that we can cut and paste , this screws up
	 * click to focus
	 */
	/*
	DNPRINTF(SWM_D_EVENT, "focusin: window: %d grabbing\n", ev->window);
	XGrabButton(display, Button1, AnyModifier, ev->window, False,
	    ButtonPress, GrabModeAsync, GrabModeSync, None, None);
	*/
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent		*ev = &e->xmapping;

	DNPRINTF(SWM_D_EVENT, "mappingnotify: window: %d\n", ev->window);

	XRefreshKeyboardMapping(ev);
	if(ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	XMapRequestEvent	*ev = &e->xmaprequest;

	DNPRINTF(SWM_D_EVENT, "maprequest: window: %d\n", ev->window);
}

void
propertynotify(XEvent *e)
{
	XPropertyEvent		*ev = &e->xproperty;

	DNPRINTF(SWM_D_EVENT, "propertynotify: window: %d\n", ev->window);
}

void
unmapnotify(XEvent *e)
{
	XUnmapEvent		*ev = &e->xunmap;

	DNPRINTF(SWM_D_EVENT, "unmapnotify: window: %d\n", ev->window);
}

void			(*handler[LASTEvent])(XEvent *) = {
				[Expose] = expose,
				[KeyPress] = keypress,
				[ButtonPress] = buttonpress,
				[ConfigureRequest] = configurerequest,
				[ConfigureNotify] = configurenotify,
				[DestroyNotify] = destroynotify,
				[EnterNotify] = enternotify,
				[FocusIn] = focusin,
				[MappingNotify] = mappingnotify,
				[MapRequest] = maprequest,
				[PropertyNotify] = propertynotify,
				[UnmapNotify] = unmapnotify,
};

int
xerror_start(Display *d, XErrorEvent *ee)
{
	other_wm = 1;
	return (-1);
}

int
xerror(Display *d, XErrorEvent *ee)
{
	fprintf(stderr, "error: %p %p\n", display, ee);

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
	if(other_wm)
		return (1);

	XSetErrorHandler(xerror);
	XSync(display, False);
	return (0);
}

int
main(int argc, char *argv[])
{
	XEvent			e;
	int			i;

	fprintf(stderr, "Welcome to scrotwm V%s\n", SWM_VERSION);
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		warnx("no locale support");

	if(!(display = XOpenDisplay(0)))
		errx(1, "can not open display");

	if (active_wm())
		errx(1, "other wm running");

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);
	width = DisplayWidth(display, screen) - 2;
	height = DisplayHeight(display, screen) - 2;

	/* make work space 1 active */
	ws[0].visible = 1;
	ws[0].focus = NULL;
	ws[0].winno = 0;
	TAILQ_INIT(&ws[0].winlist);
	for (i = 1; i < SWM_WS_MAX; i++) {
		ws[i].visible = 0;
		ws[i].focus = NULL;
		ws[i].winno = 0;
		TAILQ_INIT(&ws[i].winlist);
	}

	XSelectInput(display, root, SubstructureRedirectMask |
	    SubstructureNotifyMask | ButtonPressMask | KeyPressMask |
	    EnterWindowMask | LeaveWindowMask | StructureNotifyMask |
	    FocusChangeMask | PropertyChangeMask);

	grabkeys();

	while (running) {
		XNextEvent(display, &e);
		if (handler[e.type])
			handler[e.type](&e);
	}

	XCloseDisplay(display);

	return (0);
}
