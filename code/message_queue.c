#include "mqueuelib.h"
#include "message_queue.h"

// tracking counters
static int curr_num_queues = 0;
static int curr_num_req = 0;

static int test_counter = 0;
static int group_id_counter = 1;

static mq_t queues[MAX_QUEUES];	/* list of open queues */
static int queues_mask[MAX_QUEUES] = {0};
static req_t req_receivers[MAX_REQ_NOT] = {0}; /* list of requests for notifications */
static uid_t admin_ids[MAX_ADMINS];
static mqgroup_t* groups_attr[MAX_ADMINS] = {0};
static int groups_ids[MAX_ADMINS];

int do_mq_addadminuser(){
	uid_t my_uid = mproc[who_p].mp_realuid;
	uid_t new_admin_id = m_in.m1_i1;

	if(my_uid == 0) {
		//add
		int free_element;
			if (free_element = array_search(admin_ids, MAX_ADMINS, 0) < 0) {
				printf("add-admin: exceeded maximum number of admins (%d)\n", MAX_ADMINS);
				return FAIL;
			} 

		admin_ids[free_element] = new_admin_id;
		return TRUE;

	} else {
		
		printf("add-admin: error, user not authorized (should be root)\n");
		return FAIL;
	}
}

int do_mq_removeadminuser(){
	uid_t my_uid = mproc[who_p].mp_realuid;
	uid_t de_admin_id = m_in.m1_i1;

	if(my_uid == 0) {
		// remove
		int target_element = array_search(admin_ids, MAX_ADMINS, de_admin_id);
		if (target_element < 0) {
			return FAIL;
		} else {
			admin_ids[target_element] = 0;
			return TRUE;	
		}
	} else {
		// not root, exit
		printf("remove-admin: error, user not authorized (should be root)\n");
		return FALSE;
	}
}

int do_mq_createmqgroup(){
	printf("do_mq_createmqgroup: beginning of method\n");
	mqgroup_t *group_attr = (mqgroup_t *)malloc(sizeof(mqgroup_t));
	group_attr->grouptype = m_in.m1_i1;

	char *senders = (char *)malloc(sizeof(char)*m_in.m1_i2*5);
	char *receivers = (char *)malloc(sizeof(char)*m_in.m1_i2*5);

	sys_datacopy(who_e, (vir_bytes)m_in.m1_p1, SELF, (vir_bytes)senders, m_in.m1_i2*5);
	sys_datacopy(who_e, (vir_bytes)m_in.m1_p2, SELF, (vir_bytes)receivers, m_in.m1_i2*5);

	uid_t my_uid = mproc[who_p].mp_realuid;
	
	group_attr->creator = my_uid;
	printf("do_mq_createmqgroup: before malloc sending and receiving...\n");
	group_attr->sendingusers = (int *)malloc(sizeof(int)*MAX_ADMINS);
	group_attr->receivingusers = (int *)malloc(sizeof(int)*MAX_ADMINS);

	group_attr->sendingusers[0] = atoi(strtok(senders,","));
	int i;
	for(i = 1; i<m_in.m1_i2; i++) {
		group_attr->sendingusers[i] = atoi(strtok(NULL,","));
	}	

	group_attr->receivingusers[0] = atoi(strtok(receivers,","));
	for(i = 1; i<m_in.m1_i2; i++) {
		group_attr->receivingusers[i] = atoi(strtok(NULL,","));
	}

	printf("do_mq_createmqgroup: after parsing senders and receivers, before array search admin\n");

	if(array_search(admin_ids, MAX_ADMINS, my_uid) >= 0) {
		// do stuff
		printf("do_mq_createmqgroup: after array search admin, before searching empty_spot\n");
		int empty_spot = array_search(groups_ids, MAX_ADMINS, 0);

		if (empty_spot < 0) {
			printf("Error: max group number reached\n");
			return FAIL;
		}
		printf("do_mq_createmqgroup: after searching empty_spot(%d), before using it in global arrays\n",empty_spot);

		groups_ids[empty_spot] = group_id_counter++;
		groups_attr[empty_spot] = group_attr;

		return group_id_counter - 1;

	} else {
		// not enough privileges
		return FALSE;
	}

}

