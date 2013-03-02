//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

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
    int ret;

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
     
    if ((handle = mbus_context_tcp(host, port)) == NULL)
    {
        printf("Scan failed: Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (mbus_connect(handle) == -1)
    {
        printf("Scan failed: Could not setup connection to M-bus gateway: %s\n",  mbus_error_str());
        return 1;
    }

    if (debug)
        printf("Scanning primary addresses:\n");

    for (address = 0; address <= MBUS_MAX_PRIMARY_SLAVES; address++)
    {
        mbus_frame reply;

        memset((void *)&reply, 0, sizeof(mbus_frame));

        if (debug)
        {
            printf("%d ", address);
            fflush(stdout);
        }

        if (mbus_send_ping_frame(handle, address, 0) == -1)
        {
            printf("Scan failed. Could not send ping frame: %s\n", mbus_error_str());
            return 1;
        } 
        
        ret = mbus_recv_frame(handle, &reply);

        if (ret == MBUS_RECV_RESULT_TIMEOUT)
        {
            continue;
        }
        
        if (debug)
            printf("\n");
        
        if (ret == MBUS_RECV_RESULT_INVALID)
        {
            /* check for more data (collision) */
            mbus_purge_frames(handle);
            printf("Collision at address %d\n", address);
            continue;
        } 

        if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
        {
            /* check for more data (collision) */
            if (mbus_purge_frames(handle))
            {
                printf("Collision at address %d\n", address);
                continue;
            }

            printf("Found a M-Bus device at address %d\n", address);
        }
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}

