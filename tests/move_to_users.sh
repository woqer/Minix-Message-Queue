#!/bin/bash

find /home -maxdepth 1 -type d -iname "f*" -exec cp test_send {} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_receiver {} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_groups {} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_deletegroup {} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_addDesignatedUsers{} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_createPublicMessageQueueGroup {} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_createSecureMessageQueueGroup {} \;
find /home -maxdepth 1 -type d -iname "f*" -exec cp test_removeDesignatedUsers {} \;

find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_send \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_receiver \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_groups \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_deletegroup \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_addDesignatedUsers \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_createPublicMessageQueueGroup \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_createSecureMessageQueueGroup \;
find /home -maxdepth 1 -type d -iname "f*" -exec chmod 777 {}/test_removeDesignatedUsers \;