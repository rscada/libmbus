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
    int port, address, debug = 0;

    if (argc == 3)
    {
        host = argv[1];   
        port = atoi(argv[2]);
    }
    else if (argc == 4 && strcmp(argv[1], "-d") == 0)
    {
        debug = 1;
        host = argv[2];   
        port = atoi(argv[3]);
    }
    else
    {
        printf("usage: %s [-d] host port\n", argv[0]);
        return 0;
    }
    
    if (debug)
    {
        mbus_register_send_event(&mbus_dump_send_event);
        mbus_register_recv_event(&mbus_dump_recv_event);
    }
     
    if ((handle = mbus_connect_tcp(host, port)) == NULL)
    {
        printf("Scan failed: Could not setup connection to M-bus gateway: %s\n",  mbus_error_str());
        return 1;
    }

    if (debug)
        printf("Scanning primary addresses:\n");
        
    for (address = 0; address < 254; address++)
    {
        mbus_frame reply;

        if (debug)
        {
            printf("%d ", address);
            fflush(stdout);
        }

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
            if (debug)
                printf("\n");
                
            printf("Found a M-Bus device at address %d\n", address);
        }
    }

    mbus_disconnect(handle);
    return 0;
}

