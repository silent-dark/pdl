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

#include <stdlib.h>
#include <limits>
#include <sstream>
#include "field_info_conv.h"

using namespace pdl;

static const char * const VALID_TYPE[] = {
    "NUL", "bln", "int", "hex", "flt", "dbl", "buf", "str", "mbs"
};
static uint32_t const VALID_TYPE_SIZE = \
    sizeof(VALID_TYPE) / sizeof(const char *);

static const char * const VALID_OFLAG[] = {
    "num", "max_num", "pos", "size", "leaf_only"
};
static uint32_t const VALID_OFLAG_MAP[] = {
    FIC_OUTPUT_FIELD_NUMBER,
    FIC_OUTPUT_MAX_FIELD_NUM,
    FIC_OUTPUT_FIELD_OFFSET,
    FIC_OUTPUT_FIELD_SIZE,
    FIC_OUTPUT_LEAF_FIELD_ONLY
};
static uint32_t const VALID_OFLAG_SIZE = \
    sizeof(VALID_OFLAG) / sizeof(const char *);

static std::streamsize const MAX_IGNORE = \
    std::numeric_limits<std::streamsize>::max();

uint32_t field_info_conv_traits_base::getTypeId(const char * type) {
    for (uint32_t i = 0; i < VALID_TYPE_SIZE; ++i) {
        if ( 0 == strcmp(VALID_TYPE[i], type) )
            return i;
    }
    return 0;
}

void field_info_conv_traits_base::outputOFlags(
    std::ostream & ost, uint32_t oflags)
{
    char sep = '"';
    for (uint32_t i = 0; i < VALID_OFLAG_SIZE; ++i) {
        if ( static_cast<bool>(VALID_OFLAG_MAP[i] & oflags) ) {
            ost << sep << VALID_OFLAG[i];
            sep = '|';
        }
    }
    ost << '"';
}

uint32_t field_info_conv_traits_base::getOFlags(const char * oflags) {
    uint32_t ret = 0;
    const char * num = strstr(oflags, "num");
    if (  num && ( num == oflags || '_' != *(num - 1) )  )
        ret |= FIC_OUTPUT_FIELD_NUMBER;
    for (uint32_t i = 1; i < VALID_OFLAG_SIZE; ++i) {
        if ( strstr(oflags, VALID_OFLAG[i]) )
            ret |= VALID_OFLAG_MAP[i];
    }
    return ret;
}

void field_info_conv_traits_base::outputFieldBegin(
    std::ostream & ost,
    uint32_t oflags,
    const field_info * fieldInfo,
    const value_obj * valObj,
    const char * dividerName,
    const char * dividerAttr,
    const char * dividerVal)
{
    uint32_t metaInfo[] = {
        fieldInfo->MaxFieldNum(),
        fieldInfo->Offset(),
        fieldInfo->SizeInBit()
    };
    const char * sep = dividerName;
    genFieldName(ost, fieldInfo, oflags);
    for (uint32_t i = 1; i < 4; ++i) {
        if ( static_cast<bool>(VALID_OFLAG_MAP[i] & oflags) ) {
            ost << sep << VALID_OFLAG[i] << dividerVal << '"' << \
                metaInfo[i - 1] << '"';
            sep = dividerAttr;
        }
    }
    if (valObj) {
        uint32_t typeId = valObj->GetValType();
        if (typeId < VALID_TYPE_SIZE) {
            ost << sep << "type" << dividerVal << '"' << \
                VALID_TYPE[typeId] << '"';
        }
    }
}

