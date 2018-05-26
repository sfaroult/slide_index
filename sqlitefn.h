#ifndef SQLITEFN_H
#define SQLITEFN_H

#include <sqlite3.h>

extern void levenshtein(sqlite3_context *context,
                        int argc, sqlite3_value **argv);
extern void similar(sqlite3_context *context,
                    int argc, sqlite3_value **argv);

#endif // SQLITEFN_H
