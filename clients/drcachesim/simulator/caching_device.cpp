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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "caching_device.h"
#include "caching_device_block.h"
#include "caching_device_stats.h"
#include "prefetcher.h"
#include "snoop_filter.h"
#include "../common/utils.h"
#include "dr_api.h"
#include <assert.h>
#include "trace_entry.h"

caching_device_t::caching_device_t()
    : page_stats_impl()
    , blocks_(NULL)
    , stats_(NULL)
    , prefetcher_(NULL)
    // The tag being hashed is already right-shifted to the cache line and
    // an identity hash is plenty good enough and nice and fast.
    // We set the size and load factor only if being used, in set_hashtable_use().
    , tag2block(0, [](addr_t key) { return static_cast<unsigned long>(key); })
{
}

caching_device_t::~caching_device_t()
{
    
    if( last_level )
    {
        //write 4KiB
        std::ofstream stream_4( stats_dir + "/ll_cache_usage_4KiB.dat" );
        //write 64KiB
        std::ofstream stream_64( stats_dir + "/ll_cache_usage_64KiB.dat" );
        //write 1MiB
        std::ofstream stream_1M( stats_dir + "/ll_cache_usage_1MiB.dat" );

        page_stats_impl::write(  stream_4,
                                 stream_64,
                                 stream_1M );

        page_stats_impl::destroy();

    }

    if (blocks_ == NULL)
        return;
    for (int i = 0; i < settings_.num_blocks; i++)
        delete blocks_[i];
    delete[] blocks_;
}


/**
 * TODO - the param lists here have gotten a bit insane...
 */
bool
caching_device_t::init( caching_device_settings_t &&settings,
                        caching_device_t *parent, 
                        caching_device_stats_t *stats,
                        prefetcher_t *prefetcher, 
                        snoop_filter_t *snoop_filter,
                        const std::vector<caching_device_t *> &children)
{
    settings_ = settings;
    if (!IS_POWER_OF_2(settings_.associativity) || !IS_POWER_OF_2( settings_.block_size) ||
        !IS_POWER_OF_2(settings_.num_blocks) ||
        // Assuming caching device block size is at least 4 bytes
        settings_.block_size < 4)
    {        
        return false;
    }
    if (stats == NULL)
    {
        return false; // A stats must be provided for perf: avoid conditional code
    }
    else if (!*stats)
    {
        return false;
    }
    loaded_blocks_      = 0;
    blocks_per_set_     = settings_.num_blocks / settings_.associativity;
    assoc_bits_         = compute_log2( settings_.associativity );
    block_size_bits_    = compute_log2( settings_.block_size);
    blocks_per_set_mask_ = blocks_per_set_ - 1;
    if (assoc_bits_ == -1 || block_size_bits_ == -1 || !IS_POWER_OF_2(blocks_per_set_))
        return false;
    parent_ = parent;
    stats_ = stats;
    prefetcher_ = prefetcher;
    snoop_filter_ = snoop_filter;
    blocks_ = new caching_device_block_t *[ settings_.num_blocks ];
    
    init_blocks( settings_.block_size );

    last_tag_ = TAG_INVALID; // sentinel

    children_ = children;
    
    last_level = false;
    return true;
}

std::pair<caching_device_block_t *, int>
caching_device_t::find_caching_device_block(addr_t tag)
{
    if (use_tag2block_table_) 
    {
        auto it = tag2block.find(tag);
        if (it == tag2block.end())
            return std::make_pair(nullptr, 0);
        assert(it->second.first->tag_ == tag);
        return it->second;
    }
    int block_idx = compute_block_idx(tag);
    for (int way = 0; way < settings_.associativity; ++way ) 
    {
        caching_device_block_t &block = get_caching_device_block(block_idx, way);
        if (block.tag_ == tag)
            return std::make_pair(&block, way);
    }
    return std::make_pair(nullptr, 0);
}

