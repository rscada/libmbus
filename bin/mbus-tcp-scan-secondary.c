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
    char *host, *addr_mask = NULL;
    long port;
    mbus_handle *handle = NULL;
    mbus_frame *frame = NULL, reply;

    memset((void *)&reply, 0, sizeof(mbus_frame));

    if (argc != 4 && argc != 3)
    {
        fprintf(stderr, "usage: %s host port [address-mask]\n", argv[0]);
        fprintf(stderr, "\trestrict the search by supplying an optional address mask on the form\n");
        fprintf(stderr, "\t'FFFFFFFFFFFFFFFF' where F is a wildcard character\n");
        return 0;
    }

    host = argv[1];
    port = atol(argv[2]);
    if (argc == 4)
    {
        addr_mask = strdup(argv[3]);
    }
    else
    {
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
    }

    if (addr_mask == NULL)
    {
        fprintf(stderr, "Failed to allocate address mask.\n");
        return 1;
    }

    if ((port < 0) || (port > 0xFFFF))
    {
        fprintf(stderr, "Invalid port: %ld\n", port);
        return 1;
    }

    if (mbus_is_secondary_address(addr_mask) == 0)
    {
        fprintf(stderr, "Misformatted secondary address mask. Must be 16 character HEX number.\n");
        return 1;
    }

    if ((handle = mbus_context_tcp(host, port)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    //
    // init slaves
    //
    if (debug)
        printf("%s: debug: sending init frame #1\n", __PRETTY_FUNCTION__);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1)
    {
        free(addr_mask);
        return 1;
    }

    //
    // resend SND_NKE, maybe the first get lost
    //
    if (debug)
        printf("%s: debug: sending init frame #2\n", __PRETTY_FUNCTION__);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_BROADCAST_NOREPLY, 1) == -1)
    {
        free(addr_mask);
        return 1;
    }

    mbus_scan_2nd_address_range(handle, 0, addr_mask);

    mbus_disconnect(handle);
    mbus_context_free(handle);

    //printf("Summary: Tried %ld address masks and found %d devices.\n", probe_count, match_count);

    free(addr_mask);

    return 0;
}

