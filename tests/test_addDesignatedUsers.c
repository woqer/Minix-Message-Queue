#include <mqueuelib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, const char *argv[])
{

	if (argc < 2) {
		printf("Usage: %s user1_id user2_id...\n", argv[0]);
		exit(0);
	}

	int n_users = argc - 1;
	int users[n_users];

	int i;
	for(i=0; i<n_users; i++) {
		users[i] = atoi(argv[i+1]);
	}

	printf("Executing tests for user %d...\n" ,getuid());

	for(i=0; i<n_users; i++) {
		printf("Testing mq_addadminuser(%d); %d\n",users[i],mq_addadminuser(users[i]));
	}
}