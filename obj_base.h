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
#include <string.h>

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
template <>
struct obj_constructor<void *> {
    typedef void * obj_type;

    const void * mArgs;
    uint32_t mArgCount;

    obj_constructor() {
        mArgs = 0;
        mArgCount = 0;
    }
    obj_constructor(const void * arg) {
        mArgs = arg;
        mArgCount = 1;
    }
    obj_constructor(const void * args, uint32_t argCount) {
        mArgs = args;
        mArgCount = argCount;
    }

    void Construct(void * obj) {
        if (mArgCount > 1)
            memcpy( obj, mArgs, mArgCount * sizeof(void *) );
        else
            *static_cast<const void **>(obj) = mArgs;
    }

    static mem_allocator * Allocator() {
        return mem_allocator::Allocator();
    }
};

template <typename OT, typename AT = OT>
class std_allocator: public std::allocator<OT> {
public:
    typedef OT obj_type;
    typedef std::allocator<OT> my_base;
    typedef obj_constructor<AT> constructor_type;

    typedef typename my_base::size_type size_type;
    typedef typename my_base::pointer pointer;

    template <typename RT>
    struct rebind {
        typedef std_allocator<RT,AT> other;
    };

    std_allocator() {}
    std_allocator(const std_allocator & src)
        : my_base(src)
    {}
    template <typename RT>
    std_allocator(const std_allocator<RT,AT> & src)
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

template <typename OT, typename IT>
class obj_ptr_base {
};
template <typename OT>
class obj_ptr_base<OT,obj_base *> {
public:
    typedef OT obj_type;
    typedef obj_constructor<OT> constructor_type;

protected:
    struct _ctx {
        uint32_t mRefCount;
        obj_type mObjects[1];
    };
    _ctx * mCtx;

    void createCtx(constructor_type * objConstructor, uint32_t objCount);
    void freeCtx();
    obj_type * objBegin();

public:
    uint32_t ObjCount() const {
        return mCtx? mCtx->mObjects->ItemCount(): 0;
    }
    obj_type * ObjAt(uint32_t i);
};
template <typename OT>
class obj_ptr_base<OT,void *> {
public:
    typedef OT obj_type;
    typedef obj_constructor<OT> constructor_type;

protected:
    struct _ctx {
        uint32_t mRefCount;
        uint32_t mObjCount;
        obj_type mObjects[1];
    };
    _ctx * mCtx;

    void createCtx(constructor_type * objConstructor, uint32_t objCount);
    void freeCtx();
    obj_type * objBegin();

public:
    uint32_t ObjCount() const {
        return mCtx? mCtx->mObjCount: 0;
    }
    obj_type * ObjAt(uint32_t i);
};
template <typename IT>
class obj_ptr_base<obj_base *, IT> {
public:
    typedef IT obj_type;
    typedef obj_constructor<void *> constructor_type;

protected:
    struct _ctx {
        uint32_t mRefCount;
        obj_type * mObjects;
    };
    _ctx * mCtx;

    void createCtx(constructor_type * objConstructor, uint32_t objCount);
    void freeCtx();
    obj_type * objBegin();

public:
    uint32_t ObjCount() const {
        return (mCtx && mCtx->mObjects)? mCtx->mObjects->ItemCount(): 0;
    }
    obj_type * ObjAt(uint32_t i);
};
template <typename IT>
class obj_ptr_base<void *, IT> {
public:
    typedef IT obj_type;
    typedef obj_constructor<void *> constructor_type;

protected:
    struct _ctx {
        uint32_t mRefCount;
        uint32_t mObjCount;
        obj_type * mObjects[1];
    };
    _ctx * mCtx;

