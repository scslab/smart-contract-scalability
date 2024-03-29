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

#pragma once

#include <new>
#ifndef HEAP_SIZE
#define HEAP_SIZE 64480
#endif

static size_t stack_base = 0;
static uint8_t buf[HEAP_SIZE];

// std::abort compiles to `asm ("unreachable")`

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-noreturn"

void __attribute__((__noreturn__))
abort()
{
  // the logic of when abort calls into env.abort() and when it just does unreachable is beyond me
  //std::abort();
  asm ("unreachable");
}

void __attribute__((__noreturn__))
unimplemented()
{
  abort();
}

#pragma GCC diagnostic pop

void* alloc(size_t count) noexcept
{
  size_t out = stack_base;
  stack_base += count;

  if (stack_base >= HEAP_SIZE)
  {
    abort();
  }
  return (void*) &buf[out];
}

void* malloc(size_t count) {
  return alloc(count);
}

void assert(bool x)
{
  if (!x)
  {
    abort();
  }
}
/**
 * Taken from
 * https://github.com/microsoft/mimalloc/blob/master/include/mimalloc-new-delete.h
 */

void operator delete(void* p) noexcept              {  };
void operator delete[](void* p) noexcept            {  };

void operator delete  (void* p, const std::nothrow_t&) noexcept { }
void operator delete[](void* p, const std::nothrow_t&) noexcept { }

void* operator new(std::size_t n) noexcept(false)   { return alloc(n); }
void* operator new[](std::size_t n) noexcept(false) { return alloc(n); }

void* operator new  (std::size_t n, const std::nothrow_t& tag) noexcept { (void)(tag); return alloc(n); }
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept { (void)(tag); return alloc(n); }

#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void operator delete  (void* p, std::size_t n) noexcept { abort(); };
void operator delete[](void* p, std::size_t n) noexcept { abort(); };
#endif

#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void operator delete  (void* p, std::align_val_t al) noexcept {  }
void operator delete[](void* p, std::align_val_t al) noexcept {  }
void operator delete  (void* p, std::size_t n, std::align_val_t al) noexcept {  };
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept {  };
void operator delete  (void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {  }
void operator delete[](void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {  }

void* operator new  (std::size_t n, std::align_val_t al) noexcept(false) { abort(); }
void* operator new[](std::size_t n, std::align_val_t al) noexcept(false) { abort(); }
void* operator new  (std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { abort(); }
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { abort(); }
#endif
