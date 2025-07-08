#include "csapp.h"

void echo(int connfd)
{
    size_t n;                   // 읽은 바이트 수
    char buf[MAXLINE];          // 데이터를 저장할 버퍼
    rio_t rio;                  // 버퍼링된 입출력 구조체

    Rio_readinitb(&rio, connfd);               // 소켓을 기반으로 버퍼링된 읽기 구조체 초기화
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { // 한 줄씩 입력을 읽음
        printf("server received %d bytes\n", (int)n);     // 몇 바이트 읽었는지 출력
        Rio_writen(connfd, buf, n);                       // 읽은 데이터를 클라이언트에게 다시 전송 (echo)
    }
}
