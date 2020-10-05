#pragma once

#include "globals.h"

#define CLA                0x80
#define INS_SIGN           0x02
#define INS_GET_PUBLIC_KEY 0x04

#define TAG_ARG1 0X01
#define TAG_ARG2 0X02

#define P1_LAST 0x80
#define P1_MORE 0x00

void main_loop();
