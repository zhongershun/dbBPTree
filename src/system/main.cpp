#include <iostream>
#include "Record.h"
#include<vector>
#include "db_rw_lock.h"
#include "db_latch.h"

#include <pthread.h>
#include <unistd.h>

#include "nodePool.h"
#include "options.h"

#include "db_impl.h"

#include "ThreadPool.h"

#include <ctime>
#include <cstdarg>

#include <cmath>

#include <random>
#include <chrono>

using namespace std;

DBrwLock rw_lock;

int count_;
int print_scan = 0;
int tupleSize = 4; // x Bytes
int tree_height_ = 3;

List<Tuple*> val_list;
// List<int> order_key_list;
// List<int> rand_key_list;
vector<int> order_key_list;
vector<int> rand_key_list;

List<int> added_key_list;

void printProgressBar(int progress) {
    int barWidth = 70;
    std::cout << "[";
    int pos = barWidth * progress / 100;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << progress << " %\n";
    // std::cout.flush();
}

template<class Function>
void runBlock(Function func, const char *msg) {
    // clock_t start = clock();
    auto start = std::chrono::high_resolution_clock::now();
    func();
    // clock_t end = clock();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // printf("%s use: %f ms\n", msg, (1000.0 * (end - start)) / CLOCKS_PER_SEC);
    printf("%s use: %f ms\n", msg, (1000.0*duration.count())/(1000.0*1000.0));
}

void genKV(int count, int order_KV){
    runBlock([&]() {
    order_key_list.clear();
    rand_key_list.clear();
    val_list.clear();
    added_key_list.clear();
    order_key_list.reserve(count);
    rand_key_list.reserve(count);
    val_list.reserve(count);
    cout<<".......preparing order data.......\n";
    for (long i = 0; i < count; i++)
    {
        if(i%(count/100)==0){
            int progress = i*100/count;
            printProgressBar(progress);
        }
        order_key_list.push_back(i);
    }
    cout<<".......preparing order data finished.......\n\n";
    cout<<".......preparing rand data.......\n";
    if(order_KV){
        rand_key_list.swap(order_key_list);
        // rand_key_list.add(order_key_list);
        
        // while (order_key_list.size())
        // {
        //     if(rand_key_list.size()%(count/1000)==0){
        //         int progress = ((long)(rand_key_list.size()*100))/count;        
        //         printProgressBar(progress);
        //     }
        //     int idx = 0;
        //     rand_key_list.push_back(order_key_list[idx]);
        //     order_key_list.removeAt(idx);
        // }
    }else{
        shuffle(order_key_list.begin(),order_key_list.end(),default_random_engine(time(NULL)));
        rand_key_list.swap(order_key_list);
    }
    for (int i = 0; i < count; i++)
    {
        val_list.push_back(new Tuple(tupleSize));
    }
    },"preparing data");
    cout<<".......preparing rand data finished.......\n\n";
}

void list_test(){
    int count = 10;
    // genKV(count,1);
    vector<int> testList;
    for (int i = 0; i < count; i++)
    {
        testList.push_back(i);
    }
    
    for (int i = 0; i < count; i++)
    {
        cout<<testList[i]<<" ";
    }
    // for (int i = 0; i < count; i++)
    // {
    //     if(i%1000==0){
    //         cout<<rand_key_list[i]<<" ";
    //     }
    // }
    cout<<"\n";
    for (auto it = testList.begin();it!=testList.end();++it)
    {
        // if(*it%1000==0){
            cout<<*it<<" ";
        // }
    }
    // for (auto it = rand_key_list.begin();it!=rand_key_list.end();++it)
    // {
    //     if(*it%1000==0){
    //         cout<<*it<<" ";
    //     }
    // }
    cout<<"\n";

    shuffle(testList.begin(),testList.end(),default_random_engine(time(NULL)));
    for (auto it = testList.begin();it!=testList.end();++it)
    {
        // if(*it%1000==0){
            cout<<*it<<" ";
        // }
    }
    cout<<"\n";
}

