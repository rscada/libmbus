//------------------------------------------------------------------------------
// Copyright (C) 2010-2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mbus-protocol.h"

static int parse_debug = 0, debug = 0;
static char error_str[512];

#define NITEMS(x) (sizeof(x)/sizeof(x[0]))

//------------------------------------------------------------------------------
// internal data
//------------------------------------------------------------------------------
static mbus_slave_data slave_data[MBUS_MAX_PRIMARY_SLAVES];

//
// init event callback
//
void (*_mbus_recv_event)(unsigned char src_type, const char *buff, size_t len) = NULL;
void (*_mbus_send_event)(unsigned char src_type, const char *buff, size_t len) = NULL;

//
//  trace callbacks
//
void
mbus_dump_recv_event(unsigned char src_type, const char *buff, size_t len)
{
    mbus_hex_dump("RECV", buff, len);
}

void
mbus_dump_send_event(unsigned char src_type, const char *buff, size_t len)
{
    mbus_hex_dump("SEND", buff, len);
}

//------------------------------------------------------------------------------
/// Register a function for receive events.
//------------------------------------------------------------------------------
void
mbus_register_recv_event(void (*event)(unsigned char src_type, const char *buff, size_t len))
{
    _mbus_recv_event = event;
}

//------------------------------------------------------------------------------
/// Register a function for send events.
//------------------------------------------------------------------------------
void
mbus_register_send_event(void (*event)(unsigned char src_type, const char *buff, size_t len))
{
    _mbus_send_event = event;
}

//------------------------------------------------------------------------------
/// Return a string that contains an the latest error message.
//------------------------------------------------------------------------------
char *
mbus_error_str()
{
    return error_str;
}

void
mbus_error_str_set(char *message)
{
    if (message)
    {
        snprintf(error_str, sizeof(error_str), "%s", message);
    }
}

void
mbus_error_reset()
{
    snprintf(error_str, sizeof(error_str), "no errors");
}

