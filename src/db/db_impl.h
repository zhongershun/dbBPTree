#ifndef DB_IMPL_H
#define DB_IMPL_H

#include "nodePool.h"
#include "tree.h"

class DB {
public:
    virtual ~DB() {}

    static DB* open(const TableID& table_id, const Options& options);

    virtual bool put(IndexKey key, Tuple* value) = 0;

    virtual bool del(IndexKey key) = 0;

    virtual bool get(IndexKey key, Tuple*& value) = 0;

    virtual bool rangeGet(IndexKey startKey, IndexKey endKey, List<Tuple*>& values) = 0;
    
    // inline bool get(IndexKey key, Tuple*& value)
    // {
    //     Tuple* v;
    //     if (!get(key, v)) {
    //         return false;
    //     }
    //     return true;
    // }

    virtual void scan() = 0;

    virtual size_t byteSize() = 0;

    virtual size_t poolSize() = 0;

    virtual int descendCount()=0;

    virtual int treeHeight()=0;
};

class Tree;

class DBImpl : public DB {
public:
    DBImpl(const TableID& table_id, const Options& options)
    : table_id_(table_id),
      options_(options),
      nodepool_(nullptr),
      tree_(nullptr){}

    ~DBImpl();

    bool init();

    bool put(IndexKey key, Tuple* value);

    bool del(IndexKey key);

    bool get(IndexKey key, Tuple*& value);

    void scan(){
        tree_->scan();
    };

    bool rangeGet(IndexKey startKey, IndexKey endKey, List<Tuple*>& values);

    int treeHeight(){
        return tree_->treeHeight();
    }

    size_t byteSize(){
        return tree_->byteSize();
    }

    size_t poolSize(){
        return nodepool_->byteSize();
    }

    int descendCount(){
        return tree_->global_count_;
    }

private:
    TableID     table_id_;
    Options     options_;
    NodePool    *nodepool_;
    Tree        *tree_;
};

#endif