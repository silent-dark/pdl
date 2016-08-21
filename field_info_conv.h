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

#include <iostream>
#include <errno.h>
#include "field_des.h"

namespace pdl {

typedef enum {
    FIC_OUTPUT_DEFAULT,
    FIC_OUTPUT_FIELD_NUMBER,
    FIC_OUTPUT_MAX_FIELD_NUM,
    FIC_OUTPUT_FIELD_OFFSET = 4,
    FIC_OUTPUT_FIELD_SIZE = 8,
    FIC_OUTPUT_LEAF_FIELD_ONLY = 0x80000000
} fic_output_flag;

template <typename T, uint32_t BUFFER_EXTEND_SIZE = 16>
class field_info_conv: public combined_field_des::parse_callback {
    typedef T traits_type;
    typedef typename traits_type::stack_item stack_item;
    typedef std::vector< stack_item,std_allocator<field_info> > stack_buf;

    std::ostream * mOST;
    uint32_t mOFlags;
    stack_buf mStackBuf;
    traits_type mTraits;
    bool mIsHeadOutputed;

    const field_des * topFieldDes() const {
        return mTraits.GetFieldDes( mStackBuf.back() );
    }
    bool isParentEnd(const field_des * fieldDes) const {
        return ( mStackBuf.size() && !topFieldDes()->IsSubField(fieldDes, 0) );
    }

    void pushStackItem(stack_item stackItem) {
        uint32_t stackDepth = mStackBuf.size();
        if ( stackDepth == mStackBuf.capacity() )
            mStackBuf.reserve( stackDepth + BUFFER_EXTEND_SIZE );
        mStackBuf.push_back(stackItem);
    }
    bool popStackItem(stack_item * out_popItem) {
        if ( mStackBuf.size() ) {
            *out_popItem = mStackBuf.back();
            mStackBuf.pop_back();
            return true;
        }
        return false;
    }

