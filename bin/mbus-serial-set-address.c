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
    mbus_frame reply;
    char *device, *old_address_str, *xml_result;
    int old_address, new_address;
    long baudrate = 9600;
    int ret;

    if (argc == 4)
    {
        device   = argv[1];
        old_address_str = argv[2];
        new_address = atoi(argv[3]);
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0)
    {
        device   = argv[2];
        old_address_str = argv[3];
        new_address = atoi(argv[4]);
        debug = 1;
    }
    else if (argc == 6 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atol(argv[2]);
        device = argv[3];
        old_address_str = argv[4];
        new_address = atoi(argv[5]);
    }
    else if (argc == 7 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-b") == 0)
    {
        baudrate = atol(argv[3]);
        device = argv[4];
        old_address_str = argv[5];
        new_address = atoi(argv[6]);
        debug = 1;
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] [-b BAUDRATE] device mbus-address new-primary-address\n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
        fprintf(stderr, "    optional flag -b for selecting baudrate\n");
        return 0;
    }

    if (mbus_is_primary_address(new_address) == 0)
    {
        fprintf(stderr, "Invalid new primary address\n");
        return -1;
    }

    switch (new_address)
    {
        case MBUS_ADDRESS_NETWORK_LAYER:
        case MBUS_ADDRESS_BROADCAST_REPLY:
        case MBUS_ADDRESS_BROADCAST_NOREPLY:
            fprintf(stderr, "Invalid new primary address\n");
            return -1;
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

    if (mbus_send_ping_frame(handle, new_address, 0) == -1)
    {
        fprintf(stderr, "Verification failed. Could not send ping frame: %s\n", mbus_error_str());
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_TIMEOUT)
    {
        fprintf(stderr, "Verification failed. Got a response from new address\n");
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_is_secondary_address(old_address_str))
    {
        // secondary addressing

        ret = mbus_select_secondary_address(handle, old_address_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, old_address_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, old_address_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, old_address_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        // else MBUS_PROBE_SINGLE

        old_address = MBUS_ADDRESS_NETWORK_LAYER;
    }
    else
    {
        // primary addressing
        old_address = atoi(old_address_str);
    }

    if (mbus_set_primary_address(handle, old_address, new_address) == -1)
    {
        fprintf(stderr, "Failed to send set primary address frame: %s\n", mbus_error_str());
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    memset(&reply, 0, sizeof(mbus_frame));
    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        fprintf(stderr, "No reply from device\n");
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }
    else if (mbus_frame_type(&reply) != MBUS_FRAME_TYPE_ACK)
    {
        fprintf(stderr, "Unknown reply:\n");
        mbus_frame_print(&reply);
    }
    else
    {
        printf("Set primary address of device to %d\n", new_address);
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}
