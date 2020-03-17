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

#include <mbus/mbus.h>

int
main(int argc, char *argv[])
{
    FILE *fp = NULL;
    size_t len;
    int normalized = 0;
    unsigned char buf[1024];
    mbus_frame reply;
    mbus_frame_data frame_data;
    char *xml_result = NULL, *file = NULL;

    if (argc == 3 && strcmp(argv[1], "-n") == 0)
    {
        file = argv[2];
        normalized = 1;
    }
    else if (argc == 2)
    {
        file = argv[1];
    }
    else
    {
        fprintf(stderr, "usage: %s [-n] binary-file\n", argv[0]);
        fprintf(stderr, "    optional flag -n for normalized values\n");
        return 1;
    }

    if ((fp = fopen(file, "r")) == NULL)
    {
        fprintf(stderr, "%s: failed to open '%s'\n", argv[0], file);
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    len = fread(buf, 1, sizeof(buf), fp);

    if (ferror(fp) != 0)
    {
        fprintf(stderr, "%s: failed to read '%s'\n", argv[0], file);
        fclose(fp);
        return 1;
    }

    fclose(fp);

    memset(&reply, 0, sizeof(reply));
    memset(&frame_data, 0, sizeof(frame_data));
    mbus_parse(&reply, buf, len);
    mbus_frame_data_parse(&reply, &frame_data);
    mbus_frame_print(&reply);

    xml_result = normalized ? mbus_frame_data_xml_normalized(&frame_data) : mbus_frame_data_xml(&frame_data);

    if (xml_result == NULL)
    {
        fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
        return 1;
    }
    printf("%s", xml_result);
    free(xml_result);

    return 0;
}

