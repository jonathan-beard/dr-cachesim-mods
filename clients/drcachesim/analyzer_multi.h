/* **********************************************************
 * Copyright (c) 2016-2020 Google, Inc.  All rights reserved.
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

/* analyzer_multi: represent a memory trace analysis tool that can process
 * a trace from multiple inputs: a file, from a raw file, or over a pipe online.
 */

#ifndef _ANALYZER_MULTI_H_
#define _ANALYZER_MULTI_H_ 1

#include "analyzer.h"
#include "dr_api.h"
#include "dr_inject.h"

class analyzer_multi_t : public analyzer_t {
public:
    // Usage: errors encountered during the constructor will set a flag that should
    // be queried via operator!.
    analyzer_multi_t( sdt_probe_t *probe_list );
    virtual ~analyzer_multi_t();

protected:
    bool
    create_analysis_tools( const std::uint64_t  start_pc,
                           const std::uint64_t  stop_pc );
    void
    destroy_analysis_tools();

    static const int max_num_tools_ = 8;
};

#endif /* _ANALYZER_MULTI_H_ */
