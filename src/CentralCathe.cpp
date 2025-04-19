#include "../include/CentralCathe.h"
#include "../include/PageCathe.h"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>

namespace TinyMemoryPool {
    //每次从pageCathe获取的页数
    static const size_t SPAN_PAGES = 8;

    //TODO 可能存在问题
    void* CentralCathe::fetchRange(size_t index, size_t& batchNum){
        if(index >= FREE_LIST_SIZE||batchNum == 0){
            return nullptr;
        }
        //加锁
        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        void* res = nullptr;
        try{
            //从中心缓存获取
            res = centralFreeList_[index].load(std::memory_order_relaxed);
            if(!res){
                //没有就从pageCathe获取
                auto size = SizeClass::size(index);
                res = fetchFromPageCathe(size);
                if(!res){
                    locks_[index].clear(std::memory_order_release);
                    return nullptr;
                }

                //切割页缓存
                char* start = static_cast<char*>(res);
                size_t totalBlocks = (SPAN_PAGES * PageCathe::PAGE_SIZE) / size;
                size_t allocBlocks = std::min(totalBlocks, batchNum);

                // 构建链表
                if(allocBlocks > 1){
                    char* cur = start;
                    for(size_t i = 0; i < allocBlocks - 1; i++){
                        char* next = cur + size;
                        *reinterpret_cast<void**>(cur) = next;
                        cur = next;
                    }
                    *reinterpret_cast<void**>(start + (allocBlocks-1)*size) = nullptr;
                }

                //分割链表,保留剩余部分
                if(totalBlocks > allocBlocks){
                    void* remain = start + allocBlocks * size;
                    for(size_t i = allocBlocks; i < totalBlocks - 1; i++){
                        void* cur = start + i * size;
                        void* next = start + (i + 1) * size;
                        *reinterpret_cast<void**>(cur) = next;
                    }
                    *reinterpret_cast<void**>(start + (totalBlocks - 1) * size) = nullptr;


                    centralFreeList_[index].store(remain, std::memory_order_release);
                }
                batchNum = allocBlocks;

            }
            else{//中心缓存有
                void* cur = res;
                void* prev = nullptr;
                size_t cnt = 0;
                while(cur && cnt < batchNum){
                    prev = cur;
                    cur = *reinterpret_cast<void**>(cur);
                    cnt++;
                }
                if(prev){
                    *reinterpret_cast<void**>(prev) = nullptr;
                }
                batchNum = cnt;
                centralFreeList_[index].store(cur, std::memory_order_release);
            }
        }
        catch(...){
            locks_[index].clear(std::memory_order_release);
            throw;
        }
        //解锁
        locks_[index].clear(std::memory_order_release);
        return res;
    }

    void* CentralCathe::fetchFromPageCathe(size_t size){
        //计算需要的页数
        size_t numPage = (size + PageCathe::PAGE_SIZE - 1) / PageCathe::PAGE_SIZE;

        //如果小于SPAN_PAGES就直接分配 页缓存最小单位32KB
        if(size<PageCathe::PAGE_SIZE*SPAN_PAGES){
            return PageCathe::getInstance().allocSpan(SPAN_PAGES);
        }
        else{
            //大于32KB就直接分配
            return PageCathe::getInstance().allocSpan(numPage);
        }
    }

    void CentralCathe::returnRange(void* ptr, size_t index, size_t batchNum){
        //索引大于FreeListSize或者batchNum为0直接返回 异常情况
        if(!ptr || index >= FREE_LIST_SIZE){
            return;
        }
        //加锁
        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        try{
            void* cur = ptr;
            while (*reinterpret_cast<void**>(cur)!=nullptr){
                cur = *reinterpret_cast<void**>(cur);
            }
            void * old = centralFreeList_[index].load(std::memory_order_relaxed);
            *reinterpret_cast<void**>(cur) = old;
            
            centralFreeList_[index].store(ptr, std::memory_order_release);
        }
        catch(...){
            locks_[index].clear(std::memory_order_release);
            throw;
        }
        //解锁
        locks_[index].clear(std::memory_order_release);
    }

}