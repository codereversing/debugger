#pragma once

#include <Windows.h>

namespace CodeReversing
{
    //An extremely incomplete RAII wrapper around the HANDLE object
    class SafeHandle
    {
    public:
        SafeHandle() : m_handle{ INVALID_HANDLE_VALUE }
        {
        }

        SafeHandle(HANDLE handle) : m_handle{ handle }
        {
        }

        SafeHandle(const SafeHandle &copy) = delete;
        SafeHandle &operator=(const SafeHandle &copy) = delete;

        SafeHandle(SafeHandle &&obj)
        {
            m_handle = obj.m_handle;
            obj.m_handle = INVALID_HANDLE_VALUE;
        }

        SafeHandle &operator=(SafeHandle &&obj)
        {
            m_handle = obj.m_handle;
            obj.m_handle = INVALID_HANDLE_VALUE;
            return *this;
        }

        SafeHandle &operator=(const HANDLE handle)
        {
            m_handle = handle;
            return *this;
        }

        ~SafeHandle()
        {
            if (IsValid())
            {
                (void)CloseHandle(m_handle);
            }
        }

        const bool IsValid() const
        {
            return (m_handle != INVALID_HANDLE_VALUE) || (m_handle != nullptr);
        }

        HANDLE operator()() const
        {
            return m_handle;
        }

    private:
        HANDLE m_handle;
    };
}