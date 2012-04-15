//------------------------------------------------------------------------------
// Copyright (C) 2010, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "CUnit/Basic.h"

#include <mbus/mbus-protocol.h>

static char *test_frame_table[][2] = {
//    {"test-frames/frame1.hex", "test-frames/frame1.xml"},
    {"test-frames/frame2.hex", "test-frames/frame2.xml"},
    {"test-frames/kamstrup_multical_601.hex", "test-frames/kamstrup_multical_601.xml"},
    {"test-frames/manual_frame2.hex", "test-frames/manual_frame2.xml"},
    {"test-frames/manual_frame3.hex", "test-frames/manual_frame3.xml"},
    {"test-frames/manual_frame7.hex", "test-frames/manual_frame7.xml"},
    {"test-frames/svm_f22_telegram1.hex", "test-frames/svm_f22_telegram1.xml"},
    {"test-frames/electricity-meter-1.hex", "test-frames/electricity-meter-1.xml"},
    {"test-frames/electricity-meter-2.hex", "test-frames/electricity-meter-2.xml"},
    };
static int no_test_frames = 6;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *read_file(char *filename)
{
    FILE *f;
    size_t size, size2;
    char *file_content;
    

    if ((f = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "%s: Failed to open file %s.\n", __PRETTY_FUNCTION__, filename);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END); 
    size = ftell(f); 
    fseek(f, 0, SEEK_SET); 

    if ((file_content = (char *)malloc(size+1)) == NULL)
    {
        fprintf(stderr, "%s: Failed to allocate memory of size %d.\n", __PRETTY_FUNCTION__, size);
        fclose(f);
        return NULL;
    }

    if ((size2 = fread(file_content, 1, size, f)) != size)
    {
        fprintf(stderr, "%s: Failed to read file content (%s, %d:%d).\n", __PRETTY_FUNCTION__, filename, size, size2);
        free(file_content);
        fclose(f);
        return NULL;
    }

    file_content[size] = 0;
    fclose(f);
    return file_content;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int
hex_to_bin(char *buff, size_t buff_size, char *hex_buff)
{
    char *ptr, *endptr;
    int i;
    
    i = 0;
    ptr    = 0;
    endptr = hex_buff;
    while ((ptr != endptr) && i < buff_size-1)
    {
        ptr = endptr;
        buff[i++] = (u_char)strtol(ptr, (char **)&endptr, 16);
    }

    return i;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int init_suite(void)
{
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int clean_suite(void)
{
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void test_frames(void)
{
    int i, len;

    for (i = 0; i < no_test_frames; i++)
    {
       	mbus_frame frame;
	    mbus_frame_data frame_data;

        char *hex, bin[2048], *xml_ref, *xml = NULL;

        hex     = read_file(test_frame_table[i][0]);
        xml_ref = read_file(test_frame_table[i][1]);

        len = hex_to_bin(bin, sizeof(bin), hex);

    	mbus_parse(&frame, bin, len);
	
    	mbus_frame_data_parse(&frame, &frame_data);

        xml = mbus_frame_data_xml(&frame_data);

        //printf("xml     = %s\n", xml);
        //printf("xml_ref = %s\n", xml_ref);      

        CU_ASSERT(xml != NULL);

        if (xml)
        {
            int res = strcmp(xml, xml_ref);
            CU_ASSERT(res == 0);
            if (res != NULL)
            {
                printf("Unit test failed for %s\n", test_frame_table[i][0]);
            }
        }

        if (hex)     free(hex);
        if (xml_ref) free(xml_ref);
    }
    
    return;
}

//------------------------------------------------------------------------------
// setup and run the tests
//------------------------------------------------------------------------------
int main()
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    if ((pSuite = CU_add_suite("MBUS frame parse test suite", init_suite, clean_suite)) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test_frames", test_frames))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}



