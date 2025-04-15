#pragma once

#include "PageCathe.h"
#include "ThreadCathe.h"
namespace TinyMemoryPool {

    class MemoryPool{
        public:
        static void* alloc(size_t size){
            return  ThreadCathe::getInstance()-> alloc(size);
        }
        static void dealloc(void *ptr, size_t size){
            ThreadCathe::getInstance()->dealloc(ptr,size);
        }

        static void destroy(){
            PageCathe::getInstance().releaseAll();
        }
    };

}
