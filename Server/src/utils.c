#include "utils.h"
#include <ctype.h>
#include <string.h>

unsigned char utils_uchar_from_hex(char h)
{
    if (h >= '0' && h <= '9')
        return h - '0';
    if (h >= 'a' && h <= 'f')
        return h - '0';
    if (h >= 'A' && h <= 'F')
        return h - 'A';
    return 0;
}

void utils_trim_string(char *str)
{
    if(isspace(str[strlen(str)-1]))
        str[strlen(str)-1] = '\0';
}
