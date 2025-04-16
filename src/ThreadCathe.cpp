
#include "../include/ThreadCathe.h"
#include "../include/CentralCathe.h"
#include <cstdlib>

namespace TinyMemoryPool {
    //线程局部不需要考虑多线程问题
    void* ThreadCathe::alloc(size_t size){
        if(size ==0){//不能为0，至少Alignment大小
            size = ALIGNMENT;
        }
        if(size > MAX_BYTES){ //超过最大大小就直接分配
            return malloc(size);
        }
        auto index = SizeClass::index(size);
        freeListSize_[index]--;
        if(void* ptr = freeList_[index]){
            freeList_[index] = *reinterpret_cast<void**>(ptr);//取出下一个指针
            return ptr;//返回前一个指针
        }
        //没有就从中心缓存获取
        return fetchFromCentralCathe(index);
        
    }
    //从中心缓存获取内存
    void* ThreadCathe::fetchFromCentralCathe(size_t index){
        size_t size = (index+1)*ALIGNMENT;
        //计算批量获取的内存
        size_t batchNum = getBatchNum(size);
        //从中心缓存获取
        void* start  = CentralCathe::getInstance().fetchRange(index,batchNum);
        if(!start){
            return nullptr;
        }

        //更新freeList
        freeListSize_[index] += batchNum;
        void* res = start;
        if(batchNum>1){
            freeList_[index] = *reinterpret_cast<void**>(start);
        }
        return res;
    }




    void ThreadCathe::dealloc(void*ptr, size_t size){
        if(size ==0){
            size = ALIGNMENT;
        }
        if(size > MAX_BYTES){
            free(ptr);
            return;
        }
        auto index = SizeClass::index(size);
        //头插法
        (*reinterpret_cast<void**>(ptr)) = freeList_[index];
        freeList_[index] = ptr;
        freeListSize_[index]++;

        //判断是否需要归还部分给中心缓存
        if(shouldReturnToCentralCathe(index)){
            returnToCentralCathe(index);
        }
    }


    //归还中心缓存
    void ThreadCathe::returnToCentralCathe(size_t index){
        //根据freeListSize_的大小判断是否需要归还

        if(freeListSize_[index] <=10){//小于10就不归还
            return;
        }
        size_t batchNum = freeListSize_[index];
        //保留1/4
        size_t keepNum = batchNum/4;
        size_t returnNum = batchNum - keepNum;
        //直接摘掉后面的3/4
        void* end = freeList_[index];
        for(size_t i = 0; i<keepNum-1; i++){//TODO为什么会异常？
            end = *reinterpret_cast<void**>(end);
        }
        void* tmp = end;
        *reinterpret_cast<void**>(tmp) = nullptr;//断开连接
        end = *reinterpret_cast<void**>(end);
        freeListSize_[index] = keepNum;
        CentralCathe::getInstance().returnRange(end, index,returnNum);
    }

    //判断是否需要归还给中心缓存
    bool ThreadCathe::shouldReturnToCentralCathe(size_t index){
        // 超过64个就归还给中心缓存
        // TODO 万一频繁介入这个中间值呢？不会，因为一次归还3/4
        size_t threshold = 64;
        return freeListSize_[index] > threshold;
    }
    //计算批量获取的内存
    size_t ThreadCathe::getBatchNum(size_t size){
        //计算批量获取的内存 不超4KB
        constexpr size_t MAX_BATCH_SIZE = 4*1024;
        // 根据对象大小设置合理的基准批量数
        size_t baseNum;
        if (size <= 32) baseNum = 64;    // 64 * 32 = 2KB
        else if (size <= 64) baseNum = 32;  // 32 * 64 = 2KB
        else if (size <= 128) baseNum = 16; // 16 * 128 = 2KB
        else if (size <= 256) baseNum = 8;  // 8 * 256 = 2KB
        else if (size <= 512) baseNum = 4;  // 4 * 512 = 2KB
        else if (size <= 1024) baseNum = 2; // 2 * 1024 = 2KB
        else baseNum = 1;                   // 大于1024的对象每次只从中心缓存取1个

        // 计算最大获取的批量数
        size_t maxNum = MAX_BATCH_SIZE / size;
        return baseNum < maxNum ? baseNum : maxNum;

    }
}