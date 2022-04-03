#ifndef VBOXDLOG_H
#define VBOXDLOG_H

#include <conio.h>

/** Logs a single character to the VBox debug message port. */
static inline void vbox_log_putc(char c)
{
	outp(0x504, c);
}

#endif // VBOXDLOG_H