void record_test(){
    // int count = atoi(argv[1]);
    int count = 10;
    Records records;
    for (int i = 0; i < count; i++)
    {
        IndexKey k = i;
        Tuple *v= new Tuple(10);
        Record record = Record(k,v);
        records.push_back(record);
    }
    RecordBucket rb;
    rb.set_bucket(&records);
    cout<<"size: "<<rb.size()<<"\n";
    cout<<"length: "<<rb.length()<<"\n";
    for (int i = 0; i < count; i++)
    {
        IndexKey k = i;
        Tuple *v= new Tuple(10);
        Record record = Record(k,v);
        rb.push_back(record);
    }
    cout<<"size: "<<rb.size()<<"\n";
    cout<<"length: "<<rb.length()<<"\n";

    RecordBucket rb2;
    int res = rb.split(rb2);
    cout<<"size: "<<rb.size()<<"\n";
    cout<<"length: "<<rb.length()<<"\n";
    cout<<"size: "<<rb2.size()<<"\n";
    cout<<"length: "<<rb2.length()<<"\n";
    cout<<"res: "<<res<<"\n";

    RecordBucket rb3;
    int res2 = rb2.split(rb3);
    cout<<"size: "<<rb2.size()<<"\n";
    cout<<"length: "<<rb2.length()<<"\n";
    cout<<"res: "<<res2<<"\n";

    rb.swap(rb2);
    cout<<"size: "<<rb.size()<<"\n";
    cout<<"length: "<<rb.length()<<"\n";
    cout<<"size: "<<rb2.size()<<"\n";
    cout<<"length: "<<rb2.length()<<"\n";
}

void *run_read(void* arg){
    rw_lock.GetReadLock();
    cout<<"count: "<<count_<<"\n";
    usleep(3000000);
    rw_lock.ReleaseReadLock();
}

void *run_write(void* arg){
    rw_lock.GetWriteLock();
    count_++;
    cout<<"count: "<<count_<<"\n";
    usleep(3000000);
    rw_lock.ReleaseWriteLock();
}

void *run_2rlock(void* arg){
    rw_lock.GetReadLock();
    // rw_lock.GetReadLock();
    cout<<"count: "<<count_<<"\n";
    rw_lock.ReleaseReadLock();
    rw_lock.ReleaseReadLock();
    rw_lock.GetWriteLock();
}

void rwlock_test(){
    int thread_num = 2;
    pthread_t ids[thread_num];
    int* t;
    
    // 首先测试两个线程读
    pthread_create(&ids[0],0,run_read,(void*)t);
    pthread_create(&ids[1],0,run_read,(void*)t);
    pthread_join(ids[0],(void**)&t);
    pthread_join(ids[1],(void**)&t);

    cout<<"success\n";

    // 首先测试两个线程写
    pthread_create(&ids[0],0,run_write,(void*)t);
    pthread_create(&ids[1],0,run_write,(void*)t);
    pthread_join(ids[0],(void**)&t);
    pthread_join(ids[1],(void**)&t);

    cout<<"success\n";

}

void nodePool_test(){
    NodePool *nodePool = new NodePool();
    TableID tbid = 1;
    class FakeNode : public Node {
    public:
        FakeNode(const TableID& table_name, bid_t nid) : Node(table_name, nid), data(0) {}

        bool find(IndexKey key, Tuple& value, InnerNode* parent) { return false; }

        void lock_path(IndexKey key, std::vector<Node*>& path) {}
    
        size_t size()
        {
            return 4096;
        }

        uint64_t data;
    };
    class FakeNodeFactory: public NodeFactory {
    public:
        FakeNodeFactory(const TableID& tbid):
        tbid_(tbid){}
        Node* new_node(bid_t nid) {
            return new FakeNode(tbid_, nid);
        }
        TableID tbid_;
    };

    NodeFactory *factory = new FakeNodeFactory(tbid);

    nodePool->add_table(tbid,factory);
    for (int i = 0; i < 3; i++)
    {
        Node *nod = new FakeNode(tbid,i);
        nodePool->put(tbid,i,nod);
    }
    for (int i = 0; i < 3; i++)
    {
        assert(nodePool->get(tbid,i));
    }
    nodePool->del_table(tbid);

}

void msg_test(){
    MsgBuf msgbuf;

    for (int i = 0; i < 3; i++)
    {
        msgbuf.write(Msg(Put,i,new Tuple(10)));
    }
    cout<<msgbuf.count()<<"\n";
    int id1 = msgbuf.find(1);
    assert(id1==1);
    
    for (int i = 0; i < 3; i++)
    {
        assert(msgbuf.get(i).key==i);
    }
    cout<<"success\n";
    
}

