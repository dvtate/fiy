#pragma once

#include <atomic>
#include <mutex>

/**
 * Mutex that supports multiple simultaneous reads but only a single write
 */
// TODO Maybe use std::shared_mutex instead?
struct RWMutex {
protected:
    std::mutex m_mtx;
    std::atomic<unsigned short> m_readers{0};

public:
    void read_lock() {
        m_mtx.lock();
        ++m_readers;
        m_mtx.unlock();
    }

    void read_unlock() {
        --m_readers;
    }

    /**
     * Upgrade read lock to write lock (optimization)
     */
    void read_to_write() {
        if (m_mtx.try_lock()) {
            --m_readers;
        } else {
            --m_readers;
            m_mtx.lock();
        }
        while (m_readers > 0); // spinlock is kinda ugly but eh
    }

    void write_lock() {
        m_mtx.lock();
        while (m_readers > 0); // spinlock is kinda ugly but eh
    }
    void write_unlock() {
        m_mtx.unlock();
    }

    /// Scoped write_lock + write_unlock
    struct LockForWrite {
        RWMutex& m_mtx;
        explicit LockForWrite(RWMutex& mtx): m_mtx(mtx) {
            mtx.write_lock();
        }
        ~LockForWrite() {
            m_mtx.write_unlock();
        }
    };

    /// Scoped read_lock + read_unlock
    struct LockForRead {
        RWMutex& m_mtx;
        explicit LockForRead(RWMutex& mtx): m_mtx(mtx) {
            mtx.read_lock();
        }
        ~LockForRead() {
            m_mtx.read_unlock();
        }
    };
};
