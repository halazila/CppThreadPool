#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <future>

//using namespace std;

////简单的线程池
class SingleThreadPool
{
public:
	///任务
	using Task = std::function<void()>;
	SingleThreadPool(int size = 0) :bStoped{ false }
	{
		int const hardware_threads = std::thread::hardware_concurrency();
		int const num_thread = std::min(hardware_threads != 0 ? hardware_threads : 2, size < 1 ? 1 : size);
		nIdlThredNum = num_thread;
		for (int i = 0; i < num_thread; i++)
			vecThread.emplace_back(std::thread(&SingleThreadPool::do_work, this));
	}
	~SingleThreadPool()
	{
		bStoped.store(true);
		cvTask.notify_all();	///唤醒所有线程
		for (auto& thread : vecThread)
			if (thread.joinable())
				thread.join();
	}
	///线程运行逻辑
	void do_work()
	{
		while (!bStoped)
		{
			Task task;
			{
				std::unique_lock<std::mutex> lock{ mtxTask };
				cvTask.wait(lock,
					[this] {
						return this->bStoped.load() || !this->queTask.empty();
					});
				if (bStoped && queTask.empty())
					return;
				task = std::move(queTask.front());
				queTask.pop();
			}
			nIdlThredNum--;
			task();
			nIdlThredNum++;
		}
	}
	///提交任务
	template<typename FunctionType, typename... Args>
	std::future<typename std::result_of<FunctionType(Args...)>::type> submit(FunctionType&& f, Args&&... args)//模板函数使用&&，实现精确传递，前提是类型已经实现右值引用的赋值和拷贝构造
	{
		typedef typename std::result_of<FunctionType(Args...)>::type result_type;
		///使用decltype实现result_type
		///using result_type = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型

		if (bStoped.load())
			return std::future<result_type>();	///future is invalid

		auto task = std::make_shared<std::packaged_task<result_type()>>(		///packaged_task<T>,包装可调用目标（函数、lambda表达式、bind表达式或者其他函数对象），是的能异步调用它，其返回值或所抛出的异常被存储于future中
			std::bind(std::forward<FunctionType>(f), std::forward<Args>(args)...)///std::forwarn<T>,精确传递
			);
		std::future<result_type> future = task->get_future();
		{
			lock_guard<mutex> lock{ mtxTask };
			queTask.emplace([task]() {
				(*task)();
				}
			);
		}
		cvTask.notify_one();
		return future;
	}
	inline int idlCount() { return nIdlThredNum; }
private:
	///线程池
	std::vector<std::thread> vecThread;
	///任务队列
	std::queue<Task> queTask;
	///同步
	std::mutex mtxTask;
	///条件阻塞
	std::condition_variable cvTask;
	///是否关闭
	std::atomic<bool> bStoped;
	///空闲线程数
	std::atomic<int> nIdlThredNum;
};
