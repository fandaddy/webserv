/*************************************************************************
	> File Name: socklib.c
	> Author:jiangxiaobai 
	> Mail: jiangxiaobai1989@gmail.com
	> Created Time: Tue 19 Jul 2016 09:36:39 AM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HOST "115.28.49.209"

int make_server_socket(int portnum)
{
    int sock_id;
    struct sockaddr_in saddr;
    
    sock_id = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_id == -1)
    {
        perror("socket");
        return -1;
    }

    bzero((void *)&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(portnum);
    saddr.sin_addr.s_addr = inet_addr(HOST);

    if(bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        perror("bind");
        return -1;
    }

    if( listen(sock_id, 1) != 0)
    {
        return -1;
    }

    return sock_id;

}
