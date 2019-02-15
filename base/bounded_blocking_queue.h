// Copyright 2019 YQ

#ifndef YQBASE_BOUNDED_BLOCKING_QUEUE_H
#define YQBASE_BOUNDED_BLOCKING_QUEUE_H

#include <memory>
#include <string>
#include <deque>
#include <assert.h>
#include <mutex>

#include "noncopyable.h"
#include "condition.h"
#include "ring_buffer.h"

namespace YQ {

template  <typename T>
class BoundedBlockingQueue : NonCopyable {
public:
    BoundedBlockingQueue(size_t max_size) {
		ring_ptr_ = std::make_shared<RingBuffer<T>>(max_size);
	}

	void Put(const T& x) {
		std::unique_lock<std::mutex> lk(mutex_);
		while (ring_ptr_->Full()) {
			not_full_.Wait(lk);
		}
		assert(!ring_ptr_->Full());
		ring_ptr_->Put(x);
		not_empty_.Notify();
		}

	T Take() {
		std::unique_lock<std::mutex> lk(mutex_);
		while (ring_ptr_->Empty()) {
			not_empty_.Wait(lk);
		}
		assert(!ring_ptr_->Empty());
		T x = ring_ptr_->Take();
		not_full_.Notify();
		return std::move(x);
		}

	void NotEmptyNotifyAll() {
		std::unique_lock<std::mutex> lk(mutex_);
		not_empty_.Notify();
	}

	bool Empty() const
		{
		std::unique_lock<std::mutex> lk(mutex_);
		return queue_.Empty();
		}

	bool Full() const
		{
		std::unique_lock<std::mutex> lk(mutex_);
		return queue_.Full();
		}

	size_t Size() const
		{
		std::unique_lock<std::mutex> lk(mutex_);
		return queue_.Size();
		}

	size_t Capacity() const
		{
		std::unique_lock<std::mutex> lk(mutex_);
		return queue_.Capacity();
		}

private:
    std::mutex mutex_;
    Condition not_empty_;
	Condition not_full_;
	YQ::RingBufferPtr<T> ring_ptr_;

};

template <typename T>
using BoundedBlockingQueuePtr = std::shared_ptr<BoundedBlockingQueue<T>>;

}  // namespace YQ

#endif  // YQBASE_BOUNDED_BLOCKING_QUEUE_H
