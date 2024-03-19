#include "storage/tuple.h"
#include <malloc.h>
#include <string.h>

Tuple::Tuple()
{
    tuple_data_    = nullptr;
    tuple_size_    = 0;
}

Tuple::Tuple(uint64_t tuple_size):tuple_size_(tuple_size)
{
    tuple_data_ = (TupleData)malloc(tuple_size * sizeof(char));
    memset(tuple_data_, '0', tuple_size);
    tuple_size_ = tuple_size;
}


Tuple::~Tuple()
{
    free(tuple_data_);
    tuple_size_ = 0;
}


void Tuple::CopyTupleData(Tuple* source_tuple, uint64_t size)
{
    memcpy(tuple_data_, source_tuple->tuple_data_, size);
}

uint64_t Tuple::size(){
    return tuple_size_;
}