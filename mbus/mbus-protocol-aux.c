//------------------------------------------------------------------------------
// Copyright (C) 2010-2011, Robert Johansson and contributors, Raditex AB
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

#include "mbus-protocol-aux.h"
#include "mbus-serial.h"
#include "mbus-tcp.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/*@ignore@*/
#define MBUS_ERROR(...) fprintf (stderr, __VA_ARGS__)

#ifdef _DEBUG_
#define MBUS_DEBUG(...) fprintf (stderr, __VA_ARGS__)
#else
#define MBUS_DEBUG(...)
#endif
/*@end@*/

static int debug = 0;

typedef struct _mbus_variable_vif {
    unsigned     vif;
    double       exponent;
    const char * unit;
    const char * quantity;
} mbus_variable_vif;

mbus_variable_vif vif_table[] = {
/*  Primary VIFs (main table), range 0x00 - 0xFF */

    /*  E000 0nnn    Energy Wh (0.001Wh to 10000Wh) */
    { 0x00, 1.0e-3, "Wh", "Energy" },
    { 0x01, 1.0e-2, "Wh", "Energy" },
    { 0x02, 1.0e-1, "Wh", "Energy" },
    { 0x03, 1.0e0,  "Wh", "Energy" },
    { 0x04, 1.0e1,  "Wh", "Energy" },
    { 0x05, 1.0e2,  "Wh", "Energy" },
    { 0x06, 1.0e3,  "Wh", "Energy" },
    { 0x07, 1.0e4,  "Wh", "Energy" },

    /* E000 1nnn    Energy  J (0.001kJ to 10000kJ) */
    { 0x08, 1.0e0, "J", "Energy" },
    { 0x09, 1.0e1, "J", "Energy" },
    { 0x0A, 1.0e2, "J", "Energy" },
    { 0x0B, 1.0e3, "J", "Energy" },
    { 0x0C, 1.0e4, "J", "Energy" },
    { 0x0D, 1.0e5, "J", "Energy" },
    { 0x0E, 1.0e6, "J", "Energy" },
    { 0x0F, 1.0e7, "J", "Energy" },

    /* E001 0nnn    Volume m^3 (0.001l to 10000l) */
    { 0x10, 1.0e-6, "m^3", "Volume" },
    { 0x11, 1.0e-5, "m^3", "Volume" },
    { 0x12, 1.0e-4, "m^3", "Volume" },
    { 0x13, 1.0e-3, "m^3", "Volume" },
    { 0x14, 1.0e-2, "m^3", "Volume" },
    { 0x15, 1.0e-1, "m^3", "Volume" },
    { 0x16, 1.0e0,  "m^3", "Volume" },
    { 0x17, 1.0e1,  "m^3", "Volume" },

    /* E001 1nnn    Mass kg (0.001kg to 10000kg) */
    { 0x18, 1.0e-3, "kg", "Mass" },
    { 0x19, 1.0e-2, "kg", "Mass" },
    { 0x1A, 1.0e-1, "kg", "Mass" },
    { 0x1B, 1.0e0,  "kg", "Mass" },
    { 0x1C, 1.0e1,  "kg", "Mass" },
    { 0x1D, 1.0e2,  "kg", "Mass" },
    { 0x1E, 1.0e3,  "kg", "Mass" },
    { 0x1F, 1.0e4,  "kg", "Mass" },

    /* E010 00nn    On Time s */
    { 0x20,     1.0, "s", "On time" },  /* seconds */
    { 0x21,    60.0, "s", "On time" },  /* minutes */
    { 0x22,  3600.0, "s", "On time" },  /* hours   */
    { 0x23, 86400.0, "s", "On time" },  /* days    */

    /* E010 01nn    Operating Time s */
    { 0x24,     1.0, "s", "Operating time" },  /* seconds */
    { 0x25,    60.0, "s", "Operating time" },  /* minutes */
    { 0x26,  3600.0, "s", "Operating time" },  /* hours   */
    { 0x27, 86400.0, "s", "Operating time" },  /* days    */

    /* E010 1nnn    Power W (0.001W to 10000W) */
    { 0x28, 1.0e-3, "W", "Power" },
    { 0x29, 1.0e-2, "W", "Power" },
    { 0x2A, 1.0e-1, "W", "Power" },
    { 0x2B, 1.0e0,  "W", "Power" },
    { 0x2C, 1.0e1,  "W", "Power" },
    { 0x2D, 1.0e2,  "W", "Power" },
    { 0x2E, 1.0e3,  "W", "Power" },
    { 0x2F, 1.0e4,  "W", "Power" },

    /* E011 0nnn    Power J/h (0.001kJ/h to 10000kJ/h) */
    { 0x30, 1.0e0, "J/h", "Power" },
    { 0x31, 1.0e1, "J/h", "Power" },
    { 0x32, 1.0e2, "J/h", "Power" },
    { 0x33, 1.0e3, "J/h", "Power" },
    { 0x34, 1.0e4, "J/h", "Power" },
    { 0x35, 1.0e5, "J/h", "Power" },
    { 0x36, 1.0e6, "J/h", "Power" },
    { 0x37, 1.0e7, "J/h", "Power" },

    /* E011 1nnn    Volume Flow m3/h (0.001l/h to 10000l/h) */
    { 0x38, 1.0e-6, "m^3/h", "Volume flow" },
    { 0x39, 1.0e-5, "m^3/h", "Volume flow" },
    { 0x3A, 1.0e-4, "m^3/h", "Volume flow" },
    { 0x3B, 1.0e-3, "m^3/h", "Volume flow" },
    { 0x3C, 1.0e-2, "m^3/h", "Volume flow" },
    { 0x3D, 1.0e-1, "m^3/h", "Volume flow" },
    { 0x3E, 1.0e0,  "m^3/h", "Volume flow" },
    { 0x3F, 1.0e1,  "m^3/h", "Volume flow" },

    /* E100 0nnn     Volume Flow ext.  m^3/min (0.0001l/min to 1000l/min) */
    { 0x40, 1.0e-7, "m^3/min", "Volume flow" },
    { 0x41, 1.0e-6, "m^3/min", "Volume flow" },
    { 0x42, 1.0e-5, "m^3/min", "Volume flow" },
    { 0x43, 1.0e-4, "m^3/min", "Volume flow" },
    { 0x44, 1.0e-3, "m^3/min", "Volume flow" },
    { 0x45, 1.0e-2, "m^3/min", "Volume flow" },
    { 0x46, 1.0e-1, "m^3/min", "Volume flow" },
    { 0x47, 1.0e0,  "m^3/min", "Volume flow" },

    /* E100 1nnn     Volume Flow ext.  m^3/s (0.001ml/s to 10000ml/s) */
    { 0x48, 1.0e-9, "m^3/s", "Volume flow" },
    { 0x49, 1.0e-8, "m^3/s", "Volume flow" },
    { 0x4A, 1.0e-7, "m^3/s", "Volume flow" },
    { 0x4B, 1.0e-6, "m^3/s", "Volume flow" },
    { 0x4C, 1.0e-5, "m^3/s", "Volume flow" },
    { 0x4D, 1.0e-4, "m^3/s", "Volume flow" },
    { 0x4E, 1.0e-3, "m^3/s", "Volume flow" },
    { 0x4F, 1.0e-2, "m^3/s", "Volume flow" },

    /* E101 0nnn     Mass flow kg/h (0.001kg/h to 10000kg/h) */
    { 0x50, 1.0e-3, "kg/h", "Mass flow" },
    { 0x51, 1.0e-2, "kg/h", "Mass flow" },
    { 0x52, 1.0e-1, "kg/h", "Mass flow" },
    { 0x53, 1.0e0,  "kg/h", "Mass flow" },
    { 0x54, 1.0e1,  "kg/h", "Mass flow" },
    { 0x55, 1.0e2,  "kg/h", "Mass flow" },
    { 0x56, 1.0e3,  "kg/h", "Mass flow" },
    { 0x57, 1.0e4,  "kg/h", "Mass flow" },

    /* E101 10nn     Flow Temperature °C (0.001°C to 1°C) */
    { 0x58, 1.0e-3, "°C", "Flow temperature" },
    { 0x59, 1.0e-2, "°C", "Flow temperature" },
    { 0x5A, 1.0e-1, "°C", "Flow temperature" },
    { 0x5B, 1.0e0,  "°C", "Flow temperature" },

    /* E101 11nn Return Temperature °C (0.001°C to 1°C) */
    { 0x5C, 1.0e-3, "°C", "Return temperature" },
    { 0x5D, 1.0e-2, "°C", "Return temperature" },
    { 0x5E, 1.0e-1, "°C", "Return temperature" },
    { 0x5F, 1.0e0,  "°C", "Return temperature" },

    /* E110 00nn    Temperature Difference  K   (mK to  K) */
    { 0x60, 1.0e-3, "K", "Temperature difference" },
    { 0x61, 1.0e-2, "K", "Temperature difference" },
    { 0x62, 1.0e-1, "K", "Temperature difference" },
    { 0x63, 1.0e0,  "K", "Temperature difference" },

    /* E110 01nn     External Temperature °C (0.001°C to 1°C) */
    { 0x64, 1.0e-3, "°C", "External temperature" },
    { 0x65, 1.0e-2, "°C", "External temperature" },
    { 0x66, 1.0e-1, "°C", "External temperature" },
    { 0x67, 1.0e0,  "°C", "External temperature" },

    /* E110 10nn     Pressure bar (1mbar to 1000mbar) */
    { 0x68, 1.0e-3, "bar", "Pressure" },
    { 0x69, 1.0e-2, "bar", "Pressure" },
    { 0x6A, 1.0e-1, "bar", "Pressure" },
    { 0x6B, 1.0e0,  "bar", "Pressure" },

    /* E110 110n     Time Point */
    { 0x6C, 1.0e0, "-", "Time point (date)" },            /* n = 0        date, data type G */
    { 0x6D, 1.0e0, "-", "Time point (date & time)" },     /* n = 1 time & date, data type F */

    /* E110 1110     Units for H.C.A. dimensionless */
    { 0x6E, 1.0e0,  "Units for H.C.A.", "H.C.A." },

    /* E110 1111     Reserved */
    { 0x6F, 0.0,  "Reserved", "Reserved" },

    /* E111 00nn     Averaging Duration s */
    { 0x70,     1.0, "s", "Averaging Duration" },  /* seconds */
    { 0x71,    60.0, "s", "Averaging Duration" },  /* minutes */
    { 0x72,  3600.0, "s", "Averaging Duration" },  /* hours   */
    { 0x73, 86400.0, "s", "Averaging Duration" },  /* days    */

    /* E111 01nn     Actuality Duration s */
    { 0x74,     1.0, "s", "Actuality Duration" },  /* seconds */
    { 0x75,    60.0, "s", "Actuality Duration" },  /* minutes */
    { 0x76,  3600.0, "s", "Actuality Duration" },  /* hours   */
    { 0x77, 86400.0, "s", "Actuality Duration" },  /* days    */

    /* Fabrication No */
    { 0x78, 1.0, "", "Fabrication No" },

    /* E111 1001 (Enhanced) Identification */
    { 0x79, 1.0, "", "(Enhanced) Identification" },

    /* E111 1010 Bus Address */
    { 0x7A, 1.0, "", "Bus Address" },

    /* Any VIF: 7Eh */
    { 0x7E, 1.0, "", "Any VIF" },

    /* Manufacturer specific: 7Fh */
    { 0x7F, 1.0, "", "Manufacturer specific" },

    /* Any VIF: 7Eh */
    { 0xFE, 1.0, "", "Any VIF" },

    /* Manufacturer specific: FFh */
    { 0xFF, 1.0, "", "Manufacturer specific" },


/* Main VIFE-Code Extension table (following VIF=FDh for primary VIF)
   See 8.4.4 a, only some of them are here. Using range 0x100 - 0x1FF */

    /* E000 00nn   Credit of 10nn-3 of the nominal local legal currency units */
    { 0x100, 1.0e-3, "Currency units", "Credit" },
    { 0x101, 1.0e-2, "Currency units", "Credit" },
    { 0x102, 1.0e-1, "Currency units", "Credit" },
    { 0x103, 1.0e0,  "Currency units", "Credit" },

    /* E000 01nn   Debit of 10nn-3 of the nominal local legal currency units */
    { 0x104, 1.0e-3, "Currency units", "Debit" },
    { 0x105, 1.0e-2, "Currency units", "Debit" },
    { 0x106, 1.0e-1, "Currency units", "Debit" },
    { 0x107, 1.0e0,  "Currency units", "Debit" },

    /* E000 1000 Access Number (transmission count) */
    { 0x108, 1.0e0,  "", "Access Number (transmission count)" },

    /* E000 1001 Medium (as in fixed header) */
    { 0x109, 1.0e0,  "", "Medium" },

    /* E000 1010 Manufacturer (as in fixed header) */
    { 0x10A, 1.0e0,  "", "Manufacturer" },

    /* E000 1011 Parameter set identification */
    { 0x10B, 1.0e0,  "", "Parameter set identification" },

    /* E000 1100 Model / Version */
    { 0x10C, 1.0e0,  "", "Model / Version" },

    /* E000 1101 Hardware version # */
    { 0x10D, 1.0e0,  "", "Hardware version" },

    /* E000 1110 Firmware version # */
    { 0x10E, 1.0e0,  "", "Firmware version" },

    /* E000 1111 Software version # */
    { 0x10F, 1.0e0,  "", "Software version" },


    /* E001 0000 Customer location */
    { 0x110, 1.0e0,  "", "Customer location" },

    /* E001 0001 Customer */
    { 0x111, 1.0e0,  "", "Customer" },

    /* E001 0010 Access Code User */
    { 0x112, 1.0e0,  "", "Access Code User" },

    /* E001 0011 Access Code Operator */
    { 0x113, 1.0e0,  "", "Access Code Operator" },

    /* E001 0100 Access Code System Operator */
    { 0x114, 1.0e0,  "", "Access Code System Operator" },

    /* E001 0101 Access Code Developer */
    { 0x115, 1.0e0,  "", "Access Code Developer" },

    /* E001 0110 Password */
    { 0x116, 1.0e0,  "", "Password" },

    /* E001 0111 Error flags (binary) */
    { 0x117, 1.0e0,  "", "Error flags" },

    /* E001 1000 Error mask */
    { 0x118, 1.0e0,  "", "Error mask" },

    /* E001 1001 Reserved */
    { 0x119, 1.0e0,  "Reserved", "Reserved" },


    /* E001 1010 Digital Output (binary) */
    { 0x11A, 1.0e0,  "", "Digital Output" },

    /* E001 1011 Digital Input (binary) */
    { 0x11B, 1.0e0,  "", "Digital Input" },

    /* E001 1100 Baudrate [Baud] */
    { 0x11C, 1.0e0,  "Baud", "Baudrate" },

    /* E001 1101 Response delay time [bittimes] */
    { 0x11D, 1.0e0,  "Bittimes", "Response delay time" },

    /* E001 1110 Retry */
    { 0x11E, 1.0e0,  "", "Retry" },

    /* E001 1111 Reserved */
    { 0x11F, 1.0e0,  "Reserved", "Reserved" },


    /* E010 0000 First storage # for cyclic storage */
    { 0x120, 1.0e0,  "", "First storage # for cyclic storage" },

    /* E010 0001 Last storage # for cyclic storage */
    { 0x121, 1.0e0,  "", "Last storage # for cyclic storage" },

    /* E010 0010 Size of storage block */
    { 0x122, 1.0e0,  "", "Size of storage block" },

    /* E010 0011 Reserved */
    { 0x123, 1.0e0,  "Reserved", "Reserved" },

    /* E010 01nn Storage interval [sec(s)..day(s)] */
    { 0x124,        1.0,  "s", "Storage interval" },   /* second(s) */
    { 0x125,       60.0,  "s", "Storage interval" },   /* minute(s) */
    { 0x126,     3600.0,  "s", "Storage interval" },   /* hour(s)   */
    { 0x127,    86400.0,  "s", "Storage interval" },   /* day(s)    */
    { 0x128,  2629743.83, "s", "Storage interval" },   /* month(s)  */
    { 0x129, 31556926.0,  "s", "Storage interval" },   /* year(s)   */

    /* E010 1010 Reserved */
    { 0x12A, 1.0e0,  "Reserved", "Reserved" },

    /* E010 1011 Reserved */
    { 0x12B, 1.0e0,  "Reserved", "Reserved" },

    /* E010 11nn Duration since last readout [sec(s)..day(s)] */
    { 0x12C,     1.0, "s", "Duration since last readout" },  /* seconds */
    { 0x12D,    60.0, "s", "Duration since last readout" },  /* minutes */
    { 0x12E,  3600.0, "s", "Duration since last readout" },  /* hours   */
    { 0x12F, 86400.0, "s", "Duration since last readout" },  /* days    */

    /* E011 0000 Start (date/time) of tariff  */
    /* The information about usage of data type F (date and time) or data type G (date) can */
    /* be derived from the datafield (0010b: type G / 0100: type F). */
    { 0x130, 1.0e0,  "Reserved", "Reserved" }, /* ???? */

    /* E011 00nn Duration of tariff (nn=01 ..11: min to days) */
    { 0x131,       60.0,  "s", "Duration of tariff" },   /* minute(s) */
    { 0x132,     3600.0,  "s", "Duration of tariff" },   /* hour(s)   */
    { 0x133,    86400.0,  "s", "Duration of tariff" },   /* day(s)    */

    /* E011 01nn Period of tariff [sec(s) to day(s)]  */
    { 0x134,        1.0, "s", "Period of tariff" },  /* seconds  */
    { 0x135,       60.0, "s", "Period of tariff" },  /* minutes  */
    { 0x136,     3600.0, "s", "Period of tariff" },  /* hours    */
    { 0x137,    86400.0, "s", "Period of tariff" },  /* days     */
    { 0x138,  2629743.83,"s", "Period of tariff" },  /* month(s) */
    { 0x139, 31556926.0, "s", "Period of tariff" },  /* year(s)  */

    /* E011 1010 dimensionless / no VIF */
    { 0x13A, 1.0e0,  "", "Dimensionless" },

    /* E011 1011 Reserved */
    { 0x13B, 1.0e0,  "Reserved", "Reserved" },

    /* E011 11xx Reserved */
    { 0x13C, 1.0e0,  "Reserved", "Reserved" },
    { 0x13D, 1.0e0,  "Reserved", "Reserved" },
    { 0x13E, 1.0e0,  "Reserved", "Reserved" },
    { 0x13F, 1.0e0,  "Reserved", "Reserved" },

    /* E100 nnnn   Volts electrical units */
    { 0x140, 1.0e-9, "V", "Voltage" },
    { 0x141, 1.0e-8, "V", "Voltage" },
    { 0x142, 1.0e-7, "V", "Voltage" },
    { 0x143, 1.0e-6, "V", "Voltage" },
    { 0x144, 1.0e-5, "V", "Voltage" },
    { 0x145, 1.0e-4, "V", "Voltage" },
    { 0x146, 1.0e-3, "V", "Voltage" },
    { 0x147, 1.0e-2, "V", "Voltage" },
    { 0x148, 1.0e-1, "V", "Voltage" },
    { 0x149, 1.0e0,  "V", "Voltage" },
    { 0x14A, 1.0e1,  "V", "Voltage" },
    { 0x14B, 1.0e2,  "V", "Voltage" },
    { 0x14C, 1.0e3,  "V", "Voltage" },
    { 0x14D, 1.0e4,  "V", "Voltage" },
    { 0x14E, 1.0e5,  "V", "Voltage" },
    { 0x14F, 1.0e6,  "V", "Voltage" },

    /* E101 nnnn   A */
    { 0x150, 1.0e-12, "A", "Current" },
    { 0x151, 1.0e-11, "A", "Current" },
    { 0x152, 1.0e-10, "A", "Current" },
    { 0x153, 1.0e-9,  "A", "Current" },
    { 0x154, 1.0e-8,  "A", "Current" },
    { 0x155, 1.0e-7,  "A", "Current" },
    { 0x156, 1.0e-6,  "A", "Current" },
    { 0x157, 1.0e-5,  "A", "Current" },
    { 0x158, 1.0e-4,  "A", "Current" },
    { 0x159, 1.0e-3,  "A", "Current" },
    { 0x15A, 1.0e-2,  "A", "Current" },
    { 0x15B, 1.0e-1,  "A", "Current" },
    { 0x15C, 1.0e0,   "A", "Current" },
    { 0x15D, 1.0e1,   "A", "Current" },
    { 0x15E, 1.0e2,   "A", "Current" },
    { 0x15F, 1.0e3,   "A", "Current" },

    /* E110 0000 Reset counter */
    { 0x160, 1.0e0,  "", "Reset counter" },

    /* E110 0001 Cumulation counter */
    { 0x161, 1.0e0,  "", "Cumulation counter" },

    /* E110 0010 Control signal */
    { 0x162, 1.0e0,  "", "Control signal" },

    /* E110 0011 Day of week */
    { 0x163, 1.0e0,  "", "Day of week" },

    /* E110 0100 Week number */
    { 0x164, 1.0e0,  "", "Week number" },

    /* E110 0101 Time point of day change */
    { 0x165, 1.0e0,  "", "Time point of day change" },

    /* E110 0110 State of parameter activation */
    { 0x166, 1.0e0,  "", "State of parameter activation" },

    /* E110 0111 Special supplier information */
    { 0x167, 1.0e0,  "", "Special supplier information" },

    /* E110 10pp Duration since last cumulation [hour(s)..years(s)] */
    { 0x168,     3600.0, "s", "Duration since last cumulation" },  /* hours    */
    { 0x169,    86400.0, "s", "Duration since last cumulation" },  /* days     */
    { 0x16A,  2629743.83,"s", "Duration since last cumulation" },  /* month(s) */
    { 0x16B, 31556926.0, "s", "Duration since last cumulation" },  /* year(s)  */

    /* E110 11pp Operating time battery [hour(s)..years(s)] */
    { 0x16C,     3600.0, "s", "Operating time battery" },  /* hours    */
    { 0x16D,    86400.0, "s", "Operating time battery" },  /* days     */
    { 0x16E,  2629743.83,"s", "Operating time battery" },  /* month(s) */
    { 0x16F, 31556926.0, "s", "Operating time battery" },  /* year(s)  */

    /* E111 0000 Date and time of battery change */
    { 0x170, 1.0e0,  "", "Date and time of battery change" },

    /* E111 0001-1111 Reserved */
    { 0x171, 1.0e0,  "Reserved", "Reserved" },
    { 0x172, 1.0e0,  "Reserved", "Reserved" },
    { 0x173, 1.0e0,  "Reserved", "Reserved" },
    { 0x174, 1.0e0,  "Reserved", "Reserved" },
    { 0x175, 1.0e0,  "Reserved", "Reserved" },
    { 0x176, 1.0e0,  "Reserved", "Reserved" },
    { 0x177, 1.0e0,  "Reserved", "Reserved" },
    { 0x178, 1.0e0,  "Reserved", "Reserved" },
    { 0x179, 1.0e0,  "Reserved", "Reserved" },
    { 0x17A, 1.0e0,  "Reserved", "Reserved" },
    { 0x17B, 1.0e0,  "Reserved", "Reserved" },
    { 0x17C, 1.0e0,  "Reserved", "Reserved" },
    { 0x17D, 1.0e0,  "Reserved", "Reserved" },
    { 0x17E, 1.0e0,  "Reserved", "Reserved" },
    { 0x17F, 1.0e0,  "Reserved", "Reserved" },


/* Alternate VIFE-Code Extension table (following VIF=0FBh for primary VIF)
   See 8.4.4 b, only some of them are here. Using range 0x200 - 0x2FF */

    /* E000 000n Energy 10(n-1) MWh 0.1MWh to 1MWh */
    { 0x200, 1.0e5,  "Wh", "Energy" },
    { 0x201, 1.0e6,  "Wh", "Energy" },

    /* E000 001n Reserved */
    { 0x202, 1.0e0,  "Reserved", "Reserved" },
    { 0x203, 1.0e0,  "Reserved", "Reserved" },

    /* E000 01nn Reserved */
    { 0x204, 1.0e0,  "Reserved", "Reserved" },
    { 0x205, 1.0e0,  "Reserved", "Reserved" },
    { 0x206, 1.0e0,  "Reserved", "Reserved" },
    { 0x207, 1.0e0,  "Reserved", "Reserved" },

    /* E000 100n Energy 10(n-1) GJ 0.1GJ to 1GJ */
    { 0x208, 1.0e8,  "Reserved", "Energy" },
    { 0x209, 1.0e9,  "Reserved", "Energy" },

    /* E000 101n Reserved */
    { 0x20A, 1.0e0,  "Reserved", "Reserved" },
    { 0x20B, 1.0e0,  "Reserved", "Reserved" },

    /* E000 11nn Reserved */
    { 0x20C, 1.0e0,  "Reserved", "Reserved" },
    { 0x20D, 1.0e0,  "Reserved", "Reserved" },
    { 0x20E, 1.0e0,  "Reserved", "Reserved" },
    { 0x20F, 1.0e0,  "Reserved", "Reserved" },

    /* E001 000n Volume 10(n+2) m3 100m3 to 1000m3 */
    { 0x210, 1.0e2,  "m^3", "Volume" },
    { 0x211, 1.0e3,  "m^3", "Volume" },

    /* E001 001n Reserved */
    { 0x212, 1.0e0,  "Reserved", "Reserved" },
    { 0x213, 1.0e0,  "Reserved", "Reserved" },

    /* E001 01nn Reserved */
    { 0x214, 1.0e0,  "Reserved", "Reserved" },
    { 0x215, 1.0e0,  "Reserved", "Reserved" },
    { 0x216, 1.0e0,  "Reserved", "Reserved" },
    { 0x217, 1.0e0,  "Reserved", "Reserved" },

    /* E001 100n Mass 10(n+2) t 100t to 1000t */
    { 0x218, 1.0e5,  "kg", "Mass" },
    { 0x219, 1.0e6,  "kg", "Mass" },

    /* E001 1010 to E010 0000 Reserved */
    { 0x21A, 1.0e0,  "Reserved", "Reserved" },
    { 0x21B, 1.0e0,  "Reserved", "Reserved" },
    { 0x21C, 1.0e0,  "Reserved", "Reserved" },
    { 0x21D, 1.0e0,  "Reserved", "Reserved" },
    { 0x21E, 1.0e0,  "Reserved", "Reserved" },
    { 0x21F, 1.0e0,  "Reserved", "Reserved" },
    { 0x220, 1.0e0,  "Reserved", "Reserved" },

    /* E010 0001 Volume 0,1 feet^3 */
    { 0x221, 1.0e-1, "feet^3", "Volume" },

    /* E010 001n Volume 0,1-1 american gallon */
    { 0x222, 1.0e-1, "American gallon", "Volume" },
    { 0x223, 1.0e-0, "American gallon", "Volume" },

    /* E010 0100    Volume flow 0,001 american gallon/min */
    { 0x224, 1.0e-3, "American gallon/min", "Volume flow" },

    /* E010 0101 Volume flow 1 american gallon/min */
    { 0x225, 1.0e0,  "American gallon/min", "Volume flow" },

    /* E010 0110 Volume flow 1 american gallon/h */
    { 0x226, 1.0e0,  "American gallon/h", "Volume flow" },

    /* E010 0111 Reserved */
    { 0x227, 1.0e0, "Reserved", "Reserved" },

    /* E010 100n Power 10(n-1) MW 0.1MW to 1MW */
    { 0x228, 1.0e5, "W", "Power" },
    { 0x229, 1.0e6, "W", "Power" },

    /* E010 101n Reserved */
    { 0x22A, 1.0e0, "Reserved", "Reserved" },
    { 0x22B, 1.0e0, "Reserved", "Reserved" },

    /* E010 11nn Reserved */
    { 0x22C, 1.0e0, "Reserved", "Reserved" },
    { 0x22D, 1.0e0, "Reserved", "Reserved" },
    { 0x22E, 1.0e0, "Reserved", "Reserved" },
    { 0x22F, 1.0e0, "Reserved", "Reserved" },

    /* E011 000n Power 10(n-1) GJ/h 0.1GJ/h to 1GJ/h */
    { 0x230, 1.0e8, "J", "Power" },
    { 0x231, 1.0e9, "J", "Power" },

    /* E011 0010 to E101 0111 Reserved */
    { 0x232, 1.0e0, "Reserved", "Reserved" },
    { 0x233, 1.0e0, "Reserved", "Reserved" },
    { 0x234, 1.0e0, "Reserved", "Reserved" },
    { 0x235, 1.0e0, "Reserved", "Reserved" },
    { 0x236, 1.0e0, "Reserved", "Reserved" },
    { 0x237, 1.0e0, "Reserved", "Reserved" },
    { 0x238, 1.0e0, "Reserved", "Reserved" },
    { 0x239, 1.0e0, "Reserved", "Reserved" },
    { 0x23A, 1.0e0, "Reserved", "Reserved" },
    { 0x23B, 1.0e0, "Reserved", "Reserved" },
    { 0x23C, 1.0e0, "Reserved", "Reserved" },
    { 0x23D, 1.0e0, "Reserved", "Reserved" },
    { 0x23E, 1.0e0, "Reserved", "Reserved" },
    { 0x23F, 1.0e0, "Reserved", "Reserved" },
    { 0x240, 1.0e0, "Reserved", "Reserved" },
    { 0x241, 1.0e0, "Reserved", "Reserved" },
    { 0x242, 1.0e0, "Reserved", "Reserved" },
    { 0x243, 1.0e0, "Reserved", "Reserved" },
    { 0x244, 1.0e0, "Reserved", "Reserved" },
    { 0x245, 1.0e0, "Reserved", "Reserved" },
    { 0x246, 1.0e0, "Reserved", "Reserved" },
    { 0x247, 1.0e0, "Reserved", "Reserved" },
    { 0x248, 1.0e0, "Reserved", "Reserved" },
    { 0x249, 1.0e0, "Reserved", "Reserved" },
    { 0x24A, 1.0e0, "Reserved", "Reserved" },
    { 0x24B, 1.0e0, "Reserved", "Reserved" },
    { 0x24C, 1.0e0, "Reserved", "Reserved" },
    { 0x24D, 1.0e0, "Reserved", "Reserved" },
    { 0x24E, 1.0e0, "Reserved", "Reserved" },
    { 0x24F, 1.0e0, "Reserved", "Reserved" },
    { 0x250, 1.0e0, "Reserved", "Reserved" },
    { 0x251, 1.0e0, "Reserved", "Reserved" },
    { 0x252, 1.0e0, "Reserved", "Reserved" },
    { 0x253, 1.0e0, "Reserved", "Reserved" },
    { 0x254, 1.0e0, "Reserved", "Reserved" },
    { 0x255, 1.0e0, "Reserved", "Reserved" },
    { 0x256, 1.0e0, "Reserved", "Reserved" },
    { 0x257, 1.0e0, "Reserved", "Reserved" },

    /* E101 10nn Flow Temperature 10(nn-3) °F 0.001°F to 1°F */
    { 0x258, 1.0e-3, "°F", "Flow temperature" },
    { 0x259, 1.0e-2, "°F", "Flow temperature" },
    { 0x25A, 1.0e-1, "°F", "Flow temperature" },
    { 0x25B, 1.0e0,  "°F", "Flow temperature" },

    /* E101 11nn Return Temperature 10(nn-3) °F 0.001°F to 1°F */
    { 0x25C, 1.0e-3, "°F", "Return temperature" },
    { 0x25D, 1.0e-2, "°F", "Return temperature" },
    { 0x25E, 1.0e-1, "°F", "Return temperature" },
    { 0x25F, 1.0e0,  "°F", "Return temperature" },

    /* E110 00nn Temperature Difference 10(nn-3) °F 0.001°F to 1°F */
    { 0x260, 1.0e-3, "°F", "Temperature difference" },
    { 0x261, 1.0e-2, "°F", "Temperature difference" },
    { 0x262, 1.0e-1, "°F", "Temperature difference" },
    { 0x263, 1.0e0,  "°F", "Temperature difference" },

    /* E110 01nn External Temperature 10(nn-3) °F 0.001°F to 1°F */
    { 0x264, 1.0e-3, "°F", "External temperature" },
    { 0x265, 1.0e-2, "°F", "External temperature" },
    { 0x266, 1.0e-1, "°F", "External temperature" },
    { 0x267, 1.0e0,  "°F", "External temperature" },

    /* E110 1nnn Reserved */
    { 0x268, 1.0e0, "Reserved", "Reserved" },
    { 0x269, 1.0e0, "Reserved", "Reserved" },
    { 0x26A, 1.0e0, "Reserved", "Reserved" },
    { 0x26B, 1.0e0, "Reserved", "Reserved" },
    { 0x26C, 1.0e0, "Reserved", "Reserved" },
    { 0x26D, 1.0e0, "Reserved", "Reserved" },
    { 0x26E, 1.0e0, "Reserved", "Reserved" },
    { 0x26F, 1.0e0, "Reserved", "Reserved" },

    /* E111 00nn Cold / Warm Temperature Limit 10(nn-3) °F 0.001°F to 1°F */
    { 0x270, 1.0e-3, "°F", "Cold / Warm Temperature Limit" },
    { 0x271, 1.0e-2, "°F", "Cold / Warm Temperature Limit" },
    { 0x272, 1.0e-1, "°F", "Cold / Warm Temperature Limit" },
    { 0x273, 1.0e0,  "°F", "Cold / Warm Temperature Limit" },

    /* E111 01nn Cold / Warm Temperature Limit 10(nn-3) °C 0.001°C to 1°C */
    { 0x274, 1.0e-3, "°C", "Cold / Warm Temperature Limit" },
    { 0x275, 1.0e-2, "°C", "Cold / Warm Temperature Limit" },
    { 0x276, 1.0e-1, "°C", "Cold / Warm Temperature Limit" },
    { 0x277, 1.0e0,  "°C", "Cold / Warm Temperature Limit" },

    /* E111 1nnn cumul. count max power § 10(nnn-3) W 0.001W to 10000W */
    { 0x278, 1.0e-3, "W", "Cumul count max power" },
    { 0x279, 1.0e-3, "W", "Cumul count max power" },
    { 0x27A, 1.0e-1, "W", "Cumul count max power" },
    { 0x27B, 1.0e0,  "W", "Cumul count max power" },
    { 0x27C, 1.0e1,  "W", "Cumul count max power" },
    { 0x27D, 1.0e2,  "W", "Cumul count max power" },
    { 0x27E, 1.0e3,  "W", "Cumul count max power" },
    { 0x27F, 1.0e4,  "W", "Cumul count max power" },

/* End of array */
    { 0xFFFF, 0.0, "", "" },
};


