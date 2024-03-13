#include "nodePool.h"

using namespace std;

NodePool::NodePool(){};

NodePool::~NodePool(){};

bool NodePool::add_table(const TableID& tbid, NodeFactory* factory){
    tables_lock_.GetWriteLock();
    if(tables_.find(tbid)!=tables_.end()){
        tables_lock_.ReleaseWriteLock();
        return false;
    }

    tables_[tbid] = factory;
    tables_lock_.ReleaseWriteLock();
    return true;
}

void NodePool::del_table(const TableID& tbid){
    tables_lock_.GetWriteLock();
    auto it = tables_.find(tbid);
    if(it==tables_.end()){
        tables_lock_.ReleaseWriteLock();
        return;
    }
    tables_.erase(it);
    tables_lock_.ReleaseWriteLock();

    nodes_rwlock_.GetWriteLock();
    for (auto it = nodes_.begin(); it != nodes_.end(); it++)
    {
        if(it->first.tbid == tbid){
            Node* node = it->second;
            delete node;

            nodes_.erase(it);
        }
    }

    nodes_rwlock_.ReleaseWriteLock();
}

void NodePool::put(const TableID& tbid, bid_t nid, Node* node){
    PoolKey key(tbid,nid);
    nodes_rwlock_.GetWriteLock();
    assert(nodes_.find(key) == nodes_.end());
    nodes_[key] = node;
    nodes_rwlock_.ReleaseWriteLock();
}

Node* NodePool::get(const TableID& tbid, bid_t nid){
    PoolKey key(tbid,nid);
    Node* node;
    nodes_rwlock_.GetReadLock();
    auto it = nodes_.find(key);
    if(it!=nodes_.end()){
        node = it->second;
        nodes_rwlock_.ReleaseReadLock();
        return node;
    }
    nodes_rwlock_.ReleaseReadLock();

    // 若节点不存在，利用存储的table的factory创建一个新的节点
    tables_lock_.GetReadLock();
    NodeFactory* factory=nullptr;
    auto it2 = tables_.find(tbid);
    if(it2!=tables_.end()){
        factory = it2->second;
    }
    tables_lock_.ReleaseReadLock();
    if(!factory){
        assert(false);
    }

    // 将新创建的node放入
    node = factory->new_node(nid);
    nodes_rwlock_.GetWriteLock();
    it = nodes_.find(key);
    if(it!=nodes_.end()){ //又找到该节点
        delete node;
        node = it->second;
    }else{
        nodes_[key] = node;
    }
    nodes_[key] = node;
    nodes_rwlock_.ReleaseWriteLock();
    
    return node;
}

size_t NodePool::byteSize(){
    return 8*nodes_.size();
}