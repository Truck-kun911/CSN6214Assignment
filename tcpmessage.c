#include "tcpmessage.h"

typedef struct
{
    int fd;
    int *isRunning;
    void (*handler)(char *, char *);
} ThreadLoopArgs;

void receiveThread(void *arguments)
{
    ThreadLoopArgs *args = (ThreadLoopArgs *)arguments;
    while (*(args->isRunning))
    {
        receiveMessage(args->fd, (void *)args->handler);
    }
}

pthread_t createReceiveThread(int fd, void (*handler)(char *, char *), int *running_flag)
{
    pthread_t tid;

    // Seperate args for each thread.
    ThreadLoopArgs *args;
    args = malloc(sizeof(ThreadLoopArgs));
    (*args).fd = fd;
    (*args).handler = handler;
    (*args).isRunning = running_flag;

    if (pthread_create(&tid, NULL, (void *)receiveThread, args) != 0)
    {
        perror("pthread_create failed");
        close(fd);
        return 0;
    }

    pthread_detach(tid);
    return tid;
}

void sendMessage(int fd, char *message, char *msg_code)
{
    if (msg_code == NULL)
        msg_code = MSG_TEXT;
    char s_buff[100] = "";
    sprintf(s_buff, "%s;%s;", msg_code, message);
    int t = send(fd, s_buff, sizeof message, 0);
    printf("\n");
}

void receiveMessage(int fd, void (*handler)(char *, char *))
{
    char r_buff[100] = "";
    ssize_t recv_success = recv(fd, r_buff, sizeof r_buff, 0);
    if (recv_success == -1)
        return;
    char msg_code[2];
    char message[97];
    sscanf(r_buff, "%[^;];%[^;];", &msg_code, &message);
    handler(message, msg_code);
}