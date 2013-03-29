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
#include <stdlib.h>
#include <errno.h>

#define net_lock 333
#define net_unlock 334

enum __netlock_t {
       NET_LOCK_USE,
       NET_LOCK_SLEEP
};
typedef enum __netlock_t netlock_t;

int main(int argc, char* argv[]){
		if(argc!=4){
			printf("invalid arguments\n");
			exit(1);
		}
		
		u_int16_t timeout_val;
		int spin;
		int sleep_val;

		netlock_t type=NET_LOCK_USE;

		struct timeval tv;
		pid_t pid = getpid();

		sscanf(argv[1], "%d", &sleep_val);
		sscanf(argv[2], "%d", &timeout_val);
		sscanf(argv[3], "%d", &spin);

		gettimeofday(&tv, NULL);
		printf("%d: sleeping pid:%d args:%d,%d\n", tv.tv_sec, pid, sleep_val, timeout_val);
		sleep(sleep_val);

		gettimeofday(&tv, NULL);
		printf("%d: calling_net_lock pid:%d\n", tv.tv_sec, pid);
		syscall(net_lock, type, timeout_val);

		gettimeofday(&tv, NULL);
		printf("%d: return_net_lock pid:%d\n", tv.tv_sec, pid);
		while(spin--)
			;

		gettimeofday(&tv, NULL);
		printf("%d: calling_net_unlock pid:%d\n", tv.tv_sec, pid);
		syscall(net_unlock);
}