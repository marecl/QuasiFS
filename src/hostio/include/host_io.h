#pragma once

#include <memory>

#include "host_io_base.h"
#include "host_io_virtual.h"
#include "host_io_posix.h"
#include "host_io_win32.h"

using HostIOBase = HostIODriver::HostIO_Base;

using HostVIO = HostIODriver::HostIO_Virtual;

// native implementation
#ifdef __linux__
using HostIO = HostIODriver::HostIO_POSIX;
#elif _WIN32
#warning Contributors needed
using HostIO = HostIODriver::HostIO_Win32;
#else
#warning This architecture isn't supported by HostIO
#endif

template <class T>
class Singleton
{
public:
    static T *Instance()
    {
        if (!m_instance)
        {
            m_instance = std::make_unique<T>();
        }
        return m_instance.get();
    }

protected:
    Singleton();
    ~Singleton();

private:
    static inline std::unique_ptr<T> m_instance{};
};