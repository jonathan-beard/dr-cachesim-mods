/**
 * vm_map_entry.hpp - 
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
#ifndef _VM_MAP_ENTRY_HPP_
#define _VM_MAP_ENTRY_HPP_  1
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <climits>
#include <string>

/**
 * /proc/<id>/maps format - note: maps can change and is also
 * just like everything else in proc, it's created on demand.
 *
 * address           perms offset  dev   inode   pathname
 * 08048000-08056000 r-xp 00000000 03:0c 64593   /usr/sbin/gpm
 * for more info on this format, use `man proc` then go forward
 * to the section on proc/[pid]/maps 
**/


/**
 * NOTE: physical address: long index = (vas / PAGE_SIZE) * sizeof(unsigned long long); 
 */



struct vm_maps_entry_t
{
    using address_t = std::uint64_t;
    
    
    vm_maps_entry_t() = default;
    virtual ~vm_maps_entry_t() = default;

    
    virtual void add_fields_to_entry( std::ifstream&, const std::string&);


    address_t       start   = 0;
    address_t       end     = 0;
    std::uint32_t   perms   = 0; /** one bit per RWX[S/P] **/
    std::uint64_t   offset  = 0;
    std::uint32_t   maj_dev = 0;
    std::uint32_t   min_dev = 0;
    std::uint64_t   inode   = 0;
    char            pathname[ PATH_MAX /** use linux vs. POSIX path length **/ ];
    /** this one is derived (end-start)+1 **/
    address_t       addy_range = 0;

    enum page_permissions : std::uint32_t {  read    = ( 1 << 0 ), 
                                             write   = ( 1 << 1 ), 
                                             execute = ( 1 << 2 ), 
                                             shared  = ( 1 << 3 ) };

};


std::ostream& 
operator << ( std::ostream &stream, const vm_maps_entry_t &entry);


#endif /* END _VM_MAP_ENTRY_HPP_ */
