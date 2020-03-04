/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

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
#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS_32 // Windows
#elif defined(_WIN64)
#define PLATFORM PLATFORM_WINDOWS_64 // Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS_CYGWIN // Windows (Cygwin POSIX under Microsoft Window)
#elif defined(__ANDROID__)
#define PLATFORM PLATFORM_ANDROID // Android (implies Linux, so it must come first)
#elif defined(__linux__)
#define PLATFORM PLATFORM_LINUX               // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1
#define PLATFORM PLATFORM_IOS_SIM // Apple iOS
#elif TARGET_OS_IPHONE == 1
#define PLATFORM PLATFORM_IOS // Apple iOS
#elif TARGET_OS_MAC == 1
#define PLATFORM PLATFORM_OSX // Apple OSX
#endif
#elif defined(__unix__)
#include <sys/param.h>
#if defined(BSD)
#define PLATFORM PLATFORM_BSD // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#endif
#elif defined(__hpux)
#define PLATFORM PLATFORM_HP_UX // HP-UX
#elif defined(_AIX)
#define PLATFORM PLATFORM_AIX // IBM AIX
#elif defined(__sun) && defined(__SVR4)
#define PLATFORM PLATFORM_SOLARIS // Oracle Solaris, Open Indiana
#endif
#endif

#if !defined(PLATFORM)
#define PLATFORM PLATFORM_UNKNOWN
#endif

#define PLATFORM_IS_WINDOWS(p_) (p_ == PLATFORM_WINDOWS_32 || p_ == PLATFORM_WINDOWS_64)
#define PLATFORM_IS_POSIX(p_) (p_ == PLATFORM_WINDOWS_CYGWIN || p_ == PLATFORM_LINUX || p_ == PLATFORM_OSX)

#define IS_BIG_ENDIAN (!*(unsigned char*) &(uint16_t){1})

#endif //IN3_PLATFORM_H
