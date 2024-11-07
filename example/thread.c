#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// 쓰레드 함수
void *test(void *data)
{
    int tmp =0 ;
    for(int i=0;i<10;++i) tmp = tmp + *((int*)data);

    return (void *)(tmp);
}

int main()
{
    int a = 1;
    pthread_t thread_t;
    int status;

    // 쓰레드를 생성한다. 
    if (pthread_create(&thread_t, NULL, test, (void *)&a) < 0)
    {
        perror("thread create error:");
        exit(0);
    }

    // 쓰레드가 종료되기를 기다린후 
    // 쓰레드의 리턴값을 출력한다. 
    pthread_join(thread_t, (void **)&status);
    printf("Thread End %d\n", status);
    printf("Result of a : %d\n",a);
    return 1;
}
