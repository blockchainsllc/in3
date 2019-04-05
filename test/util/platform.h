#ifndef IN3_PLATFORM_H
#define IN3_PLATFORM_H

/**
 * Determination a platform of an operation system
 * Fully supported only GNU GCC/G++, partially on Clang/LLVM
 */

#define PLATFORM_WINDOWS_32 1
#define PLATFORM_WINDOWS_64 10
#define PLATFORM_WINDOWS_CYGWIN 11
#define PLATFORM_ANDROID 20
#define PLATFORM_LINUX 2
#define PLATFORM_BSD 3
#define PLATFORM_HP_UX 4
#define PLATFORM_AIX 5
#define PLATFORM_IOS 60
#define PLATFORM_IOS_SIM 61
#define PLATFORM_OSX 6
#define PLATFORM_SOLARIS 7
#define PLATFORM_UNKNOWN 0

#if !defined(PLATFORM)
# if defined(_WIN32)
#  define PLATFORM PLATFORM_WINDOWS_32 // Windows
# elif defined(_WIN64)
#  define PLATFORM PLATFORM_WINDOWS_64 // Windows
# elif defined(__CYGWIN__) && !defined(_WIN32)
#  define PLATFORM PLATFORM_WINDOWS_CYGWIN // Windows (Cygwin POSIX under Microsoft Window)
# elif defined(__ANDROID__)
#  define PLATFORM PLATFORM_ANDROID // Android (implies Linux, so it must come first)
# elif defined(__linux__)
#  define PLATFORM PLATFORM_LINUX // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
# elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
#  include <TargetConditionals.h>
#  if TARGET_IPHONE_SIMULATOR == 1
#   define PLATFORM PLATFORM_IOS_SIM // Apple iOS
#  elif TARGET_OS_IPHONE == 1
#   define PLATFORM PLATFORM_IOS // Apple iOS
#  elif TARGET_OS_MAC == 1
#   define PLATFORM PLATFORM_OSX // Apple OSX
#  endif
# elif defined(__unix__)
#  include <sys/param.h>
#  if defined(BSD)
#   define PLATFORM PLATFORM_BSD // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#  endif
# elif defined(__hpux)
#  define PLATFORM PLATFORM_HP_UX // HP-UX
# elif defined(_AIX)
#  define PLATFORM PLATFORM_AIX                 // IBM AIX
# elif defined(__sun) && defined(__SVR4)
#  define PLATFORM PLATFORM_SOLARIS // Oracle Solaris, Open Indiana
# endif
#endif

#if !defined(PLATFORM)
# define PLATFORM PLATFORM_UNKNOWN
#endif

#define PLATFORM_IS_WINDOWS(p_) (p_ == PLATFORM_WINDOWS_32 || p_ == PLATFORM_WINDOWS_64)

#endif //IN3_PLATFORM_H
