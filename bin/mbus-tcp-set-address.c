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

//------------------------------------------------------------------------------
// set primary address
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle = NULL;
    mbus_frame reply;
    char *host, *old_address_str, *xml_result;
    long port;
    int old_address, new_address;
    int ret;

    if (argc == 5)
    {
        host = argv[1];
        port = atol(argv[2]);
        old_address_str = argv[3];
        new_address = atoi(argv[4]);
        debug = 0;
    }
    else if (argc == 6 && strcmp(argv[1], "-d") == 0)
    {
        host = argv[2];
        port = atol(argv[3]);
        old_address_str = argv[4];
        new_address = atoi(argv[5]);
        debug = 1;
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] host port mbus-address new-primary-address\n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
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

    if ((handle = mbus_context_tcp(host, port)) == NULL)
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
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, old_address_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, old_address_str);
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
        return 1;
    }

    memset(&reply, 0, sizeof(mbus_frame));
    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        fprintf(stderr, "No reply from device\n");
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
