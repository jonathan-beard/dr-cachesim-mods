/**
 * vm_map_db.hpp - 
 * @author: Jonathan Beard
 * @version: Wed Mar 30 10:28:08 2022
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
#ifndef VM_MAP_DB_HPP
#define VM_MAP_DB_HPP  1
#include <map>
#include "vm_map_entry.hpp"


    /** for right now **/
using vm_maps_db_t =    std::map<  vm_maps_entry_t::address_t /** base virtual addy **/, 
                                   vm_maps_entry_t* >;


#endif /* END VM_MAP_DB_HPP */
