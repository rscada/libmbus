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
	int fd, len;
	u_char buf[1024];
	mbus_frame reply;
	mbus_frame_data frame_data;

	if (argc != 2)
    {
        fprintf(stderr, "%s binary-file\n", argv[0]);
        return 1;
    }

	if ((fd = open(argv[1], O_RDONLY, 0)) == -1)
    {
		fprintf(stderr, "%s: failed to open '%s'", argv[0], argv[1]);
        return 1;
    }

	bzero(buf, sizeof(buf));
	len = read(fd, buf, sizeof(buf));

	close(fd);

	bzero(&reply, sizeof(reply));
	bzero(&frame_data, sizeof(frame_data));
	mbus_parse(&reply, buf, len);
	mbus_frame_data_parse(&reply, &frame_data);
	mbus_frame_print(&reply);
	printf("%s", mbus_frame_data_xml(&frame_data));
}