    void createCtx(constructor_type * objConstructor, uint32_t objCount);
    void freeCtx();
    obj_type * objBegin();

public:
    uint32_t ObjCount() const {
        return mCtx? mCtx->mObjCount: 0;
    }
    obj_type * ObjAt(uint32_t i);
};

template <typename OT>
void obj_ptr_base<OT,obj_base *>::createCtx(
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
void obj_ptr_base<OT,void *>::createCtx(
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
    mCtx->mObjCount = objCount;
}

template <typename IT>
void obj_ptr_base<obj_base *, IT>::createCtx(
    constructor_type * objConstructor, uint32_t objCount)
{
    mCtx = static_cast<_ctx *>(
        constructor_type::Allocator()->Allocate( sizeof(_ctx) )
    );
    mCtx->mRefCount = 1;
    objConstructor->Construct( &(mCtx->mObjects) );
}

template <typename IT>
void obj_ptr_base<void *, IT>::createCtx(
    constructor_type * objConstructor, uint32_t objCount)
{
    mCtx = static_cast<_ctx *>(
        constructor_type::Allocator()->Allocate(
            sizeof(_ctx) + sizeof(obj_type *) * (objCount - 1)
        )
    );
    mCtx->mRefCount = 1;
    objConstructor->Construct(mCtx->mObjects);
    mCtx->mObjCount = objCount;
}

template <typename OT>
void obj_ptr_base<OT,obj_base *>::freeCtx() {
    uint32_t i, n;
    for (i = 0, n = ObjCount(); i < n; ++i)
        ObjAt(i)->~OT();
    constructor_type::Allocator()->Recycle(mCtx);
}

template <typename OT>
void obj_ptr_base<OT,void *>::freeCtx() {
    uint32_t i, n;
    for (i = 0, n = ObjCount(); i < n; ++i)
        ObjAt(i)->~OT();
    constructor_type::Allocator()->Recycle(mCtx);
}

template <typename IT>
void obj_ptr_base<obj_base *, IT>::freeCtx() {
    if ( mCtx->mObjects && 0 == mCtx->mObjects->ItemIndex() ) {
        uint32_t i, n;
        for (i = 0, n = ObjCount(); i < n; ++i)
            ObjAt(i)->~IT();
        constructor_type::Allocator()->Recycle(mCtx->mObjects);
    }
    constructor_type::Allocator()->Recycle(mCtx);
}

template <typename IT>
void obj_ptr_base<void *, IT>::freeCtx() {
    uint32_t i, n;
    for (i = 0, n = ObjCount(); i < n; ++i) {
        ObjAt(i)->~IT();
        constructor_type::Allocator()->Recycle( ObjAt(i) );
    }
    constructor_type::Allocator()->Recycle(mCtx);
}

template <typename OT>
OT * obj_ptr_base<OT,obj_base *>::objBegin() {
    if (mCtx)
        return mCtx->mObjects;
    PDL_THROW( std::runtime_error(
        std::string(__FUNCTION__) + " null!"
    ) );
    return 0;
}

template <typename OT>
OT * obj_ptr_base<OT,void *>::objBegin() {
    if (mCtx)
        return mCtx->mObjects;
    PDL_THROW( std::runtime_error(
        std::string(__FUNCTION__) + " null!"
    ) );
    return 0;
}

template <typename IT>
IT * obj_ptr_base<obj_base *, IT>::objBegin() {
    if (mCtx && mCtx->mObjects)
        return mCtx->mObjects;
    PDL_THROW( std::runtime_error(
        std::string(__FUNCTION__) + " null!"
    ) );
    return 0;
}

template <typename IT>
IT * obj_ptr_base<void *, IT>::objBegin() {
    if (mCtx)
        return mCtx->mObjects[0];
    PDL_THROW( std::runtime_error(
        std::string(__FUNCTION__) + " null!"
    ) );
    return 0;
}

template <typename OT>
OT * obj_ptr_base<OT,obj_base *>::ObjAt(uint32_t i) {
    if ( objBegin() ) {
        if ( i < mCtx->mObjects->ItemCount() )
            return mCtx->mObjects + i;
        PDL_THROW( std::out_of_range(
            std::string(__FUNCTION__) + " out of range!"
        ) );
    }
    return 0;
}

template <typename OT>
OT * obj_ptr_base<OT,void *>::ObjAt(uint32_t i) {
    if ( objBegin() ) {
        if (i < mCtx->mObjCount)
            return mCtx->mObjects + i;
        PDL_THROW( std::out_of_range(
            std::string(__FUNCTION__) + " out of range!"
        ) );
    }
    return 0;
}

template <typename IT>
IT * obj_ptr_base<obj_base *, IT>::ObjAt(uint32_t i) {
    if ( objBegin() ) {
        return static_cast<obj_type *>(
            obj_base::ItemAt( mCtx->mObjects, (int) i )
        );
    }
    return 0;
}

template <typename IT>
IT * obj_ptr_base<void *, IT>::ObjAt(uint32_t i) {
    if ( objBegin() ) {
        if (i < mCtx->mObjCount)
            return mCtx->mObjects[i];
        PDL_THROW( std::out_of_range(
            std::string(__FUNCTION__) + " out of range!"
        ) );
    }
    return 0;
}

template <typename OT, typename IT = obj_base *>
class obj_ptr: public obj_ptr_base<OT,IT> {
    typedef obj_ptr_base<OT,IT> my_base;
    typedef typename my_base::_ctx _ctx;
    using my_base::mCtx;
    using my_base::createCtx;
    using my_base::freeCtx;
    using my_base::objBegin;

public:
    typedef typename my_base::obj_type obj_type;
    typedef typename my_base::constructor_type constructor_type;
    using my_base::ObjAt;

    const obj_type * ObjAt(uint32_t i) const {
        return const_cast<obj_ptr *>(this)->ObjAt(i);
    }

private:
    void addRef();
    void release();
    const obj_type * objBegin() const {
        return const_cast<obj_ptr *>(this)->objBegin();
    }

public:
    obj_ptr() {
        mCtx = 0;
    }
    obj_ptr(constructor_type * objConstructor, uint32_t objCount = 1) {
        if (objConstructor && objCount)
            createCtx(objConstructor, objCount);
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
        return objBegin();
    }
    const obj_type * operator ->() const {
        return objBegin();
    }
    operator OT *() {
        return objBegin();
    }
    operator const OT *() const {
        return objBegin();
    }
    obj_type & operator *() {
        return * objBegin();
    }
    const obj_type & operator *() const {
        return * objBegin();
    }
    operator OT &() {
        return * objBegin();
    }
    operator const OT &() const {
        return * objBegin();
    }
    obj_type & operator [] (uint32_t i) {
        return *ObjAt(i);
    }
    const obj_type & operator [] (uint32_t i) const {
        return *ObjAt(i);
    }
};

template <typename OT, typename IT>
void obj_ptr<OT,IT>::addRef() {
    if (0 == mCtx->mRefCount + 1)
        PDL_THROW( std::overflow_error(
            std::string(__FUNCTION__) + " overflow!"
        ) );
    else
        ++(mCtx->mRefCount);
}

template <typename OT, typename IT>
void obj_ptr<OT,IT>::release() {
    if (0 == mCtx->mRefCount)
        PDL_THROW( std::underflow_error(
            std::string(__FUNCTION__) + " underflow!"
        ) );
    else if ( 0 == --(mCtx->mRefCount) )
        freeCtx();
    mCtx = 0;
}

} // namespace pdl

#endif // _OBJ_BASE_H_

