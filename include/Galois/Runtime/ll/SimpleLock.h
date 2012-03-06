/** Simple Spin Lock -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in
 * irregular programs.
 *
 * Copyright (C) 2011, The University of Texas at Austin. All rights
 * reserved.  UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES
 * CONCERNING THIS SOFTWARE AND DOCUMENTATION, INCLUDING ANY
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR ANY PARTICULAR PURPOSE,
 * NON-INFRINGEMENT AND WARRANTIES OF PERFORMANCE, AND ANY WARRANTY
 * THAT MIGHT OTHERWISE ARISE FROM COURSE OF DEALING OR USAGE OF
 * TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH RESPECT TO
 * THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect,
 * direct or consequential damages or loss of profits, interruption of
 * business, or related expenses which may arise from use of Software
 * or Documentation, including but not limited to those resulting from
 * defects in Software and/or Documentation, or loss or inaccuracy of
 * data of any kind.  
 *
 * @section Description
 *
 * This contains the basic spinlock used in Galois.  We use a
 * test-and-test-and-set approach, with pause instructions on x86 and
 * compiler barriers on unlock.
 *
 * @author Andrew Lenharth <andrew@lenharth.org>
 */

#ifndef _SIMPLE_LOCK_H
#define _SIMPLE_LOCK_H

#include <cassert>

namespace GaloisRuntime {
namespace LL {

/// SimpleLock is a spinlock.  If the template parameter is
/// false, the lock is a noop.
template<bool isALock>
class SimpleLock;

template<>
class SimpleLock<true> {
  volatile mutable int _lock; //Allow locking a const
public:
  SimpleLock() : _lock(0) {
  }

  inline void lock() const {
    do {
      while (_lock != 0) {
#if defined(__i386__) || defined(__amd64__)
	asm volatile ( "pause");
#endif
      }
      if (try_lock())
	break;
    } while (true);
  }

  inline void unlock() const {
    assert(_lock);
    asm volatile ("":::"memory");
    _lock = 0;
    asm volatile ("":::"memory");
  }

  inline bool try_lock() const {
    if (_lock != 0)
      return false;
    int oldval = __sync_fetch_and_or(&_lock, 1);
    return !(oldval & 1);
  }
};

template<>
class SimpleLock<false> {
public:
  inline void lock() const {}
  inline void unlock() const {}
  inline bool try_lock() const { return true; }
};


void LockPairOrdered(SimpleLock<true>& L1, SimpleLock<true>& L2);
bool TryLockPairOrdered(SimpleLock<true>& L1, SimpleLock<true>& L2);
void UnLockPairOrdered(SimpleLock<true>& L1, SimpleLock<true>& L2);
void LockPairOrdered(SimpleLock<false>& L1, SimpleLock<false>& L2);
bool TryLockPairOrdered(SimpleLock<false>& L1, SimpleLock<false>& L2);
void UnLockPairOrdered(SimpleLock<false>& L1, SimpleLock<false>& L2);

}
}

#endif
