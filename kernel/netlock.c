#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <asm/param.h>
#include <asm/uaccess.h>
#include <linux/spinlock_types.h>
#include <linux/preempt.h>

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

static void show_lock()
{
    my_lock_t* lock = &mylock;
    printk("#------My Lock--------\n");
    printk("#type: %d\n", lock->type);
    printk("#sleep_waiter: %d\n", lock->sleep_waiter);
    printk("#sleeper: %d\n", lock->sleeper);
    printk("#use_waiter: %d\n", lock->use_waiter);
    printk("#user: %d\n", lock->user);
    printk("#  >> semaphore below");

    printk("\n#  >> use_waiter_sem:");
    struct semaphore_waiter *waiter;
    list_for_each_entry(waiter, &lock->use_waiter_sem.wait_list, list)
        printk("pid:%d ", waiter->task->pid);

    printk("\n#  >> sleep_waiter_sem:");
    struct semaphore_waiter *waiter2;
    list_for_each_entry(waiter2, &lock->sleep_waiter_sem.wait_list, list)
    printk("pid:%d ", waiter2->task->pid);

    printk("\n#  >> wait_timeout_sem:");
    struct semaphore_waiter *waiter3;
    list_for_each_entry(waiter3, &lock->wait_timeout_sem.wait_list, list)
    printk("pid:%d ", waiter3->task->pid);
    printk("\n#------END my lock-------\n");

}

static  void  __up__(struct semaphore *sem)
{
  
  if(list_empty(&sem->wait_list)){
    printk("I am returning\n");
    return ;
  }
  struct semaphore_waiter *waiter = list_first_entry(&sem->wait_list,
            struct semaphore_waiter, list);
  waiter->up = 1;
  list_del(&waiter->list);
  wake_up_process(waiter->task);
}

static void my_process_timeout(unsigned long __data)
{  
    // wake_up_process((struct task_struct *)__data);

    spin_lock(&mylock.guard);
    if(!list_empty(&mylock.wait_timeout_sem.wait_list))
          __up__(&mylock.wait_timeout_sem);
    spin_unlock(&mylock.guard);
}


signed long  my_schedule_timeout(signed long timeout)
{
  struct timer_list timer;
  unsigned long expire;

  switch (timeout)
  {
  case MAX_SCHEDULE_TIMEOUT:
    /*
     * These two special cases are useful to be comfortable
     * in the caller. Nothing more. We could take
     * MAX_SCHEDULE_TIMEOUT from one of the negative value
     * but I' d like to return a valid offset (>=0) to allow
     * the caller to do everything it want with the retval.
     */
    schedule();
    goto out;
  default:
    /*
     * Another bit of PARANOID. Note that the retval will be
     * 0 since no piece of kernel is supposed to do a check
     * for a negative retval of schedule_timeout() (since it
     * should never happens anyway). You just have the printk()
     * that will tell you if something is gone wrong and where.
     */
    if (timeout < 0) {
      printk(KERN_ERR "schedule_timeout: wrong timeout "
        "value %lx\n", timeout);
      dump_stack();
      current->state = TASK_RUNNING;
      goto out;
    }
  }

  expire = timeout + jiffies;

  setup_timer_on_stack(&timer, my_process_timeout,  (unsigned long) current);
  __mod_timer(&timer, expire);
  schedule();
  del_singleshot_timer_sync(&timer);

  /* Remove the timer from the object tracker */
  destroy_timer_on_stack(&timer);

  timeout = expire - jiffies;

 out:
  return timeout < 0 ? 0 : timeout;
}

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

      printk("PID %d: Before schedule_timeout\n", current->pid);
      long ret_val;
      if (timeout ==MAX_SCHEDULE_TIMEOUT){
          schedule();
          printk("AFTER SCHEDUKE\n");
          printk("PID %d: After schedule_timeout: timeoutval: %ld\n", current->pid, timeout);
          return timeout < 0 ? 0 : timeout;
      }
      ret_val = my_schedule_timeout(timeout * HZ);
      printk("PID %d: After schedule_timeout: timeoutval: %ld, ret_val:%ld\n", current->pid, timeout, ret_val);

      if (waiter.up)
        return 0;
    }
}


