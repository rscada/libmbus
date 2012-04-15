//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdio.h>
#include <strings.h>

#include <mbus/mbus.h>
#include <mbus/mbus-tcp.h>

#define PACKET_BUFF_SIZE 2048

//------------------------------------------------------------------------------
/// Setup a TCP/IP handle.
//------------------------------------------------------------------------------
mbus_tcp_handle *
mbus_tcp_connect(char *host, int port)
{
    mbus_tcp_handle *handle;

    struct hostent *host_addr;
    struct sockaddr_in s;
    struct timeval time_out;

    if (host == NULL)
    {
        return NULL;
    }

    if ((handle = (mbus_tcp_handle *)malloc(sizeof(mbus_tcp_handle))) == NULL)
    {
        char error_str[128];
        snprintf(error_str, sizeof(error_str), "%s: failed to allocate memory for handle\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        return NULL;
    }

    handle->host = host; // strdup ?
    handle->port = port;

    //
    // create the TCP connection
    //
    if ((handle->sock = socket(AF_INET,SOCK_STREAM, 0)) < 0)
    {
        char error_str[128];
        snprintf(error_str, sizeof(error_str), "%s: failed to setup a socket.", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        return NULL;
    }

    s.sin_family = AF_INET;
    s.sin_port = htons(port);

    /* resolve hostname */
    if ((host_addr = gethostbyname(host)) == NULL)
    {
        char error_str[128];
        snprintf(error_str, sizeof(error_str), "%s: unknown host: %s", __PRETTY_FUNCTION__, host);
        mbus_error_str_set(error_str);
        free(handle);
        return NULL;
    }

    bcopy((void *)(host_addr->h_addr), (void *)(&s.sin_addr), host_addr->h_length);

    if (connect(handle->sock, (struct sockaddr *)&s, sizeof(s)) < 0)
    {
        char error_str[128];
        snprintf(error_str, sizeof(error_str), "%s: Failed to establish connection to %s:%d", __PRETTY_FUNCTION__, host, port);
        mbus_error_str_set(error_str);
        free(handle);
        return NULL;
    }

    // Set a timeout
    time_out.tv_sec  = 4; //seconds
    time_out.tv_usec = 0;
    setsockopt(handle->sock, SOL_SOCKET, SO_SNDTIMEO, &time_out, sizeof(time_out));
    setsockopt(handle->sock, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out));

    return handle;    
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_tcp_disconnect(mbus_tcp_handle *handle)
{
    if (handle == NULL)
    {
        return -1;
    }

    close(handle->sock);
    
    free(handle);

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_tcp_send_frame(mbus_tcp_handle *handle, mbus_frame *frame)
{
    u_char buff[PACKET_BUFF_SIZE];
    int len, ret;

    if (handle == NULL || frame == NULL)
    {
        return -1;
    }

    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        char error_str[128];
        snprintf(error_str, sizeof(error_str), "%s: mbus_frame_pack failed\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        return -1;
    }

    if ((ret = write(handle->sock, buff, len)) != len)
    {   
        char error_str[128];
        snprintf(error_str, sizeof(error_str), "%s: Failed to write frame to socket (ret = %d)\n", __PRETTY_FUNCTION__, ret);
        mbus_error_str_set(error_str);
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_tcp_recv_frame(mbus_tcp_handle *handle, mbus_frame *frame)
{
    char buff[PACKET_BUFF_SIZE];
    int len, remaining, nread;

    bzero((void *)buff, sizeof(buff));

    //
    // read data until a packet is received
    //
    remaining = 1; // start by reading 1 byte
    len = 0;

    do {

        if ((nread = read(handle->sock, &buff[len], remaining)) == -1)
        {
            mbus_error_str_set("M-Bus tcp transport layer failed to read data.");
            return -1;
        }

        len += nread;

    } while ((remaining = mbus_parse(frame, buff, len)) > 0);
      
    if (remaining < 0)
    {
        mbus_error_str_set("M-Bus layer failed to parse data.");
        return -2;
    }
  
    return 0;
}

