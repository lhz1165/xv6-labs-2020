#include "user/user.h"
#define NUMS 34
void findPrimes(int *array, int len);
int main(int argc, char const *argv[])
{   
    int* array = (int*)malloc(NUMS * sizeof(int));
    for (int i = 0; i < NUMS + 2; i++)
    {
       array[i]=i+2;
    }
    findPrimes(array,NUMS);
    wait(0);
    free(array);
    exit(0);
}

void findPrimes(int *array, int len){
    if (len == 0){
        exit(0);
        //return;
    }
    if (len == 1 ){
        printf("prime %d\n",array[0]);
        exit(0);
    }
    int p[2];
    pipe(p);

    int pid = fork();

    if (pid == 0)
    {   
        close(p[1]);
        // 2.1读取数组长度
        int curLen = 0;
        read(p[0], &curLen, sizeof(int)); 
         //2.2读取管道array
        int* curArray = (int*)malloc(curLen * sizeof(int));
        if (curArray==0)
        {
             printf("error curArray malloc\n");
        }
        
        if (read(p[0],curArray,curLen * sizeof(int)) != curLen * sizeof(int)) {
            printf("error read array\n");
            exit(-1);
        }
        close(p[0]); // 关闭读端
        findPrimes(curArray,curLen);
        free(curArray);
        exit(0);
    }else{
        close(p[0]);
        int min = array[0];
        printf("prime %d\n",min);

        int nextLen = 0;
        int* newArray = (int*)malloc(len * sizeof(int));

        if (newArray == 0) {
            printf("error newArray malloc\n");
            exit(-1);
        }
        for (int i = 1; i < len; i++)
        {   
            if (array[i] % min != 0) {
                newArray[nextLen] = array[i];
                nextLen++;
            }
        }
        if (nextLen!=0)
        {
            //1.1管道发送array长度
            if (write(p[1], &nextLen, sizeof(int)) != sizeof(int)) {
                printf("error write length\n");
                exit(-1);
            }
            //1.2管道发送array
            if (write(p[1], newArray, nextLen * sizeof(int)) != nextLen * sizeof(int)) {
                printf("error write array\n");
                exit(-1);
            }
            
        }
        close(p[1]);
        
    
        //等待子进程
        wait(0);
        free(newArray);
    }    
}