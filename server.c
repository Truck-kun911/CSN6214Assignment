#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>

#define PORT 8080
#define MAX_PLAYERS 5
#define TARGET_POS 30
#define BUF_SIZE 256

typedef struct {
    int position[MAX_PLAYERS];
    int current_turn;
    int total_players;
    int winner;
    pthread_mutex_t lock;
} GameState;

GameState *game;


void send_msg(int fd, const char *msg) {
    write(fd, msg, strlen(msg));
}

void *handle_turn(void *arg) {
    int *data = (int *)arg;
    int player_id = data[0];
    int client_fd = data[1];

    char buffer[BUF_SIZE];

    send_msg(client_fd, "Your turn! Type ROLL\n");

    int n = read(client_fd, buffer, sizeof(buffer)-1);
    if (n <= 0) return NULL;
    buffer[n] = '\0';

    pthread_mutex_lock(&game->lock);

    if (game->winner != -1) {
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }

    if (strcmp(buffer, "ROLL\n") != 0) {
        send_msg(client_fd, "Invalid command. Type ROLL\n");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }

    if (game->current_turn != player_id) {
        send_msg(client_fd, "Not your turn.\n");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }

    int dice = rand() % 6 + 1;
    game->position[player_id] += dice;

    sprintf(buffer,
            "You rolled %d. New position: %d\n",
            dice, game->position[player_id]);
    send_msg(client_fd, buffer);

    printf("Player %d rolled %d → position %d\n",
           player_id, dice, game->position[player_id]);

    if (game->position[player_id] >= TARGET_POS) {
        game->winner = player_id;
        sprintf(buffer, "Player %d WINS!\n", player_id);
        printf("%s", buffer);
    } else {
        game->current_turn =
            (player_id + 1) % game->total_players;
    }

    pthread_mutex_unlock(&game->lock);
    return NULL;
}

int main() {
    // Shared memory
    game = mmap(NULL, sizeof(GameState),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&game->lock, &attr);

    game->current_turn = 0;
    game->winner = -1;
    game->total_players = 0;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, MAX_PLAYERS);

    printf("Dice Racing Server started...\n");

    int connected_players = 0;

    while (connected_players < MAX_PLAYERS) {
        int client_fd = accept(server_fd, NULL, NULL);
        int my_id = connected_players;

        pthread_mutex_lock(&game->lock);
        game->total_players++;
        pthread_mutex_unlock(&game->lock);

        connected_players++;

        if (fork() == 0) {
            close(server_fd);

            srand(time(NULL) ^ getpid());

            char msg[64];
            sprintf(msg, "You are Player %d\n", my_id);
            send_msg(client_fd, msg);

            while (game->winner == -1) {
                if (game->current_turn == my_id) {
                    pthread_t tid;
                    int data[2] = {my_id, client_fd};
                    pthread_create(&tid, NULL, handle_turn, data);
                    pthread_join(tid, NULL);
                }
                sleep(1);
            }

            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
