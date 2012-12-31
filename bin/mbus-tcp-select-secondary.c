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
    mbus_frame reply;
    char *host, *addr = NULL;
    int port, ret;

    if (argc != 4)
    {
        printf("usage: %s host port secondary-mbus-address\n", argv[0]);
        return 0;
    }
 
    host = argv[1];   
    port = atoi(argv[2]);
    
    if ((addr = strdup(argv[3])) == NULL)
    {
        fprintf(stderr, "Failed to allocate address.\n");
        return 1;
    }

    if (mbus_is_secondary_address(addr) == 0)
    {
        printf("Misformatted secondary address. Must be 16 character HEX number.\n");
        return 1;
    }

    if ((handle = mbus_context_tcp(host, port)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_send_select_frame(handle, addr) == -1)
    {
        printf("Failed to send selection frame: %s\n", mbus_error_str());
        return 1; 
    }

    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        printf("No reply from device with secondary address %s: %s\n", argv[3], mbus_error_str());
        return 1;
    }    

    if (ret == MBUS_RECV_RESULT_INVALID)
    {
        printf("Invalid reply from %s: The address address probably match more than one device: %s\n", argv[3], mbus_error_str());
        return 1;
    }

    if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
    {
        if (mbus_send_request_frame(handle, MBUS_ADDRESS_NETWORK_LAYER) == -1)
	    {
            printf("Failed to send request to selected secondary device: %s\n", mbus_error_str());
            return 1;
        }

        if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_OK)
        {
            printf("Failed to recieve reply from selected secondary device: %s\n", mbus_error_str());
            return 1;
        }

        if (mbus_frame_type(&reply) != MBUS_FRAME_TYPE_ACK)
        {
            printf("Recieved a reply from secondarily addressed device: Searched for [%s] and found [%s].\n",
                   argv[3], mbus_frame_get_secondary_address(&reply));
        }
    }
    else
    {
        printf("Unknown reply:\n");
        mbus_frame_print(&reply);
    }
 
    free(addr);
    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}

