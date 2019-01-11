#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef __ZEPHYR__
#include <zephyr.h>
#endif

void __dbg_log(int raw, char* file, const char* func, int line, char* fmt, ...) {
  char*   d = 0;
  va_list args;
  va_start(args, fmt);

  d = strrchr(file, '/');
  if (d)
    d++;
  else
    d = file;

  if (!raw)
#ifdef __ZEPHYR__
	printk("(%s) %s():%d - ", d, func, line);
#else		
	printf("(%s) %s():%d - ", d, func, line);
#endif

#ifdef __ZEPHYR__
	vprintk(fmt, args);
#else		
	vprintf(fmt, args);
#endif

  va_end(args);
}
