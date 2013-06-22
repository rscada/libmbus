//------------------------------------------------------------------------------
// Copyright (C) 2010-2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
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
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

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

    unsigned char start1;
    unsigned char length1;
    unsigned char length2;
    unsigned char start2;
    unsigned char control;
    unsigned char address;
    unsigned char control_information;
    // variable data field
    unsigned char checksum;
    unsigned char stop;
    
    unsigned char   data[252]; 
    size_t data_size;
    
    int type;
    time_t timestamp;
    
    //mbus_frame_data frame_data;

    void *next; // pointer to next mbus_frame for multi-telegram replies

} mbus_frame;

typedef struct _mbus_slave_data {

    int state_fcb;
    int state_acd;
    
} mbus_slave_data;

#define NITEMS(x) (sizeof(x)/sizeof(x[0]))

//
// Supported handle types
//
#define MBUS_HANDLE_TYPE_TCP    0
#define MBUS_HANDLE_TYPE_SERIAL 1

//
// Resultcodes for mbus_recv_frame
//
#define MBUS_RECV_RESULT_OK        0
#define MBUS_RECV_RESULT_ERROR     -1
#define MBUS_RECV_RESULT_INVALID   -2
#define MBUS_RECV_RESULT_TIMEOUT   -3
#define MBUS_RECV_RESULT_RESET     -4

//------------------------------------------------------------------------------
// MBUS FRAME DATA FORMATS
//

// DATA RECORDS
#define MBUS_DIB_DIF_WITHOUT_EXTENSION     0x7F
#define MBUS_DIB_DIF_EXTENSION_BIT         0x80
#define MBUS_DIB_VIF_WITHOUT_EXTENSION     0x7F
#define MBUS_DIB_VIF_EXTENSION_BIT         0x80
#define MBUS_DIB_DIF_MANUFACTURER_SPECIFIC 0x0F
#define MBUS_DIB_DIF_MORE_RECORDS_FOLLOW   0x1F
#define MBUS_DIB_DIF_IDLE_FILLER           0x2F


typedef struct _mbus_data_information_block {
        
        unsigned char dif;
        unsigned char dife[10];
        size_t  ndife;

} mbus_data_information_block;

typedef struct _mbus_value_information_block {
        
        unsigned char vif;
        unsigned char vife[10];
        size_t  nvife;

        unsigned char custom_vif[128];

} mbus_value_information_block;

typedef struct _mbus_data_record_header {
    
    mbus_data_information_block  dib;
    mbus_value_information_block vib;      

} mbus_data_record_header;

typedef struct _mbus_data_record {
        
    mbus_data_record_header drh;

    unsigned char data[234];
    size_t data_len;    

    time_t timestamp;

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
    
    unsigned char id_bcd[4];         // 88 63 80 09
    unsigned char manufacturer[2];   // 82 4D 
    unsigned char version;           // 02
    unsigned char medium;            // 04
    unsigned char access_no;         // 15
    unsigned char status;            // 00
    unsigned char signature[2];      // 00 00
    
} mbus_data_variable_header;

#define MBUS_DATA_VARIABLE_HEADER_LENGTH 12

//
// VARIABLE LENGTH DATA FORMAT
//
typedef struct _mbus_data_variable {
  
    mbus_data_variable_header header;
    
    mbus_data_record *record; 
    size_t nrecords;
    
    unsigned char *data;
    size_t  data_len;
    
    unsigned char more_records_follow;
    
    // are these needed/used?
    unsigned char  mdh;
    unsigned char *mfg_data;
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

    unsigned char id_bcd[4];
    unsigned char tx_cnt;
    unsigned char status;
    unsigned char cnt1_type;
    unsigned char cnt2_type;
    unsigned char cnt1_val[4];
    unsigned char cnt2_val[4];

} mbus_data_fixed;

#define MBUS_DATA_FIXED_LENGTH 16

//
// ABSTRACT DATA FORMAT (error, fixed or variable length)
//
#define MBUS_DATA_TYPE_FIXED    1
#define MBUS_DATA_TYPE_VARIABLE 2
#define MBUS_DATA_TYPE_ERROR    3

typedef struct _mbus_frame_data {

    mbus_data_variable data_var;
    mbus_data_fixed    data_fix;

    int type;
    int error;

} mbus_frame_data;

