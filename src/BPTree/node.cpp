#include "node.h"
#include "tree.h"

#include <iostream>

using namespace std;

// ...... SchemaNode ...... //

size_t SchemaNode::size(){
    return 32;
}

// ...... InnerNode ...... //

InnerNode::~InnerNode(){
    pivots_.clear();
}

void InnerNode::init_root(){
    bottom_ = true;
}

int InnerNode::find_pivot(IndexKey k){
    Pivot keyPivot(k,NID_NIL);
    int res =  pivots_.binaryFind(keyPivot);
    if(res==pivots_.size()){
        return res;
    }
    else if(k==pivots_[res].key){
        res++;
    }
    // if(res==0&&pivots_.size()){
    //     if(k==pivots_[0].key){
    //         res++;
    //     }
    // }
    return res;
}

void InnerNode::add_pivot(IndexKey key, bid_t nid, List<DataNode*>& path){
    assert(path.back()==this);
    Pivot newPivot(key,nid);
    int idx = pivots_.binaryFind(newPivot);
    pivots_.insert(idx,newPivot);

    if(pivots_.size()+1>tree_->options_.inner_node_children_number){
        split(path);
    }else{
        while (path.size())
        {
            path.back()->wlock_unlock();
            path.pop_back();
        }
    }
}

void InnerNode::rm_pivot(bid_t nid, List<DataNode*>& path){
    assert(path.back()==this);

    if(first_child_==nid){
        if(pivots_.size()==0){ //当前节点移除pivot之后不存在值，此时该节点也需要移除
            path.back()->wlock_unlock();
            path.pop_back();
            if(path.size()==0){
                tree_->collapse();
            }else{
                InnerNode* parent = (InnerNode*) path.back();
                assert(parent);
                parent->rm_pivot(nid_,path);
            }
            return;
        }
        first_child_ = pivots_[0].child;
        pivots_.removeAt(0);
    }else{
        int idx = 0;
        for (int i = 0; i < pivots_.size(); i++)
        {
            if (pivots_[i].child==nid)
            {
                idx = i;
                break; 
            }
        }
        assert(idx>=0&&idx<pivots_.size());
        pivots_.removeAt(idx);
    }
    while (path.size())
    {
        path.back()->wlock_unlock();
        path.pop_back();
    }
}

void InnerNode::split(List<DataNode*>& path){
    assert(pivots_.size()>1);
    assert(path.back()==this);
    int n = pivots_.size()/2; //将0———n-1给前节点，n——size-1给后节点
    IndexKey k = pivots_[n].key;

    InnerNode *ni = tree_->new_inner_node();
    // ni->write_lock();
    ni->bottom_ = IS_LEAF(pivots_[n].child);
    assert(ni);

    ni->first_child_ = pivots_[n].child;

    if(n+1>=pivots_.size()){ // 分裂之后ni节点上无pivot
        ni->pivots_.clear();
    }else{
        ni->pivots_.clear();
        ni->pivots_.add(pivots_,n+1);
    }
    
    int removeCount_  = pivots_.size()-n;
    pivots_.removeRange(n,removeCount_);

    path.back()->wlock_unlock();
    path.pop_back();
    // ni->wlock_unlock();

    if(path.size()==0){
        InnerNode *newRoot = tree_->new_inner_node();
        assert(newRoot);
        newRoot->bottom_ = false;
        newRoot->first_child_ = nid_;
        newRoot->pivots_.clear();
        newRoot->pivots_.add(Pivot(k,ni->nid_));
        tree_->pileup(newRoot);

    }else{
        InnerNode* parent = (InnerNode*) path.back();
        assert(parent);
        parent->add_pivot(k,ni->nid_,path);
    }
}


bid_t InnerNode::child(int idx){
    assert(idx >= 0 && (size_t)idx<=pivots_.size());
    if (idx==0)
    {
        return first_child_;
    }else{
        return pivots_[idx-1].child;
    }
}

void InnerNode::set_child(int idx, bid_t nid){
    assert(idx>=0&&(size_t)idx<=pivots_.size());
    if(idx==0){
        first_child_= nid;
    }else{
        pivots_[idx-1].child = nid;
    }
}

bool InnerNode::find(IndexKey key, Tuple*& value, InnerNode* parent){
    bool ret = false;
    read_lock();
    if(parent){
        parent->rLock_unlock();
    }
    int idx = find_pivot(key);
    bid_t chidx = child(idx);
    if(chidx==NID_NIL){
        assert(idx==0);
        rLock_unlock();
        return false;
    }
    DataNode* ch = tree_->load_node(chidx);
    assert(ch);
    ret = ch->find(key,value,this);
    return ret;
}

