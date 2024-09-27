/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#include <X11/Xft/Xft.h>

#include "patches.h"
/* Patch incompatibility overrides */
#if MULTI_SELECTION_PATCH
#undef NON_BLOCKING_STDIN_PATCH
#undef PIPEOUT_PATCH
#undef PRINTINPUTTEXT_PATCH
#endif // MULTI_SELECTION_PATCH

#include "drw.h"
#include "util.h"
#if GRIDNAV_PATCH
#include <stdbool.h>
#endif // GRIDNAV_PATCH

/* macros */
#define INTERSECT(x,y,w,h,r)  (MAX(0, MIN((x)+(w),(r).x_org+(r).width)  - MAX((x),(r).x_org)) \
                             * MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))
#if PANGO_PATCH
#define TEXTW(X)              (drw_font_getwidth(drw, (X), False) + lrpad)
#define TEXTWM(X)             (drw_font_getwidth(drw, (X), True) + lrpad)
#else
#define TEXTW(X)              (drw_fontset_getwidth(drw, (X)) + lrpad)
#endif // PANGO_PATCH
#if ALPHA_PATCH
#define OPAQUE                0xffU
#define OPACITY               "_NET_WM_WINDOW_OPACITY"
#endif // ALPHA_PATCH

/* enums */
enum {
	SchemeNorm,
	SchemeSel,
	SchemeOut,
	#if BORDER_PATCH
	SchemeBorder,
	#endif // BORDER_PATCH
	#if MORECOLOR_PATCH
	SchemeMid,
	#endif // MORECOLOR_PATCH
	#if HIGHLIGHT_PATCH
	SchemeNormHighlight,
	SchemeSelHighlight,
	#endif // HIGHLIGHT_PATCH
	#if HIGHPRIORITY_PATCH
	SchemeHp,
	#endif // HIGHPRIORITY_PATCH
	#if EMOJI_HIGHLIGHT_PATCH
	SchemeHover,
	SchemeGreen,
	SchemeYellow,
	SchemeBlue,
	SchemePurple,
	SchemeRed,
	#endif // EMOJI_HIGHLIGHT_PATCH
	SchemeLast,
}; /* color schemes */

struct item {
	char *text;
	#if SEPARATOR_PATCH
	char *text_output;
	#elif TSV_PATCH
	char *stext;
	#endif // SEPARATOR_PATCH | TSV_PATCH
	struct item *left, *right;
	#if NON_BLOCKING_STDIN_PATCH
	struct item *next;
	#endif // NON_BLOCKING_STDIN_PATCH
	#if MULTI_SELECTION_PATCH
	int id; /* for multiselect */
	#else
	int out;
	#endif // MULTI_SELECTION_PATCH
	#if HIGHPRIORITY_PATCH
	int hp;
	#endif // HIGHPRIORITY_PATCH
	#if FUZZYMATCH_PATCH
	double distance;
	#endif // FUZZYMATCH_PATCH
	#if PRINTINDEX_PATCH
	int index;
	#endif // PRINTINDEX_PATCH
};

static char text[BUFSIZ] = "";
#if PIPEOUT_PATCH
static char pipeout[8] = " | dmenu";
#endif // PIPEOUT_PATCH
static char *embed;
#if SEPARATOR_PATCH
static char separator;
static int separator_greedy;
static int separator_reverse;
#endif // SEPARATOR_PATCH
static int bh, mw, mh;
#if XYW_PATCH
static int dmx = 0, dmy = 0; /* put dmenu at these x and y offsets */
static unsigned int dmw = 0; /* make dmenu this wide */
#endif // XYW_PATCH
static int inputw = 0, promptw;
#if PASSWORD_PATCH
static int passwd = 0;
#endif // PASSWORD_PATCH
static int lrpad; /* sum of left and right padding */
#if BARPADDING_PATCH
static int vp; /* vertical padding for bar */
static int sp; /* side padding for bar */
#endif // BARPADDING_PATCH
#if REJECTNOMATCH_PATCH
static int reject_no_match = 0;
#endif // REJECTNOMATCH_PATCH
static size_t cursor;
static struct item *items = NULL;
static struct item *matches, *matchend;
static struct item *prev, *curr, *next, *sel;
static int mon = -1, screen;
#if PRINTINDEX_PATCH
static int print_index = 0;
#endif // PRINTINDEX_PATCH
#if MANAGED_PATCH
static int managed = 0;
#endif // MANAGED_PATCH
#if MULTI_SELECTION_PATCH
static int *selid = NULL;
static unsigned int selidsize = 0;
#endif // MULTI_SELECTION_PATCH
#if NO_SORT_PATCH
static unsigned int sortmatches = 1;
#endif // NO_SORT_PATCH
#if PRINTINPUTTEXT_PATCH
static int use_text_input = 0;
#endif // PRINTINPUTTEXT_PATCH
#if PRESELECT_PATCH
static unsigned int preselected = 0;
#endif // PRESELECT_PATCH
#if EMOJI_HIGHLIGHT_PATCH
static int commented = 0;
static int animated = 0;
#endif // EMOJI_HIGHLIGHT_PATCH

static Atom clip, utf8;
#if WMTYPE_PATCH
static Atom type, dock;
#endif // WMTYPE_PATCH
static Display *dpy;
static Window root, parentwin, win;
static XIC xic;

#if ALPHA_PATCH
static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;
#endif // ALPHA_PATCH

static Drw *drw;
static Clr *scheme[SchemeLast];

#include "patch/include.h"

#include "config.h"

static unsigned int
textw_clamp(const char *str, unsigned int n)
{
	unsigned int w = drw_fontset_getwidth_clamp(drw, str, n) + lrpad;
	return MIN(w, n);
}

static void appenditem(struct item *item, struct item **list, struct item **last);
static void calcoffsets(void);
static void cleanup(void);
static char * cistrstr(const char *s, const char *sub);
static int drawitem(struct item *item, int x, int y, int w);
static void drawmenu(void);
static void grabfocus(void);
static void grabkeyboard(void);
static void match(void);
static void insert(const char *str, ssize_t n);
static size_t nextrune(int inc);
static void movewordedge(int dir);
static void keypress(XKeyEvent *ev);
static void paste(void);
#if ALPHA_PATCH
static void xinitvisual(void);
#endif // ALPHA_PATCH
static void readstdin(void);
static void run(void);
static void setup(void);
static void usage(void);

#if CASEINSENSITIVE_PATCH
static int (*fstrncmp)(const char *, const char *, size_t) = strncasecmp;
static char *(*fstrstr)(const char *, const char *) = cistrstr;
#else
static int (*fstrncmp)(const char *, const char *, size_t) = strncmp;
static char *(*fstrstr)(const char *, const char *) = strstr;
#endif // CASEINSENSITIVE_PATCH

#include "patch/include.c"

static void
appenditem(struct item *item, struct item **list, struct item **last)
{
	if (*last)
		(*last)->right = item;
	else
		*list = item;

	item->left = *last;
	item->right = NULL;
	*last = item;
}

static void
calcoffsets(void)
{
	int i, n, rpad = 0;

	if (lines > 0) {
		#if GRID_PATCH
		if (columns)
			n = lines * columns * bh;
		else
			n = lines * bh;
		#else
		n = lines * bh;
		#endif // GRID_PATCH
	} else {
		#if NUMBERS_PATCH
		rpad = TEXTW(numbers);
		#endif // NUMBERS_PATCH
		#if SYMBOLS_PATCH
		n = mw - (promptw + inputw + TEXTW(symbol_1) + TEXTW(symbol_2) + rpad);
		#else
		n = mw - (promptw + inputw + TEXTW("<") + TEXTW(">") + rpad);
		#endif // SYMBOLS_PATCH
	}
	/* calculate which items will begin the next page and previous page */
	for (i = 0, next = curr; next; next = next->right)
		if ((i += (lines > 0) ? bh : textw_clamp(next->text, n)) > n)
			break;
	for (i = 0, prev = curr; prev && prev->left; prev = prev->left)
		if ((i += (lines > 0) ? bh : textw_clamp(prev->left->text, n)) > n)
			break;
}

static void
cleanup(void)
{
	size_t i;

	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	#if INPUTMETHOD_PATCH
	XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	#endif // INPUTMETHOD_PATCH
	for (i = 0; i < SchemeLast; i++)
		free(scheme[i]);
	for (i = 0; items && items[i].text; ++i)
		free(items[i].text);
	free(items);
	#if HIGHPRIORITY_PATCH
	for (i = 0; i < hplength; ++i)
		free(hpitems[i]);
	free(hpitems);
	#endif // HIGHPRIORITY_PATCH
	drw_free(drw);
	XSync(dpy, False);
	XCloseDisplay(dpy);
	#if MULTI_SELECTION_PATCH
	free(selid);
	#endif // MULTI_SELECTION_PATCH
}

static char *
cistrstr(const char *s, const char *sub)
{
	size_t len;

	for (len = strlen(sub); *s; s++)
		if (!strncasecmp(s, sub, len))
			return (char *)s;
	return NULL;
}

