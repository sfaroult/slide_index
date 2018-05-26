#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "slide_index.h"
#include "si_tree_ops.h"
#include "si_db_ops.h"
#include "miniz.h"

static void parse_insert(char *str, int slideid, char origin) {
    char  *s = str;
    char  *w;
    char  *index_entry;
    short  kw;
    int    len;
    char   done = 0;

    if (str) {
      record_special(str, slideid, origin);
      while (*s) {
        while (*s && !isalpha(*s) && (*s != get_kw())) {
          s++;
        }
        if (*s == get_kw()) {
          kw = 1;
          s++;
        }
        if (*s) {
          len = 1;
          w = s;
          s++;
          while (isalnum(*s) || (*s == '\'')) {
            len++;
            s++; 
          }
          if (*s) { 
            *s = '\0';
          } else {
            done = 1;
          }
          if (get_mode() & OPT_IFILE) {
            // Only insert the "word" if found in the list of words
            // to index. Note that "word" may be a variant of the
            // real word to index.
            if ((index_entry = must_index(w, &kw))!= (char *)NULL) {
              new_word(index_entry, slideid, origin, kw);
            }
          } else {
            if (len >= MIN_WORD_LEN) {
              if (!must_ignore(w)) {
                new_word(w, slideid, origin, kw);
              }
            }
          }
          if (!done) {
            s++;
          }
        }
      }
    }
}

static void analyze_text(int slideid, char *xml, char origin) {
   char *p = xml;
   char *s;
   char *t;

   if (p) {
     while ((s = strstr(p, "<a:t>")) != (char *)NULL) {
       s += 5;
       p = s;
       if ((s = strstr(p, "</a:t>")) != (char *)NULL) {
         *s = '\0';
         if ((t = cleanup(p)) != (char *)NULL) {
           parse_insert(t, slideid, origin);
           free(t);
         }
         p = s + 6;
       } else {
         break;
       }
     }
   }
}

static void analyze_pic(int slideid, char *xml) {
   // We assume that the name of the picture
   // may be meaningful
   char *p = xml;
   char *t;
   char *s;
   char *q;

   if (p) {
     while ((s = strstr(p, "<p:cNvPr ")) != (char *)NULL) {
       if ((p = strstr(s, "descr")) != (char *)NULL) {
         if ((s = strchr(p, '"')) != (char *)NULL) { 
           s++;
           if ((p = strchr(s, '"')) != (char *)NULL) {
             *p++ = '\0';
           }
           // Suppress extension, don't want to index it
           if ((q = strchr(s, '.')) != (char *)NULL) {
             *q = '\0';
           }
           if ((t = cleanup(p)) != (char *)NULL) {
             parse_insert(t, slideid, 'P');
             free(t);
           }
           if (p == (char *)NULL) {
             break;
           }
         } else {
           break;
         }
       } else {
         break;
       }
     }
   }
}

