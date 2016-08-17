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

#include <errno.h>
#include "field_des.h"

using namespace pdl;

extern mem_allocator * gFieldDesAllocator;
static field_info_backtrace_buf gBacktraceItemPool;

void field_info::OverlayAllocator(mem_allocator * memAllocator) {
    gFieldDesAllocator = memAllocator;
}

mem_allocator * field_info::Allocator() {
    return gFieldDesAllocator;
}

uint32_t bit_ref::copyBits(
    const buf_val * src,
    buf_val * dst,
    uint32_t srcOffset,
    uint32_t dstOffset,
    uint32_t length)
{
    if (  src && dst && length && \
        srcOffset < ( src->Size() << 3 ) && dstOffset < ( dst->Size() << 3 )  )
    {
        bit_ref copySrc(src, srcOffset), copyDst(dst, dstOffset);
        uint32_t maxLength = copySrc.MaxSize() - srcOffset;
        if (length > maxLength)
            length = maxLength;
        if (copyDst.MaxSize() < length + dstOffset) {
            uint32_t bitsBufSize = ( (length + dstOffset) >> 3 );
            if ( (length + dstOffset) & 7 )
                ++bitsBufSize;
            dst->Resize(bitsBufSize);
        }
        for (uint32_t i = 0; i < length; ++i) {
            copyDst.setVal( copySrc.getVal() );
            ++copySrc.mOffset;
            ++copyDst.mOffset;
        }
        return length;
    }
    return 0;
}

uint32_t bit_ref::copyBits(
    const uint8_t * src,
    uint32_t srcSize,
    uint32_t srcOffset,
    uint8_t * dst,
    uint32_t dstSize,
    uint32_t dstOffset,
    uint32_t length)
{
    if ( src && dst && length && \
        srcOffset < (srcSize << 3) && dstOffset < (dstSize << 3) )
    {
        uint32_t maxLength[2] = {
            (srcSize << 3) - srcOffset, (dstSize << 3) - dstOffset
        };
        if (length > maxLength[0])
            length = maxLength[0];
        if (length > maxLength[1])
            length = maxLength[1];
        for (uint32_t i = 0; i < length; ++i) {
            if ( src[blockIdx(srcOffset + i)] & bitMask(srcOffset + i) )
                dst[blockIdx(dstOffset + i)] |= bitMask(dstOffset + i);
            else
                dst[blockIdx(dstOffset + i)] &= ~ bitMask(dstOffset + i);
        }
        return length;
    }
    return 0;
}

bool field_des::IsSubField(
    const field_des * fieldDes, sub_idx * out_idx) const
{
    if (mTreeNode) {
        uint32_t n = \
            field_des_tree::node_adapter::GetSubNodeCount(mTreeNode);
        for (uint32_t i = 0; i < n; ++i) {
            if ( fieldDes->mTreeNode == \
                field_des_tree::node_adapter::GetSubNode(mTreeNode, i) )
            {
                if (out_idx) {
                    out_idx->mIndex = i;
                    out_idx->mCount = n;
                }
                return true;
            }
        }
    }
    return false;
}

bool field_info_generator::findDepFieldInfo(
    const field_des * depFieldDes, field_info_ctx * out_depFieldInfo)
{
    field_info_backtrace_buf::iterator itrEnd = mBacktraceBuf.end();
    field_info_backtrace_buf::iterator itr = mBacktraceBuf.begin();
    while (itr != itrEnd) {
        if (depFieldDes == itr->mFieldDes) {
            *out_depFieldInfo = *itr;
            return true;
        }
        ++itr;
    }
    return false;
}

