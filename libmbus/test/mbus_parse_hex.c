//------------------------------------------------------------------------------
// Copyright (C) 2010, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mbus/mbus-protocol.h>

int
main(int argc, char *argv[])
{
	int fd, len, i, result;
	u_char raw_buff[4096], buff[4096], *ptr, *endptr;
	mbus_frame reply;
	mbus_frame_data frame_data;

	if (argc != 2)
    {
        fprintf(stderr, "%s binary-file\n", argv[0]);
        return -1;
    }

	if ((fd = open(argv[1], O_RDONLY, 0)) == -1)
    {
		fprintf(stderr, "%s: failed to open '%s'", argv[0], argv[1]);
        return -1;
    }

	bzero(raw_buff, sizeof(raw_buff));
	len = read(fd, raw_buff, sizeof(raw_buff));
	close(fd);

    i = 0;
    ptr    = 0;
    endptr = raw_buff;
    while (i < sizeof(buff)-1)
    {
        ptr = endptr;
        buff[i] = (u_char)strtol(ptr, (char **)&endptr, 16);
        
        // abort at non hex value 
        if (ptr == endptr)
            break;
           
        i++;
    }

	bzero(&reply, sizeof(reply));
	bzero(&frame_data, sizeof(frame_data));
	
	//mbus_parse_set_debug(1);
	
	result = mbus_parse(&reply, buff, i);

	if (result < 0)
	{
	    fprintf(stderr, "mbus_parse: %s\n", mbus_error_str());
        return -1;
    }
    else if (result > 0)
    {
        fprintf(stderr, "mbus_parse: need %d more bytes\n", result);
        return -1;
    }
    
    result = mbus_frame_data_parse(&reply, &frame_data);
	
	if (result != 0)
	{
	    mbus_frame_print(&reply);
	    fprintf(stderr, "mbus_frame_data_parse: %s\n", mbus_error_str());
        return 1;
    }
    
    //mbus_frame_print(&reply);
    //mbus_frame_data_print(&frame_data);
	printf("%s", mbus_frame_data_xml(&frame_data));
}