// 
// HEADER FOR SECONDARY ADDRESSING
// 
typedef struct _mbus_data_secondary_address {
    
    //Ident.Nr. Manufr. Version Medium 
    //4 Byte    2 Byte  1 Byte  1 Byte 
    
    // ex
    // 14 49 10 01 10 57 01 06
    
    unsigned char id_bcd[4];         // 14 49 10 01
    unsigned char manufacturer[2];   // 10 57
    unsigned char version;           // 01
    unsigned char medium;            // 06
    
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
#define MBUS_MAX_PRIMARY_SLAVES            250

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

//Mode 1 Mode 2                   Application                   Definition in
// 70h             report of general application errors     Usergroup March 94
// 71h                      report of alarm status          Usergroup March 94
// 72h   76h                variable data respond                EN1434-3
// 73h   77h                 fixed data respond                  EN1434-3
#define MBUS_CONTROL_INFO_ERROR_GENERAL      0x70
#define MBUS_CONTROL_INFO_STATUS_ALARM       0x71

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
#define MBUS_DATA_RECORD_DIF_MASK_DATA           0x0F
#define MBUS_DATA_RECORD_DIF_MASK_FUNCTION       0x30
#define MBUS_DATA_RECORD_DIF_MASK_STORAGE_NO     0x40
#define MBUS_DATA_RECORD_DIF_MASK_EXTENTION      0x80
#define MBUS_DATA_RECORD_DIF_MASK_NON_DATA       0xF0

//
// GENERAL APPLICATION ERRORS
//
#define MBUS_ERROR_DATA_UNSPECIFIED        0x00
#define MBUS_ERROR_DATA_UNIMPLEMENTED_CI   0x01
#define MBUS_ERROR_DATA_BUFFER_TOO_LONG    0x02
#define MBUS_ERROR_DATA_TOO_MANY_RECORDS   0x03
#define MBUS_ERROR_DATA_PREMATURE_END      0x04
#define MBUS_ERROR_DATA_TOO_MANY_DIFES     0x05
#define MBUS_ERROR_DATA_TOO_MANY_VIFES     0x06
#define MBUS_ERROR_DATA_RESERVED           0x07
#define MBUS_ERROR_DATA_APPLICATION_BUSY   0x08
#define MBUS_ERROR_DATA_TOO_MANY_READOUTS  0x09

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
#define MBUS_VARIABLE_DATA_MEDIUM_HEAT_OUT      0x04
#define MBUS_VARIABLE_DATA_MEDIUM_STEAM         0x05
#define MBUS_VARIABLE_DATA_MEDIUM_HOT_WATER     0x06
#define MBUS_VARIABLE_DATA_MEDIUM_WATER         0x07
#define MBUS_VARIABLE_DATA_MEDIUM_HEAT_COST     0x08
#define MBUS_VARIABLE_DATA_MEDIUM_COMPR_AIR     0x09
#define MBUS_VARIABLE_DATA_MEDIUM_COOL_OUT      0x0A
#define MBUS_VARIABLE_DATA_MEDIUM_COOL_IN       0x0B
#define MBUS_VARIABLE_DATA_MEDIUM_HEAT_IN       0x0C
#define MBUS_VARIABLE_DATA_MEDIUM_HEAT_COOL     0x0D
#define MBUS_VARIABLE_DATA_MEDIUM_BUS           0x0E
#define MBUS_VARIABLE_DATA_MEDIUM_UNKNOWN       0x0F
#define MBUS_VARIABLE_DATA_MEDIUM_COLD_WATER    0x16
#define MBUS_VARIABLE_DATA_MEDIUM_DUAL_WATER    0x17
#define MBUS_VARIABLE_DATA_MEDIUM_PRESSURE      0x18
#define MBUS_VARIABLE_DATA_MEDIUM_ADC           0x19

#define MBUS_VARIABLE_DATA_MAN_ACW              0x0477
#define MBUS_VARIABLE_DATA_MAN_ABB              0x0442
#define MBUS_VARIABLE_DATA_MAN_AMT              0x05B4
#define MBUS_VARIABLE_DATA_MAN_EFE              0x14C5
#define MBUS_VARIABLE_DATA_MAN_ELS              0x1593
#define MBUS_VARIABLE_DATA_MAN_ELV              0x1596
#define MBUS_VARIABLE_DATA_MAN_EMH              0x15A8
#define MBUS_VARIABLE_DATA_MAN_HYD              0x2324
#define MBUS_VARIABLE_DATA_MAN_KAM              0x2C2D
#define MBUS_VARIABLE_DATA_MAN_LSE              0x3265
#define MBUS_VARIABLE_DATA_MAN_LUG              0x32A7
#define MBUS_VARIABLE_DATA_MAN_NZR              0x3B52
#define MBUS_VARIABLE_DATA_MAN_PAD              0x4024
#define MBUS_VARIABLE_DATA_MAN_QDS              0x4493
#define MBUS_VARIABLE_DATA_MAN_RKE              0x4965
#define MBUS_VARIABLE_DATA_MAN_SEN              0x4CAE
#define MBUS_VARIABLE_DATA_MAN_SLB              0x4D82
#define MBUS_VARIABLE_DATA_MAN_SON              0x4DEE
#define MBUS_VARIABLE_DATA_MAN_SPX              0x4E18
#define MBUS_VARIABLE_DATA_MAN_SVM              0x4ECD
#define MBUS_VARIABLE_DATA_MAN_TCH              0x5068
#define MBUS_VARIABLE_DATA_MAN_ZRM              0x6A4D

//
// Event callback functions
//
extern void (*_mbus_recv_event)(unsigned char src_type, const char *buff, size_t len);
extern void (*_mbus_send_event)(unsigned char src_type, const char *buff, size_t len);

void mbus_dump_recv_event(unsigned char src_type, const char *buff, size_t len);
void mbus_dump_send_event(unsigned char src_type, const char *buff, size_t len);

//
// Event register functions
//
void mbus_register_recv_event(void (*event)(unsigned char src_type, const char *buff, size_t len));
void mbus_register_send_event(void (*event)(unsigned char src_type, const char *buff, size_t len));

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
int mbus_parse(mbus_frame *frame, unsigned char *data, size_t data_size);

int mbus_data_fixed_parse   (mbus_frame *frame, mbus_data_fixed    *data);
int mbus_data_variable_parse(mbus_frame *frame, mbus_data_variable *data);

int mbus_frame_data_parse   (mbus_frame *frame, mbus_frame_data *data);
  
int mbus_frame_pack(mbus_frame *frame, unsigned char *data, size_t data_size);

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
int mbus_frame_direction(mbus_frame *frame);

//
// Slave status data register.
//
mbus_slave_data *mbus_slave_data_get(size_t i);

//
// XML generating functions
//
void  mbus_str_xml_encode(unsigned char *dst, const unsigned char *src, size_t max_len);
char *mbus_data_xml(mbus_frame_data *data);
char *mbus_data_variable_xml(mbus_data_variable *data);
char *mbus_data_fixed_xml(mbus_data_fixed *data);
char *mbus_data_error_xml(int error);
char *mbus_frame_data_xml(mbus_frame_data *data);

char *mbus_data_variable_header_xml(mbus_data_variable_header *header);

char *mbus_frame_xml(mbus_frame *frame);

//
// Debug/dump
//
int mbus_frame_print(mbus_frame *frame);
int mbus_frame_data_print(mbus_frame_data *data);
int mbus_data_fixed_print(mbus_data_fixed *data);
int mbus_data_error_print(int error);
int mbus_data_variable_header_print(mbus_data_variable_header *header);
int mbus_data_variable_print(mbus_data_variable *data);

char *mbus_error_str();
void  mbus_error_str_set(char *message);
void  mbus_error_reset();

void  mbus_parse_set_debug(int debug);
void  mbus_hex_dump(const char *label, const char *buff, size_t len);

//
// data encode/decode functions
//
int mbus_data_manufacturer_encode(unsigned char *m_data, unsigned char *m_code);
const char *mbus_decode_manufacturer(unsigned char byte1, unsigned char byte2);
const char *mbus_data_product_name(mbus_data_variable_header *header);
 
int mbus_data_bcd_encode(unsigned char *bcd_data, size_t bcd_data_size, int value);
int mbus_data_int_encode(unsigned char *int_data, size_t int_data_size, int value);

long long mbus_data_bcd_decode(unsigned char *bcd_data, size_t bcd_data_size);
int  mbus_data_int_decode(unsigned char *int_data, size_t int_data_size);
long mbus_data_long_decode(unsigned char *int_data, size_t int_data_size);
long long mbus_data_long_long_decode(unsigned char *int_data, size_t int_data_size);

float mbus_data_float_decode(unsigned char *float_data);

void mbus_data_tm_decode(struct tm *t, unsigned char *t_data, size_t t_data_size);

void mbus_data_str_decode(unsigned char *dst, const unsigned char *src, size_t len);

void mbus_data_bin_decode(unsigned char *dst, const unsigned char *src, size_t len, size_t max_len);

const char *mbus_data_fixed_medium(mbus_data_fixed *data);
const char *mbus_data_fixed_unit(int medium_unit_byte);
const char *mbus_data_variable_medium_lookup(unsigned char medium);
const char *mbus_unit_prefix(int exp);

const char *mbus_data_error_lookup(int error);

const char *mbus_vib_unit_lookup(mbus_value_information_block *vib);
const char *mbus_vif_unit_lookup(unsigned char vif);

unsigned char mbus_dif_datalength_lookup(unsigned char dif);

char *mbus_frame_get_secondary_address(mbus_frame *frame);
int   mbus_frame_select_secondary_pack(mbus_frame *frame, char *address);

int mbus_is_primary_address(int value);
int mbus_is_secondary_address(const char * value);

#ifdef __cplusplus
}
#endif

#endif /* _MBUS_PROTOCOL_H_ */

