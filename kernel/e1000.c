#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "e1000_dev.h"
#include "net.h"

#define TX_RING_SIZE 16
//发送环数组，每次发送需要一个tx_desc对应一个mbuf
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
//mbuf里面是数据包，放在char buf[MBUF_SIZE]，每一层会在之前基础上加上数据包头，因此需要有一个head来指向当前数据包的头部，为了继续封装加上新得头部,使用head-offset来实现，效率很高
static struct mbuf *tx_mbufs[TX_RING_SIZE];

#define RX_RING_SIZE 16
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));
static struct mbuf *rx_mbufs[RX_RING_SIZE];

// remember where the e1000's registers live.
static volatile uint32 *regs;

struct spinlock e1000_lock;

// called by pci_init().
// xregs is the memory address at which the
// e1000's registers are mapped.
void
e1000_init(uint32 *xregs)
{
  int i;

  initlock(&e1000_lock, "e1000");

  regs = xregs;

  // Reset the device
  regs[E1000_IMS] = 0; // disable interrupts
  regs[E1000_CTL] |= E1000_CTL_RST;
  regs[E1000_IMS] = 0; // redisable interrupts
  __sync_synchronize();

  // [E1000 14.5] Transmit initialization
  memset(tx_ring, 0, sizeof(tx_ring));
  for (i = 0; i < TX_RING_SIZE; i++) {
    tx_ring[i].status = E1000_TXD_STAT_DD;
    tx_mbufs[i] = 0;
  }
  regs[E1000_TDBAL] = (uint64) tx_ring;
  if(sizeof(tx_ring) % 128 != 0)
    panic("e1000");
  regs[E1000_TDLEN] = sizeof(tx_ring);
  regs[E1000_TDH] = regs[E1000_TDT] = 0;
  
  // [E1000 14.4] Receive initialization
  memset(rx_ring, 0, sizeof(rx_ring));
  for (i = 0; i < RX_RING_SIZE; i++) {
    rx_mbufs[i] = mbufalloc(0);
    if (!rx_mbufs[i])
      panic("e1000");
    rx_ring[i].addr = (uint64) rx_mbufs[i]->head;
  }
  regs[E1000_RDBAL] = (uint64) rx_ring;
  if(sizeof(rx_ring) % 128 != 0)
    panic("e1000");
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_RING_SIZE - 1;
  regs[E1000_RDLEN] = sizeof(rx_ring);

  // filter by qemu's MAC address, 52:54:00:12:34:56
  regs[E1000_RA] = 0x12005452;
  regs[E1000_RA+1] = 0x5634 | (1<<31);
  // multicast table
  for (int i = 0; i < 4096/32; i++)
    regs[E1000_MTA + i] = 0;

  // transmitter control bits.
  regs[E1000_TCTL] = E1000_TCTL_EN |  // enable
    E1000_TCTL_PSP |                  // pad short packets
    (0x10 << E1000_TCTL_CT_SHIFT) |   // collision stuff
    (0x40 << E1000_TCTL_COLD_SHIFT);
  regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // inter-pkt gap

  // receiver control bits.
  regs[E1000_RCTL] = E1000_RCTL_EN | // enable receiver
    E1000_RCTL_BAM |                 // enable broadcast
    E1000_RCTL_SZ_2048 |             // 2048-byte rx buffers
    E1000_RCTL_SECRC;                // strip CRC
  
  // ask e1000 for receive interrupts.
  regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
  regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
  regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
}

int
e1000_transmit(struct mbuf *m)
{
  //
  // Your code here.
  //
  // the mbuf contains an ethernet frame; program it into
  // the TX descriptor ring so that the e1000 sends it. Stash
  // a pointer so that it can be freed after sending.
  //
  printf("hello world e1000_transmit\n");
  acquire(&e1000_lock);
  //队列尾保存在寄存器中
  int curTxRingTail = regs[E1000_TDT];
  //获取发送环当前得具体结构体
  struct tx_desc *txDescP = &tx_ring[curTxRingTail];

  //检查当前位置是否准备好了
  //if ((txDescP->status&E1000_TXD_STAT_DD)!=1)
  if (!(txDescP->status & E1000_TXD_STAT_DD))
  {
    release(&e1000_lock);
    return -1; 
  }

   // 释放 desc 指向的原内存
  if(tx_mbufs[curTxRingTail]){
    mbuffree(tx_mbufs[curTxRingTail]);
  };


  //把数据放入缓冲区
  tx_mbufs[curTxRingTail] = m;

  //更新发送环结构体状态，表示当前有一个完整数据包
  txDescP->addr = (uint64)m->head;
  txDescP->length = m->len;
  txDescP->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;

  //更新尾部位置
  regs[E1000_TDT] = (curTxRingTail+1) %TX_RING_SIZE;

  release(&e1000_lock);
  return 0;
}

static void
e1000_recv(void)
{
  //
  // Your code here.
  //
  // Check for packets that have arrived from the e1000
  // Create and deliver an mbuf for each packet (using net_rx()).
  //
  //struct mbuf *m;
  //net_rx(m);
  printf("hello world e1000_recv\n");

  while (1)
  {
    //当前已经处理过了，这里指向下一个
    int curRxRingHead = regs[E1000_RDT];
    //获取接收送环，需要处理得结构体
    int idx = (curRxRingHead+1)%RX_RING_SIZE;
    struct rx_desc *rxDescP = &rx_ring[idx];

    //检查环里是否还有数据包
    if (!(rxDescP->status & E1000_RXD_STAT_DD))
    {
      return;
    }

    //取出环中的数据包,交给上层处理
    struct mbuf* mbuffP = rx_mbufs[idx];
    mbuffP->len=rxDescP->length;
    net_rx(mbuffP);

    //更新环结构体状态
    rx_mbufs[idx]= mbufalloc(0);
    rxDescP->addr = (uint64) rx_mbufs[idx]->head;
    rxDescP->status=0;

    regs[E1000_RDT]=idx;

  }
}

void
e1000_intr(void)
{
  // tell the e1000 we've seen this interrupt;
  // without this the e1000 won't raise any
  // further interrupts.
  regs[E1000_ICR] = 0xffffffff;
 printf("hello world e1000_intr\n");
  e1000_recv();
}
