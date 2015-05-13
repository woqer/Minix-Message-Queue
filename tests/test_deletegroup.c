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
		printf("Usage: %s group_id\n", argv[0]);
		exit(0);
	}

	int group_id = atoi(argv[1]);

	printf("Testing mq_deletemqgroup(%d): %d\n", group_id, mq_deletemqgroup(group_id));

}