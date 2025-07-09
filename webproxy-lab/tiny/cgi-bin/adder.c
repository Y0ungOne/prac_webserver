/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"  // CS:APP에서 제공하는 편의 함수와 상수 정의 포함

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    /* Extract the two arguments */
    // 클라이언트 요청 URI에서 QUERY_STRING 환경 변수 가져오기 (예: 3&5)
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');     // '&' 기호 위치 찾기
        *p = '\0';                // '&'를 널문자로 바꿔 앞부분 종료 → buf는 arg1만 남음
        strcpy(arg1, buf);        // 첫 번째 숫자 추출
        strcpy(arg2, p + 1);      // 두 번째 숫자 추출 (p+1부터 끝까지)
        n1 = atoi(arg1);          // 문자열 → 정수 변환
        n2 = atoi(arg2);
    }

    /* Make the response body */
    // 아래 sprintf들은 모두 content에 출력 문자열을 만든다.
    // ⚠️ 하지만 매번 sprintf(content, ...) 형식이라 이전 내용이 덮어써짐 → 나중에 출력되는 것만 남음
    sprintf(content, "QUERY_STRING=%s", buf);  // 실제로는 곧 덮여서 사라짐
    sprintf(content, "Welcome to add.com: ");  // 또 덮어쓰기됨
    sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);  // 위 줄에 이어붙이기
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);  // 이어붙이기
    sprintf(content, "%sThanks for visiting!\r\n", content);  // 이어붙이기

    /* Generate the HTTP response */
    // 클라이언트에 HTTP 응답 헤더 출력
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));  // 전체 응답 본문 길이
    printf("Content-type: text/html\r\n\r\n");               // 헤더 끝 표시

    // 응답 본문 출력 (HTML 콘텐츠)
    printf("%s", content);
    fflush(stdout);  // 출력 버퍼를 즉시 전송

    exit(0);  // 정상 종료
}
