#ifndef NODEPOOL_H
#define NODEPOOL_H

#include "node.h"
#include "options.h"
// #include "config.h"
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

    bool add_table(const TableID& tbid, NodeFactory *factory);

    void del_table(const TableID& tbid);

    void put(const TableID& tbid, bid_t nid, Node* node);

    Node* get(const TableID& tbid, bid_t nid);

    size_t byteSize();

private:

    class PoolKey {
    public:
        PoolKey(const TableID& t, bid_t n):tbid(t),nid(n){}

        bool operator<(const PoolKey& other) const {
            if(tbid==other.tbid){
                return nid<other.nid;
            }
            return tbid<other.tbid;
        }
        TableID tbid;
        bid_t nid;
    };

    DBrwLock tables_lock_;
    map<TableID,NodeFactory*> tables_;

    DBrwLock nodes_rwlock_;
    map<PoolKey,Node*> nodes_;
};


#endif