void InnerNode::rangeFind(IndexKey startKey, IndexKey endKey, List<Tuple*>& values, InnerNode* parent){
    read_lock();
    if(parent){
        parent->rLock_unlock();
    }
    int idx = find_pivot(startKey);
    bid_t chidx = child(idx);
    if(chidx==NID_NIL){
        assert(idx==0);
        rLock_unlock();
        return;
    }
    DataNode* ch = tree_->load_node(chidx);
    assert(ch);
    ch->rangeFind(startKey,endKey,values,this);
    return;
}

void InnerNode::lock_path(IndexKey key, List<DataNode*>& path){
    int idx = find_pivot(key);
    // assert(idx>=0&&idx<pivots_.size());
    bid_t chidx = child(idx);
    DataNode* ch = tree_->load_node(chidx);
    assert(ch);
    ch->write_lock();
    path.push_back(ch);
    ch->lock_path(key,path);
}

bool InnerNode::write(const Msg& m){
    read_lock();
    maybe_descend(m);
    return true;
}

void InnerNode::maybe_descend(const Msg& m){
    IndexKey k = m.key;
    int idx = find_pivot(k);
    bid_t nid = child(idx);

    DataNode *node = NULL;
    if(nid==NID_NIL){
        rLock_unlock();
        write_lock(); // 新增child需要加写锁来防止出错的
        idx = find_pivot(k);
        nid = child(idx);
        if(nid==NID_NIL){
            cout<<"touch once\n";
            assert(bottom_);
            node = tree_->new_leaf_node();
            // LeafNode *tmpnode = (LeafNode*) node;
            // tmpnode->first_key_ = m.key;
            set_child(idx,node->nid());
        }else{
            node = tree_->load_node(nid);
        }
        wlock_unlock();
        read_lock();
    }else{
        node = tree_->load_node(nid);
    }
    assert(node);
    node->descend(m,this);
}

bool InnerNode::descend(const Msg& m,InnerNode* parent){
    read_lock();
    parent->rLock_unlock();
    maybe_descend(m);
    return true;
}
// ...... LeafNode ...... //

LeafNode::~LeafNode(){
    records_.bucket_.records->clear();
}

void LeafNode::split(IndexKey anchor){
    if(balancing_){
        wlock_unlock();
        return;
    }
    balancing_ = true;
    assert(records_.size()>1);
    wlock_unlock();

    List<DataNode*> path;
    tree_->lock_path(anchor, path);
    // assert(path.back()==this); //在进行split之前另一个insert导致提前发生了split
    if(path.back()->nid() != nid_){ // 在重新锁住之前没有提前发生其他的split
        while (path.size())
        {
            path.back()->wlock_unlock();
            path.pop_back();
        }
        balancing_ = false;
        return;
    }

    if(records_.size()<=1 || records_.size()<=(tree_->options_.leaf_node_record_count/2)){
        while (path.size())
        {
            path.back()->wlock_unlock();
            path.pop_back();
        }
        balancing_ = false;
        return;
    }

    LeafNode *nl = tree_->new_leaf_node();
    assert(nl);
    nl->write_lock();

    nl->left_sibling_ = nid_;
    nl->right_sibling_ = right_sibling_;
    if(right_sibling_>=NID_LEAF_START){
        LeafNode *rl = (LeafNode*)tree_->load_node(right_sibling_);
        assert(rl);
        rl->write_lock();
        rl->left_sibling_ = nl->nid_;
        rl->wlock_unlock();
    }
    right_sibling_ = nl->nid_;

    IndexKey k = records_.split(nl->records_);
    nl->first_key_ = k;
    nl->min_ = k;
    nl->max_ = max_;
    max_ = (*records_.bucket_.records)[records_.size()-1].key;

    balancing_ = false;
    
    nl->wlock_unlock();
    path.back()->wlock_unlock();
    path.pop_back();

    InnerNode* parent = (InnerNode*) path.back();
    assert(parent);
    parent->add_pivot(k,nl->nid_,path);
}

