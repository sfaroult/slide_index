#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "si_util.h"

typedef struct entity {
                       char *code;
                       char *replacement;
                      } ENTITY_T;

// This list could be extended if needed
static ENTITY_T      G_entities[] = {{"&nbsp;", " "},
                                     {"&#160;", " "},
                                     {"&lt;", "<"},
                                     {"&#60;", "<"},
                                     {"&gt;", ">"},
                                     {"&#62;", ">"},
                                     {"&amp;", "&"},
                                     {"&#38;", "&"},
                                     {"&#91;", "["},
                                     {"&#92;", "\\"},
                                     {"&#93;", "]"},
                                     {"&#123;", "{"},
                                     {"&#124;", "|"},
                                     {"&#125;", "}"},
                                     {"&Aacute;", "Á"},
                                     {"&#193;", "Á"},
                                     {"&aacute;", "á"},
                                     {"&#225;", "á"},
                                     {"&Acirc;", "Â"},
                                     {"&#194;", "Â"},
                                     {"&acirc;", "â"},
                                     {"&#226;", "â"},
                                     {"&AElig;", "Æ"},
                                     {"&#198;", "Æ"},
                                     {"&aelig;", "æ"},
                                     {"&#230;", "æ"},
                                     {"&Agrave;", "À"},
                                     {"&#192;", "À"},
                                     {"&agrave;", "à"},
                                     {"&#224;", "à"},
                                     {"&Aring;", "Å"},
                                     {"&#197;", "Å"},
                                     {"&aring;", "å"},
                                     {"&#229;", "å"},
                                     {"&Atilde;", "Ã"},
                                     {"&#195;", "Ã"},
                                     {"&atilde;", "ã"},
                                     {"&#227;", "ã"},
                                     {"&Auml;", "Ä"},
                                     {"&#196;", "Ä"},
                                     {"&auml;", "ä"},
                                     {"&#228;", "ä"},
                                     {"&Ccedil;", "Ç"},
                                     {"&#199;", "Ç"},
                                     {"&ccedil;", "ç"},
                                     {"&#231;", "ç"},
                                     {"&Eacute;", "É"},
                                     {"&#201;", "É"},
                                     {"&eacute;", "é"},
                                     {"&#233;", "é"},
                                     {"&Ecirc;", "Ê"},
                                     {"&#202;", "Ê"},
                                     {"&ecirc;", "ê"},
                                     {"&#234;", "ê"},
                                     {"&Egrave;", "È"},
                                     {"&#200;", "È"},
                                     {"&egrave;", "è"},
                                     {"&#232;", "è"},
                                     {"&ETH;", "Ð"},
                                     {"&#208;", "Ð"},
                                     {"&eth;", "ð"},
                                     {"&#240;", "ð"},
                                     {"&Euml;", "Ë"},
                                     {"&#203;", "Ë"},
                                     {"&euml;", "ë"},
                                     {"&#235;", "ë"},
                                     {"&Iacute;", "Í"},
                                     {"&#205;", "Í"},
                                     {"&iacute;", "í"},
                                     {"&#237;", "í"},
                                     {"&Icirc;", "Î"},
                                     {"&#206;", "Î"},
                                     {"&icirc;", "î"},
                                     {"&#238;", "î"},
                                     {"&Igrave;", "Ì"},
                                     {"&#204;", "Ì"},
                                     {"&igrave;", "ì"},
                                     {"&#236;", "ì"},
                                     {"&Iuml;", "Ï"},
                                     {"&#207;", "Ï"},
                                     {"&iuml;", "ï"},
                                     {"&#239;", "ï"},
                                     {"&Ntilde;", "Ñ"},
                                     {"&#209;", "Ñ"},
                                     {"&ntilde;", "ñ"},
                                     {"&#241;", "ñ"},
                                     {"&Oacute;", "Ó"},
                                     {"&#211;", "Ó"},
                                     {"&oacute;", "ó"},
                                     {"&#243;", "ó"},
                                     {"&Ocirc;", "Ô"},
                                     {"&#212;", "Ô"},
                                     {"&ocirc;", "ô"},
                                     {"&#244;", "ô"},
                                     {"&OElig;", "Œ"},
                                     {"&#338;", "Œ"},
                                     {"&oelig;", "œ"},
                                     {"&#339;", "œ"},
                                     {"&Ograve;", "Ò"},
                                     {"&#210;", "Ò"},
                                     {"&ograve;", "ò"},
                                     {"&#242;", "ò"},
                                     {"&Oslash;", "Ø"},
                                     {"&#216;", "Ø"},
                                     {"&oslash;", "ø"},
                                     {"&#248;", "ø"},
                                     {"&Otilde;", "Õ"},
                                     {"&#213;", "Õ"},
                                     {"&otilde;", "õ"},
                                     {"&#245;", "õ"},
                                     {"&Ouml;", "Ö"},
                                     {"&#214;", "Ö"},
                                     {"&ouml;", "ö"},
                                     {"&#246;", "ö"},
                                     {"&Scaron;", "Š"},
                                     {"&#352;", "Š"},
                                     {"&scaron;", "š"},
                                     {"&#353;", "š"},
                                     {"&szlig;", "ß"},
                                     {"&#223;", "ß"},
                                     {"&THORN;", "Þ"},
                                     {"&#222;", "Þ"},
                                     {"&thorn;", "þ"},
                                     {"&#254;", "þ"},
                                     {"&Uacute;", "Ú"},
                                     {"&#218;", "Ú"},
                                     {"&uacute;", "ú"},
                                     {"&#250;", "ú"},
                                     {"&Ucirc;", "Û"},
                                     {"&#219;", "Û"},
                                     {"&ucirc;", "û"},
                                     {"&#251;", "û"},
                                     {"&Ugrave;", "Ù"},
                                     {"&#217;", "Ù"},
                                     {"&ugrave;", "ù"},
                                     {"&#249;", "ù"},
                                     {"&Uuml;", "Ü"},
                                     {"&#220;", "Ü"},
                                     {"&uuml;", "ü"},
                                     {"&#252;", "ü"},
                                     {"&Yacute;", "Ý"},
                                     {"&#221;", "Ý"},
                                     {"&yacute;", "ý"},
                                     {"&#253;", "ý"},
                                     {"&yuml;", "ÿ"},
                                     {"&#255;", "ÿ"},
                                     {"&Yuml;", "Ÿ"},
                                     {"&#376;", "Ÿ"},
                                     {NULL, NULL}};

