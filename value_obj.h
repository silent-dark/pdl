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

#ifndef _VALUE_OBJ_H_
#define _VALUE_OBJ_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <string.h>
#include "obj_base.h"

namespace pdl {

struct val_base {
    virtual const char * TypeName() const = 0;
    virtual ~val_base() {};
};

class value_obj: public obj_base {
    template <typename IT, int VT>
    friend struct val_itf_binder;

public:
    enum val_type {
        NUL_VAL,
        BLN_VAL,
        INT_VAL,
        INT64_VAL,
        FLT_VAL,
        FLT64_VAL,
        BUF_VAL,
        STR_VAL,
        WCS_VAL,
        OBJ_VAL
    };

    virtual const char * ContextName() const {
        return "value_obj";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(value_obj);
    }

    static void OverlayAllocator(mem_allocator * memAllocator);
    static mem_allocator * Allocator();

private:
    int mValType;
    val_base * mValImp;

public:
    value_obj() {
        mValType = NUL_VAL;
        mValImp = 0;
    }
    virtual ~value_obj() {
        Reset();
    }
    int GetValType() const {
        return mValType;
    }
    const char * GetTypeName() const {
        return mValImp? mValImp->TypeName(): "nul_val";
    }
    void Reset();
};

class bln_val: public val_base {
    bool mVal;

public:
    bln_val() {
        mVal = 0;
    }
    virtual const char * TypeName() const {
        return "bln_val";
    }
    bool & Val() {
        return mVal;
    }
    bool Val() const {
        return mVal;
    }
};

class int_val: public val_base {
    uint32_t mVal;

public:
    int_val() {
        mVal = 0;
    }
    virtual const char * TypeName() const {
        return "int_val";
    }
    uint32_t & Val() {
        return mVal;
    }
    uint32_t Val() const {
        return mVal;
    }
};

class int64_val: public val_base {
    uint64_t mVal;

public:
    int64_val() {
        mVal = 0;
    }
    virtual const char * TypeName() const {
        return "int64_val";
    }
    uint64_t & Val() {
        return mVal;
    }
    uint64_t Val() const {
        return mVal;
    }
};

class flt_val: public val_base {
    double mVal;

public:
    flt_val() {
        mVal = 0.0;
    }
    virtual const char * TypeName() const {
        return "flt_val";
    }
    double & Val() {
        return mVal;
    }
    double Val() const {
        return mVal;
    }
};

class flt64_val: public val_base {
    long double mVal;

public:
    flt64_val() {
        mVal = 0.0;
    }
    virtual const char * TypeName() const {
        return "flt64_val";
    }
    long double & Val() {
        return mVal;
    }
    long double Val() const {
        return mVal;
    }
};

class buf_val: public val_base {
    struct _ctx {
        uint32_t mSize;
        uint8_t mBuf[1];
    };
    _ctx * mVal;

