#include "smallsh.h"

static char inpbuf[MAXBUF], tokbuf[2*MAXBUF], *ptr = inpbuf, *tok = tokbuf;
static char special [] = {' ', '\t', '&', ';', '\n', '\0'};

static struct sigaction act;        //시그널 처리를 위한 구조체변수 선언

int userin(char *p);
int inarg(char c);
int gettok(char **outptr);
int procline(void);
int runcommand(char **cline, int where);

void pipeline(int n,char**command);


/*프롬프트를 프린트하고 한 줄을 읽는다*/
int userin(char *p)
{
    int c, count;
    ptr = inpbuf;
    tok = tokbuf;
    printf("%s", p);	//프롬프트를 표시한다
    count = 0;
    while(1)
    {
        if((c = getchar()) == EOF)
            return (EOF);
        if(count <MAXBUF)
            inpbuf[count++] = c;
        if(c=='\n' &&count <MAXBUF)
        {
            inpbuf[count] = '\0';
            return count;
        }
        if(c == '\n')
        {
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s", p);
        }
    }
}
int inarg(char c)
{
    char *wrk;    
    for(wrk = special; *wrk; wrk++)
    {
        if(c == *wrk)
            return (0);
    }
    return (1);   
}

/*토큰을 가져와서 tokvuf에 넣는다*/
int gettok(char **outptr)
{
    int type;    
    *outptr = tok; 	//outptr문자열을 tok로 지어한다
    /*토큰을 포함하고 있는 버퍼로부터 여백을 제거*/
    while(*ptr == ' ' || *ptr == '\t')	
        ptr++;
        
      /*토큰 포인터를 버퍼내의 첫 토큰으로 지정한다*/
    *tok++ = *ptr;
    /*버퍼내의 토큰에 따라 유형 변수를 지정한다*/
    switch(*ptr++)
    {
        case '\n':
                type = EOL;
                break;
        case '&':
                type = AMPERSAND;
                break;
        case ';':
                type = SEMICOLON;
                break;
        default:
                type = ARG;
                /*유효한 보통 문자들을 계속 읽는다*/
                while(inarg(*ptr))
                    *tok++ = *ptr++;
    }    
    *tok++ = '\0';
    return type;
}

/*입력줄을 처리한다*/
int procline(void)
{
    char *arg[MAXARG + 1];	//runcommand를 위한포인터배열
    int toktype;			//명령내의 토큰의 유형
    int narg;			//지금까지의 인수 수
    int type;			//FOREGROUND 또는 BACKRFOUND
    narg = 0;

	
    for(;;) //영원히 루프를 돈다
    {        
    	   //토큰 유형에 따라 행동을 취한다
        switch(toktype = gettok(&arg[narg]))
        {
            case ARG:
                    if(narg <MAXARG)
                        narg++;
                    break;
            case EOL:    
            case SEMICOLON:
            case AMPERSAND:
                    if(toktype == AMPERSAND)
                        type = BACKGROUND;
                    else
                        type = FOREGROUND;
                    if(narg!=0){
                        arg[narg] = NULL;
                        runcommand(arg, type);
                    }error:

                    if(toktype == EOL)
                        return;
                    narg = 0;
                    break;
        }
    }
}

/*wait를 선택사항으로 하여 명령을 수행한다*/
int runcommand(char **cline, int where)
{
    pid_t pid;
    int status;
    int i;
    if(!strcmp(*cline,"logout"))            //logout명령어가 들어왔을 경우
        exit(1);
    
    
    for(i = 0;  ; i ++){                // | 명령어가 들어왔을경우(두개까지 가능)
        if(cline[i] == NULL)
            break;
    sigaction(SIGQUIT,&act,NULL);
        if(!strcmp(cline[i],"|")){
            pipeline(i,cline);
            return 0;
        }
    }
    
    if(where != BACKGROUND)                //백그라운드 프로세스가 아니라면
        act.sa_handler = SIG_DFL;        //핸들러를 디폴트로 바꾸고
    else
        act.sa_handler = SIG_IGN;
        
    sigaction(SIGINT,&act,NULL);            //SIGINT 시그널일때 행동 지정
    sigaction(SIGQUIT,&act,NULL);        //SIGQUIT 시그널일때 행동 지정

    switch(pid = fork())    
    {
        case -1:
                perror("smallsh");
                return (-1);
        case 0:
                execvp(*cline, cline);
                perror(*cline);
                exit(1);
    }
    
    //원래 쉘프로세스의 시그널 처리로 복원
    act.sa_handler = SIG_IGN;            //시그널을 무시하도록        
    sigaction(SIGINT,&act,NULL);            //SIGINT 시그널일때 행동 지정
    sigaction(SIGQUIT,&act,NULL);        //SIGQUIT 시그널일때 행동 지정
    

	//만일 백그라운드 프로세스이면 프로세스 식별자를 프린트하고 퇴장        
    if(where == BACKGROUND)
    {
        printf("[Process id %d]\n", pid);
        return (0);
    }

    // 프로세스 pid가 퇴장할 때까지 기다린다 
    if(waitpid(pid, &status, 0) == -1) // 자식이 끝나면 프롬프트 상태
        return (-1);
    else
        return (status);
}

 

