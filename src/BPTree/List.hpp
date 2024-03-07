#ifndef LIST_HPP
#define LIST_HPP

#include "comp.h"
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <stdio.h>

#define MIN_CAP (uint32_t)1 // min capcity of list

template<typename T, class Comp=DefaultCompare<T>>
class List
{
private:
    uint32_t cap{};
    uint32_t size_ = 0;
    const Comp comp;

    inline void initAllocate();
    inline void expandTo(uint32_t cap);

    T *values;

public:

    List();

    // List():List(2){}
    
    // List(cap)
    List(uint32_t cap);
    
    // List(cap, comp)
    List(uint32_t cap, Comp comp);
    
    // List(&list)
    List(List &list);
    
    
    // List(const &list)
    List(const List &list);
    
    ~List();

    class Iterator{
        public:

        using difference_type = std::ptrdiff_t; // Standard definition
        using value_type = T;                    // Type of elements
        using pointer = T*;                      // Pointer to type
        using reference = T&;                    // Reference to type
        using iterator_category = std::forward_iterator_tag; // Iterator category

        Iterator(List* container, int count):
        container_(container),
        list_idx_(count){};

        bool valid(){
            return list_idx_ != container_->size();
        }

        Iterator& operator=(const Iterator& it){
            container_ = it.container_;
            list_idx_ = it.list_idx_;
        }

        bool operator==(const Iterator& it) const{
            return list_idx_==it.list_idx_;
        }

        bool operator!=(const Iterator& it) const{
            return list_idx_!=it.list_idx_;
        }

        Iterator& operator+(const int n){
            list_idx_+n;
            return *this;    
        }

        Iterator& operator-(const int n){
            list_idx_-n;
            return *this;    
        }

        Iterator& operator++(){
            list_idx_++;
            return *this;
        }

