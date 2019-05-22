#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef __ZEPHYR__
  #include <zephyr.h>
  #define printX    printk
  #define vprintX   vprintk
#else
  #define printX    printf
  #define vprintX   vprintf
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

  if (!raw){
    printX("(%s) %s():%d - ", d, func, line);
  }
  vprintX(fmt, args);

  va_end(args);
}

void msg_dump(const char *s, unsigned char *data, unsigned len)
{
	unsigned i;

	printX("%s: ", s);
	for (i = 0U; i < len; i++) {
		printX("%02x ", data[i]);
	}
	printX("(%u bytes)\n", len);
}