uint32_t field_info_generator::getDepFieldInfo(
    const field_des_dependency * fieldDesDep,
    const field_des * fieldDes,
    const field_info_ctx ** out_depFieldInfo)
{
    uint32_t depFieldCount = fieldDesDep->FindWithB(fieldDes, 0); // get the count of dependent 'field_des'.
    if (depFieldCount) {
        bool found = false;
        mDepFieldDesBuf.resize(depFieldCount);
        fieldDesDep->FindWithB( fieldDes, &(mDepFieldDesBuf[0]) ); // get the dependent 'field_des' set.
        mDepFieldInfoBuf.resize(depFieldCount);
        for (uint32_t i = 0; i < depFieldCount; ++i) {
            found = \
                findDepFieldInfo( mDepFieldDesBuf[i], &(mDepFieldInfoBuf[i]) );
            if (!found) {
                PDL_THROW( std::runtime_error(
                    "field_info_generator::getDepFieldInfo()" \
                    " bad dependent field_des!"
                ) );
                break;
            }
        }
        if (found) {
            *out_depFieldInfo = &(mDepFieldInfoBuf[0]);
            return depFieldCount;
        }
    }
    *out_depFieldInfo = 0;
    return 0;
}

obj_ptr<field_info> field_info_generator::CreateFieldInfo(
    const field_info_env & env, field_info_ctx * io_args)
{
    if (io_args && env.mFieldDesDep && env.mBuf) {
        const field_info_ctx * depFieldInfo = 0;
        uint32_t depFieldInfoCount = getDepFieldInfo(
            env.mFieldDesDep, io_args->mFieldDes, &depFieldInfo
        );
        io_args->mMaxFieldNum = io_args->mFieldDes->FieldCount(
            bit_ref(env.mBuf, io_args->mFieldOffset),
            depFieldInfo,
            depFieldInfoCount
        );
        if (io_args->mFieldNumber <= io_args->mMaxFieldNum) {
            io_args->mFieldSize = io_args->mFieldDes->FieldSize(
                bit_ref(env.mBuf, io_args->mFieldOffset),
                depFieldInfo,
                depFieldInfoCount
            );
            obj_constructor<field_info> fieldInfoConstructor(io_args);
            obj_ptr<field_info> fieldInfo(
                &fieldInfoConstructor,
                io_args->mFieldDes->IsLeaf()? io_args->mMaxFieldNum: 1
            );
            for (uint32_t i = 1; i < fieldInfo->ItemCount(); ++i) {
                fieldInfo[i].mCtx.mFieldNumber = i + 1;
                fieldInfo[i].mCtx.mFieldOffset = \
                    fieldInfo[i - 1].mCtx.mFieldOffset + \
                    fieldInfo[i - 1].mCtx.mFieldSize;
                fieldInfo[i].mCtx.mFieldSize = io_args->mFieldDes->FieldSize(
                    bit_ref(env.mBuf, fieldInfo[i].mCtx.mFieldOffset),
                    depFieldInfo,
                    depFieldInfoCount
                );
            }
            return fieldInfo;
        }
    }
    return obj_ptr<field_info>();
}

void field_info_generator::PushBacktraceItem(
    const obj_ptr<field_info> & fieldInfo)
{
    if ( fieldInfo && fieldInfo->IsValid() ) {
        if ( gBacktraceItemPool.size() ) {
            mBacktraceBuf.splice(
                mBacktraceBuf.begin(),
                gBacktraceItemPool,
                gBacktraceItemPool.begin()
            );
            mBacktraceBuf.front() = fieldInfo->mCtx;
        } else
            mBacktraceBuf.push_front(fieldInfo->mCtx);
    }
}

void field_info_generator::Reset() {
    if ( mBacktraceBuf.size() )
        gBacktraceItemPool.splice( gBacktraceItemPool.end(), mBacktraceBuf ); // clear backtrace.
    if ( mDepFieldDesBuf.size() )
        mDepFieldDesBuf.resize(0);
    if ( mDepFieldInfoBuf.size() )
        mDepFieldInfoBuf.resize(0);
}