static int
drawitem(struct item *item, int x, int y, int w)
{
	int r;
	#if TSV_PATCH && !SEPARATOR_PATCH
	char *text = item->stext;
	#else
	char *text = item->text;
	#endif // TSV_PATCH

	#if EMOJI_HIGHLIGHT_PATCH
	int iscomment = 0;
	if (text[0] == '>') {
		if (text[1] == '>') {
			iscomment = 3;
			switch (text[2]) {
			case 'r':
				drw_setscheme(drw, scheme[SchemeRed]);
				break;
			case 'g':
				drw_setscheme(drw, scheme[SchemeGreen]);
				break;
			case 'y':
				drw_setscheme(drw, scheme[SchemeYellow]);
				break;
			case 'b':
				drw_setscheme(drw, scheme[SchemeBlue]);
				break;
			case 'p':
				drw_setscheme(drw, scheme[SchemePurple]);
				break;
			#if HIGHLIGHT_PATCH
			case 'h':
				drw_setscheme(drw, scheme[SchemeNormHighlight]);
				break;
			#endif // HIGHLIGHT_PATCH
			case 's':
				drw_setscheme(drw, scheme[SchemeSel]);
				break;
			default:
				iscomment = 1;
				drw_setscheme(drw, scheme[SchemeNorm]);
			break;
			}
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			iscomment = 1;
		}
	} else if (text[0] == ':') {
		iscomment = 2;
		if (item == sel) {
			switch (text[1]) {
			case 'r':
				drw_setscheme(drw, scheme[SchemeRed]);
				break;
			case 'g':
				drw_setscheme(drw, scheme[SchemeGreen]);
				break;
			case 'y':
				drw_setscheme(drw, scheme[SchemeYellow]);
				break;
			case 'b':
				drw_setscheme(drw, scheme[SchemeBlue]);
				break;
			case 'p':
				drw_setscheme(drw, scheme[SchemePurple]);
				break;
			#if HIGHLIGHT_PATCH
			case 'h':
				drw_setscheme(drw, scheme[SchemeNormHighlight]);
				break;
			#endif // HIGHLIGHT_PATCH
			case 's':
				drw_setscheme(drw, scheme[SchemeSel]);
				break;
			default:
				drw_setscheme(drw, scheme[SchemeSel]);
				iscomment = 0;
				break;
			}
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
		}
	}
	#endif // EMOJI_HIGHLIGHT_PATCH

	#if EMOJI_HIGHLIGHT_PATCH
	int temppadding = 0;
	if (iscomment == 2) {
		if (text[2] == ' ') {
			#if PANGO_PATCH
			temppadding = drw->font->h * 3;
			#else
			temppadding = drw->fonts->h * 3;
			#endif // PANGO_PATCH
			animated = 1;
			char dest[1000];
			strcpy(dest, text);
			dest[6] = '\0';
			drw_text(drw, x, y
				, temppadding
				#if LINE_HEIGHT_PATCH
				, MAX(lineheight, bh)
				#else
				, bh
				#endif // LINE_HEIGHT_PATCH
				, temppadding / 2.6
				, dest + 3
				, 0
				#if PANGO_PATCH
				, True
				#endif // PANGO_PATCH
			);
			iscomment = 6;
			drw_setscheme(drw, sel == item ? scheme[SchemeHover] : scheme[SchemeNorm]);
		}
	}

	char *output;
	if (commented) {
		static char onestr[2];
		onestr[0] = text[0];
		onestr[1] = '\0';
		output = onestr;
	} else {
		output = text;
	}
	#endif // EMOJI_HIGHLIGHT_PATCH

	if (item == sel)
		drw_setscheme(drw, scheme[SchemeSel]);
	#if HIGHPRIORITY_PATCH
	else if (item->hp)
		drw_setscheme(drw, scheme[SchemeHp]);
	#endif // HIGHPRIORITY_PATCH
	#if MORECOLOR_PATCH
	else if (item->left == sel || item->right == sel)
		drw_setscheme(drw, scheme[SchemeMid]);
	#endif // MORECOLOR_PATCH
	#if MULTI_SELECTION_PATCH
	else if (issel(item->id))
	#else
	else if (item->out)
	#endif // MULTI_SELECTION_PATCH
		drw_setscheme(drw, scheme[SchemeOut]);
	else
		drw_setscheme(drw, scheme[SchemeNorm]);

	r = drw_text(drw
		#if EMOJI_HIGHLIGHT_PATCH
		, x + ((iscomment == 6) ? temppadding : 0)
		#else
		, x
		#endif // EMOJI_HIGHLIGHT_PATCH
		, y
		, w
		, bh
		#if EMOJI_HIGHLIGHT_PATCH
		, commented ? (bh - TEXTW(output) - lrpad) / 2 : lrpad / 2
		#else
		, lrpad / 2
		#endif // EMOJI_HIGHLIGHT_PATCH
		#if EMOJI_HIGHLIGHT_PATCH
		, output + iscomment
		#else
		, text
		#endif // EMOJI_HIGHLIGHT_PATCH
		, 0
		#if PANGO_PATCH
		, True
		#endif // PANGO_PATCH
		);
	#if HIGHLIGHT_PATCH
	#if EMOJI_HIGHLIGHT_PATCH
	drawhighlights(item, output + iscomment, x + ((iscomment == 6) ? temppadding : 0), y, w);
	#else
	drawhighlights(item, x, y, w);
	#endif // EMOJI_HIGHLIGHT_PATCH
	#endif // HIGHLIGHT_PATCH
	return r;
}

static void
drawmenu(void)
{
	#if SCROLL_PATCH
	static int curpos, oldcurlen;
	int curlen, rcurlen;
	#else
	unsigned int curpos;
	#endif // SCROLL_PATCH
	struct item *item;
	int x = 0, y = 0, w, rpad = 0, itw = 0, stw = 0;
	#if LINE_HEIGHT_PATCH && PANGO_PATCH
	int fh = drw->font->h;
	#elif LINE_HEIGHT_PATCH
	int fh = drw->fonts->h;
	#endif // LINE_HEIGHT_PATCH
	#if PASSWORD_PATCH
	char *censort;
	#endif // PASSWORD_PATCH

	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, 0, 0, mw, mh, 1, 1);

	if (prompt && *prompt) {
		#if !PLAIN_PROMPT_PATCH
		drw_setscheme(drw, scheme[SchemeSel]);
		#endif // PLAIN_PROMPT_PATCH
		x = drw_text(drw, x, 0, promptw, bh, lrpad / 2, prompt, 0
			#if PANGO_PATCH
			, True
			#endif // PANGO_PATCH
		);
	}
	/* draw input field */
	w = (lines > 0 || !matches) ? mw - x : inputw;

	#if SCROLL_PATCH
	w -= lrpad / 2;
	x += lrpad / 2;
	rcurlen = TEXTW(text + cursor) - lrpad;
	curlen = TEXTW(text) - lrpad - rcurlen;
	curpos += curlen - oldcurlen;
	curpos = MIN(w, MAX(0, curpos));
	curpos = MAX(curpos, w - rcurlen);
	curpos = MIN(curpos, curlen);
	oldcurlen = curlen;

	drw_setscheme(drw, scheme[SchemeNorm]);
	#if PASSWORD_PATCH
	if (passwd) {
		censort = ecalloc(1, sizeof(text));
		memset(censort, '.', strlen(text));
		drw_text_align(drw, x, 0, curpos, bh, censort, cursor, AlignR);
		drw_text_align(drw, x + curpos, 0, w - curpos, bh, censort + cursor, strlen(censort) - cursor, AlignL);
		free(censort);
	} else {
		drw_text_align(drw, x, 0, curpos, bh, text, cursor, AlignR);
		drw_text_align(drw, x + curpos, 0, w - curpos, bh, text + cursor, strlen(text) - cursor, AlignL);
	}
	#else
	drw_text_align(drw, x, 0, curpos, bh, text, cursor, AlignR);
	drw_text_align(drw, x + curpos, 0, w - curpos, bh, text + cursor, strlen(text) - cursor, AlignL);
	#endif // PASSWORD_PATCH
	#if LINE_HEIGHT_PATCH
	drw_rect(drw, x + curpos - 1, 2 + (bh-fh)/2, 2, fh - 4, 1, 0);
	#else
	drw_rect(drw, x + curpos - 1, 2, 2, bh - 4, 1, 0);
	#endif // LINE_HEIGHT_PATCH
	#else // !SCROLL_PATCH
	drw_setscheme(drw, scheme[SchemeNorm]);
	#if PASSWORD_PATCH
	if (passwd) {
		censort = ecalloc(1, sizeof(text));
		memset(censort, '.', strlen(text));
		drw_text(drw, x, 0, w, bh, lrpad / 2, censort, 0
			#if PANGO_PATCH
			, False
			#endif // PANGO_PATCH
		);
		drw_text(drw, x, 0, w, bh, lrpad / 2, censort, 0
			#if PANGO_PATCH
			, False
			#endif // PANGO_PATCH
		);
		free(censort);
	} else {
		drw_text(drw, x, 0, w, bh, lrpad / 2, text, 0
			#if PANGO_PATCH
			, False
			#endif // PANGO_PATCH
		);
	}
	#else
	drw_text(drw, x, 0, w, bh, lrpad / 2, text, 0
		#if PANGO_PATCH
		, False
		#endif // PANGO_PATCH
	);
	#endif // PASSWORD_PATCH

	curpos = TEXTW(text) - TEXTW(&text[cursor]);
	if ((curpos += lrpad / 2 - 1) < w) {
		drw_setscheme(drw, scheme[SchemeNorm]);
		#if CARET_WIDTH_PATCH && LINE_HEIGHT_PATCH
		drw_rect(drw, x + curpos, 2 + (bh-fh)/2, caret_width, fh - 4, 1, 0);
		#elif CARET_WIDTH_PATCH
		drw_rect(drw, x + curpos, 2, caret_width, bh - 4, 1, 0);
		#elif LINE_HEIGHT_PATCH
		drw_rect(drw, x + curpos, 2 + (bh-fh)/2, 2, fh - 4, 1, 0);
		#else
		drw_rect(drw, x + curpos, 2, 2, bh - 4, 1, 0);
		#endif // LINE_HEIGHT_PATCH
	}
	#endif // SCROLL_PATCH

	#if NUMBERS_PATCH
	recalculatenumbers();
	rpad = TEXTW(numbers);
	#if BARPADDING_PATCH
	rpad += 2 * sp;
	#endif // BARPADDING_PATCH
	#if BORDER_PATCH
	rpad += border_width;
	#endif // BORDER_PATCH
	#endif // NUMBERS_PATCH
	if (lines > 0) {
		#if GRID_PATCH
		/* draw grid */
		int i = 0;
		for (item = curr; item != next; item = item->right, i++)
			if (columns)
				#if VERTFULL_PATCH
				drawitem(
					item,
					0 + ((i / lines) *  (mw / columns)),
					y + (((i % lines) + 1) * bh),
					mw / columns
				);
				#else
				drawitem(
					item,
					x + ((i / lines) *  ((mw - x) / columns)),
					y + (((i % lines) + 1) * bh),
					(mw - x) / columns
				);
				#endif // VERTFULL_PATCH
			else
				#if VERTFULL_PATCH
				drawitem(item, 0, y += bh, mw);
				#else
				drawitem(item, x, y += bh, mw - x);
				#endif // VERTFULL_PATCH
		#else
		/* draw vertical list */
		for (item = curr; item != next; item = item->right)
			#if VERTFULL_PATCH
			drawitem(item, 0, y += bh, mw);
			#else
			drawitem(item, x, y += bh, mw - x);
			#endif // VERTFULL_PATCH
		#endif // GRID_PATCH
	} else if (matches) {
		/* draw horizontal list */
		x += inputw;
		#if SYMBOLS_PATCH
		w = TEXTW(symbol_1);
		#else
		w = TEXTW("<");
		#endif // SYMBOLS_PATCH
		if (curr->left) {
			drw_setscheme(drw, scheme[SchemeNorm]);
			#if SYMBOLS_PATCH
			drw_text(drw, x, 0, w, bh, lrpad / 2, symbol_1, 0
			#else
			drw_text(drw, x, 0, w, bh, lrpad / 2, "<", 0
			#endif // SYMBOLS_PATCH
				#if PANGO_PATCH
				, True
				#endif // PANGO_PATCH
			);
		}
		x += w;
		for (item = curr; item != next; item = item->right) {
			#if SYMBOLS_PATCH
			stw = TEXTW(symbol_2);
			#else
			stw = TEXTW(">");
			#endif // SYMBOLS_PATCH
			#if TSV_PATCH && !SEPARATOR_PATCH
			itw = textw_clamp(item->stext, mw - x - stw - rpad);
			#else
			itw = textw_clamp(item->text, mw - x - stw - rpad);
			#endif // TSV_PATCH
			x = drawitem(item, x, 0, itw);
		}
		if (next) {
			#if SYMBOLS_PATCH
			w = TEXTW(symbol_2);
			#else
			w = TEXTW(">");
			#endif // SYMBOLS_PATCH
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_text(drw, mw - w - rpad, 0, w, bh, lrpad / 2
				#if SYMBOLS_PATCH
				, symbol_2
				#else
				, ">"
				#endif // SYMBOLS_PATCH
				, 0
				#if PANGO_PATCH
				, True
				#endif // PANGO_PATCH
			);
		}
	}
	#if NUMBERS_PATCH
	drw_setscheme(drw, scheme[SchemeNorm]);
	#if PANGO_PATCH
	drw_text(drw, mw - rpad, 0, TEXTW(numbers), bh, lrpad / 2, numbers, 0, False);
	#else
	drw_text(drw, mw - rpad, 0, TEXTW(numbers), bh, lrpad / 2, numbers, 0);
	#endif // PANGO_PATCH
	#endif // NUMBERS_PATCH
	drw_map(drw, win, 0, 0, mw, mh);
	#if NON_BLOCKING_STDIN_PATCH
	XFlush(dpy);
	#endif // NON_BLOCKING_STDIN_PATCH
}

