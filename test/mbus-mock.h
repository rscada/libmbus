//------------------------------------------------------------------------------
// Copyright (C) 2016, Torsten Mehnert, Eckelmann AG
// All rights reserved.
//
// Eckelmann AG
// http://www.eckelmann.de
// info@eckelmann.de
//
// This file is based on mbus-serial.* from Robert Johansson and Contributors
//------------------------------------------------------------------------------

/**
 * @file   mbus-mock.h
 *
 * @brief  Functions and data structures for unit testing M-Bus data.
 *
 */

#ifndef MBUS_MOCK_H
#define MBUS_MOCK_H

#include <stdbool.h>
#include "mbus-protocol-aux.h"
#include "mbus-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mbus_mock_data
{
    unsigned char is_connected;

    char *send_buf;
    size_t send_len;
    char *send_ptr;

    char *recv_buf;
    size_t recv_len;
    char *recv_ptr;
    size_t recv_pos;

    bool verbose;
} mbus_mock_data;

/**
 * Allocate and initialize M-Bus mock context.
 *
 * @return Initialized "unified" handler when successful, NULL otherwise;
 */
mbus_handle * mbus_context_mock(bool verbose);

int  mbus_mock_connect(mbus_handle *handle);
int  mbus_mock_disconnect(mbus_handle *handle);
int  mbus_mock_send_frame(mbus_handle *handle, mbus_frame *frame);
int  mbus_mock_recv_frame(mbus_handle *handle, mbus_frame *frame);
void mbus_mock_data_free(mbus_handle *handle);

int  mbus_mock_data_sendbuf_bytes_written(mbus_handle *handle);
const char *mbus_mock_data_sendbuf(mbus_handle *handle);
int  mbus_mock_data_recvbuf_write(mbus_handle *handle, const unsigned char *data, size_t data_size);
int  mbus_mock_data_recvbuf_bytes_read(mbus_handle *handle);
void mbus_mock_data_buf_reset(mbus_handle *handle);

#ifdef __cplusplus
}
#endif

#endif /* MBUS_MOCK_H */
