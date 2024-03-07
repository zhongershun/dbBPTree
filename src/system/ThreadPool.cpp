#include "ThreadPool.h"
#include <iostream>
 
using namespace std;

ThreadPool::ThreadPool(int n)
    :threadNum_(std::max(n,1)),
     sleepThreadNum_(0),
     stop_(false)
{
 
}
 
ThreadPool::~ThreadPool()
{
    {
        //唤醒子线程并退出
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
        cond_.notify_all();
    }
    for(auto& thread : threads_)
    {
        thread.join();
    }
    // cout<<"all thread exit\n";
}
 
void ThreadPool::Start()
{
    for(int i=0; i<threadNum_; i++)
    {
        threads_.emplace_back([this,i](){ThreadFun(i);});
    }
}
 
bool ThreadPool::IsStop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return stop_;
}
 
/*
fun:待处理的调用对象
group_str:该调用对象从属的队列组名，若组名为空，表示可并行执行，
若不为空，只能在队列组内串行执行
*/
void ThreadPool::AddFun(functor fun,std::string group_str)
{
    int sleepNum = 0;
    if(group_str.empty())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        fun_queue_.push(fun);
        sleepNum = sleepThreadNum_;
    }
    else
    {
        std::hash<std::string> hash_fn;
        int group_id = hash_fn(group_str);
        if(fun_groups_.count(group_id) == 0)
        {
            //创建队列组
            fun_groups_.insert(std::make_pair(group_id,std::make_shared<FunGroup>(this)));
        }
        fun_groups_[group_id]->AddFun(fun);
        sleepNum = sleepThreadNum_;
    }
    if(sleepNum > 0)
    {
        //唤醒睡眠中的线程，赶紧工作
        cond_.notify_one();
    }
}
 
void ThreadPool::ThreadFun(int threadId)
{
    while (!IsStop())
    {
        int step = 0;
        functor fun;
        int funNum = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            funNum = fun_queue_.size();
        }
 
        if(funNum)
        {
            //可调用对象队列不为空，赶紧从队列头取出一个元素处理掉
            int sleepNum = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                fun = fun_queue_.front();
                fun_queue_.pop();
                sleepNum = sleepThreadNum_;
            }
 
            --funNum;
            if(funNum>0 && sleepThreadNum_>funNum)
            {
                /*取出第一个元素后，队列仍然不为空，但是睡眠的线程数大于当前队列的元素数，
                再多唤醒一个线程一起干活*/
                cond_.notify_one();
            }
            else if(funNum>0 && sleepThreadNum_<=funNum)
            {
                /*取出第一个元素后，队列仍然不为空，而且睡眠的线程数小于当前队列的元素数，
                赶紧唤醒所有线程一起干活*/
                cond_.notify_all();
            }
            step = 1;
        }
 
        if(step == 0)
        {
            //没有可处理对象，进入睡眠状态
            // cout<<"thread: "<<threadId<<" sleeping\n";
            std::unique_lock<std::mutex> lock(mutex_);
            ++sleepThreadNum_;
            cond_.wait(lock,[this](){return (!fun_queue_.empty() || stop_);});
            //当可调用对象队列不为空，线程被唤醒开始干活，或线程被通知退出
            --sleepThreadNum_;
        }
        else
        {
            // std::cout<<"thread: "<<threadId<<" working\n";
            fun();
        }
    }
}

FunGroup::FunGroup(ThreadPool* poolPtr)
    :poolPtr_(poolPtr)
{
 
}
 
FunGroup::~FunGroup()
{
 
}
 
int FunGroup::Size()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return fun_queue_.size();
}
 
void FunGroup::AddFun(const functor& fun)
{
    if(Size() == 0)
    {
        poolPtr_->AddFun(std::bind(&FunGroup::Run,this,fun));
    }
    else
    {
        //当队列不为空，只需将可调用对象放在队列后面
        std::unique_lock<std::mutex> lock(mutex_);
        fun_queue_.push(fun);
    }
}
 
void FunGroup::Run(const functor& fun)
{
    //执行当前调用对象
    fun();
    if(Size() > 0)
    {
        //若队列不为空，继续执行下一个可调用对象
        functor nextFun;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            nextFun = fun_queue_.front();
            fun_queue_.pop();
        }
        poolPtr_->AddFun(std::bind(&FunGroup::Run,this,nextFun));
    }
}