static void
grabfocus(void)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000  };
	Window focuswin;
	int i, revertwin;

	for (i = 0; i < 100; ++i) {
		XGetInputFocus(dpy, &focuswin, &revertwin);
		if (focuswin == win)
			return;
		#if !MANAGED_PATCH
		XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
		#endif // MANAGED_PATCH
		nanosleep(&ts, NULL);
	}
	die("cannot grab focus");
}

static void
grabkeyboard(void)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000  };
	int i;

	#if MANAGED_PATCH
	if (embed || managed)
	#else
	if (embed)
	#endif // MANAGED_PATCH
		return;
	/* try to grab keyboard, we may have to wait for another process to ungrab */
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy, DefaultRootWindow(dpy), True, GrabModeAsync,
		                  GrabModeAsync, CurrentTime) == GrabSuccess)
			return;
		nanosleep(&ts, NULL);
	}
	die("cannot grab keyboard");
}

static void
match(void)
{
	#if DYNAMIC_OPTIONS_PATCH
	if (dynamic && *dynamic)
		refreshoptions();
	#endif // DYNAMIC_OPTIONS_PATCH

	#if FUZZYMATCH_PATCH
	if (fuzzy) {
		fuzzymatch();
		return;
	}
	#endif
	static char **tokv = NULL;
	static int tokn = 0;

	char buf[sizeof text], *s;
	int i, tokc = 0;
	size_t len, textsize;
	struct item *item, *lprefix, *lsubstr, *prefixend, *substrend;
	#if HIGHPRIORITY_PATCH
	struct item *lhpprefix, *hpprefixend;
	#endif // HIGHPRIORITY_PATCH
	#if NON_BLOCKING_STDIN_PATCH
	int preserve = 0;
	#endif // NON_BLOCKING_STDIN_PATCH

	strcpy(buf, text);
	/* separate input text into tokens to be matched individually */
	for (s = strtok(buf, " "); s; tokv[tokc - 1] = s, s = strtok(NULL, " "))
		if (++tokc > tokn && !(tokv = realloc(tokv, ++tokn * sizeof *tokv)))
			die("cannot realloc %zu bytes:", tokn * sizeof *tokv);
	len = tokc ? strlen(tokv[0]) : 0;

	#if PREFIXCOMPLETION_PATCH
	if (use_prefix) {
		matches = lprefix = matchend = prefixend = NULL;
		textsize = strlen(text);
	} else {
		matches = lprefix = lsubstr = matchend = prefixend = substrend = NULL;
		textsize = strlen(text) + 1;
	}
	#else
	matches = lprefix = lsubstr = matchend = prefixend = substrend = NULL;
	textsize = strlen(text) + 1;
	#endif // PREFIXCOMPLETION_PATCH
	#if HIGHPRIORITY_PATCH
	lhpprefix = hpprefixend = NULL;
	#endif // HIGHPRIORITY_PATCH
	#if NON_BLOCKING_STDIN_PATCH && DYNAMIC_OPTIONS_PATCH
	for (item = items; item && (!(dynamic && *dynamic) || item->text); item = (dynamic && *dynamic) ? item + 1 : item->next)
	#elif NON_BLOCKING_STDIN_PATCH
	for (item = items; item; item = item->next)
	#else
	for (item = items; item && item->text; item++)
	#endif
	{
		for (i = 0; i < tokc; i++)
			if (!fstrstr(item->text, tokv[i]))
				break;
		#if DYNAMIC_OPTIONS_PATCH
		if (i != tokc && !(dynamic && *dynamic)) /* not all tokens match */
			continue;
		#else
		if (i != tokc) /* not all tokens match */
			continue;
		#endif // DYNAMIC_OPTIONS_PATCH
		#if HIGHPRIORITY_PATCH
		/* exact matches go first, then prefixes with high priority, then prefixes, then substrings */
		#else
		/* exact matches go first, then prefixes, then substrings */
		#endif // HIGHPRIORITY_PATCH
		#if NO_SORT_PATCH
		if (!sortmatches)
 			appenditem(item, &matches, &matchend);
 		else
		#endif // NO_SORT_PATCH
		if (!tokc || !fstrncmp(text, item->text, textsize))
			appenditem(item, &matches, &matchend);
		#if HIGHPRIORITY_PATCH
		else if (item->hp && !fstrncmp(tokv[0], item->text, len))
			appenditem(item, &lhpprefix, &hpprefixend);
		#endif // HIGHPRIORITY_PATCH
		else if (!fstrncmp(tokv[0], item->text, len))
			appenditem(item, &lprefix, &prefixend);
		#if PREFIXCOMPLETION_PATCH
		else if (!use_prefix)
		#else
		else
		#endif // PREFIXCOMPLETION_PATCH
			appenditem(item, &lsubstr, &substrend);
		#if NON_BLOCKING_STDIN_PATCH
		if (sel == item)
			preserve = 1;
		#endif // NON_BLOCKING_STDIN_PATCH
	}
	#if HIGHPRIORITY_PATCH
	if (lhpprefix) {
		if (matches) {
			matchend->right = lhpprefix;
			lhpprefix->left = matchend;
		} else
			matches = lhpprefix;
		matchend = hpprefixend;
	}
	#endif // HIGHPRIORITY_PATCH
	if (lprefix) {
		if (matches) {
			matchend->right = lprefix;
			lprefix->left = matchend;
		} else
			matches = lprefix;
		matchend = prefixend;
	}
	#if PREFIXCOMPLETION_PATCH
	if (!use_prefix && lsubstr)
	#else
	if (lsubstr)
	#endif // PREFIXCOMPLETION_PATCH
	{
		if (matches) {
			matchend->right = lsubstr;
			lsubstr->left = matchend;
		} else
			matches = lsubstr;
		matchend = substrend;
	}
	#if NON_BLOCKING_STDIN_PATCH
	if (!preserve)
	#endif // NON_BLOCKING_STDIN_PATCH
	curr = sel = matches;

	#if INSTANT_PATCH
	if (instant && matches && matches==matchend && !lsubstr) {
		puts(matches->text);
		cleanup();
		exit(0);
	}
	#endif // INSTANT_PATCH

	calcoffsets();
}