int do_mq_deletemqgroup(){
	uid_t my_uid = mproc[who_p].mp_realuid;
	int group_id = m_in.m1_i1;

	int group_index = array_search(groups_ids, MAX_ADMINS, group_id);
	
	if (group_index < 0) {
		printf("Error, groupd not found!\n");
		return FAIL;
	}
	
	mqgroup_t *group_attr = groups_attr[group_index];
	// or root user
	if(my_uid == group_attr->creator || my_uid == 0) {
		// do stuff

		printf("Closing queue/s of group...\n");
		close_mq_from_group(group_id);

		printf("Deleting group %d...\n", group_id);
		groups_ids[group_index] = 0;
		free(groups_attr[group_index]);

		return TRUE;

	} else {
		// not privileged
		printf("do_mq_deletemqgroup: Error, not privileged user\n");
		return FALSE;
	}
}


mqd_t do_mq_open()
{
	size_t name_size = sizeof(char) * MAX_NAME_SIZE;
	char *name = (char *)malloc(name_size);
	sys_datacopy(who_e, (vir_bytes)m_in.m7_p1, SELF, (vir_bytes)name, name_size);
	int open_flag = m_in.m7_i1;
	int blocking_flag = m_in.m7_i2;
	int max_mess = m_in.m7_i3;
	int grp_id = m_in.m7_i4;
	uid_t my_uid = mproc[who_p].mp_realuid;

	int grp_index = array_search(groups_ids, MAX_ADMINS, grp_id);

	if (grp_index < 0) {
		printf("Error: queue Group not found in the group list\n");
		return FAIL;
	}

	int grp_policy = groups_attr[grp_index]->grouptype;

	int uid_index = array_search(groups_attr[grp_index]->sendingusers, MAX_ADMINS, my_uid);

	if(uid_index < 0) {
		uid_index = array_search(groups_attr[grp_index]->receivingusers, MAX_ADMINS, my_uid);
	}

	/*              public                  ||             secure               */
	if ((grp_policy != 0 && uid_index >= 0) || (grp_policy == 0 && uid_index < 0)) { 
		if (my_uid != groups_attr[grp_index]->creator || my_uid != 0) { 
			printf("Error: user not allowed to open the queue\n");
			return FAIL;
		}
	} 

	// printf("do_mq_open: printing of variables\n");
	// printf("      name           %s\n", name);
	// printf("      open_flag      %d\n", open_flag);
	// printf("      blocking_flag  %d\n", blocking_flag);
	// printf("      max_mess       %d\n", max_mess);

	if (curr_num_queues == MAX_QUEUES)
	{
		printf("Maximum number of queues have already been opened.\n");
		return FAIL;
	}

	pid_t calling_proc = mproc[who_p].mp_pid;

	if (calling_proc < 1)
	{
		printf("Unable to acquire the calling process's PID.\n");
		return FAIL;
	}

	if (open_flag != O_RDONLY && open_flag != O_WRONLY && open_flag != O_RDWR)
	{
		printf("The open flag must either be O_RDONLY, O_WRONLY, or O_RDWR.\n");
		return FAIL;
	}


	// printf("do_mq_open: name of queue: %s\n", name);
	int i;
	for (i = 0; i < curr_num_queues; i++)
	{
		if (strcmp(queues[i].attr->name, name) == 0)
		{
			if (open_flag == O_RDONLY)
			{
				int success = addproc(queues[i].receiver_pids, calling_proc);
				if (success == FAIL)
					return FAIL;
			}
			else if (open_flag == O_WRONLY)
			{
				int success = addproc(queues[i].sender_pids, calling_proc);
				if (success == FAIL)
					return FAIL;
			}
			else
			{
				int success = addproc(queues[i].sender_pids, calling_proc);
				if (success == FAIL)
					return FAIL;
				success = addproc(queues[i].receiver_pids, calling_proc);
				if (success == FAIL)
				{
					deleteproc(queues[i].sender_pids, calling_proc);
					return FAIL;
				}
			}
			return i;
		}
	}

	queues_mask[curr_num_queues++] = 1;

	// printf("do_mq_open: curr_num_queues %d\n", curr_num_req);

	mq_t *new_queue = malloc(sizeof(mq_t));
	new_queue->attr = malloc(sizeof(mq_attr_t));
	new_queue->attr->name = (char *)name;
	new_queue->attr->send_blocking = blocking_flag;
	new_queue->attr->receive_blocking = blocking_flag;
	new_queue->attr->max_message_size = MAX_MESSAGE_SIZE;
	new_queue->attr->grp_id = grp_id;

	printf("do_mq_open: new_queue->attr->grp_id = %d\n", new_queue->attr->grp_id);

	if (max_mess)
	{
		new_queue->attr->max_messages = max_mess;
		
	}
	else
	{
		new_queue->attr->max_messages = MAX_MESSAGES;
	}

	new_queue->curr_num_messages_total = 0;
	new_queue->sender_pids = malloc(sizeof(SIZE_INT * MAX_PROCESSES));
	new_queue->receiver_pids = malloc(sizeof(SIZE_INT * MAX_PROCESSES));
	new_queue->queue_high = malloc(sizeof(queue_t));
	new_queue->queue_norm = malloc(sizeof(queue_t));
	new_queue->queue_low = malloc(sizeof(queue_t));

	int num_messages = new_queue->attr->max_messages;
	int size_messages = new_queue->attr->max_message_size;

	// TODO: go back and fix this size of
	new_queue->queue_high->messages = malloc(sizeof(message_t) * num_messages * size_messages);
	new_queue->queue_norm->messages = malloc(sizeof(message_t) * num_messages * size_messages);
	new_queue->queue_low->messages = malloc(sizeof(message_t) * num_messages * size_messages);

	init_queue(new_queue->queue_high, num_messages);
	init_queue(new_queue->queue_norm, num_messages);
	init_queue(new_queue->queue_low, num_messages);

	queues[curr_num_queues - 1] = *new_queue;

	return curr_num_queues - 1;
	
}

