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

// Default value for the maximum number of frames
#define MAXFRAMES 16
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
// Wrapper for argument parsing errors
//------------------------------------------------------------------------------
static void
parse_abort(char **argv)
{
    fprintf(stderr, "usage: %s [-d] [-f FRAMES] host port mbus-address\n", argv[0]);
    fprintf(stderr, "    optional flag -d for debug printout\n");
    fprintf(stderr, "    optional flag -f for selecting the maximal number of frames\n");
    exit(1);
}

//------------------------------------------------------------------------------
// Scan for devices using secondary addressing.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_frame reply;
    mbus_handle *handle = NULL;

    char *host, *addr_str, *xml_result;
    int address;
    long port;
    int maxframes = MAXFRAMES;
    int c = 0;

    memset((void *)&reply, 0, sizeof(mbus_frame));

    for (c=1; c<argc-3; c++)
    {
        if (strcmp(argv[c], "-d") == 0)
        {
            debug = 1;
        }
        else if (strcmp(argv[c], "-f") == 0)
        {
            c++;
            maxframes = atoi(argv[c]);
        }
        else
        {
            parse_abort(argv);
        }
    }
    if (c > argc-3)
    {
        parse_abort(argv);
    }
    host     = argv[c];
    port     = atol(argv[c+1]);
    addr_str = argv[c+2];

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
        fprintf(stderr,"Failed to setup connection to M-bus gateway\n");
        mbus_context_free(handle);
        return 1;
    }

    if (init_slaves(handle) == 0)
    {
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
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, addr_str);
            mbus_disconnect(handle);
            mbus_context_free(handle);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, addr_str);
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
        address = atoi(addr_str);
    }

    // instead of the send and recv, use this sendrecv function that
    // takes care of the possibility of multi-telegram replies (limit = 16 frames)
    if (mbus_sendrecv_request(handle, address, &reply, maxframes) != 0)
    {
        fprintf(stderr, "Failed to send/receive M-Bus request.\n");
        mbus_disconnect(handle);
        mbus_context_free(handle);
        mbus_frame_free(reply.next);
        return 1;
    }

    //
    // dump hex data if debug is true
    //
    if (debug)
    {
        mbus_frame_print(&reply);
    }

    //
    // generate XML and print to standard output
    //
    if ((xml_result = mbus_frame_xml(&reply)) == NULL)
    {
        fprintf(stderr, "Failed to generate XML representation of MBUS frames: %s\n", mbus_error_str());
        mbus_disconnect(handle);
        mbus_context_free(handle);
        mbus_frame_free(reply.next);
        return 1;
    }

    printf("%s", xml_result);
    free(xml_result);

    mbus_disconnect(handle);
    mbus_context_free(handle);
    mbus_frame_free(reply.next);

    return 0;
}



