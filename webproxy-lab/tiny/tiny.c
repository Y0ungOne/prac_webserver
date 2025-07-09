/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h" // CS:APP 제공 라이브러리 (I/O, 네트워크, 에러처리 포함)

void doit(int fd);                                         // 클라이언트 요청 처리 함수
void read_requesthdrs(rio_t *rp);                          // 요청 헤더 읽는 함수
int parse_uri(char *uri, char *filename, char *cgiargs);   // URI를 분석해서 정적/동적 결정
void serve_static(int fd, char *filename, int filesize);   // 정적 콘텐츠 응답 함수
void get_filetype(char *filename, char *filetype);         // MIME 타입 결정 함수
void serve_dynamic(int fd, char *filename, char *cgiargs); // 동적 콘텐츠 응답 함수 (CGI 실행)
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg); // 클라이언트 에러 응답 생성 함수
// 11.11
int is_head = 0;
void doit(int fd)
{
  int is_static;                            // 정적 콘텐츠인지 동적 콘텐츠인지 여부 플래그
  struct stat sbuf;                         // 파일의 상태(크기, 타입, 접근 권한 등)를 저장할 구조체
  char buf[MAXLINE], method[MAXLINE];       // 요청 라인 저장용: buf는 전체 줄, method는 HTTP 메서드 (예: GET)
  char uri[MAXLINE], version[MAXLINE];      // 요청한 URI 경로와 HTTP 버전
  char filename[MAXLINE], cgiargs[MAXLINE]; // 요청된 파일 경로와 CGI 인자
  rio_t rio;                                // 버퍼링된 입력을 위한 구조체

  /* 1. 요청 라인과 헤더 읽기 */
  Rio_readinitb(&rio, fd);           // rio 구조체를 fd(클라이언트 소켓)에 대해 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // 클라이언트로부터 요청 라인 한 줄 읽기
  printf("Request headers:\n");
  printf("%s", buf);                             // 요청 라인 출력
  sscanf(buf, "%s %s %s", method, uri, version); // 요청 라인을 파싱해서 method, uri, version에 저장

  /* 2. GET 메서드만 처리하고, 나머지는 에러 응답
  if (strcasecmp(method, "GET"))
  { // 대소문자 구분 없이 GET과 비교
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method"); // 501 에러 응답 전송
    return;
  }*/
  // 11.11 HEAD메소드도 지원하도록 함
  if (strcasecmp(method, "GET") == 0)
  {
    is_head = 0; // 본문 전송-> 이전과 같은 get처리
  }

  else if (strcasecmp(method, "HEAD"))
  {
    is_head = 1;
  }
  else
  {
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method"); // 501 에러 응답 전송
    return;
  }

  read_requesthdrs(&rio); // 나머지 요청 헤더들을 읽고 무시

  /* 3. URI 파싱해서 정적/동적 콘텐츠 구분하고, 파일 이름 추출 */
  is_static = parse_uri(uri, filename, cgiargs); // uri를 분석해서 정적이면 is_static=1, 아니면 0

  /* 4. 파일 존재 여부 확인 */
  if (stat(filename, &sbuf) < 0)
  { // 해당 파일의 상태 정보를 가져옴 (없으면 -1)
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file"); // 404 에러 전송
    return;
  }

  if (is_static)
  { /* 5. 정적 콘텐츠 처리 */
    // 일반 파일이고, 읽기 권한이 있는지 확인
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file"); // 권한 없음 에러
      return;
    }

    // 정적 파일 클라이언트에게 전송(바이트 단위의 파일 사이즈 전송)
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  { /* 6. 동적 콘텐츠 (CGI) 처리 */
    // 일반 파일이고, 실행 권한이 있는지 확인
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program"); // 실행 권한 없음 에러
      return;
    }

    // CGI 프로그램 실행 (표준 출력 → 클라이언트에 전송됨)
    serve_dynamic(fd, filename, cgiargs);
  }
}
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* 1. 에러 응답 HTML 본문(body) 생성 */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);  // 상태 코드와 요약 메시지
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause); // 상세 메시지와 원인
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* 2. 전체 HTTP 응답 헤더 + 본문을 클라이언트에게 전송 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body)); // body 출력
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  {                        // ★ 정적 콘텐츠
    strcpy(cgiargs, "");   // CGI 인자 없음
    strcpy(filename, "."); // 현재 디렉토리에서 시작
    strcat(filename, uri); // ex: ./index.html

    if (uri[strlen(uri) - 1] == '/') // 끝에 /가 있으면 home.html 추가
      strcat(filename, "home.html");
    return 1;
  }
  else
  {                        // ★ 동적 콘텐츠 (cgi-bin 포함)
    ptr = index(uri, '?'); // 물음표(?) 이후는 인자
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0'; // '?' 제거
    }
    else
    {
      strcpy(cgiargs, ""); // 인자 없음
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  // 11.9
  char *srcbuf;
  rio_t rio; // 여기선 readn으로 한번에 파일 읽어서 필요 없지만 같은 줄 단위 읽기 함수를 쓸 때 필요함

  /* 1. 응답 헤더 구성 */
  get_filetype(filename, filetype); // 확장자로부터 MIME 타입 결정
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);

  Rio_writen(fd, buf, strlen(buf)); // 응답 헤더 전송
  printf("Response headers:\n%s", buf);

  /* 2. 파일 본문 전송 (메모리 매핑 방식) */
  srcfd = Open(filename, O_RDONLY, 0);
  /* 문제 풀이 위해 생략
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  */
  // 11.9
  srcbuf = (char *)malloc(filesize); // 파일 사이즈만큼 버퍼 크기 할당
  Rio_readinitb(&rio, srcfd);        // fd에 대해 rio초기화

  Rio_readn(srcfd, srcbuf, filesize); // 사실 이것만 있어도 됨, 파일 내용을 srcbuf로 읽어오기
  // 여기까지 수정본

  Close(srcfd);
  /*
  11.11 때문에 주석
  Rio_writen(fd, srcp, filesize); // 본문 전송*/
  if (!is_head)
  {
    Rio_writen(fd, srcp, filesize);
  }
  /*Munmap(srcp, filesize);*/
  // 11.9 malloc해제
  free(srcbuf);
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL}; // execve 인자용 빈 배열

  /* 1. HTTP 응답 헤더 전송 (본문 없음) */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* 2. fork 후 자식 프로세스에서 CGI 실행 */
  if (Fork() == 0)
  {                                       // 자식 프로세스
    setenv("QUERY_STRING", cgiargs, 1);   // 환경 변수 설정 (인자 전달용)
    Dup2(fd, STDOUT_FILENO);              // 표준 출력 → 클라이언트 소켓으로 연결
    Execve(filename, emptylist, environ); // CGI 프로그램 실행
    // 11.8 문제
    exit(0); // 실패 시 대비
  }

  /* 3. 부모는 자식이 끝날 때까지 기다림
  Wait(NULL);*/
}

// 11.8
void sigchld_handler(int sig)
{
  while ((waitpid(-1, NULL, WNOHANG) > 0))
    ;
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // 첫 줄 읽기
  while (strcmp(buf, "\r\n"))
  {                                  // 빈 줄(헤더 끝) 전까지 반복
    Rio_readlineb(rp, buf, MAXLINE); // 다음 줄 읽고
    printf("%s", buf);               // 출력 (서버 디버그용)
  }
  return;
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain"); // 기본값
}

int main(int argc, char **argv)
{
  int listenfd, connfd;                  // 리슨 소켓, 연결 소켓
  char hostname[MAXLINE], port[MAXLINE]; // 클라이언트 호스트명, 포트 저장
  socklen_t clientlen;
  struct sockaddr_storage clientaddr; // 클라이언트 주소 정보
  Signal(SIGCHLD, sigchld_handler);
  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // 사용법 출력
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 리슨 소켓 생성 (포트 바인딩 포함)
  while (1)                          // 반복적으로 클라이언트 요청 처리
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 접속 정보 출력

    doit(connfd);  // 클라이언트 요청 처리
    Close(connfd); // 처리 후 연결 소켓 닫기
  }
}
/* $end tinymain */
