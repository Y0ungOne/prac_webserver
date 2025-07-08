#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;                            // 서버에 연결할 클라이언트 소켓
    char *host, *port, buf[MAXLINE];         // 서버 주소, 포트, 데이터 버퍼
    rio_t rio;                               // 버퍼링된 입출력 구조체

    if (argc != 3) {                         // 인자가 부족하면 사용법 출력
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];                          // 첫 번째 인자 = 서버 주소
    port = argv[2];                          // 두 번째 인자 = 포트 번호

    clientfd = Open_clientfd(host, port);    // 서버에 연결 요청 (소켓 생성 + connect)
    Rio_readinitb(&rio, clientfd);           // 버퍼링된 읽기 구조체 초기화

    while (Fgets(buf, MAXLINE, stdin) != NULL) {   // 표준 입력으로부터 한 줄 입력
        Rio_writen(clientfd, buf, strlen(buf));    // 입력한 내용을 서버에 전송
        Rio_readlineb(&rio, buf, MAXLINE);         // 서버로부터 응답을 한 줄 읽음
        Fputs(buf, stdout);                        // 응답을 표준 출력으로 출력
    }

    Close(clientfd);                         // 소켓 닫기
    exit(0);
}

