#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sqlite3.h>

// From https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C

#define MIN3(a, b, c) ((a) < (b) ? \
                       ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

extern void levenshtein(sqlite3_context *context,
                        int argc, sqlite3_value **argv) {
    char   *s1;
    char   *s2;

    s1 = (char *)sqlite3_value_text(argv[0]);
    s2 = (char *)sqlite3_value_text(argv[1]);
    if ((s1 == NULL) || (s2 == NULL)) {
      sqlite3_result_null(context);
    } else {
      unsigned int x;
      unsigned int y;
      unsigned int s1len;
      unsigned int s2len;

      s1len = strlen(s1);
      s2len = strlen(s2);
      unsigned int matrix[s2len+1][s1len+1];
      matrix[0][0] = 0;
      for (x = 1; x <= s2len; x++) {
        matrix[x][0] = matrix[x-1][0] + 1;
      }
      for (y = 1; y <= s1len; y++) {
        matrix[0][y] = matrix[0][y-1] + 1;
      }
      for (x = 1; x <= s2len; x++) {
        for (y = 1; y <= s1len; y++) {
          matrix[x][y] = MIN3(matrix[x-1][y] + 1,
                              matrix[x][y-1] + 1,
                              matrix[x-1][y-1] + (s1[y-1] == s2[x-1] ? 0 : 1));
        }
      }
      sqlite3_result_int(context, (int)matrix[s2len][s1len]);
    }
}
