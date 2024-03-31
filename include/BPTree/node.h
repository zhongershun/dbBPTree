#ifndef NODE_H
#define NODE_H

// #include "config.h"
#include "util/db_rw_lock.h"
#include "util/db_latch.h"

#include "storage/Record.h"
#include "storage/List.h"
#include "system/config.h"
// #include <pthread.h>
#include <mutex>

using namespace std;

typedef uint64_t bid_t; //每一个节点的id号

#define NID_NIL             ((bid_t)0)
#define NID_SCHEMA          ((bid_t)1)
#define NID_START           (NID_NIL + 2)
#define NID_LEAF_START      (bid_t)((1LL << 48) + 1)  // leaf node的id从2^48+1开始直到2^64-1
#define IS_LEAF(nid)        ((nid) >= NID_LEAF_START)

class Pivot { // 节点中用于存储中间节点的k以及其子节点的id
public:
    Pivot() {}
    
    Pivot(IndexKey k, bid_t c, MsgBuf *mb)
    : key(k), child(c), msgbuf(mb){}

    IndexKey    key;
    bid_t       child;
    MsgBuf      *msgbuf;
};

inline bool operator==(const Pivot& a, const Pivot& b){
        return a.key==b.key;
    }

inline bool operator!=(const Pivot& a, const Pivot& b){
        return a.key!=b.key;
    }

inline bool operator<(const Pivot& a, const Pivot& b){
        return a.key<b.key;
    }

inline bool operator>(const Pivot& a, const Pivot& b){
        return a.key>b.key;
    }

class Tree;

class Node {
public:
    Node(const IndexID index_id, bid_t nid):
    index_id_(index_id),nid_(nid){
        // pthread_rwlock_init(&lock_,nullptr);
        }

    virtual ~Node(){};

    bid_t nid() 
    {
        return nid_;
    }

    const IndexID& index_id()
    {
        return index_id_;
    }

    void read_lock()
    {
        lock_.GetReadLock();
        // lock_.GetLatch();
        // pthread_rwlock_rdlock(&lock_);
        // lock_.lock();
    }

    void write_lock()
    {
        lock_.GetWriteLock();
        // lock_.GetLatch();
        // pthread_rwlock_wrlock(&lock_);
        // lock_.lock();
    }

    void rLock_unlock()
    {
        lock_.ReleaseReadLock();
        // lock_.ReleaseLatch();
        // pthread_rwlock_unlock(&lock_);
        // lock_.unlock();
    }
    void wlock_unlock()
    {
        lock_.ReleaseWriteLock();
        // lock_.ReleaseLatch();
        // pthread_rwlock_unlock(&lock_);
        // lock_.unlock();
    }

protected:
    IndexID index_id_;
    bid_t nid_;

    DBrwLock lock_;
    // DBLatch lock_;
    // pthread_rwlock_t lock_;
    // mutex lock_;
};

class InnerNode;
class LeafNode;

class DataNode : public Node {
public:
    DataNode(const IndexID& index_id, bid_t nid, Tree *tree):
    Node(index_id,nid),tree_(tree){};

    ~DataNode() {};

    // Find values buffered in this node and all descendants
    virtual bool find(IndexKey key, Tuple*& value, InnerNode* parent) = 0;
    
    virtual void lock_path(IndexKey key, List<DataNode*>& path) = 0;

    virtual bool descend(const Msg& mb, InnerNode* parent) = 0;
    virtual bool descend(MsgBuf* mb, InnerNode* parent) = 0;
    virtual bool descend_no_fix(MsgBuf* mb) = 0;

    virtual void rangeFind(IndexKey startKey, IndexKey endKey, List<Tuple*>& values, InnerNode* parent) = 0;

    virtual bool boundFind(IndexKey boundKey, bool MaxOrMin,Tuple*& value, InnerNode* parent) = 0;

    virtual void scan()=0;

    virtual int treeHeight()=0;

    virtual void lock_range_leaf(IndexKey startKey, IndexKey endKey, List<DataNode*>& range) = 0;

    virtual void rangeScan(IndexKey startKey, IndexKey endKey, List<Tuple*>& values) = 0;

    // virtual size_t byteSize() = 0;

protected:
    Tree *tree_;
};

