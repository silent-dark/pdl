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

#ifndef _FIELD_INFO_CONV_H_
#define _FIELD_INFO_CONV_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <ostream>
#include <errno.h>
#include "field_des.h"

namespace pdl {

typedef enum {
    FIC_OUTPUT_DEFAULT,
    FIC_OUTPUT_FIELD_SIZE,
    FIC_OUTPUT_FIELD_OFFSET,
    FIC_OUTPUT_FIELD_NUMBER = 4,
    FIC_OUTPUT_MAX_FIELD_NUM = 8
} fic_output_flag;

template <typename T, uint32_t BUFFER_EXTEND_SIZE = 16>
class field_info_conv: public combined_field_des::parse_callback {
    typedef T traits_type;
    typedef typename traits_type::stack_item stack_item;
    typedef std::vector< stack_item,std_allocator<field_info> > stack_buf;

    std::ostream * mOutputStream;
    uint32_t mOutputFlags;
    stack_buf mStackBuf;
    bool mIsHeadOutputed;
    traits_type mTraits;

    const field_des * topFieldDes() const {
        return mTraits.GetFieldDes( mStackBuf.back() );
    }
    bool isParentEnd(const field_des * fieldDes) const {
        return ( mStackBuf.size() && !topFieldDes()->IsSubField(fieldDes, 0) );
    }

    void pushStackItem(stack_item stackItem);
    bool popStackItem(stack_item * out_popItem);

    void outputIndent();
    void outputParentEnd(bool loop);

public:
    field_info_conv(std::ostream * outputStream, uint32_t outputFlags = 0) {
        mOutputStream = outputStream;
        mOutputFlags = outputFlags;
        mIsHeadOutputed = false;
    }
    ~field_info_conv() {
        Flush();
    }

    virtual int Callback(
        const field_info_env & env, obj_ptr<field_info> & fieldInfo
    );

