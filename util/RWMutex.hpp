#pragma once

#include <shared_mutex>

/**
 * Mutex that supports multiple simultaneous reads but only a single write
 * @deprecated use std::shared_mutex instead
 */
struct RWMutex {
    std::shared_mutex m_mtx;

    void read_lock() {
        m_mtx.lock_shared();
    }
    void read_unlock() {
        m_mtx.unlock_shared();
    }
    void write_lock() {
        m_mtx.lock();
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