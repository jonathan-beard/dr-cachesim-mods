/* **********************************************************
 * Copyright (c) 2015-2020 Google, Inc.  All rights reserved.
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

#include <assert.h>
#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "dr_api.h"

#include "../common/options.h"
#include "caching_device_stats.h"
#include "caching_device.h"

caching_device_stats_t::caching_device_stats_t( const std::string directory_name,
                                                const std::string cache_name, 
                                                const bool        record_utilization,
                                                int block_size, 
                                                const std::string miss_file,
                                                bool warmup_enabled,
                                                bool is_coherent )
    : success_(true)
    , num_hits_(0)
    , num_misses_(0)
    , num_compulsory_misses_(0)
    , num_child_hits_(0)
    , num_inclusive_invalidates_(0)
    , num_coherence_invalidates_(0)
    , num_hits_at_reset_(0)
    , num_misses_at_reset_(0)
    , num_child_hits_at_reset_(0)
    , warmup_enabled_(warmup_enabled)
    , is_coherent_(is_coherent)
    , access_count_(block_size)
    , stats_dir( directory_name )
    , record_utilization_( record_utilization )
    , file_(nullptr)
{
    /** multiple calls here, but subsequent ones will hit success quickly **/
    struct stat st;
    std::memset( &st, 0, sizeof( struct stat ) );
    if( stat( directory_name.c_str(), &st ) != 0 )
    {
        //make directory
        if( mkdir( directory_name.c_str(), 0777 ) != 0 )
        {
            perror( "failed to create stats directory as specified" );
            DR_ASSERT( false );
        }
    }
    //check to make sure its a directory
    else if( ! S_ISDIR( st.st_mode ) )
    {
        fprintf( stderr, "\"%s\" is not a directory, can't write stats file\n", directory_name.c_str() );
        DR_ASSERT( false );
    }
    
    if (miss_file.empty()) {
        dump_misses_ = false;
    } else {
#ifdef HAS_ZLIB
        file_ = gzopen(miss_file.c_str(), "w");
#else
        file_ = fopen(miss_file.c_str(), "w");
#endif
        if (file_ == nullptr) {
            dump_misses_ = false;
            success_ = false;
        } else
            dump_misses_ = true;
    }
    if( record_utilization_ )
    {
        //else open stats file
        histogram_stream.open( directory_name + "/" + cache_name + ".dat" );
        if( ! histogram_stream.is_open() )
        {
            std::perror( "failed to open utilization stream" );
            DR_ASSERT( false );
        }
    }
    this->cache_name = cache_name;
    this->stats_dir  = directory_name;
    /** open stream for each cache stats **/
    cache_stats_stream.open( directory_name + "/" + cache_name + ".txt" );
    if( ! cache_stats_stream.is_open() )
    {
        std::perror( "Failed to open cache stats stream, exiting!" );
        DR_ASSERT( false );
    }



    stats_map_.emplace(metric_name_t::HITS, num_hits_);
    stats_map_.emplace(metric_name_t::MISSES, num_misses_);
    stats_map_.emplace(metric_name_t::HITS_AT_RESET, num_hits_at_reset_);
    stats_map_.emplace(metric_name_t::MISSES_AT_RESET, num_misses_at_reset_);
    stats_map_.emplace(metric_name_t::COMPULSORY_MISSES, num_compulsory_misses_);
    stats_map_.emplace(metric_name_t::CHILD_HITS_AT_RESET, num_child_hits_at_reset_);
    stats_map_.emplace(metric_name_t::CHILD_HITS, num_child_hits_);
    stats_map_.emplace(metric_name_t::INCLUSIVE_INVALIDATES, num_inclusive_invalidates_);
    stats_map_.emplace(metric_name_t::COHERENCE_INVALIDATES, num_coherence_invalidates_);
}

caching_device_stats_t::~caching_device_stats_t()
{
    if (file_ != nullptr) {
#ifdef HAS_ZLIB
        gzclose(file_);
#else
        fclose(file_);
#endif
    }
}

void
caching_device_stats_t::access(const memref_t &memref, bool hit,
                               caching_device_block_t *cache_block)
{
    // We assume we're single-threaded.
    // We're only computing miss rate so we just inc counters here.
    if (hit)
    {
        num_hits_++;
    }
    else 
    {
        num_misses_++;
        if (dump_misses_)
        {
            dump_miss(memref);
        }
        check_compulsory_miss(memref.data.addr);
    }
}