    void Flush();
};

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void field_info_conv<T,BUFFER_EXTEND_SIZE>::pushStackItem(stack_item stackItem)
{
    if ( mStackBuf.size() == mStackBuf.capacity() )
        mStackBuf.reserve( mStackBuf.size() + BUFFER_EXTEND_SIZE );
    mStackBuf.push_back(stackItem);
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
bool field_info_conv<T,BUFFER_EXTEND_SIZE>::popStackItem(
    stack_item * out_popItem)
{
    if ( mStackBuf.size() ) {
        *out_popItem = mStackBuf.back();
        mStackBuf.pop_back();
        return true;
    }
    return false;
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void field_info_conv<T,BUFFER_EXTEND_SIZE>::outputIndent() {
    uint32_t n = mStackBuf.size() + 1;
    *mOutputStream << std::endl;
    std::streamsize prevWidth = mOutputStream->width(n << 2);
    *mOutputStream << ' ';
    mOutputStream->width(prevWidth);
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void field_info_conv<T,BUFFER_EXTEND_SIZE>::outputParentEnd(bool loop) {
    stack_item parentItem;
    do {
        if ( !popStackItem(&parentItem) )
            break;
        outputIndent();
        mTraits.onOutputParentEnd(*mOutputStream, parentItem);
    } while(loop);
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
int field_info_conv<T,BUFFER_EXTEND_SIZE>::Callback(
    const field_info_env & env, obj_ptr<field_info> & fieldInfo)
{
    if ( mOutputStream && mOutputStream->good() ) {
        if (!mIsHeadOutputed) {
            mTraits.onOutputHead(*mOutputStream, mOutputFlags);
            mIsHeadOutputed = true;
        }
        const field_des * fieldDes = fieldInfo->FieldDes();
        if ( isParentEnd(fieldDes) )
            outputParentEnd(false);
        uint32_t n = fieldInfo->ItemCount();
        for (uint32_t i = 0; i < n; ++i) {
            outputIndent();
            if ( fieldDes->IsCombined() ) {
                stack_item stackItem;
                mTraits.onOutputCombinedField(
                    *mOutputStream,
                    mOutputFlags,
                    &(fieldInfo[i]),
                    &stackItem
                );
                pushStackItem(stackItem);
                break;
            } else {
                value_obj val;
                fieldInfo[i].DecodeValue(env.mBuf, &val);
                mTraits.onOutputLeafField(
                    *mOutputStream,
                    mOutputFlags,
                    &(fieldInfo[i]),
                    &val
                );
            }
        }
        return 0;
    }
    PDL_THROW( std::runtime_error(
        "field_info_conv::Callback() bad output-stream!"
    ) );
    return (EBADF < 0)? EBADF: -EBADF;
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void field_info_conv<T,BUFFER_EXTEND_SIZE>::Flush() {
    if ( mIsHeadOutputed && mOutputStream && mOutputStream->good() ) {
        outputParentEnd(true);
        mTraits.onOutputTail(*mOutputStream, mOutputFlags);
        mOutputStream->flush();
        mIsHeadOutputed = false;
    }
}

class field_info_conv_traits_xml {
    static void outputFieldBegin(
        std::ostream & out, uint32_t outputFlags, const field_info * fieldInfo
    );
    static void outputFieldValue(std::ostream & out, const value_obj * valObj);

public:
    typedef const field_des * stack_item;

    static const field_des * GetFieldDes(stack_item stackItem) {
        return stackItem;
    }

    static void onOutputHead(std::ostream & out, uint32_t outputFlags);
    static void onOutputTail(std::ostream & out, uint32_t outputFlags) {
        out << std::endl << "</protocol-data>";
    }
    static void onOutputParentEnd(std::ostream & out, stack_item parentItem) {
        out << "</" << parentItem->FieldName() << '>';
    }
    static void onOutputCombinedField(
        std::ostream & out,
        uint32_t outputFlags,
        const field_info * fieldInfo,
        stack_item * out_stackItem)
    {
        outputFieldBegin(out, outputFlags, fieldInfo);
        out << '>';
        *out_stackItem = fieldInfo->FieldDes();
    }
    static void onOutputLeafField(
        std::ostream & out,
        uint32_t outputFlags,
        const field_info * fieldInfo,
        const value_obj * valObj)
    {
        outputFieldBegin(out, outputFlags, fieldInfo);
        outputFieldValue(out, valObj);
        onOutputParentEnd( out, fieldInfo->FieldDes() );
    }
};

class field_info_conv_traits_json {
    static void outputFieldBegin(
        std::ostream & out, uint32_t outputFlags, const field_info * fieldInfo
    );
    static void outputFieldValue(std::ostream & out, const value_obj * valObj);

    char mFieldSep;

public:
    struct stack_item {
        const field_des * mFieldDes;
        bool mIsArrayEnd;
    };

    static const field_des * GetFieldDes(const stack_item & stackItem) {
        return stackItem.mFieldDes;
    }

    void onOutputHead(std::ostream & out, uint32_t outputFlags);
    void onOutputTail(std::ostream & out, uint32_t outputFlags) {
        out << std::endl << '}';
    }
    void onOutputParentEnd(
        std::ostream & out, const stack_item & parentItem)
    {
        out << (parentItem.mIsArrayEnd? "}]": "}");
        mFieldSep = ',';
    }
    void onOutputCombinedField(
        std::ostream & out,
        uint32_t outputFlags,
        const field_info * fieldInfo,
        stack_item * out_stackItem
    );
    void onOutputLeafField(
        std::ostream & out,
        uint32_t outputFlags,
        const field_info * fieldInfo,
        const value_obj * valObj
    );
};

typedef field_info_conv<field_info_conv_traits_xml> field_info_conv_xml;
typedef field_info_conv<field_info_conv_traits_json> field_info_conv_json;

} // namespace pdl

#endif // _FIELD_INFO_CONV_H_

