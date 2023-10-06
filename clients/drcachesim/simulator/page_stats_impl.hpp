/**
 * page_stats_impl.hpp - 
 * @author: Jonathan Beard
 * @version: Mon Mar 21 13:33:21 2022
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
#ifndef PAGE_STATS_IMPL_HPP
#define PAGE_STATS_IMPL_HPP  1

#include "page_stats.tcc"
#include <fstream>
#include "memref.h"
#include "defs.h"

class page_stats_impl
{
public:    
    constexpr page_stats_impl() = default;

    virtual  ~page_stats_impl() = default;


    virtual void init( );
    
    virtual void update( const memref_t &memref );
    
    virtual void write( std::ofstream &ostream_4, std::ofstream &ostream_64, std::ofstream &ostream_1M );
    
    virtual void destroy();

protected:
    gran_stats_t< 64    , 12> *_4K_stats     = nullptr;
    gran_stats_t< 1024  , 16> *_64K_stats    = nullptr;
    gran_stats_t< 16384 , 20> *_1M_stats     = nullptr;
};

#endif /* END PAGE_STATS_IMPL_HPP */
