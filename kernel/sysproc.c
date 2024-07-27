#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  backtrace();
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  struct proc* p = myproc();
  int ticks;
   if(argint(0, &ticks) < 0)
    return -1;
  uint64 handlerP;
  if(argaddr(1, &handlerP) < 0)
    return -1;

  p->period = ticks;

  //把地址转化为函数指针
  p->handlerAddr = handlerP;
  p->trickCount = 0;

  return 0;
}
uint64
sys_sigreturn(void)
{
  struct proc* p = myproc();

  p->trapframe->kernel_satp=p->prevTrapframe->kernel_satp;
  p->trapframe->kernel_sp=p->prevTrapframe->kernel_sp;
  p->trapframe->kernel_trap=p->prevTrapframe->kernel_trap;
  p->trapframe->epc=p->prevTrapframe->epc;
  p->trapframe->kernel_hartid=p->prevTrapframe->kernel_hartid;
  p->trapframe->ra=p->prevTrapframe->ra;
  p->trapframe->sp=p->prevTrapframe->sp;
  p->trapframe->gp=p->prevTrapframe->gp;
  p->trapframe->tp=p->prevTrapframe->tp;
  p->trapframe->t0=p->prevTrapframe->t0;
  p->trapframe->t1=p->prevTrapframe->t1;
  p->trapframe->t2=p->prevTrapframe->t2;
  p->trapframe->s0=p->prevTrapframe->s0;
  p->trapframe->s1=p->prevTrapframe->s1;
  p->trapframe->a0=p->prevTrapframe->a0;
  p->trapframe->a1=p->prevTrapframe->a1;
  p->trapframe->a2=p->prevTrapframe->a2;
  p->trapframe->a3=p->prevTrapframe->a3;
  p->trapframe->a4=p->prevTrapframe->a4;
  p->trapframe->a5=p->prevTrapframe->a5;
  p->trapframe->a6=p->prevTrapframe->a6;
  p->trapframe->s2=p->prevTrapframe->s2;
  p->trapframe->s3=p->prevTrapframe->s3;
  p->trapframe->s5=p->prevTrapframe->s5;
  p->trapframe->s6=p->prevTrapframe->s6;
  p->trapframe->s7=p->prevTrapframe->s7;
  p->trapframe->s8=p->prevTrapframe->s8;
  p->trapframe->s9=p->prevTrapframe->s9;
  p->trapframe->s10=p->prevTrapframe->s10;
  p->trapframe->s11=p->prevTrapframe->s11;
  p->trapframe->t3=p->prevTrapframe->t3;
  p->trapframe->t4=p->prevTrapframe->t4;
  p->trapframe->t5=p->prevTrapframe->t5;
  p->trapframe->t6=p->prevTrapframe->t6;
  //*p->trapframe = *p->prevTrapframe; 
  p->rerntrantCount=0;
  return 0;
}



