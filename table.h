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

#ifndef _TABLE_H_
#define _TABLE_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <vector>
#include "obj_base.h"

namespace pdl {

template <typename VT, uint32_t BUFFER_EXTEND_SIZE = 16>
class index_map: public obj_base {
public:
    typedef VT val_type;

    virtual const char * ContextName() const {
        return "index_map";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(index_map);
    }

private:
    typedef std::vector< val_type,std_allocator<val_type> > val_buf;
    typedef std::vector< uint32_t,std_allocator<val_type> > idx_buf;

    val_buf mValBuf;
    idx_buf mIdxBuf; // Store the sorted indexes of each item in the mValBuf.

    bool find(const val_type & val, uint32_t * out_pos) const;
    uint32_t insert(const val_type & val, uint32_t pos);

public:
    uint32_t Size() const {
        return mValBuf.size();
    }
    const val_type & GetValue(uint32_t idx) const {
        return mValBuf.at(idx);
    }
    void Clear() {
        mValBuf.resize(0);
        mIdxBuf.resize(0);
    }
    bool Existed(const val_type & val, uint32_t * out_idx) const {
        uint32_t pos = 0;
        if ( find(val, &pos) ) {
            if (out_idx)
                *out_idx = mIdxBuf[pos];
            return true;
        }
        return false;
    }
    uint32_t operator [](const val_type & val) {
        uint32_t pos = 0;
        return find(val, &pos)? mIdxBuf[pos]: mIdxBuf[insert(val, pos)];
    }
};

template <typename VT, uint32_t BUFFER_EXTEND_SIZE>
bool index_map<VT,BUFFER_EXTEND_SIZE>::find(
    const val_type & val, uint32_t * out_pos) const
{
    uint32_t min = 0, max = mIdxBuf.size();
    *out_pos = 0;
    while (min < max) {
        *out_pos = (min + max) >> 1;
        if (val < mValBuf[ mIdxBuf[*out_pos] ]) {
            if (max == *out_pos)
                break;
            max = *out_pos;
        } else if (mValBuf[ mIdxBuf[*out_pos] ] < val) {
            if (min == *out_pos)
                break;
            min = *out_pos;
        } else
            return true;
    }
    return false;
}

template <typename VT, uint32_t BUFFER_EXTEND_SIZE>
uint32_t index_map<VT,BUFFER_EXTEND_SIZE>::insert(
    const val_type & val, uint32_t pos)
{
    if ( mValBuf.size() == mValBuf.capacity() ) {
        mValBuf.reserve( mValBuf.size() + BUFFER_EXTEND_SIZE );
        mIdxBuf.reserve( mIdxBuf.size() + BUFFER_EXTEND_SIZE );
    }
    uint32_t idx = mValBuf.size();
    mValBuf.push_back(val);
    while ( pos < mIdxBuf.size() && mValBuf[ mIdxBuf[pos] ] < val )
        ++pos;
    if ( mIdxBuf.size() == pos ) {
        mIdxBuf.push_back(idx);
        return pos;
    }
    mIdxBuf.push_back(idx);
    for (uint32_t i = mIdxBuf.size() - 1; i > pos; --i)
        mIdxBuf[i] = mIdxBuf[i - 1];
    mIdxBuf[pos] = idx;
    return pos;
}

template <uint32_t TABLE_WIDTH>
struct table_buf_idx_calculator {
    static uint64_t GetTableBufIdx(uint32_t x, uint32_t y) {
        if (x + 1 > TABLE_WIDTH) {
            PDL_THROW(  std::range_error(
                "table_buf_idx_calculator::GetTableBufIdx() bad range!"
            ) );
            return -1;
        }
        return uint64_t(y) * uint64_t(TABLE_WIDTH) + uint64_t(x);
    }
};

template <>
struct table_buf_idx_calculator<0> {
    static uint64_t GetTableBufIdx(uint32_t x, uint32_t y) {
        // Here is the layout of indexes:
        // B\A| X0| X1| X2| X3|...| Xx
        // ---+---+---+---+---+---+---
        // Y0 | 0 | 2 | 6 | 12|...|...
        // ---+---+---+---+---+---+---
        // Y1 | 1 | 3 | 7 | 13|...|N-4
        // ---+---+---+---+---+---+---
        // Y2 | 4 | 5 | 8 | 14|...|N-3
        // ---+---+---+---+---+---+---
        // Y3 | 9 | 10| 11| 15|...|N-2
        // ---+---+---+---+---+---+---
        // ...|...|...|...|...| n |N-1
        // ---+---+---+---+---+---+---
        // Yy |n+1|n+2|n+3|n+4|...| N
        // Note:
        //   N=(x|y+1)^2-1
        //   N-1|N-2|N-3|N-4|...=N-(x-y)
        //   n=x|y^2-1
        //   n+1|n+2|n+3|n+4|...=n+(x+1)
        if (x < y)
            return uint64_t(y) * uint64_t(y) + uint64_t(x);
        else
            return uint64_t(x) * ( uint64_t(x) + 1 ) + uint64_t(y);
    }
};

template <typename T, uint32_t W>
class table_retriever {
public:
    typedef T traits_type;
    typedef typename traits_type::a_type a_type;
    typedef typename traits_type::b_type b_type;
    typedef typename traits_type::index_map_key index_map_key;
    typedef typename traits_type::index_map_val index_map_val;
    typedef typename traits_type::table_buf table_buf;
    typedef typename traits_type::out_hint out_hint;
    typedef typename index_map_key::val_type key_type;
    typedef typename index_map_val::val_type out_type;

