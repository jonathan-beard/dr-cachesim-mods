/* **********************************************************
 * Copyright (c) 2015-2020 Google, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Google, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* cache_line: represents a cache line.
 */

#ifndef _CACHE_LINE_H_
#define _CACHE_LINE_H_ 1

#include "caching_device_block.h"
#include <iostream>
#include <iomanip>

#include <boost/dynamic_bitset.hpp>
#include <cstddef>
#include <cassert>
#include <algorithm>

template < bool utilization > struct cache_line_t : public caching_device_block_t
{
    cache_line_t( const size_t line_size ) : used( line_size ),
                                             count( line_size )
    {
    }

    virtual ~cache_line_t() = default;
    
    virtual void update_utilization( const memref_t &in, 
                                     const size_t offset, 
                                     const bool record ) override
    {
        caching_device_block_t::update_utilization( in, offset, record );
        /** calc where in line we hit, update counters **/
        const auto start( offset );
        const auto end( std::min( offset + in.data.size, count  ) );
        for( auto i( start ); i <= end; i++ )
        {
            used[ i ] = 1;
        }
        return;
    }

    virtual void reset() override
    {
        caching_device_block_t::reset();
        used.reset();
        return;
    }

    virtual boost::dynamic_bitset<> getBits() override 
    {
        return( used );
    }

    boost::dynamic_bitset<>         used;
    using bitset_type   = decltype( used.size() );
    const bitset_type               count;
};

template <> struct cache_line_t< false > : public caching_device_block_t
{
    /** no need for bitset here **/
};

#endif /* _CACHE_LINE_H_ */
