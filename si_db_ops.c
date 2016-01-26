#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "sqlite3.h"
#include "si_tree_ops.h"
#include "si_db_ops.h"
#include "slide_index.h"

static sqlite3      *G_db = (sqlite3 *)NULL;
static sqlite3_stmt *G_ins_deck = (sqlite3_stmt *)NULL;
static sqlite3_stmt *G_ins_slide = (sqlite3_stmt *)NULL;
static sqlite3_stmt *G_sel_slide = (sqlite3_stmt *)NULL;
static sqlite3_stmt *G_ins_word = (sqlite3_stmt *)NULL;
static sqlite3_stmt *G_ins_word_as_is = (sqlite3_stmt *)NULL;
static sqlite3_stmt *G_upd_word = (sqlite3_stmt *)NULL;
static sqlite3_stmt *G_upd_word_as_is = (sqlite3_stmt *)NULL;


static void finalize_sqlite() {
    if (G_ins_deck) {
      (void)sqlite3_finalize(G_ins_deck);
    }
    if (G_ins_slide) {
      (void)sqlite3_finalize(G_ins_slide);
    }
    if (G_sel_slide) {
      (void)sqlite3_finalize(G_sel_slide);
    }
    if (G_ins_word) {
      (void)sqlite3_finalize(G_ins_word);
    }
    if (G_ins_word_as_is) {
      (void)sqlite3_finalize(G_ins_word_as_is);
    }
    if (G_upd_word) {
      (void)sqlite3_finalize(G_upd_word);
    }
    if (G_upd_word_as_is) {
      (void)sqlite3_finalize(G_upd_word_as_is);
    }
    if (G_db) {
      (void)sqlite3_close(G_db);
    }
}

extern void setup_sqlite() {
     int           i;
     sqlite3_stmt *stmt = (sqlite3_stmt *)NULL;
     char         *ztail = (char *)NULL;
     char         *create[] = {"pragma foreign_keys=true",
                               "create table decks"
                               " (deckid integer primary key not null,"
                               "  filename  varchar(250) not null,"
                               "  shortname varchar(30)  not null,"
                               " constraint slides_uq unique(filename))",
                               "create table slides"
                               " (slideid integer primary key not null,"
                               "  deckid    int not null,"
                               "  slidenum  int not null,"
                               " constraint slides_uq unique(deckid,"
                               "                             slidenum),"
                               " constraint slides_fk"
                               "        foreign key(deckid)"
                               "        references decks(deckid))",
                               "create table words"
                               " (word      varchar(60) not null,"
                               "  kw        char(1) not null default 'N',"
                               "  slideid   int not null,"
                               "  counter   int not null default 1,"
                               "  origin    char(1) not null,"
                               "  stem      varchar(30),"
                               " constraint words_pk primary key(word,"
                               "       slideid,origin),"
                               " constraint words_fk"
                               "        foreign key(slideid)"
                               "        references slides(slideid))",
                               "create index words_idx"
                               " on words(slideid,word)",
                               "create index words_idx2"
                               " on words(stem)",
                               NULL};
    
     if (sqlite3_open(":memory:", &G_db) != SQLITE_OK) {
        fprintf(stderr, "sqlite3_open: %s\n", (char *)sqlite3_errmsg(G_db));
        exit(1);
     }
     i = 0;
     while (create[i]) {
       if ((sqlite3_prepare(G_db,
                            (const char *)create[i],
                            -1,
                            &stmt,
                            (const char **)&ztail) != SQLITE_OK)
            || (sqlite3_step(stmt) != SQLITE_DONE)) {
         fprintf(stderr, "%s: %s\n",
                         create[i],
                         (char *)sqlite3_errmsg(G_db));
         (void)sqlite3_close(G_db);
         exit(1);
       }
       (void)sqlite3_finalize(stmt);
       i++;
     } 
     // Prepare statements that will be repeatedly executed
     if ((sqlite3_prepare(G_db,
                          "insert into decks(filename,shortname)"
                          "values(?,lower(?))",
                          -1,
                          &G_ins_deck,
                          (const char **)&ztail) != SQLITE_OK)
         || (sqlite3_prepare(G_db,
                          "insert or ignore into slides(deckid,slidenum)"
                          "values(?,?)",
                          -1,
                          &G_ins_slide,
                          (const char **)&ztail) != SQLITE_OK)
         || (sqlite3_prepare(G_db,
                          "select slideid from slides"
                          " where deckid=? and slidenum=?",
                          -1,
                          &G_sel_slide,
                          (const char **)&ztail) != SQLITE_OK)
         || (sqlite3_prepare(G_db,
                          "insert into words(word,slideid,origin,kw)"
                          "values(lower(trim(?)),?,upper(char(?)),"
                          "case ? when 1 then 'Y' else 'N' end)",
                          -1,
                          &G_ins_word,
                          (const char **)&ztail) != SQLITE_OK)
         || (sqlite3_prepare(G_db,
                          "insert into words(word,slideid,origin,kw)"
                          "values(trim(?),?,upper(char(?)),"
                          "case ? when 1 then 'Y' else 'N' end)",
                          -1,
                          &G_ins_word_as_is,
                          (const char **)&ztail) != SQLITE_OK)
         || (sqlite3_prepare(G_db,
                          "update words set counter=counter+1"
                          " where word=lower(trim(?))"
                          " and slideid=?"
                          " and origin=upper(?)",
                          -1,
                          &G_upd_word,
                          (const char **)&ztail) != SQLITE_OK)
         || (sqlite3_prepare(G_db,
                          "update words set counter=counter+1"
                          " where word=trim(?)"
                          " and slideid=?"
                          " and origin=upper(?)",
                          -1,
                          &G_upd_word_as_is,
                          (const char **)&ztail) != SQLITE_OK)) {
       fprintf(stderr, "insert/update: %s\n",
                       (char *)sqlite3_errmsg(G_db));
       (void)sqlite3_close(G_db);
       exit(1);
     }
     atexit(finalize_sqlite);
}

