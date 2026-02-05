#!/bin/sh

rm ./client;
gcc -g client.c utils.c tcpmessage.c -o client;