main(int argc,char **argv,char **envp)
{
    act.sa_handler = SIG_IGN;        //아무일도 안하는 함수
    sigfillset(&(act.sa_mask));        //기본ㅤ쉘에서
    sigaction(SIGINT,&act,NULL);        //SIGINT시그널을 무시하도록 설정
    sigaction(SIGQUIT,&act,NULL);    //SIGQUIT시그널을 무시하도록 설정
    
    char prompt[MAXBUF];                
    char nowdir[MAXBUF];
    char * user = getenv("USER");
    getcwd(nowdir,MAXBUF);
  
    strcpy(prompt,"[");                //prompt설정
    strcat(prompt,user);
    strcat(prompt,"@");
    strcat(prompt,nowdir);
    strcat(prompt,"]#");
    
    while(userin(prompt) != EOF){
        procline();
        getcwd(nowdir,MAXBUF);            //prompt재설정
        strcpy(prompt,"[");
        strcat(prompt,user);
        strcat(prompt,"@");
        strcat(prompt,nowdir);
        strcat(prompt,"]#");
    }
}




/* 파이프 구현 함수 */
void pipeline(int n,char**command){
    int p1[2];
    int p2[2];
    int status;
    int i;
    int second = 0;
    command[n] = NULL;                //명령어를 분리 시킨다.
    
    for(i = n + 1; ; i ++){            //만약 파이프가 두 개라면
        if(command[i] == NULL)
            break;
        if(!strcmp(command[i],"|")){
            second = i;            //두번재 파이프 인덱스 저장
            break;
        }
    }
    
    if(second != 0)                //파이프가 두개 있다면
        command[second] = NULL;        //다시 명령어를 분리 시킨다.

    
    
    switch(fork()){        //자식프로세스 생성
    case -1:
        perror("1st fork call in join");
    case 0:
        break;
    default:
        wait(&status);    //부모는 자식을 기다림
        return;
    }
    
    //자식일 경우
    
    if(pipe(p1) == -1)            //파이프 생성
        perror("pipe call in join");
    if(pipe(p2) == -1)            //파이프 생성
        perror("pipe call in join");
    

    switch(fork()){            //또다른 자식을 생성 시킴
    case -1:
        perror("2nd fork call in join");
    case 0:
        dup2(p1[1],1);            //출력을 첫번째 파이프로 변경시킨다.
        
        close(p1[0]);
        close(p1[1]);
        
        execvp(*command,command);    //프로세스 실행
        perror("1st execvp call in join");
        
    default:
        if(second == 0){            //파이프가 하나밖에 없다면
            dup2(p1[0],0);        //입력을 첫번째 파이프로 변경시킨다.
        
            close(p1[0]);
            close(p1[1]);

            execvp(command[n+1],&command[n+1]);        //프로세스 실행
            perror("2nd execvp call in join");
        }
                
        else{                    //파이프가 하나더 있다면
            switch(fork()){        //또다른 프로세스를 실행시키기 위한 자식프로세스 생성
            case -1:
                perror("3nd fork call in join");
            case 0:
                dup2(p1[0],0);    //입력은 첫번째 파이프로 변경
                dup2(p2[1],1);    //출력은 두번째 파이프로 변경
                
                close(p2[0]);
                close(p2[1]);
                close(p1[0]);
                close(p1[1]);
                
                execvp(command[n+1],&command[n+1]);    //프로세스 실행            
                perror("4nd execvp call in join");                
            default:
                dup2(p2[0],0);    //입력은 두번째 파이프로 변경
                
                close(p1[0]);
                close(p1[1]);
                close(p2[0]);
                close(p2[1]);

                execvp(command[second+1],&command[second+1]);    //프로세스 실행
                perror("5nd execvp call in join");
            }        
        }
    }
}

 

 

 



