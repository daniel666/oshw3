#include <stdio.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>

typedef struct _my_lock{
      spinlock_t  guard;
      netlock_t type;
      unsigned int sleep_waiter;
      unsigned int sleeper;
      unsigned int use_waiter;
      unsigned int user;
      struct semaphore use_waiter_sem;
      struct semaphore sleep_waiter_sem;
      struct semaphore wait_timeout_sem;
} my_lock_t;

struct semaphore_waiter {
  struct list_head list;
  struct task_struct *task;
  int up;
};


my_lock_t mylock={
    .sleep_waiter=0,
    .sleeper=0,
    .use_waiter=0,
    .user=0,
    .guard = __SPIN_LOCK_UNLOCKED(mylock.guard),
    .use_waiter_sem=__SEMAPHORE_INITIALIZER(mylock.use_waiter_sem,1),
    .sleep_waiter_sem=__SEMAPHORE_INITIALIZER(mylock.sleep_waiter_sem,1),
    .wait_timeout_sem=__SEMAPHORE_INITIALIZER(mylock.wait_timeout_sem,0),
}; 

int main(){
	INIT_LIST_HEAD(&sem->wait_list)
    struct semaphore_waiter *waiter = list_first_entry(&sem->wait_list,
            struct semaphore_waiter, list);
    if(!waiter)
    	printf("NULL\n");
    else
    	printf("VALID\n");
} 


