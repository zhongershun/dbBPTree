#include "BPTree/nodePool.h"

using namespace std;

NodePool::NodePool(){};

NodePool::~NodePool(){};

bool NodePool::add_index(const IndexID& idxid, NodeFactory* factory){
    indexs_lock_.GetWriteLock();
    if(factorys_.find(idxid)!=factorys_.end()){
        indexs_lock_.ReleaseWriteLock();
        return false;
    }

    factorys_[idxid] = factory;
    indexs_lock_.ReleaseWriteLock();
    return true;
}

void NodePool::del_index(const IndexID& idxid){
    indexs_lock_.GetWriteLock();
    auto it = factorys_.find(idxid);
    if(it==factorys_.end()){
        indexs_lock_.ReleaseWriteLock();
        return;
    }
    factorys_.erase(it);
    indexs_lock_.ReleaseWriteLock();

    nodes_rwlock_.GetWriteLock();
    for (auto it = nodes_.begin(); it != nodes_.end(); it++)
    {
        if(it->first.idxid == idxid){
            Node* node = it->second;
            delete node;

            nodes_.erase(it);
        }
    }

    nodes_rwlock_.ReleaseWriteLock();
}

void NodePool::put(const IndexID& idxid, bid_t nid, Node* node){
    PoolKey key(idxid,nid);
    nodes_rwlock_.GetWriteLock();
    assert(nodes_.find(key) == nodes_.end());
    nodes_[key] = node;
    nodes_rwlock_.ReleaseWriteLock();
}

Node* NodePool::get(const IndexID& idxid, bid_t nid){
    PoolKey key(idxid,nid);
    Node* node;
    nodes_rwlock_.GetReadLock();
    auto it = nodes_.find(key);
    if(it!=nodes_.end()){
        node = it->second;
        nodes_rwlock_.ReleaseReadLock();
        return node;
    }
    nodes_rwlock_.ReleaseReadLock();

    // 若节点不存在，利用存储的index的factory创建一个新的节点
    indexs_lock_.GetReadLock();
    NodeFactory* factory=nullptr;
    auto it2 = factorys_.find(idxid);
    if(it2!=factorys_.end()){
        factory = it2->second;
    }
    indexs_lock_.ReleaseReadLock();
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