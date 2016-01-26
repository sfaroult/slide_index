#ifndef SI_TREE_OPS_H

#define SI_TREE_OPS_H

extern void  load_index(char *fname);
extern void  load_stop(char *fname);
extern void  free_trees(void);
extern char *must_index(char *w, short *kw_ptr);
extern char  must_ignore(char *w);
extern void  record_special(char *str, int slideid, char origin);

// The following function returns the index entry as read from
// the index file if there was one, or it's argument if there was none.
// The goal is to let people have some control over capitalization
// in the index that is generated through the file that contains words
// to index.
extern char *index_entry(char *w, short *kw_ptr);

#endif