static void
insert(const char *str, ssize_t n)
{
	if (strlen(text) + n > sizeof text - 1)
		return;

	#if REJECTNOMATCH_PATCH
	static char last[BUFSIZ] = "";
	if (reject_no_match) {
		/* store last text value in case we need to revert it */
		memcpy(last, text, BUFSIZ);
	}
	#endif // REJECTNOMATCH_PATCH

	/* move existing text out of the way, insert new text, and update cursor */
	memmove(&text[cursor + n], &text[cursor], sizeof text - cursor - MAX(n, 0));
	if (n > 0)
		memcpy(&text[cursor], str, n);
	cursor += n;
	match();

	#if REJECTNOMATCH_PATCH
	if (!matches && reject_no_match) {
		/* revert to last text value if theres no match */
		memcpy(text, last, BUFSIZ);
		cursor -= n;
		match();
	}
	#endif // REJECTNOMATCH_PATCH
}

static size_t
nextrune(int inc)
{
	ssize_t n;

	/* return location of next utf8 rune in the given direction (+1 or -1) */
	for (n = cursor + inc; n + inc >= 0 && (text[n] & 0xc0) == 0x80; n += inc)
		;
	return n;
}

static void
movewordedge(int dir)
{
	if (dir < 0) { /* move cursor to the start of the word*/
		while (cursor > 0 && strchr(worddelimiters, text[nextrune(-1)]))
			cursor = nextrune(-1);
		while (cursor > 0 && !strchr(worddelimiters, text[nextrune(-1)]))
			cursor = nextrune(-1);
	} else { /* move cursor to the end of the word */
		while (text[cursor] && strchr(worddelimiters, text[cursor]))
			cursor = nextrune(+1);
		while (text[cursor] && !strchr(worddelimiters, text[cursor]))
			cursor = nextrune(+1);
	}
}

static void
keypress(XKeyEvent *ev)
{
	char buf[64];
	int len;
	#if PREFIXCOMPLETION_PATCH
	struct item * item;
	#endif // PREFIXCOMPLETION_PATCH
	KeySym ksym = NoSymbol;
	Status status;
	#if GRID_PATCH && GRIDNAV_PATCH
	int i;
	struct item *tmpsel;
	bool offscreen = false;
	#endif // GRIDNAV_PATCH

	len = XmbLookupString(xic, ev, buf, sizeof buf, &ksym, &status);
	switch (status) {
	default: /* XLookupNone, XBufferOverflow */
		return;
	case XLookupChars: /* composed string from input method */
		goto insert;
	case XLookupKeySym:
	case XLookupBoth: /* a KeySym and a string are returned: use keysym */
		break;
	}

	if (ev->state & ControlMask) {
		switch(ksym) {
		#if FZFEXPECT_PATCH
		case XK_a: expect("ctrl-a", ev); ksym = XK_Home;      break;
		case XK_b: expect("ctrl-b", ev); ksym = XK_Left;      break;
		case XK_c: expect("ctrl-c", ev); ksym = XK_Escape;    break;
		case XK_d: expect("ctrl-d", ev); ksym = XK_Delete;    break;
		case XK_e: expect("ctrl-e", ev); ksym = XK_End;       break;
		case XK_f: expect("ctrl-f", ev); ksym = XK_Right;     break;
		case XK_g: expect("ctrl-g", ev); ksym = XK_Escape;    break;
		case XK_h: expect("ctrl-h", ev); ksym = XK_BackSpace; break;
		case XK_i: expect("ctrl-i", ev); ksym = XK_Tab;       break;
		case XK_j: expect("ctrl-j", ev); ksym = XK_Down;      break;
		case XK_J:/* fallthrough */
		case XK_l: expect("ctrl-l", ev); break;
		case XK_m: expect("ctrl-m", ev); /* fallthrough */
		case XK_M: ksym = XK_Return; ev->state &= ~ControlMask; break;
		case XK_n: expect("ctrl-n", ev); ksym = XK_Down; break;
		case XK_p: expect("ctrl-p", ev); ksym = XK_Up;   break;
		case XK_o: expect("ctrl-o", ev); break;
		case XK_q: expect("ctrl-q", ev); break;
		case XK_r: expect("ctrl-r", ev); break;
		case XK_s: expect("ctrl-s", ev); break;
		case XK_t: expect("ctrl-t", ev); break;
		case XK_k: expect("ctrl-k", ev); ksym = XK_Up; break;
		#else
		case XK_a: ksym = XK_Home;      break;
		case XK_b: ksym = XK_Left;      break;
		case XK_c: ksym = XK_Escape;    break;
		case XK_d: ksym = XK_Delete;    break;
		case XK_e: ksym = XK_End;       break;
		case XK_f: ksym = XK_Right;     break;
		case XK_g: ksym = XK_Escape;    break;
		case XK_h: ksym = XK_BackSpace; break;
		case XK_i: ksym = XK_Tab;       break;
		case XK_j: /* fallthrough */
		case XK_J: /* fallthrough */
		case XK_m: /* fallthrough */
		case XK_M: ksym = XK_Return; ev->state &= ~ControlMask; break;
		case XK_n: ksym = XK_Down;      break;
		case XK_p: ksym = XK_Up;        break;

		case XK_k: /* delete right */
			text[cursor] = '\0';
			match();
			break;
		#endif // FZFEXPECT_PATCH
		#if FZFEXPECT_PATCH
		case XK_u: expect("ctrl-u", ev); /* delete left */
		#else
		case XK_u: /* delete left */
		#endif // FZFEXPECT_PATCH
			insert(NULL, 0 - cursor);
			break;
		#if FZFEXPECT_PATCH
		case XK_w: expect("ctrl-w", ev); /* delete word */
		#else
		case XK_w: /* delete word */
		#endif // FZFEXPECT_PATCH
			while (cursor > 0 && strchr(worddelimiters, text[nextrune(-1)]))
				insert(NULL, nextrune(-1) - cursor);
			while (cursor > 0 && !strchr(worddelimiters, text[nextrune(-1)]))
				insert(NULL, nextrune(-1) - cursor);
			break;
		#if FZFEXPECT_PATCH || CTRL_V_TO_PASTE_PATCH
		case XK_v:
		#if FZFEXPECT_PATCH
			expect("ctrl-v", ev);
		#endif // FZFEXPECT_PATCH
		case XK_V:
			XConvertSelection(dpy, (ev->state & ShiftMask) ? clip : XA_PRIMARY,
			                  utf8, utf8, win, CurrentTime);
			return;
		#endif // FZFEXPECT_PATCH | CTRL_V_TO_PASTE_PATCH
		#if FZFEXPECT_PATCH
		case XK_y: expect("ctrl-y", ev); /* paste selection */
		#else
		case XK_y: /* paste selection */
		#endif // FZFEXPECT_PATCH
		case XK_Y:
			XConvertSelection(dpy, (ev->state & ShiftMask) ? clip : XA_PRIMARY,
			                  utf8, utf8, win, CurrentTime);
			return;
		#if FZFEXPECT_PATCH
		case XK_x: expect("ctrl-x", ev); break;
		case XK_z: expect("ctrl-z", ev); break;
		#endif // FZFEXPECT_PATCH
		case XK_Left:
		case XK_KP_Left:
			movewordedge(-1);
			goto draw;
		case XK_Right:
		case XK_KP_Right:
			movewordedge(+1);
			goto draw;
		case XK_Return:
		case XK_KP_Enter:
			#if RESTRICT_RETURN_PATCH
			if (restrict_return)
				break;
			#endif // RESTRICT_RETURN_PATCH
			#if MULTI_SELECTION_PATCH
			selsel();
			#endif // MULTI_SELECTION_PATCH
			break;
		case XK_bracketleft:
			cleanup();
			exit(1);
		default:
			return;
		}
	} else if (ev->state & Mod1Mask) {
		switch(ksym) {
		case XK_b:
			movewordedge(-1);
			goto draw;
		case XK_f:
			movewordedge(+1);
			goto draw;
		case XK_g: ksym = XK_Home;  break;
		case XK_G: ksym = XK_End;   break;
		case XK_h: ksym = XK_Up;    break;
		case XK_j: ksym = XK_Next;  break;
		case XK_k: ksym = XK_Prior; break;
		case XK_l: ksym = XK_Down;  break;
		#if NAVHISTORY_PATCH
		case XK_p:
			navhistory(-1);
			buf[0]=0;
			break;
		case XK_n:
			navhistory(1);
			buf[0]=0;
			break;
		#endif // NAVHISTORY_PATCH
		default:
			return;
		}
	}

	switch(ksym) {
	default:
insert:
		if (!iscntrl((unsigned char)*buf))
			insert(buf, len);
		break;
	case XK_Delete:
	case XK_KP_Delete:
		if (text[cursor] == '\0')
			return;
		cursor = nextrune(+1);
		/* fallthrough */
	case XK_BackSpace:
		if (cursor == 0)
			return;
		insert(NULL, nextrune(-1) - cursor);
		break;
	case XK_End:
	case XK_KP_End:
		if (text[cursor] != '\0') {
			cursor = strlen(text);
			break;
		}
		if (next) {
			/* jump to end of list and position items in reverse */
			curr = matchend;
			calcoffsets();
			curr = prev;
			calcoffsets();
			while (next && (curr = curr->right))
				calcoffsets();
		}
		sel = matchend;
		break;
	case XK_Escape:
		cleanup();
		exit(1);
	case XK_Home:
	case XK_KP_Home:
		if (sel == matches) {
			cursor = 0;
			break;
		}
		sel = curr = matches;
		calcoffsets();
		break;
	case XK_Left:
	case XK_KP_Left:
		#if GRID_PATCH && GRIDNAV_PATCH
		if (columns > 1) {
			if (!sel)
				return;
			tmpsel = sel;
			for (i = 0; i < lines; i++) {
				if (!tmpsel->left ||  tmpsel->left->right != tmpsel)
					return;
				if (tmpsel == curr)
					offscreen = true;
				tmpsel = tmpsel->left;
			}
			sel = tmpsel;
			if (offscreen) {
				curr = prev;
				calcoffsets();
			}
			break;
		}
		#endif // GRIDNAV_PATCH
		if (cursor > 0 && (!sel || !sel->left || lines > 0)) {
			cursor = nextrune(-1);
			break;
		}
		if (lines > 0)
			return;
		/* fallthrough */
	case XK_Up:
	case XK_KP_Up:
		if (sel && sel->left && (sel = sel->left)->right == curr) {
			curr = prev;
			calcoffsets();
		}
		break;
	case XK_Next:
	case XK_KP_Next:
		if (!next)
			return;
		sel = curr = next;
		calcoffsets();
		break;
	case XK_Prior:
	case XK_KP_Prior:
		if (!prev)
			return;
		sel = curr = prev;
		calcoffsets();
		break;
	case XK_Return:
	case XK_KP_Enter:
		#if RESTRICT_RETURN_PATCH
		if (restrict_return && (!sel || ev->state & (ShiftMask | ControlMask)))
			break;
		#endif // RESTRICT_RETURN_PATCH
		#if !MULTI_SELECTION_PATCH
		#if PIPEOUT_PATCH
		#if PRINTINPUTTEXT_PATCH
		if (sel && (
			(use_text_input && (ev->state & ShiftMask)) ||
			(!use_text_input && !(ev->state & ShiftMask))
		))
		#else
		if (sel && !(ev->state & ShiftMask))
		#endif // PRINTINPUTTEXT_PATCH
		{
			if (sel->text[0] == startpipe[0]) {
				strncpy(sel->text + strlen(sel->text),pipeout,8);
				puts(sel->text+1);
			}
			#if PRINTINDEX_PATCH
			if (print_index)
				printf("%d\n", sel->index);
			else
			#endif // PRINTINDEX_PATCH
			puts(sel->text);
		} else {
			if (text[0] == startpipe[0]) {
				strncpy(text + strlen(text),pipeout,8);
				puts(text+1);
			}
			puts(text);
		}
		#elif PRINTINPUTTEXT_PATCH
		if (use_text_input)
			puts((sel && (ev->state & ShiftMask)) ? sel->text : text);
		#if PRINTINDEX_PATCH
		else if (print_index)
			printf("%d\n", (sel && !(ev->state & ShiftMask)) ? sel->index : -1);
		#endif // PRINTINDEX_PATCH
		else
			#if SEPARATOR_PATCH
			puts((sel && !(ev->state & ShiftMask)) ? sel->text_output : text);
			#else
			puts((sel && !(ev->state & ShiftMask)) ? sel->text : text);
			#endif // SEPARATOR_PATCH
		#elif PRINTINDEX_PATCH
		if (print_index)
			printf("%d\n", (sel && !(ev->state & ShiftMask)) ? sel->index : -1);
		else
			#if SEPARATOR_PATCH
			puts((sel && !(ev->state & ShiftMask)) ? sel->text_output : text);
			#else
			puts((sel && !(ev->state & ShiftMask)) ? sel->text : text);
			#endif // SEPARATOR_PATCH
		#elif SEPARATOR_PATCH
		puts((sel && !(ev->state & ShiftMask)) ? sel->text_output : text);
		#else
		puts((sel && !(ev->state & ShiftMask)) ? sel->text : text);
		#endif // PIPEOUT_PATCH | PRINTINPUTTEXT_PATCH | PRINTINDEX_PATCH
		#endif // MULTI_SELECTION_PATCH
		if (!(ev->state & ControlMask)) {
			#if NAVHISTORY_PATCH
			savehistory((sel && !(ev->state & ShiftMask))
				    ? sel->text : text);
			#endif // NAVHISTORY_PATCH
			#if MULTI_SELECTION_PATCH
			printsel(ev->state);
			#endif // MULTI_SELECTION_PATCH
			cleanup();
			exit(0);
		}
		#if !MULTI_SELECTION_PATCH
		if (sel)
			sel->out = 1;
		#endif // MULTI_SELECTION_PATCH
		break;
	case XK_Right:
	case XK_KP_Right:
		#if GRID_PATCH && GRIDNAV_PATCH
		if (columns > 1) {
			if (!sel)
				return;
			tmpsel = sel;
			for (i = 0; i < lines; i++) {
				if (!tmpsel->right ||  tmpsel->right->left != tmpsel)
					return;
				tmpsel = tmpsel->right;
				if (tmpsel == next)
					offscreen = true;
			}
			sel = tmpsel;
			if (offscreen) {
				curr = next;
				calcoffsets();
			}
			break;
		}
		#endif // GRIDNAV_PATCH
		if (text[cursor] != '\0') {
			cursor = nextrune(+1);
			break;
		}
		if (lines > 0)
			return;
		/* fallthrough */
	case XK_Down:
	case XK_KP_Down:
		if (sel && sel->right && (sel = sel->right) == next) {
			curr = next;
			calcoffsets();
		}
		break;
	case XK_Tab:
		#if PREFIXCOMPLETION_PATCH
		if (!matches)
			break; /* cannot complete no matches */
		#if FUZZYMATCH_PATCH
		/* only do tab completion if all matches start with prefix */
		for (item = matches; item && item->text; item = item->right)
			if (item->text[0] != text[0])
				goto draw;
		#endif // FUZZYMATCH_PATCH
		strncpy(text, matches->text, sizeof text - 1);
		text[sizeof text - 1] = '\0';
		len = cursor = strlen(text); /* length of longest common prefix */
		for (item = matches; item && item->text; item = item->right) {
			cursor = 0;
			while (cursor < len && text[cursor] == item->text[cursor])
				cursor++;
			len = cursor;
		}
		memset(text + len, '\0', strlen(text) - len);
		#else
		if (!sel)
			return;
		cursor = strnlen(sel->text, sizeof text - 1);
		memcpy(text, sel->text, cursor);
		text[cursor] = '\0';
		match();
		#endif // PREFIXCOMPLETION_PATCH
		break;
	}

draw:
	#if INCREMENTAL_PATCH
	if (incremental) {
		puts(text);
		fflush(stdout);
	}
	#endif // INCREMENTAL_PATCH
	drawmenu();
}

