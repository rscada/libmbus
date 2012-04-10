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

//------------------------------------------------------------------------------
// Scan for devices using secondary addressing.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    char *device, *addr_mask;
    int baudrate = 9600;
    mbus_handle *handle = NULL;

    if (argc == 2)
    {
        device = argv[1];   
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
    }
    else if (argc == 3)
    {
        device = argv[1];   
        addr_mask = strdup(argv[2]);
    }
    else if (argc == 4 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atoi(argv[2]);
        device = argv[3];   
        addr_mask = strdup("FFFFFFFFFFFFFFFF");
    }
    else if (argc == 5 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atoi(argv[2]);
        device = argv[3];   
        addr_mask = strdup(argv[4]);
    }
    else 
    {
        fprintf(stderr, "usage: %s [-b BAUDRATE] device [address-mask]\n", argv[0]);
        fprintf(stderr, "\trestrict the search by supplying an optional address mask on the form\n");
        fprintf(stderr, "\t'FFFFFFFFFFFFFFFF' where F is a wildcard character\n");
        return 0;
    }
 
    if (strlen(addr_mask) != 16)
    {
        fprintf(stderr, "Misformatted secondary address mask. Must be 16 character HEX number.\n");
        return 1;
    }

    if ((handle = mbus_connect_serial(device)) == NULL)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway: %s\n", mbus_error_str());
        return 1;
    }

    if (mbus_serial_set_baudrate(handle->m_serial_handle, baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }

    mbus_scan_2nd_address_range(handle, 0, addr_mask);

    mbus_disconnect(handle);

    //printf("Summary: Tried %ld address masks and found %d devices.\n", probe_count, match_count);

    free(addr_mask);

    return 0;
}

