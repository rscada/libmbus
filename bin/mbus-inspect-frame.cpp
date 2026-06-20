#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>

extern "C" {
#include <mbus/mbus.h>
}

int read_hex_formated_frame(unsigned char* data, int maxsize) {
    std::string line;
    if(!std::getline(std::cin, line)) {
        return -1;
    }

    std::istringstream hex_chars(line);

    // read from stdin and count bytes
    int out = 0;
    unsigned int c;
    while( hex_chars >> std::hex >> c ){
        out++;
        if ( out >= maxsize) {
            return -1;
        }
        data[out - 1] = c;
    }

    data[out] = '\0';
    return out;
}


// just format framedata
// for debug usage
int
main(int argc, char **argv)
{
    unsigned char data[500];
    mbus_frame reply;
    mbus_frame_data reply_data;
    char* xml_result;

    int line_length = read_hex_formated_frame(data, 500);
    if (line_length  <= 0)
    {
        std::cout << "line couldnt be parsed in any way, or was too long (500 bytes)" << std::endl;
        return 0;
    }

    if (mbus_parse(&reply, data, line_length) < 0)
    {
        fprintf(stderr, "M-bus parse_data error: %s %d\n", mbus_error_str(), line_length);
        return 1;
    }
    //
    // parse data
    //
    if (mbus_frame_data_parse(&reply, &reply_data) == -1)
    {
        fprintf(stderr, "M-bus data parse error: %s\n", mbus_error_str());
        return 1;
    }

    //
    // generate XML and print to standard output
    //
    if ((xml_result = mbus_frame_data_xml(&reply_data)) == NULL)
    {
        fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
        return 1;
    }

    printf("%s", xml_result);
    free(xml_result);

    // manual free
    if (reply_data.data_var.record)
    {
        mbus_data_record_free(reply_data.data_var.record); // free's up the whole list
    }

    return 0;
}
