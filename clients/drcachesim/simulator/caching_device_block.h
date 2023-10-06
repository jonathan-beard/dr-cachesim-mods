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

/* caching_device_block: represents a unit block of a caching device.
 */

#ifndef _CACHING_DEVICE_BLOCK_H_
#define _CACHING_DEVICE_BLOCK_H_ 1

#include <boost/dynamic_bitset.hpp>
#include <bitset>
#include <cinttypes>
#include <cstdint>
#include <limits>
#include "memref.h"

// Assuming a block of a caching device represents a memory space of at least 4-byte,
// e.g., a CPU cache line or a virtual/physical page, we can use special value
// that cannot be computed from valid address as special tag for
// block status.
static const addr_t TAG_INVALID = (addr_t)-1; // block is invalid

class caching_device_block_t {
public:
    // Initializing counter to 0 is just to be safe and to make it easier to write new
    // replacement algorithms without errors (and we expect negligible perf cost), as
    // we expect any use of counter to only occur *after* a valid tag is put in place,
    // where for the current replacement code we also set the counter at that time.
    caching_device_block_t()          = default;

    // Destructor must be virtual and default is not.
    virtual ~caching_device_block_t() = default;
    
    virtual void update_utilization( const memref_t &in, const size_t offset, const bool record )
    {
        valid = true;
        //event_flags[0] = event_flags[0] || record; //record if we are in ROI
        return;
    }

    virtual void reset()
    { 
        valid   = false;
        counter_ = 0;
        return;
    }

    virtual boost::dynamic_bitset<> getBits()
    {
        boost::dynamic_bitset<> dummy( 0 );
        return( dummy );
    }

    addr_t tag_        = TAG_INVALID;

    int     counter_   = 0; // for use by replacement policies
    bool    valid      = false;

    //currently just marking whether this block is touched in ROI
    std::bitset<1> event_flags  = {1};
};

#endif /* _CACHING_DEVICE_BLOCK_H_ */
