#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <asm/param.h>
#include <asm/uaccess.h>


typedef struct _my_lock{
      spinlock_t  guard;
      netlock_t type;
      unsigned int sleep_waiter;
      unsigned int sleeper;
      unsigned int use_waiter;
      unsigned int user;
      struct semaphore* use_waiter_sem;
      struct semaphore* sleep_waiter_sem;
      struct semaphore* wait_timeout_sem;
} my_lock_t;

struct semaphore_waiter {
  struct list_head list;
  struct task_struct *task;
  int up;
};


// static void init_lock(my_lock_t *lock){
//     .lock->sleep_waiter=0;
//     .lock->sleeper=0;
//     .lock->use_waiter=0;
//     .lock->user=0;
//     spin_lock_init(&lock->guard);
//     init_MUTEX(lock->use_waiter_sem);
//     init_MUTEX(lock->sleep_waiter_sem);
//     init_MUTEX(lock->wait_timeout_sem);
// }



my_lock_t mylock={
    .sleep_waiter=0,
    .sleeper=0,
    .use_waiter=0,
    .user=0,
    .guard = __SPIN_LOCK_UNLOCKED(.guard),
    .use_waiter_sem=__SEMAPHORE_INITIALIZER(.use_waiter_sem,1),
    .sleep_waiter_sem=__SEMAPHORE_INITIALIZER(.sleep_waiter_sem,1),
    .wait_timeout_sem=__SEMAPHORE_INITIALIZER(.wait_timeout_sem,1),
    // init_MUTEX(.use_waiter_sem),
    // init_MUTEX(.sleep_waiter_sem),
    // init_MUTEX(.wait_timeout_sem),
}; 

// my_lock_t mylock={
//     0,
//     0,
//     0,
//     0,
//     .guard = __SPIN_LOCK_UNLOCKED(.guard),
//     .use_waiter_sem = __SEMAPHORE_INITIALIZER(.use_waiter_sem,1),
//     __SEMAPHORE_INITIALIZER(*.sleep_waiter_sem,1),
//     __SEMAPHORE_INITIALIZER(*.wait_timeout_sem,1),
//     // init_MUTEX(.use_waiter_sem),
//     // init_MUTEX(.sleep_waiter_sem),
//     // init_MUTEX(.wait_timeout_sem),
// }; 

//    init_lock(&mylock);
// init_timer(&mytimer);

static  int  __down__(struct semaphore *sem, long timeout)
{
    struct task_struct *task = current;
    struct semaphore_waiter waiter;
    long state=TASK_UNINTERRUPTIBLE;
    
    list_add_tail(&waiter.list, &sem->wait_list);
    waiter.task = task;
    waiter.up = 0;

    for (;;) {
      __set_task_state(task, state);
      spin_unlock_irq(&sem->lock);
      timeout = schedule_timeout(timeout);
      spin_lock_irq(&sem->lock);
      if (waiter.up)
        return 0;
    }
}


static  void  __up__(struct semaphore *sem)
{
  struct semaphore_waiter *waiter = list_first_entry(&sem->wait_list,
            struct semaphore_waiter, list);
  list_del(&waiter->list);
  waiter->up = 1;
  wake_up_process(waiter->task);
}

// static void process_timeout(){
//     spin_lock(mylock.guard);
//     if(mylock.wait_timeout_sem.count==0)
//           __up__(mylcok.wait_timeout_sem);
//     spin_unlock(mylock.guard)

// }

SYSCALL_DEFINE2(net_lock, netlock_t, type, u_int16_t, timeout_val)
{
   netlock_t* type_buffer;

   type_buffer = kmalloc(sizeof(netlock_t), GFP_KERNEL);
   if(!type_buffer){
        printk("Error in kmllocating storage space\n");
        return -1;
   }
  printk("HERE***********\n");
  printk("type: %d\n", *type_buffer);
   switch(*type_buffer){
       case NET_LOCK_USE:
            printk("IN NET_LOCK_USE***********\n");
            spin_lock(mylock.guard);
            if(mylock.sleeper){   //need to change to while? in case a sleeper steps in after releasing
                mylock.use_waiter++;
                spin_unlock(mylock.guard);

                // struct timer_list mytimer;

                // init_timer(&mytimer);
                // mytimer.function=process_timeout;
                // mytimer.data=0;
                // mytimer.expires=jiffies+timeout_buffer * HZ;
                // add_timer(&my_timer)

                __down__(mylock.use_waiter_sem, timeout_val) ;//semaphore count is not in use, purely as list
                
                up(mylock.wait_timeout_sem);

                spin_lock(mylock.guard);

            }
            mylock.type=NET_LOCK_USE;
            mylock.user++;

            spin_unlock(mylock.guard);

            break;
       case NET_LOCK_SLEEP:
            printk("IN NET_LOCK_SLEEP***********\n");
            spin_lock(mylock.guard);
            while(mylock.user){
                mylock.sleep_waiter++;
                spin_unlock(mylock.guard);
                down(mylock.sleep_waiter_sem);
                spin_lock(mylock.guard);
            }
            mylock.type=NET_LOCK_SLEEP;
            mylock.sleeper++;
            spin_unlock(mylock.guard);

            break;
   }

    return 0;


}

SYSCALL_DEFINE0(net_unlock)
{
    spin_lock(mylock.guard);
    switch(mylock.type){
        case NET_LOCK_USE:
            mylock.user--;
            if(!mylock.user){
                up(mylock.sleep_waiter_sem);
                mylock.sleep_waiter--;
            }
            break;
        case NET_LOCK_SLEEP:
            mylock.sleeper--;    
            while(mylock.use_waiter){
              __up__(mylock.use_waiter_sem);
              mylock.use_waiter--;
            }
            break;
    }
    spin_unlock(mylock.guard);

    return 0;

}

SYSCALL_DEFINE0(net_lock_wait_timeout)
{
    // spin_lock(mylock.guard);
    // if(mylock.wait_timeout_sem.count)
    //     mylock.wait_timeout_sem.count--;
    // spin_unlock(mylock.guard)
    // __down__(wait_timeout_sem);
    down(mylock.wait_timeout_sem);

    return 0;
}

