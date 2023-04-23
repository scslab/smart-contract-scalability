#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mtt/common/debug_levels.h"

#define CONTRACT_DEBUG DEBUG_LEVEL_NONE
#define OBJECT_DEBUG DEBUG_LEVEL_NONE
#define EXECUTION_TRACE DEBUG_LEVEL_NONE

#define LOG(s, ...) std::printf((std::string("%-45s") + s + "\n").c_str(), (std::string(__FILE__) + "." + std::to_string(__LINE__) + ":").c_str() __VA_OPT__(,) __VA_ARGS__)

#if CONTRACT_DEBUG <= DEBUG_LEVEL_ERROR
#define CONTRACT_ERROR(s, ...) LOG(s, __VA_ARGS__)
#define CONTRACT_ERROR_F(s) s
#else
#define CONTRACT_ERROR(s, ...) (void)0
#define CONTRACT_ERROR_F(s) (void)0
#endif

#if CONTRACT_DEBUG <= DEBUG_LEVEL_INFO
#define CONTRACT_INFO(s, ...) LOG(s, __VA_ARGS__)
#define CONTRACT_INFO_F(s) s
#else
#define CONTRACT_INFO(s, ...) (void)0
#define CONTRACT_INFO_F(s) (void)0
#endif

#if OBJECT_DEBUG <= DEBUG_LEVEL_INFO
#define OBJECT_INFO(s, ...) LOG(s, __VA_ARGS__)
#define OBJECT_INFO_F(s) s
#else
#define OBJECT_INFO(s, ...) (void)0
#define OBJECT_INFO_F(s) (void)0
#endif

#if EXECUTION_TRACE <= DEBUG_LEVEL_INFO
#define EXEC_TRACE(s, ...) LOG(s, __VA_ARGS__)
#define EXEC_TRACE_F(s) s
#else
#define EXEC_TRACE(s, ...) (void)0
#define EXEC_TRACE_F(s) (void)0
#endif

