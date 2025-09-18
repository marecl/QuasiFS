#pragma once

#include <memory>

#include "host_io_posix.h"
#include "host_io_win32.h"

// native implementation
#ifdef __linux__
using HostIO = HostIO_POSIX;
#elif _WIN32
using HostIO = HostIO_Win32;
#else
#warning This architecture isn't supported by HostIO
#endif