void db_test(){
    Options opts(8);
    
    // opts.inner_node_children_number = 8;
    // opts.leaf_node_record_count = 8;
    
    TableID table_id = 0;
    DB *db = DB::open(table_id,opts);
    assert(db);
    
    val_list.clear();
    int tuple_count = 100;

    // runBlock([&]() {
    // for (long i = 0; i < tuple_count; i++)
    // {
    //     val_list.push_back(new Tuple(4));
    //     // if((i+1)%10000==0){
    //     //         int progress = ((i+1)*100/tuple_count);
    //     //         printProgressBar(progress);
    //     //     }
    // }
    // },"preparing data");
    genKV(tuple_count,0);

    runBlock([&]() {
    for (long i = 0; i < tuple_count; i++)
    {
        assert(db->put(i,val_list[i]));
        // if((i+1)%10000==0){
        //         int progress = ((i+1)*100/tuple_count);
        //         printProgressBar(progress);
        //     }
    }
    },"insert");
    
    runBlock([&]() {
    for (long i = 0; i < tuple_count; i++)
    {
        Tuple* val;
        assert(db->get(i,val));
        assert(val==val_list[i]);
        // if((i+1)%10000==0){
        //         int progress = ((i+1)*100/tuple_count);
        //         printProgressBar(progress);
        //     }
    }
    },"search");

    runBlock([&]() {
    for (long i = 0; i < tuple_count; i++)
    {
        assert(db->del(i));
        // if((i+1)%10000==0){
        //         int progress = ((i+1)*100/tuple_count);
        //         printProgressBar(progress);
        //     }
    }
    },"delete");

    cout<<"success\n";
}

struct thread_args
{
    int id;
    int count;
    int thread_num;
    DB* db;
};


void* run_insert(void *arg){
    thread_args *ta = (thread_args*) arg;

    int count = ta->count; // 总的Tuple数量
    int keys = count/ta->thread_num; // 每个线程插入的Tuple数量
    assert(keys*ta->thread_num==count);

    for (int i = 0; i < keys; i++)
    {
        IndexKey k = rand_key_list[keys*ta->id+i];
        assert(ta->db->put(k,val_list[k]));
        Tuple *tmp;
        // assert(ta->db->get(k,tmp));
        if(print_scan){
            rw_lock.GetWriteLock();
            cout<<"pass: "<<k<<"\n";
            rw_lock.ReleaseWriteLock();
        }
    }
}

void* run_search(void *arg){
    thread_args *ta = (thread_args*) arg;

    int count = ta->count; // 总的Tuple数量
    int keys = count/ta->thread_num; // 每个线程插入的Tuple数量

    for (int i = 0; i < keys; i++)
    {
        Tuple* val;
        IndexKey k = rand_key_list[keys*ta->id+i];
        // if(!ta->db->get(k,val)){
        //     // cout<<"touch\n";
        //     rw_lock.GetReadLock();
        //     rw_lock.GetWriteLock();
        // }
        assert(ta->db->get(k,val));
        assert(val==val_list[k]);
    }
}

void* run_rangeSearch(void *arg){
    thread_args *ta = (thread_args*) arg;
    int count = ta->count; // 总的Tuple数量
    int keys = count/ta->thread_num; // 每个线程负责的Tuple数量
    IndexKey rangePerThread = 10;
    IndexKey startKey = keys*ta->id;
    IndexKey endKey = startKey+rangePerThread-1;
    //[ startKey, endKey ];
    List<Tuple*> vals;
    vals.clear();
    // cout<<"endKey edge : "<<keys*(ta->id+1)<<"\n";
    while (endKey<keys*(ta->id+1))
    {
        ta->db->rangeGet(startKey,endKey,vals);
        // cout<<"startKey : \t"<<startKey<<"\n";
        // cout<<"endKey : \t"<<endKey<<"\n";
        // cout<<"rangeSearch : \t"<<vals.size()<<"\n\n";
        startKey+=rangePerThread;
        endKey+=rangePerThread;
        
    }
    // cout<<"range search keys : "<<vals.size()<<"\n";
    assert(vals.size()==keys);
    for (int i = 0; i < vals.size(); i++)
    {
        assert(vals[i]==val_list[i+keys*ta->id]);
    }
    
}

