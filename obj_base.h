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

#ifndef _OBJ_BASE_H_
#define _OBJ_BASE_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <stddef.h>
#include <stdint.h>
#include <new>
#include <stdexcept>
#include <memory>

#ifdef DISABLE_RTTI
#  define AUTO_CAST static_cast
#  define PDL_THROW(...)
#else
#  define AUTO_CAST dynamic_cast
#  define PDL_THROW(ERR) throw ERR
#endif

namespace pdl {

struct mem_allocator {
    virtual void * Allocate(uint32_t size) {
        return ::operator new (size);
    }
    virtual void Recycle(void * p) {
        ::operator delete (p);
    }
    static void Overlay(mem_allocator * memAllocator);
    static mem_allocator * Allocator();
};

template <typename OT>
struct obj_constructor {
    typedef OT obj_type;

    void Construct(obj_type * obj) {
        new (obj) obj_type;
    }

    static mem_allocator * Allocator() {
        return mem_allocator::Allocator();
    }
};

template <typename OT>
class std_allocator: public std::allocator<OT> {
public:
    typedef OT obj_type;
    typedef std::allocator<obj_type> my_base;
    typedef obj_constructor<obj_type> constructor_type;

    typedef typename my_base::size_type size_type;
    typedef typename my_base::pointer pointer;

    template <typename RT>
    struct rebind {
        typedef std_allocator<RT> other;
    };

    std_allocator() {}
    std_allocator(const std_allocator & src)
        : my_base(src)
    {}
    template <typename RT>
    std_allocator(const std_allocator<RT> & src)
        : my_base(src)
    {}

    pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
    {
        return static_cast<pointer>(
            constructor_type::Allocator()->Allocate( n * sizeof(obj_type) )
        );
    }
    void deallocate(pointer p, size_type n) {
        constructor_type::Allocator()->Recycle(p);
    }
};

class obj_base {
public:
    virtual const char * ContextName() const = 0;
    virtual uint32_t ContextSize() const = 0;

private:
    uint32_t mItemIndex;
    uint32_t mItemCount;

protected:
    obj_base() {}
    virtual ~obj_base() {}

public:
    // Should always call the function after constructed an object.
    static void InitItems(obj_base * target, uint32_t itemCount);
    // The index of this item.
    uint32_t ItemIndex() const {
        return mItemIndex;
    }
    // The count of remained items (include this item).
    uint32_t ItemCount() const {
        return mItemCount;
    }
    // The range of parameter 'i' is '- ItemIndex()' ~ 'ItemCount() - 1'.
    static obj_base * ItemAt(obj_base * target, int i);
};

template <typename OT>
class obj_ptr {
public:
    typedef OT obj_type;
    typedef obj_constructor<obj_type> constructor_type;

private:
    struct _ctx {
        uint32_t mRefCount;
        obj_type mObjects[1];
    };
    _ctx * mCtx;

    void createObjects(constructor_type * objConstructor, uint32_t objCount);
    void addRef();
    void release();
    OT * objPtr();

public:
    obj_ptr() {
        mCtx = 0;
    }
    obj_ptr(constructor_type * objConstructor, uint32_t objCount = 1) {
        if (objConstructor && objCount)
            createObjects(objConstructor, objCount);
        else
            mCtx = 0;
    }
    obj_ptr(const obj_ptr & src) {
        mCtx = src.mCtx;
        if (mCtx) {
            if (0 == mCtx->mRefCount + 1)
                mCtx = 0;
            else
                addRef();
        }
    }
    ~obj_ptr() {
        if (mCtx && mCtx->mRefCount)
            release();
    }
    obj_ptr & operator =(const obj_ptr & src) {
        if (mCtx)
            release();
        mCtx = src.mCtx;
        if (mCtx)
            addRef();
        return *this;
    }
    bool operator ==(const obj_ptr & src) const {
        return mCtx == src.mCtx;
    }
    bool operator !=(const obj_ptr & src) const {
        return mCtx != src.mCtx;
    }
    bool operator !() const {
        return !mCtx;
    }
    operator bool() {
        return static_cast<bool>(mCtx);
    }
    operator bool() const {
        return static_cast<bool>(mCtx);
    }
    obj_type * operator ->() {
        return objPtr();
    }
    const obj_type * operator ->() const {
        return const_cast<obj_ptr *>(this)->objPtr();
    }
    operator OT *() {
        return objPtr();
    }
    operator const OT *() const {
        return const_cast<obj_ptr *>(this)->objPtr();
    }
    obj_type & operator *() {
        return * objPtr();
    }
    const obj_type & operator *() const {
        return * const_cast<obj_ptr *>(this)->objPtr();
    }
    operator OT &() {
        return * objPtr();
    }
    operator const OT &() const {
        return * const_cast<obj_ptr *>(this)->objPtr();
    }
    obj_type & operator [] (int i) {
        return * static_cast<obj_type *>(
            obj_base::ItemAt( objPtr(), i )
        );
    }
    const obj_type & operator [] (int i) const {
        return * static_cast<obj_type *>(
            obj_base::ItemAt( const_cast<obj_ptr *>(this)->objPtr(), i )
        );
    }
};

template <typename OT>
void obj_ptr<OT>::createObjects(
    constructor_type * objConstructor, uint32_t objCount)
{
    mCtx = static_cast<_ctx *>(
        constructor_type::Allocator()->Allocate(
            sizeof(_ctx) + sizeof(obj_type) * (objCount - 1)
        )
    );
    mCtx->mRefCount = 1;
    for (uint32_t i = 0; i < objCount; ++i)
        objConstructor->Construct(mCtx->mObjects + i);
    obj_base::InitItems(mCtx->mObjects, objCount);
}

template <typename OT>
void obj_ptr<OT>::addRef() {
    if (0 == mCtx->mRefCount + 1) {
        PDL_THROW( std::overflow_error(
            std::string("obj_ptr<") +
            mCtx->mObjects->ContextName() +
            ">::addRef() overflow!"
        ) );
    } else
        ++(mCtx->mRefCount);
}

template <typename OT>
void obj_ptr<OT>::release() {
    if (0 == mCtx->mRefCount) {
        PDL_THROW( std::underflow_error(
            std::string("obj_ptr<") +
            mCtx->mObjects->ContextName() +
            ">::release() underflow!"
        ) );
    } else if ( 0 == --(mCtx->mRefCount) ) {
        for (uint32_t i = 0; i < mCtx->mObjects->ItemCount(); ++i)
            mCtx->mObjects[i].~OT();
        constructor_type::Allocator()->Recycle(mCtx);
    }
    mCtx = 0;
}

template <typename OT>
OT * obj_ptr<OT>::objPtr() {
    if (mCtx)
        return mCtx->mObjects;
    static OT obj;
    PDL_THROW( std::runtime_error(
        std::string("obj_ptr<") + obj.ContextName() + ">::objPtr() null!"
    ) );
    return 0;
}

} // namespace pdl

#endif // _OBJ_BASE_H_

