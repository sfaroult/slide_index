#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "slide_index.h"

#define OPTIONS  "rI:tS:s:p:k:d"

static void usage(char *fname) {
   if (fname) {
     fprintf(stderr, "Usage: %s [flags] <pptx file> [<pptx file> ...]\n",
                     fname);
     fprintf(stderr, " Flags:\n");
     fprintf(stderr,
        "  -d            : Disable the \"auto-keyword\" mode. By default,\n");
     fprintf(stderr,
        "                  all-lowercase words are understood as keywords\n");
     fprintf(stderr,
        "                  and displayed differently in an RTF index.\n");
     fprintf(stderr,
        "  -I <filename> : Read words to index from <filename>.\n");
     fprintf(stderr,
        "                  Each line can contain several variants of\n");
     fprintf(stderr,
        "                  the same word, separated by a character that\n");
     fprintf(stderr,
        "                  defaults to a semi-colon. The first entry on\n");
     fprintf(stderr,
        "                  the line is the one that appears in the index.\n");
     fprintf(stderr,
        "  -k <char>     : Prefix for keywords (displayed differently\n");
     fprintf(stderr,
        "                  in an RTF index); must be a single character.\n");
     fprintf(stderr,
        "  -p <num>      : By default the slide number appear in the index.\n");
     fprintf(stderr,
        "                  If handouts contain multiple slides per page,\n");
     fprintf(stderr,
        "                  the -p flag followed by the number of slides per\n");
     fprintf(stderr,
        "                  page will list the page number in the index.\n");
     fprintf(stderr,
        "  -r            : Generate an RTF (Rich-Text-Format) index instead\n");
     fprintf(stderr,
        "                  of a plain-text one.\n");
     fprintf(stderr,
        "  -s <char>     : Change the separator from ';' to <char>\n");
     fprintf(stderr,
        "  -S <filename> : Read words that are NOT to index (stop words)\n");
     fprintf(stderr,
        "                  from <filename>. There must be only one word per\n");
     fprintf(stderr,
        "                  line and no stemming is performed (all variants\n");
     fprintf(stderr,
        "                  must be explicitly listed in the file)\n");
     fprintf(stderr,
        "  -t            : Exclusively index from tags in slide notes.\n");
   }
}

int main(int argc, char *argv[]) {
  int                k;
  int                slidecount = 0;
  int                num_slides;
  int                ch;
  char               keyword_file[FILENAME_MAX];
  char               stop_file[FILENAME_MAX];
  char               failure = 0;
  short              slides_per_page = 0;

  // Possible options:
  //    - called with -n
  //      Only tags in the notes are indexed (best option)
  //    - called with -i
  //      Words to index are read from an external file, and checked
  //      in both slide and note text. Tags in notes are processed as
  //      any other word
  //
  //      -n and -i are mutually exclusive.
  //
  //    - called with neither -n nor -i. A list of words is generated.
  //      Words that are "too frequent" are eliminated from the list.
  //      Optionally, -s reads-in a list of stop words.
  //      The generated list should be manually checked and edited to
  //      be used as input file with the -i option.
  //
  //      -s is also exclusive of -n and -i
  //
  while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
    switch (ch) {
      case 'd':  // Disable auto-keyword mode
        disable_autokw();
        break;
      case 'k':  // Set keyword prefix
        set_kw(*optarg);
        break;
      case 's':  // Set separator
        set_sep(*optarg);
        break;
      case 'r':  // Generate RTF output
        set_mode(OPT_RTF);
        break;
      case 'p':  // Slides per page
        if (sscanf(optarg, "%hd", &slides_per_page)) {
          if (slides_per_page < 1) {
            failure = 1;
          }
          switch (slides_per_page) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 6:
            case 9:
                 break;
            default:
                 failure = 1;
                 break;
          }
        } else {
          failure = 1;
        }
        if (failure) {
          fprintf(stderr, "-p must be followed by a positive integer\n");
          fprintf(stderr, "(number of slides per page in the handout)\n");
          fprintf(stderr, "This number can only be one of 1,2,3,4,6 or 9.\n");
          return 1;
        }
        set_pages(slides_per_page);
        break;
      case 'I':
        if ((get_mode() == OPT_GENERATE)
            || (get_mode() == (OPT_GENERATE | OPT_RTF))) {
          strncpy(keyword_file, optarg, FILENAME_MAX);
          set_mode(OPT_IFILE);
        } else {
          fprintf(stderr, "Option -I is incompatible with -%c\n",
                     (get_mode() & OPT_STOP ? 'S' :
                          (get_mode() & OPT_TAGS ? 't' : '?')));
          return 1;
        }
        break;
      case 'S':
        if (get_mode() == OPT_GENERATE) {
          strncpy(stop_file, optarg, FILENAME_MAX);
          set_mode(OPT_STOP);
        } else {
          fprintf(stderr, "Option -S is incompatible with -%c\n",
                     (get_mode() & OPT_IFILE ? 'I' :
                          (get_mode() & OPT_TAGS ? 't' : 'r')));
          return 1;
        }
        break;
      case 't':
        if ((get_mode() == OPT_GENERATE)
            || (get_mode() == (OPT_GENERATE | OPT_RTF))) {
          set_mode(OPT_TAGS);
        } else {
          fprintf(stderr, "Option -t is incompatible with -%c\n",
                     (get_mode() & OPT_IFILE ? 'I' :
                          (get_mode() & OPT_STOP ? 'S' : '?')));
          return 1;
        }
        break;
      case '?':
      default:
        usage(argv[0]);
        return 1;
        break; /*NOTREACHED*/
     }
  }
  if (get_mode() & OPT_RTF & OPT_STOP) {
    fprintf(stderr, "Options -r and -s are incompatible\n");
    return 1;
  }
  setup_sqlite();
  if (get_mode() & OPT_IFILE) {
     load_index(keyword_file);
  }
  if (get_mode() & OPT_STOP) {
     load_stop(stop_file);
  }
  for (k = optind; k < argc; k++) {
    num_slides = process_pptx(argv[k]);
    if (num_slides < 0) {
      exit(EXIT_FAILURE);
    }
    slidecount += num_slides;
    fprintf(stderr, "%d slide%s\n",
                    num_slides, (num_slides == 1 ? "" : "s"));
    fflush(stderr);
  } // End of loop on .pptx files
  if (slidecount > 0) {
    fprintf(stderr, "Total: %4d slides\n", slidecount);
    fflush(stderr);
    // Second phase: Generation of either a list of words or
    //               an index
    switch(get_mode()) {
       case OPT_GENERATE:
       case OPT_STOP:
            fprintf(stderr, "Stemming and cleaning up words\n");
            // Primitive stemming
            stem_words();
            // Remove words that appear everywhere
            remove_words();
            generate_list();
            break;
       default:
            generate_index();
            break;
    }
  } else {
    usage(argv[0]);
  }
  free_trees();
  exit(EXIT_SUCCESS);
}
