#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080
#define BUF_SIZE 256

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server-ip>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };

    inet_pton(AF_INET, argv[1], &server.sin_addr);
    connect(sock, (struct sockaddr *)&server, sizeof(server));

    char buffer[BUF_SIZE];

    while (1) {
        int n = read(sock, buffer, sizeof(buffer)-1);
        if (n <= 0) break;

        buffer[n] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Your turn")) {
            fgets(buffer, sizeof(buffer), stdin);
            write(sock, buffer, strlen(buffer));
        }
    }

    close(sock);
    return 0;
}
