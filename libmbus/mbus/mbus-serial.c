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

#include <sys/types.h>

#include <stdio.h>
#include <strings.h>

#include <termios.h>
#include <errno.h>
#include <string.h>

#include <mbus/mbus.h>
#include <mbus/mbus-serial.h>

#define PACKET_BUFF_SIZE 2048

//------------------------------------------------------------------------------
/// Set up a serial connection handle.
//------------------------------------------------------------------------------
mbus_serial_handle *
mbus_serial_connect(char *device)
{
    mbus_serial_handle *handle;

    if (device == NULL)
    {
        return NULL;
    }

    if ((handle = (mbus_serial_handle *)malloc(sizeof(mbus_serial_handle))) == NULL)
    {
        fprintf(stderr, "%s: failed to allocate memory for handle\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    handle->device = device; // strdup?

    //
    // create the SERIAL connection
    //

    // Use blocking read and handle it by serial port VMIN/VTIME setting
    if ((handle->fd = open(handle->device, O_RDWR | O_NOCTTY)) < 0)
    {
        fprintf(stderr, "%s: failed to open tty.", __PRETTY_FUNCTION__);
        return NULL;
    }

    bzero(&(handle->t), sizeof(handle->t));
    handle->t.c_cflag |= (CS8|CREAD|CLOCAL);
    handle->t.c_cflag |= PARENB;

    // No received data still OK
    handle->t.c_cc[VMIN]  = 0;

    // Wait at most 0.2 sec.Note that it starts after first received byte!!
    // I.e. if CMIN>0 and there are no data we would still wait forever...
    //
    // The specification mentions link layer response timeout this way:
    // The time structure of various link layer communication types is described in EN60870-5-1. The answer time
    // between the end of a master send telegram and the beginning of the response telegram of the slave shall be
    // between 11 bit times and (330 bit times + 50ms).
    //  
    // For 2400Bd this means (330 + 11) / 2400 + 0.05 = 188.75 ms (added 11 bit periods to receive first byte).
    // I.e. timeout of 0.2s seems appropriate for 2400Bd.

    handle->t.c_cc[VTIME] = 2;

    cfsetispeed(&(handle->t), B2400);
    cfsetospeed(&(handle->t), B2400);

#ifdef MBUS_SERIAL_DEBUG
    printf("%s: t.c_cflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_cflag);
    printf("%s: t.c_oflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_oflag);
    printf("%s: t.c_iflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_iflag);
    printf("%s: t.c_lflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_lflag);
#endif 

    tcsetattr(handle->fd, TCSANOW, &(handle->t));

    return handle;    
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int
mbus_serial_set_baudrate(mbus_serial_handle *handle, int baudrate)
{
    if (handle == NULL)
        return -1;

    switch (baudrate)
    {
        case 300:
            cfsetispeed(&(handle->t), B300);
            cfsetospeed(&(handle->t), B300);
            return 0;

        case 1200:
            cfsetispeed(&(handle->t), B1200);
            cfsetospeed(&(handle->t), B1200);
            return 0;

        case 2400:
            cfsetispeed(&(handle->t), B2400);
            cfsetospeed(&(handle->t), B2400);
            return 0;

        case 9600:
            cfsetispeed(&(handle->t), B9600);
            cfsetospeed(&(handle->t), B9600);
            return 0;

       default:
       return -1; // unsupported baudrate
    }
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_disconnect(mbus_serial_handle *handle)
{
    if (handle == NULL)
    {
        return -1;
    }

    close(handle->fd);
    
    free(handle);

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_send_frame(mbus_serial_handle *handle, mbus_frame *frame)
{
    u_char buff[PACKET_BUFF_SIZE];
    int len, ret;

    if (handle == NULL || frame == NULL)
    {
        return -1;
    }

    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        fprintf(stderr, "%s: mbus_frame_pack failed\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if ((ret = write(handle->fd, buff, len)) != len)
    {   
        fprintf(stderr, "%s: Failed to write frame to socket (ret = %d: %s)\n", __PRETTY_FUNCTION__, ret, strerror(errno));
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_recv_frame(mbus_serial_handle *handle, mbus_frame *frame)
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
        //printf("%s: Attempt to read %d bytes [len = %d]\n", __PRETTY_FUNCTION__, remaining, len);

        if ((nread = read(handle->fd, &buff[len], remaining)) == -1)
        {
       //     fprintf(stderr, "%s: aborting recv frame (remaining = %d, len = %d, nread = %d)\n",
         //          __PRETTY_FUNCTION__, remaining, len, nread);
            return -1;
        }

//  printf("%s: Got %d byte [remaining %d, len %d]\n", __PRETTY_FUNCTION__, nread, remaining, len);

        len += nread;

    } while ((remaining = mbus_parse(frame, buff, len)) > 0);
      
    if(remaining < 0)
    {
        // Would be OK when e.g. scanning the bus, otherwise it is a failure.
        // printf("%s: M-Bus layer failed to receive complete data.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if (len == -1)
    {
        fprintf(stderr, "%s: M-Bus layer failed to parse data.\n", __PRETTY_FUNCTION__);
        return -1;
    }
  
    return 0;
}



