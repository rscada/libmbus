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
 * @file   mbus-serial.h
 * 
 * @brief  Functions and data structures for sending M-Bus data via RS232.
 *
 */

#ifndef MBUS_SERIAL_H
#define MBUS_SERIAL_H

#include <termios.h>
#include <mbus/mbus.h>

typedef struct _mbus_serial_handle {

    char *device;

    int fd;
    struct termios t;
 
} mbus_serial_handle;


mbus_serial_handle *mbus_serial_connect(char *device);
int                 mbus_serial_disconnect(mbus_serial_handle *handle);
int                 mbus_serial_send_frame(mbus_serial_handle *handle, mbus_frame *frame);
int                 mbus_serial_recv_frame(mbus_serial_handle *handle, mbus_frame *frame);
int                 mbus_serial_set_baudrate(mbus_serial_handle *handle, int baudrate);
#endif /* MBUS_SERIAL_H */