int combined_field_des::invokeCallback(
    parse_callback * cb,
    const field_info_env & env,
    field_des_tree::stack_item * io_stackTop)
{
    const field_des * fieldDes = \
        *field_des_tree::node_adapter::GetValue(io_stackTop->mTreeNode);
    field_info_ctx args(
        fieldDes, mParseOffset, io_stackTop->mData.mFieldNumber + 1
    );
    obj_ptr<field_info> fieldInfo = mFieldInfoGen.CreateFieldInfo(env, &args);
    if (!fieldInfo)
        return 1; // refer tree::for_each_callback::onTraversal for the return value.
    io_stackTop->mData.mMaxFieldNum = args.mMaxFieldNum;
    int cbResult = cb->Callback(env, fieldInfo);
    if (cbResult >= 0) {
        if ( env.mFieldDesDep->FindWithA(fieldDes, 0) ) // check if this is a dependent 'field_des'.
            mFieldInfoGen.PushBacktraceItem(fieldInfo);
        if ( fieldDes->IsLeaf() ) {
            const field_info & lastFieldInfo = \
                fieldInfo[fieldInfo->ItemCount() - 1];
            mParseOffset = lastFieldInfo.Offset() + lastFieldInfo.SizeInBit();
        }
    }
    return cbResult;
}

int combined_field_des::ParseField(
    parse_callback * cb, const field_info_env & env, uint32_t startOffset)
{
    if (cb && env.mFieldDesDep && env.mBuf) {
        mParseOffset = startOffset;
        parse_callback_invoker cbInvoker(this, cb, env);
        field_des_tree fieldDesTree(mTreeNode);
        int cbResult = fieldDesTree.ForEach(&cbInvoker, 0);
        *( fieldDesTree.GetRootNodeAddr() ) = 0; // to avoid delete.
        mFieldInfoGen.Reset();
        return cbResult;
    }
    PDL_THROW( std::invalid_argument(
        "combined_field_des::Parse() invalid argument!"
    ) );
    return (EINVAL < 0)? EINVAL: -EINVAL;
}

#define FIELD_DES_UT
#ifdef FIELD_DES_UT

#include <iostream>
#include <fstream>
#include "field_info_conv.h"

struct BITMAP {
    int32_t bmType;
    int32_t bmWidth;
    int32_t bmHeigth;
    int32_t bmWidthBytes;
    uint16_t bmPlanes;
    uint16_t bmBitsPixel;
    uint8_t bmBits[100*100];
};

template <typename T>
class int_field: public leaf_field_des {
    enum field_size_value {
        FIELD_SIZE = sizeof(T) << 3
    };

public:
    virtual uint32_t FieldCount(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount) const
    {
        return 1;
    }
    virtual uint32_t FieldSize(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount) const
    {
        return FIELD_SIZE;
    }
    virtual bool DecodeField(bit_ref bitRef, value_obj * out_val) const;
    virtual bool EncodeField(bit_ref bitRef, const value_obj * val) const;

private:
    static void decVal(uint8_t * data, value_obj * out_val, uint16_t *) {
        val_itf_selector<int_val>::GetInterface(out_val)->Val() = \
            ( uint32_t(data[0]) << 8 ) | uint32_t(data[1]);
    }
    static bool encVal(
        const value_obj * val, uint8_t * out_dataBuf, uint16_t *)
    {
        const int_val * intVal = val_itf_selector<int_val>::GetInterface(val);
        if (intVal) {
            uint32_t data = intVal->Val();
            out_dataBuf[0] = uint8_t(data >> 8) & 0xff;
            out_dataBuf[1] = uint8_t(data) & 0xff;
            return true;
        }
        return false;
    }
    static void decVal(uint8_t * data, value_obj * out_val, uint32_t *) {
        val_itf_selector<int_val>::GetInterface(out_val)->Val() = \
            ( uint32_t(data[0]) << 24 ) | \
            ( uint32_t(data[1]) << 16 ) | \
            ( uint32_t(data[2]) << 8 ) | \
            uint32_t(data[3]);
    }
    static bool encVal(
        const value_obj * val, uint8_t * out_dataBuf, uint32_t *)
    {
        const int_val * intVal = val_itf_selector<int_val>::GetInterface(val);
        if (intVal) {
            uint32_t data = intVal->Val();
            out_dataBuf[0] = uint8_t(data >> 24) & 0xff;
            out_dataBuf[1] = uint8_t(data >> 16) & 0xff;
            out_dataBuf[2] = uint8_t(data >> 8) & 0xff;
            out_dataBuf[3] = uint8_t(data) & 0xff;
            return true;
        }
        return false;
    }
};

