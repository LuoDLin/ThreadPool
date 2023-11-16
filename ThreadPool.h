#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <future>
#include <functional>
#include <condition_variable>

class ThreadPool
{
public:
	explicit ThreadPool( size_t initThreadNum = 4, size_t maxTaskNum = 16 ) : initThreadNum_(initThreadNum),
	                                                                          maxTaskNum_(maxTaskNum) {}
	~ThreadPool() { stop(); }
	
	void stop();
	void start();
	
	template<class F, class ... Args>
	auto subTask( F &&f, Args &&...args ) -> std::future<decltype(f(args...))>;
private:
	void run();
private:
	bool stop_ = true;
	size_t maxTaskNum_ = 0;
	size_t initThreadNum_ = 0;
	
	std::vector<std::thread> threads_;
	
	std::mutex taskQueueMutex_;
	std::condition_variable notFull_, notEmpty_;
	std::queue<std::function<void()> > taskQueue_;
};

inline void ThreadPool::run()
{
	for ( ;; )
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(taskQueueMutex_);
			notEmpty_.wait(lock, [this]() { return stop_ || !taskQueue_.empty(); });
			
			if ( stop_ && taskQueue_.empty()) return;
			
			task = std::move(taskQueue_.front());
			taskQueue_.pop();
		}
		
		notFull_.notify_one();
		task();
	}
}

inline void ThreadPool::start()
{
	stop_ = false;
	for ( int i = 0; i < initThreadNum_; ++i )
	{
		threads_.emplace_back([this]() { run(); });
	}
}

template<class F, class ... Args>
inline auto ThreadPool::subTask( F &&f, Args &&...args ) -> std::future<decltype(f(args...))>
{
	using ReturnType = decltype(f(args...));
	
	auto task = std::make_shared<
			std::packaged_task<ReturnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	
	std::future<ReturnType> res = task->get_future();
	
	{
		std::unique_lock<std::mutex> lock(taskQueueMutex_);
		
		notFull_.wait(lock, [this]() { return stop_ || taskQueue_.size() < maxTaskNum_; });
		
		if ( stop_ ) throw std::runtime_error("ThreadPool has stopped!");
		
		taskQueue_.emplace([task]() { ( *task )(); });
	}
	
	notEmpty_.notify_one();
	return res;
}

inline void ThreadPool::stop()
{
	{
		std::unique_lock<std::mutex> lock(taskQueueMutex_);
		stop_ = true;
	}
	
	notEmpty_.notify_all();
	notFull_.notify_all();
	
	for ( auto &thread: threads_ )
	{
		thread.join();
	}
	
	threads_.clear();
	std::queue<std::function<void()> > emptyQueue;
	taskQueue_.swap(emptyQueue);
}
	
	

