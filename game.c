#include "game.h"

GameState *initGame()
{
    GameState *game = mmap(NULL, sizeof(GameState),
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    game->in_progress = 0;
    game->current_turn = 0;
    game->total_players = 0;
    game->start_vote = 0;
    game->winner = -1;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *player = malloc(sizeof(Player));
        player->id = i;
        player->confd = -1;
        player->voted = 0;
        player->recv_tid = -1;
        game->connected[i] = player;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&game->lock, &attr);

    return game;
}

void freeGame(GameState *game) {
    for (int i=0; i<MAX_PLAYERS;i++) {
        free(game->connected[i]);
    }
}

int connectPlayer(GameState *game, int confd)
{
    pthread_mutex_lock(&game->lock);
    int available_player_idx = -1;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (game->connected[i]->confd == -1)
        {
            available_player_idx = i;
            break;
        }
    }
    game->connected[available_player_idx]->confd = confd;
    game->total_players++;
    pthread_mutex_unlock(&game->lock);
    return available_player_idx;
}

void disconnectPlayer(GameState *game, int id)
{
    pthread_mutex_lock(&game->lock);
    int available_player_idx = -1;
    game->connected[id]->confd = -1;
    game->connected[id]->voted = 0;
    game->total_players--;
    pthread_mutex_unlock(&game->lock);
}