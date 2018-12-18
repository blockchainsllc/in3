#include <stdio.h>
#include <kernel.h>
#include <string.h>

void __dbg_log(int raw, char *file, const char *func, int line, char *fmt, ...)
{
	char *d = 0;
	va_list args;
	va_start(args, fmt);

	d = strrchr(file, '/');
	if (d)
		d++;
	else
		d = file;

	if (!raw)
		printk("(%s) %s():%d - ", d, func, line);

	vprintk(fmt, args);

	va_end(args);
}
