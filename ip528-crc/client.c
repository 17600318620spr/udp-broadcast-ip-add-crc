#include "crc.h"

void main(void *arg)
{
        int ret = -1;
        int sock = -1;
        int so_broadcast = 1;
        struct ifreq ifr;
        struct sockaddr_in broadcast_addr;//广播地址
        struct sockaddr_in from_addr;//服务端地址
        int from_len;
        int count = -1;
        fd_set readfd;//读文件描述符集合
        char buff[BUFFER_LEN] = {0};
        struct timeval timeout;
        timeout.tv_sec = 20;//超时时间为2秒
        timeout.tv_usec = 0;

        sock = socket(AF_INET, SOCK_DGRAM, 0);//建立数据报套接字
        if(sock < 0)
        {
                printf("HandleIPFound: socket init error\n");
                return;
        }
        //将使用的网络接口名字复制到ifr.ifr_name中，由于不同的网卡接口的广播地址是不一样的，因此指定网卡接口
        strcpy(ifr.ifr_name, IFNAME);
        //发送命令，获取网络接口的广播地址
        if(ioctl(sock, SIOCGIFBRDADDR, &ifr) == -1)
        {
                perror("ioctl error");
                exit(1);
        }
        //将获得的广播地址复制到broadcast_addr
        memcpy(&broadcast_addr, &ifr.ifr_broadaddr, sizeof(struct sockaddr_in));
        broadcast_addr.sin_port = htons(MCAST_PORT);//设置广播端口
        //默认的套接字描述符sock是不支持广播，必须设置套接字描述符以支持广播
        ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));
        //发送多次广播，看网络上是否有服务器存在
        int times = 10;
        int i = 0;
	int crc1 = 0, crc2 = 0;
        for(i = 0; i < times; i++)
        {
                //广播发送服务器地址请求
                ret = sendto(sock, IP_FOUND, strlen(IP_FOUND), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
                if(ret == -1)
                {
                        printf("sendto error!\n");
                        continue;
                }

                crc1 = wtlib_crc16_tc(IP_FOUND, strlen(IP_FOUND));
		printf("crc1=%d\n", crc1);

                //文件描述符清0
                FD_ZERO(&readfd);
                //将套接字文件描述符加入到文件描述符集合中
                FD_SET(sock, &readfd);
                timeout.tv_sec = 10;
                timeout.tv_usec = 0;
                //select侦听是否有数据到来
                ret = select(sock + 1, &readfd, NULL, NULL, &timeout);
                switch(ret)
                {
                case -1:
                        break;
                case 0:
                        printf("time out\n");
                        break;
                default:
                        //接收到数据
                        if(FD_ISSET(sock, &readfd))
                        {
                                from_len = sizeof(from_addr);
                                count = recvfrom(sock, buff, BUFFER_LEN, 0, (struct sockaddr*)&from_addr, &from_len);
                                printf("Client Recv msg is %s", buff);
                		crc2 = wtlib_crc16_tc(buff, strlen(buff));
				printf("crc2=%d\n\n", crc2);
                                memset(buff, 0, BUFFER_LEN);
#if 0
                                if(strstr(buff, IP_FOUND_ACK))
                                {
                                        printf("Client found server, IP is %s\n\n", inet_ntoa(from_addr.sin_addr));
                                        break;
                                }
#endif
                        }
                }
        }
        return;
}