static void
paste(void)
{
	char *p, *q;
	int di;
	unsigned long dl;
	Atom da;

	/* we have been given the current selection, now insert it into input */
	if (XGetWindowProperty(dpy, win, utf8, 0, (sizeof text / 4) + 1, False,
	                   utf8, &da, &di, &dl, &dl, (unsigned char **)&p)
	    == Success && p) {
		insert(p, (q = strchr(p, '\n')) ? q - p : (ssize_t)strlen(p));
		XFree(p);
	}
	drawmenu();
}

#if ALPHA_PATCH
static void
xinitvisual(void)
{
	XVisualInfo *infos;
	XRenderPictFormat *fmt;
	int nitems;
	int i;

	XVisualInfo tpl = {
		.screen = screen,
		.depth = 32,
		.class = TrueColor
	};
	long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

	infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
	visual = NULL;
	for(i = 0; i < nitems; i ++) {
		fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = infos[i].visual;
			depth = infos[i].depth;
			cmap = XCreateColormap(dpy, root, visual, AllocNone);
			useargb = 1;
			break;
		}
	}

	XFree(infos);

	if (!visual || !opacity) {
		visual = DefaultVisual(dpy, screen);
		depth = DefaultDepth(dpy, screen);
		cmap = DefaultColormap(dpy, screen);
	}
}
#endif // ALPHA_PATCH

#if !NON_BLOCKING_STDIN_PATCH
static void
readstdin(void)
{
	char *line = NULL;
	#if SEPARATOR_PATCH
	char *p;
	#elif TSV_PATCH
	char *buf, *p;
	#endif // SEPARATOR_PATCH | TSV_PATCH

	size_t i, linesiz, itemsiz = 0;
	ssize_t len;

	#if PASSWORD_PATCH
	if (passwd) {
		inputw = lines = 0;
		return;
	}
	#endif // PASSWORD_PATCH

	/* read each line from stdin and add it to the item list */
	for (i = 0; (len = getline(&line, &linesiz, stdin)) != -1; i++) {
		if (i + 1 >= itemsiz) {
			itemsiz += 256;
			if (!(items = realloc(items, itemsiz * sizeof(*items))))
				die("cannot realloc %zu bytes:", itemsiz * sizeof(*items));
		}
		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		if (!(items[i].text = strdup(line)))
			die("strdup:");
		#if SEPARATOR_PATCH
		if (separator && (p = separator_greedy ?
			strrchr(items[i].text, separator) : strchr(items[i].text, separator))) {
			*p = '\0';
			items[i].text_output = ++p;
		} else {
			items[i].text_output = items[i].text;
		}
		if (separator_reverse) {
			p = items[i].text;
			items[i].text = items[i].text_output;
			items[i].text_output = p;
		}
		#elif TSV_PATCH
		if (!(buf = strdup(line)))
			die("cannot strdup %u bytes:", strlen(line) + 1);
		if ((p = strchr(buf, '\t')))
			*p = '\0';
		items[i].stext = buf;
		#endif // SEPARATOR_PATCH | TSV_PATCH
		#if MULTI_SELECTION_PATCH
		items[i].id = i; /* for multiselect */
		#if PRINTINDEX_PATCH
		items[i].index = i;
		#endif // PRINTINDEX_PATCH
		#elif PRINTINDEX_PATCH
		items[i].index = i;
		#else
		items[i].out = 0;
		#endif // MULTI_SELECTION_PATCH | PRINTINDEX_PATCH

		#if HIGHPRIORITY_PATCH
		items[i].hp = arrayhas(hpitems, hplength, items[i].text);
		#endif // HIGHPRIORITY_PATCH
	}
	free(line);
	if (items)
		items[i].text = NULL;
	lines = MIN(lines, i);
}
#endif // NON_BLOCKING_STDIN_PATCH

