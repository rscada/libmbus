//------------------------------------------------------------------------------
// Copyright (C) 2016, Torsten Mehnert, Eckelmann AG
//                     Thomas Spicker, Eckelmann AG
// All rights reserved.
//
// Eckelmann AG
// http://www.eckelmann.de
// info@eckelmann.de
//
//------------------------------------------------------------------------------

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "mbus-mock.h"
#include <mbus/mbus.h>

#define PASS fprintf(stdout, "PASS: %s\r\n", __FUNCTION__); return
#define FAIL fprintf(stdout, "FAIL: %s\r\n", __FUNCTION__); return

//------------------------------------------------------------------------------
// Check if valid ping frame sent to MBUS_ADDRESS_NETWORK_LAYER
//------------------------------------------------------------------------------
void
test_send_ping_frame_networklayer(mbus_handle *handle)
{
    static const unsigned char send_data[] = { 0x10, 0x40, 0xFD, 0x3D, 0x16 };
    size_t bytes_written;

    mbus_mock_data_buf_reset(handle);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 0) == -1)
    {
        FAIL;
    }

    bytes_written = mbus_mock_data_sendbuf_bytes_written(handle);
    if (bytes_written != sizeof(send_data))
    {
        FAIL;
    }

    if (memcmp(mbus_mock_data_sendbuf(handle), send_data, bytes_written))
    {
        FAIL;
    }

    PASS;
}

//------------------------------------------------------------------------------
// Check if valid ping frame sent to primary Address 23
//------------------------------------------------------------------------------
void
test_send_ping_frame_primary_23(mbus_handle *handle)
{
    static const unsigned char send_data[] = { 0x10, 0x40, 0x17, 0x57, 0x16 };
    size_t bytes_written;

    mbus_mock_data_buf_reset(handle);

    if (mbus_send_ping_frame(handle, 23, 0) == -1)
    {
        FAIL;
    }

    bytes_written = mbus_mock_data_sendbuf_bytes_written(handle);
    if (bytes_written != sizeof(send_data))
    {
        FAIL;
    }

    if (memcmp(mbus_mock_data_sendbuf(handle), send_data, bytes_written))
    {
        FAIL;
    }

    PASS;
}

//------------------------------------------------------------------------------
// Check that no data purged (last ping_frame parameter = 0)
//------------------------------------------------------------------------------
void
test_send_ping_frame_nopurge(mbus_handle *handle)
{
    static const unsigned char recv_data[] = { 0xE5, 0xDE, 0xAD, 0xBE, 0xEF };

    mbus_mock_data_buf_reset(handle);
    mbus_mock_data_recvbuf_write(handle, recv_data, sizeof(recv_data));

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 0) == -1)
    {
        FAIL;
    }

    if (mbus_mock_data_recvbuf_bytes_read(handle) > 0)
    {
        FAIL;
    }

    PASS;
}

//------------------------------------------------------------------------------
// Check that all data purged (last ping_frame parameter = 1)
//------------------------------------------------------------------------------
void
test_send_ping_frame_purge(mbus_handle *handle)
{
    static const unsigned char recv_data[] = { 0xE5, 0xDE, 0xAD, 0xBE, 0xEF };

    mbus_mock_data_buf_reset(handle);
    mbus_mock_data_recvbuf_write(handle, recv_data, sizeof(recv_data));

    mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1);

    if (mbus_mock_data_recvbuf_bytes_read(handle) != sizeof(recv_data))
    {
        FAIL;
    }

    PASS;
}

//------------------------------------------------------------------------------
// send_request with no answer detect MBUS_RECV_RESULT_TIMEOUT
//------------------------------------------------------------------------------
void
test_send_request_timeout(mbus_handle *handle)
{
    mbus_frame reply;

    mbus_mock_data_buf_reset(handle);

    if (mbus_send_request_frame(handle, 67) == -1) {
        FAIL;
    }

    if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_TIMEOUT)
    {
        FAIL;
    }

    PASS;
}

//------------------------------------------------------------------------------
// sendrecv_request with no answer detect MBUS_RECV_RESULT_TIMEOUT
//------------------------------------------------------------------------------
void
test_sencrecv_request_timeout(mbus_handle *handle)
{
    mbus_frame reply;

    mbus_mock_data_buf_reset(handle);

    if (mbus_sendrecv_request(handle, 2, &reply, 9) == 0)
    {
        FAIL;
    }

    PASS;
}

//------------------------------------------------------------------------------
// Main function for test execution
//------------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
            if (strcmp (argv [argn], "--help") == 0
            ||  strcmp (argv [argn], "-h") == 0) {
                puts ("mbus_sendrecv [options]");
                puts ("  --help / -h            display this help");
                puts ("  --verbose / -v         verbose test output");
                return 0;
            }
            if (strcmp (argv [argn], "--verbose") == 0
            ||  strcmp (argv [argn], "-v") == 0)
                verbose = true;
    }

    mbus_handle *handle = NULL;
    if ((handle = mbus_context_mock(verbose)) == NULL)
    {
       fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
       return EXIT_FAILURE;
    }

    if (mbus_connect(handle) == -1)
    {
       fprintf(stderr,"Failed to setup connection to M-bus mock object\n");
       mbus_context_free(handle);
       return EXIT_FAILURE;
    }

    test_send_ping_frame_networklayer(handle);
    test_send_ping_frame_primary_23(handle);
    test_send_ping_frame_nopurge(handle);
    test_send_ping_frame_purge(handle);
    test_send_request_timeout(handle);
    test_sencrecv_request_timeout(handle);

    mbus_disconnect(handle);
    mbus_context_free(handle);

    return EXIT_SUCCESS;
}
