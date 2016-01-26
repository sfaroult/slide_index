#ifndef SLIDE_INDEX_H

#define SLIDE_INDEX_H

#define MIN_WORD_LEN    4
#define MAX_WORD_LEN    100

#define OPT_GENERATE   0
#define OPT_STOP       1
#define OPT_IFILE      2
#define OPT_TAGS       4
#define OPT_RTF        8

#include "si_settings.h"
#include "si_util.h"
#include "si_tree_ops.h"
#include "si_db_ops.h"

extern int process_pptx(char *pptx_file);

#endif