template <typename T>
bool int_field<T>::DecodeField(bit_ref bitRef, value_obj * out_val) const {
    if ( bitRef.IsValid() && out_val ) {
        uint8_t data[sizeof(T)];
        std::cout << "  <<< " << this->FieldName() << " [" << FIELD_SIZE << \
            " @" << bitRef.Offset() << "]: ";
        if (  bitRef.ExportBits( FIELD_SIZE, data, sizeof(T) )  ) {
            out_val->Reset();
            decVal( data, out_val, static_cast<T *>(0) );
            std::cout << \
                val_itf_selector<int_val>::GetInterface(out_val)->Val() << \
                std::endl;
            return true;
        }
    }
    return false;
}

template <typename T>
bool int_field<T>::EncodeField(bit_ref bitRef, const value_obj * val) const {
    if ( bitRef.IsValid() && val ) {
        std::cout << "  " << this->FieldName() << " [" << FIELD_SIZE << \
            " @" << bitRef.Offset() << "]: <<< ";
        uint8_t data[sizeof(T)];
        if (  encVal( val, data, static_cast<T *>(0) )  ) {
            std::cout << \
                val_itf_selector<int_val>::GetInterface(val)->Val() << \
                std::endl;
            return static_cast<bool>(
                bitRef.ImportBits( FIELD_SIZE, data, sizeof(T) )
            );
        }
    }
    return false;
}

class bm_type_field: public int_field<uint32_t> {
    virtual const char * FieldName() const {
        return "bm_type";
    }
};

class bm_width_field: public int_field<uint32_t> {
    virtual const char * FieldName() const {
        return "bm_width";
    }
};

class bm_height_field: public int_field<uint32_t> {
    virtual const char * FieldName() const {
        return "bm_height";
    }
};

class bm_width_bytes_field: public int_field<uint32_t> {
    virtual const char * FieldName() const {
        return "bm_width_bytes";
    }
};

class bm_planes_field: public int_field<uint16_t> {
    virtual const char * FieldName() const {
        return "bm_planes";
    }
};

class bm_bits_pixel_field: public int_field<uint16_t> {
    virtual const char * FieldName() const {
        return "bm_bits_pixel";
    }
};

class bm_bits_field: public leaf_field_des {
    uint32_t mFieldSize;

public:
    bm_bits_field() {
        mFieldSize = 0;
    }
    virtual const char * FieldName() const {
        return "bm_bits";
    }
    virtual uint32_t FieldCount(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount) const;
    virtual uint32_t FieldSize(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount) const
    {
        return mFieldSize;
    }
    virtual bool DecodeField(bit_ref bitRef, value_obj * out_val) const;
    virtual bool EncodeField(bit_ref bitRef, const value_obj * val) const;
};

uint32_t bm_bits_field::FieldCount(
    bit_ref bitRef,
    const field_info_ctx * depFieldInfo,
    uint32_t depFieldInfoCount) const
{
    if ( bitRef.IsValid() && depFieldInfo && 3 == depFieldInfoCount ) {
        int32_t imgSize[3] = {0};
        value_obj fieldVal;
        for (uint32_t i = 0; i < 3; ++i) {
            static_cast<const leaf_field_des *>(
                depFieldInfo[i].mFieldDes
            )->DecodeField(
                bit_ref( bitRef.Buf(), depFieldInfo[i].mFieldOffset ),
                &fieldVal
            );
            imgSize[i] = int32_t(
                val_itf_selector<int_val>::GetInterface(&fieldVal)->Val()
            );
            if (imgSize[i] < 0)
                imgSize[i] = -imgSize[i];
        }
        const_cast<bm_bits_field *>(this)->mFieldSize = \
            imgSize[0] * imgSize[1] * imgSize[2];
    }
    return mFieldSize? 1: 0;
}

