#include "../kernel/types.h"
#include "../user/user.h"
#include "../kernel/param.h"

#define MAX_STDIN 512
/**
 * echo hello too | xargs echo bye
 * =>echo bye hello too
 * 
 * read_in  hello too\n 
 * len = 10
 * args = ["echo","bye"] 
 * args_cnt=2
 * //去掉空格和\n
 * args= >args = ["echo","bye","hello","too"]
 * 
*/
void fmt_args(char *read_in, int len, char **args, int *args_cnt)
{
    printf("read_in buf is %s len is %d,args[0] is %s, args_cnt is %d\n",read_in,len,args[0],*args_cnt);
    // 将一行标准输入转为多个参数
    char cur_buf[MAX_STDIN];
    int cur_buf_len = 0;

    int i;
    //一个一个字符读取
    for (i = 0; i <= len; i++)
    {   
        //读到\n或者空格 并且cur_buf_len=1
        if ((read_in[i] == ' ' || read_in[i] == '\n') && cur_buf_len)
        {
            // 读到了一个新参数尾巴
            args[*args_cnt] = malloc(cur_buf_len + 1);
            //参数拼接到 args[2]="hello"
            memcpy(args[*args_cnt], cur_buf, cur_buf_len);
            //args[2]="hello\0"
            //args[3]="too\0"
            args[*args_cnt][cur_buf_len] = 0;
            cur_buf_len = 0;
            (*args_cnt)++;
        }
        else
        {
            //分割参数cur_buf
            cur_buf[cur_buf_len] = read_in[i];
            cur_buf_len++;
        }
    }
}

int main(int argc, char **argv)
{
    char stdin_buf[MAX_STDIN];
    int result;

    char *args[MAXARG + 1];
    int args_cnt;

    // 读 xargs 后面的命令行参数
    for (int i = 1; i < argc; i++)
    {
        args[args_cnt] = malloc(sizeof(argv[i]));
        //argv[i]开始 往后 n个长度，拷贝到args[args_cnt++]
        memcpy(args[args_cnt++], argv[i], sizeof(argv[i]));
    }
    // 读从管道传来的标准输入
    if ((result = read(0, stdin_buf, sizeof(stdin_buf))) > 0)
    {

        fmt_args(stdin_buf, strlen(stdin_buf), args, &args_cnt);
    }

    exec(args[0], args);

    exit(0);
}