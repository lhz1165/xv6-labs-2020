// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define NBUFMAP_BUCKET 13
// hash function for bufmap
#define BUFMAP_HASH(dev, blockno) ((((dev)<<27)|(blockno))%NBUFMAP_BUCKET)
// struct {
//   struct spinlock lock;
//   struct buf buf[NBUF];

//   //桶的锁
//   struct spinlock bhash_lk[BUCK_SIZ];
//   //桶的头
//   struct buf bhash_head[BUCK_SIZ];


//   // Linked list of all buffers, through prev/next.
//   // Sorted by how recently the buffer was used.
//   // head.next is most recent, head.prev is least.
//   struct buf head;
// } bcache;


struct {
  struct buf buf[NBUF];
  struct spinlock eviction_lock;

  // Hash map: dev and blockno to buf
  struct buf bufmap[NBUFMAP_BUCKET];
  struct spinlock bufmap_locks[NBUFMAP_BUCKET];
} bcache;


void
binit(void)
{
  // Initialize bufmap
  for(int i=0;i<NBUFMAP_BUCKET;i++) {
    initlock(&bcache.bufmap_locks[i], "bcache_bufmap");
    bcache.bufmap[i].next = 0;
  }

  // Initialize buffers
  for(int i=0;i<NBUFMAP_BUCKET;i++){
    struct buf *b = &bcache.buf[i];
    initsleeplock(&b->lock, "buffer");
    b->lastuse = 0;
    b->refcnt = 0;
    // put all the buffers into bufmap[0]
    b->next = bcache.bufmap[0].next;
    bcache.bufmap[0].next = b;
  }
}



// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint key = BUFMAP_HASH(dev, blockno);

  acquire(&bcache.bufmap_locks[key]);
  for(b = bcache.bufmap[key].next; b; b = b->next){
    // 查看 blockno 是否在对应的桶里被缓存
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bufmap_locks[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bufmap_locks[key]);
  
  int lru_bkt=-1;
  struct buf* pre_lru = 0;

  for(int i = 0; i < NBUFMAP_BUCKET; i++){
    acquire(&bcache.bufmap_locks[i]);
    int found_new = 0;
    for(b = &bcache.bufmap[i]; b->next; b = b->next){ 
      if(b->next->refcnt == 0 && (!pre_lru || b->next->lastuse < pre_lru->next->lastuse)){
        pre_lru = b;
        found_new = 1;
      }
    }
    if(!found_new){
      // 没有更好的选择，就一直持有这个锁（需要确保一直持有最佳选择对应桶的锁）
      release(&bcache.bufmap_locks[i]);
    }else{ // 有更好的选择（有更久没使用的）
      if(lru_bkt != -1) release(&bcache.bufmap_locks[lru_bkt]); // 直接释放以前选择的锁
      lru_bkt = i; // 更新最佳选择
    }
  }

  // pre_lru 会返回空闲缓存前一个（链表中前一个）缓存的地址
  // 并且确保拿到了缓存对应的桶锁
  // 我们会传进去一个 lru_bkt，函数执行好后，这个值会储存缓存对应的桶
  if(pre_lru == 0){
    panic("bget: no buffers");
  }
  
  struct buf* lru = pre_lru->next; 
  // lru （lru 是最久没有使用的缓存，并且 refcnt = 0）是 pre_lru 后面的一个
  pre_lru->next = lru->next; 
  // 让 pre_lru 的后面一个直接变成 lru 的后面一个，相当于删除 lru
  release(&bcache.bufmap_locks[lru_bkt]);

  acquire(&bcache.bufmap_locks[key]);  

  for(b = bcache.bufmap[key].next; b; b = b->next){
    // 拿到锁之后要确保没有重复添加缓存
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bufmap_locks[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  lru->next = bcache.bufmap[key].next; // 把找到的缓存添加到链表头部
  bcache.bufmap[key].next = lru;

  lru->dev = dev, lru->blockno = blockno;
  lru->valid = 0, lru->refcnt = 1; 

  release(&bcache.bufmap_locks[key]);

  acquiresleep(&lru->lock);
  return lru;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint key = BUFMAP_HASH(b->dev, b->blockno);

  acquire(&bcache.bufmap_locks[key]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->lastuse = ticks;
  }
  release(&bcache.bufmap_locks[key]);
}

void
bpin(struct buf *b) {
  uint key = BUFMAP_HASH(b->dev, b->blockno);

  acquire(&bcache.bufmap_locks[key]);
  b->refcnt++;
  release(&bcache.bufmap_locks[key]);
}

void
bunpin(struct buf *b) {
  uint key = BUFMAP_HASH(b->dev, b->blockno);

  acquire(&bcache.bufmap_locks[key]);
  b->refcnt--;
  release(&bcache.bufmap_locks[key]);
}

