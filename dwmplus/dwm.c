/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance.  Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag.  Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask))
#define INRECT(X,Y,RX,RY,RW,RH) ((X) >= (RX) && (X) < (RX) + (RW) && (Y) >= (RY) && (Y) < (RY) + (RH))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define MAXCOLORS               8 // avoid circular reference to NUMCOLORS
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (textnw(X, strlen(X)) + dc.font.height)
#define ROOT                    RootWindow(dpy, DefaultScreen(dpy)) //托盘相关
#define zcalloc(size)           calloc(1, (size)) //托盘相关
#define IFREE(x)                if(x) free(x) //托盘相关
#define XEMBED_EMBEDDED_NOTIFY  0 //托盘相关
#define ATOM(a)      XInternAtom(dpy, (a), False)  //托盘相关

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast };        /* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };              /* color */
enum { NetSupported, NetWMName, NetWMState,
        NetWMFullscreen,  NetSystemTray,NetLast/*系统托盘相关*/};        /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState,eNetWMFullscreen,eNetSystemTray,WMLast};        /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast };             /* clicks */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct{
	unsigned int tag;
	Arg arg;
} Arg_Tag;

Arg_Tag g_layout[10] = {
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},
	{0xFF,0},

};
unsigned int g_cur = 0;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int sfx, sfy, sfw, sfh; /* stored float geometry, used on mode revert */
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	Bool isfixed, isfloating, isurgent, oldstate,needresize;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	int x, y, w, h;
    unsigned long colors[MAXCOLORS][ColLast];
    unsigned long norm[ColLast];
	unsigned long sel[ColLast];
	Drawable drawable;
	GC gc;
	struct {
		int ascent;
		int descent;
		int height;
#ifdef XFT
		XftFont *xft;
#else
		XFontSet set;
		XFontStruct *xfont;
#endif
		
	} font;
} DC; /* draw context */

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

struct Monitor {
	char				 ltsymbol[16];
	float				 mfact;
	int					 num;
	int					 by;    /* bar geometry */
	int					 mx, my, mw, mh;		/* screen size */
	int					 wx, wy, ww, wh;		/* window area  */
	unsigned int		 seltags;
	unsigned int		 sellt;
	unsigned int		 tagset[2];
	Bool				 showbar;
	Bool				 topbar;
	Client				*clients;
	Client				*sel;
	Client				*stack;
	Monitor				*next;
	Window				 barwin;
	const Layout		*lt[2];
	int					 primary;		//托盘相关
	
	int titlebarbegin;  // 鼠标点击窗口标题切换窗口
	int titlebarend;     // 鼠标点击窗口标题切换窗口

};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	Bool isfloating;
	int monitor;
} Rule;

/*系统托盘相关*/
typedef struct Systray	 Systray;
struct Systray {
	Window				 win;
	XRectangle			 geo;
	Systray				*next, *prev;
};
Systray					*trayicons;
Window					 traywin;
#define XEMBED_MAPPED                 (1 << 0)
#define XEMBED_WINDOW_ACTIVATE        1
#define XEMBED_WINDOW_DEACTIVATE      2

unsigned int occ = 0;//画棒时需要用到的变量，判断只显示有内容的tag

/* function declarations */
static void	      applyrules(Client *c);
static Bool	      applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact);
static void	      arrange(Monitor *m);
static void	      arrangemon(Monitor *m);
static void	      attach(Client *c);
static void	      attachstack(Client *c);
static void	      buttonpress(XEvent *e);
static void	      checkotherwm(void);
static void	      cleanup(void);
static void	      cleanupmon(Monitor *mon);
static void	      clearurgent(Client *c);
static void	      clientmessage(XEvent *e);
static void	      configure(Client *c);
static void	      configurenotify(XEvent *e);
static void	      configurerequest(XEvent *e);
static Monitor       *createmon(void);
static void	      destroynotify(XEvent *e);
static void	      detach(Client *c);
static void	      detachstack(Client *c);
static void	      die(const char *errstr, ...);
static Monitor       *dirtomon(int dir);
static void	      drawbar(Monitor *m);
static void	      drawbars(void);
static void	      drawvline(unsigned long col[ColLast]);
static void	      drawcoloredtext(char *text);
// static void			 drawsquare(Bool filled, Bool empty, unsigned long col[ColLast]);
static void	      drawsquare(Bool filled, Bool empty, Bool invert, unsigned long col[ColLast]);
static void	      drawtext(const char *text, unsigned long col[ColLast], Bool pad);
// static void         drawtext(const char *text, unsigned long col[ColLast], Bool invert, Bool pad);
static void	      enternotify(XEvent *e);
static void	      expose(XEvent *e);
static void	      focus(Client *c);
static void	      focusin(XEvent *e);
static void	      focusmon(const Arg *arg);
static void	      focusstack(const Arg *arg);
static unsigned long  getcolor(const char *colstr);
static Bool	      getrootptr(int *x, int *y);
static long	      getstate(Window w);
static Bool	      gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void	      grabbuttons(Client *c, Bool focused);
static void	      grabkeys(void);
static void	      initfont(const char *fontstr);
static Bool	      isprotodel(Client *c);
static void	      keypress(XEvent *e);
static void	      killclient(const Arg *arg);
static void       launcher(const Arg *arg);
static void	      manage(Window w, XWindowAttributes *wa);
static void	      mappingnotify(XEvent *e);
static void	      maprequest(XEvent *e);
static void	      monocle(Monitor *m);
static void	      movemouse(const Arg *arg);
static Client	     *nexttiled(Client *c);
static Monitor	     *ptrtomon(int x, int y);
static void	      propertynotify(XEvent *e);
pid_t             shexec(const char *cmd);
static void	      quit(const Arg *arg);
static void	      resize(Client *c, int x, int y, int w, int h, Bool interact);
static void	      resizeclient(Client *c, int x, int y, int w, int h);
static void	      resizemouse(const Arg *arg);
static void	      restack(Monitor *m);
static void	      run(void);
static void	      scan(void);
static void	      sendmon(Client *c, Monitor *m);
static void	      setclientstate(Client *c, long state);
static void	      setlayout(const Arg *arg);
static void	      setmfact(const Arg *arg);
static void	      setup(void);
static void	      showhide(Client *c);
static void	      sigchld(int unused);
static void	      spawn(const Arg *arg);
pid_t             tpawn(const char *format, ...);
static void	      tag(const Arg *arg);
static void	      tagmon(const Arg *arg);
static int	      textnw(const char *text, unsigned int len);
static void	      tile(Monitor *);
static void       grid(Monitor *m);
static void       bstack(Monitor *m);
static void	      togglebar(const Arg *arg);
static void	      togglefloating(const Arg *arg);
static void	      toggletag(const Arg *arg);
static void	      toggleview(const Arg *arg);
static void	      unfocus(Client *c, Bool setfocus);
static void	      unmanage(Client *c, Bool destroyed);
static void	      unmapnotify(XEvent *e);
static Bool	      updategeom(void);
static void	      updatebarpos(Monitor *m);
static void	      updatebars(void);
static void	      updatenumlockmask(void);
static void	      updatesizehints(Client *c);
static void	      updatestatus(void);
static void	      updatetitle(Client *c);
static void	      updatewmhints(Client *c);
static void	      view_layout(const Arg *arg);
static void	      view(const Arg *arg);
static Client	     *wintoclient(Window w);
static Monitor	     *wintomon(Window w);
static int	      xerror(Display *dpy, XErrorEvent *ee);
static int	      xerrordummy(Display *dpy, XErrorEvent *ee);
static int	      xerrorstart(Display *dpy, XErrorEvent *ee);
static void	      zoom(const Arg *arg);
//箭头键循环切换tag
static void	      view_next_tag(const Arg *);
static void	      view_prev_tag(const Arg *);
static void	      view_adjacent_tag(const Arg *, int);
//重启dwm
static void           restart(const Arg *arg);
//水平平铺
static void bstackhoriz(Monitor *m);

void	    movestack(const Arg *arg);
// 单击窗口标题区域切换窗口
static void focusonclick(const Arg *arg);
// 鼠标右键单击关闭该窗口
static void closeonclick(const Arg *arg);

/*系统托盘相关*/
static Bool		 systray_acquire(void);
static void		 systray_add(Window win);
static void		 systray_del(Systray *s);
static void		 systray_freeicons(void);
static Systray	*systray_find(Window win);
static int		 systray_get_width(void);
static void		 systray_update(void);
static void      systray_state(Systray *s);
void setwinstate(Window win, long state);
long ewmh_get_xembed_state(Window win);
void			 dwm_send_message(Window d, Window w, char *atom, long d0, long d1, long d2, long d3, long d4);
typedef unsigned char  uchar;
typedef unsigned long  ulong;

/* variables */
static const char broken[]		     = "broken";
static char		stext[256];
static int		screen;
static int		sw, sh; /* X display screen geometry width, height */
static int		bh, blw		     = 0;	/* bar geometry */
static			int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int	numlockmask	     = 0;
unsigned int tagcount = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]			     = buttonpress,
	[ClientMessage]			     = clientmessage,
	[ConfigureRequest]		     = configurerequest,
	[ConfigureNotify]		     = configurenotify,
	[DestroyNotify]			     = destroynotify,
	[EnterNotify]			     = enternotify,
	[Expose]			         = expose,
	[FocusIn]			         = focusin,
	[KeyPress]			         = keypress,
	[MappingNotify]			     = mappingnotify,
	[MapRequest]			     = maprequest,
	[PropertyNotify]		     = propertynotify,
	[UnmapNotify]			     = unmapnotify
};

/*注意：不要使用static，否则系统托盘无法正常加载！*/
Atom wmatom[WMLast] , netatom[NetLast];

static Bool otherwm;
static Bool running = True;
static Cursor cursor[CurLast];
Display *dpy;
DC dc;
Monitor *mons = NULL, *selmon = NULL;
Window root;
#ifdef XFT
static XftDraw *xftdraw;
#endif