void LeafNode::merge(IndexKey anchor){
    if(balancing_){
        wlock_unlock();
        return;
    }
    balancing_ = true;
    assert(records_.size()==0);
    wlock_unlock();

    List<DataNode*> path;
    tree_->lock_path(anchor, path);
    // assert(path.back()==this);

    if(path.back()->nid()!=nid_){
        while (path.size())
        {
            path.back()->wlock_unlock();
            path.pop_back();
        }
        balancing_ = false;
        return;
    }

    if(records_.size()>0){
        while (path.size())
        {
            path.back()->wlock_unlock();
            path.pop_back();
        }
        balancing_ = false;
        return;
    }

    if(left_sibling_>=NID_LEAF_START) {
        LeafNode *ll = (LeafNode*)tree_->load_node(left_sibling_);
        assert(ll);
        ll->write_lock();
        ll->right_sibling_ = right_sibling_;
        ll->wlock_unlock();
    }
    if(right_sibling_>=NID_LEAF_START) {
        LeafNode *rl = (LeafNode*)tree_->load_node(right_sibling_);
        assert(rl);
        rl->write_lock();
        rl->left_sibling_ = left_sibling_;
        rl->wlock_unlock();
    }
    balancing_ = false;
    path.back()->wlock_unlock();
    path.pop_back();
    
    
    InnerNode *parent = (InnerNode*)path.back();
    assert(parent);
    parent->rm_pivot(nid_,path);
}

bool LeafNode::find(IndexKey key, Tuple*& value, InnerNode* parent){
    assert(parent);
    read_lock();
    parent->rLock_unlock();

    bool ret = false;

    int idx = records_.bucket_.records->binaryFind(Record(key,value));

    if(idx>=records_.size()){
        rLock_unlock();
        return false;
    }

    if((*(records_.bucket_.records))[idx].key==key){
        value = (*(records_.bucket_.records))[idx].value;
        ret = true;
    }
    rLock_unlock();
    return ret;
}

void LeafNode::rangeFind(IndexKey startKey, IndexKey endKey, List<Tuple*>& values, InnerNode* parent){
    assert(parent);
    read_lock();

    //原则上只往右遍历leafnode
    
    // case 1 : startKey>= min_, endKey<=max_
    //      [------]
    //        [  ]

    // case 2 : startKey<min_, endKey>max_
    //      [------]
    //     [        ]

    // case 3 : startKey>=min_, endKey>max_
    //      [------]
    //          [   ]

    // case 4 : startKey<min_, endKey<max_
    //      [------]
    //     [   ]

    // case 5 : startKey>max_
    //      [------]
    //              [   ]

    // case 6 : endKey<min_
    //      [------]
    // [   ]

    if(startKey>=min_){
        if(startKey>max_){ // case 5
            if (right_sibling_!=NID_NIL)
            {
                DataNode *rs = tree_->load_node(right_sibling_);
                rLock_unlock();
                rs->rangeFind(startKey, endKey, values, parent);
            }else{
                rLock_unlock();
                parent->rLock_unlock();
            }
            return;
        }
        if(endKey<=max_){ // case 1
            int start_idx = records_.bucket_.records->binaryFind(Record(startKey,NULL));

            int end_idx = records_.bucket_.records->binaryFind(Record(endKey,NULL));
            if(start_idx>=records_.size()||end_idx>=records_.size()){
                
            }else{
                for (int i = start_idx; i < end_idx; i++)
                {
                    values.push_back((*(records_.bucket_.records))[i].value);
                }
                if((*(records_.bucket_.records))[end_idx].key==endKey){
                    values.push_back((*(records_.bucket_.records))[end_idx].value);
                }
            }
            rLock_unlock();
            parent->rLock_unlock();
            return;
        }
        if(endKey>max_&&startKey<=max_){ // case 3
            int start_idx = records_.bucket_.records->binaryFind(Record(startKey,NULL));
            if(start_idx>=records_.size()){

            }else{
                for (int i = start_idx; i < records_.size(); i++)
                {
                    values.push_back((*(records_.bucket_.records))[i].value);
                }
            }
            if (right_sibling_!=NID_NIL)
            {
                DataNode *rs = tree_->load_node(right_sibling_);
                rLock_unlock();
                rs->rangeFind(startKey, endKey, values, parent);
            }else{
                rLock_unlock();
                parent->rLock_unlock();
            }
            return;
        }
    }else if(startKey<min_){
        if(endKey<min_){ // case 6
            rLock_unlock();
            parent->rLock_unlock();
            return;
        }
        if(endKey>max_){ // case 2
            for (int i = 0; i < records_.size(); i++)
            {
                values.push_back((*(records_.bucket_.records))[i].value);
            }
            if (right_sibling_!=NID_NIL)
            {
                DataNode *rs = tree_->load_node(right_sibling_);
                rLock_unlock();
                rs->rangeFind(startKey, endKey, values, parent);
            }else{
                rLock_unlock();
                parent->rLock_unlock();
            }
            return;
        }
        if(endKey<=max_&&endKey>=min_){ // case 4
            int end_idx = records_.bucket_.records->binaryFind(Record(endKey,NULL));
            if(end_idx>=records_.size()){

            }else{
                for (int i = 0; i < end_idx; i++)
                {
                    values.push_back((*(records_.bucket_.records))[i].value);
                }
                if((*(records_.bucket_.records))[end_idx].key==endKey){
                    values.push_back((*(records_.bucket_.records))[end_idx].value);
                }
            }
            rLock_unlock();
            parent->rLock_unlock();
            return;
        }
    }
}

