/**
 * pagemap.hpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug 20 14:47:49 2018
 * 
 * Copyright 2018 Jonathan Beard
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
#ifndef _PAGEMAP_HPP_
#define _PAGEMAP_HPP_  1
#include <unistd.h>
#include <sys/types.h>
#include <cstdio>

#include <cstdint>
#include <cstring>
#include <sys/stat.h>
#include <iostream>

/**
 * NOTE: a bit odd, I know, however, each type is just
 * a cast away so this seems to make it easier to grok
 * the entry.
 */

/** use this one before we know if it's out or in **/
struct pagemap_entry
{
    pagemap_entry() = default;


    union{
        struct
        {
            std::uint64_t   pfn         : 55,
                            sft_drty    : 1,
                            exclusive   : 1,
                            zero        : 4,
                            fp_anon     : 1,
                            reserved    : 2;
        }swapped_in;

        struct
        {
            std::uint64_t   swap_type   : 5,
                            swap_offset : 50,
                            reserved    : 9;
        }swapped_out;

        struct
        {
            std::uint64_t   reserved : 61,
                            fp_anon  : 1        /** page is file-page or shared-anon (since 3.5) **/,
                            swapped  : 1        /** page swapped **/,
                            present  : 1        /** page present **/;
        }initial;
    };
    /** this will take care of default construction / init to zero **/
    std::uint64_t   entire_entry = 0;
};


#if 0
int main()
{
    auto pid( getpid() );
    const std::string path = "/proc/" + std::to_string( pid ) + "/pagemap";
    FILE *fp = fopen( path.c_str(), "r" );
    if( fp == nullptr )
    {
        perror( "Couldn't open pagemap!!" );
    }
    entries.num  = (1 << 30);
    entries.data = (pagemap_entry*)malloc( sizeof( pagemap_entry ) * entries.num );
    auto count = fread( (void*)entries.data, sizeof( pagemap_entry ), entries.num, fp );
    std::cout << count << "\n"; 
    fclose( fp );
    free( entries.data );
}
#endif /** test main **/
#endif /* END _PAGEMAP_HPP_ */
