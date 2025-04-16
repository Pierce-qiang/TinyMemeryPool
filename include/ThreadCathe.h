#pragma once
#include "Common.h"
#include <array>
#include <cstddef>
#include <memory>


namespace TinyMemoryPool {

    class ThreadCathe{
        public:
        static ThreadCathe* getInstance(){
            static thread_local ThreadCathe instance;
            return &instance;
        }

        void* alloc(size_t size);
        void dealloc(void*ptr, size_t size);


        private:
        ThreadCathe() = default;
        //从中心缓存获取内存
        void* fetchFromCentralCathe(size_t index);
        //归还中心缓存
        void returnToCentralCathe(size_t index);
        //计算批量获取的内存
        size_t getBatchNum(size_t size);
        //判断是否需要归还给中心缓存
        bool shouldReturnToCentralCathe(size_t index);



        std::array<void*, FREE_LIST_SIZE> freeList_;
        std::array<size_t, FREE_LIST_SIZE> freeListSize_; //每个链表的大小

        
        

    };

}