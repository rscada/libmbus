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
 * @file   mbus-serial.h
 *
 * @brief  Functions and data structures for sending M-Bus data via RS232.
 *
 */

#ifndef MBUS_SERIAL_H
#define MBUS_SERIAL_H

#include <termios.h>
#include "mbus-protocol-aux.h"
#include "mbus-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _mbus_serial_data
{
    char *device;
    struct termios t;
} mbus_serial_data;

int  mbus_serial_connect(mbus_handle *handle);
int  mbus_serial_disconnect(mbus_handle *handle);
int  mbus_serial_send_frame(mbus_handle *handle, mbus_frame *frame);
int  mbus_serial_recv_frame(mbus_handle *handle, mbus_frame *frame);
int  mbus_serial_set_baudrate(mbus_handle *handle, long baudrate);
void mbus_serial_data_free(mbus_handle *handle);

#ifdef __cplusplus
}
#endif

#endif /* MBUS_SERIAL_H */

