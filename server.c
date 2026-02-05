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
#include "utils.h"
#include "tcpmessage.h"
#include "game.h"

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

void handleDisconnect(int player_id)
{
    int confd = game->connected[player_id]->confd;
    if (confd == -1)
        return;
    sendMessage(confd, "", MSG_END);
    disconnectPlayer(game, player_id);
    close(confd);
    sem_post(&connection_sem);
}

void handleInterrupt(int sig)
{
    appExit();

    for (int i = 0; i < game->total_players; i++)
        if (game->connected[i]->confd != -1)
        {
            handleDisconnect(i);
        }

    printf("%d connections closed.\n", game->total_players);
    freeGame(game);
    close(lfd);
    sem_destroy(&connection_sem);
    pthread_mutex_destroy(&app_lock);
    printf("Socket closed.\n");
    exit(0);
}

void *messageHandler(char *message, char *msg_code)
{
    printf("Handling message\n");
    if (strncmp(msg_code, MSG_END, 2) == 0)
    {
        int playerId = atoi(message);
        handleDisconnect(playerId);
        printf("Player %d disconnected.\n", playerId);
        if (game->total_players == 0)
            app_running = 0;
    }
}

void connectionHandlerThread()
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
            return;
        }

        int player_id = connectPlayer(game, confd);
        // Handle each player in a seperate thread.
        game->connected[player_id]->recv_tid = createReceiveThread(confd, (void *)messageHandler, (int *)&app_running);
        printf("Receive thread created for player %d.\n", player_id);

        char message[1];
        sprintf(message, "%d", player_id);
        printf("Player connected with id %s.\n", message);
        sendMessage(confd, message, MSG_CONN);
    }
}

int main()
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
    if (pthread_create(&conection_tid, NULL, (void *)connectionHandlerThread, NULL) != 0)
    {
        perror("pthread_create failed");
        handleInterrupt(1);
    }
    pthread_detach(conection_tid);

    
    while (app_running)
    {
        printf("Running\n");
        sleep(1);
    }

    printf("No active players, ending game...\n");

    handleInterrupt(1);
}