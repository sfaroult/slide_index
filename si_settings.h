#ifndef SI_SETTINGS_H

#define SI_SETTINGS_H

extern void  set_mode(short option);
extern short get_mode(void);
extern void  set_pages(short val);
extern short get_pages(void);
extern void  disable_autokw(void);
extern void  enable_autofunc(void);
extern char  autokw(void);
extern char  autofunc(void);
extern void  set_sep(char sep);
extern char  get_sep(void);
extern void  set_kw(char kw);
extern char  get_kw(void);

#endif