void field_info_conv_traits_base::outputFieldVal(
    std::ostream & ost, const char * dividerBuf, const value_obj * valObj)
{
    switch ( valObj->GetValType() ) {
    case value_obj::BLN_VAL:
        {
            std::ios_base::fmtflags prevFmt = ost.flags(std::ios::boolalpha);
            ost << val_itf_selector<bln_val>::GetInterface(valObj)->Val();
            ost.flags(prevFmt);
        }
        break;
    case value_obj::INT_VAL:
        {
            ost << val_itf_selector<int_val>::GetInterface(valObj)->Val();
        }
        break;
    case value_obj::INT64_VAL: // hex
        {
            std::ios_base::fmtflags prevFmt = ost.flags(
                std::ios::hex | std::ios::showbase
            );
            ost << val_itf_selector<int64_val>::GetInterface(valObj)->Val();
            ost.flags(prevFmt);
        }
        break;
    case value_obj::FLT_VAL:
        {
            std::ios_base::fmtflags prevFmt = ost.flags(std::ios::fixed);
            ost << val_itf_selector<flt_val>::GetInterface(valObj)->Val();
            ost.flags(prevFmt);
        }
        break;
    case value_obj::FLT64_VAL: // dbl
        {
            std::ios_base::fmtflags prevFmt = ost.flags(std::ios::fixed);
            ost << val_itf_selector<flt64_val>::GetInterface(valObj)->Val();
            ost.flags(prevFmt);
        }
        break;
    case value_obj::BUF_VAL:
        {
            const buf_val * buf = \
                val_itf_selector<buf_val>::GetInterface(valObj);
            const uint8_t * data = \
                static_cast<const uint8_t *>( buf->Buf() );
            uint32_t n = buf->Size();
            if (data && n) {
                std::ios_base::fmtflags prevFmt = ost.flags(
                    std::ios::hex | std::ios::showbase
                );
                ost << static_cast<int>(data[0]);
                for (uint32_t i = 1; i < n; ++i)
                    ost << dividerBuf << static_cast<int>(data[i]);
                ost.flags(prevFmt);
            }
        }
        break;
    case value_obj::STR_VAL:
        {
            ost << val_itf_selector<str_val>::GetInterface(valObj)->Str();
        }
        break;
    case value_obj::WCS_VAL: // mbs
        {
            const wcs_val * wcsVal = \
                val_itf_selector<wcs_val>::GetInterface(valObj);
            str_val mbs;
            mbs.Resize( wcsVal->Size() );
            wcstombs( mbs.Str(), wcsVal->Str(), mbs.Size() );
            ost << mbs.Str();
        }
        break;
    }
}

