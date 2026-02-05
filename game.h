#ifndef GAME_H
#define GAME_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>

#define MAX_PLAYERS 5
#define MIN_PLAYERS 3

typedef struct
{
    int id;
    int confd;
    bool voted;
    pthread_t recv_tid;
} Player;

typedef struct
{
    pthread_mutex_t lock;
    int position[MAX_PLAYERS];
    int current_turn;
    int total_players;
    int winner;
    int scores[MAX_PLAYERS];
    Player *connected[MAX_PLAYERS];
    int start_vote;
    int in_progress; /* 0 = waiting, 1 = game running */
} GameState;

GameState *initGame();
void freeGame(GameState *game);
int connectPlayer(GameState *game, int confd);
void disconnectPlayer(GameState *game, int id);

#endif