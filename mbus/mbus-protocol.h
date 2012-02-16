//------------------------------------------------------------------------------
// Copyright (C) 2010-2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// FreeSCADA 
// http://www.FreeSCADA.com
// freescada@freescada.com
//
//------------------------------------------------------------------------------

/**
 * @file   mbus-protocol.h
 * 
 * @brief  Functions and data structures for M-Bus protocol parsing.
 *
 */

#ifndef _MBUS_PROTOCOL_H_
#define _MBUS_PROTOCOL_H_

#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

//
// Packet formats:
//
// ACK: size = 1 byte
//
//   byte1: ack   = 0xE5
//
// SHORT: size = 5 byte
//
//   byte1: start   = 0x10
//   byte2: control = ...
//   byte3: address = ...
//   byte4: chksum  = ...
//   byte5: stop    = 0x16
//
// CONTROL: size = 9 byte
//
//   byte1: start1  = 0x68
//   byte2: length1 = ...
//   byte3: length2 = ...
//   byte4: start2  = 0x68
//   byte5: control = ...
//   byte6: address = ...
//   byte7: ctl.info= ...
//   byte8: chksum  = ...
//   byte9: stop    = 0x16
//
// LONG: size = N >= 9 byte
//
//   byte1: start1  = 0x68
//   byte2: length1 = ...
//   byte3: length2 = ...
//   byte4: start2  = 0x68
//   byte5: control = ...
//   byte6: address = ...
//   byte7: ctl.info= ...
//   byte8: data    = ...
//          ...     = ...
//   byteN-1: chksum  = ...
//   byteN: stop    = 0x16
//
//
//

typedef struct _mbus_frame {

    u_char start1;
    u_char length1;
    u_char length2;
    u_char start2;
    u_char control;
    u_char address;
    u_char control_information;
    // variable data field
    u_char checksum;
    u_char stop;
    
    u_char   data[252]; 
    size_t data_size;
    
    int type;
    
    //mbus_frame_data frame_data;

} mbus_frame;

typedef struct _mbus_slave_data {

    int state_fcb;
    int state_acd;
    
} mbus_slave_data;

#define NITEMS(x) (sizeof(x)/sizeof(x[0]))

//------------------------------------------------------------------------------
// MBUS FRAME DATA FORMATS
//

// DATA RECORDS
#define MBUS_DIB_DIF_EXTENSION_BIT  0x80
#define MBUS_DIB_VIF_EXTENSION_BIT  0x80

typedef struct _mbus_data_information_block {
        
        u_char dif;
        u_char dife[10];
        size_t  ndife;

} mbus_data_information_block;

typedef struct _mbus_value_information_block {
        
        u_char vif;
        u_char vife[10];
        size_t  nvife;

        u_char custom_vif[128];

} mbus_value_information_block;

typedef struct _mbus_data_record_header {
    
    mbus_data_information_block  dib;
    mbus_value_information_block vib;      

} mbus_data_record_header;

typedef struct _mbus_data_record {
        
    mbus_data_record_header drh;

    u_char data[234];
    size_t data_len;    

    void *next;

} mbus_data_record;

// 
// HEADER FOR VARIABLE LENGTH DATA FORMAT
// 
typedef struct _mbus_data_variable_header {
    
    //Ident.Nr. Manufr. Version Medium Access No. Status  Signature
    //4 Byte    2 Byte  1 Byte  1 Byte   1 Byte   1 Byte  2 Byte
    
    // ex
    // 88 63 80 09 82 4D 02 04 15 00 00 00
    
    u_char id_bcd[4];         // 88 63 80 09
    u_char manufacturer[2];   // 82 4D 
    u_char version;           // 02
    u_char medium;            // 04
    u_char access_no;         // 15
    u_char status;            // 00
    u_char signature[2];      // 00 00
    
} mbus_data_variable_header;

//
// VARIABLE LENGTH DATA FORMAT
//
typedef struct _mbus_data_variable {
  
    mbus_data_variable_header header;
    
    mbus_data_record *record; // XXX: this max num must be dynamic
    size_t nrecords;
    
    u_char *data;
    size_t  data_len;
    
    // are these needed/used?
    u_char  mdh;
    u_char *mfg_data;
    size_t  mfg_data_len;    
        
} mbus_data_variable;

//
// FIXED LENGTH DATA FORMAT
//
typedef struct _mbus_data_fixed {

    // ex
    // 35 01 00 00 counter 2 = 135 l (historic value)
    //
    // e.g.
    //
    // 78 56 34 12 identification number = 12345678
    // 0A          transmission counter = 0Ah = 10d
    // 00          status 00h: counters coded BCD, actual values, no errors
    // E9 7E       Type&Unit: medium water, unit1 = 1l, unit2 = 1l (same, but historic)
    // 01 00 00 00 counter 1 = 1l (actual value)
    // 35 01 00 00 counter 2 = 135 l (historic value)

    u_char id_bcd[4];
    u_char tx_cnt;
    u_char status;
    u_char cnt1_type;
    u_char cnt2_type;
    u_char cnt1_val[4];
    u_char cnt2_val[4];

} mbus_data_fixed;