void
caching_device_t::request(const memref_t &memref_in)
{
    //force update at a specific interval for utilization histogram
    if( (++request_counter & static_cast< decltype( request_counter ) >( (1<<12)-1 ))  == 0 
            && settings_.record_line_utilization )
    {
        /** get latest stats **/
        forceUpdateInc();
        stats_->write_histogram( memref_in, request_counter );
    }
    // Unfortunately we need to make a copy for our loop so we can pass
    // the right data struct to the parent and stats collectors.
    memref_t memref;
    // We support larger sizes to improve the IPC perf.
    // This means that one memref could touch multiple blocks.
    // We treat each block separately for statistics purposes.
    addr_t final_addr = memref_in.data.addr + memref_in.data.size - 1 /*avoid overflow*/;
    addr_t final_tag = compute_tag(final_addr);
    addr_t tag = compute_tag(memref_in.data.addr);

    // Optimization: check last tag if single-block
    if (tag == final_tag && tag == last_tag_ && memref_in.data.type != TRACE_TYPE_WRITE) 
    {
        // Make sure last_tag_ is properly in sync.
        caching_device_block_t *cache_block =
            &get_caching_device_block(last_block_idx_, last_way_);
        assert(tag != TAG_INVALID && tag == cache_block->tag_);
        
        if( settings_.record_line_utilization )
        {
            /** get offset **/
            const auto offset( ( ~( UINT64_MAX << block_size_bits_ ) ) & memref_in.data.addr );
            /** update line utilization **/
            cache_block->update_utilization( memref_in, 
                                             offset, 
                                             *(settings_.record) );
            
        }
        record_access_stats(memref_in, true /*hit*/, cache_block);
        access_update(last_block_idx_, last_way_);
        return;
    }

    memref = memref_in;
    for (; tag <= final_tag; ++tag) {
        int way = settings_.associativity;
        int block_idx = compute_block_idx(tag);
        bool missed = false;

        if (tag + 1 <= final_tag)
            memref.data.size = ((tag + 1) << block_size_bits_) - memref.data.addr;

        auto block_way = find_caching_device_block(tag);
        if (block_way.first != nullptr) {
            // Access is a hit.
            caching_device_block_t *cache_block = block_way.first;
            way = block_way.second;
            if( settings_.record_line_utilization )
            {
                /** get offset **/
                const auto offset( ( ~( UINT64_MAX << block_size_bits_ ) ) & memref.data.addr );
                /** update line utilization **/
                cache_block->update_utilization( 
                    memref, 
                    offset, 
                    (*settings_.record) );
            }
            record_access_stats(    memref, 
                                    true /*hit*/, 
                                    cache_block );
            if( settings_.coherent_cache && memref.data.type == TRACE_TYPE_WRITE ) 
            {
                // On a hit, we must notify the snoop filter of the write or propagate
                // the write to a snooped cache.
                if (snoop_filter_ != NULL) {
                    snoop_filter_->snoop(tag, 
                                         settings_.id,
                                         (memref.data.type == TRACE_TYPE_WRITE));
                } else if (parent_ != NULL) {
                    // On a miss, the parent access will inherently propagate the write.
                    parent_->propagate_write(tag, this);
                }
            }
        } 
        else 
        // Access is a miss.
        {
            way = replace_which_way(block_idx);
            caching_device_block_t *cache_block =
                &get_caching_device_block(block_idx, way);

            record_access_stats(memref, false /*miss*/, cache_block);
            missed = true;
            // If no parent we assume we get the data from main memory
            if (parent_ != NULL)
            {
                parent_->request(memref);
            }
            if( parent_ == NULL && last_level && (*settings_.record) )
            {
                //miss is a memory access we need to record
                page_stats_impl::update( memref ); 
            }
            if (snoop_filter_ != NULL) 
            {
                // Update snoop filter, other private caches invalidated on write.
                snoop_filter_->snoop(tag, settings_.id, (memref.data.type == TRACE_TYPE_WRITE));
            }

            addr_t victim_tag = cache_block->tag_;
            // Check if we are inserting a new block, if we are then increment
            // the block loaded count.
            if (victim_tag == TAG_INVALID) {
                loaded_blocks_++;
            } else {
                if (!children_.empty() && settings_.inclusive ) 
                {
                    for (auto &child : children_) {
                        child->invalidate(victim_tag, INVALIDATION_INCLUSIVE);
                    }
                }
                if (settings_.coherent_cache) {
                    bool child_holds_tag = false;
                    if (!children_.empty()) {
                        /* We must check child caches to find out if the snoop filter
                         * should clear the ownership bit for this evicted tag.
                         * If any of this cache's children contain the evicted tag the
                         * snoop filter should still consider this cache an owner.
                         */
                        for (auto &child : children_) {
                            if (child->contains_tag(victim_tag)) {
                                child_holds_tag = true;
                                break;
                            }
                        }
                    }
                    if (!child_holds_tag) {
                        if (snoop_filter_ != NULL) {
                            // Inform snoop filter of evicted line.
                            snoop_filter_->snoop_eviction(victim_tag, settings_.id);
                        } else if (parent_ != NULL) {
                            // Inform parent of evicted line.
                            parent_->propagate_eviction(victim_tag, this);
                        }
                    }
                }
            }
            update_tag(cache_block, way, tag);
        }

        access_update(block_idx, way);

        // Issue a hardware prefetch, if any, before we remember the last tag,
        // so we remember this line and not the prefetched line.
        if (missed && !type_is_prefetch(memref.data.type) && prefetcher_ != nullptr)
            prefetcher_->prefetch(this, memref);

        if (tag + 1 <= final_tag) {
            addr_t next_addr = (tag + 1) << block_size_bits_;
            memref.data.addr = next_addr;
            memref.data.size = final_addr - next_addr + 1 /*undo the -1*/;
        }

        // Optimization: remember last tag
        last_tag_ = tag;
        last_way_ = way;
        last_block_idx_ = block_idx;
    }
}

void
caching_device_t::access_update(int block_idx, int way)
{
    // We just inc the counter for LFU.  We live with any blip on overflow.
    get_caching_device_block(block_idx, way).counter_++;
}

