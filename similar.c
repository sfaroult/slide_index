#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sqlite3.h>

extern void similar(sqlite3_context *context,
                    int argc, sqlite3_value **argv) {
    char   *s;
    char    confused_with[2];

    s = (char *)sqlite3_value_text(argv[0]);
    if (s == NULL) {
      sqlite3_result_null(context);
    } else {
      confused_with[1] = '\0';
      switch (*s) {
        case ':':
             confused_with[0] = ';';
             break;
        case ';':
             confused_with[0] = ':';
             break;
        case ',':
             confused_with[0] = '.';
             break;
        case '.':
             confused_with[0] = ',';
             break;
        case '\'':
             confused_with[0] = '`';
             break;
        case '`':
             confused_with[0] = '\'';
             break;
        case '?':
             confused_with[0] = '!';
             break;
        case '!':
             confused_with[0] = '?';
             break;
        default:     
             confused_with[0] = '\\';
             break;
      } 
      sqlite3_result_text(context, confused_with, -1, SQLITE_TRANSIENT);
    }
}
