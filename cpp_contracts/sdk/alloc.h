#pragma once

#include <new>
#ifndef HEAP_SIZE
#define HEAP_SIZE 64480
#endif

static size_t stack_base = 0;
static uint8_t buf[HEAP_SIZE];

void* alloc(size_t count) noexcept
{
  size_t out = stack_base;
  stack_base += count;
  return (void*) &buf[out];
}

// std::abort compiles to `asm ("unreachable")`

void 
abort()
{
  // the logic of when abort calls into env.abort() and when it just does unreachable is beyond me
  //std::abort();
  asm ("unreachable");
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