void* run_delete(void *arg){
    thread_args *ta = (thread_args*) arg;

    int count = ta->count; // 总的Tuple数量
    int keys = count/ta->thread_num; // 每个线程插入的Tuple数量

    for (int i = 0; i < keys; i++)
    {
        Tuple* val;
        IndexKey k = rand_key_list[keys*ta->id+i];
        // if(!ta->db->get(k,val)){
        //     // cout<<"touch\n";
        //     rw_lock.GetReadLock();
        //     rw_lock.GetWriteLock();
        // }
        assert(ta->db->del(k));
        // assert(val==val_list[k]);
    }
}

void db_pthread_test(int thread_num, int test_count,int print_scan,int order_KV){
    // runBlock([&](){
    //     sleep(10);
    // },"test runBlock");
    thread_args ta;
    ta.count = test_count;
    ta.thread_num = thread_num;

    cout<<"-- initialzing DB configuration --\n\n";

    int order = 2;
    while (pow(order,tree_height_)<test_count)
    {
        if(order<10){
            order+=1;
        }else if(order<100){
            order+=10;
        }else if(order<150){
            order+=5;
        }else if(order<200){
            order+=2;
        }else{
            order+=1;
        }
    }
    cout<<"order: "<<order<<"\n\n";

    Options opts(order);
    
    // opts.inner_node_children_number = order;
    // opts.leaf_node_record_count = order;
    // return;
    TableID table_id = 0;
    ta.db = DB::open(table_id,opts);
    assert(ta.db);

    cout<<"-- DB initialization finished --\n\n";

    genKV(test_count,order_KV);

    if(print_scan){
        for (int i = 0; i < rand_key_list.size(); i++)
        {
            cout<<rand_key_list[i]<<" ";
        }
        cout<<"\n";
    }
    
    pthread_t ids[thread_num];

    cout<<"-- write start --\n";

    runBlock([&]() {
    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t = new thread_args;
        *t = ta;
        t->id = i;
        assert(pthread_create(&ids[i],0,run_insert,(void*)t)==0);
    }

    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t;
        assert(pthread_join(ids[i],(void**)&t)==0);
        // delete t;
    }
    },"tree insert");

    cout<<"-- write end --\n\n";

    int treeHeight = ta.db->treeHeight();
    size_t treeByteSize = ta.db->byteSize();
    size_t poolByteSize = ta.db->poolSize();

    cout<<"now Bplustree mem occupied:\t"<<treeByteSize<<"\n\n";
    cout<<"now nodepool mem occupied:\t"<<poolByteSize<<"\n\n";

    assert(treeHeight!=0);
    
    if(print_scan){
        ta.db->scan();
        cout<<"\n";
    }
    cout<<"insert count: "<<ta.db->descendCount()<<"\n";

    cout<<"-- search start --\n";

    runBlock([&]() {
    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t = new thread_args;
        *t = ta;
        t->id = i;
        assert(pthread_create(&ids[i],0,run_search,(void*)t)==0);
    }

    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t;
        assert(pthread_join(ids[i],(void**)&t)==0);
        // delete t;
    }
    },"tree search");

    cout<<"-- search end --\n\n";

    cout<<"-- rangeSearch start --\n";

    runBlock([&]() {
    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t = new thread_args;
        *t = ta;
        t->id = i;
        assert(pthread_create(&ids[i],0,run_rangeSearch,(void*)t)==0);
    }

    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t;
        assert(pthread_join(ids[i],(void**)&t)==0);
        // delete t;
    }
    },"tree rangeSearch");

    cout<<"-- rangeSearch end --\n\n";

    cout<<"-- delete start --\n";

    runBlock([&]() {
    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t = new thread_args;
        *t = ta;
        t->id = i;
        assert(pthread_create(&ids[i],0,run_delete,(void*)t)==0);
    }

    for (auto i = 0; i < thread_num; i++)
    {
        thread_args *t;
        assert(pthread_join(ids[i],(void**)&t)==0);
        // delete t;
    }
    },"tree delete");

    cout<<"-- delete end --\n\n";

    cout<<"\ntest finish\n";
    cout<<"thread count:\t"<<thread_num<<"\n";
    cout<<"data count:\t"<<test_count<<"\n\n";
    cout<<"tree height:\t"<<treeHeight<<"\n\n";
    cout<<"key type:\t"<<"IndexKey(uint64_t) = 8 Bytes"<<"\n";
    cout<<"key size:\t"<<test_count*8<<" Bytes\n\n";
    cout<<"value type:\t"<<"Tuple("<<tupleSize<<") = "<<tupleSize<<" Bytes"<<"\n";
    cout<<"value size:\t"<<test_count<<" Bytes\n\n";
    cout<<"data size:\t"<<test_count*sizeof(IndexKey)+test_count*tupleSize<<" Bytes\n\n";
    cout<<"max Bplustree mem occupied:\t"<<treeByteSize<<"\n\n";
    cout<<"max nodepool mem occupied:\t"<<poolByteSize<<"\n\n";
    if(order_KV){
        cout<<"key order:\t"<<"order\n";
    }else{
        cout<<"key order:\t"<<"random\n";
    }

}