int do_mq_close()
{
	mqd_t mqd = m_in.m1_i1;

	return(mq_close_helper(mqd));
}


int do_mq_setattr()
{

	size_t name_size = sizeof(char) * MAX_NAME_SIZE;
	mqd_t mqd = (int)m_in.m6_l1;
	mq_attr_t *mq_attr = malloc(sizeof(mq_attr_t));
	mq_attr->send_blocking = (int)m_in.m6_l2;
	mq_attr->receive_blocking = (int)m_in.m6_l3;
	mq_attr->max_messages = (int)m_in.m6_s1;
	mq_attr->max_message_size = (int)m_in.m6_s2;
	mq_attr->grp_id = (int)m_in.m6_s3;
	mq_attr->name = malloc(name_size);
	sys_datacopy(who_e, (vir_bytes)m_in.m6_p1, SELF, (vir_bytes)mq_attr->name, name_size);	
	// Could potentially be null...

	printf("do_mq_setattr: printing variables\n");
	printf("   mqd                         %d\n", mqd);
	printf("   mq_attr->send_blocking      %d\n", mq_attr->send_blocking);
	printf("   mq_attr->receive_blocking   %d\n", mq_attr->receive_blocking);
	printf("   mq_attr->max_messages       %d\n", mq_attr->max_messages);
	printf("   mq_attr->max_message_size   %d\n", mq_attr->max_message_size);
	printf("   mq_attr->name               %s\n", mq_attr->name);

	queues[mqd].attr = mq_attr;

	return TRUE;
}

