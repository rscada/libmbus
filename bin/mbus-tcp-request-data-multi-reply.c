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
// Execution starts here:
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_frame reply, *reply_iter;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *host, *addr_str, matching_addr[16];
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
    
    if ((handle = mbus_connect_tcp(host, port)) == NULL)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
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
        
        address = 253;
    } 
    else
    {
        // primary addressing
        address = atoi(addr_str);
    }   

    /*
    if (mbus_send_request_frame(handle, address) == -1)
    {
        fprintf(stderr, "Failed to send M-Bus request frame.\n");
        return 1;
    }

    if (mbus_recv_frame(handle, &reply) == -1)
    {
        fprintf(stderr, "Failed to receive M-Bus response frame.\n");
        return 1;
    }    
    */
    
    // instead of the send and recv, use this sendrecv function that 
    // takes care of the possibility of multi-telegram replies (limit = 16 frames)
    if (mbus_sendrecv_request(handle, address, &reply, 16) == -1)
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
    // here, figure out how the list of frames should be merged into a single
    // XML document, but for now let's just dump all frames as independent XMLs
    //
    for (reply_iter = &reply; reply_iter; reply_iter = reply_iter->next)
    {
        if (mbus_frame_data_parse(reply_iter, &reply_data) == -1)
        {
            fprintf(stderr, "M-bus data parse error.\n");
            return 1;
        }
        printf("%s", mbus_frame_data_xml(&reply_data));

        // manual free, all records in the list
        if (reply_data.data_var.record)
        {
            mbus_data_record_free(reply_data.data_var.record); 
        }
    }

    mbus_disconnect(handle);
    return 0;
}



