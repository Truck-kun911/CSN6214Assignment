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
    game->scheduler = create_scheduler(MAX_PLAYERS);

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *player = malloc(sizeof(Player));
        player->id = i;
        player->confd = -1;
        player->voted = 0;
        player->recv_tid = -1;
        player->command_buffer = CMD_NONE;
        game->connected[i] = player;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&game->lock, &attr);

    return game;
}

void freeGame(GameState *game)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        free(game->connected[i]);
    }
}

int connectPlayer(GameState *game, int confd, int id)
{
    pthread_mutex_lock(&game->lock);
    if (game->connected[id]->confd != -1)
        return 0;
    game->connected[id]->confd = confd;
    add_item(game->scheduler, id);
    pthread_mutex_unlock(&game->lock);
    return 1;
}

int connectNewPlayer(GameState *game, int confd)
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
    add_item(game->scheduler, available_player_idx);
    pthread_mutex_unlock(&game->lock);
    return available_player_idx;
}

void disconnectPlayer(GameState *game, int id)
{
    pthread_mutex_lock(&game->lock);
    game->connected[id]->confd = -1;
    game->total_players--;
    pthread_mutex_unlock(&game->lock);
}

int setPlayerCommand(GameState *game, int player_id, char *command)
{
    int incoming_command = -1;

    if (strncmp(command, "roll", 4) == 0)
        incoming_command = CMD_ROLL;

    if (incoming_command == -1)
        return 0;

    game->connected[player_id]->command_buffer = incoming_command;
    return 1;
}

void handleRoll(GameState *game, int player_id)
{
    printf("handle roll\n");
}

int handlePlayerCommand(GameState *game, int player_id)
{
    int current_player_id;
    pthread_mutex_lock(&game->lock);
    current_player_id = current(game->scheduler);
    pthread_mutex_unlock(&game->lock);

    if (current_player_id != player_id)
        return 0;

    if (game->connected[player_id]->command_buffer == CMD_ROLL)
        handleRoll(game, player_id);

    return 1;
}

void gameNextFrame(GameState *game)
{
    int frame_handled = 0;
    while (frame_handled == 0)
    {
        for (int i = 0; i < game->total_players; i++)
        {
            frame_handled = handlePlayerCommand(game, i);
        }
    }
    next(game->scheduler);
}