int do_mq_getattr()
{
	size_t str_size = sizeof(char) * (MAX_NAME_SIZE + 32);
	mqd_t mqd = (int)m_in.m6_l1;

	mq_attr_t *attr_ptr = (mq_attr_t *)malloc(sizeof(attr_ptr));
	attr_ptr->name = (char *)malloc(sizeof(char)*MAX_NAME_SIZE);
	attr_ptr = queues[mqd].attr;	


	char *parse_string = (char *)malloc(str_size);
	// m_in.m6_l2 = attr_ptr->send_blocking;
	// m_in.m6_l3 = attr_ptr->receive_blocking;
	// m_in.m6_s1 = attr_ptr->max_messages;
	// m_in.m6_s2 = attr_ptr->max_message_size;

	snprintf(parse_string,str_size,"%s",attr_ptr->name);
	snprintf(parse_string,str_size,"%s,%d",parse_string,attr_ptr->send_blocking);
	snprintf(parse_string,str_size,"%s,%d",parse_string,attr_ptr->receive_blocking);
	snprintf(parse_string,str_size,"%s,%d",parse_string,attr_ptr->max_messages);
	snprintf(parse_string,str_size,"%s,%d",parse_string,attr_ptr->max_message_size);
	snprintf(parse_string,str_size,"%s,%d",parse_string,attr_ptr->grp_id);
	
	sys_datacopy(SELF, (vir_bytes)parse_string,
		who_e, (vir_bytes)m_in.m1_p1, str_size);

	printf("do_mq_getattr: printing variables\n");
	printf("   mqd.............%d\n", mqd);
	printf("   parse_string....%s\n", parse_string);

	// printf("checking attr->send_blocking      %d\n", queues[mqd].attr->send_blocking);
	// Could potentially be null and I might be messing up pointers...
	return TRUE;
}

int do_mq_send()
{
	mqd_t mqd = m_in.m7_i1;

	if (mqd >= curr_num_queues || mqd < 0) {
		printf("Wrong mq identifier (mqd)\n");
		return FAIL;
	} else if (queues_mask[mqd] == 0) {
		printf("Error: queue does not exist\n");
		return FAIL;
	}

	message_t *data = malloc(sizeof(message_t));
	size_t message_length = m_in.m7_i2;
	unsigned int priority = m_in.m7_i3;

	uid_t my_uid = mproc[who_p].mp_realuid;
	int mq_grp = queues[mqd].attr->grp_id;

	int grp_index = array_search(groups_ids, MAX_ADMINS, mq_grp);

	printf("Sending to queue[%d]\n", mqd);

	if (grp_index < 0) {
		printf("Error: queue Group not found in the group list\n");
		return FAIL;
	}

	int grp_policy = groups_attr[grp_index]->grouptype;

	int uid_index = array_search(groups_attr[grp_index]->sendingusers, MAX_ADMINS, my_uid);

	/*              public             ||             secure               */
	if ((grp_policy && uid_index >= 0) || (grp_policy == 0 && uid_index < 0)) {
		if (my_uid != groups_attr[grp_index]->creator || my_uid != 0) { 
			printf("Error: user not allowed to send in the queue\n");
			return FALSE;
		}
	} 

	if (message_length > queues[mqd].attr->max_message_size)
	{
		printf("The length of the message is longer than we accept.\n");
		return FAIL;
	}

	char *pid_list = (char *)malloc(sizeof(char)*128);
	data->receiver_pids = malloc(sizeof(int)*m_in.m7_i4);
	data->data = malloc(sizeof(char)*message_length);
	
	sys_datacopy(who_e, (vir_bytes)m_in.m7_p1, SELF,(vir_bytes)pid_list, sizeof(char)*128);
	sys_datacopy(who_e, (vir_bytes)m_in.m7_p2, SELF,(vir_bytes)data->data, sizeof(char)*message_length);

	data->sender_pid = atoi(strtok(pid_list,","));
	printf("do_mq_send: parsing sender_pid = %d\n", data->sender_pid);

	int i;
	for (i = 0; i < m_in.m7_i4; i++)
	{
		data->receiver_pids[i] = atoi(strtok(NULL,","));
		printf("do_mq_send: before enqueue, writing data->receiver_pid[%d]=%d\n", i, data->receiver_pids[i]);
	}

	int success;
	switch (priority)
	{
		case 1:
			if (queues[mqd].queue_high->count == queues[mqd].queue_high->size)
			{
				printf("The queue is already full.\n");
				return FAIL;	// queue is full
			}
			success = enqueue(queues[mqd].queue_high, data);
			break;
		case 2:
			if (queues[mqd].queue_norm->count == queues[mqd].queue_norm->size)
			{
				printf("The queue is already full.\n");
				return FAIL;	// queue is full
			}
			success = enqueue(queues[mqd].queue_norm, data);
			break;
		case 3:
			if (queues[mqd].queue_low->count == queues[mqd].queue_low->size)
			{
				printf("The queue is already full.\n");
				return FAIL;	// queue is full
			}
			success = enqueue(queues[mqd].queue_low, data);
			break;
		default:
			return FAIL;
			break;
	}
	if (success == TRUE)
	{
		queues[mqd].curr_num_messages_total++;
		printf("do_mq_send: before notify_rec, data->receiver_pids[0]=%d\n", data->receiver_pids[0]);
		notify_rec(data->receiver_pids);
	}

	printf("do_mq_send: success %d\n", success);
	return success;
	
}