        Iterator& operator ++(int){
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator--(){
            list_idx_--;
            return *this;
        }

        Iterator& operator --(int){
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        T& operator*(){
            return (*container_)[list_idx_];
        }
        
        private:
        List* container_;
        int list_idx_;
    };


    // 设置idx处值set(idx,&val);
    void set(uint32_t index, const T &value);

    // add(val);
    void add(const T &value);

    // add(list);
    void add(const List<T, Comp> &list);

    // add(list,startIndex);
    void add(const List<T, Comp> &list, uint32_t startIndex);

    // add(list,startIndex,endIndex);
    void add(const List<T, Comp> &list, uint32_t startIndex, uint32_t endIndex);

    // insert(index,&val);
    void insert(uint32_t index, const T &value);

    // insert(index,&list);
    void insert(uint32_t index, const List<T, Comp> &list);

    // insert(index,&list,startIndex,count);
    void insert(int index, const List<T, Comp> &list, int startIndex, int count);

    // removeAt(idx);
    void removeAt(int index);

    // removeRange(startIdx,count);
    void removeRange(int start, int count);

    void clear();

    // reserve(cap);
    void reserve(uint32_t cap);
    
    // binaryFind(&val);
    uint32_t binaryFind(const T &val) const;

    uint32_t size() const;

    bool isEmpty();

    // get(idx);
    T &get(int index);

    // remove(&val);
    bool remove(T &value);

    void trimToSize();

    inline void autoTrim();

    // checkRange(idx)
    inline void checkRange(int index) const;

    // byteSize(count)
    inline size_t byteSize(uint32_t count);

    T &operator[](uint32_t index);

    const T &operator[](uint32_t index) const;

    // 新增方法
    void push_back(const T &value){
        add(value);
    };

    T &front(){
        checkRange(0);
        return values[0];
    }

    const T &front() const{
        checkRange(0);
        return values[0];
    }

    T &back(){
        int idx = size_ - 1;
        return values[idx];
    }

    const T &back() const{
        int idx = size_ - 1;
        return values[idx];
    }

    void pop_back(){
        checkRange(0);
        int idx = size_-1;
        removeAt(idx);
        return;
    }

    void pop_front(){
        checkRange(0);
        int idx = 0;
        removeAt(0);
        return;
    }

    Iterator begin(){
        return Iterator(this, 0);
    }

    Iterator end(){
        return Iterator(this, size());
    }
};

template<typename T, class Comp>
List<T,Comp>::List():List(2){}

template<typename T, class Comp>
List<T,Comp>::List(uint32_t cap):cap(std::max(MIN_CAP, cap)),comp(){
    initAllocate();
}

template<typename T, class Comp>
List<T,Comp>::List(uint32_t cap, Comp comp):cap(std::max(MIN_CAP, cap)),comp(){
    values=(T *)malloc(byteSize(cap));
    if(!values){
        printf("cannot malloc\n");
    }
}

template<typename T, class Comp>
List<T,Comp>::List(List &list):cap(list.cap),comp(list.comp),size_(list.size_){
    values=(T *)malloc(byteSize(cap));
    if(!values){
        printf("cannot malloc\n");
    }
    memcpy(values, list.values, byteSize(size_));
}

template<typename T, class Comp>
List<T,Comp>::List(const List &list):cap(list.cap),comp(list.comp),size_(list.size_){
    values=(T *)malloc(byteSize(cap));
    if(!values){
        printf("cannot malloc\n");
    }
    memcpy(values, list.values, byteSize(size_));
}

template<typename T, class Comp>
List<T,Comp>::~List(){
    free(values);
    values=NULL;
}


template<typename T, class Comp>
void List<T, Comp>::set(uint32_t index, const T &value){
    if (index<size_ && index>=0)
    {
        values[index] = value;
    }else{
        printf("index out of range\n");
        // throw "index out of range";
    }
}

template<typename T, class Comp>
void List<T,Comp>::add(const T &value){
    insert(size_, value);
}

template<typename T, class Comp>
void List<T,Comp>::add(const List<T, Comp> &list){
    insert(size_, list);
}

template<typename T, class Comp>
void List<T,Comp>::add(const List<T, Comp> &list, uint32_t startIndex){
    add(list,startIndex,list.size_-1);
}

template<typename T, class Comp>
void List<T,Comp>::add(const List<T, Comp> &list, uint32_t startIndex, uint32_t endIndex){
    insert(size_,list,startIndex,endIndex-startIndex+1);
}

template<typename T, class Comp>
void List<T, Comp>::insert(uint32_t index, const T &value){
    if(size_+1>=cap){
        expandTo(cap<<1); //cap*2
    }
    memmove(values+index+1,values+index,byteSize(size_-index));
    values[index] = value;
    size_++;
}

template<typename T, class Comp>
void List<T,Comp>::insert(uint32_t index, const List<T, Comp> &list){
    uint32_t cap = this->cap;
    while ((size_+list.size_)>=cap)
    {
        cap<<=1;
    }
    expandTo(cap);
    memmove(values+list.size_+index, values+index, byteSize(size_-index));
    memcpy(values+index,list.values,byteSize(list.size_));
    size_+=list.size_;
}

template<typename T, class Comp>
void List<T,Comp>::insert(int index, const List<T, Comp> &list, int startIndex, int count){
    if(count<=0){
        printf("invalidate insert count form another List\n");
        // throw "invalidate insert count form another List";
        return;
    }
    list.checkRange(startIndex);
    list.checkRange(startIndex+count-1);
    uint32_t cap = this->cap;
    while (cap<=size_+count)
    {
        cap<<=1;
    }
    expandTo(cap);

    memmove(values + count + index, values + index, byteSize(size_ - index));
    memcpy(values + index, list.values+startIndex, byteSize(count));
    size_ += count;
}

template<typename T, class Comp>
void List<T,Comp>::removeAt(int index){
    checkRange(index);
    memmove(values+index,values+index+1,byteSize(size_-index-1));
    size_--;
    autoTrim();
}

template<typename T, class Comp>
void List<T,Comp>::removeRange(int start, int count){
    if(count<=0){
        printf("invalidate count to remove from List: count: %d, start: %d\n",count,start);
        // throw "invalidate count to remove from List";
        return;
    }
    checkRange(start);
    checkRange(start+count-1);
    memmove(values+start,values+start+count,byteSize(size_-start-count));
    size_-=count;
    autoTrim();
}

template<typename T, class Comp>
void List<T,Comp>::clear(){
    size_ = 0;
    trimToSize();
}

template<typename T, class Comp>
void List<T,Comp>::reserve(uint32_t cap){
    this->cap = std::max(cap,MIN_CAP);
    expandTo(this->cap);
}
    
template<typename T, class Comp>    
uint32_t List<T,Comp>::binaryFind(const T &val) const{
    if(!size_){
        return 0;
    }
    uint32_t high = size_;
    uint32_t low = 0;
    uint32_t mid;
    int32_t c;
    while (low<high)
    {
        mid = (high+low)>>1;
        c = comp(val, values[mid]);
        if(c==0){
            return mid;
        }else if(c<0){
            high = mid;
        }else {
            low = mid+1;
        }
    }
    return low;
}

template<typename T, class Comp>
uint32_t List<T,Comp>::size() const{
    return size_;
}

template<typename T, class Comp>
bool List<T,Comp>::isEmpty(){
    return !size_;
}

template<typename T, class Comp>
T &List<T,Comp>::get(int index){
    checkRange(index);
    return values[index];
}

template<typename T, class Comp>
T &List<T, Comp>::operator[](uint32_t index) {
    checkRange(index);
    return values[index];
}

template<typename T, class Comp>
const T &List<T, Comp>::operator[](uint32_t index) const {
    checkRange(index);
    return values[index];
}

template<typename T, class Comp>
bool List<T,Comp>::remove(T &value){
    for (auto i = 0; i < size_; i++)
    {
        if(comp(values[i],value)==0){
            removeAt(i);
            return true;
        }
    }
    return false;
}

template<typename T, class Comp>
void List<T,Comp>::trimToSize(){
    if(cap>size_){
        cap = std::max(size_,MIN_CAP);
        values = (T *)realloc(values, byteSize(cap));
    }
}

template<typename T, class Comp>
inline void List<T,Comp>::autoTrim(){
    if(size_<(cap>>1)){
        trimToSize();
    }
}

template<typename T, class Comp>
size_t List<T,Comp>::byteSize(uint32_t count){
    return count*sizeof(T);
}

template<typename T, class Comp>
void List<T,Comp>::checkRange(int index) const{
    if(index<0||index>=size_){
        printf("index out of range\n");
        // throw "index out of size";
    }
}

template<typename T, class Comp>
void List<T, Comp>::expandTo(uint32_t cap) {
    values = (T *) realloc(values, byteSize(cap));
    if (!values) {
        printf("re-alloc failed!\n");
        // throw "re-alloc failed!";
    }
    this->cap = cap;
}

template<typename T, class Comp>
void List<T, Comp>::initAllocate() {
    values = (T *) malloc(byteSize(cap));
    if (!values) {
        printf("cannot malloc");
        // throw "cannot malloc";
    }
}

#endif // LIST_H