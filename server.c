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
    if (confd == -1)
        return;
    serverSendMessage(player, "", MSG_END);
    disconnectPlayer(game, player_id);
    close(confd);
    sem_post(&connection_sem);

    printf("Player %d disconnected.\n", player_id);
    if (game->total_players == 0)
        appExit();
}

void handleInterrupt(int sig)
{
    for (int i = 0; i < game->total_players; i++)
        if (game->connected[i]->confd != -1)
        {
            handleDisconnect(i);
        }

    printf("%d connections closed.\n", game->total_players);
    appExit();
    freeGame(game);
    close(lfd);
    sem_destroy(&connection_sem);
    pthread_mutex_destroy(&app_lock);
    printf("Socket closed.\n");
    exit(0);
}

void messageHandler(char *message, char *msg_code)
{
    if (strncmp(msg_code, MSG_END, 2) == 0)
    {
        int player_id = atoi(message);
        handleDisconnect(player_id);
    }
}

void *connectionHandlerThread(void *arg)
{
    while (app_running)
    {
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
        char id_buff[100] = "", code_buff[3] = "";

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
                char s_buff[100] = "";
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

void handleEndOfGame(GameState *game)
{
    if (game->in_progress == 1)
    {
        printf("Ending early.\n");
    }
    else
    {
        printf("Game won by %d.", game->winner);
    }

    for (int i = 0; i < game->total_players; i++)
    {
        serverSendMessage(game->connected[i], "", MSG_TEXT);
        serverSendMessage(game->connected[i], "", MSG_END);
        disconnectPlayer(game, i);
    }
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
            gameNextFrame(game);
        }
    }

    handleEndOfGame(game);

    handleInterrupt(1);
}