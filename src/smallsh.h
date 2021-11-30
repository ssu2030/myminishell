#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

#define EOL 1   // 줄의 끝
#define ARG 2   // 정상적 인수

#define AMPERSAND 3
#define SEMICOLON 4

#define MAXARG 512  // 명령인수의 최대수
#define MAXBUF 512  // 입력줄의 최대길이

#define FOREGROUND 0
#define BACKGROUND 1
