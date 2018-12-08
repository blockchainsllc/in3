
#ifdef DEBUG
#define dbg_log(msg, ...)	__dbg_log(0, __FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)
#define dbg_log_raw(msg, ...)	__dbg_log(1, __FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)
void __dbg_log(int raw, char *file, const char *func, int line, char *fmt, ...);
#else
#define dbg_log(msg, ...)
#define dbg_log_raw(msg, ...)
#endif
