#ifndef __TRACYSLAB_HPP__
#define __TRACYSLAB_HPP__

#include <assert.h>
#include <cstddef>
#include <stdint.h>
#include <vector>

#include "TracyMemory.hpp"
#include "../public/common/TracyForceInline.hpp"

namespace tracy
{

template<size_t BlockSize>
class Slab
{
public:
    Slab()
        : m_ptr( new char[BlockSize] )
        , m_offset( 0 )
        , m_buffer( { m_ptr } )
        , m_usage( BlockSize )
    {
        memUsage += BlockSize;
    }

    ~Slab()
    {
        memUsage -= m_usage;
        for( auto& v : m_buffer )
        {
            delete[] v;
        }
    }

    static tracy_force_inline size_t AlignUp( size_t v, size_t a )
    {
        return ( v + a - 1 ) & ~( a - 1 );
    }

    tracy_force_inline void* AllocRaw( size_t size )
    {
        assert( size <= BlockSize );
        // Align allocations to alignof(std::max_align_t) so that any type
        // placed here has correctly aligned members. Without this, a prior
        // variable-length allocation (e.g. a char[N] for a string with odd N)
        // leaves m_offset misaligned, and the next struct allocation ends up
        // at an odd address — causing crashes in nested fields that require
        // 8-byte alignment (std::vector, unordered_flat_map, int64_t, etc.).
        m_offset = AlignUp( m_offset, alignof( std::max_align_t ) );
        const auto offset = m_offset;
        if( offset + size > BlockSize )
        {
            return DoAlloc( size );
        }
        else
        {
            void* ret = m_ptr + offset;
            m_offset += size;
            return ret;
        }
    }

    template<typename T>
    tracy_force_inline T* AllocInit()
    {
        const auto size = sizeof( T );
        auto ret = AllocRaw( size );
        new( ret ) T;
        return (T*)ret;
    }

    template<typename T>
    tracy_force_inline T* AllocInit( size_t sz )
    {
        const auto size = sizeof( T ) * sz;
        auto ret = AllocRaw( size );
        T* ptr = (T*)ret;
        for( size_t i=0; i<sz; i++ )
        {
            new( ptr ) T;
            ptr++;
        }
        return (T*)ret;
    }

    template<typename T>
    tracy_force_inline T* Alloc()
    {
        return (T*)AllocRaw( sizeof( T ) );
    }

    template<typename T>
    tracy_force_inline T* Alloc( size_t size )
    {
        return (T*)AllocRaw( sizeof( T ) * size );
    }

    tracy_force_inline void Unalloc( size_t size )
    {
        assert( size <= m_offset );
        m_offset -= size;
    }

    tracy_force_inline void* AllocBig( size_t size )
    {
        m_offset = AlignUp( m_offset, alignof( std::max_align_t ) );
        const auto offset = m_offset;
        if( offset + size <= BlockSize )
        {
            void* ret = m_ptr + offset;
            m_offset += size;
            return ret;
        }
        else if( size <= BlockSize && BlockSize - offset <= 1024 )
        {
            return DoAlloc( size );
        }
        else
        {
            memUsage += size;
            m_usage += size;
            auto ret = new char[size];
            m_buffer.emplace_back( ret );
            return ret;
        }
    }

    void Reset()
    {
        if( m_buffer.size() > 1 )
        {
            memUsage -= m_usage - BlockSize;
            m_usage = BlockSize;
            for( int i=1; i<m_buffer.size(); i++ )
            {
                delete[] m_buffer[i];
            }
            m_ptr = m_buffer[0];
            m_buffer.clear();
            m_buffer.emplace_back( m_ptr );
        }
        m_offset = 0;
    }

    Slab( const Slab& ) = delete;
    Slab( Slab&& ) = delete;

    Slab& operator=( const Slab& ) = delete;
    Slab& operator=( Slab&& ) = delete;

private:
    void* DoAlloc( uint32_t willUseBytes )
    {
        auto ptr = new char[BlockSize];
        m_ptr = ptr;
        m_offset = willUseBytes;
        m_buffer.emplace_back( m_ptr );
        memUsage += BlockSize;
        m_usage += BlockSize;
        return ptr;
    }

    char* m_ptr;
    uint32_t m_offset;
    std::vector<char*> m_buffer;
    size_t m_usage;
};

}

#endif
