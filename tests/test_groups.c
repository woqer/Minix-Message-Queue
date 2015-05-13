#include <mqueuelib.h>

int prompt(char *question) {
	int a = 0;
	printf("%s : ", question);
	scanf("%d", &a);
	printf("printed %d\n", a);
	return a;
}

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

	mqgroup_t *group_attr = (mqgroup_t *)malloc(sizeof(mqgroup_t));
	group_attr->grouptype = prompt("Enter grouptype (0=secure, otherwise=public)");
	int max_users = prompt("Enter number of senders");
	group_attr->sendingusers = (int *)malloc(sizeof(int)*max_users);
	for(i=0; i<max_users; i++) {
		group_attr->sendingusers[i] = prompt("enter sendinguser");	
	}

	max_users = prompt("Enter number of receivers");
	group_attr->receivingusers = (int *)malloc(sizeof(int)*max_users);
	for(i=0; i<max_users; i++) {
		group_attr->receivingusers[i] = prompt("enter receivingusers");	
	}

	group_attr->creator = getuid();

	int grp_id = mq_createmqgroup(group_attr,1);
	printf("Testing mq_createmqgroup: %d\n", grp_id);
	printf("GROUP_ID = %d\n", grp_id);

	char c;
	while((c = getchar()) != '\n' && c != EOF);
	
	printf("Do you want to remove users from admin list? [y/N]: ");
	scanf("%c",&c);
	if (c == 'y') {
		for(i=0; i<n_users; i++) {
			printf("Testing mq_removeadminuser(%d): %d\n",users[i],mq_removeadminuser(users[i]));
		}
	}

	printf("Exisiting tests!\n");
	
}