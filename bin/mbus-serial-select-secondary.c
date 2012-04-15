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
    mbus_frame reply;
    char *device, *addr;
    int ret, baudrate = 9600;

    if (argc == 3)
    {
        device = argv[1];   
        addr = strdup(argv[2]);
    }   
    else if (argc == 5 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atoi(argv[2]);
        device = argv[3];   
        addr = strdup(argv[4]);
    }
    else
    {
        fprintf(stderr, "usage: %s device secondary-mbus-address\n", argv[0]);
        return 0;
    }
 
    if (strlen(addr) != 16)
    {
        printf("Misformatted secondary address. Must be 16 character HEX number.\n");
        return 1;
    }

    if ((handle = mbus_connect_serial(device)) == NULL)
    {
        printf("Failed to setup connection to M-bus device: %s\n", mbus_error_str());
        return 1;
    }

    if (mbus_serial_set_baudrate(handle->m_serial_handle, baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }

    if (mbus_send_select_frame(handle, addr) == -1)
    {
        printf("Failed to send selection frame: %s\n", mbus_error_str());
        return 1; 
    }

    ret = mbus_recv_frame(handle, &reply);

    if (ret == -1)
    {
        printf("No reply from device with secondary address %s: %s\n", argv[2], mbus_error_str());
        return 1;
    }    

    if (ret == -2)
    {
        printf("Invalid reply from %s: The address address probably match more than one device: %s\n", argv[2], mbus_error_str());
        return 1;
    }

    if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
    {
        if (mbus_send_request_frame(handle, 253) == -1)
	    {
            printf("Failed to send request to selected secondary device: %s\n", mbus_error_str());
            return 1;
        }

        if (mbus_recv_frame(handle, &reply) == -1)
        {
            printf("Failed to recieve reply from selected secondary device: %s\n", mbus_error_str());
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
        printf("Unknown reply:\n");
        mbus_frame_print(&reply);
    }
 
    free(addr);
    mbus_disconnect(handle);
    return 0;
}

