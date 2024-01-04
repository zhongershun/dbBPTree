#include "db_impl.h"

#include <iostream>

using namespace std;

DBImpl::~DBImpl(){
    delete tree_;
    delete nodepool_;
}

bool DBImpl::init(){
    nodepool_ = new NodePool();
    tree_ = new Tree(table_id_,options_,nodepool_);

    if(!tree_->init()){
        cout<<"tree init error\n";
        return false;
    }

    return true;
}

bool DBImpl::put(IndexKey key, Tuple* value){
    return tree_->put(key,value);
}

bool DBImpl::del(IndexKey key)
{
    return tree_->del(key);
}

bool DBImpl::get(IndexKey key, Tuple*& value)
{
    return tree_->get(key, value);
}


DB* DB::open(const TableID& table_id, const Options& options){
    DBImpl* db = new DBImpl(table_id, options);
    if(!db->init()){
        delete db;
        return NULL;
    }
    return db;
}