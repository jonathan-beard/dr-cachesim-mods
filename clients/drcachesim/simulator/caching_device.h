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

/* caching_device: represents a hardware caching device.
 */

#ifndef _CACHING_DEVICE_H_
#define _CACHING_DEVICE_H_ 1

#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <bitset>
#include <ostream>
#include <functional>
#include <iostream>
#include <cstdio>
#include <atomic>
#include "defs.h"
//#include "requestable.hpp"
#include "caching_device_block.h"
#include "caching_device_stats.h"
#include "memref.h"
#include "prefetcher.h"
#include "trace_entry.h"
#include "dr_api.h"
#include "page_stats_impl.hpp"
#include "cache_settings.h"
#include "caching_device_settings.h"

// Statistics collection is abstracted out into the caching_device_stats_t class.

// Different replacement policies are expected to be implemented by
// subclassing caching_device_t.

// We assume we're only invoked from a single thread of control and do
// not need to synchronize data access.

class snoop_filter_t;


class caching_device_t : public page_stats_impl {
public:
    
    caching_device_t();

    virtual bool
    init(   caching_device_settings_t    &&settings, 
            caching_device_t *parent,
            caching_device_stats_t *stats, 
            prefetcher_t *prefetcher = nullptr,
            snoop_filter_t *snoop_filter_ = nullptr,
            const std::vector<caching_device_t *> &children = {} );

    virtual ~caching_device_t();

    virtual void
    request(const memref_t &memref);
    
    virtual void
    invalidate(addr_t tag, invalidation_type_t invalidation_type_);
    
    bool
    contains_tag(addr_t tag);
    
    virtual
    void
    propagate_eviction(addr_t tag, const caching_device_t *requester);

    virtual 
    void
    propagate_write(addr_t tag, const caching_device_t *requester);

    caching_device_stats_t *
    get_stats() const
    {
        return stats_;
    }
    void
    set_stats(caching_device_stats_t *stats)
    {
        stats_ = stats;
    }
    prefetcher_t *
    get_prefetcher() const
    {
        return prefetcher_;
    }
    caching_device_t *
    get_parent() const
    {
        return parent_;
    }
    inline double
    get_loaded_fraction() const
    {
        return double(loaded_blocks_) / settings_.num_blocks;
    }
    // Must be called prior to any call to request().
    virtual inline void
    set_hashtable_use(bool use_hashtable)
    {
        if (!use_tag2block_table_ && use_hashtable) {
            // Resizing from an initial small table causes noticeable overhead, so we
            // start with a relatively large table.
            tag2block.reserve(1 << 16);
            // Even with the large initial size, for large caches we want to keep the
            // load factor small.
            tag2block.max_load_factor(0.5);
        }
        use_tag2block_table_ = use_hashtable;
    }
    
    /**
     * forceUpdate - call me if you have any stats that need
     * updating before you call stats, i.e., as in if you have cache 
     * lines that are in fact not flushed and you need stats from them
     * before you destroy the cache
     */
    virtual void forceUpdateFinal() noexcept;
    /**
     * foceUpdateInc - incremental forced update, called at granularity
     * intervals (global static set in caching_device_stats.h so that
     * we may check running stats
     */
    virtual void forceUpdateInc() noexcept;
    
    void set_as_last_level();

protected:
    virtual void
    access_update(int block_idx, int way);
    virtual int
    replace_which_way(int block_idx);
    virtual void
    record_access_stats(const memref_t &memref, bool hit,
                        caching_device_block_t *cache_block);

    inline addr_t
    compute_tag(addr_t addr)
    {
        return addr >> block_size_bits_;
    }
    inline int
    compute_block_idx(addr_t tag)
    {
        return (tag & blocks_per_set_mask_) << assoc_bits_;
    }
    inline caching_device_block_t &
    get_caching_device_block(int block_idx, int way)
    {
        return *(blocks_[block_idx + way]);
    }

    inline void
    invalidate_caching_device_block(caching_device_block_t *block)
    {
        if (use_tag2block_table_)
            tag2block.erase(block->tag_);
        block->tag_ = TAG_INVALID;
        // Xref cache_block_t constructor about why we set counter to 0.
        block->counter_ = 0;
    }

    inline void
    update_tag(caching_device_block_t *block, int way, addr_t new_tag)
    {
        if (use_tag2block_table_) {
            if (block->tag_ != TAG_INVALID)
                tag2block.erase(block->tag_);
            tag2block[new_tag] = std::make_pair(block, way);
        }
        block->tag_ = new_tag;
    }

    // Returns the block (and its way) whose tag equals `tag`.
    // Returns <nullptr,0> if there is no such block.
    std::pair<caching_device_block_t *, int>
    find_caching_device_block(addr_t tag);

    // a pure virtual function for subclasses to initialize their own block array
    virtual void
    init_blocks( const std::size_t line_size ) = 0;

    // Current valid blocks in the cache
    int loaded_blocks_;

    // Pointers to the caching device's parent and children devices.
    caching_device_t                *parent_        = nullptr;
    std::vector<caching_device_t *>  children_;

    snoop_filter_t                  *snoop_filter_  = nullptr;

    // This should be an array of caching_device_block_t pointers, otherwise
    // an extended block class which has its own member variables cannot be indexed
    // correctly by base class pointers.
    caching_device_block_t **blocks_    = nullptr;

    //set internal to this class
    int blocks_per_set_;
    // Optimization fields for fast bit operations
    int blocks_per_set_mask_;
    int assoc_bits_;
    int block_size_bits_;

    caching_device_stats_t *stats_  = nullptr;
    prefetcher_t           *prefetcher_ = nullptr;

    // Optimization: remember last tag
    addr_t last_tag_;
    int last_way_;
    int last_block_idx_;
    // Optimization: keep a hashtable for quick lookup of {block,way}
    // given a tag, if using a large cache hierarchy where serial
    // walks over the associativity end up as bottlenecks.
    // We can't easily remove the blocks_ array and replace with just
    // the hashtable as replace_which_way(), etc. want quick access to
    // every way for a given line index.
    std::unordered_map<addr_t, std::pair<caching_device_block_t *, int>,
                       std::function<unsigned long(addr_t)>>
        tag2block;
    bool use_tag2block_table_ = false;
    
    uintmax_t   request_counter     = 0;

    //only set if last level, leave these here for now
    bool        last_level          = false;
    std::string stats_dir           = "";

    caching_device_settings_t       settings_;
    friend std::ostream& operator << ( std::ostream &stream, const caching_device_t &d );
};


inline std::ostream& operator << ( std::ostream &stream, const caching_device_t &d )
{
    stream << d.settings_;
    return( stream );
}

#endif /* _CACHING_DEVICE_H_ */
