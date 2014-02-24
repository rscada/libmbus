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
int
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

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_BROADCAST_NOREPLY, 1) == -1)
    {
        return 0;
    }

    return 1;
}


//------------------------------------------------------------------------------
// Scan for devices using secondary addressing.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    char *device, *addr_mask = NULL;
    long baudrate = 9600;
    mbus_handle *handle = NULL;
    mbus_frame *frame = NULL, reply;

    memset((void *)&reply, 0, sizeof(mbus_frame));

    if (argc == 2)
    {
        device = argv[1];
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
    }
    else if (argc == 3 && strcmp(argv[1], "-d") == 0)
    {
        device = argv[2];
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
        debug = 1;
    }
    else if (argc == 3)
    {
        device = argv[1];
        addr_mask = strdup(argv[2]);
    }
    else if (argc == 4 && strcmp(argv[1], "-d") == 0)
    {
        device = argv[2];
        addr_mask = strdup(argv[3]);
        debug = 1;
    }
    else if (argc == 4 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atol(argv[2]);
        device = argv[3];
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-b") == 0)
    {
        baudrate = atol(argv[3]);
        device = argv[4];
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
        debug = 1;
    }
    else if (argc == 5 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atol(argv[2]);
        device = argv[3];
        addr_mask = strdup(argv[4]);
    }
    else if (argc == 6 && strcmp(argv[1], "-d") == 0)
    {
        baudrate = atol(argv[3]);
        device = argv[4];
        addr_mask = strdup(argv[5]);
        debug = 1;
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] [-b BAUDRATE] device [address-mask]\n", argv[0]);
        fprintf(stderr, "\toptional flag -d for debug printout\n");
        fprintf(stderr, "\toptional flag -b for selecting baudrate\n");
        fprintf(stderr, "\trestrict the search by supplying an optional address mask on the form\n");
        fprintf(stderr, "\t'FFFFFFFFFFFFFFFF' where F is a wildcard character\n");
        return 0;
    }

    if (addr_mask == NULL)
    {
        fprintf(stderr, "Failed to allocate address mask.\n");
        return 1;
    }

    if (mbus_is_secondary_address(addr_mask) == 0)
    {
        fprintf(stderr, "Misformatted secondary address mask. Must be 16 character HEX number.\n");
        free(addr_mask);
        return 1;
    }

    if ((handle = mbus_context_serial(device)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        free(addr_mask);
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
        free(addr_mask);
        return 1;
    }

    if (mbus_serial_set_baudrate(handle, baudrate) == -1)
    {
        fprintf(stderr, "Failed to set baud rate.\n");
        free(addr_mask);
        return 1;
    }


    frame = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);

    if (frame == NULL)
    {
        fprintf(stderr, "Failed to allocate mbus frame.\n");
        free(addr_mask);
        return 1;
    }

    if (init_slaves(handle) == 0)
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