int do_mq_receive()
{
	printf("do_mq_receive: hello! before getting any variable...\n");
	mqd_t mqd = m_in.m1_i1;
	size_t buffer_length = (size_t)m_in.m1_i2;
	unsigned int priority = (unsigned int)m_in.m1_i3;

	if (mqd < 0 || queues_mask[mqd] == 0) {
		printf("Error: queue does not exist\n");
		return FAIL;
	}

	uid_t my_uid = mproc[who_p].mp_realuid;
	int mq_grp = queues[mqd].attr->grp_id;

	printf("do_mq_receive: searchging for group %d...\n", mq_grp);

	int grp_index = array_search(groups_ids, MAX_ADMINS, mq_grp);

	if (grp_index < 0) {
		printf("Error: queue Group not found in the group list\n");
		return FAIL;
	}

	printf("do_mq_receive: fetching from groups_attr[%d]\n", grp_index);
	int grp_policy = groups_attr[grp_index]->grouptype;

	int uid_index = array_search(groups_attr[grp_index]->receivingusers, MAX_ADMINS, my_uid);

	/*              public             ||             secure               */
	if ((grp_policy && uid_index >= 0) || (grp_policy == 0 && uid_index < 0)) { 
		if (my_uid != groups_attr[grp_index]->creator || my_uid != 0) { 
			printf("Error: user not allowed to receive from this queue\n");
			return FALSE;
		}
	} 

	char *buffer_ptr = (char *)malloc(sizeof(char) * buffer_length);
	
	printf("do_mq_receive: after malloc before checking size...\n");

	if (buffer_length < queues[mqd].attr->max_message_size)
	{
		printf("The length of the buffer is not long enough for the message.\n");
		return FAIL;	// not a big enough buffer
	}
	
	printf("do_mq_receive: after buffer_length, before dequeueing...\n");

	int success;
	switch (priority)
	{
		case 1:
			if (queues[mqd].queue_high->count == 0)
			{
				printf("The queue is empty.\n");
				return FALSE;	// no messages
			}
			success = dequeue(queues[mqd].queue_high, &buffer_ptr);
			break;
		case 2:
			if (queues[mqd].queue_norm->count == 0)
			{
				printf("The queue is empty.\n");
				return FALSE;	// no messages
			}
			success = dequeue(queues[mqd].queue_norm, &buffer_ptr);
			break;
		case 3:
			if (queues[mqd].queue_low->count == 0)
			{
				printf("The queue is empty.\n");
				return FALSE;	// no messages
			}
			success = dequeue(queues[mqd].queue_low, &buffer_ptr);
			break;
		default:
			return FALSE;
			break;
	}
	if (success == TRUE)
	{
		queues[mqd].curr_num_messages_total--;
	}


	printf("do_mq_receive: dequeue of message: %s\n", buffer_ptr);

	sys_datacopy(SELF, (vir_bytes)buffer_ptr,
		who_e, (vir_bytes)m_in.m1_p1, queues[mqd].attr->max_message_size);

	return TRUE;
}

