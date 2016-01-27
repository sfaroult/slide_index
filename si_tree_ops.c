#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "si_tree_ops.h"
#include "si_db_ops.h"
#include "slide_index.h"

#define  MAX_LINE_LEN    1024

typedef struct node {
                     char        *word;
                     short        kw;
                     struct node *variant_of;
                     struct node *left;
                     struct node *right;
                    } NODE_T;

static NODE_T       *G_tree = (NODE_T *)NULL;
static NODE_T       *G_special = (NODE_T *)NULL;
static NODE_T       *G_stop = (NODE_T *)NULL;

static NODE_T *tree_find(NODE_T *t, char *w) {
   if (t) {
     if (w) {
       int pos;

       if ((pos = strcasecmp(w, t->word)) == 0) {
         return t;
       } else {
         if (pos < 0) {
           return tree_find(t->left, w);
         } else {
           return tree_find(t->right, w);
         }
       }
     }
   }
   return (NODE_T *)NULL;
}

static NODE_T *tree_add2(NODE_T **tptr,
                         NODE_T *root,
                         char *w, short kw,
                         char *stem, char *added) {
    if (tptr && w && added) {
      if (*tptr == (NODE_T *)NULL) {
        if ((*tptr = (NODE_T *)malloc(sizeof(NODE_T))) != (NODE_T *)NULL) {
          (*tptr)->word = strdup(w);
          (*tptr)->kw = kw;
          (*tptr)->variant_of = tree_find(root, stem);
          (*tptr)->left = (NODE_T *)NULL;
          (*tptr)->right = (NODE_T *)NULL;
          *added = 1;
        }
      } else {
        int pos;

        if ((pos = strcasecmp(w, (*tptr)->word)) == 0) {
          *added = 0;
        } else {
          if (pos < 0) {
            return tree_add2(&((*tptr)->left), root, w, kw, stem, added);
          } else {
            return tree_add2(&((*tptr)->right), root, w, kw, stem, added);
          }
        }
      }
      return *tptr;
    }
    return (NODE_T *)NULL;
}


static NODE_T *tree_add(NODE_T **tptr, char *w, char *added) {
    if (tptr && w && added) {
      if (*tptr == (NODE_T *)NULL) {
        if ((*tptr = (NODE_T *)malloc(sizeof(NODE_T))) != (NODE_T *)NULL) {
          (*tptr)->word = strdup(w);
          (*tptr)->variant_of = (NODE_T *)NULL;
          (*tptr)->left = (NODE_T *)NULL;
          (*tptr)->right = (NODE_T *)NULL;
          *added = 1;
        }
      } else {
        int pos;

        if ((pos = strcasecmp(w, (*tptr)->word)) == 0) {
          *added = 0;
        } else {
          if (pos < 0) {
            return tree_add(&((*tptr)->left), w, added);
          } else {
            return tree_add(&((*tptr)->right), w, added);
          }
        }
      }
      return *tptr;
    }
    return (NODE_T *)NULL;
}

static void tree_free(NODE_T **tptr) {
   if (tptr && *tptr) {
     tree_free(&((*tptr)->left));
     tree_free(&((*tptr)->right));
     if ((*tptr)->word) {
       free((*tptr)->word);
     }
     free(*tptr);
     *tptr = (NODE_T *)NULL;
   }
}

static char tree_contains(NODE_T *t, char *w) {
   return (tree_find(t, w) == (NODE_T *)NULL ? (char)0 : (char)1);
}

static char *tree_entry(NODE_T *t, char *w, short *kw_ptr) {
   NODE_T *e;

   if ((e = tree_find(t, w)) != (NODE_T *)NULL) {
     if (e->variant_of) {
       e = e->variant_of;
     } 
     *kw_ptr = e->kw;
     return e->word;
   }
   return (char *)NULL;
}

extern char *index_entry(char *w, short *kw_ptr) {
    if (G_tree) {
      return tree_entry(G_tree, w, kw_ptr);
    }
    return w;
}

