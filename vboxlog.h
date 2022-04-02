#ifndef VBOXDLOG_H
#define VBOXDLOG_H

/** Logs a single character to the VBox debug message port. */
static void vbox_log_putc(char c);
#pragma aux vbox_log_putc = \
	"mov dx, 0x504" \
	"out dx, al" \
	__parm [al] \
	__modify [dx]

#endif // VBOXDLOG_H
