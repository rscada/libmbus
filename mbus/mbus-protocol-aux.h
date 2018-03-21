//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson and contributors, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
// Contributors:
// Large parts of this file was contributed by Tomas Menzl.
//
//------------------------------------------------------------------------------

/**
 * @file   mbus-protocol-aux.h
 *
 * @brief  Auxiliary functions to the Freescada libmbus library
 *
 * The idea is to simplify the basic task of querying MBus slaves and
 * the data processing.
 * Typical use might be (in oversimplified "pseudocode"):
 * \verbatim
 * mbus_handle = mbus_context_serial(device);
 *   or
 * mbus_handle = mbus_context_tcp(host, port);
 *
 * mbus_context_set_option(mbus_handle,option,value); // optional
 *
 * mbus_connect(mbus_handle);
 *
 *  ...
 *
 * mbus_read_slave(mbus_handle, addresses, &reply);
 * mbus_frame_data_parse(reply, &frameData);
 * // check the header / record type (fixed/variable)
 *
 * // for fixed use mbus_data_fixed_medium and 2x mbus_parse_fixed_record
 * mbus_data_fixed_medium
 * mbusRecord = mbus_parse_fixed_record() // first record
 *  // process mbusRecord
 * mbusRecord = mbus_parse_fixed_record() // second record
 *  // process mbusRecord
 *
 * // for variable use mbus_parse_variable_record
 * for each record
 *     mbusRecord = mbus_parse_variable_record(record)
 *     // process mbusRecord
 *
 *  ...
 *
 * mbus_disconnect(mbus_handle);
 * mbus_context_free(mbus_handle);
 * \endverbatim
 *
 * Note that the quantity values are partially "normalized". For example energy
 * is in Wh even when originally used "decimal" prefixes like MWh. This seems to
 * be sensible as it make easier any processing of a dataset with huge
 * fluctuation (no need to lookup/convert the prefixes). Also the potential
 * conversion to desired units is pretty easy.
 *
 * On the other hand we acknowledge that use of certain units are expected so we
 * do not convert all units to Si (i.e. no conversion from J to Wh or Bar to Pa.)
 */

#ifndef __MBUS_PROTOCOL_AUX_H__
#define __MBUS_PROTOCOL_AUX_H__

#include "mbus-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MBUS_PROBE_NOTHING   0
#define MBUS_PROBE_SINGLE    1
#define MBUS_PROBE_COLLISION 2
#define MBUS_PROBE_ERROR     -1

#define MBUS_FRAME_PURGE_S2M  2
#define MBUS_FRAME_PURGE_M2S  1
#define MBUS_FRAME_PURGE_NONE 0

/**
 * Unified MBus handle type encapsulating either Serial or TCP gateway.
 */
typedef struct _mbus_handle {
    int fd;
    int max_data_retry;
    int max_search_retry;
    char purge_first_frame;
    char is_serial; /**< _handle type (non zero for serial) */
    int (*open) (struct _mbus_handle *handle);
    int (*close) (struct _mbus_handle *handle);
    int (*send) (struct _mbus_handle *handle, mbus_frame *frame);
    int (*recv) (struct _mbus_handle *handle, mbus_frame *frame);
    void (*free_auxdata) (struct _mbus_handle *handle);
    void (*recv_event) (unsigned char src_type, const char *buff, size_t len);
    void (*send_event) (unsigned char src_type, const char *buff, size_t len);
    void (*scan_progress) (struct _mbus_handle *handle, const char *mask);
    void (*found_event) (struct _mbus_handle *handle, mbus_frame *frame);    
    void *auxdata;
} mbus_handle;

/**
 * MBus slave address type (primary/secodary address)
 */
typedef struct _mbus_address {
    char is_primary;             /**< Address type (non zero for primary) */
    union {
        int    primary;         /**< Primary address (for slaves shall be from 1 to 250) */
        char * secondary;       /**< Secondary address (shall be 16 digits) */
    };
} mbus_address;


/**
 * _string type
 *
 * In general support binary strings (octet string, non zero terminated)
 * but so far almost all strings are zero terminated ("texts").
 */
