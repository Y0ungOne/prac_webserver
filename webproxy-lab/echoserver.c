#include "csapp.h"

void echo(int connfd); // 클라이언트 요청을 처리하는 함수 미리 선언

int main(int argc, char **argv)
{
    int listenfd, connfd;                           // listenfd: 수신 대기 소켓, connfd: 클라이언트 연결 소켓
    socklen_t clientlen;                            // 클라이언트 주소 크기
    struct sockaddr_storage clientaddr;             // 클라이언트 주소 정보 저장용
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 호스트명, 포트번호 저장용

    if (argc != 2) {                                // 포트 번호 안 넣었을 때 사용법 출력
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);              // 서버 수신 대기 소켓 생성 및 바인딩
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);    // 클라이언트 주소 구조체 크기 설정
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트 연결 수락

        Getnameinfo((SA *) &clientaddr, clientlen,    // 클라이언트 주소를 문자열로 변환
                    client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port); // 연결된 클라이언트 정보 출력

        echo(connfd);                                 // 요청 처리 (echo 함수 호출)
        Close(connfd);                                // 처리 후 소켓 닫기
    }

    exit(0);
}
