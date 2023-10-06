/**
 * vm_map_entry.cpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug 20 12:34:47 2018
 * 
 * Copyright 2018 Jonathan Beard
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
#include "vm_map_entry.hpp"

#include <cstdio>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include "vm_defs.hpp"


void
vm_maps_entry_t::add_fields_to_entry( std::ifstream &stream, const std::string &s )
{
    UNUSED( stream );
    auto *str = s.c_str();
    char perms_buffer[ 8 ];
    *reinterpret_cast< std::uint64_t* >( perms_buffer ) = 0;
    sscanf( str, "%" PRIx64 "-%" PRIx64 " %[^ ] %" PRIx64 " %" PRIx32 ":%" PRIx32 " %" PRIu64 " %[^\n]", 
                                                  &start, 
                                                  &end,
                                                  perms_buffer,
                                                  &offset,
                                                  &maj_dev,
                                                  &min_dev,
                                                  &inode,
                                                  pathname );
                                                 
                                                 
    if( perms_buffer[ 0 ] == 'r' )
    {
        perms |= read;
    }
    if( perms_buffer[ 1 ] == 'w' )
    {
        perms |= write;
    }
    if( perms_buffer[ 2 ] == 'x' )
    {
        perms |= execute;
    }
    if( perms_buffer[ 3 ] == 's' )
    {
        perms |= shared;
    }
    addy_range = (end - start) + 1;
    return;                                             
}


std::ostream& 
operator << ( std::ostream &stream, const vm_maps_entry_t &entry)
{
    stream << std::hex << entry.start << " -> " << entry.end << std::dec <<  " - o: " 
        << entry.offset << " - inode: " << entry.inode << " path (" << entry.pathname << 
            ") permissions: " << std::hex << entry.perms << std::dec;
    return( stream );
}

