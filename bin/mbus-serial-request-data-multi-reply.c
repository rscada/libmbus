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
        
    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1)
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
    mbus_frame *frame, reply;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *device, *addr_str, *xml_result;
    int address, baudrate = 9600;
 
    memset((void *)&reply, 0, sizeof(mbus_frame));
    memset((void *)&reply_data, 0, sizeof(mbus_frame_data));
     
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
    
    if (debug)
    {
        mbus_register_send_event(&mbus_dump_send_event);
        mbus_register_recv_event(&mbus_dump_recv_event);
    }
 
    if ((handle = mbus_context_serial(device)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (mbus_connect(handle) == -1)
    {
        printf("Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_serial_set_baudrate(handle, baudrate) == -1)
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }
    
    if (init_slaves(handle) == 0)
    {
        return 1;
    }

    if (strlen(addr_str) == 16)
    {
        // secondary addressing

        int ret;

        ret = mbus_select_secondary_address(handle, addr_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;    
        }
        // else MBUS_PROBE_SINGLE

        address = 253;
    } 
    else
    {
        // primary addressing
        address = atoi(addr_str);
    }   
    
    // instead of the send and recv, use this sendrecv function that 
    // takes care of the possibility of multi-telegram replies (limit = 16 frames)
    if (mbus_sendrecv_request(handle, address, &reply, 16) != 0)
    {
        fprintf(stderr, "Failed to send/receive M-Bus request.\n");
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
        return 1;
    }
    
    printf("%s", xml_result);
    free(xml_result);

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}



