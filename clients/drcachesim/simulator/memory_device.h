/**
 * memory_device.hpp - 
 * @author: Jonathan Beard
 * @version: Wed Mar  2 08:40:29 2022
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
#ifndef MEMORY_DEVICE_HPP
#define MEMORY_DEVICE_HPP  1

#include "caching_device.h"
#include "cache_line.h"
#include "cache_stats.h"

/**
 * notes: 
 * extended caching_device_t given that it's the easiest 
 * thing to extend to make a parent that'll just work 
 * with the current cache topology. It's not really a 
 * caching agent...
 */
class memory_device_t: public caching_device_t 
{
public:
    bool
    init(   const std::vector< std::uint8_t > &page_sizes,
            caching_device_t                  *parent,
            int                               id_ = -1,
            const std::vector< caching_device_t* > &children = {} ) override;


    
    void
    request(const memref_t &memref) override;
    
    virtual void flush(const memref_t &memref);

protected:
    void
    init_blocks( const std::size_t line_size ) override;
};
#endif /* END MEMORY_DEVICE_HPP */