//
// ABSTRACT DATA FORMAT (either fixed or variable length)
//
#define MBUS_DATA_TYPE_FIXED    1
#define MBUS_DATA_TYPE_VARIABLE 2
typedef struct _mbus_frame_data {

    mbus_data_variable data_var;
    mbus_data_fixed    data_fix;

    int type;

} mbus_frame_data;

// 
// HEADER FOR SECONDARY ADDRESSING
// 
typedef struct _mbus_data_secondary_address {
    
    //Ident.Nr. Manufr. Version Medium 
    //4 Byte    2 Byte  1 Byte  1 Byte 
    
    // ex
    // 14 49 10 01 10 57 01 06
    
    u_char id_bcd[4];         // 14 49 10 01
    u_char manufacturer[2];   // 10 57
    u_char version;           // 01
    u_char medium;            // 06
    
} mbus_data_secondary_address;


//
// for compatibility with non-gcc compilers:
//
//#ifndef __PRETTY_FUNCTION__
//#define __PRETTY_FUNCTION__ "libmbus"
//#endif

//------------------------------------------------------------------------------
// FRAME types
//
#define MBUS_FRAME_TYPE_ANY     0x00
#define MBUS_FRAME_TYPE_ACK     0x01
#define MBUS_FRAME_TYPE_SHORT   0x02
#define MBUS_FRAME_TYPE_CONTROL 0x03
#define MBUS_FRAME_TYPE_LONG    0x04

#define MBUS_FRAME_ACK_BASE_SIZE        1
#define MBUS_FRAME_SHORT_BASE_SIZE      5
#define MBUS_FRAME_CONTROL_BASE_SIZE    9
#define MBUS_FRAME_LONG_BASE_SIZE       9

#define MBUS_FRAME_BASE_SIZE_ACK       1
#define MBUS_FRAME_BASE_SIZE_SHORT     5
#define MBUS_FRAME_BASE_SIZE_CONTROL   9
#define MBUS_FRAME_BASE_SIZE_LONG      9

#define MBUS_FRAME_FIXED_SIZE_ACK      1
#define MBUS_FRAME_FIXED_SIZE_SHORT    5
#define MBUS_FRAME_FIXED_SIZE_CONTROL  6 
#define MBUS_FRAME_FIXED_SIZE_LONG     6

//
// Frame start/stop bits
//
#define MBUS_FRAME_ACK_START     0xE5
#define MBUS_FRAME_SHORT_START   0x10
#define MBUS_FRAME_CONTROL_START 0x68
#define MBUS_FRAME_LONG_START    0x68
#define MBUS_FRAME_STOP          0x16

//
//
//
#define MBUS_MAX_PRIMARY_SLAVES 256

//
// Control field
//
#define MBUS_CONTROL_FIELD_DIRECTION    0x07
#define MBUS_CONTROL_FIELD_FCB          0x06
#define MBUS_CONTROL_FIELD_ACD          0x06
#define MBUS_CONTROL_FIELD_FCV          0x05
#define MBUS_CONTROL_FIELD_DFC          0x05
#define MBUS_CONTROL_FIELD_F3           0x04
#define MBUS_CONTROL_FIELD_F2           0x03
#define MBUS_CONTROL_FIELD_F1           0x02
#define MBUS_CONTROL_FIELD_F0           0x01

#define MBUS_CONTROL_MASK_SND_NKE       0x40
#define MBUS_CONTROL_MASK_SND_UD        0x53
#define MBUS_CONTROL_MASK_REQ_UD2       0x5B
#define MBUS_CONTROL_MASK_REQ_UD1       0x5A
#define MBUS_CONTROL_MASK_RSP_UD        0x08

#define MBUS_CONTROL_MASK_FCB           0x20
#define MBUS_CONTROL_MASK_FCV           0x10

#define MBUS_CONTROL_MASK_ACD           0x20
#define MBUS_CONTROL_MASK_DFC           0x10

#define MBUS_CONTROL_MASK_DIR           0x40
#define MBUS_CONTROL_MASK_DIR_M2S       0x40
#define MBUS_CONTROL_MASK_DIR_S2M       0x00

//
// Address field
//
#define MBUS_ADDRESS_BROADCAST_REPLY    0xFE
#define MBUS_ADDRESS_BROADCAST_NOREPLY  0xFF
#define MBUS_ADDRESS_NETWORK_LAYER      0xFD

