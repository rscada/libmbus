//------------------------------------------------------------------------------
// Copyright (C) 2012, Robert Johansson, Raditex AB
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

static int debug = 0;

//------------------------------------------------------------------------------
// Execution starts here:
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_frame reply, request;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *device, *addr_str, matching_addr[16], *text;
    long baudrate = 9600;
    int address, result;

    memset((void *)&reply, 0, sizeof(mbus_frame));
    memset((void *)&reply_data, 0, sizeof(mbus_frame_data));

    if (argc == 4)
    {
        addr_str = argv[1];
        device = argv[2];
        text = argv[3];
        debug = 0;
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0)
    {
        addr_str = argv[2];
        device = argv[3];
        text = argv[4];
        debug = 1;
    }
    else if (argc == 6 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atol(argv[2]);
        addr_str = argv[3];
        device = argv[4];
        text = argv[5];
        debug = 0;
    }
    else if (argc == 7 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-b") == 0)
    {
        baudrate = atol(argv[3]);
        addr_str = argv[4];
        device = argv[5];
        text = argv[6];
        debug = 1;
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] [-b BAUDRATE] mbus-address device text \n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
        return 0;
    }

    if ((handle = mbus_context_serial(device)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (debug)
    {
        mbus_register_send_event(handle, &mbus_dump_send_event);
        mbus_register_recv_event(handle, &mbus_dump_recv_event);
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n%s\n", mbus_error_str());
        return 1;
    }

    if (mbus_serial_set_baudrate(handle, baudrate) == -1)
    {
        fprintf(stderr, "%s: Error: Failed to set baud rate [%ld].\n", __PRETTY_FUNCTION__, baudrate);
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_is_secondary_address(addr_str))
    {
        // secondary addressing
        int ret;

        ret = mbus_select_secondary_address(handle, addr_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        address = MBUS_ADDRESS_NETWORK_LAYER;
    }
    else
    {
        // primary addressing
        address = atoi(addr_str);
    }

    //
    // send the request
    //
    if (mbus_send_custom_text(handle, address, text) == -1)
    {
        fprintf(stderr, "Of course there is some problem: %s\n", mbus_error_str());
        return 1;
    }

    //
    // this should be option:
    //
    if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_OK)
    {
        fprintf(stderr, "Failed to receive M-Bus response frame: %s\n", mbus_error_str());
        return 1;
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}
