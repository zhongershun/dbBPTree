#include "db_rw_lock.h"


DBrwLock::DBrwLock()
{
    write_lock_ = false;
    read_cnt_   = 0;
}

DBrwLock::~DBrwLock()
{
}


void DBrwLock::GetReadLock()
{
    while (true)
    {
        while (!ATOM_CAS(write_lock_, false, false)) {}
        
        ATOM_ADD(read_cnt_, 1);

        if (!ATOM_CAS(write_lock_, false, false))
        {
            ATOM_SUB(read_cnt_, 1);
            continue;
        }
        else
        {
            break;
        }
    }
}

void DBrwLock::ReleaseReadLock()
{
    ATOM_SUB(read_cnt_, 1);
}

void DBrwLock::GetWriteLock()
{
    while (!ATOM_CAS(write_lock_, false, true)) {}
    while (!ATOM_CAS(read_cnt_, 0, 0)) {}
}

void DBrwLock::ReleaseWriteLock()
{
    ATOM_CAS(write_lock_, true, false);
}

