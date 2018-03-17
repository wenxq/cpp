//  Portions Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Borrowed from
// http://www.crazygaze.com/blog/2016/03/24/portable-c-timer-queue/
// Timer Queue
//
// License
//
// The source code in this article is licensed under the CC0 license, so feel
// free to copy, modify, share, do whatever you want with it.
// No attribution is required, but Ill be happy if you do.
// CC0 license

// The person who associated a work with this deed has dedicated the work to the
// public domain by waiving all of his or her rights to the work worldwide
// under copyright law, including all related and neighboring rights, to the
// extent allowed by law.  You can copy, modify, distribute and perform the
// work, even for commercial purposes, all without asking permission.

#ifndef _TIMER_QUEUE_H_
#define _TIMER_QUEUE_H_

#include "port.h"
#include "preprocessor.h"

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

// Allows execution of handlers at a specified time in the future
// Guarantees:
//  - All handlers are executed ONCE, even if cancelled (aborted parameter will
// be set to true)
//      - If TimerQueue is destroyed, it will cancel all handlers.
//  - Handlers are ALWAYS executed in the Timer Queue worker thread.
//  - Handlers execution order is NOT guaranteed
//
////////////////////////////////////////////////////////////////////////////////
// borrowed from
// http://www.crazygaze.com/blog/2016/03/24/portable-c-timer-queue/
class TimerQueue
{
public:
    TimerQueue()
      : finish_(false)
      , cancel_(false)
      , idcounter_(0)
      , thread_(&TimerQueue::Run, this)
    {}

    ~TimerQueue()
    {
        CancelAll();
        // Abusing the timer queue to trigger the shutdown.
        Add(0, std::bind(&TimerQueue::Done, this, std::placeholders::_1));
        thread_.join();
    }

    // Adds a new timer
    // \return
    //  Returns the ID of the new timer. You can use this ID to cancel the
    // timer
    uint64_t Add(int64_t milliseconds,
                 const std::function<std::pair<bool, int64_t>(bool)>& handler)
    {
        WorkItem item;
        Clock::time_point tp = Clock::now();
        item.end = tp + std::chrono::milliseconds(milliseconds);
        item.period = milliseconds;
        item.handler = handler;

        std::unique_lock<std::mutex> lk(lock_);
        uint64_t id = ++idcounter_;
        item.id = id;
        items_.push(item);

        // Something changed, so wake up timer thread
        checkWork_.notify_one();
        return id;
    }

    // Cancels the specified timer
    // \return
    //  1 if the timer was cancelled.
    //  0 if you were too late to cancel (or the timer ID was never valid to
    // start with)
    size_t Cancel(uint64_t id)
    {
        // Instead of removing the item from the container (thus breaking the
        // heap integrity), we set the item as having no handler, and put
        // that handler on a new item at the top for immediate execution
        // The timer thread will then ignore the original item, since it has no
        // handler.
        std::unique_lock<std::mutex> lk(lock_);
        BOOST_AUTO(&container, items_.GetContainer());
        for (BOOST_AUTO(item, container.begin()); item != container.end(); ++item)
        {
            if (item->id == id && item->handler)
            {
                WorkItem newItem;
                // Zero time, so it stays at the top for immediate execution
                newItem.end = Clock::time_point();
                newItem.id = 0;  // Means it is a canceled item
                // Move the handler from item to newitem (thus clearing item)
                newItem.handler = item->handler;
                items_.push(newItem);

                // Something changed, so wake up timer thread
                checkWork_.notify_one();
                return 1;
            }
        }
        return 0;
    }

    // Cancels all timers
    // \return
    //  The number of timers cancelled
    size_t CancelAll()
    {
        // Setting all "end" to 0 (for immediate execution) is ok,
        // since it maintains the heap integrity
        std::unique_lock<std::mutex> lk(lock_);
        cancel_ = true;
        BOOST_AUTO(&container, items_.GetContainer());
        for (BOOST_AUTO(item, container.begin()); item != container.end(); ++item)
        {
            if (item->id && item->handler)
            {
                item->end = Clock::time_point();
                item->id = 0;
            }
        }
        size_t ret = items_.size();

        checkWork_.notify_one();
        return ret;
    }

private:
    //typedef std::chrono::steady_clock Clock;
    typedef std::chrono::system_clock Clock;

    TimerQueue(const TimerQueue&);
    TimerQueue& operator=(const TimerQueue&);

    std::pair<bool, int64_t> Done(bool)
    {
        finish_ = true;
        return std::make_pair(false, 0);
    };

    void Run()
    {
        std::unique_lock<std::mutex> lk(lock_);
        while (!finish_)
        {
            BOOST_AUTO(end, CalcWaitTimeLock());
            if (end.first)
            {
                // Timers found, so wait until it expires (or something else
                // changes)
                checkWork_.wait_until(lk, end.second);
            }
            else
            {
                // No timers exist, so wait forever until something changes
                checkWork_.wait(lk);
            }

            // Check and execute as much work as possible, such as, all expired
            // timers
            CheckWork(&lk);
        }

        // If we are shutting down, we should not have any items left,
        // since the shutdown cancels all items
        assert(items_.size() == 0);
    }

    std::pair<bool, Clock::time_point> CalcWaitTimeLock()
    {
        while (items_.size())
        {
            if (items_.top().handler)
            {
                // Item present, so return the new wait time
                return std::make_pair(true, items_.top().end);
            }
            else
            {
                // Discard empty handlers (they were cancelled)
                items_.pop();
            }
        }

        // No items found, so return no wait time (causes the thread to wait
        // indefinitely)
        return std::make_pair(false, Clock::time_point());
    }

    void CheckWork(std::unique_lock<std::mutex>* lk)
    {
        while (items_.size() && items_.top().end <= Clock::now())
        {
            WorkItem item(items_.top());
            items_.pop();

            if (item.handler)
            {
                (*lk).unlock();
                BOOST_AUTO(reschedule_pair, item.handler(item.id == 0));
                (*lk).lock();
                if (!cancel_ && reschedule_pair.first)
                {
                    int64_t new_period = (reschedule_pair.second == -1)
                                         ? item.period
                                         : reschedule_pair.second;

                    item.period = new_period;
                    item.end = Clock::now() + std::chrono::milliseconds(new_period);
                    items_.push(item);
                }
            }
        }
    }

    struct WorkItem
    {
        std::function<std::pair<bool, int64_t>(bool)> handler;
        Clock::time_point end;
        int64_t  period;
        uint64_t id;  // id==0 means it was cancelled
        bool operator>(const WorkItem& other) const
        {
            return end > other.end;
        }
    };

private:
    // Inheriting from priority_queue, so we can access the internal container
    class Queue : public std::priority_queue<WorkItem, std::vector<WorkItem>, std::greater<WorkItem> >
    {
    public:
        std::vector<WorkItem>& GetContainer()
        {
            return this->c;
        }
    };

private:
    bool finish_;
    bool cancel_;
    uint64_t idcounter_;

    std::mutex lock_;
    std::condition_variable checkWork_;
    Queue items_;
    port::Thread thread_;
};

#endif // _TIMER_QUEUE_H_

