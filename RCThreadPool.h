#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <QtCore>

class RCRunnable
{
public:
	enum TaskType
	{
		emptyTask = 0,
		accountTask = 1,
		orderTask = 2,
		tradeTask = 2,		///trade task share same task type
		positionTask = 4,
		otherTask = 8,
		otpsearchTask = 16,
	};
	RCRunnable(TaskType nType = emptyTask);
	virtual ~RCRunnable();
	virtual void run();
public:
	TaskType type;
};



class RCThreadPool
{
private:
	static RCThreadPool* m_globalThreadPool;
	RCThreadPool(int taskTypes, int size = 1);
	~RCThreadPool();
public:
	static RCThreadPool* globalThreadPool();
	void submit(RCRunnable* task);
	void clear();
	void waitForDone(int mseconds = -1);
	inline int idlCount() { return nIdlThredNum; }
private:
	void doWork(int workType);
private:
	///线程池
	std::vector<std::thread> vecThread;
	///任务列表，支持常数时间插入和删除					//每种task对应一个任务列表，避免任务不均匀分布下线程空转
	//std::list<RCRunnable*> listTask;					
	std::map<int, std::list<RCRunnable*>> mapListTask;	
	///互斥量										
	//std::mutex mtxTask;								
	std::map<int, std::mutex> mapMutex;					
	///条件阻塞									
	//std::condition_variable cvTask;				
	std::map<int, std::condition_variable> mapCvTask;
	///是否关闭
	std::atomic<bool> bStopped;
	///空闲线程数
	std::atomic<int> nIdlThredNum;
	///总线程数
	int nThreadNum;
};