int
caching_device_t::replace_which_way(int block_idx)
{
    // The base caching device class only implements LFU.
    // A subclass can override this and access_update() to implement
    // some other scheme.
    int min_counter{ 0 }, min_way{ 0 }; 
    for( int way{ 0 }; way < settings_.associativity; ++way) 
    {
        const auto &block( get_caching_device_block( block_idx, way ) );
        if( block.tag_ == TAG_INVALID) 
        {
            min_way = way;
            break;
        }
        if( way == 0 || block.counter_ < min_counter ) 
        {
            min_counter = block.counter_;
            min_way = way;
        }
    }
    auto &block( get_caching_device_block( block_idx, min_way ) );
    // Clear the counter for LFU.
    //if( block.valid && block.event_flags[0] )
    //{
    //    stats_->flush_update( block.getBits() );
    //}
    block.reset();
    return min_way;
}

void
caching_device_t::invalidate(addr_t tag, invalidation_type_t invalidation_type)
{
    auto block_way = find_caching_device_block(tag);
    if (block_way.first != nullptr) {
        invalidate_caching_device_block(block_way.first);
        stats_->invalidate(invalidation_type);
        // Invalidate last_tag_ if it was this tag.
        if (last_tag_ == tag) {
            last_tag_ = TAG_INVALID;
        }
        // Invalidate the block in the children's caches.
        if (invalidation_type == INVALIDATION_INCLUSIVE && settings_.inclusive &&
            !children_.empty()) {
            for (auto &child : children_) {
                child->invalidate(tag, invalidation_type);
            }
        }
    }
    // If this is a coherence invalidation, we must invalidate children caches.
    if (invalidation_type == INVALIDATION_COHERENCE && !children_.empty()) {
        for (auto &child : children_) {
            child->invalidate(tag, invalidation_type);
        }
    }
}

// This function checks if this cache or any child caches hold a tag.
bool
caching_device_t::contains_tag(addr_t tag)
{
    auto block_way = find_caching_device_block(tag);
    if (block_way.first != nullptr)
        return true;
    if (children_.empty()) {
        return false;
    }
    for (auto &child : children_) {
        if (child->contains_tag(tag)) {
            return true;
        }
    }
    return false;
}

// A child has evicted this tag, we propagate this notification to the snoop filter,
// unless this cache or one of its other children holds this line.
void
caching_device_t::propagate_eviction(addr_t tag, const caching_device_t *requester)
{
    // Check our own cache for this line.
    auto block_way = find_caching_device_block(tag);
    if (block_way.first != nullptr)
        return;

    // Check if other children contain this line.
    if (children_.size() != 1) {
        // If another child contains the line, we don't need to do anything.
        for (auto &child : children_) {
            if (child != requester && child->contains_tag(tag)) {
                return;
            }
        }
    }

    // Neither this cache nor its children hold line,
    // inform snoop filter or propagate eviction.
    if (snoop_filter_ != NULL) {
        snoop_filter_->snoop_eviction(tag, settings_.id );
    } else if (parent_ != NULL) {
        parent_->propagate_eviction(tag, this);
    }
}

/* This function is called by a coherent child performing a write.
 * This cache must forward this write to the snoop filter and invalidate this line
 * in any other children.
 */
void
caching_device_t::propagate_write(addr_t tag, const caching_device_t *requester)
{
    // Invalidate other children.
    for (auto &child : children_) {
        if (child != requester) {
            child->invalidate(tag, INVALIDATION_COHERENCE);
        }
    }

    // Propagate write to snoop filter or to parent_.
    if (snoop_filter_ != NULL) {
        snoop_filter_->snoop(tag, settings_.id, true);
    } else if (parent_ != NULL) {
        parent_->propagate_write(tag, this);
    }
}

void
caching_device_t::record_access_stats(const memref_t &memref, bool hit,
                                      caching_device_block_t *cache_block)
{
    stats_->access(memref, hit, cache_block);
    // We propagate hits all the way up the hierachy.
    // But to avoid over-counting we only propagate misses one level up.
    if (hit) {
        for (caching_device_t *up = parent_; up != nullptr; up = up->parent_)
            up->stats_->child_access(memref, hit, cache_block);
    } else if (parent_ != nullptr)
        parent_->stats_->child_access(memref, hit, cache_block);
}


void
caching_device_t::forceUpdateInc() noexcept
{
    for( auto i{0}; i < settings_.num_blocks; i++)
    {
        auto * const block( blocks_[ i ] );
        if( block->valid && block->event_flags[0] )
        {
            stats_->inc_update( block->getBits() );
        }
    }
    return;
}


void
caching_device_t::forceUpdateFinal() noexcept
{
    for( auto i{0}; i < settings_.num_blocks; i++)
    {
        auto * const block( blocks_[ i ] );
        if( block->valid && block->event_flags[0] )
        {
            stats_->flush_update( block->getBits() );
        }
    }
    return;
}
    

void 
caching_device_t::set_as_last_level()
{
    last_level = true;
    //get here, destructor called otherwise and we lose value
    stats_dir  = stats_->get_stats_dir();
    page_stats_impl::init();
}




