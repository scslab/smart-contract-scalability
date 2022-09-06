#pragma once

namespace scs {

template<typename T>
class GarbageCollector
{

    std::vector<std::unique_ptr<const T>> ptrs_to_gc;

  public:
    void deferred_delete(const T* ptr)
    {
        ptrs_to_gc.push_back(std::unique_ptr<const T>(ptr));
    }

    void post_block_clear() { ptrs_to_gc.clear(); }
};

template<typename... Ts>
class MultitypeGarbageCollector;

template<typename T, typename... U>
class MultitypeGarbageCollector<T, U...>
    : public MultitypeGarbageCollector<U...>
{
    GarbageCollector<T> gc;

  public:
    template<typename delete_t,
             std::enable_if<std::is_same<T, delete_t>::value>::type* = nullptr>
    void deferred_delete(const delete_t* ptr)
    {
        gc.deferred_delete(ptr);
    }

    template<typename delete_t,
             std::enable_if<!std::is_same<T, delete_t>::value>::type* = nullptr>
    void deferred_delete(const delete_t* ptr)
    {
        MultitypeGarbageCollector<U...>::deferred_delete(ptr);
    }

    void post_block_clear()
    {
        gc.post_block_clear();
        MultitypeGarbageCollector<U...>::post_block_clear();
    }
};

template<>
class MultitypeGarbageCollector<>
{
  public:
    void post_block_clear() {}
};

} // namespace scs
