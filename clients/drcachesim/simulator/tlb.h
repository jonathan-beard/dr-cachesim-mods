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

/* tlb: represents a single hardware TLB.
 */

#ifndef _TLB_H_
#define _TLB_H_ 1

#include "caching_device.h"
#include "tlb_entry.h"
#include "tlb_stats.h"
#include "tlb_device_settings.h"

class tlb_t : public caching_device_t {
public:
    void
    request(const memref_t &memref) override;

    // TODO i#4816: The addition of the pid as a lookup parameter beyond just the tag
    // needs to be imposed on the parent methods invalidate(), contains_tag(), and
    // propagate_eviction() by overriding them.
    
    virtual bool
    init(   tlb_device_settings_t &&settings, 
            caching_device_t *parent,
            caching_device_stats_t *stats, 
            prefetcher_t *prefetcher = nullptr,
            snoop_filter_t *snoop_filter_ = nullptr,
            const std::vector<caching_device_t *> &children = {} );

protected:
    void
    init_blocks( const std::size_t line_size ) override;

    // Optimization: remember last pid in addition to last tag
    memref_pid_t last_pid_;

};

#endif /* _TLB_H_ */
