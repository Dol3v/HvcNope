#pragma once

#include "Globals.h"
#include "PoolFlags.h"

template <typename T>
class KernelPtr {
public:

    KernelPtr( T* Pointer = nullptr, bool ShouldFree = false ) : 
        m_Pointer( reinterpret_cast<kAddress>(Pointer) ),
        m_ShouldFree(ShouldFree) {}

    T operator*() const {
        T value;
        readFromAddress( reinterpret_cast<Byte*>(&value), sizeof( T ) );
        return value;
    }

    KernelPtr& operator=( const T& value ) {
        writeToAddress( reinterpret_cast<const Byte*>(&value), sizeof( T ) );
        return *this;
    }

    T* operator->() const {
        return reinterpret_cast<T*>(m_Pointer);
    }

    T& operator[]( std::size_t index ) {
        return *KernelPtr<T>( reinterpret_cast<T*>(m_Pointer + index * sizeof( T )) );
    }

    ~KernelPtr() {
        if (!m_ShouldFree) return;

        // free pool memory
        g_Invoker->CallKernelFunction(
            "ExFreePool",
            m_Pointer
        );
    }

private:
    void readFromAddress( Byte* buffer, std::size_t size ) const {
        for (std::size_t i = 0; i < size; ++i) {
            buffer[i] = g_Rw->ReadByte( m_Pointer + i );
        }
    }

    void writeToAddress( const Byte* buffer, std::size_t size ) {
        for (std::size_t i = 0; i < size; ++i) {
            g_Rw->WriteByte( m_Pointer + i, buffer[i] );
        }
    }

private:
	kAddress m_Pointer;
    bool m_ShouldFree;
};

template <typename T, typename... Args>
KernelPtr<T> MakeKernel( Args&&... Arguments, ULONG64 PoolFlags = POOL_FLAG_PAGED, ULONG Tag = 'HvcN')
{
    // allocate local copy
    auto* userModeCopy = new T( std::forward<Args>( Arguments )... );

    // allocate necessary memory in kernel
    kAddress mem = g_Invoker->CallKernelFunction(
        "ExAllocatePool2",
        PoolFlags,
        sizeof( T ),
        Tag
    );

    g_Rw->WriteBuffer( mem, std::span<Byte>( userModeCopy, sizeof( T ) ) );

    delete userModeCopy;

    return KernelPtr<T>( mem, true );
}