extern int process_pptx(char *pptx_file) {
  // Returns the number of slides, -1 in case of severe failure
  // (if the file doesn't exist, 0 is returned)
  mz_bool            status;
  mz_zip_archive     zip_archive;
  char              *xml_slide;
  size_t             xml_sz;
  char              *p;
  char              *q;
  char              *s;
  char              *prev;
  char              *t;
  int                i;
  int                deckid;
  int                maxslide = 0;
  int                notenum;
  int                slidenum = 0;
  int                slideid;
  char               xmlrel[FILENAME_MAX];
  short              kw;
  short              level;

  memset(&zip_archive, 0, sizeof(zip_archive));
  status = mz_zip_reader_init_file(&zip_archive, pptx_file, 0);
  if (!status) {
    fprintf(stderr, "Cannot read %s!\n", pptx_file);
  } else {
    deckid = new_deck(pptx_file);
    // Get and print information about each file in the archive.
    for (i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++) {
      mz_zip_archive_file_stat file_stat;
      if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
         fprintf(stderr, "mz_zip_reader_file_stat() failed!\n");
         mz_zip_reader_end(&zip_archive);
         return -1;
      }
      if (!mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
        if ((strncmp(file_stat.m_filename, "ppt/notesSlides/", 16) == 0)
            && strncmp(file_stat.m_filename, "ppt/notesSlides/_rels/", 22)) {
          // Notes
          //
          // *** ACHTUNG ***
          // Note numbers don't match the corresponding slide number,
          // that would be too easy. You have to dig into "_rels"
          //
          if (!sscanf(&(file_stat.m_filename[26]), "%d", &notenum)) {
            fprintf(stderr, "Failed to extract note num from %s\n",
                            &(file_stat.m_filename[16]));
            return -1;
          }
          sprintf(xmlrel,
                  "ppt/notesSlides/_rels/notesSlide%d.xml.rels",
                  notenum);
          if ((xml_slide =
                (char *)mz_zip_reader_extract_file_to_heap(&zip_archive,
                       (const char *)xmlrel,
                       &xml_sz, (mz_uint)0)) != (char *)NULL) {
            // Try to find the b****y slide number
            if ((p = strstr(xml_slide, "Target=\"../slides/slide"))
                     != NULL) {
              if (!sscanf(&(p[23]), "%d", &slidenum)) {
                fprintf(stderr, "Failed to extract slidenum from %s\n",
                        &(p[36]));
                return -1;
              }
            }
            free(xml_slide);
          }
          if (slidenum) {
            if (slidenum > maxslide) {
              maxslide = slidenum;
            }
            slideid = new_slide(deckid, slidenum);
            xml_sz = 0;
            if ((xml_slide =
                    (char *)mz_zip_reader_extract_file_to_heap(&zip_archive,
                           (const char *)file_stat.m_filename,
                           &xml_sz, (mz_uint)0)) != (char *)NULL) {
              if (get_mode() & OPT_TAGS) {
                // Exclusively look for indexing words in the notes
                q = xml_slide;
                level = 0;
                while ((p = strchr(q, '[')) != (char *)NULL) {
                  level++;
                  do {
                    p++;
                  } while (isspace(*p));
                  q = p + 1;
                  while (*q && ((*q != ']') || (level > 1))) {
                    switch (*q) {
                      case '\\': // Escaped
                           q++;
                           break;
                      case '[':
                           level++;
                           break;
                      case ']':
                           level--;
                           break;
                      default:
                           break;
                    }
                    q++;
                  }
                  if (*q == ']') {
                    *q = '\0';
                    if (get_sep() == ';') {
                      // Replace HTML entities if there are any
                      replace_entities(p);
                    }
                    while ((s = strchr(p, get_sep())) != (char *)NULL) {
                      prev = s;
                      prev--;
                      if (*prev != '\\') {
                        *s++ = '\0';
                        if (*p == get_kw()) {
                          p++;
                          kw = 1;
                        } else {
                          if (autokw()) {
                            kw = lowercase_word(p);
                          } else {
                            kw = 0;
                          }
                        } 
                        if ((t = cleanup(p)) != (char *)NULL) {
                          new_word_as_is(t, slideid, 'T', kw);
                          free(t);
                        }
                      } else {
                        s++;  // Was escaped
                      }
                      p = s;
                      while (isspace(*p)) {
                        p++;
                      }
                    }
                    if (*p == get_kw()) {
                      p++;
                      kw = 1;
                    } else {
                      if (autokw()) {
                        kw = lowercase_word(p);
                      } else {
                        kw = 0;
                      }
                    } 
                    if ((t = cleanup(p)) != (char *)NULL) {
                      new_word_as_is(t, slideid, 'T', kw);
                      free(t);
                    }
                    p = 1 + q;
                  } else {
                    break;
                  }
                }
              } else {
                // Analyze text
                analyze_text(slideid, xml_slide, 'N');
              }
              free(xml_slide);
            } else {
              fprintf(stderr, "Extract flopped\n");
              return -1;
            }
          }
        }
        // We look at regular slides even if we are only interested
        // in tags to check that we aren't missing any slide without
        // notes and that our tag count is correct
        if ((strncmp(file_stat.m_filename, "ppt/slides/", 11) == 0)
            && strncmp(file_stat.m_filename, "ppt/slides/_rels/", 17)) {
          // Regular slide
          if (!sscanf(&(file_stat.m_filename[16]), "%d", &slidenum)) {
            fprintf(stderr, "Failed to extract num from %s\n",
                            &(file_stat.m_filename[11]));
            return -1;
          }
          if (slidenum > maxslide) {
            maxslide = slidenum;
          }
          slideid = new_slide(deckid, slidenum);
          if (!(get_mode() & OPT_TAGS)) {
            xml_sz = 0;
            if ((xml_slide =
                 (char *)mz_zip_reader_extract_file_to_heap(&zip_archive,
                           (const char *)file_stat.m_filename,
                           &xml_sz, (mz_uint)0)) != (char *)NULL) {
              // Analyze text
              analyze_text(slideid, xml_slide, 'S');
              // Analyze images 
              analyze_pic(slideid, xml_slide);
              free(xml_slide);
            } else {
              fprintf(stderr, "Extract flopped\n");
              return -1;
            }
          }
        }
      }
    }
    // Close the archive, freeing any resources it was using
    mz_zip_reader_end(&zip_archive);
  }
  return maxslide;
}
