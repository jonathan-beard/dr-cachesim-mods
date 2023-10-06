/**
 * proc_tools.hpp - 
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
#ifndef PROC_TOOLS_HPP
#define PROC_TOOLS_HPP  1
#include <string>
#include <sys/types.h>

struct proc_tools
{

static bool get_bin_path_for_pid( const pid_t pid, std::string &path );

}; 

#endif /* END PROC_TOOLS_HPP */
