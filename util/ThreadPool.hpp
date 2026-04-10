//
// Created by tate on 4/10/26.
//

#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>

template<class T>
struct ThreadPool {
    ThreadPool(
        std::function<void(T)> handler,
        const int threads
    ) {
        m_handler = std::move(handler);
        m_threads.reserve(threads);
        for (int i = 0; i < threads; ++i)
            m_threads.emplace_back([this]() {
                while (true)
                    m_handler(m_queue.pop());
            });
    }

    void push(T const& val) {
        m_queue.push(val);
    }

    template<class ...Args>
    void emplace(Args&&... args) {
        m_queue.emplace(std::forward<Args>(args)...);
    }

protected:

    /**
     * Thread safe task queue
     * @tparam T task type
     */
    class Queue {
    public:
        Queue() = default;

        void push(T const& val) {
            std::lock_guard queue_lock{m_mtx};
            m_queue.push(val);
            m_cv.notify_one();
        }

        template<class ...Args>
        void emplace(Args&&... args) {
            std::lock_guard queue_lock{m_mtx};
            m_queue.emplace(std::forward<Args>(args)...);
            m_cv.notify_one();
        }

        T pop() {
            std::unique_lock queue_lock{m_mtx};
            m_cv.wait(queue_lock, [&]{ return !m_queue.empty(); });
            T ret = std::move(m_queue.front());
            m_queue.pop();
            return ret;
        }

    private:
        std::queue<T> m_queue;
        std::condition_variable m_cv;
        std::mutex m_mtx;
    };

    std::function<void(T)> m_handler;
    std::vector<std::thread> m_threads;
    Queue m_queue;
};
