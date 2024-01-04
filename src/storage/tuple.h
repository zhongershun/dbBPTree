#ifndef STORAGE_TUPLE_H_
#define STORAGE_TUPLE_H_

#include "config.h"

class Tuple{
    private:

public:
    Tuple();
    Tuple(uint64_t tuple_size);
    ~Tuple();
    
    TupleData GetTupleData() { return tuple_data_; }
    // TupleHeader* GetTupleHeader() { return tuple_header_; }

    void CopyTupleData(Tuple* source_tuple, uint64_t size);

    uint64_t size();
    TupleData tuple_data_;
    uint64_t tuple_size_;
};


#endif