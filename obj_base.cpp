/* Copyright (c) 2016 Qing Li

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include <stdio.h>
#include "obj_base.h"

using namespace pdl;

static mem_allocator gDefaultMemAllocator;
static mem_allocator * gMemAllocator = &gDefaultMemAllocator;

mem_allocator * gValObjAllocator = gMemAllocator;
mem_allocator * gFieldDesAllocator = gMemAllocator;

void mem_allocator::Overlay(mem_allocator * memAllocator) {
    gFieldDesAllocator = gValObjAllocator = gMemAllocator = memAllocator;
}

mem_allocator * mem_allocator::Allocator() {
    return gMemAllocator;
}

void obj_base::InitItems(obj_base * target, uint32_t itemCount) {
    if (target && itemCount) {
        uint32_t i = 0;
        do {
            target->mItemIndex = i;
            target->mItemCount = itemCount - i;
            if (++i == itemCount)
                break;
            target = obj_base::ItemAt(target, 1);
        } while (true);
    } else {
        PDL_THROW( std::invalid_argument(
            std::string( target->ContextName() ) +
            "::InitItems() invalid args!"
        ) );
    }
}

obj_base * obj_base::ItemAt(obj_base * target, int i) {
    if ( (i > 0 && i >= target->mItemCount) ||
        (i < 0 && -i > target->mItemIndex) )
    {
        PDL_THROW( std::out_of_range(
            std::string( target->ContextName() ) +
            "::ItemAt() out of range!"
        ) );
        return 0;
    }

    if (i) {
        uint8_t * objAddr = reinterpret_cast<uint8_t *>(target);
        objAddr += i * target->ContextSize();
        target = reinterpret_cast<obj_base *>(objAddr);
    }
    return target;
}

#ifdef OBJ_BASE_UT

#include <iostream>

class test_obj: public obj_base {
public:
    test_obj(int x = 0) {}
    virtual ~test_obj() {}
    virtual const char * ContextName() const {
        return "test_obj";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(test_obj);
    }
};

namespace pdl {

template<>
struct obj_constructor<test_obj> {
    int mArg;

    void Construct(test_obj * obj) {
        new (obj) test_obj(mArg);
    }

    static mem_allocator * Allocator() {
        return mem_allocator::Allocator();
    }
};

}

int main(void) {
    obj_ptr<test_obj> testObj;
    obj_constructor<test_obj> testObjConstructor;
    testObj = obj_ptr<test_obj>(&testObjConstructor, 2);
    std::cout << testObj->ContextName() << "-" << testObj[1].ItemIndex() << std::endl;
    return testObj->ItemCount();
}

#endif // OBJ_BASE_UT

