#pragma once
#include "Common.h"

//需要同步
#include <array>
#include <atomic>
#include <mutex>

namespace TinyMemoryPool {
    class CentralCathe{
        public:
        static CentralCathe& getInstance(){
            static CentralCathe instance;
            return instance;
        }

        void* fetchRange(size_t index, size_t& batchNum);
        void returnRange(void* ptr, size_t index, size_t batchNum);

        private:
        CentralCathe(){
            for(size_t i = 0; i < FREE_LIST_SIZE; i++){
                centralFreeList_[i].store(nullptr, std::memory_order_relaxed);
                locks_[i].clear();
            }
        }
        void* fetchFromPageCathe(size_t size);



        std::array<std::atomic<void*>, FREE_LIST_SIZE> centralFreeList_;

        //不同大小不同锁
        std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;
    };

}