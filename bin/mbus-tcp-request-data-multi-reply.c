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
// Scan for devices using secondary addressing.
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_frame *frame, reply;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *host, *addr_str, matching_addr[16], *xml_result;
    int port, address;
 
    memset((void *)&reply, 0, sizeof(mbus_frame));
    memset((void *)&reply_data, 0, sizeof(mbus_frame_data));
     
    if (argc == 4)
    {
        host = argv[1];   
        port = atoi(argv[2]);
        addr_str = argv[3];
        debug = 0;
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0)
    {
        host = argv[2];   
        port = atoi(argv[3]);
        addr_str = argv[4];
        debug = 1;
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] host port mbus-address\n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
        return 0;
    }
    
    if (debug)
    {
        mbus_register_send_event(&mbus_dump_send_event);
        mbus_register_recv_event(&mbus_dump_recv_event);
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
    // init slave to get really the beginning of the records
    //
    if (debug)
        printf("%s: debug: sending init frame\n", __PRETTY_FUNCTION__);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_BROADCAST_NOREPLY, 1) == -1)
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
        
        address = MBUS_ADDRESS_NETWORK_LAYER;
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