class SchemaNode : public Node {
public:
    SchemaNode(const IndexID& index_id)
    : Node(index_id, NID_SCHEMA)
    {
        root_node_id = NID_NIL;
        next_inner_node_id = NID_NIL;
        next_leaf_node_id = NID_NIL;
        tree_depth = 0;
    }

    size_t size();

    bid_t           root_node_id;
    bid_t           next_inner_node_id;
    bid_t           next_leaf_node_id;
    size_t          tree_depth;
};


class InnerNode : public DataNode {
public:
    InnerNode(const IndexID& index_id, bid_t nid, Tree *tree):
    DataNode(index_id,nid,tree),
    bottom_(false),
    first_child_(NID_NIL),
    first_msgbuf_(nullptr){}

    virtual ~InnerNode();

    void init_root();

    bool put(IndexKey key, Tuple* value)
    {
        return write(Msg(Put,key,value));
    }
    
    bool del(IndexKey key)
    {
        return write(Msg(Del,key,nullptr));
    }

    virtual bool find(IndexKey key, Tuple*& value, InnerNode* parent);

    void lock_path(IndexKey key, List<DataNode*>& path);

    bool descend(const Msg& m, InnerNode* parent);
    bool descend(MsgBuf* mb, InnerNode* parent);
    bool descend_no_fix(MsgBuf* mb);

    void rangeFind(IndexKey startKey, IndexKey endKey, List<Tuple*>& values, InnerNode* parent);

    bool boundFind(IndexKey boundKey, bool MaxOrMin,Tuple*& value, InnerNode* parent);

    void scan();

    int treeHeight();

    // size_t byteSize();

    void lock_range_leaf(IndexKey startKey, IndexKey endKey, List<DataNode*>& range);

    void rangeScan(IndexKey startKey, IndexKey endKey, List<Tuple*>& values);

protected:
    friend class LeafNode;

    bool write(const Msg& m);
    void maybe_descend();
    void maybe_descend(const Msg& m);

    int find_pivot(IndexKey k);
    void add_pivot(IndexKey key, bid_t nid, List<DataNode*>& path);
    void rm_pivot(bid_t nid, List<DataNode*>& path);

    bid_t child(int idx);
    void set_child(int idx, bid_t c);
    
    // msgbuf用
    void insert_msgbuf(const Msg& m, int idx);
    MsgBuf* msgbuf(int idx);
    int descend_msgbuf_idx();
    
    
    void split(List<DataNode*>& path);

    // true if children're leaf nodes
    bool bottom_;

    bid_t first_child_;
    MsgBuf* first_msgbuf_;
    size_t msgcnt_;
    
    List<Pivot> pivots_;
};


class LeafNode : public DataNode {
public:
    LeafNode(const IndexID& index_id, bid_t nid, Tree *tree):
    DataNode(index_id,nid,tree),
    balancing_(false),
    left_sibling_(NID_NIL),
    right_sibling_(NID_NIL),
    first_key_(-1),
    min_(-1),
    max_(-1){}
    
    ~LeafNode();
    
    virtual bool find(IndexKey key, Tuple*& value, InnerNode* parent);

    void lock_path(IndexKey key, List<DataNode*>& path);

    bool descend(const Msg& m,InnerNode* parent);
    bool descend(MsgBuf* mb,InnerNode* parent);
    bool descend_no_fix(MsgBuf* mb);

    void rangeFind(IndexKey startKey, IndexKey endKey, List<Tuple*>& values, InnerNode* parent);

    bool boundFind(IndexKey boundKey, bool MaxOrMin,Tuple*& value, InnerNode* parent);

    void scan();

    int treeHeight();

    void lock_range_leaf(IndexKey startKey, IndexKey endKey, List<DataNode*>& range);

    void rangeScan(IndexKey startKey, IndexKey endKey, List<Tuple*>& values);

    // size_t byteSize();

    IndexKey                first_key_;
    
protected:
    void split(IndexKey anchor);
    
    void merge(IndexKey anchor);

    void move_to_right(const Msg& m);
    

private:
    // either spliting or merging to get tree balanced
    bool                    balancing_;

    bid_t                   left_sibling_;
    bid_t                   right_sibling_;

    RecordBucket            records_;

    // 0xffff ffff ffff ffff
    IndexKey                min_;
    IndexKey                max_;
};

#endif