bool LeafNode::descend(const Msg& m,InnerNode* parent){
    write_lock();

    IndexKey anchor = m.key;
    
    if(right_sibling_!=NID_NIL){
        // cout<<"touch............\n";
        LeafNode* right_sibling_node_ = (LeafNode* )tree_->load_node(right_sibling_);
        right_sibling_node_->read_lock();
        if(m.key>=right_sibling_node_->first_key_){
            right_sibling_node_->rLock_unlock();
            // cout<<"touch............\n";
            // plan 1:
            wlock_unlock();
            return right_sibling_node_->descend(m,parent);

            // plan 2:
            // parent->rLock_unlock();
            // wlock_unlock();
            // // return tree_->root_->write(m);
            // return tree_->put(m.key,m.value);
        }else{
            right_sibling_node_->rLock_unlock();
        }
    }

    tree_->count_mutex_.GetLatch();
    tree_->global_count_++;
    tree_->count_mutex_.ReleaseLatch();

    RecordBucket res;
    Record keyRecord(m.key,m.value);
    int beforecount = records_.size();
    RecordBucket::Iterator jt = records_.get_iterator();
    bool added = false;
    while (jt.valid())
    {
        if(m.key<jt.record().key){
            added = true;
            if(m.type_==Put){
                res.push_back(keyRecord);
            }
            break;
        }else if(m.key>jt.record().key){
            res.push_back(jt.record());
            jt.next();
        }else{
            added = true;
            if(m.type_==Put){
                res.push_back(keyRecord);
            }
            jt.next();
            break;
        }
    }
    while (jt.valid())
    {
        res.push_back(jt.record());
        jt.next();
    }
    if(!added){
        res.push_back(keyRecord);
    }
    records_.swap(res);
    int aftercount = records_.size();
    // assert(aftercount==(beforecount+1));

    if(aftercount){
        if((*records_.bucket_.records)[0].key<=first_key_){
        first_key_ = (*records_.bucket_.records)[0].key;
        }
        min_ = (*records_.bucket_.records)[0].key;
        max_ = (*records_.bucket_.records)[records_.size()-1].key;
    }else{
        first_key_ = -1;
        min_ = -1;
        max_ = -1;
    }
    
    parent->rLock_unlock();
    
    if(records_.size()==0){
        // cout<<"touchmerge\n";
        merge(anchor);
    }else if(records_.size()>1&&records_.size() > tree_->options_.leaf_node_record_count){
        split(anchor);
    }else{
        wlock_unlock();
    }
    return true;
}

void LeafNode::lock_path(IndexKey key, List<DataNode*>& path)
{
}


void InnerNode::scan(){
    if(first_child_!=NID_NIL){
        bid_t chidx = first_child_;
        DataNode *ch = tree_->load_node(chidx);
        ch->scan();
    }
    for (int i = 0; i < pivots_.size(); i++)
    {
        bid_t chidx = pivots_[i].child;
        DataNode *ch = tree_->load_node(chidx);
        ch->scan();
    }
}

void LeafNode::scan(){
    for (int i = 0; i < records_.size(); i++)
    {
        cout<<(*records_.bucket_.records)[i].key<<" ";
    }
    cout<<" ";
}

int InnerNode::treeHeight(){
    if (first_child_!=NID_NIL)
    {
        bid_t chidx = first_child_;
        DataNode *ch = tree_->load_node(chidx);
        return 1+ch->treeHeight();
    }else{
        return 0;
    }
}

int LeafNode::treeHeight(){
    return 1;
}