#pragma once

#include <windows.h>

#define CACHE_LINE_SIZE 64
#define alignCache  __declspec(align(CACHE_LINE_SIZE))
#ifdef _WIN64
#   define alignArch  __declspec(align( 8))
#else
#   define alignArch  __declspec(align( 4))
#endif

class InterlockedFlag {
    protected:
        alignArch volatile unsigned int value;
    public:
        inline void set(unsigned int val) {
            this->value = val;
        }
        inline unsigned int exchange(unsigned int val) {
            return InterlockedExchange(&this->value,val);
        }
};

#pragma pack(push,1)
template <typename T> struct ObjectPoolNode {
    ObjectPoolNode<T>* next;
    T data;
    ObjectPoolNode() : next(nullptr) { };
};
#pragma pack(pop,1)

template <typename T> struct alignCache ObjectPoolList {
    ObjectPoolList<T>* nextList;
    char pad1[CACHE_LINE_SIZE - sizeof(ObjectPoolList<T>*)];
    ObjectPoolNode<T>* first;
    char pad2[CACHE_LINE_SIZE - sizeof(ObjectPoolNode<T>*)];
    InterlockedFlag consumerLock;
    char pad3[CACHE_LINE_SIZE - sizeof(InterlockedFlag)];
    ObjectPoolNode<T>* last;
    char pad4[CACHE_LINE_SIZE - sizeof(ObjectPoolNode<T>*)];
    InterlockedFlag producerLock;
    char pad5[CACHE_LINE_SIZE - sizeof(InterlockedFlag)];
    ObjectPoolNode<T>** storage;
    char pad6[CACHE_LINE_SIZE - sizeof(ObjectPoolNode<T>**)];
    size_t available;
    size_t count;

    ObjectPoolList(size_t count)
        : producerLock(false), consumerLock(false)
    {
        this->available = this->count = count;
        this->storage = new ObjectPoolNode<T>*[count+1];
        for(size_t i=0 ; i<count+1 ; i++) {
            this->storage[i] = new ObjectPoolNode<T>;
        }
        for(size_t i=0 ; i<count ; i++) {
            this->storage[i]->next = this->storage[i+1];
        }
        this->first = this->storage[0];
        this->last  = this->storage[count];
    }

    ~ObjectPoolList() {
        this->count = 0;
        this->available = 0;
        if(this->storage) {
            for(size_t i=0 ; i<count+1 ; i++) {
                delete this->storage[i];
            }
            delete[] this->storage;
            this->storage = NULL;
        }
    }
};

template <typename T> class alignCache ObjectPool {
private:
    ObjectPoolList<T>** lists;
    char pad1[CACHE_LINE_SIZE - sizeof(ObjectPoolList<T>**)];
    size_t available;
    size_t listCount;
public:
    ObjectPool(size_t count,size_t parallelCount = 0) {
        this->available = count;
        this->listCount = parallelCount;
        if(this->listCount == 0) {
            this->listCount = getSystemLogicalProcessor(); //default
        }
        this->lists = new ObjectPoolList<T>*[this->listCount];
        for(size_t i=0 ; i<this->listCount ; i++) {
            this->lists[i] = new ObjectPoolList<T>(count/this->listCount);
        }
        for(size_t i=0 ; i<this->listCount-1 ; i++) {
            this->lists[i]->nextList = this->lists[i+1];
        }
        this->lists[this->listCount-1]->nextList = this->lists[0];
    }

    ~ObjectPool() {
        if(this->lists) {
            for(size_t i=0 ; i<this->listCount ; i++) {
                delete this->lists[i];
            }
            delete[] this->lists;
            this->lists = NULL;
        }
        this->available = 0;
        this->listCount = 0;
    }

    T* borrowObj() {
        ObjectPoolList<T>* list = this->lists[0];
        while( !list->available || list->consumerLock.exchange(true) ) {
            if(!this->available) {
                return NULL;
            }
            list = list->nextList;
        }
        if(list->first->next) {
            ObjectPoolNode<T>* usedNode = list->first;
            list->first = list->first->next;
            list->available--;
            this->available--;
            list->consumerLock.set(false);
            usedNode->next = nullptr;
            return &usedNode->data;
        }
        list->consumerLock.set(false);
        return NULL;
    }

    void returnObj(T* object) {
        ObjectPoolNode<T>* node = (ObjectPoolNode<T>*)(((char*)object) - sizeof(ObjectPoolNode<T>*));
        ObjectPoolList<T>* list = this->lists[0];
        while( list->producerLock.exchange(true) ) {
            list = list->nextList;
        }
        list->last->next = node;
        list->last       = node;
        list->producerLock.set(false);
        list->available++;
        this->available++;
    }
};

