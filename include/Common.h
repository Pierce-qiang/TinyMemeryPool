
#pragma once
#include <cstddef>

namespace TinyMemoryPool {
    
    constexpr size_t ALIGNMENT = 8;
    constexpr size_t MAX_BYTES = 256 * 1024;//256KB
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES/ALIGNMENT; //内存池链表长度

    //辅助计算类
    class SizeClass{
        public:
        //根据size计算index
        static size_t index(size_t size){
            return (size + ALIGNMENT - 1) / ALIGNMENT - 1;
        }
        //根据size向上取整并舍去alignment位数之后的数
        static size_t roundUp(size_t size){
            return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }
        //根据index计算size
        static size_t size(size_t index){
            return (index + 1) * ALIGNMENT;
        }
    };



}