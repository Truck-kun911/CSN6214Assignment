#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>

// Structure for the round-robin scheduler
typedef struct {
    int* items;      // Array to store the items
    int size;        // Number of items
    int capacity;    // Maximum capacity
    int current_idx; // Current index for round-robin
} RoundRobinScheduler;

RoundRobinScheduler *create_scheduler(int capacity);
int add_item(RoundRobinScheduler *scheduler, int value);
int current(RoundRobinScheduler *scheduler);
int next(RoundRobinScheduler *scheduler);
void print_scheduler(RoundRobinScheduler *scheduler);
void free_scheduler(RoundRobinScheduler *scheduler);

#endif