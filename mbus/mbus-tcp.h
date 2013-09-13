//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
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

#include "mbus-protocol.h"
#include "mbus-protocol-aux.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _mbus_tcp_data
{
    char *host;
    uint16_t port;
} mbus_tcp_data;

int  mbus_tcp_connect(mbus_handle *handle);
int  mbus_tcp_disconnect(mbus_handle *handle);
int  mbus_tcp_send_frame(mbus_handle *handle, mbus_frame *frame);
int  mbus_tcp_recv_frame(mbus_handle *handle, mbus_frame *frame);
void mbus_tcp_data_free(mbus_handle *handle);
int  mbus_tcp_set_timeout_set(double seconds);

#ifdef __cplusplus
}
#endif

#endif /* MBUS_TCP_H */



