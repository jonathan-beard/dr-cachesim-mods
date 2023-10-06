/**
 * memorymap.cpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug 20 12:32:26 2018
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
#include "memorymap.hpp"
#include "proc_tools.hpp"
#include "vm_smap_entry.hpp"
#include <sys/types.h>
#include <cstdio>
#include <string>
#include <fstream>

    
/**
 * add_os_vm_text_offset_to_pc
 */
std::uintptr_t 
vm_memory_map::add_os_vm_offset_to_pc( const std::uintptr_t pc )
{
#if ! NDEBUG
    if( segments.code != nullptr )
    {
#endif    
        return( segments.code->start + pc );
#if ! NDEBUG
    }
    return( pc );
#endif    
}

bool    
vm_memory_map::is_address_in_code( const std::uintptr_t address )
{
#if ! NDEBUG
    if( segments.code != nullptr )
    {
#endif    
        return( ( address - segments.code->start ) < segments.code->addy_range );
#if ! NDEBUG        
    }
    return( false );
#endif
}

bool    
vm_memory_map::is_address_in_heap( const std::uintptr_t address )
{
#if ! NDEBUG
    if( segments.heap != nullptr )
    {
#endif    
        return( ( address - segments.heap->start ) < segments.heap->addy_range );
#if ! NDEBUG        
    }
    return( false );
#endif
}

bool    
vm_memory_map::is_address_in_stack( const std::uintptr_t address )
{
#if ! NDEBUG
    if( segments.stack != nullptr )
    {
#endif     
        return( ( address - segments.stack->start ) < segments.stack->addy_range );
#if ! NDEBUG        
    }
    return( false );
#endif
}

vm_memory_map::vm_memory_map( const pid_t pid ) : pid( pid ),
    pid_map_string( "/proc/" + std::to_string( pid ) + "/maps" ), 
    pid_smap_string( "/proc/" + std::to_string( pid ) + "/smaps" ) 
{
    if( ! proc_tools::get_bin_path_for_pid( pid, bin_path ) )
    {
        std::fprintf( stderr, 
                      "Failed to discover binary path, can't proceed, exiting\n" );
        exit( EXIT_FAILURE );
    }
    initialize_os_map();
}

vm_memory_map::~vm_memory_map()
{
    for( auto &pair : vm_maps_db )
    {
        delete( pair.second );
    }
}

void
vm_memory_map::initialize_os_map( )
{
    std::ifstream maps_ifs( pid_smap_string );
    if( ! maps_ifs.is_open() )
    {
        return;
    }
    //else it's open
    std::string line;
    vm_maps_entry_t *m( nullptr );
    while( std::getline( maps_ifs, line ) )
    {
        m = new vm_smap_entry_t();
        m->add_fields_to_entry( maps_ifs, line ); 
        /** for smaps will consume multiple lines **/
        add_entry( m->start, m );
        std::cout << (*m) << "\n";
    }
    m = nullptr;
    maps_ifs.close();
    return;
}



void 
vm_memory_map::add_entry( const vm_maps_entry_t::address_t start_address, 
                          vm_maps_entry_t * const          entry )
{
    const auto found( vm_maps_db.insert( std::make_pair( start_address, entry ) ) );
    if( found.second == false )
    {
        delete( entry );
    }
    if( ( entry->perms & vm_maps_entry_t::execute ) == vm_maps_entry_t::execute )
    {
        //double check to make sure it's ours
        if( strncmp( bin_path.c_str(), entry->pathname, bin_path.length() ) == 0 )
        {
            segments.code = entry;
        }
    }
    if( std::strncmp( entry->pathname, 
                      headers::stack_string, 
                      headers::stack_string_length ) == 0 )
    {
        segments.stack = entry;
    }
    if( std::strncmp( entry->pathname, 
                      headers::heap_string, 
                      headers::heap_string_length ) == 0 )
    {
        segments.heap = entry;
    }
    return;
}

vm_maps_entry_t::address_t 
vm_memory_map::add_unique_entry( const vm_maps_entry_t::address_t  start_address,
                                 vm_maps_entry_t * const           entry )
{
    const auto found( vm_maps_db.insert( std::make_pair( start_address, entry ) ) );
    if( found.second == false )
    {
        return( (*found.first).first );
    }
    //else
    return( 0 );
}
    
std::ostream& 
operator << ( std::ostream &stream, const vm_memory_map &map )
{
    stream << "Important segments\n";
    stream << "Code: \n";
    stream << *reinterpret_cast< vm_smap_entry_t*>( map.segments.code ) << "\n"; 
    stream << "Stack: \n";
    stream << *reinterpret_cast< vm_smap_entry_t*>( map.segments.stack ) << "\n"; 
    stream << "Heap: \n";
    stream << *reinterpret_cast< vm_smap_entry_t*>( map.segments.heap ) << "\n"; 
    stream << "\n\n\n";
    for( const auto &pair : map.vm_maps_db )
    {
        stream << *reinterpret_cast< vm_smap_entry_t*>( pair.second ) << "\n";
    }
    return( stream );
}