static int is_utf8(const unsigned char *u) {
   // Validate utf8 character.
   // Returns the length, 0 if invalid.
   int len = 0;

   if (u) {
      if (*u < 0xc0) {
         len = 1;
      } else {
         if ((*u & 0xe0) == 0xc0) {
            // U-00000080 - U-000007FF : 110xxxxx 10xxxxxx
            len = 2;
         } else if ((*u & 0xf0) == 0xe0) {
            // U-00000800 - U-0000FFFF : 1110xxxx 10xxxxxx 10xxxxxx
            len = 3;
         } else if ((*u & 0xf8) == 0xf0) {
            // U-00010000 - U-001FFFFF : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            len = 4;
         } else {
            // malformed UTF-8 character
            return 0;
         }
         // Check that the UTF-8 character is OK
         int i;
         for (i = 1; i < len; i++ ) {
            if ((u[i] & 0xC0) != 0x80) {
               return 0;
            }
         }
      }
   }
   return len;
}

extern unsigned int utf8_to_codepoint(const unsigned char *u,
                                      int                 *lenptr) {
   // Returns 0 if something goes wrong
   // Passes back the length
   unsigned int cp = 0;

   *lenptr = 0;
   if (u) {
      if (*u < 0xc0) {
         cp = (unsigned int)*u;
         *lenptr = 1;
      } else {
         *lenptr = is_utf8(u);
         if (*lenptr == 0) {
            return 0;
         }
         switch (*lenptr) {
            case 2:
                 cp = (u[0] - 192) * 64 + u[1] - 128;
                 break;
            case 3:
                 cp = (u[0] - 224) * 4096
                    + (u[1] - 128) * 64 + u[2] - 128;
                 break;
            default:
                 cp = (u[0] - 240) * 262144
                     + (u[1] - 128) * 4096
                     + (u[2] - 128) * 64 + u[3] - 128;
                 break;
         }
      }
   }
   return cp;
}

extern char *cleanup(char *w) {
    char *p;
    char *s;
    char *t;
    char cp = 1;

    if (w) {
      t = strdup(w);
      p = t;
      s = t;
      while (p && *p) {
        switch (*p) {
          case '<':
               cp = 0;
               break;
          case '>':
               cp = 1;
               break;
          default:
               if (cp) {
                 if (s != p) {
                   *s = *p;
                 }
                 s++;
               }
               break;
          }
        p++;
      }
      *s = '\0';
      return t;
    }
    return (char *)NULL;
}

extern void replace_entities(char *p) {
  if (p) {
    // Replace HTML entities if there are any
    int   i = 0;
    int   delta;
    char *e;
    char *e2;
    char *q = &(p[strlen(p)]);

    while (G_entities[i].code) {
      while ((e = strstr(p, G_entities[i].code)) != (char *)NULL){
        delta = strlen(G_entities[i].code)
                - strlen(G_entities[i].replacement);
        memcpy(e, G_entities[i].replacement,
                  strlen(G_entities[i].replacement));
        e2 = e+strlen(G_entities[i].code);
        memmove(e+strlen(G_entities[i].replacement),
                                   e2, strlen(e2));
        q -= delta;
        *q = '\0';
      }
      i++;
    }
  }
}

extern short lowercase_word(char *w) {
   char  *s;
   short  all_lower = 0;

   if (w && islower(*w)) {
     s = w;
     while (*s && (islower(*s) || !isalpha(*s))) {
       s++;
     }
     if (*s == '\0') {
       all_lower = 1;
     }
   }
   return all_lower;
}