//
// Control Information field
//
//Mode 1 Mode 2                   Application                   Definition in
// 51h    55h                       data send                    EN1434-3
// 52h    56h                  selection of slaves           Usergroup July  ́93
// 50h                          application reset           Usergroup March  ́94
// 54h                          synronize action                 suggestion
// B8h                     set baudrate to 300 baud          Usergroup July  ́93
// B9h                     set baudrate to 600 baud          Usergroup July  ́93
// BAh                    set baudrate to 1200 baud          Usergroup July  ́93
// BBh                    set baudrate to 2400 baud          Usergroup July  ́93
// BCh                    set baudrate to 4800 baud          Usergroup July  ́93
// BDh                    set baudrate to 9600 baud          Usergroup July  ́93
// BEh                   set baudrate to 19200 baud              suggestion
// BFh                   set baudrate to 38400 baud              suggestion
// B1h           request readout of complete RAM content     Techem suggestion
// B2h          send user data (not standardized RAM write) Techem suggestion
// B3h                 initialize test calibration mode      Usergroup July  ́93
// B4h                           EEPROM read                 Techem suggestion
// B6h                         start software test           Techem suggestion
// 90h to 97h              codes used for hashing           longer recommended

#define MBUS_CONTROL_INFO_DATA_SEND          0x51
#define MBUS_CONTROL_INFO_DATA_SEND_MSB      0x55
#define MBUS_CONTROL_INFO_SELECT_SLAVE       0x52
#define MBUS_CONTROL_INFO_SELECT_SLAVE_MSB   0x56
#define MBUS_CONTROL_INFO_APPLICATION_RESET  0x50
#define MBUS_CONTROL_INFO_SYNC_ACTION        0x54
#define MBUS_CONTROL_INFO_SET_BAUDRATE_300   0xB8
#define MBUS_CONTROL_INFO_SET_BAUDRATE_600   0xB9
#define MBUS_CONTROL_INFO_SET_BAUDRATE_1200  0xBA
#define MBUS_CONTROL_INFO_SET_BAUDRATE_2400  0xBB
#define MBUS_CONTROL_INFO_SET_BAUDRATE_4800  0xBC
#define MBUS_CONTROL_INFO_SET_BAUDRATE_9600  0xBD
#define MBUS_CONTROL_INFO_SET_BAUDRATE_19200 0xBE
#define MBUS_CONTROL_INFO_SET_BAUDRATE_38400 0xBF
#define MBUS_CONTROL_INFO_REQUEST_RAM_READ   0xB1
#define MBUS_CONTROL_INFO_SEND_USER_DATA     0xB2
#define MBUS_CONTROL_INFO_INIT_TEST_CALIB    0xB3
#define MBUS_CONTROL_INFO_EEPROM_READ        0xB4
#define MBUS_CONTROL_INFO_SW_TEST_START      0xB6

#define MBUS_CONTROL_INFO_RESP_FIXED         0x73
#define MBUS_CONTROL_INFO_RESP_FIXED_MSB     0x77

#define MBUS_CONTROL_INFO_RESP_VARIABLE      0x72
#define MBUS_CONTROL_INFO_RESP_VARIABLE_MSB  0x76

//
// DATA BITS
//
#define MBUS_DATA_FIXED_STATUS_FORMAT_MASK  0x80
#define MBUS_DATA_FIXED_STATUS_FORMAT_BCD   0x00
#define MBUS_DATA_FIXED_STATUS_FORMAT_INT   0x80
#define MBUS_DATA_FIXED_STATUS_DATE_MASK    0x40
#define MBUS_DATA_FIXED_STATUS_DATE_STORED  0x40
#define MBUS_DATA_FIXED_STATUS_DATE_CURRENT 0x00


//
// data record fields
//
#define MBUS_DATA_RECORD_DIF_MASK_INST      0x00
#define MBUS_DATA_RECORD_DIF_MASK_MIN       0x10

#define MBUS_DATA_RECORD_DIF_MASK_TYPE_INT32     0x04
#define MBUS_DATA_RECORD_DIF_MASK_STORAGE_NO     0x40
#define MBUS_DATA_RECORD_DIF_MASK_EXTENTION      0x80


//
// FIXED DATA FLAGS
//

