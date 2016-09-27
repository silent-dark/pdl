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

#ifndef _FIELD_DES_H_
#define _FIELD_DES_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <vector>
#include <list>
#include "table.h"
#include "tree.h"

namespace pdl {

class field_des; // The field descriptor of binary protocol.

struct field_des_tree_stack_data {
    uint32_t mFieldNumber;
    uint32_t mMaxFieldNum;

    field_des_tree_stack_data() {
        mFieldNumber = 0;
        mMaxFieldNum = 0;
    }
};
typedef tree_traits<field_des_tree_stack_data,const field_des *> \
    field_des_tree_traits;
typedef tree<field_des_tree_traits> field_des_tree;

// The 1st. 'field_des' is the dependent field of the 2nd. 'field_des'.
typedef table<const field_des *,const field_des *> field_des_dependency;

class bit_ref {
    buf_val * mBuf;
    uint32_t mOffset; // In bit.

    void checkValid(const char * errTag) const {
        if ( !IsValid() ) {
            PDL_THROW(
                std::runtime_error( std::string(errTag) + " invalid!" )
            );
        }
    }
    static uint8_t bitMask(uint32_t offset) {
        return uint8_t(1) << ( 7 - (offset & 7) );
    }
    static uint32_t blockIdx(uint32_t offset) {
        return (offset >> 3);
    }
    uint8_t getBlock() const {
        return static_cast<const uint8_t *>( mBuf->Buf() )[blockIdx(mOffset)];
    }
    uint8_t & getBlock() {
        return static_cast<uint8_t *>( mBuf->Buf() )[blockIdx(mOffset)];
    }
    bool getVal() const {
        return static_cast<bool>( getBlock() & bitMask(mOffset) );
    }
    void setVal(bool bit) {
        if (bit)
            getBlock() |= bitMask(mOffset);
        else
            getBlock() &= ~ bitMask(mOffset);
    }
    static uint32_t copyBits(
        const buf_val * src,
        buf_val * dst,
        uint32_t srcOffset,
        uint32_t dstOffset,
        uint32_t length
    );
    static uint32_t copyBits(
        const uint8_t * src,
        uint32_t srcSize,
        uint32_t srcOffset,
        uint8_t * dst,
        uint32_t dstSize,
        uint32_t dstOffset,
        uint32_t length
    );

public:
    explicit bit_ref(buf_val * buf, uint32_t offset) {
        mBuf = buf;
        mOffset = offset;
    }
    explicit bit_ref(const buf_val * buf, uint32_t offset) {
        mBuf = const_cast<buf_val *>(buf);
        mOffset = offset;
    }
    bit_ref(const bit_ref & src) {
        mBuf = src.mBuf;
        mOffset = src.mOffset;
    }
    const buf_val * Buf() const {
        return mBuf;
    }
    buf_val * Buf() {
        return mBuf;
    }
    uint32_t Offset() const {
        return mOffset;
    }
    uint32_t MaxSize() const {
        return mBuf? ( mBuf->Size() << 3 ): 0;
    }
    bool IsValid() const {
        return mOffset < MaxSize();
    }
    bool Val() const {
        checkValid("bit_ref::Val()");
        return getVal();
    }
    uint32_t ExportBits(
        uint32_t length, buf_val * out_bitsBuf, uint32_t startOffset = 0) const
    {
        return copyBits(mBuf, out_bitsBuf, mOffset, startOffset, length);
    }
    uint32_t ExportBits(
        uint32_t length,
        uint8_t * out_bitsBuf,
        uint32_t bufSize,
        uint32_t startOffset = 0) const
    {
        if (mBuf) {
            return copyBits(
                static_cast<const uint8_t *>( mBuf->Buf() ),
                mBuf->Size(),
                mOffset,
                out_bitsBuf,
                bufSize,
                startOffset,
                length
            );
        }
        return 0;
    }
    uint32_t ImportBits(
        uint32_t length, const buf_val * data, uint32_t startOffset = 0)
    {
        return copyBits(data, mBuf, startOffset, mOffset, length);
    }
    uint32_t ImportBits(
        uint32_t length,
        const uint8_t * data,
        uint32_t dataSize,
        uint32_t startOffset = 0)
    {
        if (mBuf) {
            return copyBits(
                data,
                dataSize,
                startOffset,
                static_cast<uint8_t *>( mBuf->Buf() ),
                mBuf->Size(),
                mOffset,
                length
            );
        }
        return 0;
    }
    bit_ref & operator =(const bit_ref & src) {
        mBuf = src.mBuf;
        mOffset = src.mOffset;
        return *this;
    }
    bit_ref & operator =(bool bit) {
        checkValid("bit_ref::operator =()");
        setVal(bit);
        return *this;
    }
    bit_ref & operator +=(uint32_t offset) {
        if (offset) {
            if ( mOffset + offset > MaxSize() ) {
                PDL_THROW( std::overflow_error(
                    "bit_ref::operator +=() overflow!"
                ) );
            } else
                mOffset += offset;
        }
        return *this;
    }
    bit_ref & operator -=(uint32_t offset) {
        if (offset) {
            if (mOffset < offset) {
                PDL_THROW(  std::underflow_error(
                    "bit_ref::operator -=() underflow!"
                ) );
            } else
                mOffset -= offset;
        }
        return *this;
    }
    bit_ref & operator ++() {
        return (*this += 1);
    }
    bit_ref & operator --() {
        return (*this -= 1);
    }
};

struct field_info_ctx: field_des_tree_stack_data {
    uint32_t mFieldOffset; // In bit.
    uint32_t mFieldSize; // In bit.
    const field_des * mFieldDes;

