#include "scheduler.h"

// Function to create a new scheduler
RoundRobinScheduler *create_scheduler(int capacity)
{
    RoundRobinScheduler *scheduler = (RoundRobinScheduler *)malloc(sizeof(RoundRobinScheduler));
    if (!scheduler)
    {
        printf("Memory allocation failed for scheduler\n");
        return NULL;
    }

    scheduler->items = (int *)malloc(capacity * sizeof(int));
    if (!scheduler->items)
    {
        printf("Memory allocation failed for items\n");
        free(scheduler);
        return NULL;
    }

    scheduler->size = 0;
    scheduler->capacity = capacity;
    scheduler->current_idx = 0;

    return scheduler;
}

// Function to add items to the scheduler
int add_item(RoundRobinScheduler *scheduler, int value)
{
    if (!scheduler)
    {
        printf("Scheduler is NULL\n");
        return -1;
    }

    if (scheduler->size >= scheduler->capacity)
    {
        printf("Scheduler is full. Cannot add more items.\n");
        return -1;
    }

    scheduler->items[scheduler->size] = value;
    scheduler->size++;

    return 0;
}

int current(RoundRobinScheduler *scheduler)
{
    if (!scheduler)
    {
        printf("Scheduler is NULL\n");
        return -1;
    }

    if (scheduler->size == 0)
    {
        printf("No items in scheduler\n");
        return -1;
    }

    return scheduler->items[scheduler->current_idx];
}

// Function to get the next item in round-robin fashion
int next(RoundRobinScheduler *scheduler)
{
    if (!scheduler)
    {
        printf("Scheduler is NULL\n");
        return -1;
    }

    if (scheduler->size == 0)
    {
        printf("No items in scheduler\n");
        return -1;
    }

    // Get the current item
    int next_item = scheduler->items[scheduler->current_idx];

    // Move to the next index (wrap around using modulo)
    scheduler->current_idx = (scheduler->current_idx + 1) % scheduler->size;

    return next_item;
}

// Function to print the current state of the scheduler
void print_scheduler(RoundRobinScheduler *scheduler)
{
    if (!scheduler)
    {
        printf("Scheduler is NULL\n");
        return;
    }

    printf("Scheduler items: ");
    for (int i = 0; i < scheduler->size; i++)
    {
        printf("%d", scheduler->items[i]);
        if (i < scheduler->size - 1)
        {
            printf(", ");
        }
    }
    printf("\n");
    printf("Size: %d, Current index: %d\n", scheduler->size, scheduler->current_idx);
}

// Function to free the scheduler memory
void free_scheduler(RoundRobinScheduler *scheduler)
{
    if (!scheduler)
        return;

    if (scheduler->items)
    {
        free(scheduler->items);
    }
    free(scheduler);
}