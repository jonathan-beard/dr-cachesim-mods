/**
 * memorymap.hpp - 
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
#ifndef _MEMORYMAP_HPP_
#define _MEMORYMAP_HPP_  1
#include <map>
#include <unistd.h>
#include <climits>
#include <sys/types.h>
#include <iostream>
#include <set>
#include <cstring>

#include "pagemap.hpp"
#include "vm_map_db.hpp"
#include "vm_map_entry.hpp"

class vm_memory_map 
{
public:
    vm_memory_map( const pid_t pid );
    
    virtual ~vm_memory_map();
    
    
    /** 
     * update_map - takes in a vm_memory_map object, defined
     * in memorymap.hpp. The map is updated with the delta 
     * from when the map was created to the current reading 
     * of /proc/<id>/maps.
     * @param - map - vm_memory_map 
     * @return  vm_memory_map
     */
    void update_map();


    /**
     * add_os_vm_text_offset_to_pc - returns input pc on fail. 
     */
    std::uintptr_t add_os_vm_offset_to_pc( const std::uintptr_t pc );


    bool    is_address_in_code( const std::uintptr_t address );
    
    bool    is_address_in_heap( const std::uintptr_t address );

    bool    is_address_in_stack( const std::uintptr_t address );

protected:
    
    /**
     * initialize_map - initialize a vm_memory_map object
     * and return it with the information from /proc/<id>/maps
     * @param - map - vm_memory_map 
     */
    void initialize_os_map();

    /**
     * addEntry - add the specified entry to the map, ignore if it's
     * a duplicate start address and not actually added. Given memory
     * is considered to be owned by this function and the map once this 
     * function is called, the entry object is de-allocated (using delete)
     * @param start_address
     * @param entry
     * @return - nothing 
     */
    virtual void add_entry( const vm_maps_entry_t::address_t start_address, 
                            vm_maps_entry_t * const          entry );
 
    /**
     * addUniqueEntry - adds an entry to the map, returns the address
     * of the entry if another entry with the same memory start address
     * is in the map.
     * @param start_address
     * @param entry
     * @return - 0 if the added address entry is unique
     */
    virtual vm_maps_entry_t::address_t 
        add_unique_entry( const vm_maps_entry_t::address_t  start_address,
                          vm_maps_entry_t * const           entry );

    struct headers
    {
        /** instructions/etc.                                   **/
        vm_maps_entry_t *code   = nullptr;
        vm_maps_entry_t *heap   = nullptr;
        vm_maps_entry_t *stack  = nullptr;

        /** 
         * many other segments, but we'd have to parse 
         * the ELF again  to figure out what's mapped
         * where, right now we don't need them so leave
         * it. 
         */
         constexpr static auto stack_string = "[stack]";
         /** why count up lines when strlen can be constexpr =)) **/
         constexpr static auto stack_string_length = std::strlen( stack_string );
         constexpr static auto heap_string  = "[heap]";
         constexpr static auto heap_string_length = std::strlen( heap_string );
    }segments;

private:
    vm_maps_db_t    vm_maps_db;
    
    
    std::uint8_t      page_granule_pow_2;
    
    const             pid_t pid;

    const std::string pid_map_string;
    const std::string pid_smap_string;
    
    std::string       bin_path;

    friend std::ostream& operator << ( std::ostream&, const vm_memory_map&);

};
    
std::ostream& operator << ( std::ostream &stream, const vm_memory_map &map );


#endif /* END _MEMORYMAP_HPP_ */
