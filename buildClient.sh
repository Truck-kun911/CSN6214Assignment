#!/bin/sh

rm ./client;
gcc -g client.c gameutils.c tcpmessage.c -o client;