typedef struct _mbus_string {
    char * value;               /**< Buffer */
    int    size;                /**< _size */
} mbus_string;


/**
 * Quantity value type union
 */
typedef union _mbus_value {
    double      real_val;   /**< Numerical  */
    mbus_string str_val;    /**< Text/binary */
} mbus_value;


/**
 * Single measured quantity record type
 */
typedef struct _mbus_record {
    mbus_value value;          /**< Quantity value */
    char                is_numeric;      /**< Quantity value type (nonzero is numeric) */
    char               *unit;           /**< Quantity unit (e.g. Wh) */
    char               *function_medium; /**< Quantity medium or function (e.g. Electricity) */
    char               *quantity;       /**< Quantity type (e.g. Energy) */
    int                 device;         /**< Quantity device */
    long                tariff;         /**< Quantity tariff */
    long                storage_number; /**< Quantity storage number */
} mbus_record;

/**
 * MBus handle option enumeration
 */
typedef enum _mbus_context_option {
    MBUS_OPTION_MAX_DATA_RETRY,  /**< option defines the maximum attempts of data request retransmission */
    MBUS_OPTION_MAX_SEARCH_RETRY,  /**< option defines the maximum attempts of search request retransmission */
    MBUS_OPTION_PURGE_FIRST_FRAME  /**< option controls the echo cancelation for mbus_recv_frame */
} mbus_context_option;

/**
 * Event register functions
 */
//
// Event register functions
//
void mbus_register_recv_event(mbus_handle *handle, void (*event)(unsigned char src_type, const char *buff, size_t len));
void mbus_register_send_event(mbus_handle *handle, void (*event)(unsigned char src_type, const char *buff, size_t len));
void mbus_register_scan_progress(mbus_handle *handle, void (*event)(mbus_handle *handle, const char *mask));
void mbus_register_found_event(mbus_handle *handle, void (*event)(mbus_handle *handle, mbus_frame *frame));

/**
 * Allocate and initialize M-Bus serial context.
 *
 * @param device Serial device (like /dev/ttyUSB0 or /dev/ttyS0)
 *
 * @return Initialized "unified" handler when successful, NULL otherwise;
 */
mbus_handle * mbus_context_serial(const char *device);

/**
 * Allocate and initialize M-Bus TCP context.
 *
 * @param host Gateway host
 * @param port Gateway port
 *
 * @return Initialized "unified" handler when successful, NULL otherwise;
 */
mbus_handle * mbus_context_tcp(const char *host, uint16_t port);

/**
 * Deallocate memory used by M-Bus context.
 *
 * @param handle Initialized handle
 *
 */
void mbus_context_free(mbus_handle * handle);

/**
 * Connect to serial bus or TCP gateway depending on context.
 *
 * @param handle Initialized handle
 *
 * @return Zero when successful.
 */
int mbus_connect(mbus_handle * handle);

/**
 * Disconnects the "unified" handle.
 *
 * @param handle Initialized handle
 *
 * @return Zero when successful.
 */
int mbus_disconnect(mbus_handle * handle);

/**
 * Set option of a M-Bus context.
 *
 * @param handle Initialized handle
 * @param option option to set
 * @param value  value to set
 *
 * @return Zero when successful.
 */
int mbus_context_set_option(mbus_handle * handle, mbus_context_option option, long value);

/**
 * Receives a frame using "unified" handle
 *
 * @param handle Initialized handle
 * @param frame  Received frame
 *
 * @return Zero when successful.
 */
int mbus_recv_frame(mbus_handle * handle, mbus_frame *frame);

/**
 * Used for handling collisions. Blocks as long as receiving frames or corrupted data.
 *
 * @param handle Initialized handle
 *
 * @return Zero when nothing received, one otherwise.
 */
int mbus_purge_frames(mbus_handle * handle);

/**
 * Sends frame using "unified" handle
 *
 * @param handle Initialized handle
 * @param frame  Frame to send
 *
 * @return Zero when successful.
 */