mbus_variable_vif fixed_table[] = {
    /* 00, 01 left out */
    { 0x02, 1.0e0, "Wh", "Energy" },
    { 0x03, 1.0e1, "Wh", "Energy" },
    { 0x04, 1.0e2, "Wh", "Energy" },
    { 0x05, 1.0e3, "Wh", "Energy" },
    { 0x06, 1.0e4, "Wh", "Energy" },
    { 0x07, 1.0e5, "Wh", "Energy" },
    { 0x08, 1.0e6, "Wh", "Energy" },
    { 0x09, 1.0e7, "Wh", "Energy" },
    { 0x0A, 1.0e8, "Wh", "Energy" },

    { 0x0B, 1.0e3, "J", "Energy" },
    { 0x0C, 1.0e4, "J", "Energy" },
    { 0x0D, 1.0e5, "J", "Energy" },
    { 0x0E, 1.0e6, "J", "Energy" },
    { 0x0F, 1.0e7, "J", "Energy" },
    { 0x10, 1.0e8, "J", "Energy" },
    { 0x11, 1.0e9, "J", "Energy" },
    { 0x12, 1.0e10,"J", "Energy" },
    { 0x13, 1.0e11,"J", "Energy" },

    { 0x14, 1.0e0, "W", "Power" },
    { 0x15, 1.0e0, "W", "Power" },
    { 0x16, 1.0e0, "W", "Power" },
    { 0x17, 1.0e0, "W", "Power" },
    { 0x18, 1.0e0, "W", "Power" },
    { 0x19, 1.0e0, "W", "Power" },
    { 0x1A, 1.0e0, "W", "Power" },
    { 0x1B, 1.0e0, "W", "Power" },
    { 0x1C, 1.0e0, "W", "Power" },

    { 0x1D, 1.0e3, "J/h", "Energy" },
    { 0x1E, 1.0e4, "J/h", "Energy" },
    { 0x1F, 1.0e5, "J/h", "Energy" },
    { 0x20, 1.0e6, "J/h", "Energy" },
    { 0x21, 1.0e7, "J/h", "Energy" },
    { 0x22, 1.0e8, "J/h", "Energy" },
    { 0x23, 1.0e9, "J/h", "Energy" },
    { 0x24, 1.0e10,"J/h", "Energy" },
    { 0x25, 1.0e11,"J/h", "Energy" },

    { 0x26, 1.0e-6,"m^3", "Volume" },
    { 0x27, 1.0e-5,"m^3", "Volume" },
    { 0x28, 1.0e-4,"m^3", "Volume" },
    { 0x29, 1.0e-3,"m^3", "Volume" },
    { 0x2A, 1.0e-2,"m^3", "Volume" },
    { 0x2B, 1.0e-1,"m^3", "Volume" },
    { 0x2C, 1.0e0, "m^3", "Volume" },
    { 0x2D, 1.0e1, "m^3", "Volume" },
    { 0x2E, 1.0e2, "m^3", "Volume" },

    { 0x2F, 1.0e-5,"m^3/h", "Volume flow" },
    { 0x31, 1.0e-4,"m^3/h", "Volume flow" },
    { 0x32, 1.0e-3,"m^3/h", "Volume flow" },
    { 0x33, 1.0e-2,"m^3/h", "Volume flow" },
    { 0x34, 1.0e-1,"m^3/h", "Volume flow" },
    { 0x35, 1.0e0, "m^3/h", "Volume flow" },
    { 0x36, 1.0e1, "m^3/h", "Volume flow" },
    { 0x37, 1.0e2, "m^3/h", "Volume flow" },

    { 0x38, 1.0e-3, "°C", "Temperature" },

    { 0x39, 1.0e0,  "Units for H.C.A.", "H.C.A." },

    { 0x3A, 0.0,  "Reserved", "Reserved" },
    { 0x3B, 0.0,  "Reserved", "Reserved" },
    { 0x3C, 0.0,  "Reserved", "Reserved" },
    { 0x3D, 0.0,  "Reserved", "Reserved" },

    { 0x3E, 1.0e0,  "", "historic" },

    { 0x3F, 1.0e0,  "", "No units" },

/* end of array */
    { 0xFFFF, 0.0, "", "" },
};