void
caching_device_stats_t::child_access(const memref_t &memref, bool hit,
                                     caching_device_block_t *cache_block)
{
    if (hit)
        num_child_hits_++;
    // else being computed in access()
}

void
caching_device_stats_t::check_compulsory_miss(addr_t addr)
{
    auto lookup_pair = access_count_.lookup(addr);

    // If the address has never been accessed insert proper bound into access_count_
    // and count it as a compulsory miss.
    if (!lookup_pair.first) {
        num_compulsory_misses_++;
        access_count_.insert(addr, lookup_pair.second);
    }
}

void
caching_device_stats_t::dump_miss(const memref_t &memref)
{
    addr_t pc, addr;
    if (type_is_instr(memref.data.type))
        pc = memref.instr.addr;
    else { // data ref: others shouldn't get here
        assert(type_is_prefetch(memref.data.type) ||
               memref.data.type == TRACE_TYPE_READ ||
               memref.data.type == TRACE_TYPE_WRITE);
        pc = memref.data.pc;
    }
    addr = memref.data.addr;
#ifdef HAS_ZLIB
    gzprintf(file_, "0x%zx,0x%zx\n", pc, addr);
#else
    fprintf(file_, "0x%zx,0x%zx\n", pc, addr);
#endif
}

void
caching_device_stats_t::print_warmup(std::string prefix)
{
    cache_stats_stream << prefix << std::setw(18) << std::left << "Warmup hits:" << std::setw(20)
              << std::right << num_hits_at_reset_ << std::endl;
    cache_stats_stream << prefix << std::setw(18) << std::left << "Warmup misses:" << std::setw(20)
              << std::right << num_misses_at_reset_ << std::endl;
}

void
caching_device_stats_t::print_counts(std::string prefix)
{
    cache_stats_stream << prefix << std::setw(18) << std::left << "Hits:" << std::setw(20)
              << std::right << num_hits_ << std::endl;
    cache_stats_stream << prefix << std::setw(18) << std::left << "Misses:" << std::setw(20)
              << std::right << num_misses_ << std::endl;
    cache_stats_stream << prefix << std::setw(18) << std::left
              << "Compulsory misses:" << std::setw(20) << std::right
              << num_compulsory_misses_ << std::endl;
    if (is_coherent_) {
        cache_stats_stream << prefix << std::setw(21) << std::left
                  << "Parent invalidations:" << std::setw(17) << std::right
                  << num_inclusive_invalidates_ << std::endl;
        cache_stats_stream << prefix << std::setw(20) << std::left
                  << "Write invalidations:" << std::setw(18) << std::right
                  << num_coherence_invalidates_ << std::endl;
    } else {
        cache_stats_stream << prefix << std::setw(18) << std::left
                  << "Invalidations:" << std::setw(20) << std::right
                  << num_inclusive_invalidates_ << std::endl;
    }
}

void
caching_device_stats_t::print_rates(std::string prefix)
{
    if (num_hits_ + num_misses_ > 0) {
        std::string miss_label = "Miss rate:";
        if (num_child_hits_ != 0)
            miss_label = "Local miss rate:";
        cache_stats_stream << prefix << std::setw(18) << std::left << miss_label << std::setw(20)
                  << std::fixed << std::setprecision(2) << std::right
                  << ((float)num_misses_ * 100 / (num_hits_ + num_misses_)) << "%"
                  << std::endl;
    }
}

void
caching_device_stats_t::print_child_stats(std::string prefix)
{
    if (num_child_hits_ != 0) {
        cache_stats_stream << prefix << std::setw(18) << std::left
                  << "Child hits:" << std::setw(20) << std::right << num_child_hits_
                  << std::endl;
        cache_stats_stream << prefix << std::setw(18) << std::left
                  << "Total miss rate:" << std::setw(20) << std::fixed
                  << std::setprecision(2) << std::right
                  << ((float)num_misses_ * 100 /
                      (num_hits_ + num_child_hits_ + num_misses_))
                  << "%" << std::endl;
    }
}