bool bm_bits_field::DecodeField(bit_ref bitRef, value_obj * out_val) const {
    if ( mFieldSize && bitRef.IsValid() && out_val ) {
        std::cout << "  <<< " << FieldName() << " [" << mFieldSize << " @" << \
            bitRef.Offset() << "]: ..." << std::endl;
        out_val->Reset();
        buf_val * bufVal = val_itf_selector<buf_val>::GetInterface(out_val);
        if (bufVal) {
            bufVal->Resize(mFieldSize >> 3);
            memcpy(
                bufVal->Buf(),
                static_cast<uint8_t *>( bitRef.Buf()->Buf() ) + \
                    (bitRef.Offset() >> 3),
                mFieldSize >> 3
            );
            return true;
        }
    }
    return false;
}

bool bm_bits_field::EncodeField(bit_ref bitRef, const value_obj * val) const {
    if ( mFieldSize && bitRef.IsValid() && val ) {
        std::cout << "  " << FieldName() << " [" << mFieldSize << " @" << \
            bitRef.Offset() << "]: <<< ..." << std::endl;
        const buf_val * bufVal = val_itf_selector<buf_val>::GetInterface(val);
        if ( bufVal && bufVal->Buf() ) {
            memcpy(
                static_cast<uint8_t *>( bitRef.Buf()->Buf() ) + \
                    (bitRef.Offset() >> 3),
                bufVal->Buf(),
                mFieldSize >> 3 
            );
            return true;
        }
    }
    return false;
}

class bitmap_field: public combined_field_des {
public:
    virtual const char * FieldName() const {
        return "bitmap";
    }
    virtual uint32_t FieldCount(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount) const
    {
        return 2; // For test.
    }
    virtual uint32_t FieldSize(
        bit_ref bitRef,
        const field_info_ctx * depFieldInfo,
        uint32_t depFieldInfoCount) const
    {
        return bitRef.MaxSize() / 2; // For test.
    }
};

struct bm_parser_callback: combined_field_des::parse_callback {
    virtual int Callback(
        const field_info_env & env, obj_ptr<field_info> & fieldInfo)
    {
        const char * fieldDesName = fieldInfo->FieldDes()->FieldName();
        std::cout << fieldDesName << ": " << fieldInfo->SizeInBit() << " @" << \
            fieldInfo->Offset() << std::endl;
        bool setFieldVal = true;
        uint32_t val = 0;
        if ( 0 == strcmp("bm_width" , fieldDesName) )
            val = 50;
        else if ( 0 == strcmp("bm_height" , fieldDesName) )
            val = 100;
        else if ( 0 == strcmp("bm_width_bytes" , fieldDesName) )
            val = 100;
        else if ( 0 == strcmp("bm_bits_pixel" , fieldDesName) )
            val = 16;
        else if ( 0 == strcmp("bm_bits" , fieldDesName) )
            setFieldVal = false;
        else if ( 0 == strcmp("bitmap" , fieldDesName) )
            setFieldVal = false;
        if (setFieldVal) {
            value_obj fieldVal;
            val_itf_selector<int_val>::GetInterface(&fieldVal)->Val() = val;
            fieldInfo->EncodeValue(
                const_cast<buf_val *>(env.mBuf), &fieldVal
            );
        }
        return 0;
    }
};

