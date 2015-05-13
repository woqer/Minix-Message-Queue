#ifndef _MQUEUELIB_H_
#define _MQUEUELIB_H_

#include <lib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int mqd_t;

/* the message structure for sending and receiving messages */
typedef struct {
	char *data;			/* data of the message */
	int sender_pid;			/* process sending the message */
	int *receiver_pids;		/* process(es) receiving the message */
	int num_receivers;
} message_t;

/* the message queue structure */
typedef struct {
	message_t *messages;		/* messages of queue */
	int first;                      /* position of first element */
	int last;                       /* position of last element */
	int count;                      /* number of queue elements */
	int size;			/* maximum size of the queue */
} queue_t;

/* the attribute structure for the message queue attributes */
typedef struct {
	char* name;			/* name of the message queue */
	int send_blocking;		/* is send blocking? */
	int receive_blocking;		/* is receiving blocking? */
	int max_messages;		/* number of messages that can be stored in each sub-queue */
	int max_message_size;		/* maximum size of the message */
	int grp_id;
} mq_attr_t;

/* the message queue structure */
typedef struct {
	mq_attr_t *attr;		/* the message queue attributes */
	int *sender_pids;		/* the processes sending messages to this queue */
	int *receiver_pids;		/* the processes receiving messages from this queue */
	int curr_num_messages_total;	/* the total number of messages in this queue */
	queue_t* queue_high;		/* the high priority queue */
	queue_t* queue_norm;		/* the regular priority queue */
	queue_t* queue_low;		/* the low priority queue*/
} mq_t;

typedef struct {
	endpoint_t proc_nr;
	int pid;			/* PID of the requester */
	int signum; 			/* the signal that should be sended to the process that request notification*/
} req_t;


/* the message queue group structure */
typedef struct {
	int grouptype; 				// 0 = secure, anything else = public
	int* sendingusers; 		// list of sending users IDs
	int* receivingusers;	// list of receiving users IDs
	uid_t creator;				// user id of the creator 
} mqgroup_t;


/******************************************************************************
** Add a new user to the admin user list (only superuser, aka root, can add new admins)
**  - uid: user id you want to add (see /etc/passwd to get information from users)
** returns 1 on success or -1 on error (non-authorized user)
******************************************************************************/
int mq_addadminuser(uid_t uid){
	message m;
	m.m1_i1 = uid;
	return(_syscall(PM_PROC_NR, 97, &m));
}

/******************************************************************************
** Remove user from the admin user list (only superuser, aka root, can remove admins)
**  - uid: user id you want to remove (see /etc/passwd to get information from users)
** returns 1 on success, -1 on error (user uid not in the list) or 0 on unauthorized user (non-root)
******************************************************************************/
int mq_removeadminuser(uid_t uid){
	message m;
	m.m1_i1 = uid;
	return(_syscall(PM_PROC_NR, 103, &m));
}

/******************************************************************************
** Creates a message queue group with specific attributes
**  - group_attr: pointer to a mqgroup_t structure with the specified attributes
**  							(see definition of the struct). Note: the group_attr->creator value
**								is not readed, the program will get the calling proccess user id
**  - max_send_rec: maximum size of sender and receiver list. They MUST be same size,
**									complete smallest list with zeros
** returns 1 on success, 0 if not authorized and -1 if fail (max group number reached)
******************************************************************************/
int mq_createmqgroup(mqgroup_t *group_attr, int max_send_rec) {
	message m;
	m.m1_i1 = group_attr->grouptype;
	m.m1_i2 = max_send_rec;
	char senders[max_send_rec*5]; // 4 digits, comma separated = 5 characters for each user
	char receivers[max_send_rec*5];

	snprintf(senders, 6,"%d",group_attr->sendingusers[0]);
	int mq_for_loop_counter;
	for(mq_for_loop_counter=1; mq_for_loop_counter<max_send_rec; mq_for_loop_counter++) {
		snprintf(senders, max_send_rec*5,"%s,%d", senders, group_attr->sendingusers[mq_for_loop_counter]);
	}

	snprintf(receivers, 6,"%d",group_attr->receivingusers[0]);
	for(mq_for_loop_counter=1; mq_for_loop_counter<max_send_rec; mq_for_loop_counter++) {
		snprintf(receivers, max_send_rec*5,"%s,%d", receivers, group_attr->receivingusers[mq_for_loop_counter]);
	}	

	m.m1_p1 = senders;
	m.m1_p2 = receivers;

	return(_syscall(PM_PROC_NR, 105, &m));

}

/******************************************************************************
** Deletes a group given its ID
**  - group_id: the group ID of the group you are going to remove
** returns 1 on success, 0 if unauthorized and -1 on error (group not found)
******************************************************************************/
int mq_deletemqgroup(int group_id) {
	message m;
	m.m1_i1 = group_id;
	return(_syscall(PM_PROC_NR, 108, &m));
}