extern void load_index(char *fname) {
    // Something special here:
    //   "words" that are to index and aren't regular words
    //   (operators such as || or &&, comment delimiters
    //    and the like) will be stored inside a special in-memory
    //    tree.
    // Also, some words may be marked as keywords, and variants
    // may be supplied on the same line (the official index entry
    // will be the first one on the line)
    //
    FILE    *fp;
    char     line[MAX_LINE_LEN];
    char    *w;
    char    *p;
    char    *official;
    char     kw;
    int      len;
    char     added;
    char     sep[2];
    NODE_T  *n;
    NODE_T **ref_ptr = (NODE_T **)NULL;

    if (fname) {
      sep[0] = get_sep();
      sep[1] = '\0';
      if ((fp = fopen(fname, "r")) != (FILE *)NULL) {
         while (fgets(line, MAX_LINE_LEN, fp)) {
           w = line;
           while (isspace(*w)) {
             w++;
           }
           p = strtok(w, sep);
           if (p) {
             official = p;
             if (*p == get_kw()) {
               p++;
               kw = 1;
             } else {
               kw = 0;
             }
             len = strlen(p);
             while (len && isspace(p[len-1])) {
               len--;
             }
             p[len] = '\0';
             if (len) {
               if (isalpha(*p)) {
                 ref_ptr = &G_tree;
               } else {
                 ref_ptr = &G_special;
               }
               n = tree_add2(ref_ptr, *ref_ptr,
                             p, kw, (char *)NULL, &added);
             }
             while ((p = strtok(NULL, sep)) != (char *)NULL) {
               len = strlen(p);
               while (len && isspace(p[len-1])) {
                 len--;
               }
               p[len] = '\0';
               if (len && ref_ptr) {
                 if (isalpha(*w)) {
                   n = tree_add2(&G_tree, *ref_ptr,
                                 p, 0, official, &added);
                 } else {
                   n = tree_add2(&G_special, *ref_ptr,
                                 p, 0, official, &added);
                 }
               }
             }
           }
         }
         fclose(fp);
      } else {
        perror(fname);
        exit(1);
      }
    }
}

extern void load_stop(char *fname) {
    FILE   *fp;
    char    line[MAX_WORD_LEN+3];
    char   *w;
    int     len;
    char    added;
    NODE_T *n;

    if (fname) {
      if ((fp = fopen(fname, "r")) != (FILE *)NULL) {
         while (fgets(line, MAX_WORD_LEN+3, fp)) {
           w = line;
           while (isspace(*w)) {
             w++;
           }
           len = strlen(w);
           while (len && isspace(w[len-1])) {
             len--;
           }
           w[len] = '\0';
           if (len) {
             n = tree_add(&G_stop, w, &added);
           }
         }
         fclose(fp);
      } else {
        perror(fname);
        exit(1);
      }
    }
}

extern void free_trees(void) {
  tree_free(&G_tree);
  tree_free(&G_special);
  tree_free(&G_stop);
}

extern char *must_index(char *w, short *kw_ptr) {
   if (G_tree == (NODE_T *)NULL) {
     return (char *)NULL;
   }
   return tree_entry(G_tree, w, kw_ptr);
}

extern char must_ignore(char *w) {
   if (G_stop == (NODE_T *)NULL) {
     return (char)0;
   }
   return tree_contains(G_stop, w);
}

static void search_special(NODE_T *tree, char *str, int slideid, char origin) {
   short  kw;
   char  *entry;

   if (tree) {
     search_special(tree->left, str, slideid, origin);
     search_special(tree->right, str, slideid, origin);
     if (strstr(str, tree->word)) {
       if (tree->variant_of) {
         entry = (tree->variant_of)->word;
         kw = (tree->variant_of)->kw;
       } else {
         entry = tree->word;
         kw = tree->kw;
       } 
       new_word(entry, slideid, origin, kw);
     }
   }
}

extern void record_special(char *str, int slideid, char origin) {
   if (G_special) {
     search_special(G_special, str, slideid, origin);
   }
}
