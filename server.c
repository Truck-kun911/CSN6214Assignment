#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include "gameutils.h"
#include "tcpmessage.h"
#include "game.h"

#define PLAYER_ID_WAIT_MS 500

volatile int app_running = 1;
pthread_mutex_t app_lock;
sem_t connection_sem;

int lfd; // listen file descriptor
struct sockaddr_in client, server;
GameState *game;

void appExit()
{
    pthread_mutex_lock(&app_lock);
    app_running = 0;
    pthread_mutex_unlock(&app_lock);
}

void initSocket(struct sockaddr_in server)
{
    lfd = socket(AF_INET, SOCK_STREAM, 0);

    bind(lfd, (struct sockaddr *)&server, sizeof server);
    listen(lfd, MAX_PLAYERS);
}

void serverSendMessage(Player *player, char *message, char *msg_code)
{
    if (player->confd == -1)
    {
        printf("Unable to send message to disconnected player %d.\n", player->id);
        return;
    }
    sendMessage(player->confd, message, msg_code);
}

void handleDisconnect(int player_id)
{
    Player *player = game->connected[player_id];
    int confd = game->connected[player_id]->confd;

    if (confd != -1)
    {
        serverSendMessage(player, "", MSG_END);
        disconnectPlayer(game, player_id);
        close(confd);
        printf("Player %d disconnected.\n", player_id);
    }

    sem_post(&connection_sem);

    if (game->total_players == 0)
        appExit();
}

void handleEndOfGame(GameState *game)
{
    if (game->in_progress == 1)
    {
        printf("Ending early.\n");
        // TODO: Handle persistence. Handle early ending.
    }
    else if (game->winner != -1)
    {
        for (int i = 0; i < game->total_players; i++)
        {
            char win_buff[250] = "";
            if (i == game->winner)
                sprintf(win_buff, "You have won the game! Congratulations!");
            else
                sprintf(win_buff, "Player %d has won the game! Better luck next time.", game->winner);
            serverSendMessage(game->connected[i], win_buff, MSG_TEXT);
        }
        resetGame(game);
    }
}

void handleCommand(char *message)
{
    int player_id, command;
    char args[200];

    sscanf(message, "%d:%d:%s", &player_id, &command, args);

    if (game->in_progress == 0)
    {
        serverSendMessage(game->connected[atoi(message)], "Game has not started yet.", MSG_TEXT);
        return;
    }

    Player *sender_player = game->connected[player_id];
    gamehandler_fn handler = getCommandHandler(command);

    if (handler == NULL)
    {
        serverSendMessage(sender_player, "Invalid command.", MSG_TEXT);
        return;
    }

    if (command == CMD_START)
    {
        int vote_result = handler(game, player_id, NULL);
        if (vote_result == -1)
        {
            serverSendMessage(sender_player, "You have already voted to start.", MSG_TEXT);
            return;
        }
        char s_buff[200] = "";
        sprintf(s_buff, "Player %d has voted to start.", player_id);
        for (int i = 0; i < game->total_players; i++)
        {
            serverSendMessage(game->connected[i], s_buff, MSG_TEXT);
        }
        printf("%s\n", s_buff);
        return;
    }

    if (command == CMD_ROLL)
    {
        int roll_result = handler(game, player_id, NULL);
        char s_buff[200] = "";

        if (roll_result == -1)
        {
            int current_player_id = current(game->scheduler);
            sprintf(s_buff, "Cannot roll on player %d's turn.", current_player_id);
            serverSendMessage(sender_player, s_buff, MSG_TEXT);
            return;
        }

        char progress_bars_buff[100] = "";
        for (int i = 0; i < game->total_players; i++)
        {
            Player *player = game->connected[i];
            char progress_buff[100] = "";
            getProgressBar(game, player, progress_buff);
            sprintf(progress_bars_buff, "%s\n%s", progress_bars_buff, progress_buff);
        }

        for (int i = 0; i < game->total_players; i++)
        {
            char s_buff[250] = "";
            char player_name[10] = "";
            if (i == player_id)
                sprintf(player_name, "You");
            else
                sprintf(player_name, "Player %d", player_id);

            sprintf(
                s_buff,
                "%s\n%s rolled a %d!",
                progress_bars_buff,
                player_name,
                roll_result);

            int next_player_id = current(game->scheduler);

            if (i == next_player_id)
                sprintf(s_buff, "%s\nYour turn to roll.", s_buff);

            serverSendMessage(game->connected[i], s_buff, MSG_TEXT);
        }

        if (roll_result == MAX_DICE_ROLL + 2)
        {
            handleEndOfGame(game);
        }

        return;
    }
}