void test_genKV(int count){
    genKV(count,0);
    for (int i = 0; i < count; i++)
    {
        if(i%100000==0){
            cout<<rand_key_list[i]<<" ";
        }
    }
    cout<<"\n";
}

void helpMsg(){
    cout<<"\n";
    cout<<"usgae: ./BPTREE_IMPL\tthread_num\ttest_count\torder_KV(default: 0; 0: inorder; 1: order)\tprint_scan(default: 0)\top_count\n";
    cout<<"\n";
}

enum OP_TYPE {INSERT, SCAN, RANGE_SCAN, DELETE};

struct mixed_thread
{
    OP_TYPE type;
    DB* db;
};

void run_mixed_op(void *arg){
    mixed_thread *ta = (mixed_thread*) arg;
    IndexKey opKey;
    
    switch (ta->type)
    {
    case INSERT:{
        rw_lock.GetReadLock();
        if(rand_key_list.size()==0){
            rw_lock.ReleaseReadLock();
            break;       
        }
        rw_lock.ReleaseReadLock();
        rw_lock.GetWriteLock();
        opKey = rand_key_list[rand_key_list.size()-1];
        rand_key_list.pop_back();
        int toAddIdx = added_key_list.binaryFind(opKey);
        added_key_list.insert(toAddIdx,opKey);
        rw_lock.ReleaseWriteLock();
        assert(ta->db->put(opKey,val_list[opKey]));
        }
        break;
    case SCAN:{
        rw_lock.GetReadLock();
        if(added_key_list.size()==0){
            rw_lock.ReleaseReadLock();
            break;
        }
        opKey = added_key_list[rand()%added_key_list.size()];
        rw_lock.ReleaseReadLock();
        Tuple *tmp;
        assert(ta->db->get(opKey,tmp));
        assert(tmp==val_list[opKey]);
        }
        break;
    case RANGE_SCAN:{
        rw_lock.GetReadLock();
        if(added_key_list.size()<1){
            rw_lock.ReleaseReadLock();
            break;
        }
        int startKeyIdx = rand()%(added_key_list.size()-1);
        IndexKey startKey = added_key_list[startKeyIdx];
        int range = rand()%(added_key_list.size()-startKeyIdx);
        if(range==0){
            range = 1;
        }
        int endKeyIdx = startKeyIdx+range;
        IndexKey endKey = added_key_list[endKeyIdx];
        List<IndexKey> matchKey;
        matchKey.clear();
        for (int i = startKeyIdx; i <= endKeyIdx; i++)
        {
            matchKey.add(added_key_list[i]);
        }
        List<Tuple*> vals;
        vals.clear();
        ta->db->rangeGet(startKey,endKey,vals);
        rw_lock.ReleaseReadLock();
        assert(vals.size()==matchKey.size());
        rw_lock.GetWriteLock();
        if(matchKey.size()!=vals.size()){
            cout<<"-- rangeScan match test --\n\n";
            cout<<"match key:\t "<<matchKey.size()<<"\n";
            cout<<"vals key:\t "<<vals.size()<<"\n";
        }
        rw_lock.ReleaseWriteLock();
        }
        break;
    case DELETE:{
        rw_lock.GetReadLock();
        if(added_key_list.size()==0){
            rw_lock.ReleaseReadLock();
            break;    
        }
        rw_lock.ReleaseReadLock();
        rw_lock.GetWriteLock();
        int opKeyIdx = rand()%added_key_list.size();
        opKey = added_key_list[opKeyIdx];
        added_key_list.removeAt(opKeyIdx);
        rw_lock.ReleaseWriteLock();
        assert(ta->db->del(opKey));
        }
        break;
    }
}

