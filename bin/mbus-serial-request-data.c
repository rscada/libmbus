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

static int debug = 0;

//------------------------------------------------------------------------------
// Scan for devices using secondary addressing.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_frame reply;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *device, *addr_str, matching_addr[16];
    int address, baudrate = 9600;

    if (argc == 3)
    {
        device   = argv[1];   
        addr_str = argv[2];
    }
    else if (argc == 4 && strcmp(argv[1], "-d") == 0)
    {
        device   = argv[2];   
        addr_str = argv[3];
        debug = 1;
    }
    else if (argc == 5 && strcmp(argv[1], "-b") == 0)
    {
        baudrate = atoi(argv[2]);
        device   = argv[3];   
        addr_str = argv[4];
    }
    else if (argc == 6 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-b") == 0)
    {
        baudrate = atoi(argv[3]);
        device   = argv[4];   
        addr_str = argv[5];
        debug = 1;
    }
    else 
    {
        fprintf(stderr, "usage: %s [-d] [-b BAUDRATE] device mbus-address\n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
        fprintf(stderr, "    optional flag -b for selecting baudrate\n");
        return 0;
    }
 
    if ((handle = mbus_connect_serial(device)) == NULL)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_serial_set_baudrate(handle->m_serial_handle, baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }


    if (strlen(addr_str) == 16)
    {
        // secondary addressing

        int probe_ret;

        probe_ret = mbus_probe_secondary_address(handle, addr_str, matching_addr);

        if (probe_ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (probe_ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (probe_ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to probe secondary address [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;    
        }
        // else MBUS_PROBE_SINGLE

        if (mbus_send_request_frame(handle, 253) == -1)
        {
            fprintf(stderr, "Failed to send M-Bus request frame.\n");
            return 1;
        }
    } 
    else
    {
        // primary addressing

        address = atoi(addr_str);
        if (mbus_send_request_frame(handle, address) == -1)
        {
            fprintf(stderr, "Failed to send M-Bus request frame.\n");
            return 1;
        }
    }   

    if (mbus_recv_frame(handle, &reply) == -1)
    {
        fprintf(stderr, "Failed to receive M-Bus response frame.\n");
        return 1;
    }    

    //
    // parse data and print in XML format
    //
    if (debug)
    {
        mbus_frame_print(&reply);
    }

    if (mbus_frame_data_parse(&reply, &reply_data) == -1)
    {
        fprintf(stderr, "M-bus data parse error.\n");
        return 1;
    }

    printf("%s", mbus_frame_data_xml(&reply_data));

    // manual free
    if (reply_data.data_var.record)
    {
        mbus_data_record_free(reply_data.data_var.record); // free's up the whole list
    }

    mbus_disconnect(handle);
    return 0;
}



