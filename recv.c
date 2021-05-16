#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int main()
{
    int port = 8888;
    char *mul = "232.1.1.1";
    char *source = "192.168.66.128";
    char *local = "192.168.65.1";
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("create socket failed.");
        return -1;
    }

    struct sockaddr_in bind_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(mul)
    };

    if (-1 == bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)))
    {
        perror("bind failed.");
        return 0;
    }

    struct ip_mreq_source ssm = {
        .imr_interface.s_addr = inet_addr(local),
        .imr_multiaddr.s_addr = inet_addr(mul),
        .imr_sourceaddr.s_addr = inet_addr(source)
    };

    if (-1 == setsockopt(fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &ssm, sizeof(ssm)))
    {
        perror("join failed.");
        return -1;
    }

    while (1)
    {
        char buf[65536] = {0};
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        int n = recvfrom(fd, buf, 65536, 0, (struct sockaddr *)&addr, &addr_len);
        printf("recv-> %s %d %s\n", inet_ntoa(addr.sin_addr), n, buf);
    }

}
