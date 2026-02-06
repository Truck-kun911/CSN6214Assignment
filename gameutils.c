#include "gameutils.h"

void toLowercase(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        str[i] = tolower(str[i]);
    }
}

int contains(int arr[], int size, int target_value) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == target_value) {
            return 1; // Value found, return 1 (true) and exit function
        }
    }
    return 0; // Value not found after checking all elements, return 0 (false)
}

int64_t currentTimeMillis() {
    struct timespec spec;
    // CLOCK_REALTIME is a common clock ID for wall-clock time
    timespec_get(&spec, TIME_UTC); 
    // Convert seconds to milliseconds and add milliseconds from the nanosecond field
    int64_t ms = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    return ms;
}

int roundedDivide(int a, int b) {
    return (a + (b / 2)) / b;
}