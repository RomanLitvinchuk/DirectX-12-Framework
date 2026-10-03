#pragma once
#include <stdexcept>
#include <string>
#include <sstream>
#include <comdef.h>

inline void ThrowIfFailed(HRESULT hr, const char* file, int line, const char* function)
{
    if (FAILED(hr))
    {
        std::stringstream ss;
        ss << "Error at " << file << " (" << line << ") in " << function << ": ";

        _com_error err(hr);
        ss << "HRESULT: 0x" << std::hex << hr << std::dec << " - " << err.ErrorMessage();

        throw std::runtime_error(ss.str().c_str());
    }
}
#define ThrowIfFailed(x) ThrowIfFailed((x), __FILE__, __LINE__, __FUNCTION__)
