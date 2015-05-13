#!/bin/sh

cc test_addDesignatedUsers.c -o test_addDesignatedUsers
cc test_createPublicMessageQueueGroup.c -o test_createPublicMessageQueueGroup
cc test_createSecureMessageQueueGroup.c -o test_createSecureMessageQueueGroup
cc test_deletegroup.c -o test_deletegroup
cc test_groups.c -o test_groups
cc test_receiver.c -o test_receiver
cc test_removeDesignatedUsers.c -o test_removeDesignatedUsers
cc test_send.c -o test_send