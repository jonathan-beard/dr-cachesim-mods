/**
 * proc_tools.cpp - 
 * @author: Jonathan Beard
 * @version: Wed Mar 30 09:24:48 2022
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
#include "proc_tools.hpp"
#include <unistd.h>
#include <cstdio>
#include <cstring>

bool 
proc_tools::get_bin_path_for_pid( const pid_t pid, std::string &path )
{
    constexpr static auto buffer_length = 1024;
    
    /** for our proc string **/
    char buffer[ buffer_length ];
    std::memset( buffer, '\0', buffer_length );
    /**
     * FIXME - this won't work for non-POSIX/linux based operating
     * systems. 
     */
    std::snprintf( buffer, buffer_length, "/proc/%d/exe", pid );
    
    /** for symlink string **/
    char sym_buffer[ buffer_length ];
    const auto link_size = readlink(  buffer, sym_buffer, buffer_length );
    if( link_size == -1 )
    {
        std::perror( "failed to get symlink" );
        return( false );
    }
    else if( link_size >= buffer_length )
    {
        fprintf( stderr, 
                 "truncation has occured on symlink path, increase buffer size in program\n" );
        return( false );
    }
    sym_buffer[ link_size ] = '\0';
    //ugly but not perf critical
    path = std::string( sym_buffer );
    return( true );
}