    void reset() {
        if (mVal) {
            value_obj::Allocator()->Recycle(mVal);
            mVal = 0;
        }
    }

public:
    buf_val() {
        mVal = 0;
    }
    virtual const char * TypeName() const {
        return "buf_val";
    }
    virtual ~buf_val() {
        reset();
    }
    uint32_t Size() const {
        return mVal? mVal->mSize: 0;
    }
    void Resize(uint32_t size, bool cleanBuf = false);
    void * Buf() {
        return mVal? mVal->mBuf: 0;
    }
    const void * Buf() const {
        return mVal? mVal->mBuf: 0;
    }
};

class str_val: public buf_val {
public:
    virtual const char * TypeName() const {
        return "str_val";
    }
    uint32_t Len() const {
        const char * str = Str();
        return str? strlen(str): 0;
    }
    char * Str() {
        return static_cast<char *>( Buf() );
    }
    const char * Str() const {
        return static_cast<const char *>( Buf() );
    }
};

class wcs_val: public buf_val {
public:
    virtual const char * TypeName() const {
        return "wcs_val";
    }
    uint32_t Len() const {
        const wchar_t * wcs = Str();
        return wcs? wcslen(wcs): 0;
    }
    wchar_t * Str() {
        return static_cast<wchar_t *>( Buf() );
    }
    const wchar_t * Str() const {
        return static_cast<const wchar_t *>( Buf() );
    }
};

template <typename PT>
class obj_val: public val_base {
public:
    typedef PT ptr_type;
    typedef typename ptr_type::obj_type obj_type;
    typedef typename ptr_type::constructor_type constructor_type;

private:
    ptr_type mVal;

public:
    virtual const char * TypeName() const {
        return mVal? mVal->ContextName(): "obj_val";
    }
    obj_type & Obj() {
        return *mVal;
    }
    const obj_type & Obj() const {
        return *mVal;
    }
    ptr_type & ObjPtr() {
        return mVal;
    }
    const ptr_type & ObjPtr() const {
        return mVal;
    }
};

template <typename IT, int VT>
struct val_itf_caster {
    static IT * Cast(val_base * valImp) {
        return static_cast<IT *>(valImp);
    }
};

template <typename IT>
struct val_itf_caster<IT,value_obj::OBJ_VAL> {
    static IT * Cast(val_base * valImp);
};

template <typename IT>
IT * val_itf_caster<IT,value_obj::OBJ_VAL>::Cast(val_base * valImp) {
    if (  0 == strcmp( "obj_val", valImp->TypeName() )  )
        return static_cast<IT *>(valImp);
#ifdef DISABLE_RTTI
    typedef typename IT::obj_type obj_type;
    typedef typename IT::ptr_type ptr_type;
    typedef typename IT::constructor_type constructor_type;

    static obj_type obj;
    if (  0 == strcmp( obj.ContextName(), valImp->TypeName() )  )
        return static_cast<IT *>(valImp);
    return 0;
#else
    return dynamic_cast<IT *>(valImp);
#endif
}

template <typename IT, int VT>
struct val_itf_binder {
    static IT * GetInterface(value_obj * valObj);
    static const IT * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <typename IT, int VT>
IT * val_itf_binder<IT,VT>::GetInterface(value_obj * valObj) {
    IT * itf = 0;
    if (valObj) {
        switch (valObj->mValType) {
        case value_obj::NUL_VAL:
            itf = static_cast<IT *>(
                value_obj::Allocator()->Allocate( sizeof(IT) )
            );
            valObj->mValType = VT;
            valObj->mValImp = new (itf) IT;
            break;
        case VT:
            itf = val_itf_caster<IT,VT>::Cast(valObj->mValImp);
            break;
        }
    }
    return itf;
}

template <typename PT>
struct val_itf_selector {
    static obj_val<PT> * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            obj_val<PT>,value_obj::OBJ_VAL
        >::GetInterface(valObj);
    }
    static const obj_val<PT> * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<bln_val> {
    static bln_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            bln_val,value_obj::BLN_VAL
        >::GetInterface(valObj);
    }
    static const bln_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<int_val> {
    static int_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            int_val,value_obj::INT_VAL
        >::GetInterface(valObj);
    }
    static const int_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<int64_val> {
    static int64_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            int64_val,value_obj::INT64_VAL
        >::GetInterface(valObj);
    }
    static const int64_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<flt_val> {
    static flt_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            flt_val,value_obj::FLT_VAL
        >::GetInterface(valObj);
    }
    static const flt_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<flt64_val> {
    static flt64_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            flt64_val,value_obj::FLT64_VAL
        >::GetInterface(valObj);
    }
    static const flt64_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<buf_val> {
    static buf_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            buf_val,value_obj::BUF_VAL
        >::GetInterface(valObj);
    }
    static const buf_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<str_val> {
    static str_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            str_val,value_obj::STR_VAL
        >::GetInterface(valObj);
    }
    static const str_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

template <>
struct val_itf_selector<wcs_val> {
    static wcs_val * GetInterface(value_obj * valObj) {
        return val_itf_binder<
            wcs_val,value_obj::WCS_VAL
        >::GetInterface(valObj);
    }
    static const wcs_val * GetInterface(const value_obj * valObj) {
        return GetInterface( const_cast<value_obj *>(valObj) );
    }
};

} // namespace pdl

#endif // _VALUE_OBJ_H_

