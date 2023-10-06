/**
 * tlb_device_settings.hpp - 
 * @author: Jonathan Beard
 * @version: Fri Mar 25 11:14:31 2022
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
#ifndef TLB_DEVICE_SETTINGS_HPP
#define TLB_DEVICE_SETTINGS_HPP  1
#include <functional>
#include "caching_device_settings.h"

struct tlb_device_settings_t : caching_device_settings_t
{


    /**
     * signature so we can maintain DR's default args. 
     */
    constexpr tlb_device_settings_t( int associativity,
                                       int block_size,
                                       int num_blocks,
                                       atomic_bool_t   *record,
                                       bool inclusive  = false,
                                       bool coherent_cache = false,
                                       int  id_ = -1 )
                                 : caching_device_settings_t( associativity,
                                                     block_size,
                                                     num_blocks,
                                                     0 /** total size ??**/,
                                                     false,
                                                     record,
                                                     inclusive,
                                                     coherent_cache,
                                                     id_ ){}
    
    /** here really just so we don't have to retype in cache.cpp when instantiating **/
    constexpr tlb_device_settings_t( const cache_settings_t &&settings,
                                     const int num_blocks ) : 
        caching_device_settings_t( std::forward< const cache_settings_t >(  settings ), 
                                   num_blocks ){} 


};

#endif /* END TLB_DEVICE_SETTINGS_HPP */
