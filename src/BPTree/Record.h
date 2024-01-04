#ifndef BPTREE_RECORD_H_
#define BPTREE_RECORD_H_

#include <vector>
#include <stdint.h>
#include <cstddef>
#include <cassert>
#include "tuple.h"
// #include "config.h"
#include "List.hpp"
#include "db_rw_lock.h"

class Record {
public:
    Record() {};
    Record(IndexKey k, Tuple* v){
        key = k;
        value = v;
    };
    size_t size();

    IndexKey key;
    Tuple* value;
};

inline bool operator==(const Record& a, const Record& b){
        return a.key==b.key;
    }

inline bool operator!=(const Record& a, const Record& b){
        return a.key!=b.key;
    }

inline bool operator<(const Record& a, const Record& b){
        return a.key<b.key;
    }

inline bool operator>(const Record& a, const Record& b){
        return a.key>b.key;
    }

typedef List<Record> Records;

class RecordBucket {
public:
    class Iterator{
    public:
        Iterator(RecordBucket* container):
        container_(container),
        record_idx_(0){};

        bool valid(){
            if(record_idx_ == container_->size()){
                return false;
            }
            return true;
        };

        void next(){
            record_idx_++;
        }

        Record& record(){
            Records *records = container_->bucket_.records;
            assert(records&&record_idx_<records->size());
            return (*records)[record_idx_];
            // assert(record_idx_<)
        }

    private:
        RecordBucket* container_;
        size_t record_idx_;
    };

    RecordBucket():
    length_(0),size_(0){bucket_.records = new Records;}

    ~RecordBucket(){ //有问题
        // delete bucket_.records;
    };
    
    size_t bucket_length(){ // 实际返回存储的records占据的大小
        return bucket_.length;
    }

    Records* records(int index){
        return bucket_.records;
    }

    void set_bucket(Records* records){
        // assert(bucket_.records==NULL);
        bucket_.records=records;
        bucket_.length = 4;
        for (size_t i = 0; i < records->size(); i++)
        {
            bucket_.length+=(*records)[i].size();
        }
        length_+=bucket_.length;
        size_+=bucket_.records->size();
    }

    Iterator get_iterator(){return Iterator(this);}
    
    void push_back(Record record);

    size_t length(){return length_;}
    size_t size(){return size_;}
    Record& operator[](size_t idx){
        assert(idx<size_);
        return (*(bucket_.records))[idx];
    }

    void swap(RecordBucket &other);

    IndexKey split(RecordBucket &other);

    struct RecordBucketInfo {
        Records    *records;
        size_t      length;
    };

    RecordBucketInfo bucket_;

private:

    size_t length_; // length_记录占用内存

    size_t size_; // size_记录含有的record的数量
};

enum MsgType {_Msg,Put,Del};

class Msg {
public: 
    Msg():type_(_Msg){}

    Msg(MsgType t, IndexKey k, Tuple* v):
    type_(t),
    key(k),
    value(v){}

    MsgType   type_;
    IndexKey  key;
    Tuple     *value;
};

inline bool operator==(const Msg& a, const Msg& b){
        return a.key==b.key;
    }

inline bool operator!=(const Msg& a, const Msg& b){
        return a.key!=b.key;
    }

inline bool operator<(const Msg& a, const Msg& b){
        return a.key<b.key;
    }

inline bool operator>(const Msg& a, const Msg& b){
        return a.key>b.key;
    }


class MsgBuf {
public:
    MsgBuf():count_(0){msgs_.clear();}

    ~MsgBuf(){
        count_ = 0;
        msgs_.clear();
    }

    void read_lock(){
        lock_.GetReadLock();
    }

    void rLock_unlock(){
        lock_.ReleaseReadLock();
    }

    void write_lock(){
        lock_.GetWriteLock();
    }

    void wlock_unlock(){
        lock_.ReleaseWriteLock();
    }

    void write(const Msg& msg){
        int idx = msgs_.binaryFind(msg);
        if(idx<msgs_.size()&&msgs_[idx].key==msg.key){
            msgs_[idx] = msg;
        }else{
            msgs_.insert(idx,msg);
        }
        count_ = msgs_.size();
    }

    int find(IndexKey key){
        Msg keyMsg(_Msg,key,NULL);
        int idx = msgs_.binaryFind(keyMsg);
        assert(idx>=0&&idx<msgs_.size());
        return idx;
    }

    const Msg& get(int idx) const{
        assert(idx>=0&&idx<=msgs_.size());
        return msgs_[idx];
    }

    void clear(){
        msgs_.clear();
    }

    size_t count(){
        return count_;
    }

    DBrwLock lock_;
    List<Msg> msgs_;
    size_t count_;
};


#endif