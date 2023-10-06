/* **********************************************************
 * Copyright (c) 2015-2021 Google, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Google, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "cache.h"
#include "defs.h"
#include "../common/utils.h"
#include <assert.h>
#include "caching_device_settings.h"
#include <functional>

bool
cache_t::init(  cache_settings_t  &&settings, 
                caching_device_t *parent,
                caching_device_stats_t *stats, 
                prefetcher_t *prefetcher, 
                snoop_filter_t *snoop_filter,
                const std::vector<caching_device_t *> &children )
{
    // convert total_size to num_blocks to fit for caching_device_t::init
    const auto num_blocks = settings.total_size / settings.block_size;
    
    return caching_device_t::init(  caching_device_settings_t( 
                                        std::forward< cache_settings_t >( settings ), 
                                        num_blocks ), 
                                    parent, 
                                    stats,
                                    prefetcher, 
                                    snoop_filter,
                                    children );
}

void
cache_t::init_blocks( const std::size_t line_size )
{
    for( int i = 0; i < settings_.num_blocks; i++) 
    {
        if( record_utilization_ )
        {
            blocks_[ i ] = new cache_line_t< true >( line_size );
        }
        else
        {
            blocks_[ i ] = new cache_line_t< false >();
        }
    }
}

void
cache_t::request(const memref_t &memref)
{
    caching_device_t::request(memref);
}

void
cache_t::flush(const memref_t &memref)
{
    addr_t tag = compute_tag(memref.flush.addr);
    addr_t final_tag =
        compute_tag(memref.flush.addr + memref.flush.size - 1 /*no overflow*/);
    last_tag_ = TAG_INVALID;
    for (; tag <= final_tag; ++tag) 
    {
        auto block_way = find_caching_device_block(tag);
        if (block_way.first == nullptr)
            continue;
        invalidate_caching_device_block(block_way.first);
    }
    // We flush parent_'s code cache here.
    // XXX: should L1 data cache be flushed when L1 instr cache is flushed?
    if (parent_ != NULL)
        ((cache_t *)parent_)->flush(memref);
    if (stats_ != NULL)
        ((cache_stats_t *)stats_)->flush(memref);
}
