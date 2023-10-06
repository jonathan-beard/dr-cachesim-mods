/**
 * caching_device_settings.hpp - 
 * @author: Jonathan Beard
 * @version: Fri Mar 25 10:33:21 2022
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
#ifndef CACHING_DEVICE_SETTINGS_HPP
#define CACHING_DEVICE_SETTINGS_HPP  1
#include "cache_settings.h"


struct caching_device_settings_t : cache_settings_t
{
    //base constructor needed for users to have constructor + init pattern
    caching_device_settings_t() = default;

    /**
     * need this one for move constructor 
     */
    constexpr caching_device_settings_t( const caching_device_settings_t &settings ) 
                                 : cache_settings_t( settings ),
                                   num_blocks( settings.num_blocks ){}


        

    /**
     * signature so we can maintain DR's default args. 
     */
    constexpr caching_device_settings_t( int associativity,
                                       int block_size,
                                       int num_blocks,
                                       int total_size,
                                       bool record_line_utilization,
                                       atomic_bool_t   *record,
                                       bool inclusive  = false,
                                       bool coherent_cache = false,
                                       int  id_ = -1 )
                                 : cache_settings_t( associativity,
                                                     block_size,
                                                     total_size,
                                                     record_line_utilization,
                                                     record,
                                                     inclusive,
                                                     coherent_cache,
                                                     id_ ),
                                    num_blocks( num_blocks ){}
    
    /** here really just so we don't have to retype in cache.cpp when instantiating **/
    constexpr caching_device_settings_t( const cache_settings_t &&settings,
                                       const int num_blocks ) : cache_settings_t( settings ),
                                                                num_blocks( num_blocks ){}

    
    int num_blocks = 0; 
};

inline std::ostream& operator << ( std::ostream &stream, const caching_device_settings_t &settings )
{
    const auto sep = ", ";
    stream <<  static_cast< const cache_settings_t >( settings ) << sep;
    stream << "num_blocks: " << sep << settings.num_blocks;
    return( stream );
}

#endif /* END CACHING_DEVICE_SETTINGS_HPP */
