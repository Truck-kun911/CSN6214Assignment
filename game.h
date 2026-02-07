#ifndef GAME_H
#define GAME_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include "scheduler.h"
#include "gameutils.h"

#define MAX_PLAYERS 5
#define MIN_PLAYERS 3
#define PROGRESS_BAR_SIZE 10
#define TARGET_POS 21
#define MAX_DICE_ROLL 6

#define CMD_NONE 0
#define CMD_ROLL 1
#define CMD_START 2

typedef struct
{
    int id;
    int confd;
    bool voted;
    pthread_t recv_tid;
    int wins;
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

typedef int (*gamehandler_fn)(GameState *, int, char *);

GameState *initGame();
void resetGame(GameState *game);
void freeGame(GameState *game);
void lockGame(GameState *game);
void unlockGame(GameState *game);
int connectPlayer(GameState *game, int confd, int id);
int connectNewPlayer(GameState *game, int confd);
void disconnectPlayer(GameState *game, int id);
void getProgressBar(GameState *game, Player *player, char *buff);
gamehandler_fn getCommandHandler(int command);

#endif
