#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#define BUFSIZE 2048

#define UNDEF_MODE 0
#define SERVER_MODE 1
#define CLIENT_MODE 2

#define INTERFACE "tun0"
#define PORT 5555
#define FLAGS (IFF_TUN | IFF_NO_PI)


// prints program usage/help
void usage(char *progname) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [-i <interface>] (-s | -c <server_ip>) [-p <port>]\n", progname);
    fprintf(stderr, "%s -h\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "-i <interface>: Name of interface (defaults to %s)\n", INTERFACE);
    fprintf(stderr, "-s | -c <server_ip>: Run in server mode (-s), or specify server address (-c <server_ip>)\n");
    fprintf(stderr, "-p <port>: Port to listen on (if run in server mode) or to connect to (in client mode), (defaults to %d)\n", PORT);
    fprintf(stderr, "-h: Prints this help text\n");
    exit(EXIT_FAILURE);
}


// allocates and connects to a TUN interface
int tun_alloc(char *dev, int flags) {

    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if((fd = open(clonedev , O_RDWR)) < 0) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}


// read routine that checks for errors
int cread(int fd, char *buf, int n) {

    int nread;

    if ((nread = read(fd, buf, n)) < 0) {
        perror("Reading data");
        exit(EXIT_FAILURE);
    }

    return nread;
}


// write routine that checks for errors
int cwrite(int fd, char *buf, int n) {

    int nwrite;

    if ((nwrite = write(fd, buf, n)) < 0) {
        perror("Writing data");
        exit(EXIT_FAILURE);
    }

    return nwrite;
}


// read routine to read an exact number of bytes
int read_n(int fd, char *buf, int n) {

    int nread;
    int remaining_bytes = n;

    while (remaining_bytes > 0) {

        if ((nread = cread(fd, buf, n)) == 0) {
            return EXIT_SUCCESS;
        } else {
            remaining_bytes -= nread;
            buf += nread;
        }
    }

    return n;
}


// print custom error messages
void print_err(char *msg, ...) {

    va_list arg;

    va_start(arg, msg);
    vfprintf(stderr, msg, arg);
    va_end(arg);
}


// print packet information
void print_packet(char *buffer, int size) {

    struct iphdr *iph = (struct iphdr *) buffer;

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(iph->saddr), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(iph->daddr), dst_ip, INET_ADDRSTRLEN);

    printf("\n-- IP Header --\n");
    printf("Version: %d\n", iph->version);
    printf("Protocol: %d\n", iph->protocol);
    printf("Source IP: %s\n", src_ip);
    printf("Destination IP: %s\n", dst_ip);

    // int ip_header_length = iph->ihl * 4;

    if (iph->protocol == IPPROTO_TCP) {
        struct tcphdr *tcph = (struct tcphdr *) (buffer + iph->ihl * 4);
        
        printf("\n-- TCP Header --\n");
        printf("Source Port: %d\n", ntohs(tcph->source));
        printf("Destination Port: %d\n", ntohs(tcph->dest));

        // int tcp_header_length = tcph->doff * 4;
        // int data_offset = ip_header_length + tcp_header_length;
        // int data_length = size - data_offset;
    }

    else if (iph->protocol == IPPROTO_UDP) {
        struct udphdr *udph = (struct udphdr *) (buffer + iph->ihl * 4);

        printf("\n-- UDP Header --\n");
        printf("Source Port: %d\n", ntohs(udph->source));
        printf("Destination Port: %d\n", ntohs(udph->dest));

        // int udp_header_length = 8;
        // int data_offset = ip_header_length + udp_header_length;
        // int data_length = size - data_offset;
    }

    printf("\n");
    printf("------------------------------------------------\n");
}


