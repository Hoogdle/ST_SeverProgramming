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
void* selecter(void* socket_num);
void page1(int n,int s_n);
void page1_1(int socket_num);
int login_id(int socket_num);
int login_pw(int socket_num);
void _page2(int socket_num);
void page1_4(int socket_num);
void page1_2(int s_n);
void page1_3(int s_n);
void page1_0(int s_n);
int check_dupli(char* input);
// argv[1]로 포트번호를 받음


struct users{
    char name[20];
    int page; // 사용자가 머물고 있는 페이지 번호 표시 (1~4페이지 존재)
};

// user에는 현재 서버에 '접속한' 사용자에 관한 정보 저장

/*struct user_database{
    char id[20];
    char pwd[20];
    char name[20];
};*/ // user_database에는 '등록된' 사용자에 관한 정보를 모두 저장 

// 함수의 편리를 위해 전역변수 선언 
int player[MAX_SOCKET]; // 플레이어 소켓 
int player_num = 0; // 플레이어 숫자
int account_num = 0;
int listen_socket;
int data_base;
struct users user[MAX_SOCKET];
char buf[BUF_SIZE];
int sign[BUF_SIZE] ={0,};
int room_num = 1; // 할당할 방 번호 
int rooms[MAX_SOCKET] = {0,}; // 방에 있는 사람의 숫자를 세기 위한 함수 

fd_set fds;


int main(int argc, char* argv[]){
    int length;
    struct sockaddr_in addr;
    pthread_t thread[MAX_SOCKET];
    char msg[MAX_LENGTH];
    int player_socket_num; // 플레이어 소켓 임시 저장 변수 
    int struct_len = sizeof(struct sockaddr_in);
    int socket_max;
    //data_base = (struct user_database*)malloc(sizeof(struct user_database));
    

    if (argc != 2){printf("Please insert 'Port' in argument\n"); exit(0);}


    // fork() 함수로 자식 -> 서버~클라이언트, 부모 -> 서버 관리전용 만들
    listen_socket = tcp_listen(INADDR_ANY, atoi(argv[1]), 5); // 사용자 기'접속' 소켓
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
                printf("there is some signal!\n");
                if(sign[player[i]] == 1) continue;
                sign[player[i]]=1;
                printf("thread created\n");
                pthread_create(&thread[player[i]],NULL,selecter,(void*)&player[i]);
                //selecter(player[i]);
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
    char tmp[10000];
    read(fd,tmp,10000-1);
    write(socket_num,tmp,strlen(tmp));
    bzero(tmp,sizeof(tmp));
}

void _room(int s_n){
    int fd = open("/home/ty/project/interface/room_list.txt",O_RDONLY);
    char tmp[10000];
    read(fd,tmp,sizeof(tmp)-1);
    write(s_n,tmp,strlen(tmp));
    bzero(tmp,sizeof(tmp));
}

