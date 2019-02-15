// Copyright 2019 YQ

#ifndef YQBASE_RING_BUFF_H
#define YQBASE_RING_BUFF_H

#include <memory>
#include <string>
#include <deque>
#include <assert.h>
#include <mutex>

#include "noncopyable.h"
#include "condition.h"

namespace YQ {

	template <typename T>
	class RingBuffer {
	public:
		RingBuffer(size_t max_size) :max_size_(max_size) {
			ring_ = std::unique_ptr<T[]>(new T[max_size_]);
			}

		~RingBuffer() {}

	public:
		void Put(T t) {
			ring_[write_index_] = t;
			write_index_ = Next(write_index_);
			}

		void Put(T&& t) {
			ring_[write_index_] = std::move(t);
			write_index_ = Next(write_index_);
			}

		T Take() {
			T t = ring_[read_index_];
			read_index_ = Next(read_index_);
			return std::move(t);
			}

		size_t Next(size_t index) {
			return  (index + 1) % max_size_;
			}

		bool Full() {
			return Next(write_index_) == read_index_;
			}

		bool Empty() {
			return read_index_ == write_index_;
			}

		size_t Size() {
			if (write_index_ >= read_index_) {
				return max_size_ - write_index_ + read_index_;
				}
			else {
				return read_index_ - write_index_;
				}
			}

		size_t Capacity() {
			return max_size_;
			}

	private:
		size_t max_size_;
		size_t write_index_ = { 0 };
		size_t read_index_ = { 0 };
		std::unique_ptr<T[]> ring_;
		};

	template <typename T>
	using RingBufferPtr = std::shared_ptr<RingBuffer<T>>;


}  // namespace YQ

#endif  // YQBASE_RING_BUFF_H
