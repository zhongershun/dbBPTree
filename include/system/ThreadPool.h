#ifndef THREADPOOL_H
#define THREADPOOL_H
 
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <functional>
#include <condition_variable>

using namespace std;

typedef function<void()> functor;

class FunGroup;

class ThreadPool
{
public:
 
    ThreadPool(int n);
    ~ThreadPool();
    //启动线程池
    void Start();
    bool IsStop();
    //将可调用对象传入线程池等待执行
    void AddFun(functor fun,std::string group_str="");
 
private:
    //子线程入口
    void ThreadFun(int threadId);
 
    int threadNum_;//线程个数
    int sleepThreadNum_;//睡眠线程个数
    vector<std::thread> threads_;//子线程数组
    mutex mutex_;
    condition_variable cond_;
    queue<functor> fun_queue_;//可调用对象队列
    map<int,std::shared_ptr<FunGroup>> fun_groups_;//可调用对象组
    bool stop_;
};

class FunGroup
{
public:
    
    FunGroup(ThreadPool* poolPtr);
    ~FunGroup();
    int Size();
    void AddFun(const functor& fun);
    void Run(const functor& fun);
private:
    ThreadPool* poolPtr_;
    queue<functor> fun_queue_;
    mutex mutex_;
};
 
#endif