bitmap_field BITMAP_FIELD;
bm_type_field BM_TYPE_FIELD;
bm_width_field BM_WIDTH_FIELD;
bm_height_field BM_HEIGHT_FIELD;
bm_width_bytes_field BM_WIDTH_BYTES_FIELD;
bm_planes_field BM_PLANES_FIELD;
bm_bits_pixel_field BM_BITS_PIXEL_FIELD;
bm_bits_field BM_BITS_FIELD;

field_des * BM_FIELD_DES[8] = {
    &BITMAP_FIELD,
    &BM_TYPE_FIELD,
    &BM_WIDTH_FIELD,
    &BM_HEIGHT_FIELD,
    &BM_WIDTH_BYTES_FIELD,
    &BM_PLANES_FIELD,
    &BM_BITS_PIXEL_FIELD,
    &BM_BITS_FIELD
};

int main() {
    std::cout << "// test bit_ref." << std::endl;
    uint8_t bitsBuf[] = {'1', '2', '3', 0};
    value_obj valObj;
    buf_val * bufVal = val_itf_selector<buf_val>::GetInterface(&valObj);
    bufVal->Resize( sizeof(bitsBuf) );
    bit_ref bitRef(bufVal, 0);
    bitRef.ImportBits( sizeof(bitsBuf) << 3, bitsBuf, sizeof(bitsBuf) );
    uint32_t i = 0;
    std::cout << "\"" << reinterpret_cast<char *>(bitsBuf) << "\":";
    do {
        std::cout << ( (i % 8)? "": " " ) << int( bitRef.Val() );
        if ( ++i < bitRef.MaxSize() )
            ++bitRef;
        else
            break;
    } while (1);
    std::cout << std::endl;

    std::cout << "// test combined_field_des." << std::endl;
    field_des_tree::node_ptr bmFieldDesNode = \
        field_des_tree::node_adapter::CreateTreeNode(BM_FIELD_DES[0]);
    BM_FIELD_DES[0]->BindTreeNode(bmFieldDesNode);
    field_des_tree::node_adapter::SetSubNodeCapacity(bmFieldDesNode, 7);
    field_des_tree::node_ptr subFieldDesNode = 0;
    for (i = 1; i < 8; ++i) {
        subFieldDesNode = \
            field_des_tree::node_adapter::CreateTreeNode(BM_FIELD_DES[i]);
        BM_FIELD_DES[i]->BindTreeNode(subFieldDesNode);
        field_des_tree::node_adapter::SetSubNode(
            bmFieldDesNode, i - 1, subFieldDesNode
        );
    }
    field_des_dependency bmFieldDesDep;
    bmFieldDesDep.Insert(BM_FIELD_DES[2], BM_FIELD_DES[7]);
    bmFieldDesDep.Insert(BM_FIELD_DES[3], BM_FIELD_DES[7]);
    bmFieldDesDep.Insert(BM_FIELD_DES[6], BM_FIELD_DES[7]);
    field_des_tree fieldDesTree(bmFieldDesNode); // to delete nodes.
    bufVal->Resize( sizeof(BITMAP) * 2 );
    memset( bufVal->Buf(), 0xff, bufVal->Size() );
    field_info_env biEnv = {&bmFieldDesDep, bufVal};
    bm_parser_callback cb;
    BITMAP_FIELD.ParseField(&cb, biEnv);
    std::ofstream convFile;
    convFile.open("field_des_ut.xml");
    field_info_conv_xml xmlConv(
        &convFile,
        FIC_OUTPUT_FIELD_NUMBER | FIC_OUTPUT_MAX_FIELD_NUM
    );
    BITMAP_FIELD.ParseField(&xmlConv, biEnv);
    xmlConv.Flush();
    convFile.close();
    convFile.open("field_des_ut.json");
    field_info_conv_json jsonConv(
        &convFile,
        FIC_OUTPUT_MAX_FIELD_NUM
    );
    BITMAP_FIELD.ParseField(&jsonConv, biEnv);
    jsonConv.Flush();
    return 0;
}

#endif // FIELD_DES_UT

