#ifndef IN3_VERSION_H
#define IN3_VERSION_H

#define IN3_COPYRIGHT "2018 - 2019 Slock.it, <info@slock.it>."
#define IN3_VERSION_BITS(x, y, z) ((x) << 16 | (y) << 8 | (z))
#define IN3_AT_LEAST_VERSION(x, y, z) (IN3_VERSION_NUM >= IN3_VERSION_BITS(x, y, z))

// Below defines are obtained from cmake
//#define IN3_VERSION "0.0.1"
//#define IN3_VERSION_MAJOR 0
//#define IN3_VERSION_MINOR 0
//#define IN3_VERSION_PATCH 1

#define IN3_VERSION_NUM IN3_VERSION_BITS(IN3_VERSION_MAJOR, IN3_VERSION_MINOR, IN3_VERSION_PATCH)

#endif //IN3_VERSION_H
