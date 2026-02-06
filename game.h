#ifndef GAME_H
#define GAME_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "scheduler.h"
#include "gameutils.h"

#define MAX_PLAYERS 5
#define MIN_PLAYERS 3

#define CMD_NONE 0
#define CMD_ROLL 1


typedef struct
{
    int id;
    int confd;
    bool voted;
    pthread_t recv_tid;
    int command_buffer;
    bool command_changed;
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
    RoundRobinScheduler *scheduler;
} GameState;

GameState *initGame();
void freeGame(GameState *game);
int connectPlayer(GameState *game, int confd, int id);
int connectNewPlayer(GameState *game, int confd);
void disconnectPlayer(GameState *game, int id);
int setPlayerCommand(GameState *game, int player_id, char* command);
void gameNextFrame(GameState *game);

#endif
