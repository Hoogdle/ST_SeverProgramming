// 선택문제
// a. 현재까지 수업시간에 fork와 thread만을 다룸
// 만약 소켓 프로그래밍에서 FD_* 시리즈와 select함수를 다룬다면 이 방법으로 구현
// 그렇지 않다면 a. 방법을 활용하여 구현 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>


#define MAX_LENGTH 512 // 오류 나면 하나 줄여보기
#define MAX_SOCKET 1024
#define BUF_SIZE 1024

int tcp_listen(int host, int port, int backlog);
int max_socket_num();
void error(char *msg){perror(msg); exit(1);}
void add_player(int socket_num, struct sockaddr_in *client_addr);
void _main();
void chatting();
void selecter(int socket_num);
void* utility(void *arg){
    printf("Command : hello\n");
}
// argv[1]로 포트번호를 받음


struct users{
    char name[20];
    int page; // 사용자가 머물고 있는 페이지 번호 표시 (1~4페이지 존재)
};
// user에는 현재 서버에 '접속한' 사용자에 관한 정보 저장

struct user_database{
    char id[20];
    char pwd[20];
    char name[20];
}; // user_database에는 '등록된' 사용자에 관한 정보를 모두 저장 

// 함수의 편리를 위해 전역변수 선언 
int player[MAX_SOCKET]; // 플레이어 소켓 
int player_num = 0; // 플레이어 숫자
int account_num = 0;
int listen_socket;
struct users user[MAX_SOCKET];
struct user_database* data_base;
char buf[BUF_SIZE];
fd_set fds;


int main(int argc, char* argv[]){
    int length;
    struct sockaddr_in addr;
    pthread_t thread;
    char msg[MAX_LENGTH];
    int player_socket_num; // 플레이어 소켓 임시 저장 변수 
    int struct_len = sizeof(struct sockaddr_in);
    int socket_max;
    data_base = (struct user_database*)malloc(sizeof(struct user_database));
    

    if (argc != 2){printf("Please insert 'Port' in argument\n"); exit(0);}

    listen_socket = tcp_listen(INADDR_ANY, atoi(argv[1]), 5); // 사용자 '접속' 소켓 
    socket_max = listen_socket;
    //pthread_create(&thread, NULL, utility, (void *)NULL);
    printf("Hello Server!\n");
    while(1){
        FD_ZERO(&fds); // 모든 FD_NUM에 대하여 0으로 세팅 
        FD_SET(listen_socket, &fds); // FD_SET은 select에서 '검사할' FD_NUM을 세팅 
        for (int i = 0; i < player_num; i++){FD_SET(player[i], &fds);} // 현재 존재하는 모든 플레이어들에 대하여 FD_NUM 세팅 

        //socket_max = max_socket_num(player,player_num);
        socket_max += max_socket_num(); // select에서 검사 범위를 지정하기 위해 갖고 있는 소켓 중 가장 큰 FD_NUM 확인 
        if(select(socket_max, &fds, NULL, NULL, NULL) < 0){error("select error");} // fd의 변화를 감지하는 함수, 이를 통해 클라이언트의 접속을 감지할 수 있음. 여기서부터!

        if(FD_ISSET(listen_socket, &fds)){ // 접속을 담당하는 소켓에 1이 세팅 => 어떤 사용자가 서버에 접속함
        // 사용자의 접속이 없었다면 위 if문에서 listen_socket을 검사했을 때 FD_NUM = 0, 따라서 if문에 걸리지 않고 다음 블럭으로 넘어감. 
            player_socket_num = accept(listen_socket,(struct sockaddr*)&addr, &struct_len); // 해당 사용자의 FD_NUM을 player_socket_num에 accpet() 인자로 전달한 addr에는 주소를 받음 
            if(player_socket_num == -1){error("accept error");}
            printf("new player\n");
            add_player(player_socket_num,&addr); // 사용자 추가 부분 => player_socket_num을 player 소켓 배열에 저장, 주소 저장 
            printf("player num : %d\n",player_num);
            _main(player_socket_num);
        } // end of connect check

        for(int i=0; i<player_num; ++i){
            if(FD_ISSET(player[i],&fds)){
                selecter(player[i]);
            }

        }
    }
}

int  tcp_listen(int host, int port, int backlog) {
    int sd;
    struct sockaddr_in servaddr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        perror("socket fail");
        exit(1);
    }
    // servaddr 구조체의 내용 세팅
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(host);
    servaddr.sin_port = htons(port);
    if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind fail");  exit(1);
    }
    // 클라이언트로부터 연결요청을 기다림
    listen(sd, backlog);
    return sd;
}

int max_socket_num(){
    int max = 0;
    max = listen_socket;
    for(int i=0;i<player_num;++i){
        if(max < player[i]) max = player[i];
    }
    return max + 1; // +1로 전달해야 원하는 길이까지 확인가능 
}

void add_player(int socket_num, struct sockaddr_in *client_addr){
    char buf[20]; // 주소를 저장할 buf 
    inet_ntop(AF_INET, &client_addr->sin_addr, buf, sizeof(buf)); // 네트워크 정보(빅 인디언 이진데이터) => AF_INET 프로토콜(IPV4)로 변환 
    player[player_num] = socket_num;
    user[socket_num].page = 1; // 처음 접속한 유저의 페이지를 1로 설정 
    player_num++;
}

void _main(int socket_num){
    int fd = open("/home/ty/project/interface/main.txt",O_RDONLY);
    char buf[BUF_SIZE];
    read(fd,buf,BUF_SIZE-1);
    write(socket_num,buf,strlen(buf));
}

void chatting(){
    int length;
    for(int i=0; i<player_num; ++i){
        if(FD_ISSET(player[i],&fds)){

            length = read(player[i],buf,sizeof(buf));
            buf[length] = 0; // 종료문자처리
            printf("input str : %s\n",buf);
            for(int j=0; j<player_num; ++j){
                if (j==i) continue;
                write(player[j],buf,length);
            }
        }
        strcpy(buf,"\0");
        length = 0;
    }
}

void selecter(int socket_num){
    int page;
    int select;
    read(socket_num,buf,sizeof(buf));

    page = user[socket_num].page;
    select = atoi(buf);

    printf("page : %d, selcet : %d\n",page,select);

/*    switch(page){
        case 1:
            page1(select);
            break;
        case 2:
            page2(select);
            break;
        case 3:
            page3(select);
            break;
        case 4:
            page4(select);
            break; */
}
