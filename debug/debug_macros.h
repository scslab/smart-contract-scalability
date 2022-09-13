#pragma once

#include "mtt/trie/debug_levels.h"

#define CONTRACT_DEBUG DEBUG_LEVEL_INFO
#define OBJECT_DEBUG DEBUG_LEVEL_INFO

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

