/**
 * page_stats_impl.cpp - 
 * @author: Jonathan Beard
 * @version: Mon Mar 21 13:33:21 2022
 * 
 * Copyright 2022 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "page_stats_impl.hpp"
#include "trace_entry.h"
#include <iostream>

void    
page_stats_impl::init( )
{
    //allocate tracking structures
    _4K_stats       = new gran_stats_t< 64    ,12 >();
    _64K_stats      = new gran_stats_t< 1024  ,16 >();
    _1M_stats       = new gran_stats_t< 16384 ,20 >();
}

void
page_stats_impl::update( const memref_t &memref ) 
{
    const auto address = memref.data.addr;
    const auto type    = memref.data.type;
    //get rid of lower bits, index only on bits > log2(page size)
    const std::uint64_t index12    = (address >> 12);
    const std::uint64_t index16    = (address >> 16);
    const std::uint64_t index20    = (address >> 20);
    /** 
     * for now, no access bigger than cache line size, 
     * so ignore for now. 
     */
    (*_4K_stats)    [ index12 ].update( address, type );
    (*_64K_stats)   [ index16 ].update( address, type );
    (*_1M_stats)    [ index20 ].update( address, type );
}

void
page_stats_impl::write( std::ofstream &ostream_4, std::ofstream &ostream_64, std::ofstream &ostream_1M )
{
            
    if( ostream_4.is_open() )
    {
        auto it = _4K_stats->begin();
        auto end = _4K_stats->end();
        for( ; it != end; ++it )
        {
            const auto actual_addy = 
                (std::uintptr_t)( (*it).first ) << 12;
            ostream_4 << "0x" << std::hex << actual_addy << std::dec << ", " << (*it).second << "\n";
        }
        ostream_4.flush();
        ostream_4.close();
    }
    else
    {
        std::cout << "error, failed to open file\n";
    }

    if( ostream_64.is_open() )
    {
        auto it = _64K_stats->begin();
        auto end = _64K_stats->end();
        for( ; it != end; ++it)
        {
            const auto actual_addy = 
                (std::uintptr_t)( (*it).first ) << 16;
            ostream_64 << "0x" << std::hex << actual_addy << std::dec << ", " << (*it).second << "\n";
        }
        ostream_64.flush();
        ostream_64.close();
    }
    else
    {
        std::cout << "error, failed to open file\n";
    }

    if( ostream_1M.is_open() )
    {
        auto it = _1M_stats->begin();
        auto end = _1M_stats->end();
        for( ; it != end; ++it )
        {
            const auto actual_addy = 
                (std::uintptr_t)( (*it).first ) << 20;
            ostream_1M << "0x" << std::hex << actual_addy << std::dec << ", " << (*it).second << "\n";
        }
        ostream_1M.flush();
        ostream_1M.close();
    }
    else
    {
        std::cout << "error, failed to open file\n";
    }

}

void 
page_stats_impl::destroy()
{
    delete( _4K_stats  );
    delete( _64K_stats );
    delete( _1M_stats  );
}
