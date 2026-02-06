#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <ctype.h>
#include <utils.h>
#include <time.h>
#include <stdint.h>

void toLowercase(char *str);
int contains(int arr[], int size, int target_value);
int64_t currentTimeMillis();

#endif