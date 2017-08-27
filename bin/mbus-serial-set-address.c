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

static int debug = 0;

//
// init slave to get really the beginning of the records
//
static int
init_slaves(mbus_handle *handle)
{
    if (debug)
        printf("%s: debug: sending init frame #1\n", __PRETTY_FUNCTION__);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1)
    {
        return 0;
    }

    //
    // resend SND_NKE, maybe the first get lost
    //

    if (debug)
        printf("%s: debug: sending init frame #2\n", __PRETTY_FUNCTION__);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1)
    {
        return 0;
    }

    return 1;
}

//------------------------------------------------------------------------------
// set primary address 
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle = NULL;
    char *device, *o_addr_str, *n_addr_str, *xml_result;
    int address;
    long baudrate = 9600;

    if (argc == 4)
    {
        device   = argv[1];
        o_addr_str = argv[2];
        n_addr_str = argv[3];
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0)
    {
        device   = argv[2];
        o_addr_str = argv[3];
        n_addr_str = argv[4];
        debug = 1;
    }
    else if (argc == 6 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atol(argv[2]);
        device = argv[3];
        o_addr_str = argv[4];
        n_addr_str = argv[5];
    }
    else if (argc == 7 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-b") == 0)
    {
        baudrate = atol(argv[3]);
        device = argv[4];
        o_addr_str = argv[5];
        n_addr_str = argv[6];
        debug = 1;
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] [-b BAUDRATE] device mbus-serial-number new-id\n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
        fprintf(stderr, "    optional flag -b for selecting baudrate\n");
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
        fprintf(stderr,"Failed to setup connection to M-bus gateway\n");
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_serial_set_baudrate(handle, baudrate) == -1)
    {
        fprintf(stderr,"Failed to set baud rate.\n");
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    if (init_slaves(handle) == 0)
    {
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_is_secondary_address(n_addr_str))
    {
        fprintf(stderr, "the new id has to be a primary id!\n");
        return 1;
    }

    if (mbus_is_secondary_address(o_addr_str))
    {
        // secondary addressing

        int ret;

        ret = mbus_select_secondary_address(handle, o_addr_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, o_addr_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, o_addr_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, o_addr_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        // else MBUS_PROBE_SINGLE
        
        address = MBUS_ADDRESS_NETWORK_LAYER;
    }
    else
    {
        // primary addressing
        address = atoi(o_addr_str);
    }

    unsigned char buffer[3] = {0x01, 0x7A, atoi(n_addr_str)};
    mbus_send_user_data_frame(handle, address, buffer, 3);

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}
