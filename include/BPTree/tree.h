#ifndef BPTREE_H
#define BPTREE_H

#include "node.h"
#include "nodePool.h"

#include "system/options.h"


#include <vector>
#include <stdint.h>
#include <cstddef>
#include <cassert>

class Tree {
public:
    Tree(const TableID& table_id,
         const Options& options,
         NodePool *nodepool,IndexID index_id)
    : table_id_(table_id),
      options_(options),
      node_factory_(nullptr),
      root_(nullptr),
      schema_(nullptr),
      nodepool_(nodepool),
      index_id_(index_id)
    {
    }
    
    ~Tree();
    
    bool init();
    
    bool put(IndexKey key, Tuple* value);
    
    bool del(IndexKey key);

    bool get(IndexKey key, Tuple*& value);

    bool minBound(IndexKey minKey, Tuple*& value);

    bool maxBound(IndexKey maxKey, Tuple*& value);

    bool rangeGet(IndexKey startKey, IndexKey endKey, List<Tuple*>& values);

    int treeHeight();

    size_t byteSize();
    
    // debug用
    void scan();
    
    int global_count_=0;
    DBLatch count_mutex_;
private:
    friend class InnerNode;
    friend class LeafNode;

    InnerNode* new_inner_node();
    
    LeafNode* new_leaf_node();
    
    DataNode* load_node(bid_t nid);
    
    InnerNode* root() { return root_; }

    void collapse();

    void pileup(InnerNode* root);

    void lock_path(IndexKey key, List<DataNode*>& path);

    class TreeNodeFactory : public NodeFactory {
    public:
        TreeNodeFactory(Tree *tree);
        Node* new_node(bid_t nid);
    private:
        Tree        *tree_;
    };

    TableID         table_id_;
    IndexID         index_id_;

    Options         options_;

    TreeNodeFactory *node_factory_;

    InnerNode       *root_;

    SchemaNode      *schema_;

    NodePool        *nodepool_;
};

#endif