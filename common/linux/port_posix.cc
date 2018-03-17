// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "linux/port_posix.h"

#include <cstdlib>
#include <stdio.h>
#include <string.h>

namespace port {

static void PthreadCall(const char* label, int result)
{
    if (result != 0)
    {
        fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
        abort();
    }
}

Mutex::Mutex()
{
    PthreadCall("init mutex", pthread_mutex_init(&mu_, NULL));
}

Mutex::~Mutex()
{
    PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_));
}

void Mutex::Lock()
{
    PthreadCall("lock", pthread_mutex_lock(&mu_));
}

void Mutex::Unlock()
{
    PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

RWMutex::RWMutex()
{
    PthreadCall("init mutex", pthread_rwlock_init(&mu_, NULL));
}

RWMutex::~RWMutex()
{
    PthreadCall("destroy mutex", pthread_rwlock_destroy(&mu_));
}

void RWMutex::ReadLock()
{
    PthreadCall("read lock", pthread_rwlock_rdlock(&mu_));
}

void RWMutex::WriteLock()
{
    PthreadCall("write lock", pthread_rwlock_wrlock(&mu_));
}

void RWMutex::ReadUnlock()
{
    PthreadCall("read unlock", pthread_rwlock_unlock(&mu_));
}

void RWMutex::WriteUnlock()
{
    PthreadCall("write unlock", pthread_rwlock_unlock(&mu_));
}

CondVar::CondVar(Mutex* mu)
    : mu_(mu)
{
    PthreadCall("init cv", pthread_cond_init(&cv_, NULL));
}

CondVar::~CondVar()
{
    PthreadCall("destroy cv", pthread_cond_destroy(&cv_));
}

void CondVar::Wait()
{
    PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
}

void CondVar::Signal()
{
    PthreadCall("signal", pthread_cond_signal(&cv_));
}

void CondVar::SignalAll()
{
    PthreadCall("broadcast", pthread_cond_broadcast(&cv_));
}

void InitOnce(OnceType* once, void (*initializer)())
{
    PthreadCall("once", pthread_once(once, initializer));
}

}  // namespace port