extern int new_deck(char *fname) {
    int   ret = 0;
    char  shortname[FILENAME_MAX];
    char *s;
    char *p;

    if (fname) {
      (void)sqlite3_reset(G_ins_deck);
      strncpy(shortname, fname, FILENAME_MAX);
      if ((s = strrchr(shortname, '/')) == (char *)NULL) {
        s = shortname;
      } else {
        s++;
      }
      if ((p = strchr(s, '.')) != (char *)NULL) {
        *p = '\0';
      }
      fprintf(stderr, "%-30.30s", s);
      if ((sqlite3_bind_text(G_ins_deck, 1,
                             fname, -1, NULL) == SQLITE_OK)
          && (sqlite3_bind_text(G_ins_deck, 2,
                             s, -1, NULL) == SQLITE_OK)
          && (sqlite3_step(G_ins_deck) == SQLITE_DONE)) {
        ret = sqlite3_last_insert_rowid(G_db);
      } else {
        fprintf(stderr, "insert deck: %s\n",
                      (char *)sqlite3_errmsg(G_db));
        (void)sqlite3_close(G_db);
        exit(1);
      }
    }
    return ret;
}

extern int new_slide(int deckid, int slidenum) {
    int ret = 0;

    (void)sqlite3_reset(G_ins_slide);
    if ((sqlite3_bind_int(G_ins_slide, 1,
                          deckid) == SQLITE_OK)
        && (sqlite3_bind_int(G_ins_slide, 2,
                             slidenum) == SQLITE_OK)
        && (sqlite3_step(G_ins_slide) == SQLITE_DONE)) {
      if (sqlite3_changes(G_db) == 1) {
        ret = sqlite3_last_insert_rowid(G_db);
      } else {
        // Already here - retrieve the slide id
        (void)sqlite3_reset(G_sel_slide);
        if ((sqlite3_bind_int(G_sel_slide, 1,
                              deckid) == SQLITE_OK)
            && (sqlite3_bind_int(G_sel_slide, 2,
                                 slidenum) == SQLITE_OK)
            && (sqlite3_step(G_sel_slide) == SQLITE_ROW)) {
          ret = sqlite3_column_int(G_sel_slide, 0);
        }
      }
    } else {
      fprintf(stderr, "insert slide: %s\n",
                      (char *)sqlite3_errmsg(G_db));
      (void)sqlite3_close(G_db);
      exit(1);
    }
    return ret;
}

