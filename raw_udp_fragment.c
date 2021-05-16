#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>

unsigned short csum(unsigned short *buf, int nwords)
{
	unsigned long sum;
	for (sum=0; nwords>0; nwords--)
	{
		sum += *buf++;
	}

	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (unsigned short) (~sum);
}

int send_udp_with_mtu(int sock_raw, char *buf, unsigned short len, char *src, unsigned short src_port, char *dst, unsigned short dst_port)
{
	static unsigned short id = 0;
	static unsigned mtu = 16;
	char tmp[1024] = {0};
	struct iphdr *ip = (struct iphdr *)tmp;
	struct udphdr *udp = (struct udphdr *)(tmp + sizeof(struct iphdr));

	id++;

	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 16;
	ip->id = id;
	ip->protocol = 17;
	ip->saddr = inet_addr(src);
	ip->daddr = inet_addr(dst);

	udp->source = htons(src_port);
	udp->dest = htons(dst_port);
	udp->len = htons(len + sizeof(struct udphdr));

	ip->check = 0;

	if (mtu - sizeof(struct udphdr) < len)
	{
		memcpy(tmp + sizeof(struct iphdr) + sizeof(struct udphdr), buf, mtu - sizeof(struct udphdr));
		ip->tot_len = htons(sizeof(struct iphdr) + mtu);
		ip->frag_off = htons(0x2000);
	}
	else
	{
		ip->frag_off = htons(0);
		memcpy(tmp + sizeof(struct iphdr) + sizeof(struct udphdr), buf, len);
		ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + len);
	}

	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(dst_port),
		.sin_addr.s_addr = inet_addr(dst)
	};
	if (-1 == sendto(sock_raw, tmp, ntohs(ip->tot_len), 0, (struct sockaddr *)&sin, sizeof(sin)))
	{
		perror("send1");
		return -1;
	}

	for (int i=mtu-sizeof(struct udphdr); i<len; i+=mtu)
	{
		unsigned short offset = (i + sizeof(struct udphdr));
		if (offset + mtu > len)
		{
			ip->frag_off = htons(offset/8);
			ip->tot_len = htons(sizeof(struct iphdr) + len - i);
		}
		else
		{
			ip->frag_off = htons(offset/8 | 0x20000);
			ip->tot_len = htons(sizeof(struct iphdr) + mtu);
		}
		memcpy(tmp + 20, buf + i, ntohs(ip->tot_len) - sizeof(struct iphdr));
		ip->check = 0;
		ip->id = id;
		
		if (-1 == sendto(sock_raw, tmp, ntohs(ip->tot_len), 0, (struct sockaddr *)&sin, sizeof(sin)))
		{
			perror("send2");
			return -1;
		}
	}


	return 0;
}

int main()
{
	int fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
	if (fd == -1)
	{
		perror("raw socket");
		return -1;
	}

	int one = 1;
	if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1)
	{
		perror("setsockopt");
		close(fd);
		return -1;
	}

	char *buf = "adcdefggehijklmnbghggxxxy";
	send_udp_with_mtu(fd, buf, strlen(buf), "192.168.66.128", 7777, "232.1.1.1", 8888);
	close(fd);
}
