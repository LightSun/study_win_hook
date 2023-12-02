#pragma once

#include <atomic>

namespace h7 {
class CppLock {
 public:
  void lock() {
    while (flag.test_and_set(std::memory_order_acquire)) {}
  }

  void unlock() {
    flag.clear(std::memory_order_release);
  }
 private:
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
};


//class GccLock {
// public:
//  void lock() {
//    while (!__sync_bool_compare_and_swap(&m_bool_val, false, true)) {}
//  }

//  void unlock() {
//    __sync_bool_compare_and_swap(&m_bool_val, true, false);
//  }
// private:
//  bool m_bool_val = false;
//};

//class SpinLock {
// public:
//  SpinLock() {
//    pthread_spin_init(&m_lock, PTHREAD_PROCESS_PRIVATE);
//  }

//  void lock() {
//    pthread_spin_lock(&m_lock);
//  }

//  void unlock() {
//    pthread_spin_unlock(&m_lock);
//  }
// private:
//  pthread_spinlock_t m_lock;
//};

}