void _page2(int socket_num){
    int fd = open("/home/ty/project/interface/room_list.txt",O_RDONLY);
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

// 각 페이지마다 페이지 담당 함수로 보내주기 
void* selecter(void* socket_num){
    char input[1000];
    int* tmp = (int*)socket_num;
    int s_n = *tmp;
    printf("socket_num : %d\n",s_n);
    int page;
    int select;
    read(s_n,input,sizeof(input));

    page = user[s_n].page;
    select = atoi(input);

    printf("page : %d, selcet : %d\n",page,select);

    switch(page){
        case 1:
            page1(select, s_n);
            sign[s_n] = 0;
            break;
        case 2:
            user[s_n] = 2;
            page2(select);
            break;
        /* 
        case 3:
            page3(select);
            break;
        case 4:
            page4(select);
            break; */
    }
}

// 페이지1 담당 함수 
void page1(int n,int s_n){
    switch(n){
        case 1:
            // 로그인 
            // thread create 로 부르기 
            page1_1(s_n);
            break;
       case 2:
            // ID 찾기 
            // use thread_create 
            page1_2(s_n);
            break;
       case 3:
            // PW 찾기
            // use thread_create 
            page1_3(s_n);
            break; 
        case 4:
            // Sign-up
            // use thread_create 
            page1_4(s_n);
            _main(s_n);
            break;
        case 0:
            // Exit 
            page1_0(s_n);
            break;

    }
}

void page1_1(int socket_num){
    int length;
    char tmp[1024];
    char t1[1024];
    char t2[1024] = "ID를 입력해주세요 \n";
    char t3[1024] = "PW를 입력해주세요 \n"; 
    char t4[1024] = "해당 ID가 존재하지 않습니다.\n";
    char t5[1024] = "해당 PW가 존재하지 않습니다.\n";
    int fd = open("/home/ty/project/interface/login_page.txt",O_RDONLY);
    read(fd,tmp,1024-1);
    write(socket_num,tmp,strlen(tmp));
    sprintf(t1,"현재 접속중인 인원 : %d\n",player_num);
    write(socket_num,t1,strlen(t1)); 
    while(1){
        write(socket_num,t2,strlen(t2));
        if(login_id(socket_num) == 0){ 
            write(socket_num,t4,sizeof(t4));
            continue;
        }
        break;
    }
    while(1){
        write(socket_num,t3,strlen(t3));
        if(login_pw(socket_num) == 0){ 
            write(socket_num,t5,sizeof(t5));
            continue;
        }
        break;
    }
    _page2(socket_num);
}
    

// id를 저장할 때, <id>{사용자 아이디}로 저장 
// pw는 <pw>{사용자 비번}으로 저장 

// 입력한 정보가 일치한 것이 있다면 리턴 1, 없다면 리턴 0 
int login_id(int socket_num){
    char id[1000] = "<id>";
    char tmp[1024];
    char input[1000];
    char null[3] = "\0";

    read(socket_num,input,sizeof(input)); // 사용자에게서 pw  읽기 

    FILE* data = fopen("/home/ty/project/database.txt","r");
    strncat(id,input,sizeof(id)-strlen(id)-1);
    strncat(id,null,sizeof(id)-strlen(id)-1);
   

    while(fgets(tmp,sizeof(tmp),data) != NULL){
        tmp[strlen(tmp)-1] = '\0';
        if(strstr(tmp,id)!=NULL) {
            fclose(data);
            bzero(input,sizeof(input));
            return 1;
        }
    } 
    fclose(data);
    bzero(input,sizeof(input));
    return 0;
}
        
int login_pw(int socket_num){
    char pwd[100000] = "<pw>";
    char tmp[1024];
    char input[10000];
    char null[3] = "\0";

    read(socket_num,input,sizeof(input)); // 사용자에게서 pw  읽기 
    
    FILE* data = fopen("/home/ty/project/database.txt","r");
    strncat(pwd,input,sizeof(pwd)-strlen(pwd)-1);
    strncat(pwd,null,sizeof(pwd)-strlen(pwd)-1);
   
    while(fgets(tmp,sizeof(tmp),data) != NULL){
        tmp[strlen(tmp)-1] = '\0';
        if(strstr(tmp,pwd)!=NULL){
            bzero(input,sizeof(input));
            fclose(data);
            return 1;
        }
    }
    bzero(input,sizeof(input));
    fclose(data);
    return 0;
}


// 회원가입 함수 
void page1_4(int socket_num){
    char info[10000]; 
    char info_id[] = "ID를 입력해주세요\n";
    char info_id2[] = "중복되는 ID 입니다. 다른 ID를 입력해주세요\n";
    char info_pw[] = "PW를 입력해주세요\n";
    char info_un[] = "닉네임을 입력해주세요\n";
    char info_un2[] = "중복되는 닉네임 입니다. 다른 닉네임을 입력해주세요\n"; 
    char finish[] = "회원가입이 완료 되었습니다!\n";
    char id[1000];
    char pw[1000];
    char un[1000];
    char id_tag[] = "<id>";
    char pw_tag[] = "<pw>";
    char un_tag[] = "<un>";
    

    int data = open("/home/ty/project/database.txt",O_RDWR | O_APPEND);
    char tmp[1024];
    int fd = open("/home/ty/project/interface/signup_page.txt",O_RDONLY);

    read(fd,info,sizeof(info)-1);
    write(socket_num,info,strlen(info));
    
    // 추후 중복아이디, 중복 닉네임 거부 코드 추가 

    // 사용자에게 Id 입력받음 
    while(1){
        write(socket_num,info_id,strlen(info_id));
        read(socket_num,tmp,sizeof(tmp));
        strcat(id,id_tag);
        strcat(id,tmp);
        if(check_dupli(id)==1){ 
            write(data,id,strlen(id));
            bzero(tmp,sizeof(tmp));
            bzero(id,sizeof(id));
            break;
        }
        //write(data,id,strlen(id));
        write(socket_num,info_id2,strlen(info_id2));
        printf("tmp is : %s\n",tmp);
        printf("saved id : %s\n",id);
        bzero(id,sizeof(id));
        bzero(tmp,sizeof(tmp));
    }
    // 사용자에게 Pw 입력받음 
    write(socket_num,info_pw,strlen(info_pw));
    read(socket_num,tmp,sizeof(tmp));
    strcat(pw,pw_tag);
    strcat(pw,tmp);
    write(data,pw,strlen(pw));
    printf("tmp is : %s\n",tmp);
    bzero(tmp,sizeof(tmp));
    printf("saved pw : %s\n",pw);

    // 사용자에게 닉네임 입력받음 
    while(1){
        write(socket_num,info_un,strlen(info_un));
        read(socket_num,tmp,sizeof(tmp));
        //tmp[strlen(tmp)] = '\n';
        strcat(un,un_tag);
        strcat(un,tmp);
        if(check_dupli(un)==1){
            write(data,un,strlen(un));
            bzero(un,sizeof(un));
            bzero(tmp,sizeof(tmp));
            break;
        }
        write(socket_num,info_un2,strlen(info_un2));
        printf("tmp is : %s\n",tmp);
        bzero(un,sizeof(un));
        bzero(tmp,sizeof(tmp));
        printf("saved un : %s\n",un);
    }

    close(data);
    close(fd);
    
    write(socket_num,finish,strlen(finish));

    bzero(id,sizeof(id));
    bzero(pw,sizeof(pw));
    bzero(un,sizeof(un));
    bzero(info,sizeof(info));
    bzero(finish,sizeof(finish));
}

// id_찾기 함수 
void page1_2(int s_n){
    FILE* fd = fopen("/home/ty/project/database.txt","r");
    char un[1000] = "<un>";
    char data[1000];
    char tmp[1000];
    char id[] = "<id>";
    char pw[] = "<pw>";
    char returns[1000];
    char info_1[] = "닉네임을 입력해주세요\n";
    char info_2[] = "찾으시는 ID : ";
    write(s_n,info_1,strlen(info_1));
    read(s_n, data, sizeof(data));
    strcat(un,data);

    while(fgets(tmp,sizeof(tmp),fd) != NULL){
        printf("tmp is : %s\n",tmp);
        printf("un is : %s\n",un);
        tmp[strlen(tmp)-1] = '\0';
        char* p;
        char* g;
        int gap;
        if((strstr(tmp,un))!=NULL){
            p = strstr(tmp,id);
            g = strstr(tmp,pw);
            gap = (g-p)/sizeof(char);
            printf("gap is : %d\n",gap);
            strncat(returns,tmp+(4*sizeof(char)),gap-4);
            printf("returns : %s\n",returns);
            returns[strlen(returns)] = '\n';
            returns[strlen(returns)] = '\0';
            write(s_n,info_2,strlen(info_2));
            write(s_n,returns,strlen(returns));
            bzero(un,sizeof(un));
            bzero(data,sizeof(data));
            bzero(tmp,sizeof(tmp));
            bzero(id,sizeof(id));
            bzero(pw,sizeof(pw));
            bzero(returns,sizeof(returns));
            bzero(info_1,sizeof(info_1));
            bzero(info_2,sizeof(info_2));
            _main(s_n);
            return ;
        }
    }
    bzero(un,sizeof(un));
    bzero(data,sizeof(data));
    bzero(tmp,sizeof(tmp));
    bzero(id,sizeof(id));
    bzero(pw,sizeof(pw));
    bzero(returns,sizeof(returns));
    bzero(info_1,sizeof(info_1));
    bzero(info_2,sizeof(info_2));
    return;
}



// 비밀번호 찾기 함수 
void page1_3(int s_n){
    FILE* fd = fopen("/home/ty/project/database.txt","r");
    char id[1000] = "<id>";
    char data[1000];
    char tmp[1000];
    char pw[] = "<pw>";
    char un[] = "<un>";
    char returns[1000];
    char info_1[] = "ID를 입력해주세요\n";
    char info_2[] = "찾으시는 PW : ";
    write(s_n,info_1,strlen(info_1));
    read(s_n, data, sizeof(data));
    strcat(id,data);

    while(fgets(tmp,sizeof(tmp),fd) != NULL){
        printf("tmp is : %s\n",tmp);
        printf("id is : %s\n",id);
        tmp[strlen(tmp)-1] = '\0';
        char* p;
        char* g;
        int gap;
        if((strstr(tmp,un))!=NULL){
            p = strstr(tmp,pw);
            g = strstr(tmp,un);
            gap = (g-p)/sizeof(char);
            printf("gap is : %d\n",gap);
            strncat(returns,p+(4*sizeof(char)),gap-4);
            printf("returns : %s\n",returns);
            returns[strlen(returns)] = '\n';
            returns[strlen(returns)] = '\0';
            write(s_n,info_2,strlen(info_2));
            write(s_n,returns,strlen(returns));
            bzero(un,sizeof(un));
            bzero(data,sizeof(data));
            bzero(tmp,sizeof(tmp));
            bzero(id,sizeof(id));
            bzero(pw,sizeof(pw));
            bzero(returns,sizeof(returns));
            bzero(info_1,sizeof(info_1));
            bzero(info_2,sizeof(info_2));
            _main(s_n);
            return ;
        }
    }
    bzero(un,sizeof(un));
    bzero(data,sizeof(data));
    bzero(tmp,sizeof(tmp));
    bzero(id,sizeof(id));
    bzero(pw,sizeof(pw));
    bzero(returns,sizeof(returns));
    bzero(info_1,sizeof(info_1));
    bzero(info_2,sizeof(info_2));
    return;
}

void page1_0(int s_n){
    char info[] = "EXIT SERVER\n";
    write(s_n,info,strlen(info));
    close(s_n);
    player_num--;
    bzero(info,sizeof(info));
}

int check_dupli(char* input){
    char tmp[1000];
    FILE* data = fopen("/home/ty/project/database.txt","r");
    while(fgets(tmp,sizeof(tmp),data) != NULL){
        tmp[strlen(tmp)-1] = '\0';
        if(strstr(tmp,input)!=NULL){
            fclose(data);
            bzero(tmp,sizeof(tmp));
            return 0; // 중복된 경우 리턴 0 
        }
    }
    bzero(tmp,sizeof(tmp));
    fclose(data);
    return 1; // 중복되지 않은 경우 리턴 1 
}

void page2(int n,int s_n){
    char info[] = "올바르지 않은 입력입니다. 다시 입력해주세요\n";
    // page2 함수 상단에서 이미 허용되지 않는 input을 처리했음 
    // 따라서 switch 아래의 함수들은 안심하고 각자의 역할을 하면 됨 
    if(n>room_num || n < (-1)){
        char tmp[100];
        while(1){
            write(s_n,info,strlen(info));
            read(s_n,tmp,sizeof(tmp));
            n = atoi(tmp);
            if(n == -1){
                user[s_n] = 1;
                _main(s_n);
                return;
            }
            if(n<=room_num && n > (-1)) break; // 올바른 정보 입력됨 
        }
    }
    
    switch(n){
       case -1:
            // input : -1 => go to main page
            user[s_n] = 1;
            _main(s_n);
            return;
       case 0: // creat room
            // ID 찾기 
            // use thread_create 
            page2_0(s_n);
            break;
       default : // join room
            page2_n(s_n,n);
            break;

    }
}

void make_room(int s_n,int r_n, char* room_name){
    int fd;
    char route[1000];
    char result[10000];
    char inter1[] = "=====================================================\n";
    char head[] = "HEAD COUNT : 1 (Game needs 5 members)\n"
    char start[] = "START GAME : PLEASE ENTER '1'\n"
    char quit[] = "QUIT ROOM : PLEASE ENTER '0'\n"
    char tmp[1000];
    char e_room_name[1000];
    spritnf(tmp,"1. %s\n",user[s_n].name);
    sprintf(e_room_name,"            %s\n",room_name);
    strcat(result,inter1);
    strcat(result,e_room_name);
    strcat(result,inter1);
    strcat(result,head);
    strcat(result,start);
    strcat(result,quit);
    strcat(result,inter1);
    strcat(result,tmp);
    sprintf(route,"/home/ty/project/interface/rooms/%d.txt",r_n);
    fd = open(route,O_TRUNC | O_WRONLY);
    write(fd,result,strlen(result));

    rooms[r_n] = 1; // 방을 만들면 만든 사람이 방에 들어가 있으므로 방 인원 1명으로 설정. 

    close(fd);
    bzero(route,sizeof(route));
    bzero(result,sizeof(result));
    bzero(inter1,sizeof(inter1));
    bzero(head,sizeof(head));
    bzero(start,sizeof(start));
    bzero(quit,sizeof(quit));
    bzero(tmp,sizeof(tmp));
    bzero(e_room_name,sizeof(e_room_name));
}
    


// 유저가 방을 생성하는 함수 
void page2_0(int s_n){
    int r_n = room_num // 타 클라이언트가 동시에 방을 생성하면 룸 넘버가 겹칠 수 있기 때문에 room_creat 함수에 들어오는 순간 바로 저장 
    int fd = open("/home/ty/project/interface/room_list.txt",O_RDWR | O_APPEND); // room list에 대한 정보를 저장하고 있는 파일 
    int wd;
    char input[1000];
    char room_name[1000];
    char file_name[1000];
    char route[1000];
    char info[] = "생성할 방 제목을 입력하세요\n";
    char room_info[10000];
    write(s_n,info,strlen(info));
    read(s_n,input,sizeof(input));
    
    sprintf(room_name,"\n%d. %s",room_num, input); // 파일에 들어갈 방 제목 정보 
    write(fd,room_name,strlen(room_name)); //  파일에 방 제목 삽입 
    make_room(s_n,r_n,input);
    sprintf(route,"/home/ty/project/interface/rooms/%d.txt",tmp_rn);
    wd = open(route,O_RDONLY);
    read(wd,room_info,sizeof(room_info));
    write(s_n,room_info,strlen(room_info));
    user[s_n].page = 3;

    bzero(input,sizeof(input));
    bzero(room_name,sizeof(room_name));
    bzero(file_name,sizeof(file_name));
    bzero(rout,sizeof(route));
    bzero(info,sizeof(info));
    bzero(room_info,sizeof(room_info));
}

// 유저가 생성된 방에 입장하는 함수 
void page2_n(int s_n,int n){
    // TODO 방에 사용자가 꽉찬 경우 방에 들어가지 못하는 기능 추가
    rooms[n]++;
    char route[1000];
    int fd;

    sprintf(route,"/home/ty/project/interface/rooms/%d.txt",n);
    fd = open(route,O_RDWR | O_APPEND);
    

    





