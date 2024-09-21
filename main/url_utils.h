#ifndef URL_UTILS_H
#define URL_UTILS_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Converts a hexadecimal character to its decimal value.
 * @param ch - hexadecimal character ('0'-'9', 'A'-'F', 'a'-'f')
 * @return Decimal value or -1 if the character is invalid.
 */
int hex2int(char ch);

/**
 * Decodes a URL-encoded string, converting '%xx' to the corresponding character
 * and '+' to a space.
 * @param str - the string to decode.
 */
void url_decode(char *str);

#endif // URL_UTILS_H