/* Open will take attributes that:
	name = describe the name of the queue
	open_flag = will the process read, write, or do both transactions to the queue:
		O_RDONLY | O_WRONLY | O_RDWR
	blocking_flag = will the queue be blocking or be asynchronous
	mq_attr = will the user define specific attributes for this queue */
/* Returns the queue number for the process to reference */
mqd_t mq_open(const char *name, int open_flag, int blocking_flag, int max_mess, int grp_id)
{
	message m;
	m.m7_i1 = open_flag;
	m.m7_i2 = blocking_flag;
	m.m7_i3 = max_mess;
	m.m7_i4 = grp_id;
	m.m7_p1 = name;
	return(_syscall(PM_PROC_NR, 35, &m));
}

/* Process wants to close its connection to the queue */
/* Returns TRUE if successful, FAIL otherwise */
int mq_close(mqd_t mqd)
{
	message m;
	m.m1_i1 = (int)mqd;
	return(_syscall(PM_PROC_NR, 44, &m));
}

/* Process wants to change the attributes of the queue */
/* Returns TRUE if successful, FAIL otherwise */
int mq_setattr(mqd_t mqd, mq_attr_t *mq_attr)
{
	message m;
	m.m6_l1 = (long)mqd;
	m.m6_l2 = (long)mq_attr->send_blocking;
	m.m6_l3 = (long)mq_attr->receive_blocking;
	m.m6_s1 = (short)mq_attr->max_messages;
	m.m6_s2 = (short)mq_attr->max_message_size;
	m.m6_s3 = (short)mq_attr->grp_id;
	m.m6_p1 = mq_attr->name;
	return(_syscall(PM_PROC_NR, 56, &m));
}

/* Process wants to see the attributes of the queue */
/* Returns the attribute structure mq_attr_t */
int mq_getattr(mqd_t mqd, mq_attr_t *mq_attr)
{
	message m;
	
	m.m1_i1 = mqd;
	char *str_ptr = (char *)malloc(sizeof(char) * 512);
	m.m1_p1 = str_ptr;

	int status = _syscall(PM_PROC_NR, 58, &m);

	mq_attr->name = strtok(str_ptr,",");
	mq_attr->send_blocking = atoi(strtok(NULL,","));
	mq_attr->receive_blocking = atoi(strtok(NULL,","));
	mq_attr->max_messages = atoi(strtok(NULL,","));
	mq_attr->max_message_size = atoi(strtok(NULL,","));
	mq_attr->grp_id = atoi(strtok(NULL,","));

	return(status);
}

/* Process wants to send a message:
	mqd = queue number
	data = message structure to store in the queue
	message_length = length of the message to be sent
	priority = the priority of the message
*/
/* Returns TRUE if successful, FAIL otherwise */
int mq_send(mqd_t mqd, message_t *data, size_t message_length, unsigned int priority)
{
	message m;
	m.m7_i1 = (int)mqd;
	m.m7_i2 = (int)message_length;
	m.m7_i3 = (int)priority;
	m.m7_i4 = data->num_receivers;
	int mq_for_loop_counter;
	char pid_list[128]; // 5(pid length) * 16(MAX_PROCCESS) + 15(comma separated) < 128
	snprintf(pid_list, 128, "%d", data->sender_pid);

	for (mq_for_loop_counter = 0; mq_for_loop_counter < m.m7_i4; mq_for_loop_counter++)
	{
		snprintf(pid_list, 128, "%s,%d", pid_list, data->receiver_pids[mq_for_loop_counter]);
	}
	m.m7_p1 = pid_list;
	m.m7_p2 = data->data;
	return(_syscall(PM_PROC_NR, 69, &m));
}


/* Process wants to receive a message:
	mqd = queue number
	buffer_ptr = pointer to the buffer where the message should be saved
	buffer_length = length of available space in the buffer
	priority = the priority of the message which is being received
*/
/* Returns the message if successfull, if not returns empty string */
int mq_receive(mqd_t mqd, size_t buffer_length, char *buffer, unsigned int priority)
{
	message m;
	m.m1_i1 = (int)mqd;
	m.m1_i2 = (int)buffer_length;
	m.m1_i3 = (int)priority;
	m.m1_p1 = buffer;
	return(_syscall(PM_PROC_NR, 70, &m));
}

/* Process may specificy how it would like to be notified about new messages in the queue */
int mq_reqnotify(int signum)
{
	message m;
	m.m1_i1 = signum;
	return(_syscall(PM_PROC_NR, 79, &m));
}

#endif /* _MQUEUELIB_H_ */