#include <stdio.h>
#include <string.h>
#include <stdarg.h>  

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
		printf("(%s) %s():%d - ", d, func, line);

	vprintf(fmt, args);

	va_end(args);
}