    void outputIndent();
    void outputParentEnd(bool loop);

public:
    field_info_conv(std::ostream * ost, uint32_t oflags = 0) {
        mOST = ost;
        mOFlags = oflags;
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
void field_info_conv<T,BUFFER_EXTEND_SIZE>::outputIndent() {
    uint32_t n = mStackBuf.size() + 1;
    *mOST << std::endl;
    std::streamsize prevWidth = mOST->width(n << 1);
    *mOST << ' ';
    mOST->width(prevWidth);
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void field_info_conv<T,BUFFER_EXTEND_SIZE>::outputParentEnd(bool loop) {
    stack_item parentItem;
    do {
        if ( !popStackItem(&parentItem) )
            break;
        outputIndent();
        mTraits.onOutputParentEnd(*mOST, parentItem);
    } while(loop);
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
int field_info_conv<T,BUFFER_EXTEND_SIZE>::Callback(
    const field_info_env & env, obj_ptr<field_info> & fieldInfo)
{
    if ( mOST && mOST->good() ) {
        if (!mIsHeadOutputed) {
            mTraits.onOutputHead(*mOST, mOFlags);
            mIsHeadOutputed = true;
        }
        if (  isParentEnd( fieldInfo->FieldDes() )  )
            outputParentEnd(false);
        uint32_t n = fieldInfo->ItemCount();
        for (uint32_t i = 0; i < n; ++i) {
            outputIndent();
            if ( fieldInfo->FieldDes()->IsLeaf() ) {
                value_obj val;
                fieldInfo[i].DecodeValue(env.mBuf, &val);
                mTraits.onOutputLeafField(
                    *mOST, mOFlags, &(fieldInfo[i]), &val
                );
            } else if ( !(FIC_OUTPUT_LEAF_FIELD_ONLY & mOFlags) ) {
                stack_item stackItem;
                mTraits.onOutputCombinedField(
                    *mOST, mOFlags, &(fieldInfo[i]), &stackItem
                );
                pushStackItem(stackItem);
            }
        }
        return 0;
    }
    PDL_THROW( std::runtime_error(
        std::string("field_info_conv_") + mTraits.Tag() + \
        "::Callback() bad output-stream!"
    ) );
    return (EBADF < 0)? EBADF: -EBADF;
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void field_info_conv<T,BUFFER_EXTEND_SIZE>::Flush() {
    if ( mIsHeadOutputed && mOST && mOST->good() ) {
        outputParentEnd(true);
        mTraits.onOutputTail(*mOST, mOFlags);
        mOST->flush();
        mIsHeadOutputed = false;
    }
}

template <typename T>
class conv_field_info: public combined_field_des::parse_callback {
    typedef T traits_type;

    std::istream * mIST;
    uint32_t mOFlags;
    traits_type mTraits;
    bool mIsHeadParsed;

public:
    conv_field_info(std::istream * ist) {
        mIST = ist;
        mOFlags = 0;
        mIsHeadParsed = false;
    }

    virtual int Callback(
        const field_info_env & env, obj_ptr<field_info> & fieldInfo
    );
};

template <typename T>
int conv_field_info<T>::Callback(
    const field_info_env & env, obj_ptr<field_info> & fieldInfo)
{
    if ( mIST && mIST->good() ) {
        if (!mIsHeadParsed) {
            mOFlags = mTraits.onParseHead(*mIST);
            mIsHeadParsed = true;
        }
        if ( fieldInfo->FieldDes()->IsLeaf() ) {
            uint32_t n = fieldInfo->ItemCount();
            for (uint32_t i = 0; i < n; ++i) {
                if (  !mTraits.onParseFieldValue(
                    *mIST, mOFlags, env.mBuf, &(fieldInfo[i]) )  )
                {
                    goto bad_input_stream;
                }
            }
        }
        return 0;
    }

bad_input_stream:
    PDL_THROW( std::runtime_error(
        std::string("conv_field_info_") + mTraits.Tag() + \
        "::Callback() bad input-stream!"
    ) );
    return (EBADF < 0)? EBADF: -EBADF;
}

class field_info_conv_traits_base {
protected:
    static bool isOutputFieldNum(uint32_t oflags) {
        return static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & oflags);
    }
    static uint32_t getTypeId(const char * type);
    static void outputOFlags(std::ostream & ost, uint32_t oflags);
    static uint32_t getOFlags(const char * oflags);
    static void genFieldName(
        std::ostream & ost, const field_des * fieldDes, uint32_t fieldNum)
    {
        ost << fieldDes->FieldName();
        if (fieldNum)
            ost << '-' << fieldNum;
    }
    static void genFieldName(
        std::ostream & ost, const field_info * fieldInfo, uint32_t oflags)
    {
        genFieldName(
            ost,
            fieldInfo->FieldDes(),
            isOutputFieldNum(oflags)? fieldInfo->FieldNumber(): 0
        );
    }
    static void outputFieldBegin(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        const value_obj * valObj,
        const char * dividerName,
        const char * dividerAttr,
        const char * dividerVal
    );
    static void outputFieldVal(
        std::ostream & ost, const char * dividerBuf, const value_obj * valObj
    );
    static bool encFieldVal(
        uint32_t typeId,
        std::istream & ist,
        const char * dividerBuf,
        char delim,
        buf_val * out_buf,
        field_info * io_fieldInfo
    );

    static bool findTag(
        std::istream & ist,
        const char * tag,
        char * buf,
        uint32_t size,
        char delimIgn,
        char delimTag
    );
    static bool getAttr(
        std::istream & ist, char dividerVal, char * out_buf, uint32_t size
    );
};

class field_info_conv_traits_xml: public field_info_conv_traits_base {
    static void myOutputFieldBegin(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        const value_obj * valObj)
    {
        ost << '<';
        outputFieldBegin(ost, oflags, fieldInfo, valObj, " ", " ", "=");
        ost << '>';
    }

    static bool findAttr(
        std::istream & ist,
        const char * elem,
        const char * attr,
        char * out_bufVal,
        uint32_t sizeVal,
        char * bufElem,
        uint32_t sizeElem,
        char * bufAttr,
        uint32_t sizeAttr
    );

public:
    struct stack_item {
        const field_des * mFieldDes;
        uint32_t mFieldNum;
    };

    static const char * Tag() {
        return "xml";
    }

    static const field_des * GetFieldDes(const stack_item & stackItem) {
        return stackItem.mFieldDes;
    }

    static void onOutputHead(std::ostream & ost, uint32_t oflags);
    static void onOutputTail(std::ostream & ost, uint32_t oflags) {
        ost << std::endl << "</protocol-data>";
    }
    static void onOutputParentEnd(
        std::ostream & ost, const stack_item & parentItem)
    {
        ost << "</";
        genFieldName(ost, parentItem.mFieldDes, parentItem.mFieldNum);
        ost << '>';
    }
    static void onOutputCombinedField(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        stack_item * out_stackItem
    );
    static void onOutputLeafField(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        const value_obj * valObj
    );

    static uint32_t onParseHead(std::istream & ist);
    static bool onParseFieldValue(
        std::istream & ist,
        uint32_t oflags,
        buf_val * out_buf,
        field_info * io_fieldInfo
    );
};

class field_info_conv_traits_json: public field_info_conv_traits_base {
public:
    struct stack_item {
        const field_des * mFieldDes;
        bool mIsArrayEnd;
    };

    static const char * Tag() {
        return "json";
    }

private:
    char mFieldSep;
    uint32_t mValType;

    static void myOutputFieldBegin(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        const value_obj * valObj
    );
    static void myOutputFieldVal(std::ostream & ost, const value_obj * valObj);
    void outputField(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        stack_item * out_stackItem,
        const value_obj * valObj
    );

    static bool seekToVal(
        std::istream & ist,
        const char * key,
        char * bufKey,
        uint32_t sizeKey
    );
    static bool findPair(
        std::istream & ist,
        const char * key,
        char * out_bufVal,
        uint32_t sizeVal,
        char * bufKey,
        uint32_t sizeKey
    );
    static bool findAttr(
        std::istream & ist,
        const char * elem,
        const char * attr,
        char * out_bufVal,
        uint32_t sizeVal,
        char * bufElem,
        uint32_t sizeElem,
        char * bufAttr,
        uint32_t sizeAttr)
    {
        return (
            findTag(ist, elem, bufElem, sizeElem, '"', '"') &&
            findPair(ist, attr, out_bufVal, sizeVal, bufAttr, sizeAttr)
        );
    }
    bool findFieldVal(
        std::istream & ist, uint32_t oflags, const field_info * fieldInfo
    );

public:
    static const field_des * GetFieldDes(const stack_item & stackItem) {
        return stackItem.mFieldDes;
    }

    void onOutputHead(std::ostream & ost, uint32_t oflags);
    static void onOutputTail(std::ostream & ost, uint32_t oflags) {
        ost << std::endl << '}';
    }
    void onOutputParentEnd(std::ostream & ost, const stack_item & parentItem) {
        ost << '}';
        if (parentItem.mIsArrayEnd)
            ost << ']';
        mFieldSep = ',';
    }
    void onOutputCombinedField(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        stack_item * out_stackItem)
    {
        outputField(ost, oflags, fieldInfo, out_stackItem, 0);
    }
    void onOutputLeafField(
        std::ostream & ost,
        uint32_t oflags,
        const field_info * fieldInfo,
        const value_obj * valObj)
    {
        outputField(ost, oflags, fieldInfo, 0, valObj);
    }

    static uint32_t onParseHead(std::istream & ist);
    bool onParseFieldValue(
        std::istream & ist,
        uint32_t oflags,
        buf_val * out_buf,
        field_info * io_fieldInfo
    );
};

typedef field_info_conv<field_info_conv_traits_xml> field_info_conv_xml;
typedef conv_field_info<field_info_conv_traits_xml> xml_conv_field_info;
typedef field_info_conv<field_info_conv_traits_json> field_info_conv_json;
typedef conv_field_info<field_info_conv_traits_json> json_conv_field_info;

} // namespace pdl

#endif // _FIELD_INFO_CONV_H_

