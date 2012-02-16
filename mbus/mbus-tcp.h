//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// FreeSCADA 
// http://www.FreeSCADA.com
// freescada@freescada.com
//
//------------------------------------------------------------------------------

/**
 * @file   mbus-tcp.h
 * 
 * @brief  Functions and data structures for sending M-Bus data via TCP.
 *
 */

#ifndef MBUS_TCP_H
#define MBUS_TCP_H

#include <mbus/mbus.h>

typedef struct _mbus_tcp_handle {

    char *host;

    int port;
    int sock;

} mbus_tcp_handle;


mbus_tcp_handle *mbus_tcp_connect(char *host, int port);
int              mbus_tcp_disconnect(mbus_tcp_handle *handle);
int              mbus_tcp_send_frame(mbus_tcp_handle *handle, mbus_frame *frame);
int              mbus_tcp_recv_frame(mbus_tcp_handle *handle, mbus_frame *frame);

#endif /* MBUS_TCP_H */



