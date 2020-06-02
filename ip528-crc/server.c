#include "crc.h"

char NAME[32];
char ADDR[64];

void main(void *arg)
{
        int ret = -1;
        int sock = -1;
        struct sockaddr_in local_addr;//服务器端地址
        struct sockaddr_in from_addr;//客户端地址
        int from_len;
        int count = -1;
        fd_set readfds;
        char buff[BUFFER_LEN] = {0};
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        printf("====>HandleIPFound\n");

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0)
        {
                printf("HandleIPFound: sock init error\n");
                return;
        }

        int i = 0, if_num = 0;
        struct sockaddr_in  sin;
        struct ifconf ifconf;
        struct ifreq  *ifreq;
        unsigned char buf[512];
        char *p;
        char *status;
        /* 初始化ifconf */
        ifconf.ifc_len = 512;
        ifconf.ifc_buf = buf;

        /* 获取所有接口信息 */
        ioctl(sock, SIOCGIFCONF, (char *)&ifconf);

        /* 接下来一个一个的获取IP地址 */
        ifreq = (struct ifreq *)buf;
        if_num = ifconf.ifc_len/sizeof(struct ifreq);


        int reuse = 1;
        if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
        {
                return;
        }

        bzero(&local_addr, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(MCAST_PORT);

        ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
        if(ret != 0)
        {
                printf("HandleIPFound: bind error\n");
                return;
        }


        //while(1)
        p = ifreq;
        for(i = if_num; i > 0; i--)
        {
                FD_ZERO(&readfds);
                FD_SET(sock, &readfds);

                timeout.tv_sec = 2;
                timeout.tv_usec = 0;

                ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
                switch (ret)
                {
                case -1:
                        break;
                case 0:
                        printf("timeout\n");
                        break;
                default:
                        if(FD_ISSET(sock, &readfds))
                        {
                                from_len = sizeof(from_addr);
                                //接收客户端发送的数据
                                count = recvfrom(sock, buff, BUFFER_LEN, 0, (struct sockaddr *)&from_addr, &from_len);
                                //memset(buff, 0, BUFFER_LEN);
                                printf("Recv msg is %s\n", buff);

                                int crc1 = wtlib_crc16_tc(buff, strlen(buff));
				printf("crc1=%d\n", crc1);
                                //判断是否吻合
                                if(strstr(buff, IP_FOUND))
                                {
                                        sprintf(NAME, "name=[%s]\t", ifreq->ifr_name);
                                        sprintf(ADDR, "local addr=[%s]", inet_ntoa(((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr));

                                        status = (char *)malloc(256*sizeof(char));
                                        bzero(status, 256);
                                        sprintf(status, "%s%s", NAME, ADDR);
                                        printf("i=%d status: %s\n", i, status);

                                        //将应答数据复制进去
                                        //memcpy(status, IP_FOUND_ACK, strlen(IP_FOUND_ACK) + 1);
                                        printf("send msg to client\n");
                                        from_len = sizeof(from_addr);
                                        //将数据发送给客户端
                                        count = sendto(sock, status, strlen(status), 0, (struct sockaddr*)&from_addr, from_len);
                                	int crc2 = wtlib_crc16_tc(status, strlen(status));
					printf("crc2=%d\n\n", crc2);

                                        free(status);
                                }
                                else
                                {
                                        printf("not equal with IP_FOUND\n");
                                }
                        }
                }
                ifreq++;
                if(i == 1)
                {
                        i = if_num + 1;
                        ifreq = p;
                }
                sleep(1);

        }
        printf("<===HandleIPFound\n");
        return;
}