static void
#if NON_BLOCKING_STDIN_PATCH
readevent(void)
#else
run(void)
#endif // NON_BLOCKING_STDIN_PATCH
{
	XEvent ev;
	#if PRESELECT_PATCH
	int i;
	#endif // PRESELECT_PATCH
	while (!XNextEvent(dpy, &ev)) {
		#if PRESELECT_PATCH
		if (preselected) {
			for (i = 0; i < preselected; i++) {
				if (sel && sel->right && (sel = sel->right) == next) {
					curr = next;
					calcoffsets();
				}
			}
			drawmenu();
			preselected = 0;
		}
		#endif // PRESELECT_PATCH
		#if INPUTMETHOD_PATCH
		if (XFilterEvent(&ev, None))
			continue;
		if (composing)
			continue;
		#else
		if (XFilterEvent(&ev, win))
			continue;
		#endif // INPUTMETHOD_PATCH
		switch(ev.type) {
		#if MOUSE_SUPPORT_PATCH
		case ButtonPress:
			buttonpress(&ev);
			break;
		#if MOTION_SUPPORT_PATCH
		case MotionNotify:
			motionevent(&ev.xbutton);
			break;
		#endif // MOTION_SUPPORT_PATCH
		#endif // MOUSE_SUPPORT_PATCH
		case DestroyNotify:
			if (ev.xdestroywindow.window != win)
				break;
			cleanup();
			exit(1);
		case Expose:
			if (ev.xexpose.count == 0)
				drw_map(drw, win, 0, 0, mw, mh);
			break;
		case FocusIn:
			/* regrab focus from parent window */
			if (ev.xfocus.window != win)
				grabfocus();
			break;
		case KeyPress:
			keypress(&ev.xkey);
			break;
		case SelectionNotify:
			if (ev.xselection.property == utf8)
				paste();
			break;
		case VisibilityNotify:
			if (ev.xvisibility.state != VisibilityUnobscured)
				XRaiseWindow(dpy, win);
			break;
		}
	}
}

static void
setup(void)
{
	int x, y, i, j;
	unsigned int du;
	#if RELATIVE_INPUT_WIDTH_PATCH
	unsigned int tmp, minstrlen = 0, curstrlen = 0;
	int numwidthchecks = 100;
	struct item *item;
	#endif // RELATIVE_INPUT_WIDTH_PATCH
	XSetWindowAttributes swa;
	XIM xim;
	Window w, dw, *dws;
	XWindowAttributes wa;
	XClassHint ch = {"dmenu", "dmenu"};
#ifdef XINERAMA
	XineramaScreenInfo *info;
	Window pw;
	int a, di, n, area = 0;
#endif
	/* init appearance */
	#if XRESOURCES_PATCH
	for (j = 0; j < SchemeLast; j++)
		#if ALPHA_PATCH
		scheme[j] = drw_scm_create(drw, (const char**)colors[j], alphas[j], 2);
		#else
		scheme[j] = drw_scm_create(drw, (const char**)colors[j], 2);
		#endif // ALPHA_PATCH
	#else
	for (j = 0; j < SchemeLast; j++)
		#if ALPHA_PATCH
		scheme[j] = drw_scm_create(drw, colors[j], alphas[j], 2);
		#else
		scheme[j] = drw_scm_create(drw, colors[j], 2);
		#endif // ALPHA_PATCH
	#endif // XRESOURCES_PATCH

	clip = XInternAtom(dpy, "CLIPBOARD",   False);
	utf8 = XInternAtom(dpy, "UTF8_STRING", False);
	#if WMTYPE_PATCH
	type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	#endif // WMTYPE_PATCH

	/* calculate menu geometry */
	#if PANGO_PATCH
	bh = drw->font->h + 2;
	#else
	bh = drw->fonts->h + 2;
	#endif // PANGO_PATCH
	#if LINE_HEIGHT_PATCH
	bh = MAX(bh,lineheight);	/* make a menu line AT LEAST 'lineheight' tall */
	#endif // LINE_HEIGHT_PATCH
	lines = MAX(lines, 0);
	mh = (lines + 1) * bh;
	#if CENTER_PATCH && PANGO_PATCH
	promptw = (prompt && *prompt) ? TEXTWM(prompt) - lrpad / 4 : 0;
	#elif CENTER_PATCH
	promptw = (prompt && *prompt) ? TEXTW(prompt) - lrpad / 4 : 0;
	#endif // CENTER_PATCH
#ifdef XINERAMA
	i = 0;
	if (parentwin == root && (info = XineramaQueryScreens(dpy, &n))) {
		XGetInputFocus(dpy, &w, &di);
		if (mon >= 0 && mon < n)
			i = mon;
		else if (w != root && w != PointerRoot && w != None) {
			/* find top-level window containing current input focus */
			do {
				if (XQueryTree(dpy, (pw = w), &dw, &w, &dws, &du) && dws)
					XFree(dws);
			} while (w != root && w != pw);
			/* find xinerama screen with which the window intersects most */
			if (XGetWindowAttributes(dpy, pw, &wa))
				for (j = 0; j < n; j++)
					if ((a = INTERSECT(wa.x, wa.y, wa.width, wa.height, info[j])) > area) {
						area = a;
						i = j;
					}
		}
		/* no focused window is on screen, so use pointer location instead */
		if (mon < 0 && !area && XQueryPointer(dpy, root, &dw, &dw, &x, &y, &di, &di, &du))
			for (i = 0; i < n; i++)
				if (INTERSECT(x, y, 1, 1, info[i]) != 0)
					break;

		#if CENTER_PATCH
		if (center) {
			#if XYW_PATCH
			mw = (dmw>0 ? dmw : MIN(MAX(max_textw() + promptw, min_width), info[i].width));
			#else
			mw = MIN(MAX(max_textw() + promptw, min_width), info[i].width);
			#endif // XYW_PATCH
			x = info[i].x_org + ((info[i].width  - mw) / 2);
			y = info[i].y_org + ((info[i].height - mh) / 2);
		} else {
			#if XYW_PATCH
			x = info[i].x_org + dmx;
			y = info[i].y_org + (topbar ? dmy : info[i].height - mh - dmy);
			mw = (dmw>0 ? dmw : info[i].width);
			#else
			x = info[i].x_org;
			y = info[i].y_org + (topbar ? 0 : info[i].height - mh);
			mw = info[i].width;
			#endif // XYW_PATCH
		}
		#elif XYW_PATCH
		x = info[i].x_org + dmx;
		y = info[i].y_org + (topbar ? dmy : info[i].height - mh - dmy);
		mw = (dmw>0 ? dmw : info[i].width);
		#else
		x = info[i].x_org;
		y = info[i].y_org + (topbar ? 0 : info[i].height - mh);
		mw = info[i].width;
		#endif // CENTER_PATCH / XYW_PATCH
		XFree(info);
	} else
#endif
	{
		if (!XGetWindowAttributes(dpy, parentwin, &wa))
			die("could not get embedding window attributes: 0x%lx",
			    parentwin);
		#if CENTER_PATCH
		if (center) {
			#if XYW_PATCH
			mw = (dmw>0 ? dmw : MIN(MAX(max_textw() + promptw, min_width), wa.width));
			#else
			mw = MIN(MAX(max_textw() + promptw, min_width), wa.width);
			#endif // XYW_PATCH
			x = (wa.width  - mw) / 2;
			y = (wa.height - mh) / 2;
		} else {
			#if XYW_PATCH
			x = dmx;
			y = topbar ? dmy : wa.height - mh - dmy;
			mw = (dmw>0 ? dmw : wa.width);
			#else
			x = 0;
			y = topbar ? 0 : wa.height - mh;
			mw = wa.width;
			#endif // XYW_PATCH
		}
		#elif XYW_PATCH
		x = dmx;
		y = topbar ? dmy : wa.height - mh - dmy;
		mw = (dmw>0 ? dmw : wa.width);
		#else
		x = 0;
		y = topbar ? 0 : wa.height - mh;
		mw = wa.width;
		#endif // CENTER_PATCH / XYW_PATCH
	}
	#if !CENTER_PATCH
	#if PANGO_PATCH
	promptw = (prompt && *prompt) ? TEXTWM(prompt) - lrpad / 4 : 0;
	#else
	promptw = (prompt && *prompt) ? TEXTW(prompt) - lrpad / 4 : 0;
	#endif // PANGO_PATCH
	#endif // CENTER_PATCH
	#if RELATIVE_INPUT_WIDTH_PATCH
	for (item = items; !lines && item && item->text; ++item) {
		curstrlen = strlen(item->text);
		if (numwidthchecks || minstrlen < curstrlen) {
			numwidthchecks = MAX(numwidthchecks - 1, 0);
			minstrlen = MAX(minstrlen, curstrlen);
			if ((tmp = textw_clamp(item->text, mw/3)) > inputw) {
				inputw = tmp;
				if (tmp == mw/3)
					break;
			}
		}
	}
	#else
	inputw = mw / 3; /* input width: ~33.33% of monitor width */
	#endif // RELATIVE_INPUT_WIDTH_PATCH
	match();

	/* create menu window */
	#if MANAGED_PATCH
	swa.override_redirect = managed ? False : True;
	#else
	swa.override_redirect = True;
	#endif // MANAGED_PATCH
	#if ALPHA_PATCH
	swa.background_pixel = 0;
	swa.colormap = cmap;
	#else
	swa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
	#endif // ALPHA_PATCH
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask
		#if MOUSE_SUPPORT_PATCH
		| ButtonPressMask
		#if MOTION_SUPPORT_PATCH
		| PointerMotionMask
		#endif // MOTION_SUPPORT_PATCH
		#endif // MOUSE_SUPPORT_PATCH
	;
	win = XCreateWindow(
		dpy, root,
		#if BARPADDING_PATCH && BORDER_PATCH
		x + sp, y + vp - (topbar ? 0 : border_width * 2), mw - 2 * sp - border_width * 2, mh, border_width,
		#elif BARPADDING_PATCH
		x + sp, y + vp, mw - 2 * sp, mh, 0,
		#elif BORDER_PATCH
		x, y - (topbar ? 0 : border_width * 2), mw - border_width * 2, mh, border_width,
		#else
		x, y, mw, mh, 0,
		#endif // BORDER_PATCH | BARPADDING_PATCH
		#if ALPHA_PATCH
		depth, InputOutput, visual,
		CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &swa
		#else
		CopyFromParent, CopyFromParent, CopyFromParent,
		CWOverrideRedirect | CWBackPixel | CWEventMask, &swa
		#endif // ALPHA_PATCH
	);
	#if BORDER_PATCH
	if (border_width)
		XSetWindowBorder(dpy, win, scheme[SchemeBorder][ColBg].pixel);
	#endif // BORDER_PATCH
	XSetClassHint(dpy, win, &ch);
	#if WMTYPE_PATCH
	XChangeProperty(dpy, win, type, XA_ATOM, 32, PropModeReplace,
			(unsigned char *) &dock, 1);
	#endif // WMTYPE_PATCH

	/* input methods */
	if ((xim = XOpenIM(dpy, NULL, NULL, NULL)) == NULL)
		die("XOpenIM failed: could not open input device");

	#if INPUTMETHOD_PATCH
	init_input_method(xim);
	#else
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                XNClientWindow, win, XNFocusWindow, win, NULL);
	#endif // INPUTMETHOD_PATCH

	#if MANAGED_PATCH
	if (managed) {
		XTextProperty prop;
		char *windowtitle = prompt != NULL ? prompt : "dmenu";
		Xutf8TextListToTextProperty(dpy, &windowtitle, 1, XUTF8StringStyle, &prop);
		XSetWMName(dpy, win, &prop);
		XSetTextProperty(dpy, win, &prop, XInternAtom(dpy, "_NET_WM_NAME", False));
		XFree(prop.value);
	}
	#endif // MANAGED_PATCH

	XMapRaised(dpy, win);
	if (embed) {
		XReparentWindow(dpy, win, parentwin, x, y);
		XSelectInput(dpy, parentwin, FocusChangeMask | SubstructureNotifyMask);
		if (XQueryTree(dpy, parentwin, &dw, &w, &dws, &du) && dws) {
			for (i = 0; i < du && dws[i] != win; ++i)
				XSelectInput(dpy, dws[i], FocusChangeMask);
			XFree(dws);
		}
		#if !INPUTMETHOD_PATCH
		grabfocus();
		#endif // INPUTMETHOD_PATCH
	}
	#if INPUTMETHOD_PATCH
	grabfocus();
	#endif // INPUTMETHOD_PATCH
	drw_resize(drw, mw, mh);
	drawmenu();
}

