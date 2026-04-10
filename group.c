/*
Group Communications Functions
Handle group communications for the project, including accepting connections and managing group membership.
There can only exist one group per server, and a group can have a maximum of 10 members. Each member is identified by their UID, which is an 8-character hexadecimal string.
*/
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "uid.h"
#include "group.h"