//------------------------------------------------------------------------------
// Copyright (C) 2010-2012, Robert Johansson and contributors, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
// Contributors:
// Large parts of this file was contributed by Stefan Wahren.
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
    char *device;
    int address, ret;
    int source_baudrate = 9600, target_baudrate;

    if (argc == 4)
    {
        device = argv[1];
        address = atoi(argv[2]);
        target_baudrate = atoi(argv[3]);
    }
    else if (argc == 6 && strcmp(argv[1], "-b") == 0)
    {
        source_baudrate = atoi(argv[2]);
        device = argv[3];
        address = atoi(argv[4]);
        target_baudrate = atoi(argv[5]);
    }
    else
    {
        fprintf(stderr, "usage: %s device address target-baudrate\n", argv[0]);
        return 0;
    }

    if ((handle = mbus_connect_serial(device)) == NULL)
    {
        printf("Failed to setup connection to M-bus device: %s\n", mbus_error_str());
        return 1;
    }
    
    if (mbus_serial_set_baudrate(handle->m_serial_handle, source_baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }

    if (mbus_send_switch_baudrate_frame(handle, address, target_baudrate) == -1)
    {
        printf("Failed to send switch baudrate frame: %s\n", mbus_error_str());
        return 1; 
    }

    ret = mbus_recv_frame(handle, &reply);  
    
    if (ret == -1)
    {
        printf("No reply from device\n");
        return 1;
    }

    if (mbus_frame_type(&reply) != MBUS_FRAME_TYPE_ACK)
    {
        printf("Unknown reply:\n");
        mbus_frame_print(&reply);
    }
    
    mbus_disconnect(handle);
    return 0;
}