SYSCALL_DEFINE2(net_lock, netlock_t, type, u_int16_t, timeout_val)
{

   printk(".......IN net_lock PID: %d Comm %s...........\n", current->pid, current->comm);
   show_lock();
   switch(type){
       case NET_LOCK_USE:
            printk("IN NET_LOCK_US\nPID %d: (type:%d, timeout:%hu)\n", current->pid, type, timeout_val);
            spin_lock(&mylock.guard);
            if(mylock.sleeper){   //need to change to while? in case a sleeper steps in after releasing
                mylock.use_waiter++;
                spin_unlock(&mylock.guard);
                printk("To be put into use_waiter***********\n");

                // struct timer_list mytimer;

                // init_timer(&mytimer);
                // mytimer.function=process_timeout;
                // mytimer.data=0;
                // mytimer.expires=jiffies+timeout_buffer * HZ;
                // add_timer(&my_timer)

                __down__(&mylock.use_waiter_sem, timeout_val) ;//semaphore count is not in use, purely as list
                
                spin_lock(&mylock.guard);
                mylock.use_waiter--;                      //

                printk("Removed from use_waiter***********\n");
            }
            mylock.type=NET_LOCK_USE;
            mylock.user++;

            spin_unlock(&mylock.guard);

            break;
       case NET_LOCK_SLEEP:
            printk("IN NET_LOCK_SLEEP\nPID %d type:%d, \n",  current->pid, type);
            spin_lock(&mylock.guard);
            while(mylock.user|mylock.use_waiter){
                printk("To be put into sleep_waiter***********\n");
                mylock.sleep_waiter++;
                spin_unlock(&mylock.guard);
                
                down(&mylock.sleep_waiter_sem);

                spin_lock(&mylock.guard);
                mylock.sleep_waiter--;                                               //////
                printk("Removed from sleep_waiter***********\n");

            }
            mylock.type=NET_LOCK_SLEEP;
            mylock.sleeper++;
            spin_unlock(&mylock.guard);
            break;
   }
  
    printk("----------OUT net_lock PID %d Comm %s acquired the lock %d------------\n\n", current->pid, current->comm, mylock.type);
    return 0;


}

SYSCALL_DEFINE0(net_unlock)
{
    printk(".......IN net_unlock PID: %d Comm %s...........\n", current->pid, current->comm);

    show_lock();
    spin_lock(&mylock.guard);
    switch(mylock.type){
        case NET_LOCK_USE:
            mylock.user--;
            if(!mylock.user){
                // mylock.sleep_waiter--;
                spin_unlock(&mylock.guard);
                up(&mylock.sleep_waiter_sem);
                spin_lock(&mylock.guard);
            }
            break;
        case NET_LOCK_SLEEP:
            mylock.sleeper--;    
            while(mylock.use_waiter){
              // mylock.use_waiter--;
              spin_unlock(&mylock.guard);
              __up__(&mylock.use_waiter_sem);

              spin_lock(&mylock.guard);
            }
            break;
    }
    spin_unlock(&mylock.guard);

    printk(".......OUT net_unlock PID %d Comm %s unlocked the lock %d...........\n\n", current->pid, current->comm, mylock.type);

    return 0;

}

SYSCALL_DEFINE0(net_lock_wait_timeout)
{
    printk("________ IN net_lock_wait_timeout PID: %d Comm %s________\n",  current->pid , current->comm);

    __down__(&mylock.wait_timeout_sem, MAX_SCHEDULE_TIMEOUT);

    printk("______ OUT net_lock_wait_timeout PID: %d COMM: %s ________\n\n", current->pid, current->comm);

    return 0;
}

