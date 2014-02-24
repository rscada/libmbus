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
    char *device, *addr = NULL;
    int ret;
    long baudrate = 9600;

    if (argc == 3)
    {
        device = argv[1];
        addr = strdup(argv[2]);
    }
    else if (argc == 5 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atol(argv[2]);
        device = argv[3];
        addr = strdup(argv[4]);
    }
    else
    {
        fprintf(stderr, "usage: %s [-b BAUDRATE] device secondary-mbus-address\n", argv[0]);
        fprintf(stderr, "    optional flag -b for selecting baudrate\n");
        return 0;
    }

    if (addr == NULL)
    {
        fprintf(stderr, "Failed to allocate address.\n");
        return 1;
    }

    if (mbus_is_secondary_address(addr) == 0)
    {
        fprintf(stderr,"Misformatted secondary address. Must be 16 character HEX number.\n");
        return 1;
    }

    if ((handle = mbus_context_serial(device)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr,"Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_serial_set_baudrate(handle, baudrate) == -1)
    {
        fprintf(stderr,"Failed to set baud rate.\n");
        return 1;
    }

    if (mbus_send_select_frame(handle, addr) == -1)
    {
        fprintf(stderr,"Failed to send selection frame: %s\n", mbus_error_str());
        return 1;
    }

    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        fprintf(stderr,"No reply from device with secondary address %s: %s\n", argv[2], mbus_error_str());
        return 1;
    }

    if (ret == MBUS_RECV_RESULT_INVALID)
    {
        fprintf(stderr,"Invalid reply from %s: The address address probably match more than one device: %s\n", argv[2], mbus_error_str());
        return 1;
    }

    if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
    {
        if (mbus_send_request_frame(handle, MBUS_ADDRESS_NETWORK_LAYER) == -1)
        {
            fprintf(stderr,"Failed to send request to selected secondary device: %s\n", mbus_error_str());
            return 1;
        }

        if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_OK)
        {
            fprintf(stderr,"Failed to recieve reply from selected secondary device: %s\n", mbus_error_str());
            return 1;
        }

        if (mbus_frame_type(&reply) != MBUS_FRAME_TYPE_ACK)
        {
            printf("Recieved a reply from secondarily addressed device: Searched for [%s] and found [%s].\n",
                   argv[2], mbus_frame_get_secondary_address(&reply));
        }
    }
    else
    {
        fprintf(stderr,"Unknown reply:\n");
        mbus_frame_print(&reply);
    }

    free(addr);
    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}

