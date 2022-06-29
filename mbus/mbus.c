//------------------------------------------------------------------------------
// Copyright (C) 2010, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include "mbus-protocol.h"

#ifdef _WIN32
#define VERSION "0.9.0"
#else
#include "../config.h"
#endif

#ifndef VERSION
#define VERSION "0.9.0"
#endif

//
//
//
int mbus_init() {return 0;}

///
/// Return current version of the library
///
const char*
mbus_get_current_version() {return VERSION;}
