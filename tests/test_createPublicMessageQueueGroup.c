#include <mqueuelib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	printf("Executing tests for user %d...\n" ,getuid());
	
	mqgroup_t *group_attr = (mqgroup_t *)malloc(sizeof(mqgroup_t));
	group_attr->sendingusers = (int *)malloc(sizeof(int)*5);
	
	int i;
	for(i=0; i<5; i++) {
		group_attr->sendingusers[i] = i+10;	
	}

	group_attr->receivingusers = (int *)malloc(sizeof(int)*5);
	for(i=0; i<5; i++) {
		group_attr->receivingusers[i] = i+50;
	}
	
	group_attr->creator = getuid();
	group_attr->grouptype = 1;
	
	printf("Testing public mq_createmqgroup(%d); %d\n", getuid(), mq_createmqgroup(group_attr,5));
}