//------------------------------------------------------------------------------
/// Return a pointer to the slave_data register. This register can be used for 
/// storing current slave status.
//------------------------------------------------------------------------------
mbus_slave_data *
mbus_slave_data_get(size_t i)
{
    if (i < MBUS_MAX_PRIMARY_SLAVES)
    {
        return &slave_data[i];
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
//
// M-Bus FRAME RELATED FUNCTIONS
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Allocate an M-bus frame data structure and initialize it according to which
/// frame type is requested.
//------------------------------------------------------------------------------
mbus_frame *
mbus_frame_new(int frame_type)
{
    mbus_frame *frame = NULL;
    
    if ((frame = malloc(sizeof(mbus_frame))) != NULL)
    {
        memset((void *)frame, 0, sizeof(mbus_frame));
        
        frame->type = frame_type;
        switch (frame->type)
        {
            case MBUS_FRAME_TYPE_ACK:
            
                frame->start1 = MBUS_FRAME_ACK_START;
            
                break;

            case MBUS_FRAME_TYPE_SHORT:
            
                frame->start1 = MBUS_FRAME_SHORT_START;
                frame->stop   = MBUS_FRAME_STOP;  

                break;

            case MBUS_FRAME_TYPE_CONTROL:

                frame->start1 = MBUS_FRAME_CONTROL_START;
                frame->start2 = MBUS_FRAME_CONTROL_START;
                frame->length1 = 3;
                frame->length2 = 3;
                frame->stop   = MBUS_FRAME_STOP;  
                  
                break;

            case MBUS_FRAME_TYPE_LONG:

                frame->start1 = MBUS_FRAME_LONG_START;
                frame->start2 = MBUS_FRAME_LONG_START;
                frame->stop   = MBUS_FRAME_STOP;              

                break;
        }            
    }

    return frame;
}

//------------------------------------------------------------------------------
/// Free the memory resources allocated for the M-Bus frame data structure.
//------------------------------------------------------------------------------
int
mbus_frame_free(mbus_frame *frame)
{
    if (frame)
    {
        if (frame->next != NULL)
            mbus_frame_free(frame->next);
    
        free(frame);
        return 0;
    }
    return -1;
}

//------------------------------------------------------------------------------
/// Caclulate the checksum of the M-Bus frame. Internal.
//------------------------------------------------------------------------------
unsigned char
calc_checksum(mbus_frame *frame)
{
    size_t i;
    unsigned char cksum;
    
    assert(frame != NULL);
    switch(frame->type)
    {
        case MBUS_FRAME_TYPE_SHORT:

            cksum = frame->control;
            cksum += frame->address;

            break;

        case MBUS_FRAME_TYPE_CONTROL:

            cksum = frame->control;
            cksum += frame->address;
            cksum += frame->control_information;

            break;

        case MBUS_FRAME_TYPE_LONG:

            cksum = frame->control;
            cksum += frame->address;
            cksum += frame->control_information;

            for (i = 0; i < frame->data_size; i++)
            {
                cksum += frame->data[i];
            }

            break;

        case MBUS_FRAME_TYPE_ACK:
        default:
            cksum = 0;
    }

    return cksum;
}

//------------------------------------------------------------------------------
/// Caclulate the checksum of the M-Bus frame. The checksum algorithm is the
/// arithmetic sum of the frame content, without using carry. Which content
/// that is included in the checksum calculation depends on the frame type.
//------------------------------------------------------------------------------
int
mbus_frame_calc_checksum(mbus_frame *frame)
{
    if (frame)
    {
        switch (frame->type)
        {
            case MBUS_FRAME_TYPE_ACK:
            case MBUS_FRAME_TYPE_SHORT:
            case MBUS_FRAME_TYPE_CONTROL:
            case MBUS_FRAME_TYPE_LONG:
                frame->checksum = calc_checksum(frame);
            
                break;
        
            default:
                return -1;
        }        
    }
    
    return 0;
}

///
/// Calculate the values of the lengths fields in the M-Bus frame. Internal.
///
unsigned char
calc_length(const mbus_frame *frame)
{
    assert(frame != NULL);
    switch(frame->type)
    {
        case MBUS_FRAME_TYPE_CONTROL:
            return 3;
        case MBUS_FRAME_TYPE_LONG:
            return frame->data_size + 3;
        default:
            return 0;
    }
}

//------------------------------------------------------------------------------
/// Calculate the values of the lengths fields in the M-Bus frame.
//------------------------------------------------------------------------------
int
mbus_frame_calc_length(mbus_frame *frame)
{
    if (frame)
    {
        frame->length1 = frame->length2 = calc_length(frame);
    }

    return 0;
}

//------------------------------------------------------------------------------
/// Return the M-Bus frame type
//------------------------------------------------------------------------------
int
mbus_frame_type(mbus_frame *frame)
{
    if (frame)
    {
        return frame->type;
    }
    return -1;
}

//------------------------------------------------------------------------------
/// Return the M-Bus frame direction 
//------------------------------------------------------------------------------
int
mbus_frame_direction(mbus_frame *frame)
{
    if (frame)
    {
        if (frame->type == MBUS_FRAME_TYPE_ACK)
        {
            return MBUS_CONTROL_MASK_DIR_S2M;    
        }
        else
        {
            return (frame->control & MBUS_CONTROL_MASK_DIR);
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
/// Verify that parsed frame is a valid M-bus frame. 
//
// Possible checks:
//
// 1) frame type
// 2) Start/stop bytes
// 3) control field
// 4) length field and actual data size
// 5) checksum
//
//------------------------------------------------------------------------------
int
mbus_frame_verify(mbus_frame *frame)
{
    unsigned char checksum;

    if (frame)
    {
        switch (frame->type)
        {
            case MBUS_FRAME_TYPE_ACK:
                return frame->start1 == MBUS_FRAME_ACK_START;

            case MBUS_FRAME_TYPE_SHORT:
                if(frame->start1 != MBUS_FRAME_SHORT_START)
                {
                    snprintf(error_str, sizeof(error_str), "No frame start");
                        
                    return -1;
                }
                
                if ((frame->control !=  MBUS_CONTROL_MASK_SND_NKE)                          &&
                    (frame->control !=  MBUS_CONTROL_MASK_REQ_UD1)                          &&
                    (frame->control != (MBUS_CONTROL_MASK_REQ_UD1 | MBUS_CONTROL_MASK_FCB)) &&
                    (frame->control !=  MBUS_CONTROL_MASK_REQ_UD2)                          &&
                    (frame->control != (MBUS_CONTROL_MASK_REQ_UD2 | MBUS_CONTROL_MASK_FCB)))
                {
                    snprintf(error_str, sizeof(error_str), "Unknown Control Code 0x%.2x", frame->control);
                
                    return -1;
                }

                break;

            case MBUS_FRAME_TYPE_CONTROL:
            case MBUS_FRAME_TYPE_LONG:
                if(frame->start1  != MBUS_FRAME_CONTROL_START ||
                   frame->start2  != MBUS_FRAME_CONTROL_START)
                {
                    snprintf(error_str, sizeof(error_str), "No frame start");
                        
                    return -1;
                }
                
                if ((frame->control !=  MBUS_CONTROL_MASK_SND_UD)                          &&
                    (frame->control != (MBUS_CONTROL_MASK_SND_UD | MBUS_CONTROL_MASK_FCB)) &&
                    (frame->control !=  MBUS_CONTROL_MASK_RSP_UD)                          &&
                    (frame->control != (MBUS_CONTROL_MASK_RSP_UD | MBUS_CONTROL_MASK_DFC)) &&
                    (frame->control != (MBUS_CONTROL_MASK_RSP_UD | MBUS_CONTROL_MASK_ACD)) &&
                    (frame->control != (MBUS_CONTROL_MASK_RSP_UD | MBUS_CONTROL_MASK_DFC | MBUS_CONTROL_MASK_ACD)))
                {
                    snprintf(error_str, sizeof(error_str), "Unknown Control Code 0x%.2x", frame->control);
                
                    return -1;
                }
                
                if (frame->length1 != frame->length2)
                {
                    snprintf(error_str, sizeof(error_str), "Frame length 1 != 2");
        
                    return -1;
                }
                
                if (frame->length1 != calc_length(frame))
                {
                    snprintf(error_str, sizeof(error_str), "Frame length 1 != calc length");
                
                    return -1;
                }

                break;

            default:
                snprintf(error_str, sizeof(error_str), "Unknown frame type 0x%.2x", frame->type);
                
                return -1;
        }

        if(frame->stop != MBUS_FRAME_STOP)
        {
            snprintf(error_str, sizeof(error_str), "No frame stop");
                        
            return -1;
        }
        
        checksum = calc_checksum(frame);
        
        if(frame->checksum != checksum)
        {
            snprintf(error_str, sizeof(error_str), "Invalid checksum (0x%.2x != 0x%.2x)", frame->checksum, checksum);
                
            return -1;
        }

        return 0;
    }
    
    snprintf(error_str, sizeof(error_str), "Got null pointer to frame.");

    return -1;
}


//------------------------------------------------------------------------------
//
// DATA ENCODING, DECODING, AND CONVERSION FUNCTIONS
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
/// Encode BCD data
///
//------------------------------------------------------------------------------
int
mbus_data_bcd_encode(unsigned char *bcd_data, size_t bcd_data_size, int value)
{ 
    int v0, v1, v2, x1, x2;
    size_t i;

    if (bcd_data && bcd_data_size && (value >= 0))
    {
        v2 = value;
    
        for (i = 0; i < bcd_data_size; i++)
        {
            v0 = v2;
            v1 = (int)(v0 / 10);
            v2 = (int)(v1 / 10);

            x1 = v0 - v1 * 10;
            x2 = v1 - v2 * 10;

            bcd_data[bcd_data_size-1-i] = (x2 << 4) | x1;
        }

        return 0;    
    }
    
    return -1;
}

//------------------------------------------------------------------------------
///
/// Decode BCD data
///
//------------------------------------------------------------------------------
long long
mbus_data_bcd_decode(unsigned char *bcd_data, size_t bcd_data_size)
{       
    long long val = 0;
    size_t i;
    
    if (bcd_data)
    {
        for (i = bcd_data_size; i > 0; i--)
        {
            val = (val * 10) + ((bcd_data[i-1]>>4) & 0xF);
            val = (val * 10) + ( bcd_data[i-1]     & 0xF);
        }

        return val;    
    }
    
    return -1;
}

//------------------------------------------------------------------------------
///
/// Decode INTEGER data
///
//------------------------------------------------------------------------------
int
mbus_data_int_decode(unsigned char *int_data, size_t int_data_size)
{
    int val = 0;
    size_t i;
    
    if (int_data)
    {
        for (i = int_data_size; i > 0; i--)
        {
            val = (val << 8) + int_data[i-1];
        }

        return val;    
    }

    return -1;
}

long
mbus_data_long_decode(unsigned char *int_data, size_t int_data_size)
{
    long val = 0;
    size_t i;
    
    if (int_data)
    {
        for (i = int_data_size; i > 0; i--)
        {
            val = (val << 8) + int_data[i-1];
        }

        return val;    
    }

    return -1;
}

long long
mbus_data_long_long_decode(unsigned char *int_data, size_t int_data_size)
{
    long long val = 0;
    size_t i;
    
    if (int_data)
    {
        for (i = int_data_size; i > 0; i--)
        {
            val = (val << 8) + int_data[i-1];
        }

        return val;    
    }

    return -1;
}

//------------------------------------------------------------------------------
///
/// Encode INTEGER data (into 'int_data_size' bytes)
///
//------------------------------------------------------------------------------
int
mbus_data_int_encode(unsigned char *int_data, size_t int_data_size, int value)
{
    int i;
    
    if (int_data)
    {
        for (i = 0; i < int_data_size; i++)
        {
            int_data[i] = (value>>(i*8)) & 0xFF;
        }

        return 0;
    }

    return -1;
}

//------------------------------------------------------------------------------
///
/// Decode float data 
///
/// see also http://en.wikipedia.org/wiki/Single-precision_floating-point_format
///
//------------------------------------------------------------------------------
float
mbus_data_float_decode(unsigned char *float_data)
{
    float val = 0.0f;
    long temp = 0, fraction;
    int sign,exponent;
    size_t i;
    
    if (float_data)
    {
        for (i = 4; i > 0; i--)
        {
            temp = (temp << 8) + float_data[i-1];
        }
        
        // first bit = sign bit
        sign     = (temp >> 31) ? -1 : 1;
  
        // decode 8 bit exponent
        exponent = ((temp & 0x7F800000) >> 23) - 127;
  
        // decode explicit 23 bit fraction
        fraction = temp & 0x007FFFFF;
  
        if ((exponent != -127) &&
            (exponent != 128))
        {
            // normalized value, add bit 24 
            fraction |= 0x800000;
        }
  
        // calculate float value
        val = (float) sign * fraction * pow(2.0f, -23.0f + exponent);

        return val;    
    }

    return -1.0;
}

//------------------------------------------------------------------------------
///
/// Decode string data.
///
//------------------------------------------------------------------------------
void
mbus_data_str_decode(unsigned char *dst, const unsigned char *src, size_t len)
{
    size_t i;

    i = 0;
    
    if (src && dst)
    {
        dst[len] = '\0';
        while(len > 0) {
            dst[i++] = src[--len];
        }
    }
}

//------------------------------------------------------------------------------
///
/// Decode binary data.
///
//------------------------------------------------------------------------------
void
mbus_data_bin_decode(unsigned char *dst, const unsigned char *src, size_t len, size_t max_len)
{
    size_t i, pos;
    
    i = 0;
    pos = 0;
    
    if (src && dst)
    {
        while((i < len) && ((pos+3) < max_len)) {
            pos += snprintf(&dst[pos], max_len - pos, "%.2X ", src[i]);
            i++;
        }
        
        if (pos > 0)
        {
            // remove last space
            pos--;
        }
        
        dst[pos] = '\0';
    }
}

//------------------------------------------------------------------------------
///
/// Decode time data (usable for type f = 4 bytes or type g = 2 bytes)
///
//------------------------------------------------------------------------------
void
mbus_data_tm_decode(struct tm *t, unsigned char *t_data, size_t t_data_size)
{
    if (t == NULL)
    {
        return;
    }
    
    t->tm_sec   = 0;
    t->tm_min   = 0;
    t->tm_hour  = 0;
    t->tm_mday  = 0;
    t->tm_mon   = 0;
    t->tm_year  = 0; 
    t->tm_wday  = 0;
    t->tm_yday  = 0;
    t->tm_isdst = 0;
    
    if (t_data)
    {
        if (t_data_size == 4)                // Type F = Compound CP32: Date and Time
        {     
            if ((t_data[0] & 0x80) == 0)     // Time valid ?
            {
                t->tm_min   = t_data[0] & 0x3F;
                t->tm_hour  = t_data[1] & 0x1F;
                t->tm_mday  = t_data[2] & 0x1F;
                t->tm_mon   = (t_data[3] & 0x0F) - 1;
                t->tm_year  = ((t_data[2] & 0xE0) >> 5) | 
                              ((t_data[3] & 0xF0) >> 1); 
                t->tm_isdst = (t_data[1] & 0x80) ? 1 : 0;  // day saving time
            }
        }
        else if (t_data_size == 2)           // Type G: Compound CP16: Date
        {
            t->tm_mday = t_data[0] & 0x1F;
            t->tm_mon  = (t_data[1] & 0x0F) - 1; 
            t->tm_year = ((t_data[0] & 0xE0) >> 5) | 
                         ((t_data[1] & 0xF0) >> 1);  
        }
    }
}

//------------------------------------------------------------------------------
///
/// Generate manufacturer code from 2-byte encoded data
///
//------------------------------------------------------------------------------
int
mbus_data_manufacturer_encode(unsigned char *m_data, unsigned char *m_code)
{
    int m_val;

    if (m_data == NULL || m_code == NULL)
        return -1;

    m_val = ((((int)m_code[0] - 64) & 0x001F) << 10) + 
            ((((int)m_code[1] - 64) & 0x001F) << 5) +
            ((((int)m_code[2] - 64) & 0x001F));

    mbus_data_int_encode(m_data, 2, m_val);
    
    return 0;
}

//------------------------------------------------------------------------------
///
/// Generate manufacturer code from 2-byte encoded data
///
//------------------------------------------------------------------------------
const char *
mbus_decode_manufacturer(unsigned char byte1, unsigned char byte2)
{
    static char m_str[4];
    
    int m_id;
    
    m_str[0] = byte1;
    m_str[1] = byte2;
    
    m_id = mbus_data_int_decode(m_str, 2);
        
    m_str[0] = (char)(((m_id>>10) & 0x001F) + 64);
    m_str[1] = (char)(((m_id>>5)  & 0x001F) + 64);
    m_str[2] = (char)(((m_id)     & 0x001F) + 64);
    m_str[3] = 0;

    return m_str;
}

const char *
mbus_data_product_name(mbus_data_variable_header *header)
{
    static char buff[128];
    unsigned int manufacturer;

    memset(buff, 0, sizeof(buff));

    if (header)
    {
        manufacturer = (header->manufacturer[1] << 8) + header->manufacturer[0];

        if (manufacturer == MBUS_VARIABLE_DATA_MAN_ACW) 
        {
            switch (header->version)
            {
                case 0x09:
                    strcpy(buff,"Itron CF Echo 2");
                    break;
                case 0x0A:
                    strcpy(buff,"Itron CF 51");
                    break;
                case 0x0B:
                    strcpy(buff,"Itron CF 55");
                    break;
                case 0x0E:
                    strcpy(buff,"Itron BM +m");
                    break;
                case 0x0F:
                    strcpy(buff,"Itron CF 800");
                    break;
                case 0x14:
                    strcpy(buff,"Itron CYBLE M-Bus 1.4");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_EFE)
        {
            switch (header->version)
            {
                case 0x00:
                    strcpy(buff, ((header->medium == 0x06) ? "Engelmann WaterStar" : "Engelmann SensoStar 2"));
                    break;
                case 0x01:
                    strcpy(buff,"Engelmann SensoStar 2C");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_ELV)
        {
            switch (header->version)
            {
                case 0x14:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1B:
                case 0x1C:
                case 0x1D:
                    strcpy(buff, "Elvaco CMa10");
                    break;
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3A:
                case 0x3B:
                    strcpy(buff,"Elvaco CMa11");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_SLB) 
        {
            switch (header->version)
            {
                case 0x02:
                    strcpy(buff,"Allmess Megacontrol CF-50");
                    break;
                case 0x06:
                    strcpy(buff,"CF Compact / Integral MK MaXX");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_HYD) 
        {
            switch (header->version)
            {
                case 0x28:
                    strcpy(buff,"ABB F95 Typ US770");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_LUG) 
        {
            switch (header->version)
            {
                case 0x02:
                    strcpy(buff,"Landis & Gyr Ultraheat 2WR5");
                    break;
                case 0x03:
                    strcpy(buff,"Landis & Gyr Ultraheat 2WR6");
                    break;
                case 0x04:
                    strcpy(buff,"Landis & Gyr Ultraheat UH50");
                    break;
                case 0x07:
                    strcpy(buff,"Landis & Gyr Ultraheat T230");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_RKE)
        {
            switch (header->version)
            {
                case 0x69:
                    strcpy(buff,"Ista sensonic II mbus");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_SVM) 
        {
            switch (header->version)
            {
                case 0x08:
                    strcpy(buff,"Elster F2");
                    break;
                case 0x09:
                    strcpy(buff,"Kamstrup SVM F22");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_SON) 
        {
            switch (header->version)
            {
                case 0x0D:
                    strcpy(buff,"Sontex Supercal 531");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_LSE)
        {
            switch (header->version)
            {
                case 0x99:
                    strcpy(buff,"Siemens WFH21");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_SEN)
        {
            switch (header->version)
            {
                case 0x19:
                    strcpy(buff,"Sensus PolluCom E");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_SPX) 
        {
            switch (header->version)
            {
                case 0x31:
                case 0x34:
                    strcpy(buff,"Sensus PolluTherm");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_ELS)
        {
            switch (header->version)
            {
                case 0x02:
                    strcpy(buff,"Elster TMP-A");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_NZR)
        {
            switch (header->version)
            {
                case 0x01:
                    strcpy(buff,"NZR DHZ 5/63");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_KAM)
        {
            switch (header->version)
            {
                case 0x01:
                    strcpy(buff,"Kamstrup 382 (6850-005)");
                    break;
                case 0x08:
                    strcpy(buff,"Kamstrup Multical 601");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_EMH)
        {
            switch (header->version)
            {
                case 0x00:
                    strcpy(buff,"EMH DIZ");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_TCH)
        {
            switch (header->version)
            {
                case 0x26:
                    strcpy(buff,"Techem m-bus S");
                    break;
            }
        }
        else if (manufacturer == MBUS_VARIABLE_DATA_MAN_ZRM)
        {
            switch (header->version)
            {
                case 0x81:
                    strcpy(buff,"Minol Minocal C2");
                    break;
                case 0x82:
                    strcpy(buff,"Minol Minocal WR3");
                    break;
            }
        }
    }

    return buff;
}

//------------------------------------------------------------------------------
//
// FIXED-LENGTH DATA RECORD FUNCTIONS
//
//------------------------------------------------------------------------------


//
//   Value         Field Medium/Unit              Medium
// hexadecimal Bit 16  Bit 15    Bit 8  Bit 7
//     0        0       0         0     0         Other
//     1        0       0         0     1         Oil
//     2        0       0         1     0         Electricity
//     3        0       0         1     1         Gas
//     4        0       1         0     0         Heat
//     5        0       1         0     1         Steam
//     6        0       1         1     0         Hot Water
//     7        0       1         1     1         Water
//     8        1       0         0     0         H.C.A.
//     9        1       0         0     1         Reserved
//     A        1       0         1     0         Gas Mode 2
//     B        1       0         1     1         Heat Mode 2
//     C        1       1         0     0         Hot Water Mode 2
//     D        1       1         0     1         Water Mode 2
//     E        1       1         1     0         H.C.A. Mode 2
//     F        1       1         1     1         Reserved
//

///
/// For fixed-length frames, get a string describing the medium.
///
const char *
mbus_data_fixed_medium(mbus_data_fixed *data)
{
    static char buff[256];

    if (data)
    {
        switch ( (data->cnt1_type&0xC0)>>6 | (data->cnt2_type&0xC0)>>4 )
        {
            case 0x00:    
                snprintf(buff, sizeof(buff), "Other");
                break;    
            case 0x01:    
                snprintf(buff, sizeof(buff), "Oil");
                break;    
            case 0x02:    
                snprintf(buff, sizeof(buff), "Electricity");
                break;    
            case 0x03:    
                snprintf(buff, sizeof(buff), "Gas");
                break;    
            case 0x04:    
                snprintf(buff, sizeof(buff), "Heat");
                break;    
            case 0x05:    
                snprintf(buff, sizeof(buff), "Steam");
                break;    
            case 0x06:    
                snprintf(buff, sizeof(buff), "Hot Water");
                break;    
            case 0x07:    
                snprintf(buff, sizeof(buff), "Water");
                break;    
            case 0x08:    
                snprintf(buff, sizeof(buff), "H.C.A.");
                break;    
            case 0x09:    
                snprintf(buff, sizeof(buff), "Reserved");
                break;    
            case 0x0A:    
                snprintf(buff, sizeof(buff), "Gas Mode 2");
                break;
            case 0x0B:
                snprintf(buff, sizeof(buff), "Heat Mode 2");
                break;
            case 0x0C:
                snprintf(buff, sizeof(buff), "Hot Water Mode 2");
                break;
            case 0x0D:
                snprintf(buff, sizeof(buff), "Water Mode 2");
                break;
            case 0x0E:
                snprintf(buff, sizeof(buff), "H.C.A. Mode 2");
                break;    
            case 0x0F:
                snprintf(buff, sizeof(buff), "Reserved");
                break;    
            default:
                snprintf(buff, sizeof(buff), "unknown");
                break;                
        }
        
        return buff;
    }

    return NULL;
}


//------------------------------------------------------------------------------
//                        Hex code                            Hex code
//Unit                    share     Unit                      share
//              MSB..LSB                            MSB..LSB
//                       Byte 7/8                            Byte 7/8
// h,m,s         000000     00        MJ/h           100000     20
// D,M,Y         000001     01        MJ/h * 10      100001     21
//     Wh        000010     02        MJ/h * 100     100010     22
//     Wh * 10   000011     03        GJ/h           100011     23
//     Wh * 100  000100     04        GJ/h * 10      100100     24
//   kWh         000101     05        GJ/h * 100     100101     25
//  kWh   * 10   000110     06           ml          100110     26
//   kWh * 100   000111     07           ml * 10     100111     27
//   MWh         001000     08           ml * 100    101000     28
//   MWh * 10    001001     09            l          101001     29
//   MWh * 100   001010     0A            l * 10     101010     2A
//     kJ        001011     0B            l * 100    101011     2B
//     kJ * 10   001100     0C           m3          101100     2C
//     kJ * 100  001101     0D        m3 * 10        101101     2D
//     MJ        001110     0E        m3 * 100       101110     2E
//     MJ * 10   001111     0F        ml/h           101111     2F
//     MJ * 100  010000     10        ml/h * 10      110000     30
//     GJ        010001     11        ml/h * 100     110001     31
//     GJ * 10   010010     12         l/h           110010     32
//     GJ * 100  010011     13         l/h * 10      110011     33
//      W        010100     14         l/h * 100     110100     34                              
//      W * 10   010101     15       m3/h           110101     35                         
//      W * 100  010110     16     m3/h * 10       110110     36
//     kW        010111     17      m3/h * 100       110111     37
//     kW * 10   011000     18        °C* 10-3       111000     38
//     kW * 100  011001     19      units   for HCA  111001     39
//     MW        011010     1A    reserved           111010     3A
//     MW * 10   011011     1B    reserved           111011     3B
//     MW * 100  011100     1C    reserved           111100     3C
//  kJ/h         011101     1D    reserved           111101     3D
//  kJ/h * 10    011110     1E    same but historic  111110     3E
//  kJ/h * 100   011111     1F    without   units    111111     3F
//
//------------------------------------------------------------------------------
///
/// For fixed-length frames, get a string describing the unit of the data.
///
const char *
mbus_data_fixed_unit(int medium_unit_byte)
{
    static char buff[256];
    
    switch (medium_unit_byte & 0x3F)
    {
        case 0x00:    
            snprintf(buff, sizeof(buff), "h,m,s");
            break;    
        case 0x01:    
            snprintf(buff, sizeof(buff), "D,M,Y");
            break;    
            
        case 0x02:    
            snprintf(buff, sizeof(buff), "Wh");
            break;    
        case 0x03:    
            snprintf(buff, sizeof(buff), "10 Wh");
            break;    
        case 0x04:    
            snprintf(buff, sizeof(buff), "100 Wh");
            break;    
        case 0x05:    
            snprintf(buff, sizeof(buff), "kWh");
            break;    
        case 0x06:    
            snprintf(buff, sizeof(buff), "10 kWh");
            break;    
        case 0x07:    
            snprintf(buff, sizeof(buff), "100 kWh");
            break;    
        case 0x08:    
            snprintf(buff, sizeof(buff), "MWh");
            break;    
        case 0x09:    
            snprintf(buff, sizeof(buff), "10 MWh");
            break;    
        case 0x0A:    
            snprintf(buff, sizeof(buff), "100 MWh");
            break;    
            
        case 0x0B:    
            snprintf(buff, sizeof(buff), "kJ");
            break;    
        case 0x0C:    
            snprintf(buff, sizeof(buff), "10 kJ");
            break;    
        case 0x0E:    
            snprintf(buff, sizeof(buff), "100 kJ");
            break;    
        case 0x0D:    
            snprintf(buff, sizeof(buff), "MJ");
            break;    
        case 0x0F:    
            snprintf(buff, sizeof(buff), "10 MJ");
            break;    
        case 0x10:    
            snprintf(buff, sizeof(buff), "100 MJ");
            break;
        case 0x11:    
            snprintf(buff, sizeof(buff), "GJ");
            break;    
        case 0x12:    
            snprintf(buff, sizeof(buff), "10 GJ");
            break;    
        case 0x13:    
            snprintf(buff, sizeof(buff), "100 GJ");
            break;

        case 0x14:    
            snprintf(buff, sizeof(buff), "W");
            break;    
        case 0x15:    
            snprintf(buff, sizeof(buff), "10 W");
            break;    
        case 0x16:    
            snprintf(buff, sizeof(buff), "100 W");
            break;    
        case 0x17:    
            snprintf(buff, sizeof(buff), "kW");
            break;    
        case 0x18:    
            snprintf(buff, sizeof(buff), "10 kW");
            break;    
        case 0x19:    
            snprintf(buff, sizeof(buff), "100 kW");
            break;
        case 0x1A:    
            snprintf(buff, sizeof(buff), "MW");
            break;    
        case 0x1B:    
            snprintf(buff, sizeof(buff), "10 MW");
            break;    
        case 0x1C:    
            snprintf(buff, sizeof(buff), "100 MW");
            break;
            
        case 0x1D:
            snprintf(buff, sizeof(buff), "kJ/h");
            break;
        case 0x1E:
            snprintf(buff, sizeof(buff), "10 kJ/h");
            break;
        case 0x1F:
            snprintf(buff, sizeof(buff), "100 kJ/h");
            break;
        case 0x20:
            snprintf(buff, sizeof(buff), "MJ/h");
            break;
        case 0x21:
            snprintf(buff, sizeof(buff), "10 MJ/h");
            break;
        case 0x22:
            snprintf(buff, sizeof(buff), "100 MJ/h");
            break;
        case 0x23:
            snprintf(buff, sizeof(buff), "GJ/h");
            break;
        case 0x24:
            snprintf(buff, sizeof(buff), "10 GJ/h");
            break;
        case 0x25:
            snprintf(buff, sizeof(buff), "100 GJ/h");
            break;
            
        case 0x26:
            snprintf(buff, sizeof(buff), "ml");
            break;
        case 0x27:
            snprintf(buff, sizeof(buff), "10 ml");
            break;
        case 0x28:
            snprintf(buff, sizeof(buff), "100 ml");
            break;
        case 0x29:    
            snprintf(buff, sizeof(buff), "l");
            break;
        case 0x2A:    
            snprintf(buff, sizeof(buff), "10 l");
            break;
        case 0x2B:    
            snprintf(buff, sizeof(buff), "100 l");
            break;           
        case 0x2C:
            snprintf(buff, sizeof(buff), "m^3");
            break;
        case 0x2D:
            snprintf(buff, sizeof(buff), "10 m^3");
            break;
        case 0x2E:
            snprintf(buff, sizeof(buff), "m^3");
            break;
 
        case 0x2F:
            snprintf(buff, sizeof(buff), "ml/h");
            break;
        case 0x30:
            snprintf(buff, sizeof(buff), "10 ml/h");
            break;
        case 0x31:
            snprintf(buff, sizeof(buff), "100 ml/h");
            break;
        case 0x32:
            snprintf(buff, sizeof(buff), "l/h");
            break;
        case 0x33:
            snprintf(buff, sizeof(buff), "10 l/h");
            break;
        case 0x34:
            snprintf(buff, sizeof(buff), "100 l/h");
            break;
        case 0x35:
            snprintf(buff, sizeof(buff), "m^3/h");
            break;
        case 0x36:
            snprintf(buff, sizeof(buff), "10 m^3/h");
            break;
        case 0x37:
            snprintf(buff, sizeof(buff), "100 m^3/h");
            break;
            
        case 0x38:
            snprintf(buff, sizeof(buff), "1e-3 °C");
            break;
        case 0x39:
            snprintf(buff, sizeof(buff), "units for HCA");
            break;    
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
            snprintf(buff, sizeof(buff), "reserved");
            break;
        case 0x3E:    
            snprintf(buff, sizeof(buff), "reserved but historic");
            break;
        case 0x3F:
            snprintf(buff, sizeof(buff), "without units");
            break;
        default:
            snprintf(buff, sizeof(buff), "unknown");
            break;    
    }

    return buff;
}

//------------------------------------------------------------------------------
//
// VARIABLE-LENGTH DATA RECORD FUNCTIONS
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Medium                                                              Code bin    Code hex
// Other                                                              0000 0000        00
// Oil                                                                0000 0001        01
// Electricity                                                        0000 0010        02
// Gas                                                                0000 0011        03
// Heat (Volume measured at return temperature: outlet)               0000 0100        04
// Steam                                                              0000 0101        05
// Hot Water                                                          0000 0110        06
// Water                                                              0000 0111        07
// Heat Cost Allocator.                                               0000 1000        08
// Compressed Air                                                     0000 1001        09
// Cooling load meter (Volume measured at return temperature: outlet) 0000 1010        0A
// Cooling load meter (Volume measured at flow temperature: inlet) ♣  0000 1011        0B
// Heat (Volume measured at flow temperature: inlet)                  0000 1100        0C
// Heat / Cooling load meter ♣                                        0000 1101        OD
// Bus / System                                                       0000 1110        0E
// Unknown Medium                                                     0000 1111        0F
// Reserved                                                           .......... 10 to 15
// Cold Water                                                         0001 0110        16
// Dual Water                                                         0001 0111        17
// Pressure                                                           0001 1000        18
// A/D Converter                                                      0001 1001        19
// Reserved                                                           .......... 20 to FF
//------------------------------------------------------------------------------

///
/// For variable-length frames, returns a string describing the medium.
///
const char *
mbus_data_variable_medium_lookup(unsigned char medium)
{
    static char buff[256];

    switch (medium)
    {
        case MBUS_VARIABLE_DATA_MEDIUM_OTHER:
            snprintf(buff, sizeof(buff), "Other");
            break;
                                          
        case MBUS_VARIABLE_DATA_MEDIUM_OIL:
            snprintf(buff, sizeof(buff), "Oil");
            break;
                  
        case MBUS_VARIABLE_DATA_MEDIUM_ELECTRICITY:
            snprintf(buff, sizeof(buff), "Electricity");
            break;
                                          
        case MBUS_VARIABLE_DATA_MEDIUM_GAS:
            snprintf(buff, sizeof(buff), "Gas");
            break;
                                          
        case MBUS_VARIABLE_DATA_MEDIUM_HEAT_OUT:
            snprintf(buff, sizeof(buff), "Heat: Outlet");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_STEAM:
            snprintf(buff, sizeof(buff), "Steam");
            break;
                                          
        case MBUS_VARIABLE_DATA_MEDIUM_HOT_WATER:
            snprintf(buff, sizeof(buff), "Hot water");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_WATER:
            snprintf(buff, sizeof(buff), "Water");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_HEAT_COST:
            snprintf(buff, sizeof(buff), "Heat Cost Allocator");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_COMPR_AIR:
            snprintf(buff, sizeof(buff), "Compressed Air");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_COOL_OUT:
            snprintf(buff, sizeof(buff), "Cooling load meter: Outlet");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_COOL_IN:
            snprintf(buff, sizeof(buff), "Cooling load meter: Inlet");
            break;
            
        case MBUS_VARIABLE_DATA_MEDIUM_HEAT_IN:
            snprintf(buff, sizeof(buff), "Heat: Inlet");
            break;
            
        case MBUS_VARIABLE_DATA_MEDIUM_HEAT_COOL:
            snprintf(buff, sizeof(buff), "Heat / Cooling load meter");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_BUS:
            snprintf(buff, sizeof(buff), "Bus/System");
            break;
            
        case MBUS_VARIABLE_DATA_MEDIUM_UNKNOWN:
            snprintf(buff, sizeof(buff), "Unknown Medium");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_COLD_WATER:
            snprintf(buff, sizeof(buff), "Cold water");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_DUAL_WATER:
            snprintf(buff, sizeof(buff), "Dual water");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_PRESSURE:
            snprintf(buff, sizeof(buff), "Pressure");
            break;

        case MBUS_VARIABLE_DATA_MEDIUM_ADC:
            snprintf(buff, sizeof(buff), "A/D Converter");
            break;

        case 0x10: // - 0x15
        case 0x20: // - 0xFF
            snprintf(buff, sizeof(buff), "Reserved");
            break;

                                          
        // add more ...
        default:
            snprintf(buff, sizeof(buff), "Unknown medium (0x%.2x)", medium);
            break;            
    }

    return buff;       
}

//------------------------------------------------------------------------------
///
/// Lookup the unit description from a VIF field in a data record
///
//------------------------------------------------------------------------------
const char *
mbus_unit_prefix(int exp)
{
    static char buff[256];
    
    switch (exp)
    {
        case 0:
            buff[0] = 0;
            break;

        case -3:
            snprintf(buff, sizeof(buff), "m");
            break;
       
        case -6:
            snprintf(buff, sizeof(buff), "my");
            break;

        case 1:
            snprintf(buff, sizeof(buff), "10 ");
            break;

        case 2:
            snprintf(buff, sizeof(buff), "100 ");
            break;

        case 3:
            snprintf(buff, sizeof(buff), "k");
            break;
            
        case 4:
            snprintf(buff, sizeof(buff), "10 k");
            break;            

        case 5:
            snprintf(buff, sizeof(buff), "100 k");
            break;            

        case 6:
            snprintf(buff, sizeof(buff), "M");
            break;
       
        case 9:
            snprintf(buff, sizeof(buff), "T");
            break;
           
        default:
            snprintf(buff, sizeof(buff), "1e%d ", exp);
    }
        
    return buff;
}

//------------------------------------------------------------------------------
/// Look up the data lenght from a VIF field in the data record.
/// 
/// See the table on page 41 the M-BUS specification.
//------------------------------------------------------------------------------
unsigned char
mbus_dif_datalength_lookup(unsigned char dif)
{
	switch (dif&0x0F)
	{
		case 0x0:
			return 0;
		case 0x1:
			return 1;
		case 0x2:
			return 2;
		case 0x3:
			return 3;
		case 0x4:
			return 4;
		case 0x5:
			return 4;
		case 0x6:
			return 6;
		case 0x7:
			return 8;

		case 0x8:
			return 0;
		case 0x9:
			return 1;
		case 0xA:
			return 2;
		case 0xB:
			return 3;
		case 0xC:
			return 4;
		case 0xD: 
            // variable data length, 
            // data length stored in data field
			return 0;
		case 0xE:
			return 6;
		case 0xF:
			return 8;

		default: // never reached
			return 0x00;

	}
}

//------------------------------------------------------------------------------
/// Look up the unit from a VIF field in the data record.
/// 
/// See section 8.4.3  Codes for Value Information Field (VIF) in the M-BUS spec
//------------------------------------------------------------------------------
const char *
mbus_vif_unit_lookup(unsigned char vif)
{
    static char buff[256];
    int n;

    switch (vif & MBUS_DIB_VIF_WITHOUT_EXTENSION) // ignore the extension bit in this selection
    {
        // E000 0nnn Energy 10(nnn-3) W
        case 0x00:
        case 0x00+1:
        case 0x00+2:
        case 0x00+3:
        case 0x00+4:
        case 0x00+5:
        case 0x00+6:
        case 0x00+7:
            n = (vif & 0x07) - 3;
            snprintf(buff, sizeof(buff), "Energy (%sWh)", mbus_unit_prefix(n)); 
            break;

        // 0000 1nnn          Energy       10(nnn)J     (0.001kJ to 10000kJ)
        case 0x08:
        case 0x08+1:
        case 0x08+2:
        case 0x08+3:
        case 0x08+4:
        case 0x08+5:
        case 0x08+6:
        case 0x08+7:
            
            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Energy (%sJ)", mbus_unit_prefix(n));
  
            break;        

        // E001 1nnn Mass 10(nnn-3) kg 0.001kg to 10000kg
        case 0x18:
        case 0x18+1:
        case 0x18+2:
        case 0x18+3:
        case 0x18+4:
        case 0x18+5:
        case 0x18+6:
        case 0x18+7:
            
            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Mass (%skg)", mbus_unit_prefix(n-3));
  
            break;        

        // E010 1nnn Power 10(nnn-3) W 0.001W to 10000W
        case 0x28:
        case 0x28+1:
        case 0x28+2:
        case 0x28+3:
        case 0x28+4:
        case 0x28+5:
        case 0x28+6:
        case 0x28+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Power (%sW)", mbus_unit_prefix(n-3));
            //snprintf(buff, sizeof(buff), "Power (10^%d W)", n-3);
  
            break;
            
        // E011 0nnn Power 10(nnn) J/h 0.001kJ/h to 10000kJ/h
        case 0x30:
        case 0x30+1:
        case 0x30+2:
        case 0x30+3:
        case 0x30+4:
        case 0x30+5:
        case 0x30+6:
        case 0x30+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Power (%sJ/h)", mbus_unit_prefix(n));
  
            break;       

        // E001 0nnn Volume 10(nnn-6) m3 0.001l to 10000l
        case 0x10:
        case 0x10+1:
        case 0x10+2:
        case 0x10+3:
        case 0x10+4:
        case 0x10+5:
        case 0x10+6:
        case 0x10+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Volume (%s m^3)", mbus_unit_prefix(n-6));
  
            break;
        
        // E011 1nnn Volume Flow 10(nnn-6) m3/h 0.001l/h to 10000l/
        case 0x38:
        case 0x38+1:
        case 0x38+2:
        case 0x38+3:
        case 0x38+4:
        case 0x38+5:
        case 0x38+6:
        case 0x38+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Volume flow (%s m^3/h)", mbus_unit_prefix(n-6));
  
            break;     

        // E100 0nnn Volume Flow ext. 10(nnn-7) m3/min 0.0001l/min to 1000l/min
        case 0x40:
        case 0x40+1:
        case 0x40+2:
        case 0x40+3:
        case 0x40+4:
        case 0x40+5:
        case 0x40+6:
        case 0x40+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Volume flow (%s m^3/min)", mbus_unit_prefix(n-7));
  
            break;  

        // E100 1nnn Volume Flow ext. 10(nnn-9) m3/s 0.001ml/s to 10000ml/
        case 0x48:
        case 0x48+1:
        case 0x48+2:
        case 0x48+3:
        case 0x48+4:
        case 0x48+5:
        case 0x48+6:
        case 0x48+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Volume flow (%s m^3/s)", mbus_unit_prefix(n-9));
  
            break;       

        // E101 0nnn Mass flow 10(nnn-3) kg/h 0.001kg/h to 10000kg/
        case 0x50:
        case 0x50+1:
        case 0x50+2:
        case 0x50+3:
        case 0x50+4:
        case 0x50+5:
        case 0x50+6:
        case 0x50+7:

            n = (vif & 0x07);
            snprintf(buff, sizeof(buff), "Mass flow (%s kg/h)", mbus_unit_prefix(n-3));
  
            break;       

        // E101 10nn Flow Temperature 10(nn-3) °C 0.001°C to 1°C
        case 0x58:
        case 0x58+1:
        case 0x58+2:
        case 0x58+3:

            n = (vif & 0x03);
            snprintf(buff, sizeof(buff), "Flow temperature (%sdeg C)", mbus_unit_prefix(n-3));
  
            break;

        // E101 11nn Return Temperature 10(nn-3) °C 0.001°C to 1°C
        case 0x5C:
        case 0x5C+1:
        case 0x5C+2:
        case 0x5C+3:

            n = (vif & 0x03);
            snprintf(buff, sizeof(buff), "Return temperature (%sdeg C)", mbus_unit_prefix(n-3));
  
            break;

        // E110 10nn Pressure 10(nn-3) bar 1mbar to 1000mbar
        case 0x68:
        case 0x68+1:
        case 0x68+2:
        case 0x68+3:

            n = (vif & 0x03);
            snprintf(buff, sizeof(buff), "Pressure (%s bar)", mbus_unit_prefix(n-3));
  
            break;

        // E010 00nn On Time
        // nn = 00 seconds
        // nn = 01 minutes
        // nn = 10   hours
        // nn = 11    days
        // E010 01nn Operating Time coded like OnTime
        // E111 00nn Averaging Duration	coded like OnTime	 
        // E111 01nn Actuality Duration coded like OnTime
        case 0x20:
        case 0x20+1:
        case 0x20+2:
        case 0x20+3:
        case 0x24:
        case 0x24+1:
        case 0x24+2:
        case 0x24+3:
        case 0x70:
        case 0x70+1:
        case 0x70+2:
        case 0x70+3:
        case 0x74:
        case 0x74+1:
        case 0x74+2:
        case 0x74+3:
            {
                int offset;

                if      ((vif & 0x7C) == 0x20)
                    offset = snprintf(buff, sizeof(buff), "On time ");
                else if ((vif & 0x7C) == 0x24) 
                    offset = snprintf(buff, sizeof(buff), "Operating time ");
                else if ((vif & 0x7C) == 0x70)
                    offset = snprintf(buff, sizeof(buff), "Averaging Duration ");
                else
                    offset = snprintf(buff, sizeof(buff), "Actuality Duration ");
                
                switch (vif & 0x03)
                {
                    case 0x00:
                        snprintf(&buff[offset], sizeof(buff)-offset, "(seconds)");
                        break;
                    case 0x01:
                        snprintf(&buff[offset], sizeof(buff)-offset, "(minutes)");
                        break;
                    case 0x02:
                        snprintf(&buff[offset], sizeof(buff)-offset, "(hours)");
                        break;
                    case 0x03:
                        snprintf(&buff[offset], sizeof(buff)-offset, "(days)");
                        break;
                }
            }
            break;                     

        // E110 110n Time Point
        // n = 0        date
        // n = 1 time & date
        // data type G
        // data type F
        case 0x6C:
        case 0x6C+1:

            if (vif & 0x1)
                snprintf(buff, sizeof(buff), "Time Point (time & date)");
            else
                snprintf(buff, sizeof(buff), "Time Point (date)");
  
            break;
            
        // E110 00nn    Temperature Difference   10(nn-3)K   (mK to  K)
        case 0x60:
        case 0x60+1:
        case 0x60+2:
        case 0x60+3:

            n = (vif & 0x03);
            
            snprintf(buff, sizeof(buff), "Temperature Difference (%s deg C)", mbus_unit_prefix(n-3));
                        
            break;

        // E110 01nn External Temperature 10(nn-3) °C 0.001°C to 1°C
        case 0x64:
        case 0x64+1:
        case 0x64+2:
        case 0x64+3:

            n = (vif & 0x03);
            snprintf(buff, sizeof(buff), "External temperature (%s deg C)", mbus_unit_prefix(n-3));
  
            break;

        // E110 1110 Units for H.C.A. dimensionless
        case 0x6E:
            snprintf(buff, sizeof(buff), "Units for H.C.A.");
            break; 

        // E110 1111 Reserved
        case 0x6F:
            snprintf(buff, sizeof(buff), "Reserved");
            break;        

        // Custom VIF in the following string: never reached...
        case 0x7C:
            snprintf(buff, sizeof(buff), "Custom VIF");
            break;        

        // Fabrication No
        case 0x78:
            snprintf(buff, sizeof(buff), "Fabrication number");
            break;
            
        // Bus Address
        case 0x7A:
            snprintf(buff, sizeof(buff), "Bus Address");
            break;

        // Manufacturer specific: 7Fh / FF  
        case 0x7F:
        case 0xFF:
            snprintf(buff, sizeof(buff), "Manufacturer specific");
            break;

        default:
            snprintf(buff, sizeof(buff), "Unknown (VIF=0x%.2X)", vif);
            break;
    }
    

    return buff;
}


//------------------------------------------------------------------------------
// Lookup the error message
//
// See section 6.6  Codes for general application errors in the M-BUS spec
//------------------------------------------------------------------------------
const char *
mbus_data_error_lookup(int error)
{
    static char buff[256];
    
    switch (error)
    {
        case MBUS_ERROR_DATA_UNSPECIFIED:
            snprintf(buff, sizeof(buff), "Unspecified error");
            break;
           
        case MBUS_ERROR_DATA_UNIMPLEMENTED_CI:
            snprintf(buff, sizeof(buff), "Unimplemented CI-Field");
            break;
            
        case MBUS_ERROR_DATA_BUFFER_TOO_LONG:
            snprintf(buff, sizeof(buff), "Buffer too long, truncated");
            break;
            
        case MBUS_ERROR_DATA_TOO_MANY_RECORDS:
            snprintf(buff, sizeof(buff), "Too many records");
            break;
            
        case MBUS_ERROR_DATA_PREMATURE_END:
            snprintf(buff, sizeof(buff), "Premature end of record");
            break;
            
        case MBUS_ERROR_DATA_TOO_MANY_DIFES:
            snprintf(buff, sizeof(buff), "More than 10 DIFE´s");
            break;
            
        case MBUS_ERROR_DATA_TOO_MANY_VIFES:
            snprintf(buff, sizeof(buff), "More than 10 VIFE´s");
            break;
            
        case MBUS_ERROR_DATA_RESERVED:
            snprintf(buff, sizeof(buff), "Reserved");
            break;
            
        case MBUS_ERROR_DATA_APPLICATION_BUSY:
            snprintf(buff, sizeof(buff), "Application busy");
            break;
            
        case MBUS_ERROR_DATA_TOO_MANY_READOUTS:
            snprintf(buff, sizeof(buff), "Too many readouts");
            break;
            
        default:
            snprintf(buff, sizeof(buff), "Unknown error (0x%.2X)", error);
            break;
    }
    
    return buff;
}


//------------------------------------------------------------------------------
/// Lookup the unit from the VIB (VIF or VIFE)
//
//  Enhanced Identification
//    E000 1000      Access Number (transmission count) 
//    E000 1001      Medium (as in fixed header) 
//    E000 1010      Manufacturer (as in fixed header) 
//    E000 1011      Parameter set identification 
//    E000 1100      Model / Version 
//    E000 1101      Hardware version # 
//    E000 1110      Firmware version # 
//    E000 1111      Software version # 
//------------------------------------------------------------------------------
const char *
mbus_vib_unit_lookup(mbus_value_information_block *vib)
{   
    static char buff[256];
    int n;
    
    if (vib == NULL)
        return "";

    if (vib->vif == 0xFD || vib->vif == 0xFB) // first type of VIF extention: see table 8.4.4 
    {
        if (vib->nvife == 0)
        {
            snprintf(buff, sizeof(buff), "Missing VIF extension");
        }
        else if (vib->vife[0] == 0x08 || vib->vife[0] == 0x88)
        {
            // E000 1000 
            snprintf(buff, sizeof(buff), "Access Number (transmission count)");
        }
        else if (vib->vife[0] == 0x09|| vib->vife[0] == 0x89)
        {
            // E000 1001
            snprintf(buff, sizeof(buff), "Medium (as in fixed header)");
        }
        else if (vib->vife[0] == 0x0A || vib->vife[0] == 0x8A)
        {
            // E000 1010
            snprintf(buff, sizeof(buff), "Manufacturer (as in fixed header)");
        }
        else if (vib->vife[0] == 0x0B || vib->vife[0] == 0x8B)
        {
            // E000 1010
            snprintf(buff, sizeof(buff), "Parameter set identification");
        }
        else if (vib->vife[0] == 0x0C || vib->vife[0] == 0x8C)
        {
            // E000 1100
            snprintf(buff, sizeof(buff), "Model / Version");
        }
        else if (vib->vife[0] == 0x0D || vib->vife[0] == 0x8D)
        {
            // E000 1100
            snprintf(buff, sizeof(buff), "Hardware version");
        }
        else if (vib->vife[0] == 0x0E || vib->vife[0] == 0x8E)
        {
            // E000 1101
            snprintf(buff, sizeof(buff), "Firmware version");
        }
        else if (vib->vife[0] == 0x0F || vib->vife[0] == 0x8F)
        {
            // E000 1101
            snprintf(buff, sizeof(buff), "Software version");
        }
        else if (vib->vife[0] == 0x17 || vib->vife[0] == 0x97)
		{
            // VIFE = E001 0111 Error flags
            snprintf(buff, sizeof(buff), "Error flags");
		}
        else if (vib->vife[0] == 0x10)
        {
            // VIFE = E001 0000 Customer location
            snprintf(buff, sizeof(buff), "Customer location");
        }
        else if (vib->vife[0] == 0x0C)
        {
            // E000 1100 Model / Version
            snprintf(buff, sizeof(buff), "Model / Version");
        }
        else if (vib->vife[0] == 0x11)
        {
            // VIFE = E001 0001 Customer
            snprintf(buff, sizeof(buff), "Customer");
        }
        else if (vib->vife[0] == 0x1A)
        {
            // VIFE = E001 1010 Digital output (binary)
            snprintf(buff, sizeof(buff), "Digital output (binary)");
        }
        else if (vib->vife[0] == 0x1B)
        {
            // VIFE = E001 1011 Digital input (binary)
            snprintf(buff, sizeof(buff), "Digital input (binary)");
        }
        else if (vib->vife[0] == 0x09)
        {
            // VIFE = E001 0110 Password
            snprintf(buff, sizeof(buff), "Password");
        }
        else if (vib->vife[0] == 0x0B)
        {
            // VIFE = E000 1011 Parameter set identification
            snprintf(buff, sizeof(buff), "Parameter set identification");
        }
        else if ((vib->vife[0] & 0x70) == 0x40)
        {
            // VIFE = E100 nnnn 10^(nnnn-9) V
            n = (vib->vife[0] & 0x0F);
            snprintf(buff, sizeof(buff), "%s V", mbus_unit_prefix(n-9));
        }
        else if ((vib->vife[0] & 0x70) == 0x50)
        {
            // VIFE = E101 nnnn 10nnnn-12 A
            n = (vib->vife[0] & 0x0F);
            snprintf(buff, sizeof(buff), "%s A", mbus_unit_prefix(n-12));
        }
        else if ((vib->vife[0] & 0xF0) == 0x70)
        {
            // VIFE = E111 nnn Reserved
            snprintf(buff, sizeof(buff), "Reserved VIF extension");
        }
        else
        {
            snprintf(buff, sizeof(buff), "Unrecongized VIF extension: 0x%.2x", vib->vife[0]);
        }
        return buff;
    }
    else if (vib->vif == 0x7C)
    {
        // custom VIF
        snprintf(buff, sizeof(buff), "%s", vib->custom_vif);
        return buff;
    }
    else if (vib->vif == 0xFC && (vib->vife[0] & 0x78) == 0x70)
    {
        // custom VIF
        n = (vib->vife[0] & 0x07);
        snprintf(buff, sizeof(buff), "%s %s", mbus_unit_prefix(n-6), vib->custom_vif);
        return buff;
    }

    return mbus_vif_unit_lookup(vib->vif); // no extention, use VIF
}

//------------------------------------------------------------------------------
// Decode data and write to string
//
// Data format (for record->data data array)
// 
// Length in Bit   Code    Meaning           Code      Meaning
//      0          0000    No data           1000      Selection for Readout
//      8          0001     8 Bit Integer    1001      2 digit BCD
//     16          0010    16 Bit Integer    1010      4 digit BCD
//     24          0011    24 Bit Integer    1011      6 digit BCD
//     32          0100    32 Bit Integer    1100      8 digit BCD
//   32 / N        0101    32 Bit Real       1101      variable length
//     48          0110    48 Bit Integer    1110      12 digit BCD
//     64          0111    64 Bit Integer    1111      Special Functions
//
// The Code is stored in record->drh.dib.dif
//
///
/// Return a string containing the data
///
// Source: MBDOC48.PDF
//
//------------------------------------------------------------------------------
const char *
mbus_data_record_decode(mbus_data_record *record)
{
    static char buff[768];
    unsigned char vif, vife;
    
    // ignore extension bit
    vif = (record->drh.vib.vif & MBUS_DIB_VIF_WITHOUT_EXTENSION);       
    vife = (record->drh.vib.vife[0] & MBUS_DIB_VIF_WITHOUT_EXTENSION);

    if (record)
    {
        int val;
        float val3;
        long long val4;
        struct tm time;
            
        switch (record->drh.dib.dif & 0x0F)
        {
            case 0x00: // no data
        
                buff[0] = 0;
                
                break; 

            case 0x01: // 1 byte integer (8 bit)
        
                val = mbus_data_int_decode(record->data, 1);
               
                snprintf(buff, sizeof(buff), "%d", val);

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 1 byte integer\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break; 


            case 0x02: // 2 byte (16 bit)
                
                // E110 1100  Time Point (date)
                if (vif == 0x6C)            
                {
                    mbus_data_tm_decode(&time, record->data, 2);
                    snprintf(buff, sizeof(buff), "%04d-%02d-%02d", 
                                                 (time.tm_year + 2000), 
                                                 (time.tm_mon + 1), 
                                                  time.tm_mday);
                }
                else  // 2 byte integer
                {
                    val = mbus_data_int_decode(record->data, 2);
                    snprintf(buff, sizeof(buff), "%d", val);
                    if (debug)
                        printf("%s: DIF 0x%.2x was decoded using 2 byte integer\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                }
        
                break; 

            case 0x03: // 3 byte integer (24 bit)

                val = mbus_data_int_decode(record->data, 3);
                        
                snprintf(buff, sizeof(buff), "%d", val);

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 3 byte integer\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break; 
                
            case 0x04: // 4 byte (32 bit)
            
                // E110 1101  Time Point (date/time)
                // E011 0000  Start (date/time) of tariff
                // E111 0000  Date and time of battery change
                if ( (vif == 0x6D) ||                                     
                    ((record->drh.vib.vif == 0xFD) && (vife == 0x30)) ||  
                    ((record->drh.vib.vif == 0xFD) && (vife == 0x70)))    
                {
                    mbus_data_tm_decode(&time, record->data, 4);
                    snprintf(buff, sizeof(buff), "%04d-%02d-%02dT%02d:%02d:%02d", 
                                                 (time.tm_year + 2000), 
                                                 (time.tm_mon + 1), 
                                                  time.tm_mday,
                                                  time.tm_hour,
                                                  time.tm_min,
                                                  time.tm_sec);
                }
                else  // 4 byte integer
                {
                    val = mbus_data_int_decode(record->data, 4);
                    snprintf(buff, sizeof(buff), "%d", val);
                }

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 4 byte integer\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;  

            case 0x05: // 4 Byte Real (32 bit)
            
                val3 = mbus_data_float_decode(record->data);
                
                snprintf(buff, sizeof(buff), "%f", val3);
                
                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 4 byte Real\n", __PRETTY_FUNCTION__, record->drh.dib.dif);    
                    
                break;

            case 0x06: // 6 byte integer (48 bit)

                val4 = mbus_data_long_long_decode(record->data, 6);
        
                snprintf(buff, sizeof(buff), "%lld", val4);

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 6 byte integer\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;          

            case 0x07: // 8 byte integer (64 bit)

                val4 = mbus_data_long_long_decode(record->data, 8);
        
                snprintf(buff, sizeof(buff), "%lld", val4);

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 8 byte integer\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;          

            //case 0x08: 

            case 0x09: // 2 digit BCD (8 bit)
  
                val = (int)mbus_data_bcd_decode(record->data, 1);  
                snprintf(buff, sizeof(buff), "%d", val);     
        
                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 2 digit BCD\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;
                
            case 0x0A: // 4 digit BCD (16 bit)
        
                val = (int)mbus_data_bcd_decode(record->data, 2);  
                snprintf(buff, sizeof(buff), "%d", val);     

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 4 digit BCD\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;

            case 0x0B: // 6 digit BCD (24 bit)

                val = (int)mbus_data_bcd_decode(record->data, 3);  
                snprintf(buff, sizeof(buff), "%d", val);     

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 6 digit BCD\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;
                
            case 0x0C: // 8 digit BCD (32 bit)

                val = (int)mbus_data_bcd_decode(record->data, 4);  
                snprintf(buff, sizeof(buff), "%d", val);

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 8 digit BCD\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;
                                                                  
            case 0x0E: // 12 digit BCD (48 bit)

                val4 = mbus_data_bcd_decode(record->data, 6);  
                snprintf(buff, sizeof(buff), "%lld", val4);

                if (debug)
                    printf("%s: DIF 0x%.2x was decoded using 12 digit BCD\n", __PRETTY_FUNCTION__, record->drh.dib.dif);

                break;

            case 0x0F: // special functions
            
                mbus_data_bin_decode(buff, record->data, record->data_len, sizeof(buff));
                break;

            case 0x0D: // variable length
                if (record->data_len <= 0xBF)
                {
                    mbus_data_str_decode(buff, record->data, record->data_len);
                    break;
                }
                /*@fallthrough@*/

            default:
        
                snprintf(buff, sizeof(buff), "Unknown DIF (0x%.2x)", record->drh.dib.dif);
                break;
        }

        return buff;     
    }

    return NULL;
}
//------------------------------------------------------------------------------
/// Return the unit description for a variable-length data record
//------------------------------------------------------------------------------
const char *
mbus_data_record_unit(mbus_data_record *record)
{
    static char buff[128];
    
    if (record)
    {
        snprintf(buff, sizeof(buff), "%s", mbus_vib_unit_lookup(&(record->drh.vib)));
                 
        return buff;    
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
/// Return the value for a variable-length data record
//------------------------------------------------------------------------------
const char *
mbus_data_record_value(mbus_data_record *record)
{
    static char buff[768];
    
    if (record)
    {
        snprintf(buff, sizeof(buff), "%s", mbus_data_record_decode(record));
                 
        return buff;    
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
/// Return a string containing the function description
//------------------------------------------------------------------------------
const char *
mbus_data_record_function(mbus_data_record *record)
{
    static char buff[128];
    
    if (record)
    {
        switch (record->drh.dib.dif & MBUS_DATA_RECORD_DIF_MASK_FUNCTION)
        {
            case 0x00:
                snprintf(buff, sizeof(buff), "Instantaneous value");           
                break;

            case 0x10:
                snprintf(buff, sizeof(buff), "Maximum value");
                break;

            case 0x20:
                snprintf(buff, sizeof(buff), "Minimum value");
                break;

            case 0x30:
                snprintf(buff, sizeof(buff), "Value during error state");                       
                break;
        
            default:
                snprintf(buff, sizeof(buff), "unknown");                 
        }

        return buff;    
    }
    
    return NULL;
}


///
/// For fixed-length frames, return a string describing the type of value (stored or actual)
///
const char *
mbus_data_fixed_function(int status)
{
    static char buff[128];

    snprintf(buff, sizeof(buff), "%s",
            (status & MBUS_DATA_FIXED_STATUS_DATE_MASK) == MBUS_DATA_FIXED_STATUS_DATE_STORED ? 
            "Stored value" : "Actual value" );
    
    return buff;
}

//------------------------------------------------------------------------------
//
// PARSER FUNCTIONS
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// PARSE M-BUS frame data structures from binary data.
//------------------------------------------------------------------------------
int
mbus_parse(mbus_frame *frame, unsigned char *data, size_t data_size)
{
    size_t i, len;
    
    if (frame && data && data_size > 0)
    {
        if (parse_debug) 
            printf("%s: Attempting to parse binary data [size = %zu]\n", __PRETTY_FUNCTION__, data_size);

        if (parse_debug) 
            printf("%s: ", __PRETTY_FUNCTION__);

        for (i = 0; i < data_size && parse_debug; i++)
        {
            printf("%.2X ", data[i] & 0xFF);
        }

        if (parse_debug)
            printf("\n%s: done.\n", __PRETTY_FUNCTION__);
    
        switch (data[0])
        {
            case MBUS_FRAME_ACK_START:
            
                // OK, got a valid ack frame, require no more data 
                frame->start1   = data[0];
                frame->type = MBUS_FRAME_TYPE_ACK;
                return 0;
                //return MBUS_FRAME_BASE_SIZE_ACK - 1; // == 0
                              
            case MBUS_FRAME_SHORT_START:
            
                if (data_size < MBUS_FRAME_BASE_SIZE_SHORT)
                {
                    // OK, got a valid short packet start, but we need more data
                    return MBUS_FRAME_BASE_SIZE_SHORT - data_size;
                }
                
                if (data_size != MBUS_FRAME_BASE_SIZE_SHORT)
                {
                    snprintf(error_str, sizeof(error_str), "Too much data in frame.");
                
                    // too much data... ?
                    return -2;
                }
                
                // init frame data structure
                frame->start1   = data[0];
                frame->control  = data[1];
                frame->address  = data[2];
                frame->checksum = data[3];
                frame->stop     = data[4];
                
                frame->type = MBUS_FRAME_TYPE_SHORT;

                // verify the frame
                if (mbus_frame_verify(frame) != 0)
                {
                    return -3;
                }

                // successfully parsed data
                return 0;

            case MBUS_FRAME_LONG_START: // (also CONTROL)
            
                if (data_size < 3)
                {
                    // OK, got a valid long/control packet start, but we need
                    // more data to determine the length
                    return 3 - data_size;
                }
                
                // init frame data structure
                frame->start1   = data[0];
                frame->length1  = data[1];
                frame->length2  = data[2];
                
                if (frame->length1 < 3)
                {
                    snprintf(error_str, sizeof(error_str), "Invalid M-Bus frame length.");
                
                    // not a valid M-bus frame
                    return -2;
                }
                
                if (frame->length1 != frame->length2)
                {
                    snprintf(error_str, sizeof(error_str), "Invalid M-Bus frame length.");
                
                    // not a valid M-bus frame
                    return -2;
                }
                
                // check length of packet:
                len = frame->length1;
                
                if (data_size < (size_t)(MBUS_FRAME_FIXED_SIZE_LONG + len))
                {
                    // OK, but we need more data
                    return MBUS_FRAME_FIXED_SIZE_LONG + len - data_size;                
                }
                
                if (data_size > (size_t)(MBUS_FRAME_FIXED_SIZE_LONG + len))
                {
                    snprintf(error_str, sizeof(error_str), "Too much data in frame.");
                
                    // too much data... ?
                    return -2;
                }
                
                // we got the whole packet, continue parsing
                frame->start2   = data[3];
                frame->control  = data[4];
                frame->address  = data[5];
                frame->control_information = data[6];
                
                frame->data_size = len - 3;
                for (i = 0; i < frame->data_size; i++)
                {
                    frame->data[i] = data[7 + i];
                }                              
                                
                frame->checksum = data[data_size-2]; // data[6 + frame->data_size + 1]
                frame->stop     = data[data_size-1]; // data[6 + frame->data_size + 2]          
                
                if (frame->data_size == 0)
                {
                    frame->type = MBUS_FRAME_TYPE_CONTROL;
                }
                else
                {
                    frame->type = MBUS_FRAME_TYPE_LONG;                
                }

                // verify the frame
                if (mbus_frame_verify(frame) != 0)
                {
                    return -3;
                }

                // successfully parsed data
                return 0;        
            default:
                snprintf(error_str, sizeof(error_str), "Invalid M-Bus frame start.");
                
                // not a valid M-Bus frame header (start byte)
                return -4;
        }    
        
    }
    
    snprintf(error_str, sizeof(error_str), "Got null pointer to frame, data or zero data_size.");

    return -1;
}


//------------------------------------------------------------------------------
/// Parse the fixed-length data of a M-Bus frame
//------------------------------------------------------------------------------
int
mbus_data_fixed_parse(mbus_frame *frame, mbus_data_fixed *data)
{
    if (frame && data)
    {  
        // copy the fixed-length data structure
        memcpy((void *)data, (void *)(frame->data), sizeof(mbus_data_fixed));

        return 0;
    }    

    return -1;
}


//------------------------------------------------------------------------------
/// Parse the variable-length data of a M-Bus frame
//------------------------------------------------------------------------------
int
mbus_data_variable_parse(mbus_frame *frame, mbus_data_variable *data)
{
    mbus_data_record *record = NULL;
    size_t i, j;
    
    if (frame && data)
    {
        // parse header
        data->nrecords = 0;
        data->more_records_follow = 0;
        i = sizeof(mbus_data_variable_header);
        if(frame->data_size < i)
            return -1;

        // first copy the variable data fixed header
        memcpy((void *)&(data->header), (void *)(frame->data), i);

        data->record = NULL;

        while (i < frame->data_size)
        {
            // Skip filler dif=2F
            if ((frame->data[i] & 0xFF) == MBUS_DIB_DIF_IDLE_FILLER)
            {
              i++;
              continue;
            }

            if ((record = mbus_data_record_new()) == NULL)
            {
                // clean up...
                return (-2);
            }
            
            // copy timestamp
            memcpy((void *)&(record->timestamp), (void *)&(frame->timestamp), sizeof(time_t));

            // read and parse DIB (= DIF + DIFE)
        
            // DIF
            record->drh.dib.dif = frame->data[i];

            if ((record->drh.dib.dif == MBUS_DIB_DIF_MANUFACTURER_SPECIFIC) || 
                (record->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW))
            {
                if ((record->drh.dib.dif & 0xFF) == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW)
                {
                  data->more_records_follow = 1;
                }
                
                i++;
                // just copy the remaining data as it is vendor specific
                record->data_len = frame->data_size - i;
                for (j = 0; j < record->data_len; j++)
                {
                    record->data[j] = frame->data[i++];
                }

                // append the record and move on to next one
                mbus_data_record_append(data, record);
                data->nrecords++;
                continue;
            }
        
            // calculate length of data record         
            record->data_len = mbus_dif_datalength_lookup(record->drh.dib.dif);       

            // read DIF extensions
            record->drh.dib.ndife = 0;
            while (frame->data[i] & MBUS_DIB_DIF_EXTENSION_BIT && 
                   record->drh.dib.ndife < NITEMS(record->drh.dib.dife))
            {
                unsigned char dife = frame->data[i+1];
                record->drh.dib.dife[record->drh.dib.ndife] = dife;

                record->drh.dib.ndife++;
                i++;
            }
            i++;

            // read and parse VIB (= VIF + VIFE)

            // VIF
            record->drh.vib.vif = frame->data[i++];
            
            if ((record->drh.vib.vif & MBUS_DIB_VIF_WITHOUT_EXTENSION) == 0x7C)
            {
                // variable length VIF in ASCII format
                int var_vif_len;
                var_vif_len = frame->data[i++];
                if (i + var_vif_len > frame->data_size)
                {
                    mbus_data_record_free(record);
                    return -1;
                }
                mbus_data_str_decode(record->drh.vib.custom_vif, &(frame->data[i]), var_vif_len);
                i += var_vif_len;
            }
            
            // VIFE
            record->drh.vib.nvife = 0;
            
            if (record->drh.vib.vif & MBUS_DIB_VIF_EXTENSION_BIT)
            {
                record->drh.vib.vife[0] = frame->data[i];
                record->drh.vib.nvife++;
                
                while (frame->data[i] & MBUS_DIB_VIF_EXTENSION_BIT && 
                       record->drh.vib.nvife < NITEMS(record->drh.vib.vife))
                {
                    unsigned char vife = frame->data[i+1];
                    record->drh.vib.vife[record->drh.vib.nvife] = vife;
                
                    record->drh.vib.nvife++;
                    i++;
                }
                i++;
            }
                
            // re-calculate data length, if of variable length type
            if ((record->drh.dib.dif & 0x0F) == 0x0D) // flag for variable length data
            {
                if(frame->data[i] <= 0xBF)
                    record->data_len = frame->data[i++];
                else if(frame->data[i] >= 0xC0 && frame->data[i] <= 0xCF)
                    record->data_len = (frame->data[i++] - 0xC0) * 2;
                else if(frame->data[i] >= 0xD0 && frame->data[i] <= 0xDF)
                    record->data_len = (frame->data[i++] - 0xD0) * 2;
                else if(frame->data[i] >= 0xE0 && frame->data[i] <= 0xEF)
                    record->data_len = frame->data[i++] - 0xE0;
                else if(frame->data[i] >= 0xF0 && frame->data[i] <= 0xFA)
                    record->data_len = frame->data[i++] - 0xF0;
            }

            // copy data
            for (j = 0; j < record->data_len; j++)
            {
                record->data[j] = frame->data[i++];       
            }

            // append the record and move on to next one
            mbus_data_record_append(data, record);
            data->nrecords++;
        }
        
        return 0;
    }    
    
    return -1;
}

//------------------------------------------------------------------------------
/// Check the stype of the frame data (error, fixed or variable) and dispatch to the
/// corresponding parser function.
//------------------------------------------------------------------------------
int
mbus_frame_data_parse(mbus_frame *frame, mbus_frame_data *data)
{
    char direction;

    if (frame == NULL)
    {
        snprintf(error_str, sizeof(error_str), "Got null pointer to frame.");
        return -1;
    }
    
    if (data == NULL)
    {
        snprintf(error_str, sizeof(error_str), "Got null pointer to data.");
        return -1;
    }
    
    direction = (frame->control & MBUS_CONTROL_MASK_DIR);
    
    if (direction == MBUS_CONTROL_MASK_DIR_S2M)
    {
        if (frame->control_information == MBUS_CONTROL_INFO_ERROR_GENERAL)
        {
            data->type = MBUS_DATA_TYPE_ERROR;
            
            if (frame->data_size > 0)
            {
                data->error = (int) frame->data[0];
            }
            else
            {
                data->error = 0;
            }
            
            return 0;
        }
        else if (frame->control_information == MBUS_CONTROL_INFO_RESP_FIXED)
        {
            if (frame->data_size == 0)
            {
                snprintf(error_str, sizeof(error_str), "Got zero data_size.");
                
                return -1;
            }
        
            data->type = MBUS_DATA_TYPE_FIXED;
            return mbus_data_fixed_parse(frame, &(data->data_fix));
        }
        else if (frame->control_information == MBUS_CONTROL_INFO_RESP_VARIABLE)
        {
            if (frame->data_size == 0)
            {
                snprintf(error_str, sizeof(error_str), "Got zero data_size.");
                
                return -1;
            }
            
            data->type = MBUS_DATA_TYPE_VARIABLE;
            return mbus_data_variable_parse(frame, &(data->data_var));
        }
        else
        {
            snprintf(error_str, sizeof(error_str), "Unknown control information 0x%.2x", frame->control_information);
            
            return -1;
        }
    }
    
    snprintf(error_str, sizeof(error_str), "Wrong direction in frame (master to slave)");
        
    return -1;
}

//------------------------------------------------------------------------------
/// Pack the M-bus frame into a binary string representation that can be sent 
/// on the bus. The binary packet format is different for the different types
/// of M-bus frames.
//------------------------------------------------------------------------------
int
mbus_frame_pack(mbus_frame *frame, unsigned char *data, size_t data_size)
{
    size_t i, offset = 0;
    
    if (frame && data)
    {   
        if (mbus_frame_calc_length(frame) == -1)
        {
            return -2;
        }

        if (mbus_frame_calc_checksum(frame) == -1)
        {
            return -3;
        }
        
        switch (frame->type)
        {
            case MBUS_FRAME_TYPE_ACK:
            
                if (data_size < MBUS_FRAME_ACK_BASE_SIZE)
                {
                    return -4;
                }
        
                data[offset++] = frame->start1; 
        
                return offset;        

            case MBUS_FRAME_TYPE_SHORT:

                if (data_size < MBUS_FRAME_SHORT_BASE_SIZE)
                {
                    return -4;
                }
                
                data[offset++] = frame->start1;
                data[offset++] = frame->control;
                data[offset++] = frame->address;
                data[offset++] = frame->checksum;
                data[offset++] = frame->stop;
        
                return offset;        

            case MBUS_FRAME_TYPE_CONTROL:
        
                if (data_size < MBUS_FRAME_CONTROL_BASE_SIZE)
                {
                    return -4;
                }
                
                data[offset++] = frame->start1;
                data[offset++] = frame->length1;
                data[offset++] = frame->length2;
                data[offset++] = frame->start2;

                data[offset++] = frame->control;
                data[offset++] = frame->address;
                data[offset++] = frame->control_information;

                data[offset++] = frame->checksum;
                data[offset++] = frame->stop;    
        
                return offset;        

            case MBUS_FRAME_TYPE_LONG:

                if (data_size < frame->data_size + MBUS_FRAME_LONG_BASE_SIZE)
                {
                    return -4;
                }

                data[offset++] = frame->start1;
                data[offset++] = frame->length1;
                data[offset++] = frame->length2;
                data[offset++] = frame->start2;

                data[offset++] = frame->control;
                data[offset++] = frame->address;
                data[offset++] = frame->control_information;

                for (i = 0; i < frame->data_size; i++)
                {
                    data[offset++] = frame->data[i];
                }
                
                data[offset++] = frame->checksum;
                data[offset++] = frame->stop;    
        
                return offset;      
            
            default:
                return -5;
        }        
    }

    return -1;
}


//------------------------------------------------------------------------------
/// pack the data stuctures into frame->data
//------------------------------------------------------------------------------
int
mbus_frame_internal_pack(mbus_frame *frame, mbus_frame_data *frame_data)
{
    mbus_data_record *record;
    int j;

    if (frame == NULL || frame_data == NULL)
        return -1;

    frame->data_size = 0;

    switch (frame_data->type)
    {
        case MBUS_DATA_TYPE_ERROR:
        
            frame->data[frame->data_size++] = (char) frame_data->error;
        
            break;
    
        case MBUS_DATA_TYPE_FIXED:

            //
            // pack fixed data structure
            //
            frame->data[frame->data_size++] = frame_data->data_fix.id_bcd[0];
            frame->data[frame->data_size++] = frame_data->data_fix.id_bcd[1];
            frame->data[frame->data_size++] = frame_data->data_fix.id_bcd[2];
            frame->data[frame->data_size++] = frame_data->data_fix.id_bcd[3];
            frame->data[frame->data_size++] = frame_data->data_fix.tx_cnt;
            frame->data[frame->data_size++] = frame_data->data_fix.status;
            frame->data[frame->data_size++] = frame_data->data_fix.cnt1_type;
            frame->data[frame->data_size++] = frame_data->data_fix.cnt2_type;
            frame->data[frame->data_size++] = frame_data->data_fix.cnt1_val[0];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt1_val[1];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt1_val[2];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt1_val[3];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt2_val[0];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt2_val[1];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt2_val[2];
            frame->data[frame->data_size++] = frame_data->data_fix.cnt2_val[3];

            break;

        case MBUS_DATA_TYPE_VARIABLE:

            //
            // first pack variable data structure header
            //
            frame->data[frame->data_size++] = frame_data->data_var.header.id_bcd[0];
            frame->data[frame->data_size++] = frame_data->data_var.header.id_bcd[1];
            frame->data[frame->data_size++] = frame_data->data_var.header.id_bcd[2];
            frame->data[frame->data_size++] = frame_data->data_var.header.id_bcd[3];
            frame->data[frame->data_size++] = frame_data->data_var.header.manufacturer[0];
            frame->data[frame->data_size++] = frame_data->data_var.header.manufacturer[1];
            frame->data[frame->data_size++] = frame_data->data_var.header.version;
            frame->data[frame->data_size++] = frame_data->data_var.header.medium;
            frame->data[frame->data_size++] = frame_data->data_var.header.access_no;
            frame->data[frame->data_size++] = frame_data->data_var.header.status;
            frame->data[frame->data_size++] = frame_data->data_var.header.signature[0];
            frame->data[frame->data_size++] = frame_data->data_var.header.signature[1];

            //
            // pack all data records
            //
            for (record = frame_data->data_var.record; record; record = record->next)
            {
                // pack DIF
                if (parse_debug)
                    printf("%s: packing DIF [%zu]", __PRETTY_FUNCTION__, frame->data_size);
                frame->data[frame->data_size++] = record->drh.dib.dif;
                for (j = 0; j < record->drh.dib.ndife; j++)
                {
                    frame->data[frame->data_size++] = record->drh.dib.dife[j];
                }

                // pack VIF
                if (parse_debug)
                    printf("%s: packing VIF [%zu]", __PRETTY_FUNCTION__, frame->data_size);
                frame->data[frame->data_size++] = record->drh.vib.vif;
                for (j = 0; j < record->drh.vib.nvife; j++)
                {
                    frame->data[frame->data_size++] = record->drh.vib.vife[j];
                }

                // pack data
                if (parse_debug) 
                    printf("%s: packing data [%zu : %zu]", __PRETTY_FUNCTION__, frame->data_size, record->data_len);
                for (j = 0; j < record->data_len; j++)
                {
                    frame->data[frame->data_size++] = record->data[j];
                }
            }         
            
            break;

        default:
            return -2;
    }

    return 0;
}

//------------------------------------------------------------------------------
//
// Print/Dump functions
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Switch parse debugging 
//------------------------------------------------------------------------------
void
mbus_parse_set_debug(int debug)
{
    parse_debug = debug;
}

//------------------------------------------------------------------------------
/// Dump frame in HEX on standard output
//------------------------------------------------------------------------------
int
mbus_frame_print(mbus_frame *frame)
{
    mbus_frame *iter;
    unsigned char data_buff[256];
    int len, i;

    if (frame == NULL)
        return -1;

    for (iter = frame; iter; iter = iter->next)
    {
        if ((len = mbus_frame_pack(iter, data_buff, sizeof(data_buff))) == -1)
        {
            return -2;
        }

        printf("%s: Dumping M-Bus frame [type %d, %d bytes]: ", __PRETTY_FUNCTION__, iter->type, len);
        for (i = 0; i < len; i++)
        {
            printf("%.2X ", data_buff[i]);
        }
        printf("\n");
    }

    return 0;
}

//------------------------------------------------------------------------------
///
/// Print the data part of a frame.
///
//------------------------------------------------------------------------------
int
mbus_frame_data_print(mbus_frame_data *data)
{
    if (data)
    {
        if (data->type == MBUS_DATA_TYPE_ERROR)
        {
            return mbus_data_error_print(data->error); 
        }
    
        if (data->type == MBUS_DATA_TYPE_FIXED)
        {
            return mbus_data_fixed_print(&(data->data_fix));
        }
        
        if (data->type == MBUS_DATA_TYPE_VARIABLE)
        {
            return mbus_data_variable_print(&(data->data_var));
        }
    }
    
    return -1;
}

//------------------------------------------------------------------------------
/// Print M-bus frame info to stdout
//------------------------------------------------------------------------------
int
mbus_data_variable_header_print(mbus_data_variable_header *header)
{
    if (header)
    {
        printf("%s: ID           = %lld\n", __PRETTY_FUNCTION__, 
               mbus_data_bcd_decode(header->id_bcd, 4));

        printf("%s: Manufacturer = 0x%.2X%.2X\n", __PRETTY_FUNCTION__,
               header->manufacturer[1], header->manufacturer[0]);
               
        printf("%s: Manufacturer = %s\n", __PRETTY_FUNCTION__,
               mbus_decode_manufacturer(header->manufacturer[0], header->manufacturer[1]));               

        printf("%s: Version      = 0x%.2X\n", __PRETTY_FUNCTION__, header->version);
        printf("%s: Medium       = %s (0x%.2X)\n", __PRETTY_FUNCTION__, mbus_data_variable_medium_lookup(header->medium), header->medium);
        printf("%s: Access #     = 0x%.2X\n", __PRETTY_FUNCTION__, header->access_no);
        printf("%s: Status       = 0x%.2X\n", __PRETTY_FUNCTION__, header->status);
        printf("%s: Signature    = 0x%.2X%.2X\n", __PRETTY_FUNCTION__,
               header->signature[1], header->signature[0]);

    }
    
    return -1;
}

int
mbus_data_variable_print(mbus_data_variable *data)
{
    mbus_data_record *record;
    size_t j;
    
    if (data)
    {   
        mbus_data_variable_header_print(&(data->header));
    
        for (record = data->record; record; record = record->next)
        {
            // DIF
            printf("DIF           = %.2X\n", record->drh.dib.dif);
            printf("DIF.Extension = %s\n",  (record->drh.dib.dif & MBUS_DIB_DIF_EXTENSION_BIT) ? "Yes":"No");
            printf("DIF.Function  = %s\n",  (record->drh.dib.dif & 0x30) ? "Minimum value" : "Instantaneous value" );
            printf("DIF.Data      = %.2X\n", record->drh.dib.dif & 0x0F);

            // VENDOR SPECIFIC
            if ((record->drh.dib.dif == MBUS_DIB_DIF_MANUFACTURER_SPECIFIC) || 
                (record->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW)) //MBUS_DIB_DIF_VENDOR_SPECIFIC
            {
                printf("%s: VENDOR DATA [size=%zd] = ", __PRETTY_FUNCTION__, record->data_len);
                for (j = 0; j < record->data_len; j++)
                {
                    printf("%.2X ", record->data[j]);        
                }
                printf("\n");
                
                if (record->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW)
                {
                  printf("%s: More records follow in next telegram\n", __PRETTY_FUNCTION__);
                }
                continue;
            }
        
            // calculate length of data record
            printf("DATA LENGTH = %zd\n", record->data_len);

            // DIFE
            for (j = 0; j < record->drh.dib.ndife; j++)
            {
                unsigned char dife = record->drh.dib.dife[j];
                
                printf("DIFE[%zd]           = %.2X\n", j,  dife);
                printf("DIFE[%zd].Extension = %s\n",   j, (dife & MBUS_DIB_DIF_EXTENSION_BIT) ? "Yes" : "No");        
                printf("DIFE[%zd].Function  = %s\n",   j, (dife & 0x30) ? "Minimum value" : "Instantaneous value" );
                printf("DIFE[%zd].Data      = %.2X\n", j,  dife & 0x0F);            
            }
   
            // VIF
            printf("VIF           = %.2X\n", record->drh.vib.vif);
            printf("VIF.Extension = %s\n",  (record->drh.vib.vif & MBUS_DIB_VIF_EXTENSION_BIT) ? "Yes":"No");
            printf("VIF.Value     = %.2X\n", record->drh.vib.vif & MBUS_DIB_VIF_WITHOUT_EXTENSION);
            
            // VIFE
            for (j = 0; j < record->drh.vib.nvife; j++)
            {
                unsigned char vife = record->drh.vib.vife[j];
                
                printf("VIFE[%zd]           = %.2X\n", j,  vife);
                printf("VIFE[%zd].Extension = %s\n",   j, (vife & MBUS_DIB_VIF_EXTENSION_BIT) ? "Yes" : "No");
                printf("VIFE[%zd].Value     = %.2X\n", j,  vife & MBUS_DIB_VIF_WITHOUT_EXTENSION);            
            }
            
            printf("\n");
        }
    }
    
    return -1;
}

int
mbus_data_fixed_print(mbus_data_fixed *data)
{
    if (data)
    {    
        printf("%s: ID       = %d\n", __PRETTY_FUNCTION__, (int)mbus_data_bcd_decode(data->id_bcd, 4));
        printf("%s: Access # = 0x%.2X\n", __PRETTY_FUNCTION__, data->tx_cnt);
        printf("%s: Status   = 0x%.2X\n", __PRETTY_FUNCTION__, data->status);
        printf("%s: Function = %s\n", __PRETTY_FUNCTION__, mbus_data_fixed_function(data->status));
        
        printf("%s: Medium1  = %s\n", __PRETTY_FUNCTION__, mbus_data_fixed_medium(data));
        printf("%s: Unit1    = %s\n", __PRETTY_FUNCTION__, mbus_data_fixed_unit(data->cnt1_type));
        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD)
        {
            printf("%s: Counter1 = %d\n", __PRETTY_FUNCTION__, (int)mbus_data_bcd_decode(data->cnt1_val, 4));
        }
        else
        {
            printf("%s: Counter1 = %d\n", __PRETTY_FUNCTION__, mbus_data_int_decode(data->cnt1_val, 4));
        }

        printf("%s: Medium2  = %s\n", __PRETTY_FUNCTION__, mbus_data_fixed_medium(data));
        printf("%s: Unit2    = %s\n", __PRETTY_FUNCTION__, mbus_data_fixed_unit(data->cnt2_type));
        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD)
        {
            printf("%s: Counter2 = %d\n", __PRETTY_FUNCTION__, (int)mbus_data_bcd_decode(data->cnt2_val, 4));
        }
        else
        {
            printf("%s: Counter2 = %d\n", __PRETTY_FUNCTION__, mbus_data_int_decode(data->cnt2_val, 4));        
        }          
    }
    
    return -1;
}

void
mbus_hex_dump(const char *label, const char *buff, size_t len)
{
    time_t rawtime;
    struct tm * timeinfo;
    char timestamp[21];
    size_t i;
    
    if (label == NULL || buff == NULL)
        return;
    
    time ( &rawtime );
    timeinfo = gmtime ( &rawtime );
    
    strftime(timestamp,20,"%Y-%m-%d %H:%M:%S",timeinfo);
    fprintf(stderr, "[%s] %s (%03zu):", timestamp, label, len);
    
    for (i = 0; i < len; i++)
    {
       fprintf(stderr, " %02X", (unsigned char) buff[i]);
    }
    
    fprintf(stderr, "\n");
}

int
mbus_data_error_print(int error)
{   
    printf("%s: Error = %d\n", __PRETTY_FUNCTION__, error);
    
    return -1;
}

//------------------------------------------------------------------------------
//
// XML RELATED FUNCTIONS
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
/// Encode string to XML
///
//------------------------------------------------------------------------------
void
mbus_str_xml_encode(unsigned char *dst, const unsigned char *src, size_t max_len)
{
    size_t i, len;
    
    i = 0;
    len = 0;
    
    if (dst == NULL)
        return;
    
    if (src != NULL)
    {
        while((len+6) < max_len) 
        {
            if (src[i] == '\0')
            {
                break;
            }
            
            if (iscntrl(src[i]))
            {
                // convert all control chars into spaces
                dst[len++] = ' ';
            }
            else
            {
                switch (src[i])
                {
                    case '&':
                        len += snprintf(&dst[len], max_len - len, "&amp;");
                        break;
                    case '<':
                        len += snprintf(&dst[len], max_len - len, "&lt;");
                        break;
                    case '>':
                        len += snprintf(&dst[len], max_len - len, "&gt;");
                        break;
                    case '"':
                        len += snprintf(&dst[len], max_len - len, "&quot;");
                        break;
                    default:
                        dst[len++] = src[i];
                        break;
                }
            }
        
            i++;
        }
    }
    
    dst[len] = '\0';
}

//------------------------------------------------------------------------------
/// Generate XML for the variable-length data header
//------------------------------------------------------------------------------
char *
mbus_data_variable_header_xml(mbus_data_variable_header *header)
{
    static char buff[8192];
    char str_encoded[768];
    size_t len = 0;
    int val;
    
    if (header)
    {
        len += snprintf(&buff[len], sizeof(buff) - len, "    <SlaveInformation>\n");

        val = (int)mbus_data_bcd_decode(header->id_bcd, 4);
        
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Id>%d</Id>\n", val);
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Manufacturer>%s</Manufacturer>\n",
                mbus_decode_manufacturer(header->manufacturer[0], header->manufacturer[1]));               
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Version>%d</Version>\n", header->version);
        
        mbus_str_xml_encode(str_encoded, mbus_data_product_name(header), sizeof(str_encoded));
        
        len += snprintf(&buff[len], sizeof(buff) - len, "        <ProductName>%s</ProductName>\n", str_encoded);
        
        mbus_str_xml_encode(str_encoded, mbus_data_variable_medium_lookup(header->medium), sizeof(str_encoded)); 
        
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Medium>%s</Medium>\n", str_encoded);
        len += snprintf(&buff[len], sizeof(buff) - len, "        <AccessNumber>%d</AccessNumber>\n", header->access_no);
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Status>%.2X</Status>\n", header->status);
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Signature>%.2X%.2X</Signature>\n", header->signature[1], header->signature[0]);

        len += snprintf(&buff[len], sizeof(buff) - len, "    </SlaveInformation>\n\n");

        return buff;
    }
    
    return ""; 
}

//------------------------------------------------------------------------------
/// Generate XML for a single variable-length data record
//------------------------------------------------------------------------------
char *
mbus_data_variable_record_xml(mbus_data_record *record, int record_cnt, int frame_cnt, mbus_data_variable_header *header)
{
    static char buff[8192];
    char str_encoded[768];
    size_t len = 0;
    struct tm * timeinfo;
    char timestamp[21];
    
    if (record)
    {
        if (frame_cnt >= 0)
        {
            len += snprintf(&buff[len], sizeof(buff) - len, 
                            "    <DataRecord id=\"%d\" frame=\"%d\">\n",
                            record_cnt, frame_cnt);
        }
        else
        {
            len += snprintf(&buff[len], sizeof(buff) - len, 
                            "    <DataRecord id=\"%d\">\n", record_cnt);
        }
    
        if (record->drh.dib.dif == MBUS_DIB_DIF_MANUFACTURER_SPECIFIC) // MBUS_DIB_DIF_VENDOR_SPECIFIC
        {
            len += snprintf(&buff[len], sizeof(buff) - len, 
                            "        <Function>Manufacturer specific</Function>\n");                
        }
        else if (record->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW)
        {
            len += snprintf(&buff[len], sizeof(buff) - len, 
                            "        <Function>More records follow</Function>\n");
        }
        else
        {   
            mbus_str_xml_encode(str_encoded, mbus_data_record_function(record), sizeof(str_encoded)); 
            len += snprintf(&buff[len], sizeof(buff) - len, 
                            "        <Function>%s</Function>\n", str_encoded);
            
            mbus_str_xml_encode(str_encoded, mbus_data_record_unit(record), sizeof(str_encoded));
            len += snprintf(&buff[len], sizeof(buff) - len, 
                            "        <Unit>%s</Unit>\n", str_encoded);
        }
        
        mbus_str_xml_encode(str_encoded, mbus_data_record_value(record), sizeof(str_encoded));
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Value>%s</Value>\n", str_encoded);
        
        timeinfo = gmtime ( &(record->timestamp) );
        strftime(timestamp,20,"%Y-%m-%dT%H:%M:%S",timeinfo);
        len += snprintf(&buff[len], sizeof(buff) - len, "        <Timestamp>%s</Timestamp>\n", timestamp);
        
        len += snprintf(&buff[len], sizeof(buff) - len, "    </DataRecord>\n\n");
            
        return buff;
    }
    
    return ""; 
}

//------------------------------------------------------------------------------
/// Generate XML for variable-length data 
//------------------------------------------------------------------------------
char *
mbus_data_variable_xml(mbus_data_variable *data)
{
    mbus_data_record *record;
    char *buff = NULL, *new_buff;
    size_t len = 0, buff_size = 8192;
    int i;
    
    if (data)
    {
        buff = (char*) malloc(buff_size);
        
        if (buff == NULL)
            return NULL;
    
        len += snprintf(&buff[len], buff_size - len, "<MBusData>\n\n");
        
        len += snprintf(&buff[len], buff_size - len, "%s", 
                        mbus_data_variable_header_xml(&(data->header)));
    
        for (record = data->record, i = 0; record; record = record->next, i++)
        {
            if ((buff_size - len) < 1024)
            {
                buff_size *= 2;
                new_buff = (char*) realloc(buff,buff_size);
                
                if (new_buff == NULL)
                {
                    free(buff);
                    return NULL;
                }
                
                buff = new_buff;
            }
        
            len += snprintf(&buff[len], buff_size - len, "%s", 
                            mbus_data_variable_record_xml(record, i, -1, &(data->header)));        
        }       
        len += snprintf(&buff[len], buff_size - len, "</MBusData>\n");

        return buff;
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
/// Generate XML representation of fixed-length frame.
//------------------------------------------------------------------------------
char *
mbus_data_fixed_xml(mbus_data_fixed *data)
{
    char *buff = NULL;
    char str_encoded[256];
    size_t len = 0, buff_size = 8192;

    if (data)
    {
        buff = (char*) malloc(buff_size);
        
        if (buff == NULL)
            return NULL;
    
        len += snprintf(&buff[len], buff_size - len, "<MBusData>\n\n");
    
        len += snprintf(&buff[len], buff_size - len, "    <SlaveInformation>\n");
        len += snprintf(&buff[len], buff_size - len, "        <Id>%d</Id>\n", (int)mbus_data_bcd_decode(data->id_bcd, 4));
        
        mbus_str_xml_encode(str_encoded, mbus_data_fixed_medium(data), sizeof(str_encoded)); 
        len += snprintf(&buff[len], buff_size - len, "        <Medium>%s</Medium>\n", str_encoded);
        
        len += snprintf(&buff[len], buff_size - len, "        <AccessNumber>%d</AccessNumber>\n", data->tx_cnt);
        len += snprintf(&buff[len], buff_size - len, "        <Status>%.2X</Status>\n", data->status);
        len += snprintf(&buff[len], buff_size - len, "    </SlaveInformation>\n\n");
             
        len += snprintf(&buff[len], buff_size - len, "    <DataRecord id=\"0\">\n");
        
        mbus_str_xml_encode(str_encoded, mbus_data_fixed_function(data->status), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        <Function>%s</Function>\n", str_encoded);
        
        mbus_str_xml_encode(str_encoded, mbus_data_fixed_unit(data->cnt1_type), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        <Unit>%s</Unit>\n", str_encoded);
        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD)
        {
            len += snprintf(&buff[len], buff_size - len, "        <Value>%d</Value>\n", (int)mbus_data_bcd_decode(data->cnt1_val, 4));
        }
        else
        {
            len += snprintf(&buff[len], buff_size - len, "        <Value>%d</Value>\n", mbus_data_int_decode(data->cnt1_val, 4));
        }
        len += snprintf(&buff[len], buff_size - len, "    </DataRecord>\n\n");      

        len += snprintf(&buff[len], buff_size - len, "    <DataRecord id=\"1\">\n");
        
        mbus_str_xml_encode(str_encoded, mbus_data_fixed_function(data->status), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        <Function>%s</Function>\n", str_encoded);
        
        mbus_str_xml_encode(str_encoded, mbus_data_fixed_unit(data->cnt2_type), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        <Unit>%s</Unit>\n", str_encoded);
        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD)
        {
            len += snprintf(&buff[len], buff_size - len, "        <Value>%d</Value>\n", (int)mbus_data_bcd_decode(data->cnt2_val, 4));
        }
        else
        {
            len += snprintf(&buff[len], buff_size - len, "        <Value>%d</Value>\n", mbus_data_int_decode(data->cnt2_val, 4));
        }
        len += snprintf(&buff[len], buff_size - len, "    </DataRecord>\n\n");      

        len += snprintf(&buff[len], buff_size - len, "</MBusData>\n");

        return buff;
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
/// Generate XML representation of a general application error.
//------------------------------------------------------------------------------
char *
mbus_data_error_xml(int error)
{
    char *buff = NULL;
    char str_encoded[256];
    size_t len = 0, buff_size = 8192;
    
    buff = (char*) malloc(buff_size);
        
    if (buff == NULL)
        return NULL;

    len += snprintf(&buff[len], buff_size - len, "<MBusData>\n\n");
    
    len += snprintf(&buff[len], buff_size - len, "    <SlaveInformation>\n");

    mbus_str_xml_encode(str_encoded, mbus_data_error_lookup(error), sizeof(str_encoded)); 
    len += snprintf(&buff[len], buff_size - len, "        <Error>%s</Error>\n", str_encoded);
    
    len += snprintf(&buff[len], buff_size - len, "    </SlaveInformation>\n\n");
    
    len += snprintf(&buff[len], buff_size - len, "</MBusData>\n");
    
    return buff;
}

//------------------------------------------------------------------------------
/// Return a string containing an XML representation of the M-BUS frame data.
//------------------------------------------------------------------------------
char *
mbus_frame_data_xml(mbus_frame_data *data)
{
    if (data)
    {
        if (data->type == MBUS_DATA_TYPE_ERROR)
        {
            return mbus_data_error_xml(data->error);
        }
    
        if (data->type == MBUS_DATA_TYPE_FIXED)
        {
            return mbus_data_fixed_xml(&(data->data_fix));
        }
        
        if (data->type == MBUS_DATA_TYPE_VARIABLE)
        {
            return mbus_data_variable_xml(&(data->data_var));
        }
    }
    
    return NULL;
}


//------------------------------------------------------------------------------
/// Return an XML representation of the M-BUS frame.
//------------------------------------------------------------------------------
char *
mbus_frame_xml(mbus_frame *frame)
{
    mbus_frame_data frame_data;
    mbus_frame *iter;
     
    mbus_data_record *record;
    char *buff = NULL, *new_buff;

    size_t len = 0, buff_size = 8192;
    int record_cnt = 0, frame_cnt;

    if (frame)
    {
        memset((void *)&frame_data, 0, sizeof(mbus_frame_data));
    
        if (mbus_frame_data_parse(frame, &frame_data) == -1)
        {
            mbus_error_str_set("M-bus data parse error.");
            return NULL;
        }
    
        if (frame_data.type == MBUS_DATA_TYPE_ERROR)
        {
            //
            // generate XML for error
            //
            return mbus_data_error_xml(frame_data.error);
        }
    
        if (frame_data.type == MBUS_DATA_TYPE_FIXED)
        {
            //
            // generate XML for fixed data 
            //
            return mbus_data_fixed_xml(&(frame_data.data_fix));
        }
        
        if (frame_data.type == MBUS_DATA_TYPE_VARIABLE)
        {
            //
            // generate XML for a sequence of variable data frames
            //
            
            buff = (char*) malloc(buff_size);
        
            if (buff == NULL)
            {
                mbus_data_record_free(frame_data.data_var.record);
                return NULL;
            }        

            // include frame counter in XML output if more than one frame
            // is available (frame_cnt = -1 => not included in output)        
            frame_cnt = (frame->next == NULL) ? -1 : 0;

            len += snprintf(&buff[len], buff_size - len, "<MBusData>\n\n");
            
            // only print the header info for the first frame (should be 
            // the same for each frame in a sequence of a multi-telegram 
            // transfer.
            len += snprintf(&buff[len], buff_size - len, "%s", 
                                    mbus_data_variable_header_xml(&(frame_data.data_var.header)));
                                    
            // loop through all records in the current frame, using a global
            // record count as record ID in the XML output
            for (record = frame_data.data_var.record; record; record = record->next, record_cnt++)
            {
                if ((buff_size - len) < 1024)
                {
                    buff_size *= 2;
                    new_buff = (char*) realloc(buff,buff_size);
                
                    if (new_buff == NULL)
                    {
                        free(buff);
                        mbus_data_record_free(frame_data.data_var.record);
                        return NULL;
                    }
                    
                    buff = new_buff;
                }
            
                len += snprintf(&buff[len], buff_size - len, "%s", 
                                mbus_data_variable_record_xml(record, record_cnt, frame_cnt, &(frame_data.data_var.header)));        
            }       

            // free all records in the list
            if (frame_data.data_var.record)
            {
                mbus_data_record_free(frame_data.data_var.record); 
            }
            
            frame_cnt++;
                        
            for (iter = frame->next; iter; iter = iter->next, frame_cnt++)
            {
                if (mbus_frame_data_parse(iter, &frame_data) == -1)
                {
                    mbus_error_str_set("M-bus variable data parse error.");
                    return NULL;
                }
            
                // loop through all records in the current frame, using a global
                // record count as record ID in the XML output
                for (record = frame_data.data_var.record; record; record = record->next, record_cnt++)
                {
                    if ((buff_size - len) < 1024)
                    {
                        buff_size *= 2;
                        new_buff = (char*) realloc(buff,buff_size);
                        
                        if (new_buff == NULL)
                        {
                            free(buff);
                            mbus_data_record_free(frame_data.data_var.record);
                            return NULL;
                        }
                        
                        buff = new_buff;
                    }
                
                    len += snprintf(&buff[len], buff_size - len, "%s", 
                                    mbus_data_variable_record_xml(record, record_cnt, frame_cnt, &(frame_data.data_var.header)));        
                }       

                // free all records in the list
                if (frame_data.data_var.record)
                {
                    mbus_data_record_free(frame_data.data_var.record); 
                }
            } 
        
            len += snprintf(&buff[len], buff_size - len, "</MBusData>\n");
                
            return buff;
        }
    }
        
    return NULL;
}    


//------------------------------------------------------------------------------
/// Allocate and initialize a new frame data structure
//------------------------------------------------------------------------------
mbus_frame_data *
mbus_frame_data_new()
{
    mbus_frame_data *data;

    if ((data = (mbus_frame_data *)malloc(sizeof(mbus_frame_data))) == NULL)
    {
        return NULL;
    }
    
    memset(data, 0, sizeof(mbus_frame_data));
    
    data->data_var.data = NULL;
    data->data_var.record = NULL;

    return data;
}


//-----------------------------------------------------------------------------
/// Free up data associated with a frame data structure
//------------------------------------------------------------------------------
void
mbus_frame_data_free(mbus_frame_data *data)
{
    if (data)
    {
        if (data->data_var.record)
        {
            mbus_data_record_free(data->data_var.record); // free's up the whole list
        }

        free(data);
    }
}



//------------------------------------------------------------------------------
/// Allocate and initialize a new variable data record
//------------------------------------------------------------------------------
mbus_data_record *
mbus_data_record_new()
{
    mbus_data_record *record;

    if ((record = (mbus_data_record *)malloc(sizeof(mbus_data_record))) == NULL)
    {
        return NULL;
    }

    memset(record, 0, sizeof(mbus_data_record));

    record->next = NULL;
    return record;    
}

//------------------------------------------------------------------------------
/// free up memory associated with a data record and all the subsequent records
/// in its list (apply recursively)
//------------------------------------------------------------------------------
void
mbus_data_record_free(mbus_data_record *record)
{
    if (record)
    {
        mbus_data_record *next = record->next;

        free(record);

        if (next)
            mbus_data_record_free(next);
    }
}

//------------------------------------------------------------------------------
/// Return a string containing an XML representation of the M-BUS frame.
//------------------------------------------------------------------------------
void
mbus_data_record_append(mbus_data_variable *data, mbus_data_record *record)
{
    mbus_data_record *iter;

    if (data && record)
    {
        if (data->record == NULL)
        {
            data->record = record;
        }
        else
        {
            // find the end of the list
            for (iter = data->record; iter->next; iter = iter->next);

            iter->next = record;
        }
    }
}

//------------------------------------------------------------------------------
// Extract the secondary address from an M-Bus frame. The secondary address
// should be a 16 character string comprised of the device ID (4 bytes),
// manufacturer ID (2 bytes), version (1 byte) and medium (1 byte).
//------------------------------------------------------------------------------
char *
mbus_frame_get_secondary_address(mbus_frame *frame)
{
    static char addr[32];
    mbus_frame_data *data;
    long id;

    if (frame == NULL || (data = mbus_frame_data_new()) == NULL)
    {
        printf("%s: Failed to allocate data structure [%p, %p].\n", __PRETTY_FUNCTION__, (void*)frame, (void*)data);
        return NULL;
    }

    if (frame->control_information != MBUS_CONTROL_INFO_RESP_VARIABLE)
    {
        snprintf(error_str, sizeof(error_str), "Non-variable data response (can't get secondary address from response).");
        return NULL;
    }

    if (mbus_frame_data_parse(frame, data) == -1)
    {
        return NULL;
    }

    id = (long) mbus_data_bcd_decode(data->data_var.header.id_bcd, 4);

    snprintf(addr, sizeof(addr), "%08lu%02X%02X%02X%02X",
             id,
             data->data_var.header.manufacturer[0],
             data->data_var.header.manufacturer[1],
             data->data_var.header.version,
             data->data_var.header.medium);
       
    // free data
    mbus_frame_data_free(data);
 
    return addr;
}

//------------------------------------------------------------------------------
// Pack a secondary address string into an mbus frame
//------------------------------------------------------------------------------
int
mbus_frame_select_secondary_pack(mbus_frame *frame, char *address)
{
    int val, i, j, k;
    char tmp[16];

    if (frame == NULL || address == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: frame or address arguments are NULL.", __PRETTY_FUNCTION__);
        return -1;
    }
    
    if (mbus_is_secondary_address(address) == 0)
    {
        snprintf(error_str, sizeof(error_str), "%s: address is invalid.", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control  = MBUS_CONTROL_MASK_SND_UD | MBUS_CONTROL_MASK_DIR_M2S | MBUS_CONTROL_MASK_FCB;
    frame->address  = MBUS_ADDRESS_NETWORK_LAYER;             // for addressing secondary slaves 
    frame->control_information = MBUS_CONTROL_INFO_SELECT_SLAVE; // mode 1
    
    frame->data_size = 8;

    // parse secondary_addr_str and populate frame->data[0-7]
    // ex: secondary_addr_str = "14491001 1057 01 06" 
    // (excluding the blank spaces)

    strncpy(tmp, &address[14], 2); tmp[2] = 0;
    val = strtol(tmp, NULL, 16);
    frame->data[7] = val & 0xFF;

    strncpy(tmp, &address[12], 2); tmp[2] = 0;
    val = strtol(tmp, NULL, 16);
    frame->data[6] = val & 0xFF;

    strncpy(tmp,  &address[8], 4); tmp[4] = 0;
    val = strtol(tmp, NULL, 16);
    frame->data[4] = (val>>8) & 0xFF;
    frame->data[5] =  val     & 0xFF;

    // parse the ID string, allowing for F wildcard characters.
    frame->data[0] = 0;
    frame->data[1] = 0;
    frame->data[2] = 0;
    frame->data[3] = 0;
    j = 3; k = 1;
    for (i = 0; i < 8; i++)
    {
        if (address[i] == 'F' || address[i] == 'f')
        {
            frame->data[j] |= 0x0F << (4 * k--);
        }
        else
        {
            frame->data[j] |= (0x0F & (address[i] - '0')) << (4 * k--);
        }

        if (k < 0)
        {
            k = 1; j--;
        }
    }

    return 0;
}

//---------------------------------------------------------
// Checks if an integer is a valid primary address.
//---------------------------------------------------------
int
mbus_is_primary_address(int value)
{
    return ((value >= 0x00) && (value <= 0xFF));
}

//---------------------------------------------------------
// Checks if an string is a valid secondary address.
//---------------------------------------------------------
int
mbus_is_secondary_address(const char * value)
{
    int i;
    
    if (value == NULL)
        return 0;
        
    if (strlen(value) != 16)
        return 0;
        
    for (i = 0; i < 16; i++)
    {
        if (!isxdigit(value[i]))
            return 0;
    }
    
    return 1;
}
