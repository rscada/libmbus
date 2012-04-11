//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <mbus/mbus.h>


//------------------------------------------------------------------------------
// Execution starts here:
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle;
    char *host;
    int port, address;

    if (argc != 3)
    {
        printf("usage: %s host port\n", argv[0]);
        return 0;
    }
 
    host = argv[1];   
    port = atoi(argv[2]);
    
    if ((handle = mbus_connect_tcp(host, port)) == NULL)
    {
        printf("Scan failed: Could not setup connection to M-bus gateway: %s\n",  mbus_error_str());
        return 1;
    }

    for (address = 0; address < 254; address++)
    {
        mbus_frame reply;

        if (mbus_send_ping_frame(handle, address) == -1)
        {
            printf("Scan failed. Could not send ping frame: %s\n", mbus_error_str());
            return 1;
        } 

        if (mbus_recv_frame(handle, &reply) == -1)
        {
            continue;
        }    

        if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
        {
            printf("Found a M-Bus device at address %d\n", address);
        }
    }

    mbus_disconnect(handle);
    return 0;
}

