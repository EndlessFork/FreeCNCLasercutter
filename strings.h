#ifndef _STRINGS_H_
#define _STRINGS_H_


#include <string.h>
#include "platform.h"
// low level stuff

void avrtest_print(const char *fmt, ...);
void cmd_print(const char *fmt, ...);
void lcd_print(const char *fmt, ...);
void debug_print(const char *fmt, ...);

#endif
