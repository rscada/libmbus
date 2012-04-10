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
// Primary addressing scanning of mbus devices.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle;
    char *device;
    int address, baudrate = 9600;

    if (argc == 2)
    {
        device = argv[1];
    }
    else if (argc == 4 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atoi(argv[2]); 
        device = argv[3];
    }
    else
    {
        fprintf(stderr, "usage: %s [-b BAUDRATE] device\n", argv[0]);
        return 0;
    }
    
    if ((handle = mbus_connect_serial(device)) == NULL)
    {
        printf("Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_serial_set_baudrate(handle->m_serial_handle, baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }


    for (address = 0; address < 254; address++)
    {
        mbus_frame reply;

        memset((void *)&reply, 0, sizeof(mbus_frame));

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

