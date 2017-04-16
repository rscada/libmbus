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

#include <unistd.h>
#include <limits.h>

#include <stdio.h>
#include <strings.h>

#include <errno.h>
#include <string.h>

#include "mbus-mock.h"
#include "mbus-protocol-aux.h"
#include "mbus-protocol.h"

#define PACKET_BUFF_SIZE 2048

//------------------------------------------------------------------------------
// Create the mock context
//------------------------------------------------------------------------------
mbus_handle *
mbus_context_mock(bool verbose)
{
    mbus_handle *handle;
    mbus_mock_data *mock_data;
    char error_str[128];

    if ((handle = (mbus_handle *) malloc(sizeof(mbus_handle))) == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: Failed to allocate handle.\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    if ((mock_data = (mbus_mock_data *)malloc(sizeof(mbus_mock_data))) == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: failed to allocate memory for handle\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        free(handle);
        return NULL;
    }

    handle->max_data_retry = 3;
    handle->max_search_retry = 1;
    handle->is_serial = 1;
    handle->purge_first_frame = MBUS_FRAME_PURGE_M2S;
    handle->auxdata = mock_data;
    handle->open = mbus_mock_connect;
    handle->close = mbus_mock_disconnect;
    handle->recv = mbus_mock_recv_frame;
    handle->send = mbus_mock_send_frame;
    handle->free_auxdata = mbus_mock_data_free;
    handle->recv_event = NULL;
    handle->send_event = NULL;
    handle->scan_progress = NULL;
    handle->found_event = NULL;

    mock_data->is_connected = 0;
    mock_data->send_buf = malloc(PACKET_BUFF_SIZE);
    mock_data->recv_buf = malloc(PACKET_BUFF_SIZE);
    mock_data->verbose = verbose;
    mbus_mock_data_buf_reset(handle);

    return handle;
}

//------------------------------------------------------------------------------
/// Set up a mock connection handle.
//------------------------------------------------------------------------------
int
mbus_mock_connect(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle == NULL)
        return -1;

    mock_data = (mbus_mock_data *) handle->auxdata;
    if (mock_data == NULL)
        return -1;

    mock_data->is_connected = 1;

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_mock_disconnect(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle == NULL)
    {
        return -1;
    }

    mock_data = (mbus_mock_data *) handle->auxdata;

    mock_data->is_connected = 0;
    mock_data->send_len = 0;
    mock_data->recv_len = 0;

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void
mbus_mock_data_free(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle)
    {
        mock_data = (mbus_mock_data *) handle->auxdata;

        if (mock_data == NULL)
        {
            return;
        }

        free(mock_data->recv_buf);
        free(mock_data->send_buf);
        free(mock_data);
        handle->auxdata = NULL;
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_mock_send_frame(mbus_handle *handle, mbus_frame *frame)
{
    unsigned char buff[PACKET_BUFF_SIZE];
    int len;
    mbus_mock_data *mock_data;

    if (handle == NULL || frame == NULL)
    {
        return -1;
    }

     mock_data = (mbus_mock_data *) handle->auxdata;

    // Make sure the connection is open
     if (mock_data->is_connected != 1) {
        return -1;
    }

    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        fprintf(stderr, "%s: mbus_frame_pack failed\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if (mock_data->verbose) {
        // dump in HEX form to stdout what we write to the send buffer
        printf("%s: Dumping M-Bus frame [%d bytes]: ", __PRETTY_FUNCTION__, len);
        int i;
        for (i = 0; i < len; i++)
        {
           printf("%.2X ", buff[i]);
        }
        printf("\n");
    }

    memcpy(mock_data->send_ptr, buff, len);
    mock_data->send_ptr += len;
    mock_data->send_len += len;

    if (handle->send_event)
        handle->send_event(MBUS_HANDLE_TYPE_SERIAL, mock_data->send_buf, mock_data->send_len);

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_mock_recv_frame(mbus_handle *handle, mbus_frame *frame)
{
    unsigned char buff[PACKET_BUFF_SIZE];
    int remaining, timeouts;
    ssize_t len, nread;
    mbus_mock_data *mock_data;

    if (handle == NULL || frame == NULL)
    {
        fprintf(stderr, "%s: Invalid parameter.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_ERROR;
    }

     mock_data = (mbus_mock_data *) handle->auxdata;

    // Make sure the connection is open
     if (mock_data->is_connected != 1) {
        return -1;
    }

     if (mock_data->recv_len > PACKET_BUFF_SIZE)
    {
        // avoid out of bounds access
        return MBUS_RECV_RESULT_ERROR;
    }

    remaining = 1; // start by reading 1 byte
    len = 0;

    do {
        if (len + remaining > PACKET_BUFF_SIZE)
        {
            // avoid out of bounds access
            return MBUS_RECV_RESULT_ERROR;
        }

        if ((remaining > (int)(mock_data->recv_len - mock_data->recv_pos)))
        {
            nread = mock_data->recv_len - mock_data->recv_pos;
        }
        else
        {
            nread = remaining;
        }

        memcpy(&buff[len], mock_data->recv_ptr, nread);
        mock_data->recv_ptr += nread;
        mock_data->recv_pos += nread;

        if (nread == 0)
        {
            timeouts++;

            if (timeouts >= 3)
            {
                // abort to avoid endless loop
                break;
            }
        }

        if (len > (SSIZE_MAX-nread))
        {
            // avoid overflow
            return MBUS_RECV_RESULT_ERROR;
        }

        len += nread;

    } while ((remaining = mbus_parse(frame, buff, len)) > 0);

    if (len == 0)
    {
        // No data received
        return MBUS_RECV_RESULT_TIMEOUT;
    }

    //
    // call the receive event function, if the callback function is registered
    //
    if (handle->recv_event)
        handle->recv_event(MBUS_HANDLE_TYPE_SERIAL, mock_data->recv_buf, mock_data->recv_len);

    if (remaining != 0)
    {
        // Would be OK when e.g. scanning the bus, otherwise it is a failure.
        return MBUS_RECV_RESULT_INVALID;
    }

     if (mock_data->recv_len == SIZE_MAX)
    {
        fprintf(stderr, "%s: M-Bus layer failed to parse data.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_ERROR;
    }

    if (mock_data->verbose) {
        // dump in HEX form to stdout what we read from the recv buffer
        printf("%s: Dumping M-Bus frame [%zu bytes]: ", __PRETTY_FUNCTION__, len);
        int i;
        for (i = 0; i < len; i++)
        {
           printf("%.2X ", buff[i]);
        }
        printf("\n");
    }

    return MBUS_RECV_RESULT_OK;
}

//------------------------------------------------------------------------------
// Reset content of mock data structure (null initialize)
//------------------------------------------------------------------------------
void
mbus_mock_data_buf_reset(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle)
    {
        mock_data = (mbus_mock_data *) handle->auxdata;

        if (mock_data == NULL)
        {
            return;
        }

        memset(mock_data->send_buf, 0, PACKET_BUFF_SIZE);
        mock_data->send_len = 0;
        mock_data->send_ptr = mock_data->send_buf;

        memset(mock_data->recv_buf, 0, PACKET_BUFF_SIZE);
        mock_data->recv_len = 0;
        mock_data->recv_ptr = mock_data->recv_buf;
        mock_data->recv_pos = 0;
    }
}

//------------------------------------------------------------------------------
// Write data in the receive buffer to be read by the library as response
//------------------------------------------------------------------------------
int
mbus_mock_data_recvbuf_write(mbus_handle *handle, const unsigned char *data, size_t data_size)
{
    mbus_mock_data *mock_data;

    if (handle == NULL)
    {
        return -1;
    }

    if (data_size > PACKET_BUFF_SIZE) {
        return -1;
    }

    if (data == NULL)
    {
        return -1;
    }

    mock_data = (mbus_mock_data *) handle->auxdata;

    if (mock_data == NULL)
    {
        return -1;
    }

    memcpy(mock_data->recv_buf, data, data_size);
    mock_data->recv_len = data_size;

    return 0;
}

//------------------------------------------------------------------------------
// Returns the count of bytes read from the recv buffer
//------------------------------------------------------------------------------
int mbus_mock_data_recvbuf_bytes_read(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle == NULL)
    {
        return -1;
    }

    mock_data = (mbus_mock_data *) handle->auxdata;

    if (mock_data == NULL)
    {
        return -1;
    }

    return (mock_data->recv_pos);
}

//------------------------------------------------------------------------------
// Returns the count of bytes written to the sendbuffer by the library
//------------------------------------------------------------------------------
int mbus_mock_data_sendbuf_bytes_written(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle == NULL)
    {
        return -1;
    }

    mock_data = (mbus_mock_data *) handle->auxdata;

    if (mock_data == NULL)
    {
        return -1;
    }

    return (mock_data->send_len);
}

//------------------------------------------------------------------------------
// Returns a pointer to the sendbuffer
//------------------------------------------------------------------------------
const char *mbus_mock_data_sendbuf(mbus_handle *handle)
{
    mbus_mock_data *mock_data;

    if (handle == NULL)
    {
        return NULL;
    }

    mock_data = (mbus_mock_data *) handle->auxdata;

    if (mock_data == NULL)
    {
        return NULL;
    }

    return (mock_data->send_buf);
}