    field_info_ctx() {
        mFieldOffset = 0;
        mFieldSize = 0;
        mFieldDes = 0;
    }
    field_info_ctx(
        const field_des * fieldDes, uint32_t fieldOffset, uint32_t fieldNum)
    {
        mFieldNumber = fieldNum;
        mFieldOffset = fieldOffset;
        mFieldSize = 0;
        mFieldDes = fieldDes;
    }
};

class field_des {
public:
    virtual const char * FieldName() const = 0;

    virtual uint32_t FieldCount(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount
    ) const = 0;

    virtual uint32_t FieldSize(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount
    ) const = 0; // In bit.

protected:
    field_des_tree::node_ptr_c mTreeNode;

public:
    void BindTreeNode(field_des_tree::node_ptr_c treeNode) {
        mTreeNode = treeNode;
    }
    field_des_tree::node_ptr_c TreeNode() const {
        return mTreeNode;
    }

    bool IsCombined() const {
        return mTreeNode && mTreeNode->GetSubNodeCount();
    }
    bool IsLeaf() const {
        return !IsCombined();
    }
    struct sub_idx {
        uint32_t mIndex;
        uint32_t mCount;
    };
    bool IsSubField(const field_des * fieldDes, sub_idx * out_idx) const;
};

class leaf_field_des: public field_des {
public:
    virtual bool DecodeField(bit_ref bitRef, value_obj * out_val) const = 0;
    virtual bool EncodeField(bit_ref bitRef, const value_obj * val) const = 0;
};

class combined_field_des;

class field_info: public obj_base {
    friend class field_info_generator;

public:
    virtual const char * ContextName() const {
        return "field_info";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(field_info);
    }

private:
    field_info_ctx mCtx;

    void checkValid(const char * errTag) const {
        if ( !IsValid() ) {
            PDL_THROW(
                std::runtime_error( std::string(errTag) + " invalid!" )
            );
        }
    }

public:
    field_info(const field_info_ctx * args = 0) {
        if (args)
            mCtx = *args;
        else
            memset( &mCtx, 0, sizeof(field_info_ctx) );
    }
    field_info(const field_info & src) {
        mCtx = src.mCtx;
    }
    field_info & operator =(const field_info & src) {
        mCtx = src.mCtx;
        return *this;
    }
    bool IsValid() const {
        return mCtx.mFieldDes && mCtx.mFieldSize;
    }
    const field_des * FieldDes() const {
        return mCtx.mFieldDes;
    }
    const leaf_field_des * LeafFieldDes() const {
        checkValid("field_info::LeafFieldDes()");
        return mCtx.mFieldDes->IsLeaf() \
            ? static_cast<const leaf_field_des *>(mCtx.mFieldDes): 0;
    }
    inline const combined_field_des * CombinedFieldDes() const;
    uint32_t Offset() const {
        return mCtx.mFieldOffset;
    }
    uint32_t SizeInBit() const {
        return mCtx.mFieldSize;
    }
    uint32_t FieldNumber() const {
        return mCtx.mFieldNumber;
    }
    uint32_t MaxFieldNum() const {
        return mCtx.mMaxFieldNum;
    }
    bit_ref RefBits(const buf_val * buf) const {
        return bit_ref(buf, mCtx.mFieldOffset);
    }
    bool DecodeValue(const buf_val * buf, value_obj * out_val) const {
        const leaf_field_des * fieldDes = LeafFieldDes();
        if (fieldDes) {
            return fieldDes->DecodeField( RefBits(buf), out_val );
        }
        return false;
    }
    bool EncodeValue(buf_val * out_buf, const value_obj * val) const {
        const leaf_field_des * fieldDes = LeafFieldDes();
        if (fieldDes)
            return fieldDes->EncodeField( RefBits(out_buf), val );
        return false;
    }

