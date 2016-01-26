#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "slide_index.h"

static short    G_modus = OPT_GENERATE;
static short    G_slides_per_page = 0;
static char     G_sep = ';';
static char     G_kw = '@';
static char     G_autokw = 1;

extern void disable_autokw(void) {
   G_autokw = 0;
}

extern char autokw(void) {
   return G_autokw;
}

extern void set_sep(char sep) {
   G_sep = sep;
}

extern char get_sep(void) {
   return G_sep;
}

extern void set_kw(char kw) {
   G_kw = kw;
}

extern char get_kw(void) {
   return G_kw;
}

extern void set_mode(short option) {
   G_modus |= option;
}

extern short get_mode(void) {
   return G_modus;
}

extern void set_pages(short val) {
   G_slides_per_page = val;
}

extern short get_pages(void) {
   return G_slides_per_page;
}