static void
usage(void)
{
	die("usage: dmenu [-bv"
		#if CENTER_PATCH
		"c"
		#endif
		#if !NON_BLOCKING_STDIN_PATCH
		"f"
		#endif // NON_BLOCKING_STDIN_PATCH
		#if INCREMENTAL_PATCH
		"r"
		#endif // INCREMENTAL_PATCH
		#if CASEINSENSITIVE_PATCH
		"s"
		#else
		"i"
		#endif // CASEINSENSITIVE_PATCH
		#if INSTANT_PATCH
		"n"
		#endif // INSTANT_PATCH
		#if PRINTINPUTTEXT_PATCH
		"t"
		#endif // PRINTINPUTTEXT_PATCH
		#if PREFIXCOMPLETION_PATCH
		"x"
		#endif // PREFIXCOMPLETION_PATCH
		#if FUZZYMATCH_PATCH
		"F"
		#endif // FUZZYMATCH_PATCH
		#if PASSWORD_PATCH
		"P"
		#endif // PASSWORD_PATCH
		#if NO_SORT_PATCH
		"S"
		#endif // NO_SORT_PATCH
		#if REJECTNOMATCH_PATCH
		"R" // (changed from r to R due to conflict with INCREMENTAL_PATCH)
		#endif // REJECTNOMATCH_PATCH
		#if RESTRICT_RETURN_PATCH
		"1"
		#endif // RESTRICT_RETURN_PATCH
		"] "
		#if CARET_WIDTH_PATCH
		"[-cw caret_width] "
		#endif // CARET_WIDTH_PATCH
		#if MANAGED_PATCH
		"[-wm] "
		#endif // MANAGED_PATCH
		#if GRID_PATCH
		"[-g columns] "
		#endif // GRID_PATCH
		"[-l lines] [-p prompt] [-fn font] [-m monitor]"
		"\n             [-nb color] [-nf color] [-sb color] [-sf color] [-w windowid]"
		#if DYNAMIC_OPTIONS_PATCH || FZFEXPECT_PATCH || ALPHA_PATCH || BORDER_PATCH || HIGHPRIORITY_PATCH
		"\n            "
		#endif
		#if DYNAMIC_OPTIONS_PATCH
		" [-dy command]"
		#endif // DYNAMIC_OPTIONS_PATCH
		#if FZFEXPECT_PATCH
		" [-ex expectkey]"
		#endif // FZFEXPECT_PATCH
		#if ALPHA_PATCH
		" [-o opacity]"
		#endif // ALPHA_PATCH
		#if BORDER_PATCH
		" [-bw width]"
		#endif // BORDER_PATCH
		#if HIGHPRIORITY_PATCH
		" [-hb color] [-hf color] [-hp items]"
		#endif // HIGHPRIORITY_PATCH
		#if INITIALTEXT_PATCH || LINE_HEIGHT_PATCH || PRESELECT_PATCH || NAVHISTORY_PATCH || XYW_PATCH
		"\n            "
		#endif
		#if INITIALTEXT_PATCH
		" [-it text]"
		#endif // INITIALTEXT_PATCH
		#if LINE_HEIGHT_PATCH
		" [-h height]"
		#endif // LINE_HEIGHT_PATCH
		#if PRESELECT_PATCH
		" [-ps index]"
		#endif // PRESELECT_PATCH
		#if NAVHISTORY_PATCH
		" [-H histfile]"
		#endif // NAVHISTORY_PATCH
		#if XYW_PATCH
		" [-X xoffset] [-Y yoffset] [-W width]" // (arguments made upper case due to conflicts)
		#endif // XYW_PATCH
		#if HIGHLIGHT_PATCH
		"\n             [-nhb color] [-nhf color] [-shb color] [-shf color]" // highlight colors
		#endif // HIGHLIGHT_PATCH
		#if SEPARATOR_PATCH
		"\n             [-d separator] [-D separator]"
		#endif // SEPARATOR_PATCH
		"\n");
}

