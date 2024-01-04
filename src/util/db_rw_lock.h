#ifndef UTIL_DB_RW_LOCK_H_
#define UTIL_DB_RW_LOCK_H_

#include "global.h"
#include <stdint.h>
#include <time.h>


class DBrwLock
{
private:

    bool     volatile write_lock_;
    uint64_t volatile read_cnt_;

    // bool volatile latch_;

public:
    DBrwLock();
    ~DBrwLock();

    void GetReadLock();
    void ReleaseReadLock();

    void GetWriteLock();
    void ReleaseWriteLock();

};



#endif