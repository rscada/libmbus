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

int ping_address(mbus_handle *handle, mbus_frame *reply, int address)
{
    int i, ret = MBUS_RECV_RESULT_ERROR;

    memset((void *)reply, 0, sizeof(mbus_frame));

    for (i = 0; i <= handle->max_search_retry; i++)
    {
        if (debug)
        {
            printf("%d ", address);
            fflush(stdout);
        }

        if (mbus_send_ping_frame(handle, address, 0) == -1)
        {
            fprintf(stderr,"Scan failed. Could not send ping frame: %s\n", mbus_error_str());
            return MBUS_RECV_RESULT_ERROR;
        }

        ret = mbus_recv_frame(handle, reply);

        if (ret != MBUS_RECV_RESULT_TIMEOUT)
        {
            return ret;
        }
    }

    return ret;
}

//------------------------------------------------------------------------------
// Primary addressing scanning of mbus devices.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_handle *handle;
    char *host;
    int address, retries = 0;
    long port;
    int ret;

    if (argc == 3)
    {
        host = argv[1];
        port = atol(argv[2]);
    }
    else if (argc == 4 && strcmp(argv[1], "-d") == 0)
    {
        debug = 1;
        host = argv[2];
        port = atol(argv[3]);
    }
    else if (argc == 5 && strcmp(argv[1], "-r") == 0)
    {
        retries = atoi(argv[2]);
        host = argv[3];
        port = atol(argv[4]);
    }
    else if (argc == 6 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-r") == 0)
    {
        debug = 1;
        retries = atoi(argv[3]);
        host = argv[4];
        port = atol(argv[5]);
    }
    else
    {
        fprintf(stderr,"usage: %s [-d] [-r RETRIES] host port\n", argv[0]);
        return 0;
    }

    if ((port < 0) || (port > 0xFFFF))
    {
        fprintf(stderr, "Invalid port: %ld\n", port);
        return 1;
    }

    if ((handle = mbus_context_tcp(host, port)) == NULL)
    {
        fprintf(stderr,"Scan failed: Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }
    
    if (debug)
    {
        mbus_register_send_event(handle, &mbus_dump_send_event);
        mbus_register_recv_event(handle, &mbus_dump_recv_event);
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr,"Scan failed: Could not setup connection to M-bus gateway: %s\n", mbus_error_str());
        return 1;
    }

    if (mbus_context_set_option(handle, MBUS_OPTION_MAX_SEARCH_RETRY, retries) == -1)
    {
        fprintf(stderr,"Failed to set retry count\n");
        return 1;
    }

    if (debug)
        printf("Scanning primary addresses:\n");

    for (address = 0; address <= MBUS_MAX_PRIMARY_SLAVES; address++)
    {
        mbus_frame reply;

        ret = ping_address(handle, &reply, address);

        if (ret == MBUS_RECV_RESULT_TIMEOUT)
        {
            continue;
        }

        if (ret == MBUS_RECV_RESULT_INVALID)
        {
            /* check for more data (collision) */
            mbus_purge_frames(handle);
            printf("Collision at address %d\n", address);
            continue;
        }

        if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
        {
            /* check for more data (collision) */
            if (mbus_purge_frames(handle))
            {
                printf("Collision at address %d\n", address);
                continue;
            }

            printf("Found a M-Bus device at address %d\n", address);
        }
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}

