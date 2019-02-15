#ifndef YQ_BASE_COUNTDOWNLATCH_H
#define YQ_BASE_COUNTDOWNLATCH_H
#include "noncopyable.h"
#include <mutex>
#include "condition.h"
namespace YQ{
class CountDownLatch : NonCopyable
{
public:
	explicit CountDownLatch(uint32_t count): count_(count) {

	};

	~CountDownLatch(){};

	void Wait(){
		std::unique_lock<std::mutex> lk(mutex_);
		if(count_ > 0){
			condition_.Wait(lk);	
		}
		
	}

	void CountDown(){
		std::unique_lock<std::mutex> lk(mutex_);
		count_--;
		if(count_ == 0){
			condition_.NotifyAll();
		}
	}

	uint32_t GetCount() const{
		std::unique_lock<std::mutex> lk(mutex_);
		return count_;
	}

private:
	YQ::Condition condition_;
	mutable std::mutex mutex_; // guard by mutex
	uint32_t count_; // guard by mutex
	};

typedef std::shared_ptr<YQ::CountDownLatch> CountDownLatchPtr;
}  // namespace YQ

#endif YQ_BASE_COUNTDOWNLATCH_H