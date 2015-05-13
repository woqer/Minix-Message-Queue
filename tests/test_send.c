#include <mqueuelib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	if(argc != 3) {
		printf("need one argument,pid of receving process\n");
		printf("usage; test_file receiving_pid grp_id\n");
		exit(0);
	}

	int grp_id = atoi(argv[2]);
	printf("opening queueu for group %d\n", grp_id);
	mqd_t mqd = mq_open("test_block", O_WRONLY, 0, 4, grp_id);
	printf("Test mqd: %d\n",mqd);

	const char *pid_rec = argv[1];
	int pid_int = atoi(pid_rec);
	message_t *mess = (message_t *)malloc(sizeof(message_t));
	mess->data = (char *)malloc(sizeof(char)*64);
	mess->receiver_pids = (int *)malloc(sizeof(int)*1);
	mess->receiver_pids[0] = pid_int; 
	mess->data = "Hello! this is a test message. Ni Hao! Holaaa";
	mess->sender_pid = getpid();
	mess->num_receivers = 1;

	int send_success = mq_send(mqd, mess, 64, 3);

	printf("Test send_success: %d\n", send_success);

}
