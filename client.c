#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int tcp_connect(int af, char *servip, unsigned short port);
void error(char* msg){perror(msg); exit(1);}

int main(int argc, char* argv[]){
    int s;
	char data[1024];
    s = tcp_connect(AF_INET, argv[1], atoi(argv[2]));
    if(s==-1){error("socket error");}

    puts("Joined Sever!\n");
	while(1){
		printf("Please insert any msg : ");
        scanf("%s",data);
        write(s,data,strlen(data));
	}
}

int tcp_connect(int af, char *servip, unsigned short port) {
	struct sockaddr_in servaddr;
	int  s;
    char buf[256];
	// 소켓 생성
	if ((s = socket(af, SOCK_STREAM, 0)) < 0)
		return -1;
	// 채팅 서버의 소켓주소 구조체 servaddr 초기화
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = af;
	inet_pton(AF_INET, servip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	// 연결요청
	if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))
		< 0)
		return -1;
        
	return s;
}
