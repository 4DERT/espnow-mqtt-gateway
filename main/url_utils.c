#include "url_utils.h"
#include <ctype.h>

int hex2int(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  return -1;
}

void url_decode(char *str) {
  char *pstr = str;
  char *pdec = str;
  while (*pstr) {
    if (*pstr == '%' && isxdigit((unsigned char)pstr[1]) &&
        isxdigit((unsigned char)pstr[2])) {
      *pdec = (char)(hex2int(pstr[1]) * 16 + hex2int(pstr[2]));
      pstr += 3;
    } else if (*pstr == '+') {
      *pdec = ' ';
      pstr++;
    } else {
      *pdec = *pstr;
      pstr++;
    }

    pdec++;
  }
  *pdec = '\0';
}