int mbus_send_frame(mbus_handle * handle, mbus_frame *frame);

/**
 * Sends secondary address selection frame using "unified" handle
 *
 * @param handle             Initialized handle
 * @param secondary_addr_str Secondary address
 *
 * @return Zero when successful.
 */
int mbus_send_select_frame(mbus_handle * handle, const char *secondary_addr_str);

/**
 * Sends application reset to given slave using "unified" handle
 *
 * @param handle    Initialized handle
 * @param address   Address (0-255)
 * @param subcode   Subcode (0-255) or no subcode (-1)
 *
 * @return Zero when successful.
 */
int mbus_send_application_reset_frame(mbus_handle * handle, int address, int subcode);

/**
 * Sends switch baudrate frame using "unified" handle
 *
 * @param handle   Initialized handle
 * @param address  Address (0-255)
 * @param baudrate Baudrate (300,600,1200,2400,4800,9600,19200,38400)
 *
 * @return Zero when successful.
 */
int mbus_send_switch_baudrate_frame(mbus_handle * handle, int address, long baudrate);

/**
 * Sends request frame (REQ_UD2) to given slave using "unified" handle
 *
 * @param handle  Initialized handle
 * @param address Address (0-255)
 *
 * @return Zero when successful.
 */
int mbus_send_request_frame(mbus_handle * handle, int address);

/**
 * Sends user data frame (SND_UD) to given slave using "unified" handle
 *
 * @param handle    Initialized handle
 * @param address   Address (0-255)
 * @param data      User data
 * @param data_size Byte count of user data
 *
 * @return Zero when successful.
 */
int mbus_send_user_data_frame(mbus_handle * handle, int address, const unsigned char *data, size_t data_size);

/**
 * Sends frame to change primary address of given slave using "unified" handle
 *
 * @param handle      Initialized handle
 * @param old_address Old Address (0-255)
 * @param new_address New Address (0-250)
 *
 * @return Zero when successful.
 */
int mbus_set_primary_address(mbus_handle * handle, int old_address, int new_address);

/**
 * Sends a request and read replies until no more records available
 * or limit is reached.
 *
 * @param handle     Initialized handle
 * @param address    Address (0-255)
 * @param reply      pointer to an mbus frame for the reply
 * @param max_frames limit of frames to readout (0 = no limit)
 *
 * @return Zero when successful.
 */
int mbus_sendrecv_request(mbus_handle *handle, int address, mbus_frame *reply, int max_frames);

/**
 * Sends ping frame to given slave using "unified" handle
 *
 * @param handle         Initialized handle
 * @param address        Address (0-255)
 * @param purge_response Response flag (=0 don't receive response, >0 purge response)
 *
 * @return Zero when successful.
 */
int mbus_send_ping_frame(mbus_handle *handle, int address, char purge_response);

/**
 * Select slave by secondary address using "unified" handle
 *
 * @param handle        Initialized handle
 * @param mask          Address/mask to select
 *
 * @return See MBUS_PROBE_* constants
 */
int mbus_select_secondary_address(mbus_handle * handle, const char *mask);

/**
 * Probe/address slave by secondary address using "unified" handle
 *
 * @param handle        Initialized handle
 * @param mask          Address/mask to probe
 * @param matching_addr Matched address (the buffer has tobe at least 16 bytes)
 *
 * @return See MBUS_PROBE_* constants
 */
int mbus_probe_secondary_address(mbus_handle * handle, const char *mask, char *matching_addr);

/**
 * Read data from given slave using "unified" handle and address types
 *
 * @param handle  Initialized handle
 * @param address Address of the slave
 * @param reply   Reply from the slave
 *
 * @return Zero when successful.
 */
int mbus_read_slave(mbus_handle *handle, mbus_address *address, mbus_frame *reply);


/**
 * Allocate new data record. Use #mbus_record_free when finished.
 *
 * @return pointer to the new record, NULL when failed
 */
mbus_record * mbus_record_new();

/**
 * Destructor for mbus_record
 *
 * @param rec record to be freed
 */
