#!/bin/sh

rm ./server;
gcc -g server.c gameutils.c scheduler.c game.c tcpmessage.c -o server;