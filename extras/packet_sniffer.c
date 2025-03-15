#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 2048
#define TUN_NAME "tun0"

int tun_alloc(char *dev, int flags) {

    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Error opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return fd;
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}


void print_packet(unsigned char *buffer, int size) {

    struct iphdr *iph = (struct iphdr *) buffer;

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(iph->saddr), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(iph->daddr), dst_ip, INET_ADDRSTRLEN);

    printf("\n-- IP Header --\n");
    printf("Version: %d\n", iph->version);
    printf("IHL: %d\n", iph->ihl);
    printf("TTL: %d\n", iph->ttl);
    printf("Protocol: %d\n", iph->protocol);
    printf("Source IP: %s\n", src_ip);
    printf("Destination IP: %s\n", dst_ip);
    printf("Type Of Service: %d\n", iph->tos);
    printf("IP Total Length: %d Bytes\n", ntohs(iph->tot_len));
    printf("Identification: %d\n", ntohs(iph->id));
    printf("Checksum: %d\n", ntohs(iph->check));

    int ip_header_length = iph->ihl * 4;

    if (iph->protocol == IPPROTO_TCP) {
        struct tcphdr *tcph = (struct tcphdr *) (buffer + iph->ihl * 4);
        
        printf("\n-- TCP Header --\n");
        printf("Source Port: %d\n", ntohs(tcph->source));
        printf("Destination Port: %d\n", ntohs(tcph->dest));
        printf("Sequence Number: %u\n", ntohl(tcph->seq));
        printf("Acknowledgement Number: %u\n", ntohl(tcph->ack_seq));
        printf("Header Length: %d\n", tcph->doff * 4);
        printf("Flags: ");
        if (tcph->fin) printf("FIN ");
        if (tcph->syn) printf("SYN ");
        if (tcph->rst) printf("RST ");
        if (tcph->psh) printf("PSH ");
        if (tcph->ack) printf("ACK ");
        if (tcph->urg) printf("URG ");
        printf("\n");

        int tcp_header_length = tcph->doff * 4;
        int data_offset = ip_header_length + tcp_header_length;
        int data_length = size - data_offset;

        if (data_length > 0) {
            printf("\n-- TCP Data --\n");

            printf("Data as UTF-8: ");
            for (int i = 0; i < data_length; i++) {
                if (isprint(buffer[data_offset + i])) {
                    printf("%c", buffer[data_offset + i]);
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
    }

    else if (iph->protocol == IPPROTO_UDP) {
        struct udphdr *udph = (struct udphdr *) (buffer + iph->ihl * 4);

        printf("\n-- UDP Header --\n");
        printf("Source Port: %d\n", ntohs(udph->source));
        printf("Destination Port: %d\n", ntohs(udph->dest));
        printf("Length: %d\n", ntohs(udph->len));

        int udp_header_length = 8;
        int data_offset = ip_header_length + udp_header_length;
        int data_length = size - data_offset;
        
        if (data_length > 0) {
            printf("\n-- UDP Data --\n");

            printf("Data as UTF-8: ");
            for (int i = 0; i < data_length; i++) {
                if (isprint(buffer[data_offset + i])) {
                    printf("%c", buffer[data_offset + i]);
                } else {
                    printf(".");
                }
            }
            printf("\n");

            if (buffer[data_offset + data_length - 1] == 0) {
                printf("Data as string: %s\n", buffer + data_offset);
            }
        }
    }

    printf("\n-- Raw Packet Data (Hex) --\n");
    for (int i = 0; i < size; i++) {
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    printf("------------------------------------------------\n");

}


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