bool field_info_conv_traits_base::encFieldVal(
    uint32_t typeId,
    std::istream & ist,
    const char * dividerBuf,
    char delim,
    buf_val * out_buf,
    field_info * io_fieldInfo)
{
    value_obj valObj;
    switch (typeId) {
    case value_obj::BLN_VAL:
        {
            std::ios_base::fmtflags prevFmt = ist.flags(std::ios::boolalpha);
            ist >> val_itf_selector<bln_val>::GetInterface(&valObj)->Val();
            ist.flags(prevFmt);
        }
        break;
    case value_obj::INT_VAL:
        {
            ist >> val_itf_selector<int_val>::GetInterface(&valObj)->Val();
        }
        break;
    case value_obj::INT64_VAL: // hex
        {
            std::ios_base::fmtflags prevFmt = ist.flags(
                std::ios::hex | std::ios::showbase
            );
            ist >> val_itf_selector<int64_val>::GetInterface(&valObj)->Val();
            ist.flags(prevFmt);
        }
        break;
    case value_obj::FLT_VAL:
        {
            std::ios_base::fmtflags prevFmt = ist.flags(std::ios::fixed);
            ist >> val_itf_selector<flt_val>::GetInterface(&valObj)->Val();
            ist.flags(prevFmt);
        }
        break;
    case value_obj::FLT64_VAL: // dbl
        {
            std::ios_base::fmtflags prevFmt = ist.flags(std::ios::fixed);
            ist >> val_itf_selector<flt64_val>::GetInterface(&valObj)->Val();
            ist.flags(prevFmt);
        }
        break;
    case value_obj::BUF_VAL:
        {
            std::stringstream ssBuf;
            ist.get(  *( ssBuf.rdbuf() ), delim  );
            std::string s = ssBuf.str();
            size_t pos = 0;
            uint32_t n = 0;
            do {
                pos = s.find(dividerBuf, pos);
                if (std::string::npos == pos)
                    break;
                ++n;
            } while (++pos);
            if (n++) {
                buf_val * bv = \
                    val_itf_selector<buf_val>::GetInterface(&valObj);
                bv->Resize(n);
                uint8_t * bvData = static_cast<uint8_t *>( bv->Buf() );
                ssBuf.str(s);
                ssBuf.flags(std::ios::hex | std::ios::showbase);
                uint32_t c, d = strlen(dividerBuf);
                for (uint32_t i = 0; i < n; ++i) {
                    ssBuf >> c;
                    bvData[i] = static_cast<uint8_t>(c);
                    ssBuf.ignore(d, delim);
                }
            }
        }
        break;
    case value_obj::STR_VAL:
        {
            std::stringbuf sBuf;
            ist.get(sBuf, delim);
            str_val * sv = val_itf_selector<str_val>::GetInterface(&valObj);
            sv->Resize( sBuf.in_avail() + 1 );
            sBuf.sgetn( sv->Str(), sBuf.in_avail() );
        }
        break;
    case value_obj::WCS_VAL: // mbs
        {
            std::stringbuf sBuf;
            ist.get(sBuf, delim);
            std::string s = sBuf.str();
            wcs_val * sv = val_itf_selector<wcs_val>::GetInterface(&valObj);
            sv->Resize(  ( s.length() + 1 ) * sizeof(wchar_t)  );
            mbstowcs( sv->Str(), s.c_str(), s.length() + 1 );
        }
        break;
    }
    if ( valObj.GetValType() )
        return io_fieldInfo->EncodeValue(out_buf, &valObj);
    return false;
}

bool field_info_conv_traits_base::findTag(
    std::istream & ist,
    const char * tag,
    char * buf,
    uint32_t size,
    char delimIgn,
    char delimTag)
{
    do {
        if ( ist.ignore(MAX_IGNORE, delimIgn).eof() ||
            ist.get(buf, size, delimTag).eof() )
        {
            return false;
        }
    } while ( 0 != strcmp(tag, buf) );
    return true;
}

bool field_info_conv_traits_base::getAttr(
    std::istream & ist, char dividerVal, char * out_buf, uint32_t size)
{
    char delim = '"';
    return !(
        ist.ignore(1, dividerVal).eof() ||
        ist.get(delim).eof() ||
        ist.get(out_buf, size, delim).eof()
    );
}

void field_info_conv_traits_xml::onOutputHead(
    std::ostream & ost, uint32_t oflags)
{
    ost << "<?xml version=\"1.0\"?>" << std::endl << "<protocol-data";
    if (oflags) {
        ost << " oflags=";
        outputOFlags(ost, oflags);
    }
    ost << '>';
}

void field_info_conv_traits_xml::onOutputCombinedField(
    std::ostream & ost,
    uint32_t oflags,
    const field_info * fieldInfo,
    stack_item * out_stackItem)
{
    myOutputFieldBegin(ost, oflags, fieldInfo, 0);
    out_stackItem->mFieldDes = fieldInfo->FieldDes();
    if ( isOutputFieldNum(oflags) )
        out_stackItem->mFieldNum = fieldInfo->FieldNumber();
    else
        out_stackItem->mFieldNum = 0;
}

void field_info_conv_traits_xml::onOutputLeafField(
    std::ostream & ost,
    uint32_t oflags,
    const field_info * fieldInfo,
    const value_obj * valObj)
{
    myOutputFieldBegin(ost, oflags, fieldInfo, valObj);
    outputFieldVal(ost, " ", valObj);
    stack_item dummyItem = {fieldInfo->FieldDes(), 0};
    if ( isOutputFieldNum(oflags) )
        dummyItem.mFieldNum = fieldInfo->FieldNumber();
    onOutputParentEnd(ost, dummyItem);
}