int
main(int argc, char *argv[])
{
	XWindowAttributes wa;
	int i;
	#if !NON_BLOCKING_STDIN_PATCH
	int fast = 0;
	#endif // NON_BLOCKING_STDIN_PATCH

	#if XRESOURCES_PATCH
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	#if INPUTMETHOD_PATCH
	if (!XSetLocaleModifiers(""))
		fputs("warning: could not set locale modifiers", stderr);
	#endif // INPUTMETHOD_PATCH
	if (!(dpy = XOpenDisplay(NULL)))
		die("cannot open display");

	/* These need to be checked before we init the visuals and read X resources. */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) { /* prints version information */
			puts("dmenu-"VERSION);
			exit(0);
		} else if (!strcmp(argv[i], "-w")) {
			argv[i][0] = '\0';
			embed = strdup(argv[++i]);
		#if ALPHA_PATCH
		} else if (!strcmp(argv[i], "-o")) {  /* opacity, pass -o 0 to disable alpha */
			opacity = atoi(argv[++i]);
		#endif // ALPHA_PATCH
		} else {
			continue;
		}
		argv[i][0] = '\0'; // mark as used
	}

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	if (!embed || !(parentwin = strtol(embed, NULL, 0)))
		parentwin = root;
	if (!XGetWindowAttributes(dpy, parentwin, &wa))
		die("could not get embedding window attributes: 0x%lx",
		    parentwin);

	#if ALPHA_PATCH
	xinitvisual();
	drw = drw_create(dpy, screen, root, wa.width, wa.height, visual, depth, cmap);
	#else
	drw = drw_create(dpy, screen, root, wa.width, wa.height);
	#endif // ALPHA_PATCH
	readxresources();
	#endif // XRESOURCES_PATCH

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '\0')
			continue;

		/* these options take no arguments */
		if (!strcmp(argv[i], "-v")) {      /* prints version information */
			puts("dmenu-"VERSION);
			exit(0);
		} else if (!strcmp(argv[i], "-b")) { /* appears at the bottom of the screen */
			topbar = 0;
		#if CENTER_PATCH
		} else if (!strcmp(argv[i], "-c")) { /* toggles centering of dmenu window on screen */
			center = !center;
		#endif // CENTER_PATCH
		#if !NON_BLOCKING_STDIN_PATCH
		} else if (!strcmp(argv[i], "-f")) { /* grabs keyboard before reading stdin */
			fast = 1;
		#endif // NON_BLOCKING_STDIN_PATCH
		#if INCREMENTAL_PATCH
		} else if (!strcmp(argv[i], "-r")) { /* incremental */
			incremental = !incremental;
		#endif // INCREMENTAL_PATCH
		#if CASEINSENSITIVE_PATCH
		} else if (!strcmp(argv[i], "-s")) { /* case-sensitive item matching */
			fstrncmp = strncmp;
			fstrstr = strstr;
		#else
		} else if (!strcmp(argv[i], "-i")) { /* case-insensitive item matching */
			fstrncmp = strncasecmp;
			fstrstr = cistrstr;
		#endif // CASEINSENSITIVE_PATCH
		#if MANAGED_PATCH
		} else if (!strcmp(argv[i], "-wm")) { /* display as managed wm window */
			managed = 1;
		#endif // MANAGED_PATCH
		#if INSTANT_PATCH
		} else if (!strcmp(argv[i], "-n")) { /* instant select only match */
			instant = !instant;
		#endif // INSTANT_PATCH
		#if PRINTINPUTTEXT_PATCH
		} else if (!strcmp(argv[i], "-t")) { /* favors text input over selection */
			use_text_input = 1;
		#endif // PRINTINPUTTEXT_PATCH
		#if PREFIXCOMPLETION_PATCH
		} else if (!strcmp(argv[i], "-x")) { /* invert use_prefix */
			use_prefix = !use_prefix;
		#endif // PREFIXCOMPLETION_PATCH
		#if FUZZYMATCH_PATCH
		} else if (!strcmp(argv[i], "-F")) { /* disable/enable fuzzy matching, depends on default */
			fuzzy = !fuzzy;
		#endif // FUZZYMATCH_PATCH
		#if PASSWORD_PATCH
		} else if (!strcmp(argv[i], "-P")) { /* is the input a password */
			passwd = 1;
		#endif // PASSWORD_PATCH
		#if FZFEXPECT_PATCH
		} else if (!strcmp(argv[i], "-ex")) { /* expect key */
			expected = argv[++i];
		#endif // FZFEXPECT_PATCH
		#if REJECTNOMATCH_PATCH
		} else if (!strcmp(argv[i], "-R")) { /* reject input which results in no match */
			reject_no_match = 1;
		#endif // REJECTNOMATCH_PATCH
		#if NO_SORT_PATCH
		} else if (!strcmp(argv[i], "-S")) { /* do not sort matches */
			sortmatches = 0;
		#endif // NO_SORT_PATCH
		#if PRINTINDEX_PATCH
		} else if (!strcmp(argv[i], "-ix")) { /* adds ability to return index in list */
			print_index = 1;
		#endif // PRINTINDEX_PATCH
		#if RESTRICT_RETURN_PATCH
		} else if (!strcmp(argv[i], "-1")) {
			restrict_return = 1;
		#endif // RESTRICT_RETURN_PATCH
		} else if (i + 1 == argc)
			usage();
		/* these options take one argument */
		#if NAVHISTORY_PATCH
		else if (!strcmp(argv[i], "-H"))
			histfile = argv[++i];
		#endif // NAVHISTORY_PATCH
		#if GRID_PATCH
		else if (!strcmp(argv[i], "-g")) {   /* number of columns in grid */
			columns = atoi(argv[++i]);
			if (columns && lines == 0)
				lines = 1;
		}
		#endif // GRID_PATCH
		else if (!strcmp(argv[i], "-l"))   /* number of lines in vertical list */
			lines = atoi(argv[++i]);
		#if XYW_PATCH
		else if (!strcmp(argv[i], "-X"))   /* window x offset */
			dmx = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-Y"))   /* window y offset (from bottom up if -b) */
			dmy = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-W"))   /* make dmenu this wide */
			dmw = atoi(argv[++i]);
		#endif // XYW_PATCH
		else if (!strcmp(argv[i], "-m"))
			mon = atoi(argv[++i]);
		#if ALPHA_PATCH && !XRESOURCES_PATCH
		else if (!strcmp(argv[i], "-o"))  /* opacity, pass -o 0 to disable alpha */
			opacity = atoi(argv[++i]);
		#endif // ALPHA_PATCH
		else if (!strcmp(argv[i], "-p"))   /* adds prompt to left of input field */
			prompt = argv[++i];
		else if (!strcmp(argv[i], "-fn"))  /* font or font set */
			#if PANGO_PATCH
			font = argv[++i];
			#else
			fonts[0] = argv[++i];
			#endif // PANGO_PATCH
		#if LINE_HEIGHT_PATCH
		else if(!strcmp(argv[i], "-h")) { /* minimum height of one menu line */
			lineheight = atoi(argv[++i]);
			lineheight = MAX(lineheight, min_lineheight); /* reasonable default in case of value too small/negative */
		}
		#endif // LINE_HEIGHT_PATCH
		else if (!strcmp(argv[i], "-nb"))  /* normal background color */
			colors[SchemeNorm][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-nf"))  /* normal foreground color */
			colors[SchemeNorm][ColFg] = argv[++i];
		else if (!strcmp(argv[i], "-sb"))  /* selected background color */
			colors[SchemeSel][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-sf"))  /* selected foreground color */
			colors[SchemeSel][ColFg] = argv[++i];
		#if HIGHPRIORITY_PATCH
		else if (!strcmp(argv[i], "-hb"))  /* high priority background color */
			colors[SchemeHp][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-hf")) /* low priority background color */
			colors[SchemeHp][ColFg] = argv[++i];
		else if (!strcmp(argv[i], "-hp"))
			hpitems = tokenize(argv[++i], ",", &hplength);
 		#endif // HIGHPRIORITY_PATCH
		#if HIGHLIGHT_PATCH
		else if (!strcmp(argv[i], "-nhb")) /* normal hi background color */
			colors[SchemeNormHighlight][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-nhf")) /* normal hi foreground color */
			colors[SchemeNormHighlight][ColFg] = argv[++i];
		else if (!strcmp(argv[i], "-shb")) /* selected hi background color */
			colors[SchemeSelHighlight][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-shf")) /* selected hi foreground color */
			colors[SchemeSelHighlight][ColFg] = argv[++i];
		#endif // HIGHLIGHT_PATCH
		#if CARET_WIDTH_PATCH
		else if (!strcmp(argv[i], "-cw"))  /* sets caret witdth */
			caret_width = atoi(argv[++i]);
		#endif // CARET_WIDTH_PATCH
		#if !XRESOURCES_PATCH
		else if (!strcmp(argv[i], "-w"))   /* embedding window id */
			embed = argv[++i];
		#endif // XRESOURCES_PATCH
		#if SEPARATOR_PATCH
		else if (!strcmp(argv[i], "-d") || /* field separator */
				(separator_greedy = !strcmp(argv[i], "-D"))) {
			separator = argv[++i][0];
			separator_reverse = argv[i][1] == '|';
		}
		#endif // SEPARATOR_PATCH
		#if PRESELECT_PATCH
		else if (!strcmp(argv[i], "-ps"))   /* preselected item */
			preselected = atoi(argv[++i]);
		#endif // PRESELECT_PATCH
		#if DYNAMIC_OPTIONS_PATCH
		else if (!strcmp(argv[i], "-dy"))  /* dynamic command to run */
			dynamic = argv[++i];
		#endif // DYNAMIC_OPTIONS_PATCH
		#if BORDER_PATCH
		else if (!strcmp(argv[i], "-bw"))  /* border width around dmenu */
			border_width = atoi(argv[++i]);
		#endif // BORDER_PATCH
		#if INITIALTEXT_PATCH
		else if (!strcmp(argv[i], "-it")) {   /* adds initial text */
			const char * text = argv[++i];
			insert(text, strlen(text));
		}
		#endif // INITIALTEXT_PATCH
		else {
			usage();
		}
	}

	#if XRESOURCES_PATCH
	#if PANGO_PATCH
	if (!drw_font_create(drw, font))
		die("no fonts could be loaded.");
	#else
	if (!drw_fontset_create(drw, (const char**)fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	#endif // PANGO_PATCH
	#else // !XRESOURCES_PATCH
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	#if INPUTMETHOD_PATCH
	if (!XSetLocaleModifiers(""))
		fputs("warning: could not set locale modifiers", stderr);
	#endif // INPUTMETHOD_PATCH
	if (!(dpy = XOpenDisplay(NULL)))
		die("cannot open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	if (!embed || !(parentwin = strtol(embed, NULL, 0)))
		parentwin = root;
	if (!XGetWindowAttributes(dpy, parentwin, &wa))
		die("could not get embedding window attributes: 0x%lx",
		    parentwin);

	#if ALPHA_PATCH
	xinitvisual();
	drw = drw_create(dpy, screen, root, wa.width, wa.height, visual, depth, cmap);
	#else
	drw = drw_create(dpy, screen, root, wa.width, wa.height);
	#endif // ALPHA_PATCH

	#if PANGO_PATCH
	if (!drw_font_create(drw, font))
		die("no fonts could be loaded.");
	#else
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	#endif // PANGO_PATCH
	#endif // XRESOURCES_PATCH

	#if PANGO_PATCH
	lrpad = drw->font->h;
	#else
	lrpad = drw->fonts->h;
	#endif // PANGO_PATCH

	#if BARPADDING_PATCH
	sp = sidepad;
	vp = (topbar ? vertpad : - vertpad);
	#endif // BARPADDING_PATCH

	#if LINE_HEIGHT_PATCH
	if (lineheight == -1)
		#if PANGO_PATCH
		lineheight = drw->font->h * 2.5;
		#else
		lineheight = drw->fonts->h * 2.5;
		#endif // PANGO_PATCH
	#endif // LINE_HEIGHT_PATCH

#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		die("pledge");
#endif
	#if NAVHISTORY_PATCH
	loadhistory();
	#endif // NAVHISTORY_PATCH

	#if NON_BLOCKING_STDIN_PATCH
	grabkeyboard();
	#else
	if (fast && !isatty(0)) {
		grabkeyboard();
		#if DYNAMIC_OPTIONS_PATCH
		if (!(dynamic && *dynamic))
			readstdin();
		#else
		readstdin();
		#endif // DYNAMIC_OPTIONS_PATCH
	} else {
		#if DYNAMIC_OPTIONS_PATCH
		if (!(dynamic && *dynamic))
			readstdin();
		#else
		readstdin();
		#endif // DYNAMIC_OPTIONS_PATCH
		grabkeyboard();
	}
	#endif // NON_BLOCKING_STDIN_PATCH
	setup();
	run();

	return 1; /* unreachable */
}
