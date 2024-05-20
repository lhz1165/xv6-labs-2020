#include "user/user.h"

int main(int argc, char const *argv[])
{

    int cp[2];
    int fp[2];
    pipe(cp);
    pipe(fp);
    int pid = fork();
    if (pid==0)
    {
        close(fp[1]);

        char ping[20];
        read(fp[0],ping,20);
        printf("%d: %s\n",getpid(),ping);

        close(cp[0]);
        char pong[20]="pong reveived";
        write(cp[1],pong,20);
        int status;
        wait(&status);
    }else{
      close(fp[0]);
      char ping[20]="ping reveived";
      write(fp[1],ping,20);


      close(cp[1]);
      char pong[20];
      read(cp[0],pong,20);
      printf("%d: %s\n", getpid(),pong);

    }
    

    
    return 0;
}
