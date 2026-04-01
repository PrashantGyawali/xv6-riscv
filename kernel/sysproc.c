#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "fcntl.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "param.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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

  argint(0, &pid);
  return kkill(pid);
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
sys_getcwd(void)
{
  uint64 uaddr;
  int    size;
  argaddr(0, &uaddr);
  argint(1, &size);

  char path[MAXPATH];
  char tmp[MAXPATH];
  int  plen = 0;

  struct proc  *p  = myproc();
  struct inode *ip = p->cwd;
  ilock(ip);
  uint inum_cur = ip->inum;
  uint dev_cur  = ip->dev;
  iunlock(ip);

  path[0] = '\0';

  // Walk up from cwd to root via ".." entries
  while (1) {
    // Are we at root?
    if (inum_cur == ROOTINO) {
      break;
    }

    // Open parent ".."
    struct inode *dp = iget(dev_cur, inum_cur);
    ilock(dp);

    // Read the ".." entry to get parent inode number
    struct dirent de;
    uint parent_inum = 0;
    for (uint off = 0; off < (uint)dp->size; off += sizeof(de)) {
      if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        break;
      if (strncmp(de.name, "..", DIRSIZ) == 0) {
        parent_inum = de.inum;
        break;
      }
    }
    iunlockput(dp);

    if (parent_inum == 0)
      break;

    // Open parent dir and find the name of inum_cur inside it
    struct inode *pp = iget(dev_cur, parent_inum);
    ilock(pp);

    char found[DIRSIZ + 1];
    found[0] = '\0';
    for (uint off = 0; off < (uint)pp->size; off += sizeof(de)) {
      if (readi(pp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        break;
      if (de.inum == inum_cur &&
          strncmp(de.name, ".",  DIRSIZ) != 0 &&
          strncmp(de.name, "..", DIRSIZ) != 0) {
        strncpy(found, de.name, DIRSIZ);
        found[DIRSIZ] = '\0';
        break;
      }
    }
    iunlockput(pp);

    if (found[0] == '\0')
      break;

    // Prepend "/name" to path
    int flen = strlen(found);
    int need = flen + 1; // "/" + name
    if (plen + need >= MAXPATH)
      return -1;

    // Shift existing path right and insert "/name" at front
    memmove(tmp, path, plen + 1);
    path[0] = '/';
    memmove(path + 1, found, flen);
    memmove(path + 1 + flen, tmp, plen + 1);
    plen += need;

    inum_cur = parent_inum;
  }

  // If path is empty we are at root
  if (plen == 0) {
    path[0] = '/';
    path[1] = '\0';
    plen = 1;
  }

  if (plen + 1 > size)
    return -1;

  if (copyout(p->pagetable, uaddr, path, plen + 1) < 0)
    return -1;

  return uaddr;
}