extern void new_word(char *w, int slideid, char origin, short kw) {
    int code = SQLITE_DONE;

    if (w) {
      (void)sqlite3_reset(G_ins_word);
      if ((sqlite3_bind_text(G_ins_word, 1, w, -1, NULL) == SQLITE_OK)
          && (sqlite3_bind_int(G_ins_word, 2, slideid) == SQLITE_OK)
          && (sqlite3_bind_int(G_ins_word, 3, (int)origin) == SQLITE_OK)
          && (sqlite3_bind_int(G_ins_word, 4, (int)kw) == SQLITE_OK)
          && ((code = sqlite3_step(G_ins_word)) == SQLITE_DONE)) {
        return; 
      }
      (void)sqlite3_reset(G_upd_word);
      if ((sqlite3_bind_text(G_upd_word, 1, w, -1, NULL) == SQLITE_OK)
        && (sqlite3_bind_int(G_upd_word, 2, slideid) == SQLITE_OK)
        && ((code = sqlite3_step(G_upd_word)) == SQLITE_DONE)) {
        return; 
      }
      fprintf(stderr,
              "update word: %s (%d/code %d - %s)\n",
              (char *)sqlite3_errmsg(G_db),
              sqlite3_errcode(G_db),
              code,
              sqlite3_errstr(code));
    }
}

extern void new_word_as_is(char *w, int slideid, char origin, short kw) {
    int   code = SQLITE_DONE;
    char *t;

    if (w && ((t = cleanup(w)) != (char *)NULL)) {
      (void)sqlite3_reset(G_ins_word_as_is);
      if ((sqlite3_bind_text(G_ins_word_as_is, 1, t, -1, NULL) == SQLITE_OK)
          && (sqlite3_bind_int(G_ins_word_as_is, 2, slideid) == SQLITE_OK)
          && (sqlite3_bind_int(G_ins_word_as_is, 3, (int)origin) == SQLITE_OK)
          && (sqlite3_bind_int(G_ins_word, 4, (int)kw) == SQLITE_OK)
          && ((code = sqlite3_step(G_ins_word_as_is)) == SQLITE_DONE)) {
        return; 
      }
      (void)sqlite3_reset(G_upd_word_as_is);
      if ((sqlite3_bind_text(G_upd_word_as_is, 1, t, -1, NULL) == SQLITE_OK)
        && (sqlite3_bind_int(G_upd_word_as_is, 2, slideid) == SQLITE_OK)
        && ((code = sqlite3_step(G_upd_word_as_is)) == SQLITE_DONE)) {
        return; 
      }
      if (t) {
        free(t);
      }
      fprintf(stderr,
              "update word: %s (%d/code %d - %s)\n",
              (char *)sqlite3_errmsg(G_db),
              sqlite3_errcode(G_db),
              code,
              sqlite3_errstr(code));
    }
}

