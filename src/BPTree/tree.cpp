#include "tree.h"
#include <iostream>
#include <cassert>

using namespace std;

Tree::~Tree(){
    nodepool_->del_table(table_id_);
    delete node_factory_;
}

// 初始化树，负责为数构建节点工厂,初始化root_节点
bool Tree::init(){
    assert(options_.comparator);
    
    node_factory_ = new TreeNodeFactory(this);

    if(!nodepool_->add_table(table_id_,node_factory_)){
        return false;
    }
    
    schema_ = new SchemaNode(table_id_);
    schema_->root_node_id = NID_NIL;
    schema_->next_inner_node_id = NID_START;
    schema_->next_leaf_node_id = NID_LEAF_START;
    schema_->tree_depth = 2;
    nodepool_->put(table_id_, NID_SCHEMA, schema_);

    root_ = new_inner_node();
    root_->init_root();

    schema_->write_lock();
    schema_->root_node_id = root_->nid();
    schema_->wlock_unlock();

    assert(root_);
    return true;
}

bool Tree::put(IndexKey key, Tuple* value){
    assert(root_);
    InnerNode *root = root_;
    bool ret = root->put(key, value);
    return ret;
}

bool Tree::del(IndexKey key){
    assert(root_);
    InnerNode *root = root_;
    bool ret = root->del(key);
    return ret;
}

bool Tree::get(IndexKey key, Tuple*&value){
    assert(root_);
    InnerNode *root = root_;
    bool ret = root->find(key, value, NULL);
    return ret;
}

// [startKey, endKey]
bool Tree::rangeGet(IndexKey startKey, IndexKey endKey, List<Tuple*>& values){
    assert(root_);
    InnerNode *root = root_;
    root->rangeFind(startKey, endKey, values, NULL);
    // cout<<"touch.........\n";
    // cout<<"range search keys : "<<values.size()<<"\n\n\n";
    bool ret = values.size();
    return ret;
}

int Tree::treeHeight(){
    assert(root_);
    InnerNode *root = root_;
    return root->treeHeight();
}

InnerNode* Tree::new_inner_node(){
    schema_->write_lock();

    bid_t nid = schema_->next_inner_node_id;
    schema_->next_inner_node_id++;
    InnerNode* node = (InnerNode*)node_factory_->new_node(nid);
    schema_->wlock_unlock();

    assert(node);
    nodepool_->put(table_id_,nid,node);
    return node;
}

LeafNode* Tree::new_leaf_node(){
    schema_->write_lock();
    bid_t nid = schema_->next_leaf_node_id;
    schema_->next_leaf_node_id++;
    LeafNode* node = (LeafNode*)node_factory_->new_node(nid);
    schema_->wlock_unlock();

    assert(node);
    nodepool_->put(table_id_,nid,node);
    return node;
}

DataNode* Tree::load_node(bid_t nid){
    assert(nid!=NID_NIL&&nid!=NID_SCHEMA);
    return (DataNode*) nodepool_->get(table_id_,nid);
}

void Tree::collapse(){
    root_ = new_inner_node();
    root_->init_root();

    schema_->write_lock();
    schema_->root_node_id = root_->nid();
    schema_->tree_depth  = 2;
    schema_->wlock_unlock();
}

void Tree::pileup(InnerNode* root){
    assert(root_!=root);
    root_=root;
    schema_->write_lock();
    schema_->root_node_id = root_->nid();
    schema_->tree_depth++;
    schema_->wlock_unlock();
}

void Tree::lock_path(IndexKey key, List<DataNode*>& path){
    assert(root_);
    InnerNode *root = root_;
    root->write_lock();
    path.push_back(root);
    root->lock_path(key, path);
}

Tree::TreeNodeFactory::TreeNodeFactory(Tree *tree)
: tree_(tree)
{
}

Node* Tree::TreeNodeFactory::new_node(bid_t nid)
{
    DataNode *node;
    if (nid >= NID_LEAF_START) {
        node = new LeafNode(tree_->table_id_, nid, tree_);
    } else {
        node = new InnerNode(tree_->table_id_, nid, tree_);
    }
    return node;
    
}

void Tree::scan(){
    root_->scan();
}