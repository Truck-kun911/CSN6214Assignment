#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

void toLowercase(char *str);
int contains(int arr[], int size, int target_value);
int64_t currentTimeMillis();
int roundedDivide(int a, int b);

#endif