//
// VARIABLE DATA FLAGS
// 
#define MBUS_VARIABLE_DATA_MEDIUM_OTHER         0x00
#define MBUS_VARIABLE_DATA_MEDIUM_OIL           0x01
#define MBUS_VARIABLE_DATA_MEDIUM_ELECTRICITY   0x02
#define MBUS_VARIABLE_DATA_MEDIUM_GAS           0x03
#define MBUS_VARIABLE_DATA_MEDIUM_HEAT          0x04
#define MBUS_VARIABLE_DATA_MEDIUM_STEAM         0x05
#define MBUS_VARIABLE_DATA_MEDIUM_HOT_WATER     0x06
#define MBUS_VARIABLE_DATA_MEDIUM_WATER         0x07
#define MBUS_VARIABLE_DATA_MEDIUM_HEAT_COST     0x08
#define MBUS_VARIABLE_DATA_MEDIUM_COMPR_AIR     0x09
#define MBUS_VARIABLE_DATA_MEDIUM_COOL_OUT      0x0A
#define MBUS_VARIABLE_DATA_MEDIUM_COOL_IN       0x0B
#define MBUS_VARIABLE_DATA_MEDIUM_BUS           0x0E
#define MBUS_VARIABLE_DATA_MEDIUM_COLD_WATER    0x16
#define MBUS_VARIABLE_DATA_MEDIUM_DUAL_WATER    0x17
#define MBUS_VARIABLE_DATA_MEDIUM_PRESSURE      0x18
#define MBUS_VARIABLE_DATA_MEDIUM_ADC           0x19


//
// variable length records
//
mbus_data_record *mbus_data_record_new();
void              mbus_data_record_free(mbus_data_record *record);
void              mbus_data_record_append(mbus_data_variable *data, mbus_data_record *record);


// XXX: Add application reset subcodes

mbus_frame *mbus_frame_new(int frame_type);
int         mbus_frame_free(mbus_frame *frame);

mbus_frame_data *mbus_frame_data_new();
void             mbus_frame_data_free(mbus_frame_data *data);

//
//
//
int mbus_frame_calc_checksum(mbus_frame *frame);
int mbus_frame_calc_length  (mbus_frame *frame);

//
// Parse/Pack to bin
//
int mbus_parse(mbus_frame *frame, u_char *data, size_t data_size);

int mbus_data_fixed_parse   (mbus_frame *frame, mbus_data_fixed    *data);
int mbus_data_variable_parse(mbus_frame *frame, mbus_data_variable *data);

int mbus_frame_data_parse   (mbus_frame *frame, mbus_frame_data *data);
  
int mbus_frame_pack(mbus_frame *frame, u_char *data, size_t data_size);

int mbus_frame_verify(mbus_frame *frame);

int mbus_frame_internal_pack(mbus_frame *frame, mbus_frame_data *frame_data);

//
// data parsing
//
const char *mbus_data_record_function(mbus_data_record *record);
const char *mbus_data_fixed_function(int status);

//
// M-Bus frame data struct access/write functions
//
int mbus_frame_type(mbus_frame *frame);

//
// Slave status data register.
//
mbus_slave_data *mbus_slave_data_get(size_t i);

//
// XML generating functions
//
void  mbus_str_xml_encode(u_char *dst, const u_char *src, size_t max_len);
char *mbus_data_xml(mbus_frame_data *data);
char *mbus_data_variable_xml(mbus_data_variable *data);
char *mbus_data_fixed_xml(mbus_data_fixed *data);
char *mbus_frame_data_xml(mbus_frame_data *data);

char *mbus_data_variable_header_xml(mbus_data_variable_header *header);


//
// Debug/dump
//
int mbus_frame_print(mbus_frame *frame);
int mbus_frame_data_print(mbus_frame_data *data);
int mbus_data_fixed_print(mbus_data_fixed *data);
int mbus_data_variable_header_print(mbus_data_variable_header *header);
int mbus_data_variable_print(mbus_data_variable *data);

char *mbus_error_str();
void  mbus_error_str_set(char *message);
void  mbus_error_reset();

//
// data encode/decode functions
//
int mbus_data_manufacturer_encode(u_char *m_data, u_char *m_code);
const char *mbus_decode_manufacturer(u_char byte1, u_char byte2);
 
int mbus_data_bcd_encode(u_char *bcd_data, size_t bcd_data_size, int value);
int mbus_data_int_encode(u_char *int_data, size_t int_data_size, int value);

long mbus_data_bcd_decode(u_char *bcd_data, size_t bcd_data_size);
int  mbus_data_int_decode(u_char *int_data, size_t int_data_size);
long mbus_data_long_decode(u_char *int_data, size_t int_data_size);

void mbus_data_tm_decode(struct tm *t, u_char *t_data, size_t t_data_size);

void mbus_data_str_decode(u_char *dst, const u_char *src, size_t len);

const char *mbus_data_fixed_medium(mbus_data_fixed *data);
const char *mbus_data_fixed_unit(int medium_unit_byte);
const char *mbus_data_variable_medium_lookup(u_char medium);
const char *mbus_unit_prefix(int exp);


const char *mbus_vib_unit_lookup(mbus_value_information_block *vib);
const char *mbus_vif_unit_lookup(u_char vif);

u_char mbus_dif_datalength_lookup(u_char dif);

char *mbus_frame_get_secondary_address(mbus_frame *frame);
int   mbus_frame_select_secondary_pack(mbus_frame *frame, char *address);

#endif /* _MBUS_PROTOCOL_H_ */