bool field_info_conv_traits_xml::findAttr(
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
        findTag(ist, elem, bufElem, sizeElem, '<', '>') &&
        '>' != ist.peek() &&
        findTag(ist, attr, bufAttr, sizeAttr, ' ', '=') &&
        getAttr(ist, '=', out_bufVal, sizeVal)
    );
}

uint32_t field_info_conv_traits_xml::onParseHead(std::istream & ist) {
    char elem[] = "protocol-data";
    memset( elem, 0, sizeof(elem) );
    char attr[] = "oflags";
    memset( attr, 0, sizeof(attr) );
    char oflags[] = "num|max_num|pos|size|leaf_only";
    memset( oflags, 0, sizeof(oflags) );
    if (  findAttr(
        ist,
        "protocol-data",
        "oflags",
        oflags,
        sizeof(oflags),
        elem,
        sizeof(elem),
        attr,
        sizeof(attr) )  )
    {
        return getOFlags(oflags);
    }
    return 0;
}

bool field_info_conv_traits_xml::onParseFieldValue(
    std::istream & ist,
    uint32_t oflags,
    buf_val * out_buf,
    field_info * io_fieldInfo)
{
    std::stringstream ssFieldName;
    genFieldName(ssFieldName, io_fieldInfo, oflags);
    std::string fieldName = ssFieldName.str();
    str_val svElem;
    svElem.Resize( fieldName.length() + 1 );
    char attr[] = "type";
    memset( attr, 0, sizeof(attr) );
    char type[] = "NUL";
    memset( type, 0, sizeof(type) );
    return (
        findAttr(
            ist,
            fieldName.c_str(),
            "type",
            type,
            sizeof(type),
            svElem.Str(),
            svElem.Size(),
            attr,
            sizeof(attr) ) &&
        !ist.ignore(MAX_IGNORE, '>').eof() &&
        encFieldVal( getTypeId(type), ist, " ", '<', out_buf, io_fieldInfo )
    );
}

void field_info_conv_traits_json::myOutputFieldBegin(
    std::ostream & ost,
    uint32_t oflags,
    const field_info * fieldInfo,
    const value_obj * valObj)
{
    uint32_t oflagMask = \
        FIC_OUTPUT_MAX_FIELD_NUM | \
        FIC_OUTPUT_FIELD_OFFSET  | \
        FIC_OUTPUT_FIELD_SIZE;
    if ( valObj || static_cast<bool>(oflags & oflagMask) ) {
        ost << "\"meta-";
        outputFieldBegin(
            ost, oflags, fieldInfo, valObj, "\":{\"", ",\"", "\":"
        );
        ost << "},";
    }
    ost << '"';
    genFieldName(ost, fieldInfo, oflags);
    ost << "\":";
}

void field_info_conv_traits_json::myOutputFieldVal(
    std::ostream & ost, const value_obj * valObj)
{
    uint32_t typeId = valObj->GetValType();
    if (value_obj::BUF_VAL == typeId)
        ost << '[';
    ost << '"';
    outputFieldVal(ost, "\",\"", valObj);
    ost << '"';
    if (value_obj::BUF_VAL == typeId)
        ost << ']';
}

void field_info_conv_traits_json::outputField(
    std::ostream & ost,
    uint32_t oflags,
    const field_info * fieldInfo,
    stack_item * out_stackItem,
    const value_obj * valObj)
{
    uint32_t m = fieldInfo->MaxFieldNum();
    uint32_t n = fieldInfo->FieldNumber();
    bool o = isOutputFieldNum(oflags);
    ost << mFieldSep;
    if (1 == n || o) {
        myOutputFieldBegin(ost, oflags, fieldInfo, valObj);
        if (m > 1 && !o)
            ost << '[';            
    }
    if (valObj)
        myOutputFieldVal(ost, valObj);
    if (out_stackItem) {
        out_stackItem->mFieldDes = fieldInfo->FieldDes();
        out_stackItem->mIsArrayEnd = (m > 1 && n == m && !o);
        ost << '{';
        mFieldSep = ' ';
    } else {
        if (m > 1 && n == m && !o)
            ost << ']';
        mFieldSep = ',';
    }
}

