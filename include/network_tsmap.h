#pragma once
#include "network_core.h"
#include <optional>

namespace network
{
    template<typename Key, typename Value, typename Hash = std::hash<Key>>
    class ThreadSafeMap
    {
    public:
        ThreadSafeMap() = default;
        ThreadSafeMap(const ThreadSafeMap<Key,Value,Hash>&) = delete;
        virtual ~ThreadSafeMap() { clear(); }

        void set(const Key& key, const Value& value)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            map_[key] = value;
        }

        std::optional<Value> get(const Key& key)
        {
            std::scoped_lock lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end())
                return it->second;
            return std::nullopt;
        }
        
        void erase(const Key& key)
        {
            std::scoped_lock lock(mutex_);
            map_.erase(key);
        }

        // Limpia todo el mapa
        void clear()
        {
            std::scoped_lock lock(mutex_);
            map_.clear();
        }

        // Número de elementos
        size_t size()
        {
            std::scoped_lock lock(mutex_);
            return map_.size();
        }

        // Comprueba si el mapa está vacío
        bool empty()
        {
            std::scoped_lock lock(mutex_);
            return map_.empty();
        }

    private:
        std::unordered_map<Key, Value, Hash> map_;
        mutable std::mutex mutex_;
    };
}