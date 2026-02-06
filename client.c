#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include "tcpmessage.h"

int lfd;
volatile int app_running = 1;
pthread_mutex_t app_lock;
volatile int client_id = -1;
int requested_id = -1;

void appExit()
{
    pthread_mutex_lock(&app_lock);
    app_running = 0;
    pthread_mutex_unlock(&app_lock);
}

void waitForPlayerId(char *message, char *msg_code)
{
    if (strncmp(msg_code, MSG_CONN, 2) == 0)
        sendMessage(lfd, "", MSG_CONN);
}

void handleInterrupt(int sig)
{
    appExit();
    pthread_mutex_destroy(&app_lock);
    close(lfd);
    printf("Connection closed.\n");
    exit(0);
}

void handleConnectionAccepted(char *message)
{
    printf("Incoming id %s\n", message);
    client_id = atoi(message);
}

void handleReceiveMessage(char *message)
{
    printf("\n[server] %s\n", message);
}

void handleReceiveEnd()
{
    appExit();
}

void handlePing() {}

void messageHandler(char *message, char *msg_code)
{
    if (strncmp(msg_code, MSG_CONN, 2) == 0)
        handleConnectionAccepted(message);

    if (strncmp(msg_code, MSG_PING, 2) == 0)
        handlePing();

    if (strncmp(msg_code, MSG_END, 2) == 0)
    {
        handleReceiveEnd();
        printf("Server closed connection. Press any key to exit.\n");
    }

    if (strncmp(msg_code, MSG_TEXT, 2) == 0)
    {
        handleReceiveMessage(message);
        printf("\n\nCommand: \n");
    }
}

void gameSend(char *message)
{
    char s_buff[100] = "";
    if (strncmp(message, "end", 3) == 0)
    {
        sprintf(s_buff, "%d", client_id);
        sendMessage(lfd, s_buff, MSG_END);
        app_running = 0;
    };
}

void handleArgs(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printf("Usage: %s [-h|--help] [-i|--id id]\n", argv[0]);
            exit(0); // Exit after showing help
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--id") == 0)
        {
            requested_id = atoi(argv[i + 1]);
            i++; // Skip next argv position.
        }
        else
        {
            // Treat as a positional argument (e.g., a filename)
            printf("Processing file: %s\n", argv[i]);
        }
    }
}

int main(int argc, char *argv[])
{
    handleArgs(argc, argv);

    signal(SIGINT, handleInterrupt);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&app_lock, &attr);

    struct sockaddr_in server;
    char s_buff[100] = "";

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = 8080;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(lfd, (struct sockaddr *)&server, sizeof server);
    createReceiveThread(lfd, messageHandler, (int *)&app_running);
    printf("Receive thread created.\n");
    sendMessage(lfd,
                requested_id == -1 ? "" : "" + requested_id,
                MSG_CONN);

    // Wait to receive id from client.
    while (client_id == -1)
    {
        sleep(1);
    }
    printf("Connected with id %d\n", client_id);

    while (app_running)
    {
        printf("\nCommand: \n");
        fgets(s_buff, sizeof s_buff, stdin);
        gameSend(s_buff);
    }

    handleInterrupt(1);

    return 0;
}