    static uint32_t Retrieve(
        const index_map_key & idxMapKey,
        const index_map_val & idxMapVal,
        const table_buf & tblBuf,
        const key_type & key,
        out_type * out_buf
    );

private:
    static uint64_t getTableBufIdx(uint32_t x, uint32_t y, out_type *) {
        return table_buf_idx_calculator<W>::GetTableBufIdx(x, y);
    }
    static uint64_t getTableBufIdx(uint32_t y, uint32_t x, void *) {
        return table_buf_idx_calculator<W>::GetTableBufIdx(x, y);
    }
};

template <typename T, uint32_t W>
uint32_t table_retriever<T,W>::Retrieve(
    const index_map_key & idxMapKey,
    const index_map_val & idxMapVal,
    const table_buf & tblBuf,
    const key_type & key,
    out_type * out_buf)
{
    uint32_t n = 0;
    uint32_t k = 0;
    if ( idxMapKey.Existed(key, &k) ) {
        for (uint32_t v = 0; v < idxMapVal.Size(); ++v) {
            uint64_t i = getTableBufIdx( k, v, out_hint(out_buf) );
            if (tblBuf[i]) {
                if (out_buf)
                    out_buf[n] = idxMapVal.GetValue(v);
                ++n;
            }
        }
    }
    return n;
}

// The 'table' container is a bidirectional multi-map which supports indicating
// an A or B value as the key to retrieve a B or A set.
template <typename A, typename B, uint32_t TABLE_WIDTH = 0>
class table: public obj_base {
    template <typename T, uint32_t W>
    friend class table_retriever;

public:
    typedef A a_type;
    typedef B b_type;

    virtual const char * ContextName() const {
        return "table";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(table);
    }

private:
    typedef index_map<a_type> index_map_a;
    typedef index_map<b_type> index_map_b;
    typedef std::vector< bool,std_allocator<a_type> > table_buf;

    struct find_traits_a {
        typedef typename table::a_type a_type;
        typedef typename table::b_type b_type;
        typedef typename table::index_map_a index_map_key;
        typedef typename table::index_map_b index_map_val;
        typedef typename table::table_buf table_buf;
        typedef b_type * out_hint;
    };
    struct find_traits_b {
        typedef typename table::a_type a_type;
        typedef typename table::b_type b_type;
        typedef typename table::index_map_b index_map_key;
        typedef typename table::index_map_a index_map_val;
        typedef typename table::table_buf table_buf;
        typedef void * out_hint;
    };

    index_map_a mIdxMapA;
    index_map_b mIdxMapB;
    table_buf mTableBuf;

public:
    void Insert(const a_type & a, const b_type & b);
    void Remove(const a_type & a, const b_type & b);
    void Clear();
    uint32_t FindWithA(const a_type & a, b_type * out_buf) const {
        return table_retriever<find_traits_a,TABLE_WIDTH>::Retrieve(
            mIdxMapA, mIdxMapB, mTableBuf, a, out_buf
        );
    }
    uint32_t FindWithB(const b_type & b, a_type * out_buf) const {
        return table_retriever<find_traits_b,TABLE_WIDTH>::Retrieve(
            mIdxMapB, mIdxMapA, mTableBuf, b, out_buf
        );
    }
};

template <typename A, typename B, uint32_t TABLE_WIDTH>
void table<A,B,TABLE_WIDTH>::Insert(const a_type & a, const b_type & b) {
    uint32_t x = mIdxMapA[a];
    uint32_t y = mIdxMapB[b];
    uint64_t i = table_buf_idx_calculator<TABLE_WIDTH>::GetTableBufIdx(x, y);
    if ( i + 1 > mTableBuf.size() )
        mTableBuf.resize(i + 1);
    mTableBuf[i] = true;
}

template <typename A, typename B, uint32_t TABLE_WIDTH>
void table<A,B,TABLE_WIDTH>::Remove(const a_type & a, const b_type & b) {
    uint32_t x = 0, y = 0;
    if ( mIdxMapA.Existed(a, &x) && mIdxMapB.Existed(b, &y) ) {
        uint64_t i = \
            table_buf_idx_calculator<TABLE_WIDTH>::GetTableBufIdx(x, y);
        if ( i < mTableBuf.size() )
            mTableBuf[i] = false;
    }
}

template <typename A, typename B, uint32_t TABLE_WIDTH>
void table<A,B,TABLE_WIDTH>::Clear() {
    mIdxMapA.Clear();
    mIdxMapB.Clear();
    mTableBuf.resize(0);
}

} // namespace pdl

#endif // _TABLE_H_

