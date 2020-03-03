//
// Control sequences, based on ANSI.
// Can be used to control color, and clear the screen
//
#define RESET "\x1B[0m" // Reset to default colors
#define CLEAR "\x1B[2J" // Clear screen, reposition cursor to top left


#ifdef LOG_USE_COLOR
#define COLOR_BLACK "\x1B[30m"
#define COLOR_RED "\x1B[31m"
#define COLOR_GREEN "\x1B[32m"
#define COLOR_YELLOW "\x1B[33m"
#define COLOR_BLUE "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN "\x1B[36m"
#define COLOR_WHITE "\x1B[37m"
#define COLOR_DEFAULT "\x1B[39m"
#define COLOR_DEFAULT "\x1B[39m"
#else
#define COLOR_BLACK ""
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN ""
#define COLOR_WHITE ""
#define COLOR_DEFAULT ""
#define COLOR_DEFAULT ""
#endif

#define COLOR_RED_STR COLOR_RED"%s"RESET
#define COLOR_GREEN_STR COLOR_GREEN"%s"RESET
#define COLOR_GREEN_S2 COLOR_GREEN"%-10s"RESET
#define COLOR_GREEN_X1 COLOR_GREEN"%01x"RESET
#define COLOR_GREEN_STR_INT COLOR_GREEN"%s%i"RESET
#define COLOR_YELLOW_STR COLOR_YELLOW"%s"RESET
#define COLOR_YELLOW_STR COLOR_YELLOW"%s"RESET
#define COLOR_MAGENTA_STR COLOR_MAGENTA"%s"RESET
#define COLOR_YELLOW_PRIu64 COLOR_YELLOW"%5" PRIu64 ""RESET
#define COLOR_YELLOW_PRIu64plus COLOR_YELLOW"%5" PRIu64 ""RESET
#define COLOR_BRIGHT_BLACK "\x1B[90m"
#define COLOR_BRIGHT_RED "\x1B[91m"
#define COLOR_BRIGHT_GREEN "\x1B[92m"
#define COLOR_BRIGHT_YELLOW "\x1B[93m"
#define COLOR_BRIGHT_BLUE "\x1B[94m"
#define COLOR_BRIGHT_MAGENTA "\x1B[95m"
#define COLOR_BRIGHT_CYAN "\x1B[96m"
#define COLOR_BRIGHT_WHITE "\x1B[97m"

#define COLOR_BG_DEFAULT "\x1B[24;49m"
#define COLOR_BG_BLACK "\x1B[24;40m"
#define COLOR_BG_RED "\x1B[24;41m"
#define COLOR_BG_GREEN "\x1B[24;42m"
#define COLOR_BG_YELLOW "\x1B[24;43m"
#define COLOR_BG_BLUE "\x1B[24;44m"
#define COLOR_BG_MAGENTA "\x1B[24;45m"
#define COLOR_BG_CYAN "\x1B[24;46m"
#define COLOR_BG_WHITE "\x1B[24;47m"

#define COLOR_BG_BRIGHT_BLACK "\x1B[4;100m"
#define COLOR_BG_BRIGHT_RED "\x1B[4;101m"
#define COLOR_BG_BRIGHT_GREEN "\x1B[4;102m"
#define COLOR_BG_BRIGHT_YELLOW "\x1B[4;103m"
#define COLOR_BG_BRIGHT_BLUE "\x1B[4;104m"
#define COLOR_BG_BRIGHT_MAGENTA "\x1B[4;105m"
#define COLOR_BG_BRIGHT_CYAN "\x1B[4;106m"
#define COLOR_BG_BRIGHT_WHITE "\x1B[4;107m"