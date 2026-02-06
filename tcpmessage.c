#include "tcpmessage.h"

typedef void (*handler_fn)(char *, char *);

typedef struct
{
    int fd;
    int *isRunning;
    handler_fn handler;
} ThreadLoopArgs;

void *receiveThread(void *arguments)
{
    ThreadLoopArgs *args = (ThreadLoopArgs *)arguments;
    handler_fn handler = &(*args->handler);
    while (*(args->isRunning))
    {
        char msg_code[3] = "";
        char message[98] = "";
        receiveMessage(args->fd, message, msg_code);
        handler(message, msg_code);
    }
}

pthread_t createReceiveThread(int fd, handler_fn handler, int *running_flag)
{
    pthread_t tid;

    // Seperate args for each thread.
    ThreadLoopArgs *args;
    args = malloc(sizeof(ThreadLoopArgs));
    (*args).fd = fd;
    (*args).handler = handler;
    (*args).isRunning = running_flag;

    if (pthread_create(&tid, NULL, receiveThread, args) != 0)
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

void receiveMessage(int fd, char *msg_buff, char *msg_code_buff)
{
    char r_buff[100] = "";
    if (recv(fd, r_buff, sizeof r_buff, 0) == -1)
        return;
    sscanf(r_buff, "%[^;];%[^;];", msg_code_buff, msg_buff);
}