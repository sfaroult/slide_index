#ifndef SI_UTIL_H

#define SI_UTIL_H

extern char        *cleanup(char *w);
extern void         replace_entities(char *str);
extern short        lowercase_word(char *w);
extern unsigned int utf8_to_codepoint(const unsigned char *u,
                                      int                 *lenptr);

#endif
