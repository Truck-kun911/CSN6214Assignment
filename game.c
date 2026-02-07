#include "game.h"

GameState *initGame()
{
    GameState *game = mmap(NULL, sizeof(GameState),
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    resetGame(game);
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *player = malloc(sizeof(Player));
        player->id = i;
        player->confd = -1;
        player->voted = 0;
        player->recv_tid = -1;
        player->wins = 0;
        game->connected[i] = player;
        game->position[i] = 0;
    }

    return game;
}

void resetGame(GameState *game)
{
    game->in_progress = 0;
    game->current_turn = 0;
    game->total_players = 0;
    game->start_vote = 0;
    game->winner = -1;
    if (game->scheduler != NULL)
        free_scheduler(game->scheduler);
    game->scheduler = create_scheduler(MAX_PLAYERS);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&game->lock, &attr);
}

void freeGame(GameState *game)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        free(game->connected[i]);
    }
    if (munmap(game, sizeof(GameState)) == -1)
    {
        perror("munmap failed");
    }
}

void lockGame(GameState *game)
{
    pthread_mutex_lock(&game->lock);
}

void unlockGame(GameState *game)
{
    pthread_mutex_unlock(&game->lock);
}

int connectPlayer(GameState *game, int confd, int id)
{
    lockGame(game);
    if (game->connected[id]->confd != -1)
    {
        unlockGame(game);
        return 0;
    }

    game->connected[id]->confd = confd;
    add_item(game->scheduler, id);
    unlockGame(game);
    return 1;
}

int connectNewPlayer(GameState *game, int confd)
{
    lockGame(game);
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
    unlockGame(game);
    return available_player_idx;
}

void disconnectPlayer(GameState *game, int id)
{
    lockGame(game);
    game->connected[id]->confd = -1;
    game->total_players--;
    unlockGame(game);
}

int handleStart(GameState *game, int player_id, char *args)
{
    lockGame(game);
    Player *voting_player = game->connected[player_id];

    if (voting_player->voted == 1)
    {
        unlockGame(game);
        return -1;
    }

    voting_player->voted = 1;
    game->start_vote++;
    if (
        game->total_players >= MIN_PLAYERS && game->start_vote >= game->total_players)
        game->in_progress = 1;
    unlockGame(game);
    return 1;
}

int handleWin(GameState *game, int player_id)
{
    lockGame(game);
    game->in_progress = 0;
    game->winner = player_id;
    unlockGame(game);
    return 1;
}

int handleRoll(GameState *game, int player_id, char *args)
{
    int current_player_id;
    lockGame(game);
    current_player_id = current(game->scheduler);

    if (current_player_id != player_id)
    {
        unlockGame(game);
        return -1;
    }

    int dice = rand() % MAX_DICE_ROLL + 1;
    game->position[player_id] += dice;
    next(game->scheduler);
    unlockGame(game);

    if (game->position[player_id] >= 21)
    {
        handleWin(game, player_id);
        return MAX_DICE_ROLL + 2;
    }

    return dice;
}

void getProgressBar(GameState *game, Player *player, char *buff)
{
    char progress_bar[PROGRESS_BAR_SIZE + 4] = "";
    lockGame(game);
    int player_position = game->position[player->id];
    int progress_rounded = roundedDivide(roundedDivide(player_position * 100, TARGET_POS), PROGRESS_BAR_SIZE);
    int whitespace_fill = PROGRESS_BAR_SIZE - progress_rounded;
    for (int i = 0; i < progress_rounded; i++)
    {
        if (i < progress_rounded - 1)
            strcat(progress_bar, "=");
        else
            strcat(progress_bar, ".o\"");
    }
    for (int i = 0; i < whitespace_fill; i++)
    {
        strcat(progress_bar, " ");
    }
    sprintf(buff, "%d: [%s] [%d]", player->id, progress_bar, player_position);
    unlockGame(game);
}

gamehandler_fn getCommandHandler(int command)
{
    if (command == CMD_START)
        return handleStart;

    if (command == CMD_ROLL)
        return handleRoll;

    return NULL;
}