void field_info_conv_traits_json::onOutputHead(
    std::ostream & ost, uint32_t oflags)
{
    ost << '{';
    if (oflags) {
        ost << "\"protocol-oflags\":";
        outputOFlags(ost, oflags);
        mFieldSep = ',';
    } else
        mFieldSep = ' ';
}

bool field_info_conv_traits_json::findPair(
    std::istream & ist,
    const char * key,
    char * out_bufVal,
    uint32_t sizeVal,
    char * bufKey,
    uint32_t sizeKey)
{
    return (
        findTag(ist, key, bufKey, sizeKey, '"', '"') &&
        !ist.ignore(1, '"').eof() &&
        getAttr(ist, ':', out_bufVal, sizeVal)
    );
}

uint32_t field_info_conv_traits_json::onParseHead(std::istream & ist) {
    if ( !ist.ignore(1, '{').eof() && '"' == ist.peek() ) {
        char key[] = "protocol-oflags";
        memset( key, 0, sizeof(key) );
        char oflags[] = "num|max_num|pos|size|leaf_only";
        memset( oflags, 0, sizeof(oflags) );
        if ( findPair(
            ist,
            "protocol-oflags",
            oflags,
            sizeof(oflags),
            key,
            sizeof(key) )  )
        {
            return getOFlags(oflags);
        }
    }
    return 0;
}

bool field_info_conv_traits_json::seekToVal(
    std::istream & ist,
    const char * key,
    char * bufKey,
    uint32_t sizeKey)
{
    return findTag(ist, key, bufKey, sizeKey, '"', '"') && !(
        ist.ignore(2, ':').eof() ||
        ist.ignore(MAX_IGNORE, '"').eof()
    );
}

bool field_info_conv_traits_json::findFieldVal(
    std::istream & ist, uint32_t oflags, const field_info * fieldInfo)
{
    memset(mValType, 0, 4);
    std::stringstream ssFieldName;
    ssFieldName << "meta-";
    genFieldName(ssFieldName, fieldInfo, oflags);
    std::string fieldName = ssFieldName.str();
    str_val svElem;
    svElem.Resize( fieldName.length() + 1 );
    char attr[] = "type";
    memset( attr, 0, sizeof(attr) );
    return (
        findAttr(
            ist,
            fieldName.c_str(),
            "type",
            mValType,
            4,
            svElem.Str(),
            svElem.Size(),
            attr,
            sizeof(attr)
        ) && seekToVal(
            ist,
            fieldName.c_str() + 5, // skip "meta-"
            svElem.Str(),
            svElem.Size() - 5
        )
    );
}

bool field_info_conv_traits_json::onParseFieldValue(
    std::istream & ist,
    uint32_t oflags,
    buf_val * out_buf,
    field_info * io_fieldInfo)
{
    if ( 1 == io_fieldInfo->FieldNumber() || isOutputFieldNum(oflags) ) {
        if ( !findFieldVal(ist, oflags, io_fieldInfo) )
            return false;
    } else if (
        ist.ignore(MAX_IGNORE, ',').eof() ||
        ist.ignore(MAX_IGNORE, '"').eof() )
    {
        return false;
    }
    uint32_t typeId = getTypeId(mValType);
    return encFieldVal(
        typeId,
        ist,
        "\",\"",
        (value_obj::BUF_VAL == typeId)? ']': '"',
        out_buf,
        io_fieldInfo
    );
}
