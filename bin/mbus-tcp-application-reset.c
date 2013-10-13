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
// Execution starts here:
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle;
    mbus_frame reply;
    char *host, *addr = NULL;
    int ret;
    long port;
    int address, subcode = -1;

    if (argc == 4)
    {
        host     = argv[1];
        port     = atol(argv[2]);
        addr     = argv[3];
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0)
    {
        debug    = 1;
        host     = argv[2];
        port     = atol(argv[3]);
        addr     = argv[4];
    }
    else if (argc == 5)
    {
        host     = argv[1];
        port     = atol(argv[2]);
        addr     = argv[3];
        subcode  = atoi(argv[4]);
    }
    else if (argc == 6 && strcmp(argv[1], "-d") == 0)
    {
        debug    = 1;
        host     = argv[2];
        port     = atol(argv[3]);
        addr     = argv[4];
        subcode  = atoi(argv[5]);
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] host port mbus-address [subcode]\n", argv[0]);
        return 0;
    }

    if ((port < 0) || (port > 0xFFFF))
    {
        fprintf(stderr, "Invalid port: %ld\n", port);
        return 1;
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
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_is_secondary_address(addr))
    {
        // secondary addressing

        int ret;

        ret = mbus_select_secondary_address(handle, addr);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "The address mask [%s] matches more than one device.\n", addr);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "The selected secondary address does not match any device [%s].\n", addr);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "Failed to select secondary address [%s].\n", addr);
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

        address = atoi(addr);
    }

    if (mbus_send_application_reset_frame(handle, address, subcode) == -1)
    {
        fprintf(stderr,"Failed to send reset frame: %s\n", mbus_error_str());
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        fprintf(stderr,"No reply from device\n");
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }
    else if (mbus_frame_type(&reply) != MBUS_FRAME_TYPE_ACK)
    {
        fprintf(stderr,"Unknown reply:\n");
        mbus_frame_print(&reply);
    }
    else
    {
        if (subcode > -1)
        {
            printf("Successful reset device (Subcode: %d)\n", subcode);
        }
        else
        {
            printf("Successful reset device\n");
        }
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}