void messageHandler(char *message, char *msg_code)
{
    if (strncmp(msg_code, MSG_END, 2) == 0)
    {
        int player_id = atoi(message);
        handleDisconnect(player_id);
    }

    if (strncmp(msg_code, MSG_CMD, 2) == 0)
    {
        handleCommand(message);
    }
}

void *connectionHandlerThread(void *arg)
{
    while (app_running)
    {
        while (game->in_progress)
        {
        }

        sem_wait(&connection_sem);
        printf("Creating new connection handler thread.\n");
        printf("Waiting for players to connect: %d players waiting.\n", game->total_players);
        int n, confd;
        n = sizeof client;
        confd = accept(lfd, (struct sockaddr *)&client, &n);

        if (confd == -1)
        {
            perror("accept failed");
            pthread_exit((void *)0);
        }

        // See if incoming connection is requesting a specific id.
        int64_t start_time = currentTimeMillis();
        int player_id = -1;
        char id_buff[200] = "", code_buff[3] = "";

        receiveMessage(confd, id_buff, code_buff);
        if (strncmp(code_buff, MSG_CONN, 2) == 0 && id_buff[0] != '\0')
            player_id = atoi(id_buff);

        if (player_id == -1)
            player_id = connectNewPlayer(game, confd);
        else
        {
            int connect_success = connectPlayer(game, confd, player_id);
            if (!connect_success)
            {
                char s_buff[200] = "";
                sprintf(s_buff, "Player id %d is already in use.", player_id);
                sendMessage(confd, s_buff, MSG_TEXT);
                sendMessage(confd, "", MSG_END);
                pthread_exit((void *)0);
            }
        }

        // Handle each player in a seperate thread.
        game->connected[player_id]->recv_tid = createReceiveThread(confd, messageHandler, (int *)&app_running);
        printf("Receive thread created for player %d.\n", player_id);

        char message[1];
        sprintf(message, "%d", player_id);
        printf("Player connected with id %s.\n", message);
        sendMessage(confd, message, MSG_CONN);
        printf("CONN sent.\n");
    }
}

void cleanup()
{
    for (int i = 0; i < game->total_players; i++)
        if (game->connected[i]->confd != -1)
        {
            handleDisconnect(i);
        }

    printf("%d connections closed.\n", game->total_players);
    appExit();
    freeGame(game);
    sem_destroy(&connection_sem);
    pthread_mutex_destroy(&app_lock);
    close(lfd);
    printf("Socket closed.\n");
}

void handleInterrupt(int sig)
{
    cleanup();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handleInterrupt);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&app_lock, &attr);

    sem_init(&connection_sem, 0, MAX_PLAYERS);

    server.sin_family = AF_INET;
    server.sin_port = 8080;
    server.sin_addr.s_addr = INADDR_ANY;

    initSocket(server);
    game = initGame();

    pthread_t conection_tid;
    if (pthread_create(&conection_tid, NULL, connectionHandlerThread, NULL) != 0)
    {
        perror("pthread_create failed");
        handleInterrupt(1);
    }
    pthread_detach(conection_tid);

    while (app_running)
    {
        if (!game->in_progress)
        {
            printf("Waiting for start...\n");
            sleep(1);
        }
        else
        {
            printf("Game in progress...\n");
            sleep(1);
        }
    }

    handleEndOfGame(game);

    cleanup();
}