extern void remove_words() {
   // Remove words that appear too often
   sqlite3_stmt *stmt = (sqlite3_stmt *)NULL;
   char         *ztail = (char *)NULL;

   // Delete words that appear in more than 60% of decks
   if ((sqlite3_prepare(G_db,
                        "delete from words"
                        " where origin in('S','N') and word in"
                        "(select w.word"
                        " from slides s"
                        " join words w"
                        " on w.slideid=s.slideid"
                        " cross join (select count(distinct deckid) totfilecnt"
                        "             from slides) z"
                        " group by w.word, z.totfilecnt"
                        " having round(100*count(distinct s.deckid)"
                        "  /z.totfilecnt)>60)"
                        " and (stem is null"
                        " or not exists(select null from words w"
                        " where w.stem=words.stem and origin='I'))",
                        -1,
                        &stmt,
                        (const char **)&ztail) != SQLITE_OK)) {
     fprintf(stderr, "Delete words 0: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   }
   if ((sqlite3_step(stmt) != SQLITE_DONE)) {
     fprintf(stderr, "Delete words 1: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   } else {
     (void)sqlite3_finalize(stmt);
   }
   // Delete words that appear in more than 20% of slides
   if ((sqlite3_prepare(G_db,
                        "delete from words"
                        " where origin in('S','N') and word in"
                        "(select w.word"
                        " from words w"
                        " cross join (select count(*) slidecnt"
                        "             from slides) z"
                        " group by w.word, z.slidecnt"
                        " having round(100*count(distinct w.slideid)"
                        "  /z.slidecnt)>20)"
                        " and (stem is null"
                        " or not exists(select null from words w"
                        " where w.stem=words.stem and origin='I'))",
                        -1,
                        &stmt,
                        (const char **)&ztail) != SQLITE_OK)
            || (sqlite3_step(stmt) != SQLITE_DONE)) {
     fprintf(stderr, "Delete words 2: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   } else {
     (void)sqlite3_finalize(stmt);
   }
}

extern void stem_words() {
   // Basic stemming
   sqlite3_stmt *stmt = (sqlite3_stmt *)NULL;
   char         *ztail = (char *)NULL;
   int           i;
   char         *sql[] = {"create temporary table stemtmp as"
                          " select w2.word as word, min(w1.word) as stem"
                          " from words w1"
                          " join words w2"
                          " on (w2.word=w1.word||'ed'"
                          " or w2.word=w1.word||'s'"
                          " or w2.word=w1.word||'ing'"
                          " or w2.word=w1.word||'able'"
                          " or (w1.word like '%y'"
                          "     and w2.word=substr(w1.word,1,"
                          "length(w1.word)-1)||'ies'))"
                          " group by w2.word",
                          "update words"
                          " set stem=(select stem"
                          " from stemtmp"
                          " where stemtmp.word=words.word)"
                          " where word in (select word from stemtmp)",
                          NULL};

   i = 0;
   while (sql[i]) {
     if ((sqlite3_prepare(G_db, sql[i], -1,
                          &stmt, (const char **)&ztail) != SQLITE_OK)
          || (sqlite3_step(stmt) != SQLITE_DONE)) {
       fprintf(stderr, "%s: %s\n",
                       sql[i],
                       (char *)sqlite3_errmsg(G_db));
       (void)sqlite3_close(G_db);
       exit(1);
     }
     (void)sqlite3_finalize(stmt);
     i++;
   }
}

extern void generate_list(void) {
   sqlite3_stmt *stmt = (sqlite3_stmt *)NULL;
   char         *ztail = (char *)NULL;
   int           rc;

   if ((sqlite3_prepare(G_db,
                        "select replace(list,',',char(?))"
                        " from(select replace(list, ',,', ',') as list"
                        " from(select distinct stem||case word"
                        " when '' then '' else ','"
                        " ||group_concat(distinct word) end list" 
                        " from (select upper(substr(stem,1,1))||"
                        " substr(stem,2,length(stem)-1) as stem,"
                        " upper(substr(word,1,1))||"
                        " substr(word,2,length(word)-1) as word"
                        " from words"
                        " where stem is not null"
                        " union"
                        " select upper(substr(word,1,1))||"
                        " substr(word,2,length(word)-1) as stem,"
                        " '' as word"
                        " from words"
                        " where stem is null"
                        " order by 1) x"
                        " group by stem) y) z"
                        " order by 1",
                        -1,
                        &stmt,
                        (const char **)&ztail) != SQLITE_OK)
                || (sqlite3_bind_int(stmt, 1,
                                     (int)get_sep())!= SQLITE_OK)) {
     fprintf(stderr, "generate_list 0: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   }
   while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
     printf("%s\n", (char *)sqlite3_column_text(stmt, 0));
   }
   if (rc != SQLITE_DONE) {
     fprintf(stderr, "generate_list 1: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   } else {
     (void)sqlite3_finalize(stmt);
   }
}

extern void generate_index(void) {
   sqlite3_stmt *stmt = (sqlite3_stmt *)NULL;
   char         *ztail = (char *)NULL;
   char          initial = '\0';
   char          prevword[MAX_WORD_LEN];
   char         *w;
   char         *d;   // deck
   char         *s;   // slide list
   char         *p;
   int           slast;  // Slide or page number
   int           sn = 0;  // Slide or page number
   short         range;
   char          last_out;
   int           rc;
   short         kw;
   char          keyword;
   char          rtf = get_mode() & OPT_RTF;

   if ((sqlite3_prepare(G_db,
                        "select x.word,x.shortname,"
                        "       group_concat(x.slidenum),"
                        "       case x.kw when 'Y' then 1 else 0 end as kw"
                        " from (select distinct coalesce(w.stem, w.word)"
                        "         as word, d.shortname,"
                        "         case ?"
                        "           when 0 then s.slidenum"
                        "           when 1 then s.slidenum"
                        "           else cast(round((s.slidenum"
                        "                     - 1)/?)+1 as int)"
                        "         end as slidenum,"
                        "         w.kw"
                        "       from words w"
                        "            join slides s"
                        "              on s.slideid = w.slideid"
                        "            join decks d"
                        "              on d.deckid = s.deckid"
                        "       where length(trim(word))>0"
                        "       order by 1, 2, 3) x"
                        " group by x.word,x.shortname,x.kw"
                        " order by upper(x.word),x.shortname",
                        -1,
                        &stmt,
                        (const char **)&ztail) != SQLITE_OK)
                || (sqlite3_bind_int(stmt, 1,
                                     (int)get_pages())!= SQLITE_OK)
                || (sqlite3_bind_int(stmt, 2,
                                     (int)get_pages())!= SQLITE_OK)) {
     fprintf(stderr, "generate_index 0: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   }
   if (rtf) {
     printf("{\\rtf1\\ansi\\ansicpg1252\\cocoartf1404\\cocoasubrtf340\n");
     printf("{\\fonttbl\\f0\\fswiss\\fcharset0 Helvetica;\\f1\\fnil\\fcharset0 Consolas-Bold;}\n");
     printf("{\\colortbl;\\red255\\green255\\blue255;\\red59\\green0\\blue164;}\n");
    printf("\\margl1440\\margr1440\\vieww18540\\viewh14540\\viewkind0\n");
    printf("\\pard\\tx566\\tx1133\\tx1700\\tx2267\\tx2834\\tx3401\\tx3968\\tx4535\\tx5102\\tx5669\\tx6236\\tx6803\\pardirnatural\\partightenfactor0\n");
    printf("\n\\f0\\fs24 \\cf0 \\\n");
   }
   prevword[0] = '\0';
   while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
     w = (char *)sqlite3_column_text(stmt, 0);
     kw = (char)sqlite3_column_int(stmt, 4);
     if ((toupper(*w) != initial) && isalpha(*w)) {
       if (rtf) {
         printf("\\\n\\\n\\b\\fs44 \\cf2 %c\n\\b0\\fs24 \\cf0 \\\n",
                toupper(*w));
       } else {
         printf("\n\n--- %c ---", toupper(*w));
       }
       initial = toupper(*w);
     }
     w = index_entry(w, &kw);
     keyword = (kw == 1);
     if (strncmp(w, prevword, MAX_WORD_LEN)) {
       if (rtf && autokw() && !keyword) {
         keyword = lowercase_word(w);
       }
       if (rtf) { 
         if (keyword) {
           printf("\\\n\\f1\\b %s\n\\f0\\b0 \\\n", w);
         } else {
           printf("\\\n%s\\\n", w);
         }
       } else {
         printf("\n%s\n", w);
       }
       strncpy(prevword, w, MAX_WORD_LEN);
     } else {
       if (rtf) {
         putchar('\\');
       }
       putchar('\n');
     }
     d = (char *)sqlite3_column_text(stmt, 1);
     printf("   %s%-30.30s%s %s ",
            (rtf ? "\\i ":""),
            d,
            (rtf ? "\n\\i0":""),
            (get_pages() ? "p." : ""));
     s = strdup((char *)sqlite3_column_text(stmt, 2));
     // Try to condense by replacing three or more consecutive
     // page or slide values with a n-m range
     range = 0;
     slast = -1;
     p = strtok(s, ",");
     last_out = 0;
     while (p) {
       if (sscanf(p, "%d", &sn)) {
         if (sn == 1 + slast) {
           range++;
           last_out = 0;
         } else {
           // Not in the same range of values
           if (slast > 0) {
             if (range) {
               if (range > 1) {
                 printf("-%d,%d", slast, sn);
                 last_out = 1;
               } else {
                 if (slast > 0) {
                   if (!last_out) {
                     printf(",%d", slast);
                   }
                   printf(",%d", sn);
                 } else {
                   printf("%d", sn);
                 }
                 last_out = 1;
               }
               range = 0;
             } else {
               printf(",%d", sn);
               last_out = 1;
             }
           } else {
             printf("%d", sn);
             last_out = 1;
           }
         }
         slast = sn;
       }
       p = strtok(NULL, ",");
     }
     if (!last_out) {
       if (range > 1) {
         printf("-%d", sn);
       } else {
         printf(",%d", sn);
       }
     }
     if (s) {
       free(s);
     }
   }
   if (rtf) {
     putchar('\\');
   }
   putchar('\n');
   fflush(stdout);
   if (rc != SQLITE_DONE) {
     fprintf(stderr, "generate_index 1: %s\n",
                     (char *)sqlite3_errmsg(G_db));
     (void)sqlite3_close(G_db);
     exit(1);
   } else {
     (void)sqlite3_finalize(stmt);
   }
   if (rtf) {
     printf("}\n");
   }
}