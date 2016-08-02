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

#include "value_obj.h"

using namespace pdl;

extern mem_allocator * gValObjAllocator;

void value_obj::OverlayAllocator(mem_allocator * memAllocator) {
    gValObjAllocator = memAllocator;
}

mem_allocator * value_obj::Allocator() {
    return gValObjAllocator;
}

void value_obj::Reset() {
    if (mValImp) {
        mValImp->~val_base();
        gValObjAllocator->Recycle(mValImp);
        mValType = NUL_VAL;
        mValImp = 0;
    }
}

void buf_val::Resize(uint32_t size, bool cleanBuf) {
    if ( size > Size() ) {
        if (cleanBuf) {
            reset();
            mVal = static_cast<_ctx *>(
                value_obj::Allocator()->Allocate( sizeof(uint32_t) + size )
            );
        } else {
            _ctx * newVal = static_cast<_ctx *>(
                value_obj::Allocator()->Allocate( sizeof(uint32_t) + size )
            );
            if (mVal) {
                memcpy(newVal->mBuf, mVal->mBuf, mVal->mSize);
                reset();
            }
            mVal = newVal;
        }
        mVal->mSize = size;
    }
}

#ifdef VALUE_OBJ_UT

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
    obj_constructor<value_obj> valObjConstructor;
    obj_ptr<value_obj> valObj(&valObjConstructor);
    obj_val<test_obj> * objVal = val_itf_selector<test_obj>::GetInterface(valObj);
    obj_constructor<test_obj> testObjConstructor;
    objVal->CreateObj(&testObjConstructor, 2);
    std::cout << "set value as " << valObj->GetTypeName() << ": " << \
        objVal->ObjPtr()[0].ItemCount() << std::endl;
    valObj->Reset();
    str_val * strVal = val_itf_selector<str_val>::GetInterface(valObj);
    std::cout << "change value type as " << valObj->GetTypeName() << std::endl;
    strVal->Resize(10);
    strncpy(strVal->Str(), "'hello!'", 10);
    std::cout << "set value as " << strVal->Str() << std::endl;
    return 0;
}

#endif // VALUE_OBJ_UT

