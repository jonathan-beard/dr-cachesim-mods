/**
 * vm_smap_entry.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Mar 31 06:16:00 2022
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
#include <fstream>
#include <string>

#include "vm_smap_entry.hpp"
#include "vm_defs.hpp"

vm_smap_entry_t::vm_smap_entry_t() : vm_maps_entry_t()
{
    //do we need anything here
}



void
vm_smap_entry_t::add_fields_to_entry( std::ifstream &stream, const std::string &str )
{
    //use the string input here, it's the first element, ignore the rest
    vm_maps_entry_t::add_fields_to_entry( stream, str );
    //continue consuming fields here
    std::string line;
    int         line_counter = 0;
    while( std::getline( stream, line ) && line_counter < vm_smap_entry_t::n_fields )
    {
        auto *str = line.c_str();
        std::sscanf( str, 
                     "%*[^:]: %" PRINT_TYPE " kB",
                     &field_data[ line_counter ] );
        line_counter++;
    }
    /**
     * NOTE: we're ignoring vm flags for now given lack of time,
     * but there's a structure there to do so if we need, just 
     * use above code and put them in vm_flags vector. 
     */
    return;
}


std::ostream& 
operator << ( std::ostream &stream, const vm_smap_entry_t &entry)
{
    stream << *reinterpret_cast< const vm_maps_entry_t* >( &entry );
    //finish printing here
    stream << "\nfields: {";
    for( auto idx = 0; idx < vm_smap_entry_t::n_fields; idx++ ) 
    {
        stream << vm_smap_entry_t::field_name[ idx ];
        stream << ": ";
        stream << entry.field_data[ idx ];
        if( idx != vm_smap_entry_t::n_fields - 1 )
        {
            stream << ", ";
        }
    }
    stream << "}";
    return( stream );
}