//------------------------------------------------------------------------------
/// Register a function for receive events.
//------------------------------------------------------------------------------
void
mbus_register_recv_event(mbus_handle * handle, void (*event)(unsigned char src_type, const char *buff, size_t len))
{
    handle->recv_event = event;
}

//------------------------------------------------------------------------------
/// Register a function for send events.
//------------------------------------------------------------------------------
void
mbus_register_send_event(mbus_handle * handle, void (*event)(unsigned char src_type, const char *buff, size_t len))
{
    handle->send_event = event;
}

//------------------------------------------------------------------------------
/// Register a function for the scan progress.
//------------------------------------------------------------------------------
void
mbus_register_scan_progress(mbus_handle * handle, void (*event)(mbus_handle * handle, const char *mask))
{
    handle->scan_progress = event;
}

//------------------------------------------------------------------------------
/// Register a function for the found events.
//------------------------------------------------------------------------------
void
mbus_register_found_event(mbus_handle * handle, void (*event)(mbus_handle * handle, mbus_frame *frame))
{
    handle->found_event = event;
}

int mbus_fixed_normalize(int medium_unit, long medium_value, char **unit_out, double *value_out, char **quantity_out)
{
    medium_unit = medium_unit & 0x3F;

    if (unit_out == NULL || value_out == NULL || quantity_out == NULL)
    {
        MBUS_ERROR("%s: Invalid parameter.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    switch (medium_unit)
    {
        case 0x00:
            *unit_out = strdup("h,m,s"); /*  todo convert to unix time... */
            *quantity_out = strdup("Time");
            break;
        case 0x01:
            *unit_out = strdup("D,M,Y"); /*  todo convert to unix time... */
            *quantity_out = strdup("Time");
            break;

    default:
        for(int i=0; fixed_table[i].vif < 0xfff; ++i)
        {
            if (fixed_table[i].vif == medium_unit)
            {
                *unit_out = strdup(fixed_table[i].unit);
                *value_out = ((double) (medium_value)) * fixed_table[i].exponent;
                *quantity_out = strdup(fixed_table[i].quantity);
                return 0;
            }
        }

        *unit_out = strdup("Unknown");
        *quantity_out = strdup("Unknown");
        *value_out = 0.0;
        return -1;
    }

    return -2;
}


int mbus_variable_value_decode(mbus_data_record *record, double *value_out_real, char **value_out_str, int *value_out_str_size)
{
    int result = 0;
    unsigned char vif, vife;
    struct tm time;
    int value_out_int;
    long long value_out_long_long;
    *value_out_real = 0.0;
    *value_out_str = NULL;
    *value_out_str_size = 0;

    if (record)
    {
        MBUS_DEBUG("coding = 0x%02X \n", record->drh.dib.dif);

        // ignore extension bit
        vif = (record->drh.vib.vif & MBUS_DIB_VIF_WITHOUT_EXTENSION);
        vife = (record->drh.vib.vife[0] & MBUS_DIB_VIF_WITHOUT_EXTENSION);

        switch (record->drh.dib.dif & MBUS_DATA_RECORD_DIF_MASK_DATA)
        {
            case 0x00: /* no data */
                if ((*value_out_str = (char*) malloc(1)) == NULL)
                {
                    MBUS_ERROR("Unable to allocate memory");
                    return -1;
                }
                *value_out_str[0] = '\0';
                *value_out_str_size = 0;
                result = 0;
                break;

            case 0x01: /* 1 byte integer (8 bit) */
                result = mbus_data_int_decode(record->data, 1, &value_out_int);
                *value_out_real = value_out_int;
                break;

            case 0x02: /* 2 byte integer (16 bit) */
                // E110 1100  Time Point (date)
                if (vif == 0x6C)
                {
                    mbus_data_tm_decode(&time, record->data, 2);
                    if ((*value_out_str = (char*) malloc(11)) == NULL)
                    {
                        MBUS_ERROR("Unable to allocate memory");
                        return -1;
                    }
                    *value_out_str_size = snprintf(*value_out_str, 11, "%04d-%02d-%02d",
                                                 (time.tm_year + 1900),
                                                 (time.tm_mon + 1),
                                                  time.tm_mday);
                    result = 0;
                }
                else  // normal integer
                {
                    result = mbus_data_int_decode(record->data, 2, &value_out_int);
                    *value_out_real = value_out_int;
                }
                break;

            case 0x03: /* 3 byte integer (24 bit) */
                result = mbus_data_int_decode(record->data, 3, &value_out_int);
                *value_out_real = value_out_int;
                break;

            case 0x04: /* 4 byte integer (32 bit) */
                // E110 1101  Time Point (date/time)
                // E011 0000  Start (date/time) of tariff
                // E111 0000  Date and time of battery change
                if ( (vif == 0x6D) ||
                    ((record->drh.vib.vif == 0xFD) && (vife == 0x30)) ||
                    ((record->drh.vib.vif == 0xFD) && (vife == 0x70)))
                {
                    mbus_data_tm_decode(&time, record->data, 4);
                    if ((*value_out_str = (char*) malloc(21)) == NULL)
                    {
                        MBUS_ERROR("Unable to allocate memory");
                        return -1;
                    }
                    *value_out_str_size = snprintf(*value_out_str, 21, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                                                 (time.tm_year + 1900),
                                                 (time.tm_mon + 1),
                                                  time.tm_mday,
                                                  time.tm_hour,
                                                  time.tm_min,
                                                  time.tm_sec);
                    result = 0;
                }
                else  // normal integer
                {
                    result = mbus_data_int_decode(record->data, 4, &value_out_int);
                    *value_out_real = value_out_int;
                }
                break;

            case 0x05: /* 32b real */
                *value_out_real = mbus_data_float_decode(record->data);
                result = 0;
                break;

            case 0x06: /* 6 byte integer (48 bit) */
                // E110 1101  Time Point (date/time)
                // E011 0000  Start (date/time) of tariff
                // E111 0000  Date and time of battery change
                if ( (vif == 0x6D) ||
                    ((record->drh.vib.vif == 0xFD) && (vife == 0x30)) ||
                    ((record->drh.vib.vif == 0xFD) && (vife == 0x70)))
                {
                    mbus_data_tm_decode(&time, record->data, 6);
                    if ((*value_out_str = (char*) malloc(21)) == NULL)
                    {
                        MBUS_ERROR("Unable to allocate memory");
                        return -1;
                    }
                    *value_out_str_size = snprintf(*value_out_str, 21, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                                                 (time.tm_year + 1900),
                                                 (time.tm_mon + 1),
                                                  time.tm_mday,
                                                  time.tm_hour,
                                                  time.tm_min,
                                                  time.tm_sec);
                    result = 0;
                }
                else  // normal integer
                {
                    result = mbus_data_long_long_decode(record->data, 6, &value_out_long_long);
                    *value_out_real = value_out_long_long;
                }
                break;

            case 0x07: /* 8 byte integer (64 bit) */
                result = mbus_data_long_long_decode(record->data, 8, &value_out_long_long);
                *value_out_real = value_out_long_long;
                break;

            case 0x09: /* 2 digit BCD (8 bit) */
                *value_out_real = mbus_data_bcd_decode(record->data, 1);
                result = 0;
                break;

            case 0x0A: /* 4 digit BCD (16 bit) */
                *value_out_real = mbus_data_bcd_decode(record->data, 2);
                result = 0;
                break;

            case 0x0B: /* 6 digit BCD (24 bit) */
                *value_out_real = mbus_data_bcd_decode(record->data, 3);
                result = 0;
                break;

            case 0x0C: /* 8 digit BCD (32 bit) */
                *value_out_real = mbus_data_bcd_decode(record->data, 4);
                result = 0;
                break;

            case 0x0D: /* variable length */
            {
                if (record->data_len <= 0xBF) {
                    if ((*value_out_str = (char*) malloc(record->data_len + 1)) == NULL)
                    {
                        MBUS_ERROR("Unable to allocate memory");
                        return -1;
                    }
                    *value_out_str_size = record->data_len;
                    mbus_data_str_decode((unsigned char*)(*value_out_str), record->data, record->data_len);
                    result = 0;
                    break;
                }
                result = -2;
                MBUS_ERROR("Non ASCII variable length not implemented yet\n");
                break;
            }

            case 0x0E: /* 12 digit BCD (40 bit) */
                *value_out_real = mbus_data_bcd_decode(record->data, 6);
                result = 0;
                break;

            case 0x0F: /* Special functions */
                if ((*value_out_str = (char*) malloc(3 * record->data_len + 1)) == NULL)
                {
                    MBUS_ERROR("Unable to allocate memory");
                    return -1;
                }
                *value_out_str_size = 3 * record->data_len;
                mbus_data_bin_decode((unsigned char*)(*value_out_str), record->data, record->data_len, (3 * record->data_len + 1));
                result = 0;
                break;

            default:
                result = -2;
                MBUS_ERROR("Unknown DIF (0x%.2x)", record->drh.dib.dif);
                break;
        }
    }
    else
    {
        MBUS_ERROR("record is null");
        result = -3;
    }

    return result;
}

int
mbus_vif_unit_normalize(int vif, double value, char **unit_out, double *value_out, char **quantity_out)
{
    int i;
    unsigned newVif = vif & 0xF7F; /* clear extension bit */

    MBUS_DEBUG("vif_unit_normalize = 0x%03X \n", vif);

    if (unit_out == NULL || value_out == NULL || quantity_out == NULL)
    {
        MBUS_ERROR("%s: Invalid parameter.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    for(i=0; vif_table[i].vif < 0xfff; ++i)
    {
        if (vif_table[i].vif == newVif)
        {
            *unit_out = strdup(vif_table[i].unit);
            *value_out = value * vif_table[i].exponent;
            *quantity_out = strdup(vif_table[i].quantity);
            return 0;
        }
    }

    MBUS_ERROR("%s: Unknown VIF 0x%03X\n", __PRETTY_FUNCTION__, newVif);
    *unit_out = strdup("Unknown (VIF=0x%.02X)");
    *quantity_out = strdup("Unknown");
    *value_out = 0.0;
    return -1;
}


int
mbus_vib_unit_normalize(mbus_value_information_block *vib, double value, char **unit_out, double *value_out, char **quantity_out)
{
    int code;

    if (vib == NULL || unit_out == NULL || value_out == NULL || quantity_out == NULL)
    {
        MBUS_ERROR("%s: Invalid parameter.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    MBUS_DEBUG("%s: vib_unit_normalize - VIF=0x%02X\n", __PRETTY_FUNCTION__, vib->vif);

    if (vib->vif == 0xFD) /* first type of VIF extention: see table 8.4.4 a */
    {
        if (vib->nvife == 0)
        {
            MBUS_ERROR("%s: Missing VIF extension\n", __PRETTY_FUNCTION__);
            return -1;
        }

        code = ((vib->vife[0]) & MBUS_DIB_VIF_WITHOUT_EXTENSION) | 0x100;
        if (mbus_vif_unit_normalize(code, value, unit_out, value_out, quantity_out) != 0)
        {
            MBUS_ERROR("%s: Error mbus_vif_unit_normalize\n", __PRETTY_FUNCTION__);
            return -1;
        }
    } else {
        if (vib->vif == 0xFB) /* second type of VIF extention: see table 8.4.4 b */
        {
            if (vib->nvife == 0)
            {
                MBUS_ERROR("%s: Missing VIF extension\n", __PRETTY_FUNCTION__);
                return -1;
            }

            code = ((vib->vife[0]) & MBUS_DIB_VIF_WITHOUT_EXTENSION) | 0x200;
            if (0 != mbus_vif_unit_normalize(code, value, unit_out, value_out, quantity_out))
            {
                MBUS_ERROR("%s: Error mbus_vif_unit_normalize\n", __PRETTY_FUNCTION__);
                return -1;
            }
        }
        else if ((vib->vif == 0x7C) ||
                 (vib->vif == 0xFC))
        {
            // custom VIF
            *unit_out = strdup("-");
            *quantity_out = strdup(vib->custom_vif);
            *value_out = value;
        }
        else
        {
            code = (vib->vif) & MBUS_DIB_VIF_WITHOUT_EXTENSION;
            if (0 != mbus_vif_unit_normalize(code, value, unit_out, value_out, quantity_out))
            {
                MBUS_ERROR("%s: Error mbus_vif_unit_normalize\n", __PRETTY_FUNCTION__);
                return -1;
            }
        }
    }

    if ((vib->vif & MBUS_DIB_VIF_EXTENSION_BIT) &&
        (vib->vif != 0xFD) &&
        (vib->vif != 0xFB))                       /* codes for VIF extention: see table 8.4.5 */
    {
        code = (vib->vife[0]) & 0x7f;
        switch (code)
        {
            case 0x70:
            case 0x71:
            case 0x72:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x76:
            case 0x77: /* Multiplicative correction factor: 10^nnn-6 */
                *value_out *= pow(10.0, (vib->vife[0] & 0x07) - 6);
                break;

            case 0x78:
            case 0x79:
            case 0x7A:
            case 0x7B: /* Additive correction constant: 10^nn-3 unit of VIF (offset) */
                *value_out += pow(10.0, (vib->vife[0] & 0x03) - 3);
                break;

            case 0x7D: /* Multiplicative correction factor: 10^3 */
                *value_out *= 1000.0;
                break;
        }
    }

    return 0;
}


mbus_record *
mbus_record_new()
{
    mbus_record * record;

    if (!(record = (mbus_record *) malloc(sizeof(mbus_record))))
    {
        MBUS_ERROR("%s: memory allocation error\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    record->value.real_val = 0.0;
    record->is_numeric = 1;
    record->unit = NULL;
    record->function_medium = NULL;
    record->quantity = NULL;
    record->device = -1;
    record->tariff = -1;
    record->storage_number = 0;
    return record;
}


void
mbus_record_free(mbus_record * rec)
{
    if (rec)
    {
        if (! rec->is_numeric)
        {
            free((rec->value).str_val.value);
            (rec->value).str_val.value = NULL;
        }

        if (rec->unit)
        {
            free(rec->unit);
            rec->unit = NULL;
        }

        if (rec->function_medium)
        {
            free(rec->function_medium);
            rec->function_medium = NULL;
        }

        if (rec->quantity)
        {
            free(rec->quantity);
            rec->quantity = NULL;
        }
        free(rec);
    }
}


mbus_record *
mbus_parse_fixed_record(char status_byte, char medium_unit, unsigned char *data)
{
    long value = 0;
    mbus_record * record = NULL;

    if (!(record = mbus_record_new()))
    {
        MBUS_ERROR("%s: memory allocation error\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    /* shared/static memory - get own copy */
    record->function_medium = strdup(mbus_data_fixed_function((int)status_byte));  /* stored / actual */

    if (record->function_medium == NULL)
    {
        MBUS_ERROR("%s: memory allocation error\n", __PRETTY_FUNCTION__);
        mbus_record_free(record);
        return NULL;
    }

    if ((status_byte & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD)
    {
        value = mbus_data_bcd_decode(data, 4);
    }
    else
    {
        mbus_data_long_decode(data, 4, &value);
    }

    record->unit = NULL;
    record->is_numeric = 1;
    if (0 != mbus_fixed_normalize(medium_unit, value, &(record->unit), &(record->value.real_val), &(record->quantity)))
    {
        MBUS_ERROR("Problem with mbus_fixed_normalize.\n");
        mbus_record_free(record);
        return NULL;
    }

    return record;
}


mbus_record *
mbus_parse_variable_record(mbus_data_record *data)
{
    mbus_record * record = NULL;
    double value_out_real    = 0.0;  /**< raw value */
    char * value_out_str     = NULL;
    int    value_out_str_size = 0;
    double real_val         = 0.0;  /**< normalized value */

    if (data == NULL)
    {
        MBUS_ERROR("%s: Invalid record.\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    if (!(record = mbus_record_new()))
    {
        MBUS_ERROR("%s: memory allocation error\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    record->storage_number = mbus_data_record_storage_number(data);
    record->tariff = mbus_data_record_tariff(data);
    record->device = mbus_data_record_device(data);

    if ((data->drh.dib.dif == MBUS_DIB_DIF_MANUFACTURER_SPECIFIC) ||
        (data->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW)) /* MBUS_DIB_DIF_VENDOR_SPECIFIC */
    {
        if (data->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW)
        {
            record->function_medium = strdup("More records follow");
        }
        else
        {
            record->function_medium = strdup("Manufacturer specific");
        }

        if (record->function_medium == NULL)
        {
            MBUS_ERROR("%s: memory allocation error\n", __PRETTY_FUNCTION__);
            mbus_record_free(record);
            return NULL;
        }

        /* parsing of data not implemented yet
           manufacturer specific data structures to end of user data */

        if (mbus_variable_value_decode(data, &value_out_real, &value_out_str, &value_out_str_size) != 0)
        {
            MBUS_ERROR("%s: problem with mbus_variable_value_decode\n", __PRETTY_FUNCTION__);
            mbus_record_free(record);
            return NULL;
        }

        if (value_out_str != NULL)
        {
            record->is_numeric = 0;
            (record->value).str_val.value = value_out_str;
            (record->value).str_val.size = value_out_str_size;
        }
        else
        {
            record->is_numeric = 1;
            (record->value).real_val = real_val;
        }
    }
    else
    {
        record->function_medium = strdup(mbus_data_record_function(data));

        if (record->function_medium == NULL)
        {
            MBUS_ERROR("%s: memory allocation error\n", __PRETTY_FUNCTION__);
            mbus_record_free(record);
            return NULL;
        }

        MBUS_DEBUG("record->function_medium = %s \n", record->function_medium);

        if (mbus_variable_value_decode(data, &value_out_real, &value_out_str, &value_out_str_size) != 0)
        {
            MBUS_ERROR("%s: problem with mbus_variable_value_decode\n", __PRETTY_FUNCTION__);
            mbus_record_free(record);
            return NULL;
        }
        MBUS_DEBUG("value_out_real = %lf \n", value_out_real);

        if (mbus_vib_unit_normalize(&(data->drh.vib), value_out_real, &(record->unit), &real_val, &(record->quantity)) != 0)
        {
            MBUS_ERROR("%s: problem with mbus_vib_unit_normalize\n", __PRETTY_FUNCTION__);
            mbus_record_free(record);
            record = NULL;
            return NULL;
        }
        MBUS_DEBUG("record->unit = %s \n", record->unit);
        MBUS_DEBUG("real_val = %lf \n", real_val);

        if (value_out_str != NULL)
        {
            record->is_numeric = 0;
            (record->value).str_val.value = value_out_str;
            (record->value).str_val.size = value_out_str_size;
        }
        else
        {
            record->is_numeric = 1;
            (record->value).real_val = real_val;
        }
    }

    return record;
}

//------------------------------------------------------------------------------
/// Generate XML for variable-length data
//------------------------------------------------------------------------------
char *
mbus_data_variable_xml_normalized(mbus_data_variable *data)
{
    mbus_data_record *record;
    mbus_record *norm_record;
    char *buff = NULL, *new_buff = NULL;
    char str_encoded[768] = "";
    size_t len = 0, buff_size = 8192;
    size_t i;

    if (data)
    {
        buff = (char*) malloc(buff_size);

        if (buff == NULL)
            return NULL;

        len += snprintf(&buff[len], buff_size - len, MBUS_XML_PROCESSING_INSTRUCTION);

        len += snprintf(&buff[len], buff_size - len, "<MBusData>\n\n");

        len += snprintf(&buff[len], buff_size - len, "%s", mbus_data_variable_header_xml(&(data->header)));

        for (record = data->record, i = 0; record; record = record->next, i++)
        {
            norm_record = mbus_parse_variable_record(record);

            if ((buff_size - len) < 1024)
            {
                buff_size *= 2;
                new_buff = (char*) realloc(buff,buff_size);

                if (new_buff == NULL)
                {
                    mbus_record_free(norm_record);
                    free(buff);
                    return NULL;
                }

                buff = new_buff;
            }

            len += snprintf(&buff[len], buff_size - len, "    <DataRecord id=\"%zu\">\n", i);

            if (norm_record != NULL)
            {
                mbus_str_xml_encode(str_encoded, norm_record->function_medium, sizeof(str_encoded));
                len += snprintf(&buff[len], buff_size - len, "        <Function>%s</Function>\n", str_encoded);

                len += snprintf(&buff[len], buff_size - len, "        <StorageNumber>%ld</StorageNumber>\n", norm_record->storage_number);

                if (norm_record->tariff >= 0)
                {
                    len += snprintf(&buff[len], buff_size - len, "        <Tariff>%ld</Tariff>\n", norm_record->tariff);
                    len += snprintf(&buff[len], buff_size - len, "        <Device>%d</Device>\n", norm_record->device);
                }

                mbus_str_xml_encode(str_encoded, norm_record->unit, sizeof(str_encoded));

                len += snprintf(&buff[len], buff_size - len, "        <Unit>%s</Unit>\n", str_encoded);

                mbus_str_xml_encode(str_encoded, norm_record->quantity, sizeof(str_encoded));
                len += snprintf(&buff[len], buff_size - len, "        <Quantity>%s</Quantity>\n", str_encoded);


                if (norm_record->is_numeric)
                {
                    len += snprintf(&buff[len], buff_size - len, "        <Value>%f</Value>\n", norm_record->value.real_val);
                }
                else
                {
                    mbus_str_xml_encode(str_encoded, norm_record->value.str_val.value, sizeof(str_encoded));
                    len += snprintf(&buff[len], buff_size - len, "        <Value>%s</Value>\n", str_encoded);
                }

                mbus_record_free(norm_record);
            }
            else
            {
            }

            len += snprintf(&buff[len], buff_size - len, "    </DataRecord>\n\n");
        }

        len += snprintf(&buff[len], buff_size - len, "</MBusData>\n");

        return buff;
    }

    return NULL;
}

//------------------------------------------------------------------------------
/// Return a string containing an XML representation of the M-BUS frame data.
//------------------------------------------------------------------------------
char *
mbus_frame_data_xml_normalized(mbus_frame_data *data)
{
    if (data)
    {
        if (data->type == MBUS_DATA_TYPE_FIXED)
        {
            return mbus_data_fixed_xml(&(data->data_fix));
        }

        if (data->type == MBUS_DATA_TYPE_VARIABLE)
        {
            return mbus_data_variable_xml_normalized(&(data->data_var));
        }
    }

    return NULL;
}

mbus_handle *
mbus_context_serial(const char *device)
{
    mbus_handle *handle;
    mbus_serial_data *serial_data;
    char error_str[128];

    if ((handle = (mbus_handle *) malloc(sizeof(mbus_handle))) == NULL)
    {
        MBUS_ERROR("%s: Failed to allocate handle.\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    if ((serial_data = (mbus_serial_data *)malloc(sizeof(mbus_serial_data))) == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: failed to allocate memory for handle\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        free(handle);
        return NULL;
    }

    handle->max_data_retry = 3;
    handle->max_search_retry = 1;
    handle->is_serial = 1;
    handle->purge_first_frame = MBUS_FRAME_PURGE_M2S;
    handle->auxdata = serial_data;
    handle->open = mbus_serial_connect;
    handle->close = mbus_serial_disconnect;
    handle->recv = mbus_serial_recv_frame;
    handle->send = mbus_serial_send_frame;
    handle->free_auxdata = mbus_serial_data_free;
    handle->recv_event = NULL;
    handle->send_event = NULL;
    handle->scan_progress = NULL;
    handle->found_event = NULL;

    if ((serial_data->device = strdup(device)) == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: failed to allocate memory for device\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        free(serial_data);
        free(handle);
        return NULL;
    }

    return handle;
}

mbus_handle *
mbus_context_tcp(const char *host, uint16_t port)
{
    mbus_handle *handle;
    mbus_tcp_data *tcp_data;
    char error_str[128];

    if ((handle = (mbus_handle *) malloc(sizeof(mbus_handle))) == NULL)
    {
        MBUS_ERROR("%s: Failed to allocate handle.\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    if ((tcp_data = (mbus_tcp_data *)malloc(sizeof(mbus_tcp_data))) == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: failed to allocate memory for handle\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        free(handle);
        return NULL;
    }

    handle->max_data_retry = 3;
    handle->max_search_retry = 1;
    handle->is_serial = 0;
    handle->purge_first_frame = MBUS_FRAME_PURGE_M2S;
    handle->auxdata = tcp_data;
    handle->open = mbus_tcp_connect;
    handle->close = mbus_tcp_disconnect;
    handle->recv = mbus_tcp_recv_frame;
    handle->send = mbus_tcp_send_frame;
    handle->free_auxdata = mbus_tcp_data_free;
    handle->recv_event = NULL;
    handle->send_event = NULL;
    handle->scan_progress = NULL;
    handle->found_event = NULL;

    tcp_data->port = port;
    if ((tcp_data->host = strdup(host)) == NULL)
    {
        snprintf(error_str, sizeof(error_str), "%s: failed to allocate memory for host\n", __PRETTY_FUNCTION__);
        mbus_error_str_set(error_str);
        free(tcp_data);
        free(handle);
        return NULL;
    }

    return handle;
}

void
mbus_context_free(mbus_handle * handle)
{
    if (handle)
    {
        handle->free_auxdata(handle);
        free(handle);
    }
}

int
mbus_connect(mbus_handle * handle)
{
    if (handle == NULL)
    {
        MBUS_ERROR("%s: Invalid M-Bus handle for disconnect.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    return handle->open(handle);
}

int
mbus_disconnect(mbus_handle * handle)
{
    if (handle == NULL)
    {
        MBUS_ERROR("%s: Invalid M-Bus handle for disconnect.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    return handle->close(handle);
}

int
mbus_context_set_option(mbus_handle * handle, mbus_context_option option, long value)
{
    if (handle == NULL)
    {
        MBUS_ERROR("%s: Invalid M-Bus handle to set option.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    switch (option)
    {
        case MBUS_OPTION_MAX_DATA_RETRY:
            if ((value >= 0) && (value <= 9))
            {
                handle->max_data_retry = value;
                return 0;
            }
            break;
        case MBUS_OPTION_MAX_SEARCH_RETRY:
            if ((value >= 0) && (value <= 9))
            {
                handle->max_search_retry = value;
                return 0;
            }
            break;
        case MBUS_OPTION_PURGE_FIRST_FRAME:
            if ((value == MBUS_FRAME_PURGE_NONE) ||
                (value == MBUS_FRAME_PURGE_M2S)  ||
                (value == MBUS_FRAME_PURGE_S2M))
            {
                handle->purge_first_frame = value;
                return 0;
            }
            break;
    }

    return -1; // unable to set option
}

int
mbus_recv_frame(mbus_handle * handle, mbus_frame *frame)
{
    int result = 0;

    if (handle == NULL)
    {
        MBUS_ERROR("%s: Invalid M-Bus handle for receive.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_ERROR;
    }

    if (frame == NULL)
    {
        MBUS_ERROR("%s: Invalid frame.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_ERROR;
    }

    result = handle->recv(handle, frame);

    switch (mbus_frame_direction(frame))
    {
        case MBUS_CONTROL_MASK_DIR_M2S:
            if (handle->purge_first_frame == MBUS_FRAME_PURGE_M2S)
                result = handle->recv(handle, frame);  // purge echo and retry
            break;
        case MBUS_CONTROL_MASK_DIR_S2M:
            if (handle->purge_first_frame == MBUS_FRAME_PURGE_S2M)
                result = handle->recv(handle, frame);  // purge echo and retry
            break;
    }

    if (frame != NULL)
    {
        /* set timestamp to receive time */
        time(&(frame->timestamp));
    }

    return result;
}

int mbus_purge_frames(mbus_handle *handle)
{
    int err, received;
    mbus_frame reply;

    memset((void *)&reply, 0, sizeof(mbus_frame));

    received = 0;
    while (1)
    {
        err = mbus_recv_frame(handle, &reply);
        if (err != MBUS_RECV_RESULT_OK &&
            err != MBUS_RECV_RESULT_INVALID)
            break;

        received = 1;
    }

    return received;
}

int
mbus_send_frame(mbus_handle * handle, mbus_frame *frame)
{
    if (handle == NULL)
    {
        MBUS_ERROR("%s: Invalid M-Bus handle for send.\n", __PRETTY_FUNCTION__);
        return 0;
    }

    return handle->send(handle, frame);
}

//------------------------------------------------------------------------------
// send a data request packet to from master to slave: the packet selects
// a slave to be the active secondary addressed slave if the secondary address
// matches that of the slave.
//------------------------------------------------------------------------------
int
mbus_send_select_frame(mbus_handle * handle, const char *secondary_addr_str)
{
    mbus_frame *frame;

    frame = mbus_frame_new(MBUS_FRAME_TYPE_LONG);

    if (mbus_frame_select_secondary_pack(frame, (char*) secondary_addr_str) == -1)
    {
        MBUS_ERROR("%s: Failed to pack selection mbus frame.\n", __PRETTY_FUNCTION__);
        mbus_frame_free(frame);
        return -1;
    }

    if (mbus_send_frame(handle, frame) == -1)
    {
        MBUS_ERROR("%s: Failed to send mbus frame.\n", __PRETTY_FUNCTION__);
        mbus_frame_free(frame);
        return -1;
    }

    mbus_frame_free(frame);
    return 0;
}

//------------------------------------------------------------------------------
// send a user data packet from master to slave: the packet let the
// adressed slave(s) switch to the given baudrate
//------------------------------------------------------------------------------
int
mbus_send_switch_baudrate_frame(mbus_handle * handle, int address, long baudrate)
{
    int retval = 0;
    int control_information = 0;
    mbus_frame *frame;

    if (mbus_is_primary_address(address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, address);
        return -1;
    }

    switch (baudrate)
    {
      case 300:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_300;
        break;
      case 600:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_600;
        break;
      case 1200:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_1200;
        break;
      case 2400:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_2400;
        break;
      case 4800:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_4800;
        break;
      case 9600:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_9600;
        break;
      case 19200:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_19200;
        break;
      case 38400:
        control_information = MBUS_CONTROL_INFO_SET_BAUDRATE_38400;
        break;
      default:
        MBUS_ERROR("%s: invalid baudrate %ld\n", __PRETTY_FUNCTION__, baudrate);
        return -1;
    }

    frame = mbus_frame_new(MBUS_FRAME_TYPE_CONTROL);

    if (frame == NULL)
    {
        MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control = MBUS_CONTROL_MASK_SND_UD | MBUS_CONTROL_MASK_DIR_M2S;
    frame->address = address;
    frame->control_information = control_information;

    if (mbus_send_frame(handle, frame) == -1)
    {
        MBUS_ERROR("%s: failed to send mbus frame.\n", __PRETTY_FUNCTION__);
        retval = -1;
    }

    mbus_frame_free(frame);
    return retval;
}

//------------------------------------------------------------------------------
// send a user data packet from master to slave: the packet resets
// the application layer in the slave
//------------------------------------------------------------------------------
int
mbus_send_application_reset_frame(mbus_handle * handle, int address, int subcode)
{
    int retval = 0;
    mbus_frame *frame;

    if (mbus_is_primary_address(address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, address);
        return -1;
    }

    if (subcode > 0xFF)
    {
        MBUS_ERROR("%s: invalid subcode %d\n", __PRETTY_FUNCTION__, subcode);
        return -1;
    }

    frame = mbus_frame_new(MBUS_FRAME_TYPE_LONG);

    if (frame == NULL)
    {
        MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control = MBUS_CONTROL_MASK_SND_UD | MBUS_CONTROL_MASK_DIR_M2S;
    frame->address = address;
    frame->control_information = MBUS_CONTROL_INFO_APPLICATION_RESET;

    if (subcode >= 0)
    {
        frame->data_size = 1;
        frame->data[0] = (subcode & 0xFF);
    }
    else
    {
        frame->data_size = 0;
    }

    if (mbus_send_frame(handle, frame) == -1)
    {
        MBUS_ERROR("%s: failed to send mbus frame.\n", __PRETTY_FUNCTION__);
        retval = -1;
    }

    mbus_frame_free(frame);
    return retval;
}


//------------------------------------------------------------------------------
// send a request packet to from master to slave
//------------------------------------------------------------------------------
int
mbus_send_request_frame(mbus_handle * handle, int address)
{
    int retval = 0;
    mbus_frame *frame;

    if (mbus_is_primary_address(address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, address);
        return -1;
    }

    frame = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);

    if (frame == NULL)
    {
        MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control = MBUS_CONTROL_MASK_REQ_UD2 | MBUS_CONTROL_MASK_DIR_M2S;
    frame->address = address;

    if (mbus_send_frame(handle, frame) == -1)
    {
        MBUS_ERROR("%s: failed to send mbus frame.\n", __PRETTY_FUNCTION__);
        retval = -1;
    }

    mbus_frame_free(frame);
    return retval;
}

//------------------------------------------------------------------------------
// send a user data packet from master to slave
//------------------------------------------------------------------------------
int
mbus_send_user_data_frame(mbus_handle * handle, int address, const unsigned char *data, size_t data_size)
{
    int retval = 0;
    mbus_frame *frame;

    if (mbus_is_primary_address(address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, address);
        return -1;
    }

    if (data == NULL)
    {
        MBUS_ERROR("%s: Invalid data\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if ((data_size > MBUS_FRAME_DATA_LENGTH) || (data_size == 0))
    {
        MBUS_ERROR("%s: illegal data_size %zu\n", __PRETTY_FUNCTION__, data_size);
        return -1;
    }

    frame = mbus_frame_new(MBUS_FRAME_TYPE_LONG);

    if (frame == NULL)
    {
        MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control = MBUS_CONTROL_MASK_SND_UD | MBUS_CONTROL_MASK_DIR_M2S;
    frame->address = address;
    frame->control_information = MBUS_CONTROL_INFO_DATA_SEND;
    frame->data_size = data_size;
    memcpy(frame->data, data, data_size);

    if (mbus_send_frame(handle, frame) == -1)
    {
        MBUS_ERROR("%s: failed to send mbus frame.\n", __PRETTY_FUNCTION__);
        retval = -1;
    }

    mbus_frame_free(frame);
    return retval;
}

//------------------------------------------------------------------------------
// send a request from master to slave in order to change the primary address
//------------------------------------------------------------------------------
int
mbus_set_primary_address(mbus_handle * handle, int old_address, int new_address)
{
    /* primary address record, see chapter 6.4.2 */
    unsigned char buffer[3] = { 0x01, 0x7A, new_address };

    if (mbus_is_primary_address(new_address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, new_address);
        return -1;
    }

    switch (new_address)
    {
        case MBUS_ADDRESS_NETWORK_LAYER:
        case MBUS_ADDRESS_BROADCAST_REPLY:
        case MBUS_ADDRESS_BROADCAST_NOREPLY:
            MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, new_address);
            return -1;
    }

    return mbus_send_user_data_frame(handle, old_address, buffer, sizeof(buffer));
}

//------------------------------------------------------------------------------
// send a request from master to slave and collect the reply (replies)
// from the slave.
//------------------------------------------------------------------------------
int
mbus_sendrecv_request(mbus_handle *handle, int address, mbus_frame *reply, int max_frames)
{
    int retval = 0, more_frames = 1, retry = 0;
    mbus_frame_data reply_data;
    mbus_frame *frame, *next_frame;
    int frame_count = 0, result;

    if (handle == NULL)
    {
        MBUS_ERROR("%s: Invalid M-Bus handle for request.\n", __PRETTY_FUNCTION__);
        return 1;
    }

    if (mbus_is_primary_address(address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, address);
        return 1;
    }

    frame = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);

    if (frame == NULL)
    {
        MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control = MBUS_CONTROL_MASK_REQ_UD2 |
                     MBUS_CONTROL_MASK_DIR_M2S |
                     MBUS_CONTROL_MASK_FCV     |
                     MBUS_CONTROL_MASK_FCB;

    frame->address = address;

    //
    // continue to read until no more records are available (usually only one
    // reply frame, but can be more for so-called multi-telegram replies)
    //
    next_frame = reply;

    memset((void *)&reply_data, 0, sizeof(mbus_frame_data));

    while (more_frames)
    {
        if (retry > handle->max_data_retry)
        {
            // Give up
            retval = 1;
            break;
        }

        if (debug)
            printf("%s: debug: sending request frame\n", __PRETTY_FUNCTION__);

        if (mbus_send_frame(handle, frame) == -1)
        {
            MBUS_ERROR("%s: failed to send mbus frame.\n", __PRETTY_FUNCTION__);
            retval = -1;
            break;
        }

        if (debug)
            printf("%s: debug: receiving response frame #%d\n", __PRETTY_FUNCTION__, frame_count);

        result = mbus_recv_frame(handle, next_frame);

        if (result == MBUS_RECV_RESULT_OK)
        {
            retry = 0;
            mbus_purge_frames(handle);
        }
        else if (result == MBUS_RECV_RESULT_TIMEOUT)
        {
            MBUS_ERROR("%s: No M-Bus response frame received.\n", __PRETTY_FUNCTION__);
            retry++;
            continue;
        }
        else if (result == MBUS_RECV_RESULT_INVALID)
        {
            MBUS_ERROR("%s: Received invalid M-Bus response frame.\n", __PRETTY_FUNCTION__);
            retry++;
            mbus_purge_frames(handle);
            continue;
        }
        else
        {
            MBUS_ERROR("%s: Failed to receive M-Bus response frame.\n", __PRETTY_FUNCTION__);
            retval = 1;
            break;
        }

        frame_count++;

        //
        // We need to parse the data in the received frame to be able to tell
        // if more records are available or not.
        //
        if (mbus_frame_data_parse(next_frame, &reply_data) == -1)
        {
            MBUS_ERROR("%s: M-bus data parse error.\n", __PRETTY_FUNCTION__);
            retval = 1;
            break;
        }

        //
        // Continue a cycle of sending requests and reading replies until the
        // reply do not have DIF=0x1F in the last record (which signals that
        // more records are available.
        //

        if (reply_data.type != MBUS_DATA_TYPE_VARIABLE)
        {
            // only single frame replies for FIXED type frames
            more_frames = 0;
        }
        else
        {
            more_frames = 0;

            if (reply_data.data_var.more_records_follow &&
                ((max_frames > 0) && (frame_count < max_frames))) // only readout max_frames
            {
                if (debug)
                    printf("%s: debug: expecting more frames\n", __PRETTY_FUNCTION__);

                more_frames = 1;

                // allocate new frame and increment next_frame pointer
                next_frame->next = mbus_frame_new(MBUS_FRAME_TYPE_ANY);

                if (next_frame->next == NULL)
                {
                    MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
                    retval = -1;
                    more_frames = 0;
                }

                next_frame = next_frame->next;

                // toogle FCB bit
                frame->control ^= MBUS_CONTROL_MASK_FCB;
            }
            else
            {
                if (debug)
                    printf("%s: debug: no more frames\n", __PRETTY_FUNCTION__);
            }
        }

        if (reply_data.data_var.record)
        {
            // free's up the whole list
            mbus_data_record_free(reply_data.data_var.record);
        }
    }

    mbus_frame_free(frame);
    return retval;
}


//------------------------------------------------------------------------------
// send a data request packet to from master to slave and optional purge response
//------------------------------------------------------------------------------
int
mbus_send_ping_frame(mbus_handle *handle, int address, char purge_response)
{
    int retval = 0;
    mbus_frame *frame;

    if (mbus_is_primary_address(address) == 0)
    {
        MBUS_ERROR("%s: invalid address %d\n", __PRETTY_FUNCTION__, address);
        return 1;
    }

    frame = mbus_frame_new(MBUS_FRAME_TYPE_SHORT);

    if (frame == NULL)
    {
        MBUS_ERROR("%s: failed to allocate mbus frame.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    frame->control  = MBUS_CONTROL_MASK_SND_NKE | MBUS_CONTROL_MASK_DIR_M2S;
    frame->address  = address;

    if (mbus_send_frame(handle, frame) == -1)
    {
        MBUS_ERROR("%s: failed to send mbus frame.\n", __PRETTY_FUNCTION__);
        mbus_frame_free(frame);
        return -1;
    }

    if (purge_response)
    {
        mbus_purge_frames(handle);
    }

    mbus_frame_free(frame);
    return retval;
}

//------------------------------------------------------------------------------
// Select a device using the supplied secondary address  (mask).
//------------------------------------------------------------------------------
int
mbus_select_secondary_address(mbus_handle * handle, const char *mask)
{
    int ret;
    mbus_frame reply;

    if (mask == NULL || strlen(mask) != 16)
    {
        MBUS_ERROR("%s: Invalid address masks.\n", __PRETTY_FUNCTION__);
        return MBUS_PROBE_ERROR;
    }

    /* send select command */
    if (mbus_send_select_frame(handle, mask) == -1)
    {
        MBUS_ERROR("%s: Failed to send selection frame: %s.\n",
                   __PRETTY_FUNCTION__,
                   mbus_error_str());
        return MBUS_PROBE_ERROR;
    }

    memset((void *)&reply, 0, sizeof(mbus_frame));
    ret = mbus_recv_frame(handle, &reply);

    if (ret == MBUS_RECV_RESULT_TIMEOUT)
    {
        return MBUS_PROBE_NOTHING;
    }

    if (ret == MBUS_RECV_RESULT_INVALID)
    {
        /* check for more data (collision) */
        mbus_purge_frames(handle);
        return MBUS_PROBE_COLLISION;
    }

    if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
    {
        /* check for more data (collision) */
        if (mbus_purge_frames(handle))
        {
            return MBUS_PROBE_COLLISION;
        }

        return MBUS_PROBE_SINGLE;
    }

    MBUS_ERROR("%s: Unexpected reply for address [%s].\n", __PRETTY_FUNCTION__, mask);

    return MBUS_PROBE_NOTHING;
}

//------------------------------------------------------------------------------
// Probe for the presence of a device(s) using the supplied secondary address
// (mask).
//------------------------------------------------------------------------------
int
mbus_probe_secondary_address(mbus_handle *handle, const char *mask, char *matching_addr)
{
    int ret, i;
    mbus_frame reply;

    if (mask == NULL || matching_addr == NULL || strlen(mask) != 16)
    {
        MBUS_ERROR("%s: Invalid address masks.\n", __PRETTY_FUNCTION__);
        return MBUS_PROBE_ERROR;
    }

    for (i = 0; i <= handle->max_search_retry; i++)
    {
        ret = mbus_select_secondary_address(handle, mask);

        if (ret == MBUS_PROBE_SINGLE)
        {
            /* send a data request command to find out the full address */
            if (mbus_send_request_frame(handle, MBUS_ADDRESS_NETWORK_LAYER) == -1)
            {
                MBUS_ERROR("%s: Failed to send request to selected secondary device [mask %s]: %s.\n",
                           __PRETTY_FUNCTION__,
                           mask,
                           mbus_error_str());
                return MBUS_PROBE_ERROR;
            }

            memset((void *)&reply, 0, sizeof(mbus_frame));
            ret = mbus_recv_frame(handle, &reply);

            if (ret == MBUS_RECV_RESULT_TIMEOUT)
            {
                return MBUS_PROBE_NOTHING;
            }

            if (ret == MBUS_RECV_RESULT_INVALID)
            {
                /* check for more data (collision) */
                mbus_purge_frames(handle);
                return MBUS_PROBE_COLLISION;
            }

            /* check for more data (collision) */
            if (mbus_purge_frames(handle))
            {
                return MBUS_PROBE_COLLISION;
            }

            if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_LONG)
            {
                char *addr = mbus_frame_get_secondary_address(&reply);

                if (addr == NULL)
                {
                    // show error message, but procede with scan
                    MBUS_ERROR("Failed to generate secondary address from M-Bus reply frame: %s\n",
                               mbus_error_str());
                    return MBUS_PROBE_NOTHING;
                }

                snprintf(matching_addr, 17, "%s", addr);

                if (handle->found_event)
                {
                    handle->found_event(handle,&reply);
                }

                return MBUS_PROBE_SINGLE;
            }
            else
            {
                MBUS_ERROR("%s: Unexpected reply for address [mask %s]. Expected long frame.\n",
                           __PRETTY_FUNCTION__, mask);
                return MBUS_PROBE_NOTHING;
            }
        }
        else if ((ret == MBUS_PROBE_ERROR) ||
                 (ret == MBUS_PROBE_COLLISION))
        {
            break;
        }
    }

    return ret;
}


int mbus_read_slave(mbus_handle * handle, mbus_address *address, mbus_frame * reply)
{
    if (handle == NULL || address == NULL)
    {
        MBUS_ERROR("%s: Invalid handle or address.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if (address->is_primary)
    {
        if (mbus_send_request_frame(handle, address->primary) == -1)
        {
            MBUS_ERROR("%s: Failed to send M-Bus request frame.\n",
                       __PRETTY_FUNCTION__);
            return -1;
        }
    }
    else
    {
        /* secondary addressing */
        int probe_ret;

        if (address->secondary == NULL)
        {
            MBUS_ERROR("%s: Secondary address not set.\n",
                       __PRETTY_FUNCTION__);
            return -1;
        }

        probe_ret = mbus_select_secondary_address(handle, address->secondary);

        if (probe_ret == MBUS_PROBE_COLLISION)
        {
            MBUS_ERROR("%s: The address mask [%s] matches more than one device.\n",
                       __PRETTY_FUNCTION__,
                       address->secondary);
            return -1;
        }
        else if (probe_ret == MBUS_PROBE_NOTHING)
        {
            MBUS_ERROR("%s: The selected secondary address [%s] does not match any device.\n",
                       __PRETTY_FUNCTION__,
                       address->secondary);
            return -1;
        }
        else if (probe_ret == MBUS_PROBE_ERROR)
        {
            MBUS_ERROR("%s: Failed to probe secondary address [%s].\n",
                       __PRETTY_FUNCTION__,
                       address->secondary);
            return -1;
        }
        /* else MBUS_PROBE_SINGLE */

        if (mbus_send_request_frame(handle, MBUS_ADDRESS_NETWORK_LAYER) == -1)
        {
            MBUS_ERROR("%s: Failed to send M-Bus request frame.\n",
                       __PRETTY_FUNCTION__);
            return -1;
        }
    }

    if (mbus_recv_frame(handle, reply) != 0)
    {
        MBUS_ERROR("%s: Failed to receive M-Bus response frame.\n",
                   __PRETTY_FUNCTION__);
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
// Iterate over all address masks according to the M-Bus probe algorithm.
//------------------------------------------------------------------------------
int
mbus_scan_2nd_address_range(mbus_handle * handle, int pos, char *addr_mask)
{
    int i, i_start, i_end, probe_ret;
    char *mask, matching_mask[17];

    if (handle == NULL || addr_mask == NULL)
    {
        MBUS_ERROR("%s: Invalid handle or address mask.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if (strlen(addr_mask) != 16)
    {
        MBUS_ERROR("%s: Illegal address mask [%s]. Not 16 characters long.\n", __PRETTY_FUNCTION__, addr_mask);
        return -1;
    }

    if (pos < 0 || pos >= 16)
    {
        return 0;
    }

    if ((mask = strdup(addr_mask)) == NULL)
    {
        MBUS_ERROR("%s: Failed to allocate local copy of the address mask.\n", __PRETTY_FUNCTION__);
        return -1;
    }

    if (mask[pos] == 'f' || mask[pos] == 'F')
    {
        // mask[pos] is a wildcard -> enumerate all 0..9 at this position
        i_start = 0;
        i_end   = 9;
    }
    else
    {
        if (pos < 15)
        {
            // mask[pos] is not a wildcard -> don't iterate, recursively check pos+1
            mbus_scan_2nd_address_range(handle, pos+1, mask);
        }
        else
        {
            // .. except if we're at the last pos (==15) and this isn't a wildcard we still need to send the probe
            i_start = (int)(mask[pos] - '0');
            i_end   = (int)(mask[pos] - '0');
        }
    }

    // skip the scanning if we're returning from the (pos < 15) case above
    if (mask[pos] == 'f' || mask[pos] == 'F' || pos == 15)
    {
        for (i = i_start; i <= i_end; i++)
        {
            mask[pos] = '0'+i;

            if (handle->scan_progress)
                handle->scan_progress(handle,mask);

            probe_ret = mbus_probe_secondary_address(handle, mask, matching_mask);

            if (probe_ret == MBUS_PROBE_SINGLE)
            {
                if (!handle->found_event)
                {
                    printf("Found a device on secondary address %s [using address mask %s]\n", matching_mask, mask);
                }
            }
            else if (probe_ret == MBUS_PROBE_COLLISION)
            {
                // collision, more than one device matching, restrict the search mask further
                mbus_scan_2nd_address_range(handle, pos+1, mask);
            }
            else if (probe_ret == MBUS_PROBE_NOTHING)
            {
                 // nothing... move on to next address mask
            }
            else // MBUS_PROBE_ERROR
            {
                MBUS_ERROR("%s: Failed to probe secondary address [%s].\n", __PRETTY_FUNCTION__, mask);
                return -1;
            }
        }
    }

    free(mask);
    return 0;
}

//------------------------------------------------------------------------------
// Convert a buffer with hex values into a buffer with binary values.
// - invalid character stops convertion
// - whitespaces will be ignored
//------------------------------------------------------------------------------
size_t
mbus_hex2bin(unsigned char * dst, size_t dst_len, const unsigned char * src, size_t src_len)
{
    size_t i, result = 0;
    unsigned long val;
    unsigned char *ptr, *end, buf[3];

    if (!src || !dst)
    {
        return 0;
    }

    memset(buf, 0, sizeof(buf));
    memset(dst, 0, dst_len);

    for (i = 0; i+1 < src_len; i++)
    {
        // ignore whitespace
        if (isspace(src[i]))
            continue;

        buf[0] = src[i];
        buf[1] = src[++i];

        end = buf;
        ptr = end;
        val = strtoul(ptr, (char **)&end, 16);

        // abort at non hex value
        if (ptr == end)
            break;

        // abort at end of buffer
        if (result >= dst_len)
            break;

        dst[result++] = (unsigned char) val;
    }

    return result;
}