int do_mq_reqnotify()
{
	req_t notification;
	notification.pid = mproc[who_p].mp_pid;
	notification.proc_nr = who_e;
	notification.signum = m_in.m1_i1;
	req_receivers[curr_num_req++] = notification;

	printf("proccess %d registered for signal %d\n", notification.pid, notification.signum);

	return 1;
}

void init_queue(queue_t *q, unsigned int num_messages)
{
	q->first = 0;
	if (num_messages == 0)
	{
		q->last = MAX_MESSAGES - 1;
		q->size = MAX_MESSAGES;
	}
	else
	{
		q->last = num_messages - 1;
		q->size = num_messages;
	}
	q->count = 0;
}

int enqueue(queue_t *q, message_t *msg)
{
	if (q->count >= q->size)
	{
		printf("Warning, queue overflow enqueue.\n");
		return FAIL;
	}
	else
	{
		q->last = (q->last + 1) % q->size;
		q->messages[q->last] = *msg;
		q->count = q->count + 1;
		return TRUE;
	}
}

int dequeue(queue_t *q, char **msg)
{
	if (q->count <= 0)
	{
		printf("Warning, empty queue dequeue.\n");
		return FAIL;
	}
	else
	{
		message_t *msg_t;
		msg_t = &(q->messages[q->first]);
		*msg = msg_t->data;

		printf("dequeue: msg: %s\n", *msg);

		int success = deleteproc(msg_t->receiver_pids, mproc[who_p].mp_pid);
		
		success = emptyprocs(msg_t->receiver_pids);

		if (success == FALSE)
		{
			return TRUE;
		}
		else
		{
			q->first = (q->first + 1) % q->size;
			q->count = q->count - 1;
		}
	}

	return TRUE;
}

/* initprocs - Initialize the process list */
void initprocs(int *procs)
{
	int i;

	for (i = 0; i < MAX_PROCESSES; i++)
	{
		procs[i] = 0;
	}
}

/* addproc - Add a proc to the proc list */
int addproc(int *procs, pid_t pid)
{
	int i;

	if (pid < 1)
	{
		printf("Cannot add process of ID less than 1...\n");
		return FAIL;
	}

	for (i = 0; i < MAX_PROCESSES; i++) {
		if (procs[i] == 0)
		{
			procs[i] = pid;
			printf("Process registered successfully...\n");
			return TRUE;
		}
	}
	printf("Tried to add too many processes...\n");
	return FAIL;
}

/* deleteproc - Delete a proc whose PID=pid from the proc list */
int deleteproc(int *procs, pid_t pid)
{
	int i;

	if (pid < 1)
	{
		printf("Cannot delete process of ID less than 1...\n");
		return FAIL;
	}

	for (i = 0; i < MAX_PROCESSES; i++)
	{
		if (procs[i] == pid)
		{
			procs[i] = 0;
			printf("Process removed successfully...\n");
			return TRUE;
		}
	}
	printf("Could not find the process you are trying to remove...\n");
	return FAIL;
}

