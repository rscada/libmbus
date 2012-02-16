//------------------------------------------------------------------------------
// Copyright (C) 2010, Raditex AB
// All rights reserved.
//
// FreeSCADA 
// http://www.FreeSCADA.com
// freescada@freescada.com
//
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <mbus.h>

static gboolean server_io(GIOChannel *gio, GIOCondition condition, gpointer data);
static GIOChannel *gio_server;
#define PACKET_BUFF_SIZE 128

static int addr = 0x01;

//------------------------------------------------------------------------------
// send a data request packet to from master to slave
//------------------------------------------------------------------------------
static gboolean
send_packet_timeout(gpointer data)
{
    static gchar *func_name = "send_packet_timer";
    u_char buff[256];
    int len, ret;

    mbus_frame *frame;
    
    frame = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);
    
    frame->control  = MBUS_CONTROL_MASK_SND_NKE | MBUS_CONTROL_MASK_DIR_M2S; // data request from master to slave
    frame->address  = addr; // address

    //if ((ret = mbus_frame_print(frame)) < 0)
    //{
    //    printf("%s: mbus_frame_print -> %d\n", __PRETTY_FUNCTION__, ret);
    //    return -1;
    //}
   
    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        printf("%s: mbus_frame_pack -> %d\n", __PRETTY_FUNCTION__, ret);
        mbus_frame_free(frame);
        return -1;
    }    

    if (fs_net_send(gio_server, buff, len) < len)
    { 
        g_message("%s: failed to write data to server", func_name);
    }    
	
    mbus_frame_free(frame);
    return FALSE; 
}

//------------------------------------------------------------------------------
// client_io:
//
//------------------------------------------------------------------------------
static gboolean
server_io(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    static gchar *func_name = "server_io";
    gchar buff[PACKET_BUFF_SIZE];
    gint len, len_tot = 0;
    int fd;
    FsNetBuffer *nbdata;

    bzero((void *)buff, sizeof(buff));

    mbus_frame *frame;
    mbus_frame_data frame_data;

    //
    // init
    //

    if ((fd = g_io_channel_unix_get_fd(gio)) < 0)
    {
       g_warning("%s: Failed to aquire socket fd.", func_name);
       return FALSE;
    }

    if (condition & (G_IO_HUP|G_IO_NVAL|G_IO_ERR))
    {
        exit(0);
    }

    if ((nbdata  = fs_net_buffer_get(gio)) == NULL)
    {
        g_warning("%s: Serious error, couldn't retrieve data buffer for GIO Channel.", func_name);
        return FALSE;
    }

    //
    // start reading data
    //
    frame = mbus_frame_new(MBUS_FRAME_TYPE_ANY);
    len_tot = 1;
    
    while (len > 0)
    {
        if ((len = fs_net_read_data_nonblocking(gio, nbdata, len_tot)) < 0)
        {
            if (len == -2 && nbdata->offset == 0)
            {
                // Return value -2 is a special return value that we use to indicate
                // that the read operation was interrupted due to unavailability of 
                // data (so we got an try-again error, preventing a blocking read).
                //
                // Restart the read operation later.
                mbus_frame_free(frame);
                return TRUE;
            }        
	
	        if (len == -1)
	        {
                fs_net_buffer_free(gio);
	            fs_net_socket_close(fd);
                exit(0);
            }
        }
        
        nbdata->offset = len_tot; // start next read at the correct offset
        len = mbus_parse(frame, nbdata->buff, len_tot);
       	len_tot += len;
    }
  
    if (len == -1)
    {
        mbus_frame_free(frame);
        return TRUE;
    }
  
    //
    // parse data and print in XML format
    //
    mbus_frame_data_parse(frame, &frame_data);
    printf("%s", mbus_frame_data_xml(&frame_data));
    
        
    //
    // Free and exit
    //
    
    mbus_frame_free(frame);
    exit(0);	

    return TRUE;
}

//------------------------------------------------------------------------------
// Execution starts here:
//
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    static gchar *func_name = "mbus_test1", *host;
    int port;

    GMainLoop *gloop;
 
    if (argc != 4)
    {
        g_message("%s: usage: %s host port mbus-address", func_name, argv[0]);
        return 0;
    }
 
    host = argv[1];   
    port = atoi(argv[2]);
    addr = atoi(argv[3]);
    
    if ((gio_server = fs_net_connection_setup(host, port, (void *)server_io, NULL)) == NULL)
    {
        g_warning("%s: Failed to setup connection to server.\n", func_name);
        return -1;
    }

    // Register new data buffer for the new GIO Channel.
    if (fs_net_buffer_new(gio_server) != 0)
    {
        g_warning("%s: Failed to allocate GIO channel data buffer.", __PRETTY_FUNCTION__);
        return -1;
    }
   	
    /* setup a timer */
    g_timeout_add(100, send_packet_timeout, NULL);
   	
    /* Pass over flow control to GLIB */
    gloop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(gloop);

    /* Never reached */
    return 0;
}



