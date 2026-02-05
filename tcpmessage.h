#ifndef TCP_MESSAGE_H
#define TCP_MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MSG_ACK "00"
#define MSG_PING "01"
#define MSG_CONN "02"
#define MSG_END "03"
#define MSG_TEXT "04"
#define MSG_CMD "05"

pthread_t createReceiveThread(int fd, void (*handler)(char*, char*), int *running_flag);
void sendMessage(int fd, char* message, char* message_type);
void receiveMessage(int fd, void (*handler)(char*, char*));

#endif