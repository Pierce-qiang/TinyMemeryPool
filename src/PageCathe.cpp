#include "../include/PageCathe.h"
#include <cstring>
#include <sys/mman.h>

namespace TinyMemoryPool {
    void* PageCathe::allocSpan(size_t numPage){
        std::lock_guard<std::mutex> lock(mtx_);
        //找到第一个页数大于等于numPage的Span
        auto it = freeSpans_.lower_bound(numPage);
        if(it != freeSpans_.end()){
            Span* span = it->second;
            //从链表中取出
            if(span->next){
                freeSpans_[it->first] = span->next;
            }else{
                freeSpans_.erase(it);
            }
            // 如果span的页数大于numPage，将多余的部分重新加入freeSpans_
            if(span->numPages > numPage){
                Span* newSpan = new Span();
                newSpan->numPages = span->numPages - numPage;
                newSpan->start = (char*)span->start + numPage * PAGE_SIZE;
                newSpan->next = nullptr;

                //把新的span加入freeSpans_
                auto old = freeSpans_[newSpan->numPages];
                newSpan->next = old;
                freeSpans_[newSpan->numPages] = newSpan;
                span->numPages = numPage;
            }
            //记录分配出去的span的起始地址
            allSpans_[span->start] = span;
            return span->start;
        }
        //没有找到合适的span，向系统申请内存
        void* ptr = systemAlloc(numPage);
        if(ptr == nullptr){
            return nullptr;
        }
        Span* span = new Span();
        span->numPages = numPage;
        span->start = ptr;
        span->next = nullptr;
        //记录分配出去的span的起始地址
        allSpans_[ptr] = span;
        return ptr;
        
    }



    void PageCathe::deallocSpan(void* ptr, size_t numPage){
        std::lock_guard<std::mutex> lock(mtx_);
        //找到ptr对应的span
        auto it = allSpans_.find(ptr);
        if(it == allSpans_.end()){
            //没有找到 说明不是PageCathe管理的内存
            return;
        }
        Span* span = it->second;
        //尝试合并相邻的span
        void* nextAddr = (char*)ptr + numPage * PAGE_SIZE;
        auto nextIt = allSpans_.find(nextAddr);
        if(nextIt != allSpans_.end()){
            Span* nextSpan = nextIt->second;
            //如果相邻的span是空闲的，合并
            bool found = false;
            auto& nextList = freeSpans_[nextSpan->numPages];

            //检查是否是头结点
            if(nextList == nextSpan){
                nextList = nextSpan->next;
                found = true;
            }else if (nextList){
                Span* prev = nextList;
                Span* cur = nextList->next;
                while(cur){
                    if(cur == nextSpan){
                        prev->next = cur->next;
                        found = true;
                        break;
                    }
                    prev = cur;
                    cur = cur->next;
                }
            }
            if(found){
                span->numPages += nextSpan->numPages;
                allSpans_.erase(nextAddr);
                delete nextSpan;
            }
        }

        //将span加入freeSpans_
        auto old = freeSpans_[span->numPages];
        span->next = old;
        freeSpans_[span->numPages] = span;

    }




    void * PageCathe::systemAlloc(size_t numPage){
        size_t size = numPage * PAGE_SIZE;
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(ptr == MAP_FAILED){
            return nullptr;
        }
        //清零
        memset(ptr, 0, size);
        mmapMap_[ptr] = size;
        return ptr;
    }




    void PageCathe::releaseAll(){
        for(auto& p : mmapMap_){
            munmap(p.first, p.second);
        }
        for(auto& p : allSpans_){
            delete p.second;
        }
        allSpans_.clear();
        freeSpans_.clear();
        mmapMap_.clear();
    }
}