int main(int argc, char *argv[]) {

    char *progname = argv[0];

    char interface_name[IFNAMSIZ] = INTERFACE;

    int tun_fd;
    int net_fd;
    int sock_fd;
    int max_fd;

    int sock_optval = 1;

    fd_set read_fds;

    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    int mode = UNDEF_MODE;

    int port = PORT;

    char remote_ip[16] = "";

    socklen_t remote_len;

    int nread;
    int nwrite;
    int packet_len;
    char buffer[BUFSIZE];

    int option;

    while ((option = getopt(argc, argv, "i:sc:p:h")) > 0) {

        switch(option) {

            case 'h':
                usage(progname);
                break;

            case 'i':
                strncpy(interface_name, optarg, IFNAMSIZ - 1);
                break;

            case 's':
                if (mode != UNDEF_MODE) {
                    print_err("Error : program can only run as either server or client\n");
                    usage(progname);
                }

                mode = SERVER_MODE;
                break;

            case 'c':
                if (mode != UNDEF_MODE) {
                    print_err("Error : program can only run as either server or client\n");
                    usage(progname);
                }

                mode = CLIENT_MODE;
                strncpy(remote_ip, optarg, 15);
                break;

            case 'p':
                port = atoi(optarg);
                break;
        }
    }

    argv += optind;
    argc -= optind;

    if (argc > 0) {
        print_err("Error : too many options!\n");
        usage(progname);
    }

    if (mode == UNDEF_MODE) {
        print_err("Error : specify server or client mode!\n");
        usage(progname);
    }
    else if ((mode == CLIENT_MODE) && (*remote_ip == '\0')) {
        print_err("Error : specify server IP in client mode!\n");
        usage(progname);
    }


    tun_fd = tun_alloc(interface_name, FLAGS);
    if (tun_fd < 0) {
        print_err("Error connecting to TUN interface %s\n", interface_name);
        exit(EXIT_FAILURE);
    }
    printf("Successfully connected to TUN interface %s\n", interface_name);


    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }


    if (mode == CLIENT_MODE) {

        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_addr.s_addr = inet_addr(remote_ip);
        remote_addr.sin_port = htons(port);

        if (connect(sock_fd, (struct sockaddr *) &remote_addr, sizeof(remote_addr)) < 0) {
            perror("connect()");
            exit(EXIT_FAILURE);
        }

        net_fd = sock_fd;
        printf("CLIENT: Connected to server %s\n", inet_ntoa(remote_addr.sin_addr));
    }

    else if (mode == SERVER_MODE) {

        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *) &sock_optval, sizeof(sock_optval)) < 0) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(port);

        if (bind(sock_fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
            perror("bind()");
            exit(EXIT_FAILURE);
        }

        if (listen(sock_fd, 5) < 0) {
            perror("listen()");
            exit(EXIT_FAILURE);
        }

        remote_len = sizeof(remote_addr);
        memset(&remote_addr, 0, remote_len);
        if ((net_fd = accept(sock_fd, (struct sockaddr *) &remote_addr, &remote_len)) < 0) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        printf("SERVER: Client connected from %s\n", inet_ntoa(remote_addr.sin_addr));
    }


    max_fd = (tun_fd > net_fd) ? tun_fd : net_fd;

    while (1) {

        FD_ZERO(&read_fds);
        FD_SET(tun_fd, &read_fds);
        FD_SET(net_fd, &read_fds);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0 && errno == EINTR){
            continue;
        }

        if (activity < 0) {
            perror("select()");
            exit(EXIT_FAILURE);
        }


        if (FD_ISSET(tun_fd, &read_fds)) {

            nread = cread(tun_fd, buffer, BUFSIZE);
            printf("\nTUN2NET : Read %d bytes from TUN interface\n", nread);
            print_packet(buffer, nread);

            packet_len = htons(nread);
            nwrite = cwrite(net_fd, (char *) &packet_len, sizeof(packet_len));
            nwrite = cwrite(net_fd, buffer, nread);
            printf("\nTUN2NET : Written %d bytes to NET interface\n", nwrite);
        }

        if (FD_ISSET(net_fd, &read_fds)) {

            nread = read_n(net_fd, (char *) &packet_len, sizeof(packet_len));
            if (nread == 0) {
                break;
            }

            nread = read_n(net_fd, buffer, ntohs(packet_len));
            printf("\nNET2TUN : Read %d bytes from NET interface\n", nread);
            print_packet(buffer, nread);

            nwrite = cwrite(tun_fd, buffer, nread);
            printf("\nNET2TUN : Written %d bytes to the TUN interface\n", nwrite);

        }

    }

    return EXIT_SUCCESS;
}
