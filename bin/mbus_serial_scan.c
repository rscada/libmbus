//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
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

#include "mbus-protocol.h"

__attribute__((noreturn)) static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "%s device|file\n", __progname);
	exit(-1);
}

void parse_file(const char *);
void scan_device(const char *);

int
main(int argc, char *argv[])
{
	struct stat sb;

	if (argc != 2)
		usage();

	bzero(&sb, sizeof(sb));
	if(stat(argv[1], &sb))
		err(-1, "stat on '%s' failed", argv[1]);
	if(S_ISREG(sb.st_mode))
		parse_file(argv[1]);
	else if(S_ISBLK(sb.st_mode))
		scan_device(argv[1]);
	else
		usage();

	return (0);
}

void
parse_file(const char *path)
{
	int fd, len;
	u_char buf[256];
	mbus_frame reply;
	mbus_frame_data frame_data;

	bzero(buf, sizeof(buf));
	if((fd = open(path, O_RDONLY, 0)) == -1)
		err(-1, "open '%s'", path);

	len = read(fd, buf, sizeof(buf));
	close(fd);

	bzero(&reply, sizeof(reply));
	bzero(&frame_data, sizeof(frame_data));
	mbus_parse(&reply, buf, len);
	mbus_frame_data_parse(&reply, &frame_data);
	mbus_frame_print(&reply);
	mbus_frame_data_print_xml(&frame_data);
}

void
scan_device(const char *path)
{
	int fd, len;
/*	int fd2;*/
	ssize_t i, rc;
	u_char buf[256];
	mbus_frame *frame, *reply;
	mbus_frame_data frame_data;
	struct termios tio;

	if((fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0)) == -1)
		err(-1, "failed to open device %s", path);

	bzero(buf, sizeof(buf));
	tcgetattr(fd, &tio);
	cfsetispeed(&tio, B2400);
	cfsetospeed(&tio, B2400);
	cfmakeraw(&tio);
	tio.c_cflag |= PARENB;
	tcsetattr(fd, TCSANOW, &tio);

	if((rc = read(fd, buf, sizeof(buf))) > 0) {
		for(i = 0; i < rc; i++)
			printf(":%.2X ", buf[i]);
		printf("\n");
	}

	frame = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);
	reply = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);
	frame->control  = MBUS_CONTROL_MASK_REQ_UD2 | MBUS_CONTROL_MASK_DIR_M2S;
	frame->address  = 0;
	while(frame->address <= 0) {
		frame->address = MBUS_ADDRESS_BROADCAST_REPLY;
		mbus_frame_print(frame);

		if((len = mbus_frame_pack(frame, buf, sizeof(buf))) > 0) {
			if((rc = write(fd, buf, len)) != len)
				warn("writing %zd", rc);

			usleep(1000000);
			len = 0;
			while((rc = read(fd, buf+len, sizeof(buf)-len)) > 0) {
				len += rc;
				usleep(1000000);
			}

/*			printf("%d\n", fd2 = open("output.bin", O_WRONLY | O_CREAT | O_TRUNC, 0));*/
/*			write(fd2, buf, len);*/
/*			printf("%d", len);*/
/*			close(fd2);*/

			mbus_parse(reply, buf, len);
			mbus_frame_data_parse(reply, &frame_data);
			mbus_frame_print(reply);
			mbus_frame_data_print_xml(&frame_data);
		}

		frame->address++;
	}

	mbus_frame_free(reply);
	mbus_frame_free(frame);
	close(fd);
}
