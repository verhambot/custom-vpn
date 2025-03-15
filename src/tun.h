#ifndef TUN_H
#define TUN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define TUN_NAME "tun0"

int tun_alloc(char *dev, int flags);

#endif // TUN_H
