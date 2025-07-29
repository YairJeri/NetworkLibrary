#pragma once
#include "network_core.h"

namespace network {
    template <typename T>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue() = default;
        ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
        virtual ~ThreadSafeQueue() { clear(); }

        const T& front()
        {
            std::scoped_lock lock(mutex_);
            return queue_.front();
        }

        const T& back()
        {
            std::scoped_lock lock(mutex_);
            return queue_.back();
        }

        void push_back(const T& item)
        {
            std::scoped_lock lock(mutex_);
            queue_.emplace_back(std::move(item));
            cv_.notify_one();
        }

        void push_front(const T& item)
        {
            std::scoped_lock lock(mutex_);
            queue_.emplace_front(std::move(item));
            cv_.notify_one();
        }


        T pop_front()
        {
            std::scoped_lock lock(mutex_);
            auto t = std::move(queue_.front());
            queue_.pop_front();
            return t;
        }

        T pop_back()
        {
            std::scoped_lock lock(mutex_);
            auto t = std::move(queue_.back());
            queue_.pop_back();
            return t;
        }

        void clear()
        {
            std::scoped_lock lock(mutex_);
            queue_.clear();
        }

        void wait()
        {
            std::unique_lock<std::mutex> ul(mutex_);
            cv_.wait(ul, [&](){return !queue_.empty();});
        }

        bool empty()
        {
            std::scoped_lock lock(mutex_);
            return queue_.empty();
        }

        size_t count()
        {
            std::scoped_lock lock(mutex_);
            return queue_.size();
        }

    protected:
        std::mutex mutex_;
        std::deque<T> queue_;
        std::condition_variable cv_;
    };
}