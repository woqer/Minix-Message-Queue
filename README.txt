# Authors:
  * Wilfrido Vidana
  * Mohit Hota
  * William Bafia
  * Chih-Feng Yu


*******************************************************************************
					README
*******************************************************************************
The enhancements made for Minix now provide for the protection of message passing between user processes using message queues and
subscriptions if required.

1. Message Queue Open: to open or create a message queue
2. Message Queue Close: to close a process' connection to the queue
3. Message Queue Set Attributes: to set queue attributes
4. Message Queue Get Attributes: to get queue attributes
5. Message Queue Send: to send a message into the message queue
6. Message Queue Receive: to remove a message from the message queue
7. Message Queue Request Notification: to subscribe or unsubscribe from asynchronous notifications
8. Message Queue Add Admin User: to add new designated user
9. Message Queue Remove Admin User: to remove existing designated user
10.Create Message Queue Group: to add new message queue group
11.Delete Message Queue Group: to delete existing message queue group


-------------------------------------------------------------------------------
Compiling and installing
-------------------------------------------------------------------------------
Step 1:
Add and update system files to the minix:
/usr/src/servers/pm/: table.c, message_queue.c, message_queue.h, proto.h, Makefile
/usr/src/include/minix/: callnr.h
/usr/src/include/: mqueuelib.h

Step 2:
Edit:
/usr/src/include/Makefile: change mqueue.h with mqueuelib.h

Step 3:
-Go to /usr/src/releasetools - run "make hdboot"
-then run "sync"
-then reboot the system

Note: Step1 & stpe3 are made by the copy.sh script (it should be placed along with the other files in the same folder, for example /root/source)

Step 4:
Add 5 new user in the minix(admin, f1, f2, f3, f4):
  # user add -m -g user fadmin
  # user add -m -g user f1
  # user add -m -g user f2
  # user add -m -g user f3
  # user add -m -g user f4

Step 5:
Add client program files and compile:
/root: test_send.c, test_receiver.c, test_groups.c, test_deletegroup.c
/home/f* :test_send.c, test_receiver.c, test_groups.c, test_deletegroup.c

Note: Place all the tset programs in the same folder along with move_to_users.sh and compile-all.sh 

Step6:
Execute

-------------------------------------------------------------------------------
Execution
-------------------------------------------------------------------------------

Minix Credentials:
username: root
password: 1234

1.test_groups.c
In this test, we will test every hierarchy of users whether allowed to use the system calls. 
Different hierarchy of users should have differnt results.
  
  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Example 1: Super user('root',uid=0)
  
    $ ./test_groups 1000
  
  After excuting the command above and putting the numbers or values that the super user want, it will return all 
  values positive printed on the screen which means the super user allowed to do everything.
  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Example 2: Designated user('fadmin',uid=1000 ; the 'root' should add 'fadmin' into Designated user list first)

    $ su fadmin			//user change to 'fadmin'
    $ ./test_groups 1000
  
  After excuting the commands above and putting the numbers or values that the designated user want, it will return 
  positive values only when adding sending or receiving users into public or secure mq-groups but return nagetive values 
  while adding or removing admin users. It means the designated user can create two types of mq-groups but can't add or 
  remove designated users.
  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.test_deletegroup.c
In this test, we will test every hierarchy of users whether allowed to delete an exsitng mq-group (or created by other creator).

3.test_send.c & test_receiver.c
After assigning the hierarchy form test_group.c, all users can test these two test files to show whether they are allowed to open a message 
queue and send message to the queue or receive message from the queue. 

Note: Need one user(sender) creat message queue and send message first, then the other user(receiver) can receive it.

There are other several test programs for individual functions.

