#include "RCThreadPool.h"


////task
RCRunnable::RCRunnable(TaskType nType) :type(nType)
{
}

RCRunnable::~RCRunnable()
{
}

void RCRunnable::run()
{
}

RCThreadPool* RCThreadPool::m_globalThreadPool = new RCThreadPool(5, 5);//4种task，4个线程
////线程池
RCThreadPool::RCThreadPool(int taskTypes, int size) :bStopped(false)
{

	int const hardware_threads = std::thread::hardware_concurrency();
	int const num_thread = std::min(std::min(hardware_threads != 0 ? hardware_threads : 2, size < 1 ? 1 : size), taskTypes);

	nIdlThredNum = nThreadNum = num_thread;

	std::vector<int> vecType(num_thread, 0);
	///当任务种类大于线程数时，暂定将多出来的任务按照线程创建的顺序叠加到线程中
	for (int i = 0; i < taskTypes; i++)
	{
		vecType[i % num_thread] |= 1 << i;
	}
	////初始化mutex,condition_variable,task list
	for (int i = 0; i < std::min(taskTypes, num_thread); i++)
	{
		mapMutex[vecType[i]];
		mapCvTask[vecType[i]];
		mapListTask[vecType[i]];
	}
	for (int i = taskTypes; i < nThreadNum; i++)///线程数多于任务类型，则剩余的全部设置为处理otherTask
	{
		vecType[i] |= RCRunnable::otherTask;
	}
	for (int i = 0; i < num_thread; i++)
		vecThread.emplace_back(std::thread(&RCThreadPool::doWork, this, vecType[i]));
}

RCThreadPool::~RCThreadPool()
{
	bStopped.store(true);
	for (auto it = mapCvTask.begin(); it != mapCvTask.end(); it++)
	{
		auto& mcv = it->second;
		mcv.notify_all();
	}
	for (auto& thr : vecThread)
		if (thr.joinable())
			thr.join();
}

RCThreadPool* RCThreadPool::globalThreadPool()
{
	return m_globalThreadPool;
}

void RCThreadPool::submit(RCRunnable* task)
{
	if (bStopped.load())
	{
		return;
	}
	int type = task->type;
	for (auto it = mapMutex.begin(); it != mapMutex.end(); it++)
	{
		if (type & (it->first))
		{
			type = it->first;
			{
				std::lock_guard<decltype(mapMutex)::mapped_type> lock(mapMutex[type]);
				mapListTask[type].push_back(task);
			}
			mapCvTask[type].notify_all();
			break;
		}
	}
	return;
}

void RCThreadPool::clear()
{
	for (auto it = mapMutex.begin(); it != mapMutex.end(); it++)
	{
		auto& mtx = it->second;
		std::lock_guard<decltype(mapMutex)::mapped_type> lock(mtx);
		qDeleteAll(mapListTask[it->first]);
	}
}

void RCThreadPool::waitForDone(int mseconds)
{
	if (mseconds > 0)
	{
		if (nIdlThredNum < nThreadNum)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(mseconds));
		}
	}
	if (mseconds < 0)
	{
		while (nIdlThredNum.load() < nThreadNum)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

void RCThreadPool::doWork(int workType)
{
	while (!bStopped)
	{
		RCRunnable* task = nullptr;
		auto& mtx = mapMutex[workType];	///获取对应mutex
		auto& mcv = mapCvTask[workType];///获取对应condition_variable
		{
			std::unique_lock<decltype(mapMutex)::mapped_type> lock(mtx);
			mcv.wait(lock, [workType, this] {
				return this->bStopped.load() || !this->mapListTask[workType].empty();
				});
			if (bStopped && mapListTask[workType].empty())
				return;
			auto it = mapListTask[workType].begin();
			task = *it;
			mapListTask[workType].erase(it);
		}

		if (task) {
			nIdlThredNum--;
			task->run();
			nIdlThredNum++;
			delete task;
		}
	}
}
