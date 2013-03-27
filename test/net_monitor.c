#include <stdio.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#define net_lock 333
#define net_unlock 334
#define net_lock_wait_timeout 335

enum __netlock_t {
       NET_LOCK_USE,
       NET_LOCK_SLEEP
};
typedef enum __netlock_t netlock_t;

int main(int argc, char* argv[]){
	netlock_t type=NET_LOCK_SLEEP; 
	u_int16_t timeout = 0;
	while(1){
		syscall(net_lock, type, timeout);
		syscall(net_lock_wait_timeout);
		syscall(net_unlock);
	}
}