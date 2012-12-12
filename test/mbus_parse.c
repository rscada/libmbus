//------------------------------------------------------------------------------
// Copyright (C) 2010, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <err.h>
#include <stdio.h>
#include <string.h>

#include <mbus/mbus-protocol.h>

int
main(int argc, char *argv[])
{
    FILE *fp = NULL;
    size_t buff_len, len;
	u_char buf[1024];
	mbus_frame reply;
	mbus_frame_data frame_data;
	char *xml_result = NULL;

	if (argc != 2)
    {
        fprintf(stderr, "usage: %s binary-file\n", argv[0]);
        return 1;
    }

	if ((fp = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "%s: failed to open '%s'\n", argv[0], argv[1]);
        return 1;
    }

	memset(buf, 0, sizeof(buf));
	len = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);

	memset(&reply, 0, sizeof(reply));
	memset(&frame_data, 0, sizeof(frame_data));
	mbus_parse(&reply, buf, len);
	mbus_frame_data_parse(&reply, &frame_data);
	mbus_frame_print(&reply);
	
	if ((xml_result = mbus_frame_data_xml(&frame_data)) == NULL)
    {
        fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
        return 1;
    }
    printf("%s", xml_result);
    free(xml_result);
	
	return 0;
}


