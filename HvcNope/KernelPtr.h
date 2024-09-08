#pragma once

#include "Globals.h"

template <typename T>
class KernelPtr {
public:

	KernelPtr( T* Pointer = nullptr ) : m_Pointer( reinterpret_cast<kAddress>(Pointer) ) {}

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
};