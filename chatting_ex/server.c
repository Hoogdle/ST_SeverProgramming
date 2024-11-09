#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>


#define MAX_LENGTH 512 // 최대 문자열 길이 (채팅에 사용) 
#define MAX_SOCKET 1024 // 소켓 최대 갯수 

int tcp_listen(int host, int port, int backlog); // listen socket 만드는 함수, listen socket이란 사용자가 서버에 '처음' 접속할 때 이 '접속요청'을 받아들이는 소켓 
int max_socket_num(); // 최대 소켓 갯수를 리턴하는 함수, 이 함수의 리턴 값은 나중에 select() 함수에 사용됨 
void error(char *msg){perror(msg); exit(1);} // 에러 처리 함수 
void add_player(int socket_num, struct sockaddr_in *client_addr); // 사용자가 서버에 접속했을 때 접속한 사용자를 서버에 추가하는 과정을 처리하는 함수 


// 함수의 편리를 위해 전역변수 선언. 아래 변수들은 함수에서 사용될 변수들(인자로 안 받고 전역변수로써 편하게 사용하기 위함)
int player[MAX_SOCKET]; // 플레이어 소켓 배열 
int player_num = 0; // 플레이어 숫자(현재 접속한 클라이언트의 숫자) 
int listen_socket; // listen sokect 변수 


// user에 대한 정보 구조체

// 'user' 구조체는 '현재 접속한' 사용자를 다룰 때 사용 
struct user{
    char name[20]; // 유저 닉네임 
    int page; // 사용자가 머물고 있는 페이지 번호 표시 (1~4페이지 존재)
};

// 'user_database' 구조체는 모든 유저의 개인정보(id,pwd etc) 등을 저장할 구조체(추후 .txt 파일로 다룰 생각)
// id찾기, pwd찾기를 구현하는데에 필요 
struct user_databse{
    char id[20];
    char pwd[20];
    char name[20];
}; 


// start main 
int main(int argc, char* argv[]){
    int length;
    struct sockaddr_in addr; // 사용자의 ip 주소를 저장하는 변수 
    fd_set fds; // 소켓을 관리할 FD_num들의 배열 
    char msg[MAX_LENGTH]; // 채팅에서 사용될 문자열을 저장할 msg 변수 
    int player_socket_num; // 플레이어 소켓 임시 저장 변수 
    int struct_len = sizeof(struct sockaddr_in);

    int socket_max; // 최대 소켓의 fd_num을 저장할 변수, 나중에 select() 함수의 인자로 사용됨 


    if (argc != 2){printf("Please insert 'Port' in argument\n"); exit(0);}
    
    // =========================== 서버 초기 세팅 ============================//
    listen_socket = tcp_listen(INADDR_ANY, atoi(argv[1]), 5); // 사용자 '접속' 소켓을 생성하여 listen_socket 변수에 저장 
    socket_max = listen_socket; // 초기(아직 사용자가 아무도 접속하지 않은 상황)에는 listen_socket 만이 존재하므로 socket_max를 listen_socket으로 지정 
    printf("Hello Server!\n");
    // =======================================================================//

    // =========================== 서버 가동 =================================//
    // 반복문을 통해서 연속적으로 사용자의 요청을 처리하는 방법 

    while(1){
        FD_ZERO(&fds); // 모든 FD_NUM에 대하여 0으로 세팅(fds 변수는 fd_num의 배열(0~1024)임. fd_num은 각 소켓에 지정된 숫자를 의미) 
        FD_SET(listen_socket, &fds); // FD_SET은 select에서 '검사할' FD_NUM을 세팅. listen_socket을 검사할 fd_num을 지정. 즉, listen_socket을 '변화를 감지할' 소켓에 등록.
        for (int i = 0; i < player_num; i++){FD_SET(player[i], &fds);} // 현재 존재하는 모든 플레이어들에 대하여 FD_NUM 세팅. 즉, 모든 client의 소켓을 '변화를 감지할' 소켓에 등록. 

        socket_max += max_socket_num(); // select에서 검사 범위를 지정하기 위해 갖고 있는 소켓 중 가장 큰 FD_NUM 확인. 
        if(select(socket_max, &fds, NULL, NULL, NULL) < 0){error("select error");} // select 함수에서는 arg1으로 받은 숫자의 크기만큼 fds(fd_num의 배열)을 조사
        // select 함수의 역할은 어떤 소켓에서 신호가 왔을 때 이를 감지하는 역할을 함  
        
        if(FD_ISSET(listen_socket, &fds)){ // 접속을 담당하는 소켓에 1이 세팅 => 어떤 사용자가 서버에 접속함
        // 사용자의 접속이 없었다면 위 if문에서 listen_socket을 검사했을 때 FD_NUM = 0, 따라서 if문에 걸리지 않고 다음 블럭으로 넘어감. 
            player_socket_num = accept(listen_socket,(struct sockaddr*)&addr, &struct_len); // 해당 사용자의 FD_NUM을 player_socket_num에 accpet() 인자로 전달한 addr에는 주소를 받음 
            if(player_socket_num == -1){error("accept error");}
            printf("new player\n");
            add_player(player_socket_num,&addr); // 사용자 추가 부분 => player_socket_num을 player 소켓 배열에 저장, 주소 저장 
            printf("player num : %d\n",player_num);
        }

        // 별도로 보내드린 문서를 참고해주세요!
        
        for(int i=0; i<player_num; ++i){
            if(FD_ISSET(player[i],&fds)){

            length = read(player[i],msg,sizeof(msg));
            msg[length] = 0; // 종료문자처리
            printf("input str : %s\n",msg);
            for(int j=0; j<player_num; ++j){
                if (j==i) continue;
                write(player[j],msg,length);
                }
            }
        strcpy(msg,"\0");
        length = 0;
        }
    }
    // ======================================================================//




}

//  관례적으로 쓰이는 코드, 이해할 필요는 없음 
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
    player_num++;
}