    static void OverlayAllocator(mem_allocator * memAllocator);
    static mem_allocator * Allocator();
};

template <>
struct obj_constructor<field_info> {
    const field_info_ctx * mArgs;

    obj_constructor(const field_info_ctx * args = 0) {
        mArgs = args;
    }

    void Construct(field_info * obj) {
        new (obj) field_info(mArgs);
    }

    static mem_allocator * Allocator() {
        return field_info::Allocator();
    }
};

typedef std_allocator<field_info_ctx,field_info> field_info_ctx_allocator;
typedef std::list<field_info_ctx,field_info_ctx_allocator> \
    field_info_backtrace_buf;

struct field_info_env {
    const field_des_dependency * mFieldDesDep;
    buf_val * mBuf;
};

class field_info_generator {
    typedef std_allocator<const field_des *,field_info> cp_field_des_allocator;
    typedef std::vector<const field_des *,cp_field_des_allocator> field_des_buf;
    typedef std::vector<field_info_ctx,field_info_ctx_allocator> field_info_buf;

    field_info_backtrace_buf mBacktraceBuf;
    field_des_buf mDepFieldDesBuf;
    field_info_buf mDepFieldInfoBuf;

    bool findDepFieldInfo(
        const field_des * depFieldDes, field_info_ctx * out_depFieldInfo
    );
    uint32_t getDepFieldInfo(
        const field_des_dependency * fieldDesDep,
        const field_des * fieldDes,
        const field_info_ctx ** out_depFieldInfoCount
    );

public:
    obj_ptr<field_info> CreateFieldInfo(
        const field_info_env & env, field_info_ctx * io_args
    );
    void PushBacktraceItem(const obj_ptr<field_info> & fieldInfo);
    void Reset();
};

class combined_field_des: public field_des {
public:
    struct parse_callback {
        // refer tree::for_each_callback::onTraversal for more informations
        // about return value.
        virtual int Callback(
            const field_info_env & env, obj_ptr<field_info> & fieldInfo
        ) = 0;
    };

    // DON'T invoke this function in the implement of FieldSize() function.
    int ParseField(
        parse_callback * cb,
        const field_info_env & env,
        uint32_t startOffset = 0
    );

private:
    class parse_callback_invoker: public field_des_tree::for_each_callback {
        combined_field_des * mParser;
        parse_callback * mCallback;
        field_info_env mEnv;

    public:
        parse_callback_invoker(
            combined_field_des * parser,
            parse_callback * cb,
            const field_info_env & env)
        {
            mParser = parser;
            mCallback = cb;
            mEnv = env;
        }
        virtual void onPushStack(field_des_tree::stack_item * io_stackTop) {
            // Do nothing.
        }
        virtual void afterPopStack(field_des_tree::stack_item * io_stackTop) {
            if (io_stackTop->mNextSubNodeIdx == io_stackTop->mSubNodeCount) {
                if ( ++(io_stackTop->mData.mFieldNumber) < \
                    io_stackTop->mData.mMaxFieldNum )
                {
                    io_stackTop->mNextSubNodeIdx = 0;
                }
            }
        }
        virtual int onTraversal(
            field_des_tree::stack_item * io_stackTop, int order)
        {
            return mParser->invokeCallback(mCallback, mEnv, io_stackTop);
        }
    };
    friend class combined_field_des::parse_callback_invoker;

    uint32_t mParseOffset;
    field_info_generator mFieldInfoGen;

    int invokeCallback(
        parse_callback * cb,
        const field_info_env & env,
        field_des_tree::stack_item * io_stackTop
    );
};

inline const combined_field_des * field_info::CombinedFieldDes() const {
    checkValid("field_info::CombinedFieldDes()");
    return mCtx.mFieldDes->IsCombined() \
        ? static_cast<const combined_field_des *>(mCtx.mFieldDes): 0;
}

} // namespace pdl

#endif // _FIELD_DES_H_