void
caching_device_stats_t::print_stats(std::string prefix)
{
    cache_stats_stream.imbue(std::locale("")); // Add commas, at least for my locale
    if (warmup_enabled_) {
        print_warmup(prefix);
    }
    print_counts(prefix);
    print_rates(prefix);
    print_child_stats(prefix);
    cache_stats_stream.imbue(std::locale("C")); // Reset to avoid affecting later prints.
}

std::size_t
caching_device_stats_t::init_histogram( histogram_t **hist, 
                                        const std::size_t size )
{
    const auto byte_size( sizeof( histogram_t ) * size ); 
    *hist = (histogram_t*) malloc( byte_size );
    std::memset( *hist, 0x0, byte_size );
    //return here is to assist in initialization, nothing more
    //the size comes from a vector size call, the ret simply makes
    //the assignment from the caller easier. 
    return( size );
}

void
caching_device_stats_t::inc_update( const boost::dynamic_bitset<> &dbs )
{
    /**
     * This should only be called if the cache line was allocated when 
     * the record flag was switched on
     */
    (this)->bytes_used      += dbs.count();
    (this)->bytes_requested += histogram_size;
    /** lets get the histogram initialized for the first time **/
    if( histogram == nullptr )
    {
        (this)->histogram_size = init_histogram( &histogram, dbs.size() );
    }
    /** generate histogram of where bytes were used in the line **/
    auto index( dbs.find_first() );
    const auto loop_end( boost::dynamic_bitset<>::npos );
    while( index != loop_end )
    {
        histogram[ index ]++;
        index = dbs.find_next( index );
    }
    return;
}

void
caching_device_stats_t::flush_update( const boost::dynamic_bitset<> &dbs )
{
    // This should only be called if the cache line was allocated when the record flag was switched on
    /** get the number of bytes used out of each line vs. total requested **/
    (this)->bytes_used      += dbs.count();
    (this)->bytes_requested += histogram_size;
    
    /** lets get the histogram initialized for the first time **/
    if( histogram == nullptr )
    {
        (this)->histogram_size = init_histogram( &histogram, dbs.size() );
    }
    /** we want to update histogram on every flush **/
    auto index( dbs.find_first() );
    const auto loop_end( boost::dynamic_bitset<>::npos );
    while( index != loop_end )
    {
        //increment count offset occupied
        histogram[ index ]++;
        //find next bit offset in bit vector
        index = dbs.find_next( index );
    }
    return;
}

void
caching_device_stats_t::reset()
{
    num_hits_at_reset_ = num_hits_;
    num_misses_at_reset_ = num_misses_;
    num_child_hits_at_reset_ = num_child_hits_;
    num_hits_ = 0;
    num_misses_ = 0;
    num_compulsory_misses_ = 0;
    num_child_hits_ = 0;
    num_inclusive_invalidates_ = 0;
    num_coherence_invalidates_ = 0;
}


void
caching_device_stats_t::write_histogram( const memref_t &mref, const size_t req_counter )
{
    const auto length( histogram_size );
    if( histogram_stream.is_open() )
    {
        histogram_stream << "{{" << req_counter << ", " <<  mref.data.pc << "},";
        histogram_stream << "{";
        for( std::remove_const< decltype( length ) >::type  index( 0 ); index < length; index++ )
        {
            histogram_stream << histogram[ index ];
            if( index < length - 1 )
            {
                histogram_stream << ", ";
            }
        }
        histogram_stream << "}, " << ((double)bytes_used / (double)bytes_requested) << ", " <<  bytes_used << ", " <<  bytes_requested << "},\n";
    }
    const auto byte_size( sizeof( histogram_t ) * histogram_size );
    bytes_requested = 0;
    bytes_used      = 0;
    std::memset( histogram, 0x0, byte_size );
    return;
}

void
caching_device_stats_t::invalidate(invalidation_type_t invalidation_type)
{
    if (invalidation_type == INVALIDATION_INCLUSIVE) {
        num_inclusive_invalidates_++;
    } else if (invalidation_type == INVALIDATION_COHERENCE) {
        num_coherence_invalidates_++;
    }
}
    

std::string
caching_device_stats_t::get_stats_dir()
{
    return( (this)->stats_dir );
}
