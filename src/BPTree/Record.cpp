#include "Record.h"

size_t Record::size(){
    return 8+8+value->size();
}

void RecordBucket::push_back(Record record){
    Records *records;
    if(bucket_.records==NULL){
        records = new Records;
        RecordBucketInfo info;
        info.records = records;
        info.length = 4;
        bucket_ = info;
        length_+= info.length;
    }else{
        records = bucket_.records;
    }
    records->push_back(record);
    length_ += record.size();
    size_++;
}

IndexKey RecordBucket::split(RecordBucket &other){
    assert(other.size()==0);
    assert(size_);

    Records *src = bucket_.records;
    Records *dst = new Records();

    size_t n = src->size()/2;
    dst->add(*src,n);

    src->removeRange(n,src->size()-n);

    bucket_.length = 4;
    for (int i = 0; i < src->size(); i++)
    {
        bucket_.length += (*src)[i].size();
    }
    length_ = bucket_.length;
    size_ = src->size();
    other.set_bucket(dst);

    assert(other.bucket_.records);
    assert(other.bucket_.records->size());
    return other.bucket_.records->front().key;
    
}

void RecordBucket::swap(RecordBucket &other){
    std::swap(bucket_,other.bucket_);
    std::swap(length_,other.length_);
    std::swap(size_,other.size_);
}