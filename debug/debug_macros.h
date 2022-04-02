#pragma once

#include "mtt/trie/debug_macros.h"

#define CONTRACT_DEBUG DEBUG_LEVEL_INFO

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