void mbus_record_free(mbus_record *rec);


/**
 * Create/parse single counter from the fixed data structure
 *
 * @param statusByte     status byte
 * @param medium_unit_byte medium/unit byte
 * @param data           pointer to the data counter (4 bytes)
 *
 * @return Newly allocated record if succesful, NULL otherwise. Later on need to use #mbus_record_free
 */
mbus_record *mbus_parse_fixed_record(char statusByte, char medium_unit_byte, unsigned char *data);


/**
 * Create/parse single counter from the variable data structure record
 *
 * @param record record data to be parsed
 *
 * @return Newly allocated record if succesful, NULL otherwise. Later on need to use #mbus_record_free
 */
mbus_record * mbus_parse_variable_record(mbus_data_record *record);



/**
 * Get normalized counter value for a fixed counter
 *
 * Get "normalized" value and unit of the counter
 *
 * @param medium_unit_byte medium/unit byte of the fixed counter
 * @param medium_value    raw counter value
 * @param unit_out        units of the counter - use free when done
 * @param value_out       resulting counter value
 * @param quantity_out    parsed quantity, when done use "free"
 *
 * @return zero when OK
 */
int mbus_data_fixed_normalize(int medium_unit_byte, long medium_value, char **unit_out, double *value_out, char **quantity_out);



/**
 * Decode value of a variable data structure
 *
 * @param record          record to be decoded
 * @param value_out_real    numerical counter value output (when numerical)
 * @param value_out_str     string counter value output (when string, NULL otherwise), when finished use "free *value_out_str"
 * @param value_out_str_size string counter value size
 *
 * @return zero when OK
 */
int mbus_variable_value_decode(mbus_data_record *record, double *value_out_real, char **value_out_str, int *value_out_str_size);

/**
 * Decode units and normalize value using VIF/VIFE (used internally by mbus_vib_unit_normalize)
 *
 * @param vif         VIF (including standard extensions)
 * @param value       already parsed "raw" numerical value
 * @param unit_out     parsed unit, when done use "free"
 * @param value_out    normalized value
 * @param quantity_out parsed quantity, when done use "free"
 *
 * @return zero when OK
 */
int mbus_vif_unit_normalize(int vif, double value, char **unit_out, double *value_out, char **quantity_out);

/**
 * Decode units and normalize value from VIB
 *
 * @param vib         mbus value information block of the variable record
 * @param value       already parsed "raw" numerical value
 * @param unit_out     parsed unit, when done use "free"
 * @param value_out    normalized value
 * @param quantity_out parsed quantity, when done use "free"
 *
 * @return zero when OK
 */
int mbus_vib_unit_normalize(mbus_value_information_block *vib, double value, char **unit_out, double *value_out, char ** quantity_out);

/**
 * Generate XML for normalized variable-length data
 *
 * @param data    variable-length data
 *
 * @return string with XML
 */
char * mbus_data_variable_xml_normalized(mbus_data_variable *data);

/**
 * Return a string containing an XML representation of the normalized M-BUS frame data.
 *
 * @param data    M-Bus frame data
 *
 * @return string with XML
 */
char * mbus_frame_data_xml_normalized(mbus_frame_data *data);

/**
 * Iterate over secondary addresses, send a probe package to all addresses matching
 * the given addresses mask.
 *
 * @param handle      Initialized handle
 * @param pos         current address
 * @param addr_mask   address mask to
 *
 * @return zero when OK
 */
int mbus_scan_2nd_address_range(mbus_handle * handle, int pos, char *addr_mask);

/**
 * Convert a buffer with hex values into a buffer with binary values.
 *
 * @param dst      destination buffer with binary values
 * @param dst_len  byte count of destination buffer
 * @param src      source buffer with hex values
 * @param src_len  byte count of source buffer
 *
 * @return byte count of successful converted values
 */
size_t mbus_hex2bin(unsigned char * dst, size_t dst_len, const unsigned char * src, size_t src_len);

#ifdef __cplusplus
}
#endif

#endif // __MBUS_PROTOCOL_AUX_H__
