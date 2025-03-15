#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tun.h"
#include "packet.h"

int main(int argc, char *argv[]) {
    
    char tun_name[IFNAMSIZ];
    unsigned char buffer[BUFFER_SIZE];
    int tun_fd, nread;

    if (argc > 1) {
        strncpy(tun_name, argv[1], IFNAMSIZ - 1);
    } else {
        strcpy(tun_name, TUN_NAME);
    }

    tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);
    if (tun_fd < 0) {
        perror("Failed to allocate TUN device");
        return 1;
    }

    printf("TUN device %s created. Waiting for packets...\n", tun_name);


    while(1) {
        nread = read(tun_fd, buffer, BUFFER_SIZE);
        if (nread < 0) {
            perror("Reading from TUN interface");
            close(tun_fd);
            return 1;
        }

        printf("\nReceived packet with %d bytes from %s\n", nread, tun_name);
        print_packet(buffer, nread);
    }

    return 0;
}
