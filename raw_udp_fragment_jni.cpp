#include "UdpWithRaw.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
	if (sock_raw <=0 || !buf || len<=0 || !src || src_port<=0 || !dst || dst_port<=0)
	{
		return -1;
	}

	static unsigned short id = 0;
	static unsigned mtu = 512;
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

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(dst_port);
	sin.sin_addr.s_addr = inet_addr(dst);

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
			ip->frag_off = htons(offset/8 | 0x2000);
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

int create_socket_raw()
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

	return fd;
}


JNIEXPORT jint JNICALL Java_UdpWithRaw_createRawSock
  (JNIEnv *env, jobject jo)
{
	return (jint)create_socket_raw();
}

JNIEXPORT jint JNICALL Java_UdpWithRaw_closeRawSock
  (JNIEnv *env, jobject jo, jint fd)
{
	if ((int) fd <= 0)
	{
		return (jint) -1;
	}

	close((int) fd);
	return (jint) 0;
}

JNIEXPORT jint JNICALL Java_UdpWithRaw_sentWithMtu
  (JNIEnv *env, jobject jo, jint fd, jbyteArray buf, jstring src, jint src_port, jstring dst, jint dst_port)
{
	if ((int)fd <= 0)
	{
		return (jint)-1;
	}

	const char *src_c = env->GetStringUTFChars(src, 0);
	const char *dst_c = env->GetStringUTFChars(dst, 0);
	
	const int len = env->GetArrayLength(buf);
	jbyte *buf_c = env->GetByteArrayElements(buf, NULL);

//int send_udp_with_mtu(int sock_raw, char *buf, unsigned short len, char *src, unsigned short src_port, char *dst, unsigned short dst_port)
	int r = send_udp_with_mtu((int)fd, (char *)buf_c, (unsigned short)len, (char *)src_c, (int) src_port, (char *)dst_c, (int)dst_port);

	env->ReleaseStringUTFChars(src, src_c);
	env->ReleaseStringUTFChars(dst, dst_c);
	env->ReleaseByteArrayElements(buf, buf_c, 0);

	if (r == 0)
	{
		return (jint) 0;
	}
	else
	{
		return (jint) -1;
	}
}


