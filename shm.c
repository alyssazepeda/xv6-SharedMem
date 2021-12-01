#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

  //acquire a spinlock to make sure we don't lose updates
  acquire(&(shm_table.lock));
  //get current process
  struct proc *curproc = myproc();
  //free virtual address we want to attach our page to
  uint va = PGROUNDUP(curproc->sz);
  //booleans arent initialized so this holds the T/F value if the id exists
  unsigned exists = 0; 
  unsigned i;
  //used to find an empty entry in the table 
  //assigned to 64 so we can check if it was assigned any spot in the table (0-63)
  unsigned int entry = 64; 
  //Case 1: The id we are opening already exists
  for(i = 0; i<64; i++)
  {
    if(shm_table.shm_pages[i].id == id) 
    {
      //the id exists in the table so we map the page
      exists = 1;
      mappages(curproc->pgdir, (char *)va, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
      shm_table.shm_pages[i].refcnt++; //increment the refcnt
      break; //GOTO : Updating & Returning
    }
    //As we loop through the table we look for empty entries for Case 2
    if(shm_table.shm_pages[i].id==0 && entry==64) 
    {
      entry = i; //we found the empty spot
    }
  }
  //Case 2: The id we are opening does not exist(e.g. exists still = 0)
  if(!exists && entry<64)
  {
    shm_table.shm_pages[entry].id = id; //initialize id to the one passed to us
    memset(kalloc(), 0, PGSIZE);
    shm_table.shm_pages[entry].frame = kalloc(); //store the addesss in frame
    shm_table.shm_pages[entry].refcnt++;
    mappages(curproc->pgdir, (char *)va, PGSIZE, V2P(shm_table.shm_pages[entry].frame), PTE_W|PTE_U);
  }

  //Updating & Returning//
  *pointer = (char *)va; //return pointer to the virtual address
  curproc->sz = va + PGSIZE; //update sz since we expanded the virtual address
  release(&(shm_table.lock)); //release the lock
  return 0; //remains 0 bc we updated the values outputted in the tests in the code
}


int shm_close(int id) {
  acquire(&(shm_table.lock)); //acquire the spinlock
  unsigned i;
  for(i=0; i<64; i++) 
  { //look for the shared memory segment
    if(shm_table.shm_pages[i].id == id)
    {//found it
      if(shm_table.shm_pages[i].refcnt > 0)
      {
        shm_table.shm_pages[i].refcnt--; //decrement the refcnt
      }
      else //refcnt = 0, so we clear the shm_table
      {
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
        break;
      }
    }
  }
  release(&(shm_table.lock)); //release the lock
  return 0; //remains 0 bc we updated the values outputted in the tests in the code
}
