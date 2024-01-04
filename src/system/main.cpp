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


#include <ctime>
#include <cstdarg>

#include <cmath>

using namespace std;

DBrwLock rw_lock;

int count_;
int print_scan = 0;

List<Tuple*> val_list;
List<int> order_key_list;
List<int> rand_key_list;

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
    clock_t start = clock();
    func();
    clock_t end = clock();
    printf("%s use: %f ms\n", msg, 1000.0 * (end - start) / CLOCKS_PER_SEC);
}

void genKV(int count,int order_KV){

    order_key_list.clear();
    rand_key_list.clear();
    val_list.clear();
    for (int i = 0; i < count; i++)
    {
        order_key_list.push_back(i);
    }
    if(order_KV){
        while (order_key_list.size())
        {
            int idx = 0;
            rand_key_list.push_back(order_key_list[idx]);
            order_key_list.removeAt(idx);
        }
    }else{
    while (order_key_list.size())
    {
        int idx = rand() % order_key_list.size();
        rand_key_list.push_back(order_key_list[idx]);
        order_key_list.removeAt(idx);
    }
    }

    runBlock([&]() {
    for (int i = 0; i < count; i++)
    {
        val_list.push_back(new Tuple(4));
    }
    },"preparing data");
    
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
    Options opts;
    
    opts.inner_node_children_number = 8;
    opts.leaf_node_record_count = 8;
    
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
        int k = rand_key_list[keys*ta->id+i];
        assert(ta->db->put(k,val_list[k]));
        Tuple *tmp;
        assert(ta->db->get(k,tmp));
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
        int k = rand_key_list[keys*ta->id+i];
        // if(!ta->db->get(k,val)){
        //     // cout<<"touch\n";
        //     rw_lock.GetReadLock();
        //     rw_lock.GetWriteLock();
        // }
        assert(ta->db->get(k,val));
        assert(val==val_list[k]);
    }
}

void* run_delete(void *arg){

}

void db_pthread_test(int thread_num, int test_count,int print_scan,int order_KV){
    thread_args ta;
    ta.count = test_count;
    ta.thread_num = thread_num;

    int order = 2;
    while (pow(order,3)<test_count)
    {
        order = order<<1;
    }
    cout<<"order: "<<order<<"\n";

    Options opts;
    
    opts.inner_node_children_number = order;
    opts.leaf_node_record_count = order;

    TableID table_id = 0;
    ta.db = DB::open(table_id,opts);
    assert(ta.db);

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

    cout<<"-- write end --\n";
    
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
    },"tree insert");

    cout<<"-- search end --\n";
}

void test_genKV(int count){
    genKV(count,0);
    for (int i = 0; i < count; i++)
    {
        cout<<rand_key_list[i]<<" ";
    }
    cout<<"\n";
}

int main(int argc,char **argv){
    // record_test();
    // rwlock_test();
    // nodePool_test();
    // msg_test();
    // db_test();

    int thread_num = atoi(argv[1]);
    int test_count = atoi(argv[2]);
    int order_KV = 0;
    if(argc>=4){
        order_KV = atoi(argv[3]);
    }
    if(argc>=5){
        print_scan = atoi(argv[4]);
    }
    // test_genKV(test_count);
    db_pthread_test(thread_num,test_count,print_scan,order_KV);
}