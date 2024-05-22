#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]){
    if (argc < 2) {
        printf(3, "Usage: %s -n <value> [other arguments]\n", argv[0]);
        return 1;
    }
    char buf[1024];
    char tmep[1];
    int count;
    int i = 0;
    int fd = 0;
    while ((count = read(fd,tmep,1)) > 0)
    {
        buf[i] = tmep[0];
        i++;
    }
    buf[i]='\0';


   
    //exec();

    
  
}