void mixed_test(int thread_num, int test_count, int op_count, int order_KV){
    
    mixed_thread ta;

    cout<<"-- initialzing DB configuration --\n\n";

    int order = 2;
    while (pow(order,tree_height_)<test_count)
    {
        order = order<<1;
    }
    cout<<"order: "<<order<<"\n\n";

    Options opts(order);
    
    // opts.inner_node_children_number = order;
    // opts.leaf_node_record_count = order;

    TableID table_id = 0;
    ta.db = DB::open(table_id,opts);
    assert(ta.db);

    cout<<"-- DB initialization finished --\n\n";
    genKV(test_count,order_KV);

    cout<<"-- insert test_count/4 (K,V) for warm-up --\n";

    runBlock([&](){
    for (int i = 0; i < test_count/4; i++)
    {
        IndexKey opKey = rand_key_list[rand_key_list.size()-1];
        rand_key_list.pop_back();
        int toAddIdx = added_key_list.binaryFind(opKey);
        added_key_list.insert(toAddIdx,opKey);
        assert(ta.db->put(opKey,val_list[opKey]));
    }
    },"warm-up");
    
    cout<<"-- warm-up finished --\n\n";

    pthread_t ids[thread_num];

    List<int> type_count;
    type_count.reserve(4);
    for (int i = 0; i < 4; i++)
    {
        type_count.add(0);
    }
    
    cout<<"-- mixed test start --\n";

    // 初始化线程池
    ThreadPool pool(thread_num);
    pool.Start();
    
    runBlock([&](){
        for (int i = 0; i < op_count; i++)
        {
            mixed_thread *t = new mixed_thread;
            *t = ta;
            int type_id = rand()%4;
            t->type = OP_TYPE(type_id);

            type_count[type_id] = type_count[type_id]+1;
            
            auto fun = std::bind(run_mixed_op,(void*)t);
            if(type_id==0){
                pool.AddFun(fun,"insert");
            }else if(type_id==1){
                pool.AddFun(fun,"scan");
            }else if(type_id==2){
                pool.AddFun(fun,"range_scan");
            }else{
                pool.AddFun(fun,"delete");
            }
        }
        },"mixed operation test");

    sleep(1);

    cout<<"-- mixed test end --\n\n";

    cout<<"\ntest finish\n";
    cout<<"thread count:\t"<<thread_num<<"\n";
    cout<<"oprate count:\t"<<op_count<<"\n\n";
    
    cout<<"INSERTATION op_count:\t"<<type_count[0]<<"\n";
    cout<<"SINGALSCAN op_count:\t"<<type_count[1]<<"\n";
    cout<<"RANGESCAN op_count:\t"<<type_count[2]<<"\n";
    cout<<"DELETATION op_count:\t"<<type_count[3]<<"\n\n";
}


void test(int i)
{
    cout<<i<<"\n";
    usleep(50);
}

void threadPool_test(){
    ThreadPool pool(4);
    pool.Start();
    for(int i=0; i<100; i++)
    {
        auto fun = std::bind(test,i);
        pool.AddFun(fun,"group");
    }
 
    sleep(3);
    cout<<"exit\n";

}

int main(int argc,char **argv){
    // record_test();
    // rwlock_test();
    // nodePool_test();
    // msg_test();
    // db_test();
    // list_test();
    // return 0;
    if(argc==1){
        helpMsg();
        return 0;
    }else{

    int thread_num = atoi(argv[1]);
    int test_count = atoi(argv[2]);
    int order_KV = 0;
    int op_count = 1000000;
    if(argc>=4){
        order_KV = atoi(argv[3]);
    }
    if(argc>=5){
        print_scan = atoi(argv[4]);
    }
    if(argc>=6){
        op_count = atoi(argv[5]);
    }
    // test_genKV(test_count);
    db_pthread_test(thread_num,test_count,print_scan,order_KV);
    // mixed_test(thread_num, test_count, op_count, order_KV);
    // threadPool_test();
    }
}