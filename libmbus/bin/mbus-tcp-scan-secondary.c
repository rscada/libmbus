//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <mbus/mbus.h>

static mbus_handle *handle = NULL;


//------------------------------------------------------------------------------
// Execution starts here:
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    char *host, *addr_mask;
    int port;

    if (argc != 4 && argc != 3)
    {
        fprintf(stderr, "usage: %s host port [address-mask]\n", argv[0]);
        fprintf(stderr, "\trestrict the search by supplying an optional address mask on the form\n");
        fprintf(stderr, "\t'FFFFFFFFFFFFFFFF' where F is a wildcard character\n");
        return 0;
    }
 
    host = argv[1];   
    port = atoi(argv[2]);
    if (argc == 4)
    {
        addr_mask = strdup(argv[3]);
    }
    else
    {
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
    }

    if (strlen(addr_mask) != 16)
    {
        fprintf(stderr, "Misformatted secondary address mask. Must be 16 character HEX number.\n");
        return 1;
    }

    if ((handle = mbus_connect_tcp(host, port)) == NULL)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway: %s\n", mbus_error_str());
        return 1;
    }

    mbus_scan_2nd_address_range(handle, 0, addr_mask);

    mbus_disconnect(handle);

    //printf("Summary: Tried %ld address masks and found %d devices.\n", probe_count, match_count);

    free(addr_mask);

    return 0;
}

