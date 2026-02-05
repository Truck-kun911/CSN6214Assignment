#!/bin/sh

rm ./server;
gcc -g server.c utils.c game.c tcpmessage.c -o server;