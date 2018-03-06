#ifndef SI_DB_OPS_H

#define SI_DB_OPS_H

extern void setup_sqlite(void);
extern int  new_deck(char *fname);
extern int  new_slide(int deckid, int slidenum);
extern void new_word(char *w, int slideid, char origin, short kw);
extern void new_word_as_is(char *w, int slideid, char origin, short kw);
extern void remove_words(void);
extern void stem_words(void);
extern void generate_list(void);
extern void generate_index(char debug);

#endif
