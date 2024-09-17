#pragma once

#include "Globals.h"
#include "PoolFlags.h"

template <typename T>
class KernelPtr {
private:

    template<typename T>
    class KernelPtrProxy {
        kAddress m_Address;
        T m_CachedObject;

    public:
        KernelPtrProxy( kAddress buf ) : m_Address( buf ) {
            auto dataVec = g_Rw->ReadBuffer( m_Address, sizeof( T ) );
            m_CachedObject = *(T*)dataVec.data();
        }

        T& operator*() {
            return m_CachedObject;
        }

        T* operator->() {
            return &m_CachedObject;
        }

        template<typename U>
        KernelPtrProxy& operator=( const U& value ) {
            m_CachedObject = value;
            g_Rw->WriteBuffer( m_Address, std::span<Byte>( (Byte*)&m_CachedObject, sizeof( T ) ) );
            return *this;
        }

        template<typename U>
        KernelPtrProxy& operator=( const KernelPtrProxy<U>& other ) {
            m_CachedObject = *other; // Copy assignment from another proxy
            g_Rw->WriteBuffer( m_Address, std::span<Byte>( (Byte*)&m_CachedObject, sizeof( T ) ) );

            return *this;
        }

        ~KernelPtrProxy() {
            g_Rw->WriteBuffer( m_Address, std::span<Byte>( (Byte*)&m_CachedObject, sizeof( T ) ) );

        }
    };

public:

    KernelPtr( T* Pointer = nullptr, bool ShouldFree = false ) : 
        m_Pointer( reinterpret_cast<kAddress>(Pointer) ),
        m_ShouldFree(ShouldFree) 
    {
        auto dataVec = g_Rw->ReadBuffer( m_Pointer, sizeof( T ) );
        m_Cached = *(T*)dataVec.data();
    }

    //
    // Templated constructor, allow constructing a KernelPtr<T>
    // if the address type is convertible to kAddress (we do a simple sanity check - 
    // verify that the sizes are the same).
    //
    template <
        typename AddrType, 
        typename std::enable_if<sizeof(AddrType) == sizeof(kAddress), bool>::type = true
    >
    KernelPtr(AddrType Address, bool ShouldFree = false) :
        m_Pointer(kAddress(Address)),
        m_ShouldFree(ShouldFree) 
    {
        auto dataVec = g_Rw->ReadBuffer( m_Pointer, sizeof( T ) );
        m_Cached = *(T*)dataVec.data();
    }

    //T operator*() const {
    //    T value;
    //    readFromAddress( reinterpret_cast<Byte*>(&value), sizeof( T ) );
    //    return value;
    //}

    KernelPtr& operator=( const T& value ) {
        writeToAddress( reinterpret_cast<const Byte*>(&value), sizeof( T ) );
        return *this;
    }

    //template <
    //    typename AddrType,
    //    typename std::enable_if<sizeof( AddrType ) == sizeof( kAddress ), bool>::type = true
    //>
    //KernelPtr& operator=( AddrType Address ) {
    //    m_Pointer = kAddress( Address );
    //    m_ShouldFree = false;
    //    return *this;
    //}


    //template <typename FieldType>
    //KernelPtr& operator=( const FieldType& Value ) {
    //    // Handle fields correctly
    //    m_Cached = Value;
    //    g_Rw->WriteBuffer( m_Pointer, std::span<Byte>( (Byte*)m_Cached, sizeof( T ) ) );
    //    return *this;
    //}


    //T* operator->() {
    //    return &m_Cached;
    //}

    KernelPtrProxy<T> operator->() {
        return KernelPtrProxy<T>( m_Pointer );
    }

    //T& operator[]( std::size_t index ) {
    //    return *KernelPtr<T>( reinterpret_cast<T*>(m_Pointer + index * sizeof( T )) );
    //}

    T* get() { return (T*) m_Pointer;  }

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
    T m_Cached;
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