Bool sel_win = False;
Bool istitledraw = False;

/* configuration, allows nested code to access above variables */
#include "config.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c) {
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { 0 };

	/* rule matching */
	c->isfloating = c->tags = 0;
	if(XGetClassHint(dpy, c->win, &ch)) {
		class = ch.res_class ? ch.res_class : broken;
		instance = ch.res_name ? ch.res_name : broken;
		for(i = 0; i < LENGTH(rules); i++) {
			r = &rules[i];
			if((!r->title || strstr(c->name, r->title))
			&& (!r->class || strstr(class, r->class))
			&& (!r->instance || strstr(instance, r->instance)))
			{
				c->isfloating = r->isfloating;
				c->tags |= r->tags;
				for(m = mons; m && m->num != r->monitor; m = m->next);
				if(m)
					c->mon = m;
			}
		}
		if(ch.res_class)
			XFree(ch.res_class);
		if(ch.res_name)
			XFree(ch.res_name);
	}
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

Bool
applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact) {
	Bool baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if(interact) {
		if(*x > sw)
			*x = sw - WIDTH(c);
		if(*y > sh)
			*y = sh - HEIGHT(c);
		if(*x + *w + 2 * c->bw < 0)
			*x = 0;
		if(*y + *h + 2 * c->bw < 0)
			*y = 0;
	}
	else {
		if(*x > m->mx + m->mw)
			*x = m->mx + m->mw - WIDTH(c);
		if(*y > m->my + m->mh)
			*y = m->my + m->mh - HEIGHT(c);
		if(*x + *w + 2 * c->bw < m->mx)
			*x = m->mx;
		if(*y + *h + 2 * c->bw < m->my)
			*y = m->my;
	}
	if(*h < bh)
		*h = bh;
	if(*w < bh)
		*w = bh;
	if(resizehints || c->isfloating) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if(!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if(c->mina > 0 && c->maxa > 0) {
			if(c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if(c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if(baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if(c->incw)
			*w -= *w % c->incw;
		if(c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w += c->basew;
		*h += c->baseh;
		*w = MAX(*w, c->minw);
		*h = MAX(*h, c->minh);
		if(c->maxw)
			*w = MIN(*w, c->maxw);
		if(c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m) {
	if(m)
		showhide(m->stack);
	else for(m = mons; m; m = m->next)
		showhide(m->stack);
	focus(NULL);
	if(m)
		arrangemon(m);
	else for(m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m) {
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if(m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
	restack(m);
}

void
attach(Client *c) {
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachstack(Client *c) {
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e) {
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click                               = ClkRootWin;
	// focus monitor if necessary
	if((m = wintomon(ev->window)) && m != selmon) {
            unfocus(selmon->sel, True);
            selmon                          = m;
            focus(NULL);
           
        }
        
	if(ev->window == selmon->barwin) {
		i = x = 0;
		do {
			// 只让有内容显示的tag响应鼠标单击事件
			if(autohide) {
				if((occ & 1 << i ) || (m->tagset[m->seltags] & 1 << i))
					x += TEXTW(tags[i]);
			}
			else
				x += TEXTW(tags[i]);	
		} while(ev->x >= x && ++i < LENGTH(tags));
		
		if(i < LENGTH(tags)) {
			// 只让有内容显示的tag响应鼠标单击事件
			if(autohide) {
			 if((occ & 1 << i ) || (m->tagset[m->seltags] & 1 << i))
				 {
					 click  = ClkTagBar;
					 arg.ui = 1 << i;
				 }
			}
			else {
				click  = ClkTagBar;
				arg.ui = 1 << i;
			}
		}
		else if(ev->x < x + blw)
			click = ClkLtSymbol;
		else if(ev->x > selmon->wx + selmon->ww - TEXTW(stext))
			click = ClkStatusText;
		else{
			arg.ui = ev->x; //鼠标点击切换窗口焦点，否则鼠标点击后没有任何反应
			click = ClkWinTitle;
		}
	}
	else if((c = wintoclient(ev->window))) {
		focus(c);
                click = ClkClientWin;
        }

        for(i = 0; i < LENGTH(buttons); i++)
		if(click                              == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		   && CLEANMASK(buttons[i].mask)      == CLEANMASK(ev->state))
		    //注意加上对窗口标题栏点击：click == ClkWinTitle，否则鼠标点击无反应
                    buttons[i].func((click            == ClkTagBar || click == ClkWinTitle || click == ClkClientWin) && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void) {
	otherwm = False;
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	if(otherwm)
		die("dwm: another window manager is already running\n");
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void) {
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for(m = mons; m; m = m->next)
		while(m->stack)
			unmanage(m->stack, False);
#ifdef XFT
	XftFontClose(dpy, dc.font.xft);
#else
	if(dc.font.set)
		XFreeFontSet(dpy, dc.font.set);
	else
		XFreeFont(dpy, dc.font.xfont);
#endif
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XFreePixmap(dpy, dc.drawable);
	XFreeGC(dpy, dc.gc);
	XFreeCursor(dpy, cursor[CurNormal]);
	XFreeCursor(dpy, cursor[CurResize]);
	XFreeCursor(dpy, cursor[CurMove]);
	while(mons)
		cleanupmon(mons);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

void
cleanupmon(Monitor *mon) {
	Monitor *m;

	if(mon == mons)
		mons = mons->next;
	else {
		for(m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
clearurgent(Client *c) {
	XWMHints *wmh;

	c->isurgent = False;
	if(!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags &= ~XUrgencyHint;
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
configure(Client *c) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e) {
	Monitor *m;
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root) {
		sw = ev->width;
		sh = ev->height;
		if(updategeom()) {
			if(dc.drawable != 0)
				XFreePixmap(dpy, dc.drawable);
			dc.drawable = XCreatePixmap(dpy, root, sw, bh, DefaultDepth(dpy, screen));
			updatebars();
			for(m = mons; m; m = m->next)
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e) {
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if((c = wintoclient(ev->window))) {
		if(ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if(c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if(ev->value_mask & CWX)
				c->x = m->mx + ev->x;
			if(ev->value_mask & CWY)
				c->y = m->my + ev->y;
			if(ev->value_mask & CWWidth)
				c->w = ev->width;
			if(ev->value_mask & CWHeight)
				c->h = ev->height;
			if((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - c->w / 2); /* center in x direction */
			if((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - c->h / 2); /* center in y direction */
			if((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if(ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
			else
				c->needresize=1;
		}
		else
			configure(c);
	}
	else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void) {
	Monitor *m;

	if(!(m = (Monitor *)calloc(1, sizeof(Monitor))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->showbar = showbar;
	m->topbar = topbar;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	return m;
}

void
destroynotify(XEvent *e) {
	Client *c;
	Systray *s;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if((c = wintoclient(ev->window)))
		unmanage(c, True);
	else if((s = systray_find(ev->window))){
		systray_del(s);
		systray_update();
	}	
}

void
detach(Client *c) {
	Client **tc;

	for(tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c) {
	Client **tc, *t;

	for(tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if(c == c->mon->sel) {
		for(t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

Monitor *
dirtomon(int dir) {
	Monitor *m = NULL;

	if(dir > 0) {
		if(!(m = selmon->next))
			m = mons;
	}
	else {
		if(selmon == mons)
			for(m = mons; m->next; m = m->next);
		else
			for(m = mons; m->next != selmon; m = m->next);
	}
	return m;
}

/*void
drawbar(Monitor *m) {			//本函数的作用：绘制tag区域和状态栏
int					x;
	unsigned int		i , urg = 0;
	occ = 0;//注意使之等于0，当有内容的tag关闭时，能自动隐藏该tag
	unsigned long *col;
	Client *c;

	for(c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if(c->isurgent)
			urg |= c->tags;
	}
	dc.x = 0;
	// 绘制标签页（tags）
	for(i = 0; i < LENGTH(tags); i++) {
		// 自动隐藏没有内容的标签
		if(autohide) {
	        if((occ & 1 << i ) || (m->tagset[m->seltags] & 1 << i)) {// 自动隐藏没有内容的tag
				//dc.w = textnw(tags[i], strlen(tags[i]));  //tag区块很紧凑，废弃该行代码
				dc.w = TEXTW(tags[i]);    //注意使用TEXTW，否则tag区块之间无间隔，很紧凑，致使鼠标的点击焦点出现问题
				col = dc.colors[ (m->tagset[m->seltags] & 1 << i) ? 
								 1 : (urg & 1 << i ? 2:0) ];
             
				drawtext(tags[i], col, True);   //True 表示将数字画在中间位置，False：左对齐
           	    dc.x += dc.w;
			}
		}

		 if(!autohide) {// 非自动隐藏没有内容的tag
			 dc.w = TEXTW(tags[i]);    //注意使用TEXTW，否则tag区块之间无间隔，很紧凑，致使鼠标的点击焦点出现问题
			 col = dc.colors[ (m->tagset[m->seltags] & 1 << i) ? 
							  1 : (urg & 1 << i ? 2:0) ];
             
			 drawtext(tags[i], col, True);   //True 表示将数字画在中间位置，False：左对齐非中间位置
            
			 //在有内容的tag上面用左上角的小正方形来标示，不错的创意！
			 drawsquare(m == selmon && selmon->sel && selmon->sel->tags & 1 << i, occ & 1 << i, urg & 1 << i , col);
		           
			 dc.x += dc.w;
       }
	}
	
	//绘制显示平铺方式的东东，比如“[M]=”等符号
	dc.w = blw = TEXTW(m->ltsymbol);
	drawtext(m->ltsymbol, dc.colors[0], True);
	dc.x += dc.w;
	x = dc.x;
	
	//绘制状态栏，包括窗口标题显示部分和显示系统时间等信息的部分，也就是绘制除tag之外的部分
	if(m == selmon) {			// status is only drawn on selected monitor 
		dc.w	= textnw(stext, strlen(stext)) + dc.font.height / 2;	// 字体高度的二分之一为信息栏和托盘之间的一个小间隙
		// dc.w	= TEXTW(stext);
		dc.x	= m->ww - dc.w;
		if(systray_enable)
			{
				dc.x  = dc.x - systray_get_width();		// subtract systray width
				
			}
		// 如果信息栏将窗口标题栏挤没了，那就委屈窗口标题栏吧，窗口标题栏没了
		if(dc.x < x) {
			dc.x = x;
			dc.w = m->ww -  x;
		}
		
		//绘制状态棒信息，显示系统时间、CPU占用率等东东，彩色显示（Colorbar）		
		drawcoloredtext(stext);
		
	}
	else
		dc.x = m->ww;
		
	if((dc.w = dc.x - x) > bh) {
		dc.x = x;
		if(m->sel) {
			//用固定的颜色来绘制窗口标题栏，定义中的第8种颜色
			//col = dc.colors[ m == selmon ? 3 : 0 ]; // color 3 is title text
			col = m == selmon ? dc.colors[7] : dc.colors[0]; //colors[1]决定颜色值
			drawtext(m->sel->name, col, True);
			drawsquare(m->sel->isfixed, m->sel->isfloating, False , col);
		}
		else
			drawtext(NULL, dc.colors[0], False);
	}
		
	XCopyArea(dpy, dc.drawable, m->barwin, dc.gc, 0, 0, m->ww, bh, 0, 0);
	XSync(dpy, False);
}*/

void
drawbar(Monitor *m) {
    int x, a = 0, s= 0, ow, mw = 0, extra, tw;
    unsigned int   i, n	= 0 , urg = 0;
    occ = 0;    //注意使之等于0，当有内容的tag关闭时，能自动隐藏该tag
    char	   posbuf[10];
    unsigned long *col;
    Client	  *c, *firstvis, *lastvis = NULL;
    DC seldc;
    
    for(c = m->clients; c; c = c->next) {
		if(ISVISIBLE(c))
			n++;
		occ |= c->tags;
		if(c->isurgent)
			urg |= c->tags;
	}
	dc.x	   = 0;
	tagcount = 0;
	// 绘制标签页（tags）
	for(i = 0; i < LENGTH(tags); i++) {
            col	   = dc.colors[ (m->tagset[m->seltags] & 1 << i) ? 
                                1 : (urg & 1 << i ? 2:0) ];
            if(autohide) {
                
                if((occ & 1 << i ) || (m->tagset[m->seltags] & 1 << i)) { // 自动隐藏没有内容的tag
                    dc.w = TEXTW(tags[i]);
                    //记录有效的标签页的个数，在launcher里面要用到
                    tagcount ++;
                    drawtext(tags[i], col , True);
                    dc.x += dc.w;
                }
		
            }
            else {// 非自动隐藏
                dc.w	 = TEXTW(tags[i]);
                drawtext(tags[i], col , True);
                drawsquare(m == selmon && selmon->sel && selmon->sel->tags & 1 << i , occ & 1 << i, urg & 1 << i, col);
                dc.x += dc.w;
            }
	
	}

	// 绘制表示平铺样式的符号
	dc.w  = blw = TEXTW(m->ltsymbol);
	drawtext(m->ltsymbol, dc.colors[0], True);
	dc.x += dc.w;
	x = dc.x;
	
	if(m == selmon) {	// status is only drawn on selected monitor
		// 绘制窗口获得焦点的标示诸如："[1/3]"
		if(m->lt[m->sellt]->arrange == monocle) {
			  dc.x = x;
			  for(c= nexttiled(m->clients), a  = 0, s= 0; c; c= nexttiled(c->next), a++)
				  if(c == m->stack)
					  s = a;
			  if(!s && a)
				  s = a;
			  snprintf(posbuf, LENGTH(posbuf), "[%d/%d]", s, a);
			  dc.w = TEXTW(posbuf);
			  col = dc.colors[0];
			  drawtext(posbuf, col , True);
			  x = dc.x + dc.w;
                }
		
		// 绘制状态信息栏
		dc.w=0;
		char *buf = stext, *ptr = buf;
		while( *ptr ) {
			for( i = 0; *ptr < 0 || *ptr > NUMCOLORS; i++, ptr++);
			dc.w += textnw(buf,i);
			buf=++ptr;
		}
		dc.w +=	dc.font.height / 2;
		dc.x  = m->ww - dc.w;
		if(systray_enable)
			{
				dc.x  = dc.x - systray_get_width();		// subtract systray width
			}
		if(dc.x < x) {
			dc.x = x;
			dc.w = m->ww - x;
		}
		// 窗口标题结束的位置（信息栏开始的位置）
		m->titlebarend = dc.x;
		// 开始绘制信息栏
		drawcoloredtext(stext);	
	} //end if(m == selmon)
	else {
		dc.x = m->ww;
		m->titlebarend=dc.x;
	}

	for(c = m->clients; c && !ISVISIBLE(c); c = c->next);
	firstvis = c;

	// col = m == selmon ? dc.sel : dc.norm;
	col = m == selmon ? dc.colors[7] : dc.colors[0]; 
	dc.w = dc.x - x;
	dc.x = x;
	// 如果当前有可视窗口存在（有应用程序打开）
	if(n > 0) {
		mw = dc.w / n;
		extra = 0;
		seldc = dc;
		i = 0;

		while(c) {
			lastvis = c;
			tw = TEXTW(c->name);
			if(tw < mw) extra += (mw - tw); else i++;
			for(c = c->next; c && !ISVISIBLE(c); c = c->next);
 		}

		if(i > 0) mw += extra / i;

		c = firstvis;
		x = dc.x;
		}
	// 窗口标题起始位置
	m->titlebarbegin = dc.x;
	while(dc.w > bh) {
		if(c) {
			ow = dc.w;
			tw = TEXTW(c->name);
			dc.w = MIN(ow, tw);

			if(dc.w > mw) dc.w    = mw;
			if(m->sel == c) seldc = dc;
			if(c == lastvis) dc.w = ow;
                        
                        // 绘制窗口标题
			drawtext(c->name, col , True);
			// 绘制分隔线
			if(c != firstvis)
                  drawvline(col);
			//单独对绘制正方形进行控制,使小正方形向右边偏离一点点，不压竖线
			istitledraw = True;
			drawsquare(c->isfixed, c->isfloating, False, col);
			istitledraw = False;
			
			dc.x += dc.w;
			dc.w = ow - dc.w;
			for(c = c->next; c && !ISVISIBLE(c); c = c->next);
		} else {
 			drawtext(NULL, dc.colors[0] , True);
			break;
		}
	}

	// 绘制当前获得焦点的窗口的标题
	if(m == selmon && m->sel && ISVISIBLE(m->sel)) {
		dc = seldc;
		col = dc.colors[1]; // 被选中的颜色
		//调整被选择的窗口的标题，使之对中
		sel_win = True;
		drawtext(m->sel->name, col, True);
		
		drawsquare(m->sel->isfixed, m->sel->isfloating, True, col);
		
		sel_win = False;
		
	}
	
	XCopyArea(dpy, dc.drawable, m->barwin, dc.gc, 0, 0, m->ww, bh, 0, 0);
	XSync(dpy, False);
	}

void
drawbars(void) {
	Monitor *m;

	for(m = mons; m; m = m->next)
		drawbar(m);
}

void
drawvline(unsigned long col[ColLast]) {
        XGCValues gcv;

	gcv.foreground = col[ColFG];
	XChangeGC(dpy, dc.gc, GCForeground, &gcv);
	// XDrawLine(dpy, dc.drawable, dc.gc, dc.x, dc.y, dc.x, dc.y + (dc.font.ascent + dc.font.descent + 2));
    // 绘制的竖线在中间，与上下有间隔
	XDrawLine(dpy, dc.drawable, dc.gc, dc.x, dc.y + 3 , dc.x, dc.y + bh - 3);
	//XDrawLine(dpy, dc.drawable, dc.gc, dc.x - 1 , dc.y + 3 , dc.x - 1 , dc.y + bh - 3);
    //XDrawRectangle(dpy , dc.drawable , dc.gc , dc.x - 3 , dc.y + 3 , 2 , bh - 6);
        
        
	}

void
drawcoloredtext(char *text) {
    char *buf = text, *ptr = buf, c = 1;
    unsigned long *col = dc.colors[0];
    int i, ox = dc.x;

    while( *ptr ) {
        for( i = 0; *ptr < 0 || *ptr > NUMCOLORS; i++, ptr++);
        if( !*ptr ) break;
        c=*ptr;
        *ptr=0;
        if( i ) {
            dc.w = selmon->ww - dc.x;
            drawtext(buf, col, False);
            dc.x += textnw(buf, i);
        }
        *ptr = c;
        col = dc.colors[ c-1 ];
        buf = ++ptr;
    }
    drawtext(buf, col, False);
    dc.x = ox;
}

/*void
drawsquare(Bool filled, Bool empty, unsigned long col[ColLast]) {
	int x;
	XGCValues gcv;
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };

	gcv.foreground = col[ColFG];
	XChangeGC(dpy, dc.gc, GCForeground, &gcv);
	x = (dc.font.ascent + dc.font.descent + 2) / 4;
	r.x = dc.x + 1;
	r.y = dc.y + 1;
	if(filled) {
		r.width = r.height = x + 1;
		XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	}
	else if(empty) {
		r.width = r.height = x;
		XDrawRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	}
	}*/

void
drawsquare(Bool filled, Bool empty, Bool invert, unsigned long col[ColLast]) {
	int x;
	XGCValues gcv;
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };

	gcv.foreground = col[invert ? ColBG : ColFG];
	XChangeGC(dpy, dc.gc, GCForeground, &gcv);
	x = (dc.font.ascent + dc.font.descent + 2) / 4;
	if(istitledraw)//绘制标题上面的小正方形
	  r.x = dc.x + 3;
	else
	  r.x = dc.x + 1;  
	r.y = dc.y + 1;
	if(filled) {
		r.width = r.height = x + 1;
		XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	}
	else if(empty) {
		r.width = r.height = x;
		XDrawRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	}
	}

void
drawtext(const char *text, unsigned long col[ColLast], Bool pad) {
	char buf[256];
	int i, x, y, h, len, olen;
	
	if(sel_win) //当前窗口被选择
	{
		dc.x = dc.x + 3;
		dc.w = dc.w - 5;
				
	}
		
	XRectangle r = { dc.x , dc.y, dc.w , dc.h };
	
		 
#ifdef XFT
	int p;
	XColor color;
	XftColor xftcolor;

	color.pixel = col[ColFG];
	XQueryColor(dpy, DefaultColormap(dpy, screen), &color);
	xftcolor.color.red = color.red;
	xftcolor.color.green = color.green;
	xftcolor.color.blue = color.blue;
	xftcolor.color.alpha = 0xffff;
#endif

	XSetForeground(dpy, dc.gc, col[ColBG]);
	XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	if(!text)
		return;
	olen = strlen(text);
	h    = pad ? (dc.font.ascent + dc.font.descent) : 0;
	 y    = dc.y + ((dc.h + dc.font.ascent - dc.font.descent) / 2);
	 x    = dc.x + (h / 2);
	/* shorten text if necessary */
	for(len = MIN(olen, sizeof buf); len && textnw(text, len) > dc.w - h; len--);
	if(!len)
		return;
	memcpy(buf, text, len);
#ifdef XFT
	if(len < olen) {
		for(i = len - 1, p = 0; i; --i, ++p) {
			if(buf[i] & 0x80) { /* 10xxxxxx */
				if((buf[i] & 0x40) && (p >= 2)) { /* 11xxxxxx */
					buf[i] = '.';
					break;
				}
				buf[i] = '.';
			}
			else {
				buf[i] = '.';
				if(p == 2) break;
			}
		}
		++p;
		if (p > 3)
			len -= p - 3;
	}

	XftColorAllocValue(dpy, XDefaultVisual(dpy, screen),
			   DefaultColormap(dpy, screen),
			   &xftcolor.color, &xftcolor);
 	XftDrawStringUtf8(xftdraw, &xftcolor, dc.font.xft, x, y,
			  (XftChar8*)buf, len);
	XftColorFree(dpy, XDefaultVisual(dpy, screen),
		     DefaultColormap(dpy, screen), &xftcolor);
#else
	if(len < olen)
		for(i = len; i && i > len - 3; buf[--i] = '.');
	XSetForeground(dpy, dc.gc, col[ColFG]);
	if(dc.font.set)
		XmbDrawString(dpy, dc.drawable, dc.font.set, dc.gc, x, y, buf, len);
	else
		XDrawString(dpy, dc.drawable, dc.gc, x, y, buf, len);
#endif
}

void
enternotify(XEvent *e) {
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	if((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, True);
		selmon = m;
	}
	if((c = wintoclient(ev->window)))
		focus(c);
	else
		focus(NULL);
}

void
expose(XEvent *e) {
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if(ev->count == 0 && (m = wintomon(ev->window)))
		drawbar(m);
}

void
focus(Client *c) {
	if(!c || !ISVISIBLE(c))
		for(c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	/* was if(selmon->sel) */
	if(selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, False);
	if(c) {
		if(c->mon != selmon)
			selmon = c->mon;
		if(c->isurgent)
			clearurgent(c);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, True);
		XSetWindowBorder(dpy, c->win, dc.colors[1][ColBorder]);
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	}
	else
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	selmon->sel = c;
	drawbars();
}

void
focusin(XEvent *e) { /* there are some broken focus acquiring clients */
	XFocusChangeEvent *ev = &e->xfocus;

	if(selmon->sel && ev->window != selmon->sel->win)
		XSetInputFocus(dpy, selmon->sel->win, RevertToPointerRoot, CurrentTime);
}

void
focusmon(const Arg *arg) {
	Monitor *m = NULL;

	if(!mons->next)
		return;
	if((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, True);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg) {
	Client *c = NULL, *i;

	if(!selmon->sel)
		return;
	if(arg->i > 0) {
		for(c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if(!c)
			for(c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	}
	else {
		for(i = selmon->clients; i != selmon->sel; i = i->next)
			if(ISVISIBLE(i))
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i))
					c = i;
	}
	if(c) {
		focus(c);
		restack(selmon);
	}
}

unsigned long
getcolor(const char *colstr) {
	Colormap cmap = DefaultColormap(dpy, screen);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
		die("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

Bool
getrootptr(int *x, int *y) {
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w) {
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if(XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
	                      &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if(n != 0)
		result = *p;
	XFree(p);
	return result;
}

Bool
gettextprop(Window w, Atom atom, char *text, unsigned int size) {
	char **list = NULL;
	int n;
	XTextProperty name;

	if(!text || size == 0)
		return False;
	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if(!name.nitems)
		return False;
	if(name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if(XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return True;
}

void
grabbuttons(Client *c, Bool focused) {
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if(focused) {
			for(i = 0; i < LENGTH(buttons); i++)
				if(buttons[i].click == ClkClientWin)
					for(j = 0; j < LENGTH(modifiers); j++)
						XGrabButton(dpy, buttons[i].button,
						            buttons[i].mask | modifiers[j],
						            c->win, False, BUTTONMASK,
						            GrabModeAsync, GrabModeSync, None, None);
		}
		else
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
			            BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void) {
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for(i = 0; i < LENGTH(keys); i++) {
			if((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for(j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						 True, GrabModeAsync, GrabModeAsync);
		}
	}
}

void
initfont(const char *fontstr) {
#ifdef XFT
	XftResult result;
	XftPattern *p;
	XftPattern *m;

	p = XftNameParse(fontstr);
	m = XftFontMatch(dpy, screen, p, &result);
	dc.font.xft = XftFontOpenPattern(dpy, m);
	dc.font.ascent = dc.font.xft->ascent;
	dc.font.descent = dc.font.xft->descent;

	dc.font.height = dc.font.ascent + dc.font.descent;
#else
	char *def, **missing;
	int i, n;

	missing = NULL;
	dc.font.set = XCreateFontSet(dpy, fontstr, &missing, &n, &def);
	if(missing) {
		while(n--)
			fprintf(stderr, "dwm: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(dc.font.set) {
		XFontSetExtents *font_extents;
		XFontStruct **xfonts;
		char **font_names;

		dc.font.ascent = dc.font.descent = 0;
		font_extents = XExtentsOfFontSet(dc.font.set);
		n = XFontsOfFontSet(dc.font.set, &xfonts, &font_names);
		for(i = 0, dc.font.ascent = 0, dc.font.descent = 0; i < n; i++) {
			dc.font.ascent = MAX(dc.font.ascent, (*xfonts)->ascent);
			dc.font.descent = MAX(dc.font.descent,(*xfonts)->descent);
			xfonts++;
		}
	}
	else {
		if(!(dc.font.xfont = XLoadQueryFont(dpy, fontstr))
		&& !(dc.font.xfont = XLoadQueryFont(dpy, "fixed")))
			die("error, cannot load font: '%s'\n", fontstr);
		dc.font.ascent = dc.font.xfont->ascent;
		dc.font.descent = dc.font.xfont->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
#endif
}

Bool
isprotodel(Client *c) {
  	//  注意：需要再次给wmatom赋值，因定义中取消了static，初始化中的wmatom值有可能出现了变化。如果不再次赋值，则客户窗口有可能会收不到关闭的消息（直接kill掉客户窗口），从而导致数据丢失。
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	
	/*方法一*/
  /*	int i, n;
	Atom *protocols;
	Bool ret = False;

	if(XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		for(i = 0; !ret && i < n; i++)
			if(protocols[i] == wmatom[WMDelete])
				ret = True;
		XFree(protocols);
	}
	return ret;*/

	/*方法二*/
  /*	int n;
	Atom *protocols;
	Bool ret = False;

	if(XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while(!ret && n--)
			ret = protocols[n] == wmatom[WMDelete];
		XFree(protocols);
	}
	return ret;*/

	/*方法三*/
	int	canbedel = 0;
	Atom *atom = NULL;
	int proto;
	Bool ret = False;
	if(XGetWMProtocols(dpy, c->win, &atom, &proto) && atom)
	  {
	    while(proto--)
	      if(atom[proto] == wmatom[WMDelete])
		++canbedel;
	    if(canbedel)
	      ret = True;
	    XFree(atom);
	  }
 
	return ret;
}

#ifdef XINERAMA
static Bool
isuniquegeom(XineramaScreenInfo *unique, size_t len, XineramaScreenInfo *info) {
	unsigned int i;

	for(i = 0; i < len; i++)
		if(unique[i].x_org == info->x_org && unique[i].y_org == info->y_org
		&& unique[i].width == info->width && unique[i].height == info->height)
			return False;
	return True;
}
#endif /* XINERAMA */

void
keypress(XEvent *e) {
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for(i = 0; i < LENGTH(keys); i++)
		if(keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

void
killclient(const Arg *arg) {
	XEvent	ev;
	//  注意：需要再次给wmatom赋值，因定义中取消了static，初始化中的wmatom值有可能出现了变化。如果不再次赋值，则客户窗口有可能会收不到关闭的消息（直接kill掉客户窗口），从而导致数据丢失。
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	if(!selmon->sel)
		return;
	if(isprotodel(selmon->sel)) {
		ev.type = ClientMessage;
		ev.xclient.window = selmon->sel->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = wmatom[WMDelete];
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, selmon->sel->win, False, NoEventMask, &ev);
	}
	else {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
	        XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa) {
	static Client cz;
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	if(!(c = malloc(sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));
	*c = cz;
	c->win = w;
	updatetitle(c);
	if(XGetTransientForHint(dpy, w, &trans))
		t = wintoclient(trans);
	if(t) {
		c->mon = t->mon;
		c->tags = t->tags;
	}
	else {
		c->mon = selmon;
		applyrules(c);
	}
	/* geometry */
	c->x = c->oldx = wa->x + c->mon->wx;
	c->y = c->oldy = wa->y + c->mon->wy;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	if(c->w == c->mon->mw && c->h == c->mon->mh) {
		c->isfloating = 1;
		c->x = c->mon->mx;
		c->y = c->mon->my;
		c->bw = 0;
	}
	else {
		if(c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
			c->x = c->mon->mx + c->mon->mw - WIDTH(c);
		if(c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
			c->y = c->mon->my + c->mon->mh - HEIGHT(c);
		c->x = MAX(c->x, c->mon->mx);
		/* only fix client y-offset, if the client center might cover the bar */
		c->y = MAX(c->y, ((c->mon->by == 0) && (c->x + (c->w / 2) >= c->mon->wx)
		           && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
		c->bw = borderpx;
	}
	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, dc.colors[0][ColBorder]);
	configure(c); /* propagates border_width, if size doesn't change */
	updatesizehints(c);
	c->sfx = c->x;
	c->sfy = c->y;
	c->sfw = c->w;
	c->sfh = c->h;
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, False);
	if(!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if(c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	XMapWindow(dpy, c->win);
	setclientstate(c, NormalState);
	arrange(c->mon);
}

void
mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if(ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e) {
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	Systray *s; //系统托盘相关
	
	if((s = systray_find(ev->window)))
     {
          dwm_send_message(s->win, s->win, "_XEMBED", CurrentTime, XEMBED_WINDOW_ACTIVATE , 0, 0, 0);
          
          systray_update();
	  }

	if(!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if(wa.override_redirect)
		return;
	if(!wintoclient(ev->window))
		manage(ev->window, &wa);
}

/*单个窗口显示（窗口最大化显示）*/
void
monocle(Monitor *m) {
	unsigned int n = 0;
	Client *c;

	for(c = m->clients; c; c = c->next)
		if(ISVISIBLE(c))
			n++;
	if(n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for(c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, False);
}

void
movemouse(const Arg *arg) {
		
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	
	if(!(c = selmon->sel))
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	None, cursor[CurMove], CurrentTime) != GrabSuccess)
		return;
	if(!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if(snap && nx >= selmon->wx && nx <= selmon->wx + selmon->ww
			&& ny >= selmon->wy && ny <= selmon->wy + selmon->wh) {
				if(abs(selmon->wx - nx) < snap)
					nx = selmon->wx;
				else if(abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
					nx = selmon->wx + selmon->ww - WIDTH(c);
				if(abs(selmon->wy - ny) < snap)
					ny = selmon->wy;
				else if(abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
					ny = selmon->wy + selmon->wh - HEIGHT(c);
				if(!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
					togglefloating(NULL);
			}
			if(!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, True);
			break;
		}
	} while(ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if((m = ptrtomon(c->x + c->w / 2, c->y + c->h / 2)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
	
	
}

Client *
nexttiled(Client *c) {
	for(; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

Monitor *
ptrtomon(int x, int y) {
	Monitor *m;

	for(m = mons; m; m = m->next)
		if(INRECT(x, y, m->wx, m->wy, m->ww, m->wh))
			return m;
	return selmon;
}

void
propertynotify(XEvent *e) {
	Client *c;
	Systray *s;//系统托盘
	Window trans;
	XPropertyEvent *ev = &e->xproperty;
	
	if((s = systray_find(ev->window)))
     {
          systray_state(s);
                   
          systray_update();
	 }

	if((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if(ev->state == PropertyDelete)
		return; /* ignore */
	else if((c = wintoclient(ev->window))) {
		switch (ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			XGetTransientForHint(dpy, c->win, &trans);
			if(!c->isfloating && (c->isfloating = (wintoclient(trans) != NULL)))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if(ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if(c == c->mon->sel)
				drawbar(c->mon);
		}
	}
}

void
clientmessage(XEvent *e) {
  XClientMessageEvent *cme = &e->xclient;
	Client *c;
	
	if((c = wintoclient(cme->window))
	&& (cme->message_type == netatom[NetWMState] && cme->data.l[1] == netatom[NetWMFullscreen]))
	{
		if(cme->data.l[0]) {
			XChangeProperty(dpy, cme->window, netatom[NetWMState], XA_ATOM, 32,
			                PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
			c->oldstate = c->isfloating;
			c->oldbw = c->bw;
			c->bw = 0;
			c->isfloating = 1;
			resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
			XRaiseWindow(dpy, c->win);
		}
		else {
			XChangeProperty(dpy, cme->window, netatom[NetWMState], XA_ATOM, 32,
			                PropModeReplace, (unsigned char*)0, 0);
			c->isfloating = c->oldstate;
			c->bw = c->oldbw;
			c->x = c->oldx;
			c->y = c->oldy;
			c->w = c->oldw;
			c->h = c->oldh;
			resizeclient(c, c->x, c->y, c->w, c->h);
			arrange(c->mon);
		}
	}
	else						// 增加系统托盘
	  if(cme->window == traywin) {
		if(cme->data.l[1] == XEMBED_EMBEDDED_NOTIFY){
			systray_add(cme->data.l[2]);
			systray_update();
			//注意刷新一下状态栏信息（显示dwm版本的东东，虽然用其他的信息显示代替它）
			
			updatestatus();
		}
		}
}

void
quit(const Arg *arg) {
    
	running = False;
}

void
resize(Client *c, int x, int y, int w, int h, Bool interact) {
	if(applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h) {
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg) {
			
	int ocx, ocy;
	int nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;

	if(!(c = selmon->sel))
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	                None, cursor[CurResize], CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if(snap && nw >= selmon->wx && nw <= selmon->wx + selmon->ww
			&& nh >= selmon->wy && nh <= selmon->wy + selmon->wh)
			{
				if(!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if(!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, True);
			break;
		}
	} while(ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if((m = ptrtomon(c->x + c->w / 2, c->y + c->h / 2)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
		
}

void
restack(Monitor *m) {
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if(!m->sel)
		return;
	if(m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if(m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for(c = m->stack; c; c = c->snext)
			if(!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void) {
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	
	//程序启动时自动执行autostart.sh脚本
	shexec("~/.dwm/autostart.sh");
	
	while(running && !XNextEvent(dpy, &ev)) {
		if(handler[ev.type])
			handler[ev.type](&ev); /* call handler */
	}
}

void
scan(void) {
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if(XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for(i = 0; i < num; i++) {
			if(!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if(wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for(i = 0; i < num; i++) { /* now the transients */
			if(!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if(XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if(wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m) {
	if(c->mon == m)
		return;
	unfocus(c, True);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state) {
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
			PropModeReplace, (unsigned char *)data, 2);
}

void
setlayout(const Arg *arg) {
	if(!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if(arg && arg->v)
	{
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
		g_layout[g_cur].arg.v = arg->v;
		g_layout[g_cur].tag = g_cur;
	}
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if(selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutly */
void
setmfact(const Arg *arg) {
	float f;

	if(!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if(f < 0.1 || f > 0.9)
		return;
	selmon->mfact = f;
	arrange(selmon);
}

void
setup(void) {
	XSetWindowAttributes wa;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init screen */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	initfont(font);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	//面板的高度在config.h中有定义：status_height
	int fh = dc.font.height;
	if(fh < status_height)
		bh = dc.h = status_height;
	else
	  bh = dc.h = fh + 2; //面板的最小高度是字体的高度（保证字体正常显示而不被掩盖）
	updategeom();
	/* init atoms */
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		
	/*系统托盘相关*/
	//netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	
	Screen *screenx = XDefaultScreenOfDisplay(dpy);
	int iScreen = XScreenNumberOfScreen(screenx);
	char systray_atom[48];
	snprintf(systray_atom, sizeof(systray_atom), "_NET_SYSTEM_TRAY_S%d", iScreen/*SCREEN*/);
	netatom[NetSystemTray] = ATOM(systray_atom);
        
	/* init cursors */
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
	/* init appearance */
    for(int i=0; i<NUMCOLORS; i++) {
        dc.colors[i][ColBorder] = getcolor( colors[i][ColBorder] );
        dc.colors[i][ColFG] = getcolor( colors[i][ColFG] );
        dc.colors[i][ColBG] = getcolor( colors[i][ColBG] );
    }
	dc.drawable = XCreatePixmap(dpy, root, DisplayWidth(dpy, screen), bh, DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, dc.gc, 1, LineSolid, CapButt, JoinMiter);
#ifndef XFT
	if(!dc.font.set)
		XSetFont(dpy, dc.gc, dc.font.xfont->fid);
#else
	xftdraw = XftDrawCreate(dpy, dc.drawable, XDefaultVisual(dpy, screen), DefaultColormap(dpy, screen));
#endif
	/* init bars */
	updatebars();
	updatestatus();
	/* 请求系统托盘 */
	systray_acquire();
	
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
			PropModeReplace, (unsigned char *) netatom, NetLast);
	/* select for events */
	wa.cursor     = cursor[CurNormal];
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask
	                |EnterWindowMask|LeaveWindowMask|StructureNotifyMask
	                |PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
}

void
showhide(Client *c) {
	if(!c)
		return;
	if(ISVISIBLE(c)) { /* show clients top down */
		if(c->needresize) {
			c->needresize=0;
			XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else {
			XMoveWindow(dpy, c->win, c->x, c->y);
		}
		if(!c->mon->lt[c->mon->sellt]->arrange || c->isfloating)
			resize(c, c->x, c->y, c->w, c->h, False);
		showhide(c->snext);
	}
	else { /* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, c->x + 2 * sw, c->y);
	}
}


void
sigchld(int unused) {
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg) {
	if(fork() == 0) {
		if(dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(0);
	}
}

void
tag(const Arg *arg) {
	if(selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg) {
	if(!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

int
textnw(const char *text, unsigned int len) {
	//返回文本的宽度，tag所占的宽度
    // remove non-printing color codes before calculating width
	char *ptr = (char *) text;
    unsigned int i, ibuf, lenbuf=len;
    char buf[len + 1];

    for(i=0, ibuf=0; *ptr && i<len; i++, ptr++) {
        if(*ptr <= NUMCOLORS && *ptr > 0) {
            if (i < len) { lenbuf--; }
        } else {
            buf[ibuf]=*ptr;
            ibuf++;
        }
    }
    buf[ibuf]=0;

#ifdef XFT
	XGlyphInfo g;
	XftTextExtentsUtf8(dpy, dc.font.xft, (XftChar8*)buf, lenbuf, &g);
    return g.xOff;
#else
	if(dc.font.set) {
	    XRectangle r;
		XmbTextExtents(dc.font.set, buf, lenbuf, NULL, &r);
		return r.width;
	} else {
	    return XTextWidth(dc.font.xfont, buf, lenbuf);
    }
	#endif
	
}

// 窗口左右平铺
void
tile(Monitor *m) {
	int x, y, h, w, mw;
	unsigned int i, n;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if(n == 0)
		return;
	/* master */
	c = nexttiled(m->clients);
	mw = m->mfact * m->ww;
	resize(c, m->wx, m->wy, (n == 1 ? m->ww : mw) - 2 * c->bw, m->wh - 2 * c->bw, False);
	if(--n == 0)
		return;
	/* tile stack */
	x = (m->wx + mw > c->x + c->w) ? c->x + c->w + 2 * c->bw : m->wx + mw;
	y = m->wy;
	w = (m->wx + mw > c->x + c->w) ? m->wx + m->ww - x : m->ww - mw;
	h = m->wh / n;
	if(h < bh)
		h = m->wh;
	for(i = 0, c = nexttiled(c->next); c; c = nexttiled(c->next), i++) {
		resize(c, x, y, w - 2 * c->bw, /* remainder */ ((i + 1 == n)
		       ? m->wy + m->wh - y - 2 * c->bw : h - 2 * c->bw), False);
		if(h != m->wh)
			y = c->y + HEIGHT(c);
	}
}

/*隐藏状态棒*/
void
togglebar(const Arg *arg) {
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	arrange(selmon);
	//隐藏托盘图标，实现原理：使托盘窗口的y为-bh，这样就到屏幕外面了，bh：状态栏高度，sh：屏幕高度
	if(systray_enable)
	{
		XWindowChanges wc;
		if(!selmon->showbar)
		{
			
			wc.y = -bh;
			XConfigureWindow(dpy, traywin, CWY, &wc);
		}
		else
		if(selmon->showbar)
		{
			if(topbar)
				wc.y = 0;
			else
				wc.y = sh - bh;
				
			XConfigureWindow(dpy, traywin, CWY, &wc);
		}
	}
}

void
togglefloating(const Arg *arg) {
	if(!selmon->sel)
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if(selmon->sel->isfloating)
		/*restore last known float dimensions*/
		resize(selmon->sel, selmon->sel->sfx, selmon->sel->sfy,
		       selmon->sel->sfw, selmon->sel->sfh, False);
	else {
		/*save last known float dimensions*/
		selmon->sel->sfx = selmon->sel->x;
		selmon->sel->sfy = selmon->sel->y;
		selmon->sel->sfw = selmon->sel->w;
		selmon->sel->sfh = selmon->sel->h;
	}
	arrange(selmon);
}

void
toggletag(const Arg *arg) {
	unsigned int newtags;

	if(!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if(newtags) {
		selmon->sel->tags = newtags;
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg) {
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if(newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;
		arrange(selmon);
	}
}

void
unfocus(Client *c, Bool setfocus) {
	if(!c)
		return;
	grabbuttons(c, False);
	XSetWindowBorder(dpy, c->win, dc.colors[0][ColBorder]);
	if(setfocus)
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
}

void
unmanage(Client *c, Bool destroyed) {
	Monitor *m = c->mon;
	XWindowChanges wc;

	/* The server grab construct avoids race conditions. */
	detach(c);
	detachstack(c);
	if(!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	arrange(m);
}

void
unmapnotify(XEvent *e) {
	Client *c;
	Systray *s;
	XUnmapEvent *ev = &e->xunmap;

	if((c = wintoclient(ev->window)))
		unmanage(c, False);
		
	if((s = systray_find(ev->window)))
     {
          systray_del(s);
          systray_update();
	  }	
}

void
updatebars(void) {
	Monitor *m;
	XSetWindowAttributes wa;

	wa.override_redirect = True;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ButtonPressMask|ExposureMask;
	for(m = mons; m; m = m->next) {
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
		                          CopyFromParent, DefaultVisual(dpy, screen),
		                          CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]);
		XMapRaised(dpy, m->barwin);
	}
}

void
updatebarpos(Monitor *m) {
	m->wy = m->my;
	m->wh = m->mh;
	if(m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	}
	else
		m->by = -bh;
}

Bool
updategeom(void) {
	Bool dirty = False;

#ifdef XINERAMA
	if(XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		info = XineramaQueryScreens(dpy, &nn);
		for(n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		if(!(unique = (XineramaScreenInfo *)malloc(sizeof(XineramaScreenInfo) * nn)))
			die("fatal: could not malloc() %u bytes\n", sizeof(XineramaScreenInfo) * nn);
		for(i = 0, j = 0; i < nn; i++)
			if(isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if(n <= nn) {
			for(i = 0; i < (nn - n); i++) { /* new monitors available */
				for(m = mons; m && m->next; m = m->next);
				if(m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for(i = 0, m = mons; i < nn && m; m = m->next, i++)
				if(i >= n
				|| (unique[i].x_org != m->mx || unique[i].y_org != m->my
				    || unique[i].width != m->mw || unique[i].height != m->mh))
				{
					dirty = True;
					m->num = i;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
					updatebarpos(m);
				}
		}
		else { /* less monitors available nn < n */
			for(i = nn; i < n; i++) {
				for(m = mons; m && m->next; m = m->next);
				while(m->clients) {
					dirty = True;
					c = m->clients;
					m->clients = c->next;
					detachstack(c);
					c->mon = mons;
					attach(c);
					attachstack(c);
				}
				if(m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	}
	else
#endif /* XINERAMA */
	/* default monitor setup */
	{
		if(!mons)
			mons = createmon();
		if(mons->mw != sw || mons->mh != sh) {
			dirty = True;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if(dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void) {
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for(i = 0; i < 8; i++)
		for(j = 0; j < modmap->max_keypermod; j++)
			if(modmap->modifiermap[i * modmap->max_keypermod + j]
			   == XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c) {
	long msize;
	XSizeHints size;

	if(!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if(size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	}
	else if(size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	}
	else
		c->basew = c->baseh = 0;
	if(size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	}
	else
		c->incw = c->inch = 0;
	if(size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	}
	else
		c->maxw = c->maxh = 0;
	if(size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	}
	else if(size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	}
	else
		c->minw = c->minh = 0;
	if(size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	}
	else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->minw && c->maxh && c->minh
	             && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatetitle(Client *c) {
	if(!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if(c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatestatus(void) {
	if(!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "dwmplus-"VERSION);
	drawbar(selmon);
}

void
updatewmhints(Client *c) {
	XWMHints *wmh;

	if((wmh = XGetWMHints(dpy, c->win))) {
		if(c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		}
		else
			c->isurgent = (wmh->flags & XUrgencyHint) ? True : False;
		XFree(wmh);
	}
}


static unsigned int get_bit_pos(unsigned int n)
{
	unsigned int i = 0;	
	if(n > 0){
		while((n >> i) != 1) {
			i++;
		}
	}
	return i;

}
void
view_layout(const Arg *arg) {
	if((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if(arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	arrange(selmon);
	g_cur = get_bit_pos(arg->ui);
	if(g_layout[g_cur].tag != 0xFF)
	{
		setlayout(&g_layout[g_cur].arg);
	}
}


void
view(const Arg *arg) {
	if((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if(arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	arrange(selmon);
}

Client *
wintoclient(Window w) {
	Client *c;
	Monitor *m;

	for(m = mons; m; m = m->next)
		for(c = m->clients; c; c = c->next)
			if(c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w) {
	int x, y;
	Client *c;
	Monitor *m;

	if(w == root && getrootptr(&x, &y))
		return ptrtomon(x, y);
	for(m = mons; m; m = m->next)
		if(w == m->barwin)
			return m;
	if((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int
xerror(Display *dpy, XErrorEvent *ee) {
	if(ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
			ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee) {
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee) {
	otherwm = True;
	return -1;
}

void
zoom(const Arg *arg) {
	Client *c = selmon->sel;

	if(!selmon->lt[selmon->sellt]->arrange
	|| selmon->lt[selmon->sellt]->arrange == monocle
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	if(c == nexttiled(selmon->clients))
		if(!c || !(c = nexttiled(c->next)))
			return;
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

int
main(int argc, char *argv[]) {
	if(argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION", © 2006-2010 dwm engineers, see LICENSE for details\n");
	else if(argc != 1)
		die("usage: dwm [-v]\n");
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if(!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display\n");
	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return 0;
        
}

//箭头键循环切换tag
static void
view_adjacent_tag(const Arg *arg, int distance)
{
    int i, curtags;
    int seltag = 0;
    Arg a;
	// 注意去掉：^ (arg->ui & TAGMASK)，否则当鼠标在标签页上滚动时会不受控制
	curtags = selmon->tagset[selmon->seltags];// ^ (arg->ui & TAGMASK);
	for (i = 0; i < LENGTH(tags); i++) {
        if ((curtags & (1 << i)) != 0) {
            seltag = i;
            break;
        }
    }
    
    seltag = (seltag + distance) % (int)LENGTH(tags);
    if (seltag < 0)
       seltag += LENGTH(tags);

    a.i = (1 << seltag);
    view(&a);
}

static void
view_next_tag(const Arg *arg)
{
    view_adjacent_tag(arg, +1);
}

static void
view_prev_tag(const Arg *arg)
{
    view_adjacent_tag(arg, -1);
}

// Restart function
void
restart(const Arg *arg)
{
		
	if (arg->v) {
		execvp(((char **)arg->v)[0], (char **)arg->v);
	} else {
		execlp("dwm", "dwm", NULL);
	}
		 
}

void
movestack(const Arg *arg) {
	Client		*c = NULL, *p = NULL, *pc = NULL, *i;
	// 如果当前没有活动窗口则返回，否则会导致dwm出错而重新登录
	if(!selmon->sel)
		return;
		
	if(arg->i > 0) {
		/* find the client after selmon->sel */
		for(c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
		if(!c)
			for(c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);

	}
	else {
		/* find the client before selmon->sel */
		for(i = selmon->clients; i != selmon->sel; i = i->next)
			if(ISVISIBLE(i) && !i->isfloating)
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i) && !i->isfloating)
					c = i;
	}
	/* find the client before selmon->sel and c */
	for(i = selmon->clients; i && (!p || !pc); i = i->next) {
		if(i->next == selmon->sel)
			p = i;
		if(i->next == c)
			pc = i;
	}

	/* swap c and selmon->sel selmon->clients in the selmon->clients list */
	if(c && c != selmon->sel) {
		Client *temp = selmon->sel->next==c?selmon->sel:selmon->sel->next;
		selmon->sel->next = c->next==selmon->sel?c:c->next;
		c->next = temp;

		if(p && p != c)
			p->next = c;
		if(pc && pc != selmon->sel)
			pc->next = selmon->sel;

		if(selmon->sel == selmon->clients)
			selmon->clients = c;
		else if(c == selmon->clients)
			selmon->clients = selmon->sel;

		arrange(selmon);
	}
	
}

static void
bstackhoriz(Monitor *m) {
	int x, y, h, w, mh;
	unsigned int i, n;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if(n == 0)
		return;
	/* master */
	c = nexttiled(m->clients);
	mh = m->mfact * m->wh;
	resize(c, m->wx, m->wy, m->ww - 2 * c->bw, (n == 1 ? m->wh : mh) - 2 * c->bw, False);
	if(--n == 0)
		return;
	/* tile stack */
	x = m->wx;
	y = (m->wy + mh > c->y + c->h) ? c->y + c->h + 2 * c->bw : m->wy + mh;
	w = m->ww;
	h = (m->wy + mh > c->y + c->h) ? m->wy + m->wh - y : m->wh - mh;
	h /= n;
	if(h < bh)
		h = m->wh;
	for(i = 0, c = nexttiled(c->next); c; c = nexttiled(c->next), i++) {
		resize(c, x, y, w - 2 * c->bw, /* remainder */ ((i + 1 == n)
		       ? m->wy + m->wh - y - 2 * c->bw : h - 2 * c->bw), False);
		if(h != m->wh)
			y = c->y + HEIGHT(c);
	}
}

Bool 
systray_acquire(void) {
	XSetWindowAttributes wattr;

	if(!systray_enable || traywin) return False;

	if(XGetSelectionOwner(dpy, netatom[NetSystemTray]) != None) {
		fprintf(stderr, "Can't initialize system tray: owned by another process\n");
		return False;
	}

	// Init traywin window
	wattr.event_mask        = ButtonPressMask | ExposureMask;
	wattr.override_redirect = True;
	wattr.background_pixmap = ParentRelative;
	wattr.background_pixel  = dc.colors[0][ColBG]; //取颜色索引是0（也就是第一种颜色）的背景
	
	//测试使用
	//traywin = XCreateSimpleWindow(dpy, root, 200, 100, 500, 500, 0, BlackPixel (dpy, 0), BlackPixel(dpy, 0)); 
	traywin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, dc.colors[0][ColBG]);
	
	XChangeWindowAttributes(dpy, traywin, CWEventMask | CWOverrideRedirect | CWBackPixel, &wattr);
	XSelectInput(dpy, traywin, KeyPressMask | ButtonPressMask);

	XMapRaised(dpy, traywin);

	XSetSelectionOwner(dpy, netatom[NetSystemTray], traywin, CurrentTime);

	if(XGetSelectionOwner(dpy, netatom[NetSystemTray]) != traywin) {
		systray_freeicons();
		fprintf(stderr, "System tray: can't get systray manager\n");
		return False;
	}
	// 很重要，如果不发送下面的消息，则比dwm更早运行的托盘程序有可能无法正常显示！
	dwm_send_message(ROOT, ROOT, "MANAGER", CurrentTime, netatom[NetSystemTray], traywin, 0, 0);

	XSync(dpy, False);
	
	return True;
}

void 
systray_add(Window win) {
	Systray *s;

	if(!systray_enable) return;

	s = zcalloc(sizeof(Systray));
	s->win = win;

	s->geo.height = bh;
	s->geo.width  = bh;

	XSelectInput(dpy, s->win, StructureNotifyMask | PropertyChangeMask| EnterWindowMask | FocusChangeMask);
	XReparentWindow(dpy, s->win, traywin, 0, 0);

	// Attach
	if(trayicons) trayicons->prev = s;

	s->next = trayicons;
	trayicons = s;

	return;
}

void 
systray_del(Systray *s) {
	Systray **ss;

	if(!systray_enable) return;

	for(ss = &trayicons; *ss && *ss != s; ss = &(*ss)->next);
	*ss = s->next;

	IFREE(s);
	return;
}

void 
systray_freeicons(void) {
	Systray *i;

	if(!systray_enable) return;

	for(i = trayicons; i; i = i->next) {
		XUnmapWindow(dpy, i->win);
		XReparentWindow(dpy, i->win, ROOT, 0, 0);
		IFREE(i);
	}

	XSetSelectionOwner(dpy, netatom[NetSystemTray], None, CurrentTime);
	XDestroyWindow(dpy, traywin);
	XSync(dpy, 0);

	return;
}

Systray* 
systray_find(Window win) {
	Systray *i;

	if(!systray_enable) return NULL;

	for(i = trayicons; i; i = i->next)
		if(i->win == win) return i;

	return NULL;
}

int 
systray_get_width(void) {
	int w = 0;
	Systray *i;

	if(!systray_enable) return 0;

	for(i = trayicons; i; i = i->next)
		w += i->geo.width + systray_spacing;

	return w;
}

void 
systray_update(void) {
	Systray *i;
	Monitor *m;
	int x = 1;
	//sw为屏幕的宽度
	int pos = sw;//blw;
	int pos_y;
	//判断状态栏的位置以决定系统托盘的位置
	if(topbar)
		pos_y = 0;
	else
		pos_y = sh - bh; //sh：屏幕的高度  bh：面板的高度	

	if(!systray_enable) return;

	for(m = mons; m; m = m -> next){
		if(m->primary == 1) pos = m -> ww;
	}

	//如果不存在系统托盘程序
	if(!trayicons) {
		pos -= 1;
		XMoveResizeWindow(dpy, traywin, pos, 0, 1, 1);
		return;
	}

	//遍历托盘程序
	for(i = trayicons; i; i = i->next) {
		XMapWindow(dpy, i->win);
		XMoveResizeWindow(dpy, i->win, (i->geo.x = x), 0, i->geo.width, i->geo.height);

		x += i->geo.width + systray_spacing;
	}

	pos -= x;
	XMoveResizeWindow(dpy, traywin, pos, pos_y, x, bh);

	return;
}

void
systray_state(Systray *s)
{
     long flags;
     int code = 0;

     if(!(flags = ewmh_get_xembed_state(s->win)) ||!systray_enable)
          return;

     if(flags & XEMBED_MAPPED)
     {
          code = XEMBED_WINDOW_ACTIVATE;
          XMapRaised(dpy, s->win);
          setwinstate(s->win, NormalState);
     }
     else
     {
          code = XEMBED_WINDOW_DEACTIVATE;
          XUnmapWindow(dpy, s->win);
          setwinstate(s->win, WithdrawnState);
     }

     dwm_send_message(s->win, s->win, "_XEMBED", CurrentTime, code, 0, 0, 0);

     return;
}

long
ewmh_get_xembed_state(Window win)
{
     Atom rf;
     int f;
     ulong n, il;
     long ret = 0;
     uchar *data = NULL;

     if(XGetWindowProperty(dpy, win, ATOM("_XEMBED_INFO"), 0L, 2, False,
                    ATOM("_XEMBED_INFO"), &rf, &f, &n, &il, &data) != Success)
          return 0;

     if(rf == ATOM("_XEMBED_INFO") && n == 2)
          ret = (long)data[1];

     if(n && data)
          XFree(data);

     return ret;
}

void
setwinstate(Window win, long state)
{
     long data[] = {state, None};

     XChangeProperty(dpy, win, ATOM("WM_STATE"), ATOM("WM_STATE"), 32,
						
						PropModeReplace, (uchar *)data, 2);

     return;
}

/** Send ewmh message  */
void
dwm_send_message(Window d, Window w, char *atom, long d0, long d1, long d2, long d3, long d4)
{

     XClientMessageEvent e;

     e.type          = ClientMessage;
     e.message_type  = ATOM(atom);
     e.window        = w;
     e.format        = 32;
     e.data.l[0]     = d0;
     e.data.l[1]     = d1;
     e.data.l[2]     = d2;
     e.data.l[3]     = d3;
     e.data.l[4]     = d4;

     XSendEvent(dpy, d, False, StructureNotifyMask, (XEvent*)&e);
     XSync(dpy, False);

     return;
}

// 鼠标点击窗口标题显示区域切换窗口
void
focusonclick(const Arg *arg) {
       int x, w, mw = 0, tw, n = 0, i = 0, extra = 0;
       Monitor *m = selmon;
       Client *c, *firstvis;
        
       // 寻找第一个可视的窗口
	for(c = m->clients; c && !ISVISIBLE(c); c = c->next);
	firstvis								  = c;
	// 列举当前标签页中所有可视的窗口并记录在变量n中
	for(c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
		    n++;
	// 如果当前有可视的窗口（有应用程序打开）
	if(n > 0) {
		// 计算当前可视窗口标题宽度的平均值
		mw									  = (m->titlebarend - m->titlebarbegin) / n;
		c									  = firstvis;
		// 获取第一个窗口的标题栏宽度
		while(c) {
			tw								  = TEXTW(c->name);
			// 标题栏宽度小于平均宽度，则由extra来记录它们之间的差值，否则由变量i记录当前的窗口数量
			if(tw < mw) extra				  += (mw - tw); else i++;
			// 下一个没有获得焦点的窗口（不可视的窗口）
			for(c = c->next; c && !ISVISIBLE(c); c = c->next);
 		}
		// 重新计算窗口标题的平均宽度，mw
		if(i > 0) mw += extra / i;
	}

	x=m->titlebarbegin;

	c = firstvis;
        while(x < m->titlebarend) {
		if(c) {
			// w等于最小值（窗口标题宽度和窗口标题的平均宽度）
			w =	MIN(TEXTW(c->name), mw);
			// 如果鼠标在某个窗口标题上面点击，则聚焦这个窗口
			if (x < arg->i && x+w > arg->i) {
				focus(c);
				restack(selmon);
				
				break;
			} else
				// 重新给开始值
				x+=w;
			// 寻找下一个可视窗口
			for(c = c->next; c && !ISVISIBLE(c); c = c->next);
		}
		else {// 如果找不到可视窗口，则终止while循环
			break;
		}
        }
		
        }

// 鼠标右键点击窗口标题显示区域关闭该窗口
void
closeonclick(const Arg *arg) {
	   int x, w, mw = 0, tw, n = 0, i = 0, extra = 0;
       Monitor *m = selmon;
       Client *c, *firstvis;
        
       // 寻找第一个可视的窗口
	for(c = m->clients; c && !ISVISIBLE(c); c = c->next);
	firstvis								  = c;
	// 列举当前标签页中所有可视的窗口并记录在变量n中
	for(c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
		    n++;
	// 如果当前有可视的窗口（有应用程序打开）
	if(n > 0) {
		// 计算当前可视窗口标题宽度的平均值
		mw									  = (m->titlebarend - m->titlebarbegin) / n;
		c									  = firstvis;
		// 获取第一个窗口的标题栏宽度
		while(c) {
                    tw								  = TEXTW(c->name);
                    // 标题栏宽度小于平均宽度，则由extra来记录它们之间的差值，否则由变量i记录当前的窗口数量
                    if(tw < mw) extra				  += (mw - tw); else i++;
                    // 下一个没有获得焦点的窗口（不可视的窗口）
                    for(c = c->next; c && !ISVISIBLE(c); c = c->next);
 		}
		// 重新计算窗口标题的平均宽度，mw
		if(i > 0) mw += extra / i;
	}

	x=m->titlebarbegin;

	c                 = firstvis;
        while(x < m->titlebarend) {
		if(c) {
			// w等于最小值（窗口标题宽度和窗口标题的平均宽度）
			w = MIN(TEXTW(c->name), mw);
			// 如果鼠标在某个窗口标题上面点击，则聚焦这个窗口
			if (x < arg->i && x+w > arg->i) {
				focus(c);
				restack(selmon);
                killclient(0);//关闭被选择的窗口
				break;
			} else
				// 重新给开始值
				x+=w;
			// 寻找下一个可视窗口
			for(c = c->next; c && !ISVISIBLE(c); c = c->next);
		}
		else {// 如果找不到可视窗口，则终止while循环
			break;
		}
        }
}

void launcher(const Arg *arg){
    int x, pos;
    unsigned int i , tcount = 0;
    const char prompt[] = "Run: ";
    char tmp[32];
    char buf[256];
    Bool grabbing = True;
    KeySym ks;
    XEvent ev;

    // Clear the array
    memset(tmp, 0, sizeof(tmp));
    memset(buf, 0, sizeof(buf));
    pos = 0;

    // Initial x position
    x = 0;
    
    if(autohide) 
		{
          //if((occ & 1 << i ) || (selmon->tagset[selmon->seltags] & 1 << i)) // 自动隐藏没有内容的tag
              tcount = tagcount;
        }        
	else // 非自动隐藏
       tcount = LENGTH(tags);
        
    for(i = 0; i < tcount; i++) x+= TEXTW(tags[i]);
    x += TEXTW(selmon->ltsymbol);
    dc.x = x;
    dc.w = selmon->ww - x - textnw(stext , strlen(stext)) - dc.font.height/2; //TEXTW(stext);
    if(systray_enable) dc.w -= systray_get_width(); // primary monitor has systray

    XGrabKeyboard(dpy, ROOT, True, GrabModeAsync, GrabModeAsync, CurrentTime);

    drawtext(prompt, dc.colors[7], False);
    dc.x += TEXTW(prompt);
    XDrawLine(dpy, dc.drawable, dc.gc, dc.x, 3, dc.x, bh-3);

    XCopyArea(dpy, dc.drawable, selmon -> barwin, dc.gc, x, 0, dc.w, bh, x, 0);
    XSync(dpy, False);

    while(grabbing){
        if(ev.type == KeyPress) {
            XLookupString(&ev.xkey, tmp, sizeof(tmp), &ks, 0);

            switch(ks){
                case XK_Return:
                    shexec(buf);
                    grabbing = False;
                    break;
                case XK_BackSpace:
                    if(pos) buf[--pos] = 0;
                    break;
                case XK_Escape:
                    grabbing = False;
                    break;
                default:
                    strncat(buf, tmp, sizeof(tmp));
                    ++pos;
                    break;
            }
        drawtext(buf, dc.colors[7], False);
        XDrawLine(dpy, dc.drawable, dc.gc, dc.x+TEXTW(buf), 3, dc.x+TEXTW(buf), bh-3);

        XCopyArea(dpy, dc.drawable, selmon->barwin, dc.gc, dc.x, 0, dc.w-TEXTW(prompt), bh, dc.x, 0);
        XSync(dpy, False);
        }
        XNextEvent(dpy, &ev);
    }

    // Restore bar
    drawbar(selmon);

    XUngrabKeyboard(dpy, CurrentTime);
    return;
}

pid_t shexec(const char *cmd){
    char *sh = NULL;
    pid_t pid;

    if(!(sh = getenv("SHELL"))) sh = "/bin/sh";
    
    if((pid = fork()) == 0){
        if(dpy) close(ConnectionNumber(dpy));

        setsid();
        execl(sh, sh, "-c", cmd, (char*)NULL);
        err(1, "execl(%s)", cmd);
    }
    if (pid == -1)
        warn("fork()");

    return pid;
}

pid_t
tpawn(const char *format, ...)
{
     char *sh = NULL;
     char cmd[512];
     va_list ap;
     pid_t pid;
     size_t len;

     va_start(ap, format);
     len = vsnprintf(cmd, sizeof(cmd), format, ap);
     va_end(ap);

     if (len >= sizeof(cmd))
     {
          warnx("command too long (> 512 bytes)");
          return -1;
     }

     if(!(sh = getenv("SHELL")))
          sh = "/bin/sh";

     if((pid = fork()) == 0)
     {
          if(dpy)
               close(ConnectionNumber(dpy));
          setsid();
          if (execl(sh, sh, "-c", cmd, (char*)NULL) == -1)
               warn("execl(sh -c %s)", cmd);
          exit(EXIT_FAILURE);
     }
     else if (pid == -1)
          warn("fork");

     return pid;
}

void
grid(Monitor *m) {
	unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
		n++;

	/* grid dimensions */
	for(rows = 0; rows <= n/2; rows++)
		if(rows*rows >= n)
			break;
	cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

	/* window geoms (cell height/width) */
	ch = m->wh / (rows ? rows : 1);
	cw = m->ww / (cols ? cols : 1);
	for(i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
		cx = m->wx + (i / rows) * cw;
		cy = m->wy + (i % rows) * ch;
		/* adjust height/width of last row/column's windows */
		ah = ((i + 1) % rows == 0) ? m->wh - ch * rows : 0;
		aw = (i >= rows * (cols - 1)) ? m->ww - cw * cols : 0;
		resize(c, cx, cy, cw - 2 * c->bw + aw, ch - 2 * c->bw + ah, False);
		i++;
	}
}

void
bstack(Monitor *m) {
	int x, y, h, w, mh;
	unsigned int i, n;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if(n == 0)
		return;
	/* master */
	c = nexttiled(m->clients);
	mh = m->mfact * m->wh;
	resize(c, m->wx, m->wy, m->ww - 2 * c->bw, (n == 1 ? m->wh : mh) - 2 * c->bw, False);
	if(--n == 0)
		return;
	/* tile stack */
	x = m->wx;
	y = (m->wy + mh > c->y + c->h) ? c->y + c->h + 2 * c->bw : m->wy + mh;
	w = m->ww / n;
	h = (m->wy + mh > c->y + c->h) ? m->wy + m->wh - y : m->wh - mh;
	if(w < bh)
		w = m->ww;
	for(i = 0, c = nexttiled(c->next); c; c = nexttiled(c->next), i++) {
		resize(c, x, y, /* remainder */ ((i + 1 == n)
		       ? m->wx + m->ww - x - 2 * c->bw : w - 2 * c->bw), h - 2 * c->bw, False);
		if(w != m->ww)
			x = c->x + WIDTH(c);
	}
}




