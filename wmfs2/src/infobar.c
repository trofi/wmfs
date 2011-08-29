/*
 *  wmfs2 by Martin Duquesnoy <xorg62@gmail.com> { for(i = 2011; i < 2111; ++i) ©(i); }
 *  For license, see COPYING.
 */

#include "wmfs.h"
#include "draw.h"
#include "infobar.h"
#include "barwin.h"
#include "util.h"

#define ELEM_DEFAULT_ORDER "tlsS"
#define INFOBAR_DEF_W (12)

#define ELEMX(e, head) TAILQ_PREV(e, head, next)->geo.

static void infobar_elem_tag_init(Element *e);
static void infobar_elem_tag_update(Element *e);

const struct elem_funcs
{
     char c;
     void (*func_init)(Element *e);
     void (*func_update)(Element *e);
} elem_funcs[] =
{
     { 't', infobar_elem_tag_init, infobar_elem_tag_update },

     /* { 'l',  infobar_elem_layout_init, infobar_elem_layout_update },
        { 's',  infobar_elem_selbar_init, infobar_elem_selbar_update },
        { 'S',  infobar_elem_status_init, infobar_elem_status_update },*/

     { '\0', NULL, NULL }
};

static inline void
infobar_elem_placement(Element *e)
{
     Element *p = TAILQ_PREV(e, esub, next);

     e->geo.y = 0;
     e->geo.w = 0;
     e->geo.h = e->infobar->geo.h;
     e->geo.x = (p ? p->geo.x + p->geo.w + PAD : 0);
}

static void
infobar_elem_tag_init(Element *e)
{
     Tag *t;
     Barwin *b, *prev;
     int tmp, j;

     infobar_elem_placement(e);

     j = e->geo.x;

     TAILQ_FOREACH(t, &e->infobar->screen->tags, next)
     {
          tmp = draw_textw(t->name) + PAD;

          b = barwin_new(e->infobar->bar->win, j, 0, tmp, e->geo.h, 0x009900, 0x777777, false);
          b->ptr = (void*)t;
          barwin_map(b);

          if(SLIST_EMPTY(&e->bars))
               SLIST_INSERT_HEAD(&e->bars, b, next);
          else
               SLIST_INSERT_AFTER(prev, b, next);

          prev = b;
          b = NULL;
          j += tmp;
     }
}

static void
infobar_elem_tag_update(Element *e)
{
     Tag *t, *sel = e->infobar->screen->seltag;
     Barwin *b;

     SLIST_FOREACH(b, &e->bars, next)
     {
          t = (Tag*)b->ptr;

          /* Selected */
          if(t == sel)
          {
               b->fg = 0x000000;
               b->bg = 0x3D5700;
          }
          else
          {
               b->fg = 0x3D5700;
               b->bg = 0x000000;
          }

          barwin_refresh_color(b);

          draw_text(b->dr, (PAD >> 1), TEXTY(e->geo.h), b->fg, t->name);

          barwin_refresh(b);
     }
}

void
infobar_elem_init(Infobar *i)
{
     Element *e;
     int n, j;

     for(n = 0; n < strlen(i->elemorder); ++n)
     {
          for(j = 0; j < LEN(elem_funcs); ++j)
               if(elem_funcs[j].c == i->elemorder[n])
               {
                    e = xcalloc(1, sizeof(Element));

                    SLIST_INIT(&e->bars);

                    e->infobar = i;
                    e->type = j;
                    e->func_init = elem_funcs[j].func_init;
                    e->func_update = elem_funcs[j].func_update;

                    TAILQ_INSERT_TAIL(&i->elements, e, next);

                    e->func_init(e);

                    break;
               }
     }
}

void
infobar_elem_update(Infobar *i)
{
     Element *e;

     TAILQ_FOREACH(e, &i->elements, next)
          if(i->elemupdate & FLAGINT(e->type))
               e->func_update(e);
}

void
infobar_elem_remove(Element *e)
{
     Barwin *b;

     TAILQ_REMOVE(&e->infobar->elements, e, next);

     while(!SLIST_EMPTY(&e->bars))
     {
          b = SLIST_FIRST(&e->bars);
          SLIST_REMOVE_HEAD(&e->bars, next);
          barwin_remove(b);
     }
}

void
infobar_init(void)
{
     Infobar *i;
     Scr33n *s;

     SLIST_FOREACH(s, &W->h.screen, next)
     {
          i = (Infobar*)xcalloc(1, sizeof(Infobar));

          i->screen = s;
          i->elemorder = xstrdup(ELEM_DEFAULT_ORDER);
          TAILQ_INIT(&i->elements);

          /* Positions TODO: geo = infobar_position(Position {Top,Bottom,Hidden}) */
          i->geo = s->geo;
          i->geo.h = INFOBAR_DEF_W;

          /* Barwin create */
          i->bar = barwin_new(W->root, i->geo.x, i->geo.y, i->geo.w, i->geo.h, 0x222222, 0xCCCCCC, false);

          /* Render */
          barwin_map(i->bar);
          barwin_map_subwin(i->bar);
          barwin_refresh_color(i->bar);

          infobar_elem_init(i);
          infobar_refresh(i);

          SLIST_INSERT_HEAD(&s->infobars, i, next);
     }
}

void
infobar_refresh(Infobar *i)
{
     draw_text(i->bar->dr, 0, TEXTY(INFOBAR_DEF_W), 0x005500, "|");

     i->elemupdate |= FLAGINT(ElemTag);

     infobar_elem_update(i);

     barwin_refresh(i->bar);
}

void
infobar_remove(Infobar *i)
{
     Element *e;

     free(i->elemorder);

     TAILQ_FOREACH(e, &i->elements, next)
          infobar_elem_remove(e);

     barwin_remove(i->bar);

     SLIST_REMOVE(&i->screen->infobars, i, Infobar, next);

     free(i);
}

void
infobar_free(Scr33n *s)
{
     Infobar *i;

     while(!SLIST_EMPTY(&s->infobars))
     {
          i = SLIST_FIRST(&s->infobars);
          infobar_remove(i);
     }
}






