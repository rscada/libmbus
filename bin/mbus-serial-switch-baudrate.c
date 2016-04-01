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
    long source_baudrate = 9600, target_baudrate;

    if (argc == 4)
    {
        device = argv[1];
        address = atoi(argv[2]);
        target_baudrate = atol(argv[3]);
    }
    else if (argc == 6 && strcmp(argv[1], "-b") == 0)
    {
        source_baudrate = atol(argv[2]);
        device = argv[3];
        address = atoi(argv[4]);
        target_baudrate = atol(argv[5]);
    }
    else
    {
        fprintf(stderr, "usage: %s [-b BAUDRATE] device address target-baudrate\n", argv[0]);
        return 0;
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

    if (mbus_serial_set_baudrate(handle, source_baudrate) == -1)
    {
        fprintf(stderr,"Failed to set baud rate.\n");
        return 1;
    }

    if (mbus_send_switch_baudrate_frame(handle, address, target_baudrate) == -1)
    {
        fprintf(stderr,"Failed to send switch baudrate frame: %s\n", mbus_error_str());
        return 1;
    }

    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        fprintf(stderr,"No reply from device\n");
        return 1;
    }
    else if (mbus_frame_type(&reply) != MBUS_FRAME_TYPE_ACK)
    {
        fprintf(stderr,"Unknown reply:\n");
        mbus_frame_print(&reply);
    }
    else
    {
        printf("Switched baud rate of device to %ld\n", target_baudrate);
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}

