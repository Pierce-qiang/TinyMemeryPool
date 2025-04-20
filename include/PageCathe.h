#pragma once

#include "Common.h"
#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace TinyMemoryPool {
    class PageCathe{
        public:

        static const size_t PAGE_SIZE = 4 * 1024;
        static PageCathe& getInstance(){
            static PageCathe instance;
            return instance;
        }

        void* allocSpan(size_t numPage);
        void deallocSpan(void* ptr, size_t numPage);


        private:

        PageCathe() = default;
        //向系统申请内存
        void * systemAlloc(size_t numPage);
        struct Span{
            Span* next; //下一个Span
            size_t numPages; //页数
            void* start; //页起始地址
        };
        //按页数管理Span，不同页数不同span链表
        std::map<size_t, Span*> freeSpans_;
        //页起始地址到Span的映射
        std::map<void*, Span*> allSpans_;
        std::mutex mtx_;
        std::unordered_map<void*, size_t> mmapMap_;
        //释放所有内存
        void releaseAll();
        friend class MemoryPool;
    };
}