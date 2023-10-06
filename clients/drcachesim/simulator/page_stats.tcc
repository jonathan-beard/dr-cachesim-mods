/**
 * page_stats.tcc - 
 * @author: Jonathan Beard
 * @version: Mon Mar 21 12:38:54 2022
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
#ifndef PAGE_STATS_TCC
#define PAGE_STATS_TCC  1
#include <cstdint>
#include <bitset>
#include <string>
#include <ostream>
#include <limits>
#include <map>
#include <functional>

#include "trace_entry.h"

template< int GRANULE_OFFSETS, std::uint8_t BIT /** page size pow 2 **/ > struct page_stats
{
    void update( const addr_t address, const trace_type_t type )
    {
        counter++;
        constexpr std::uint64_t mask = ~(std::numeric_limits< std::uint64_t >::max() << BIT);
        const std::uint64_t index    = (address & mask) >> 6 /** coherence granule size **/;
        if( type == TRACE_TYPE_READ || 
            type == TRACE_TYPE_PREFETCH || 
            type == TRACE_TYPE_HARDWARE_PREFETCH ||
            type == TRACE_TYPE_INSTR )
        {
            read_access[ index ] = 1;
            if( type == TRACE_TYPE_INSTR && insn_page == 2 )
            {
                insn_page = 3;
            }
        }
        else if( type == TRACE_TYPE_WRITE )
        {
            write_access[ index ] = 1;
        }
        //NOTE, WE'RE IGNORING SEV TYPES
        if( insn_page == 0 )
        {
            if( type == TRACE_TYPE_INSTR )
            {
                insn_page = 1;
            }
            else
            {
                insn_page = 2 /** not insn **/;
            }
        }
    }
    

    std::uint64_t               counter = 0;
    std::uint8_t                insn_page = 0;
    std::bitset< GRANULE_OFFSETS > read_access;
    std::bitset< GRANULE_OFFSETS > write_access;

};

inline std::string return_type_helper( const std::uint8_t insn_page )
{
    std::string pg_type = "";
    switch( insn_page )
    {
        case( 0 ):
        {
            pg_type = "unknown";
        }
        break;
        case( 1 ):
        {
            pg_type = "instructions";
        }
        break;
        case( 2 ):
        {
            pg_type = "data";
        }
        break;
        case( 3 ):
        {
            pg_type = "mixed";
        }
        break;
        default:
            pg_type = "unknown";
    }
    return( pg_type );
}
    
inline std::ostream& operator << ( std::ostream &stream, page_stats< 64, 12 > &ps)
{
    const auto pg_type( return_type_helper( ps.insn_page ) );
    stream << ps.counter << ", " << pg_type << ", " << 
        ps.read_access.to_string() << ", " << ps.write_access.to_string();
    return( stream );
}

inline std::ostream& operator << ( std::ostream &stream, page_stats< 1024, 16 > &ps)
{
    const auto pg_type( return_type_helper( ps.insn_page ) );
    stream << ps.counter << ", " << pg_type << ", " << 
        ps.read_access.to_string() << ", " << ps.write_access.to_string();
    return( stream );
}

inline std::ostream& operator << ( std::ostream &stream, page_stats< 16384, 20 > &ps)
{
    const auto pg_type( return_type_helper( ps.insn_page ) );
    stream << ps.counter << ", " << pg_type << ", " << 
        ps.read_access.to_string() << ", " << ps.write_access.to_string();
    return( stream );
}


template < int GRANULES,    
           int PAGE_SIZE_POW_2 > 
using gran_stats_t = 
                std::map< std::uint64_t, 
                          page_stats< GRANULES, 
                                      PAGE_SIZE_POW_2 >, 
                                      std::greater< std::uint64_t > >;


#endif /* END PAGE_STATS_TCC */
