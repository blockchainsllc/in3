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

#ifndef IN3_TEST_UTILS_H
#define IN3_TEST_UTILS_H

#include "unity/unity.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TESTS_BEGIN() UNITY_BEGIN()
#define TESTS_END() UNITY_END()
#define TEST_LOG(fmt_, ...) printf("%s:%d:%s:LOG:" fmt_, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define TEST_LOG_INTERNAL(f_, fmt_, ...) printf("%s:%d:%s:LOG:" fmt_, __FILE__, __LINE__, f_, __VA_ARGS__)

// Timing
#define TIMING_START() gettimeofday(&begin, NULL)
#define TIMING_END() gettimeofday(&end, NULL)
#define TIMING_GET() ((double) (end.tv_usec - begin.tv_usec) / 1000000 + (double) (end.tv_sec - begin.tv_sec))

#define RUN_TIMED_TEST(t)                                      \
  do {                                                         \
    TIMING_START();                                            \
    RUN_TEST(t);                                               \
    TIMING_END();                                              \
    TEST_LOG_INTERNAL(#t, "Completed in %fs\n", TIMING_GET()); \
  } while (0)

// if t is NULL, adds 10 to previously returned value and returns it
// otherwise expects t to point to a uint64_t value; if this value is non-zero the same value is returned
// otherwise the previously returned value is returned
static inline uint64_t mock_time(void* t) {
  static uint64_t now = 0;
  if (t)
    now = (*(uint64_t*) t) ? *(uint64_t*) t : now;
  else
    now += 10;
  return now;
}

// a not-so-random number generator
// starts with zero and returns a number that is one more than the previously returned value, unless
// s is not NULL, in which case s is treated as a pointer to int and it's `pointed-to' value is returned
static inline int mock_rand(void* s) {
  static uint64_t rand = 0;
  if (s)
    rand = *(int*) s;
  else
    rand += 1;
  return rand;
}

#ifdef __cplusplus
}
#endif

#endif //IN3_TEST_UTILS_H
