#pragma once

#include <memory>

#include "host_io_base.h"
#include "host_io_posix.h"
#include "host_io_win32.h"

using HostIOBase = HostIODriver::HostIO_Base;

// native implementation
#ifdef __linux__
using HostIO = HostIODriver::HostIO_POSIX;
#elif _WIN32
using HostIO = HostIODriver::HostIO_Win32;
#else
#warning This architecture isn't supported by HostIO
#endif
