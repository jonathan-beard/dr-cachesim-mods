/**
 * cache_settings.hpp - 
 * @author: Jonathan Beard
 * @version: Fri Mar 25 10:03:00 2022
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
#ifndef CACHE_SETTINGS_HPP
#define CACHE_SETTINGS_HPP  1

#include "defs.h"
#include <ostream>

struct cache_settings_t
{
    
    cache_settings_t() = default;

    /**
     * signature so we can maintain DR's default args. 
     */
    constexpr cache_settings_t( int associativity,
                                int block_size,
                                int total_size,
                                bool record_line_utilization,
                                atomic_bool_t   *record,
                                bool inclusive  = false,
                                bool coherent_cache = false,
                                int  id = -1 )
                                 : associativity( associativity ),
                                    block_size( block_size ),
                                    total_size( total_size ),
                                    record_line_utilization( record_line_utilization ),
                                    record( record ),
                                    inclusive( inclusive ),
                                    coherent_cache( coherent_cache ),
                                    id( id ){}

    /**
     * need this one for move constructor 
     */
    constexpr cache_settings_t( const cache_settings_t &settings ) 
                                 : associativity( settings.associativity ),
                                    block_size(    settings.block_size ),
                                    total_size(   settings.total_size ),
                                    record_line_utilization( settings.record_line_utilization ),
                                    record(         settings.record ),
                                    inclusive(      settings.inclusive ),
                                    coherent_cache( settings.coherent_cache ),
                                    id(            settings.id ){}


    constexpr cache_settings_t( const cache_settings_t &&settings ) 
                                 : associativity( settings.associativity ),
                                    block_size(    settings.block_size ),
                                    total_size(   settings.total_size ),
                                    record_line_utilization( settings.record_line_utilization ),
                                    record(         settings.record ),
                                    inclusive(      settings.inclusive ),
                                    coherent_cache( settings.coherent_cache ),
                                    id(            settings.id ){}

    cache_settings_t& operator=( const cache_settings_t &other )
    {
        associativity= other.associativity ;
        block_size=    other.block_size ;
        total_size=   other.total_size ;
        record_line_utilization= other.record_line_utilization ;
        record=         other.record ;
        inclusive=      other.inclusive ;
        coherent_cache= other.coherent_cache ;
        id  =            other.id;
        return( *this );
    }


    int             associativity;
    //refers to blocks which could be cache lines or pages
    int             block_size;
    //total syze in bytes
    int             total_size;
    //this only refers to cache lines, better mechanisms are there for pages
    bool            record_line_utilization    = false;
    //inside instrumentation, record
    atomic_bool_t   *record                    = nullptr;
    bool            inclusive                  = false;
    bool            coherent_cache             = false;
    //index into snoop filter's array of caches
    int             id                         = -1;
};

inline std::ostream& operator << ( std::ostream &stream, const cache_settings_t &settings )
{
    const auto sep = ", ";
    stream << "associativity: "     << settings.associativity   << sep;
    stream << "block_size: "        << settings.block_size      << sep;
    stream << "total_size: "        << settings.total_size      << sep;
    stream << "inclusive: "         << settings.inclusive       << sep;
    stream << "coherent_cache: "    << settings.coherent_cache  << sep;
    stream << "id: "              << settings.id              << sep;
    return( stream );
}

#endif /* END CACHE_SETTINGS_HPP */
