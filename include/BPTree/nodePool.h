#ifndef NODEPOOL_H
#define NODEPOOL_H

#include "node.h"
#include "system/options.h"
#include "system/config.h"
#include <map>
#include <stdint.h>
#include <cstddef>
#include <cassert>

using namespace std;

class NodeFactory {
public:
    virtual Node* new_node(bid_t nid) = 0;
    virtual ~NodeFactory(){}
};


class NodePool {
public: 
    NodePool();

    ~NodePool();

    bool add_index(const IndexID& idxid, NodeFactory *factory);

    void del_index(const IndexID& idxid);

    void put(const IndexID& idxid, bid_t nid, Node* node);

    Node* get(const IndexID& idxid, bid_t nid);

    size_t byteSize();

private:

    class PoolKey {
    public:
        PoolKey(const IndexID& t, bid_t n):idxid(t),nid(n){}

        bool operator<(const PoolKey& other) const {
            if(idxid==other.idxid){
                return nid<other.nid;
            }
            return idxid<other.idxid;
        }
        IndexID idxid;
        bid_t nid;
    };

    DBrwLock indexs_lock_;
    map<IndexID,NodeFactory*> factorys_;

    DBrwLock nodes_rwlock_;
    map<PoolKey,Node*> nodes_;
};


#endif