int emptyprocs(int *procs)
{
	int i;
	for (i = 0; i < MAX_PROCESSES; i++)
	{
		if (procs[i] != 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/* initnotify - Initialize the process list of notify requesters*/
// void initnotify()
// {
// 	int i;
// 	for (i = 0; i < MAX_REQ_NOT; i++)
// 	{
// 		req_receivers[i].pid = 0;
// 		req_receivers[i].signum = 0;
// 	}
// }


void notify_rec(int *receivers)
{
	printf("inside notify_rec..., argument receivers[0]=%d\n",receivers[0]);
	int i, status;
	for (i = 0; i < MAX_REQ_NOT; i++)
	{
		if (req_receivers[i].pid > 0)
		{
			printf("inside notify_rec... at least 1 pid (%d) requested notify\n", req_receivers[i].pid);

			status = array_search(receivers, MAX_PROCESSES, req_receivers[i].pid);
			if (status != FAIL)
			{
				printf("inside notify_rec... found receiver %d in array_search, executing kill\n",req_receivers[i].pid);

				int kill_status = sys_kill(req_receivers[i].proc_nr, req_receivers[i].signum);
				if ( kill_status < 0)
				{
					printf("Error %d: some proccess (%d) didn't receive the notification (%d)\n", kill_status, req_receivers[i].proc_nr, req_receivers[i].signum);
				}	

			}
		}
	}	
}

// returns the index of the match
int array_search(int *ary, int max, int element)
{
	int i = 0;

	while (i < max && ary[i] != element)
	{
		if (ary[i] > 0)
		{
			i++;
		}
		else
		{
			return FAIL;
		}
	}

	if (i < max)
	{
		return i;
	}
	else
	{
		return FAIL;
	}
}

void close_mq_from_group(int group_id) {
	int mqd = 0;
	int queue_group = 0;
	if (group_id < 1) {
		printf("Error (search mq from group): invalid group_id\n");
		return FAIL;
	}
	
	for(mqd=0; mqd < MAX_QUEUES; mqd++) {
			if (queues_mask[mqd] != 0) {
				queue_group = queues[mqd].attr->grp_id;
				if (queue_group == group_id) {
					printf(" Trying to close Queue [%d] because group deleted\n", mqd);
					mq_close_helper(mqd);
				}
			}
	}

	// if (queues_mask[mqd] != 0) {
	// 	queue_group = queues[mqd++].attr->grp_id;
	// }
	// while((queue_group < 1) && (queue_group != group_id) && (mqd < MAX_QUEUES)) {
	// 	if (queues_mask[mqd] != 0) {
	// 		queue_group = queues[mqd++].attr->grp_id;
	// 	}
	// }

}

int mq_close_helper(mqd_t mqd) {

	if ( mqd < 0 || queues_mask[mqd] == 0) {
		printf("Error: queue does not exist\n");
		return FALSE;
	}
	uid_t my_uid = mproc[who_p].mp_realuid;
	int mq_grp = queues[mqd].attr->grp_id;

	printf("mq_close_helper: mq_grp=%d, mqd=%d\n", mq_grp, mqd);

	int grp_index = array_search(groups_ids, MAX_ADMINS, mq_grp);

	if (grp_index < 0) {
		printf("Fatal Error: queue Group not found in the group list\n");
		int j;
		for (j=0; j<MAX_ADMINS; j++) {
			printf("groups_ids[%d]=%d\t",j,groups_ids[j]);
		}
		return FAIL;
	}

	int grp_policy = groups_attr[grp_index]->grouptype;

	int uid_index = array_search(groups_attr[grp_index]->sendingusers, MAX_ADMINS, my_uid);

	if(uid_index < 0) {
		uid_index = array_search(groups_attr[grp_index]->receivingusers, MAX_ADMINS, my_uid);
	}

	/*              public                  ||             secure               */
	if ((grp_policy != 0 && uid_index >= 0) || (grp_policy == 0 && uid_index < 0)) { 
		if (my_uid != groups_attr[grp_index]->creator || my_uid != 0) { 
			printf("Error: user not allowed to close the queue\n");
			return FALSE;
		}
	} 

	int success = deleteproc(queues[mqd].sender_pids, mproc[who_p].mp_pid);
	int success_sender = emptyprocs(queues[mqd].sender_pids);
	success = deleteproc(queues[mqd].receiver_pids, mproc[who_p].mp_pid);
	int success_receiver = emptyprocs(queues[mqd].receiver_pids);

	if (success_sender == TRUE && success_receiver == TRUE)
	{
		free(queues[mqd].attr);
		free(queues[mqd].sender_pids);
		free(queues[mqd].receiver_pids);
		free(queues[mqd].queue_high->messages);
		free(queues[mqd].queue_norm->messages);
		free(queues[mqd].queue_low->messages);
		free(queues[mqd].queue_high);
		free(queues[mqd].queue_norm);
		free(queues[mqd].queue_low);
		free(&queues[mqd]);
	}

	queues_mask[mqd] = 0;

	printf("Queue [%d] closed succesfully\n", mqd);

	return TRUE;
}