#include <stdlib.h>
#include "field_info_conv.h"

using namespace pdl;

void field_info_conv_traits_xml::onOutputHead(
    std::ostream & out, uint32_t outputFlags)
{
    out << "<?xml version=\"1.0\"?>" << std::endl << "<protocol-data";
    if (outputFlags) {
        out << " outflags=";
        char sep = '"';
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_SIZE & outputFlags) ) {
            out << sep << "size";
            sep = '|';
        }
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_OFFSET & outputFlags) ) {
            out << sep << "pos";
            sep = '|';
        }
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) ) {
            out << sep << "num";
            sep = '|';
        }
        if ( static_cast<bool>(FIC_OUTPUT_MAX_FIELD_NUM & outputFlags) ) {
            out << sep << "max_num";
            sep = '|';
        }
        out << '"';
    }
    out << '>';
}

void field_info_conv_traits_xml::outputFieldBegin(
    std::ostream & out, uint32_t outputFlags, const field_info * fieldInfo)
{
    out << '<' << fieldInfo->FieldDes()->FieldName();
    if ( static_cast<bool>(FIC_OUTPUT_FIELD_SIZE & outputFlags) )
        out << " size=\"" << fieldInfo->SizeInBit() << '"';
    if ( static_cast<bool>(FIC_OUTPUT_FIELD_OFFSET & outputFlags) )
        out << " pos=\"" << fieldInfo->Offset() << '"';
    if ( static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) )
        out << " num=\"" << fieldInfo->FieldNumber() << '"';
    if ( static_cast<bool>(FIC_OUTPUT_MAX_FIELD_NUM & outputFlags) )
        out << " max_num=\"" << fieldInfo->MaxFieldNumber() << '"';
}

void field_info_conv_traits_xml::outputFieldValue(
    std::ostream & out, const value_obj * valObj)
{
    out << " type=";
    switch ( valObj->GetValType() ) {
        case value_obj::NUL_VAL:
        {
            out << "\"NUL\">";
        }
        break;

        case value_obj::BOOL_VAL:
        {
            out << "\"bool\">";
            std::ios_base::fmtflags prevFmt = out.flags(std::ios::boolalpha);
            out << val_itf_selector<bool_val>::GetInterface(valObj)->Val();
            out.flags(prevFmt);
        }
        break;

        case value_obj::INT_VAL:
        {
            out << "\"int\">" << \
                val_itf_selector<int_val>::GetInterface(valObj)->Val();
        }
        break;

        case value_obj::INT64_VAL:
        {
            out << "\"hex\">";
            std::ios_base::fmtflags prevFmt = out.flags(
                std::ios::hex | std::ios::showbase
            );
            out << val_itf_selector<int64_val>::GetInterface(valObj)->Val();
            out.flags(prevFmt);
        }
        break;

        case value_obj::FLT_VAL:
        {
            out << "\"float\">";
            std::ios_base::fmtflags prevFmt = out.flags(std::ios::fixed);
            out << val_itf_selector<flt_val>::GetInterface(valObj)->Val();
            out.flags(prevFmt);
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
                out << "\"bytes\">";
                std::ios_base::fmtflags prevFmt = out.flags(
                    std::ios::hex | std::ios::showbase
                );
                out << static_cast<int>(data[0]);
                for (uint32_t i = 1; i < n; ++i)
                    out << ' ' << static_cast<int>(data[i]);
                out.flags(prevFmt);
            } else
                out << "\"NUL\">";
        }
        break;

        case value_obj::STR_VAL:
        {
            out << "\"string\">" << \
                val_itf_selector<str_val>::GetInterface(valObj)->Str();
        }
        break;

        case value_obj::WCS_VAL:
        {
            const wcs_val * wcsVal = \
                val_itf_selector<wcs_val>::GetInterface(valObj);
            str_val mbs;
            mbs.Resize( wcsVal->Size() * sizeof(wchar_t) );
            wcstombs( mbs.Str(), wcsVal->Str(), mbs.Size() );
            out << "\"string\">" << mbs.Str();
        }
        break;

        case value_obj::OBJ_VAL:
        {
            out << '"' << valObj->GetTypeName() << "\">...";
        }
        break;
    }
}

void field_info_conv_traits_json::onOutputHead(
    std::ostream & out, uint32_t outputFlags)
{
    out << '{';
    if (outputFlags) {
        out << "\"protocol-outflags\":";
        char sep = '"';
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_SIZE & outputFlags) ) {
            out << sep << "size";
            sep = '|';
        }
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_OFFSET & outputFlags) ) {
            out << sep << "pos";
            sep = '|';
        }
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) ) {
            out << sep << "num";
            sep = '|';
        }
        if ( static_cast<bool>(FIC_OUTPUT_MAX_FIELD_NUM & outputFlags) ) {
            out << sep << "max_num";
            sep = '|';
        }
        out << "\",";
    }
    mFieldSep = ' ';
}

void field_info_conv_traits_json::outputFieldBegin(
    std::ostream & out, uint32_t outputFlags, const field_info * fieldInfo)
{
    uint32_t mask = \
        FIC_OUTPUT_FIELD_SIZE | \
        FIC_OUTPUT_FIELD_OFFSET | \
        FIC_OUTPUT_MAX_FIELD_NUM;
    if ( static_cast<bool>(outputFlags & mask) ) {
        out << "\"meta-" << fieldInfo->FieldDes()->FieldName();
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) )
            out << '-' << fieldInfo->FieldNumber();
        out << "\":";
        char sep = '{';
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_SIZE & outputFlags) ) {
            out << sep << "\"size\":\"" << fieldInfo->SizeInBit() << '"';
            sep = ',';
        }
        if ( static_cast<bool>(FIC_OUTPUT_FIELD_OFFSET & outputFlags) ) {
            out << sep << "\"pos\":\"" << fieldInfo->Offset() << '"';
            sep = ',';
        }
        if ( static_cast<bool>(FIC_OUTPUT_MAX_FIELD_NUM & outputFlags) ) {
            out << sep << "\"max_num\":\"" << fieldInfo->MaxFieldNumber() << \
                '"';
            sep = ',';
        }
        out << "},";
    }
    out << '"' << fieldInfo->FieldDes()->FieldName();
    if ( static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) )
        out << '-' << fieldInfo->FieldNumber();
    out << "\":";
}

void field_info_conv_traits_json::outputFieldValue(
    std::ostream & out, const value_obj * valObj)
{
    switch ( valObj->GetValType() ) {
        case value_obj::NUL_VAL:
        {
            out << "\"(NUL)\"";
        }
        break;

        case value_obj::BOOL_VAL:
        {
            out << '"';
            std::ios_base::fmtflags prevFmt = out.flags(std::ios::boolalpha);
            out << val_itf_selector<bool_val>::GetInterface(valObj)->Val();
            out.flags(prevFmt);
            out << '"';
        }
        break;

        case value_obj::INT_VAL:
        {
            out << '"' << \
                val_itf_selector<int_val>::GetInterface(valObj)->Val() << '"';
        }
        break;

        case value_obj::INT64_VAL:
        {
            out << '"';
            std::ios_base::fmtflags prevFmt = out.flags(
                std::ios::hex | std::ios::showbase
            );
            out << val_itf_selector<int64_val>::GetInterface(valObj)->Val();
            out.flags(prevFmt);
            out << '"';
        }
        break;

        case value_obj::FLT_VAL:
        {
            out << '"';
            std::ios_base::fmtflags prevFmt = out.flags(std::ios::fixed);
            out << val_itf_selector<flt_val>::GetInterface(valObj)->Val();
            out.flags(prevFmt);
            out << '"';
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
                out << "[\"";
                std::ios_base::fmtflags prevFmt = out.flags(
                    std::ios::hex | std::ios::showbase
                );
                out << static_cast<int>(data[0]);
                for (uint32_t i = 1; i < n; ++i)
                    out << "\",\"" << static_cast<int>(data[i]);
                out.flags(prevFmt);
                out << "\"]";
            } else
                out << "\"(NUL)\"";
        }
        break;

        case value_obj::STR_VAL:
        {
            out << '"' << \
                val_itf_selector<str_val>::GetInterface(valObj)->Str() << '"';
        }
        break;

        case value_obj::WCS_VAL:
        {
            const wcs_val * wcsVal = \
                val_itf_selector<wcs_val>::GetInterface(valObj);
            str_val mbs;
            mbs.Resize( wcsVal->Size() * sizeof(wchar_t) );
            wcstombs( mbs.Str(), wcsVal->Str(), mbs.Size() );
            out << '"' << mbs.Str() << '"';
        }
        break;

        case value_obj::OBJ_VAL:
        {
            out << '"' << valObj->GetTypeName() << "(...)\"";
        }
        break;
    }
}

void field_info_conv_traits_json::onOutputCombinedField(
    std::ostream & out,
    uint32_t outputFlags,
    const field_info * fieldInfo,
    stack_item * out_stackItem)
{
    out_stackItem->mIsArrayEnd = false;
    out << mFieldSep;
    if ( 1 == fieldInfo->MaxFieldNumber() || \
        static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) )
    {
        outputFieldBegin(out, outputFlags, fieldInfo);
    } else {
        if ( 1 == fieldInfo->FieldNumber() ) {
            outputFieldBegin(out, outputFlags, fieldInfo);
            out << '[';
        } else if ( fieldInfo->FieldNumber() == fieldInfo->MaxFieldNumber() )
            out_stackItem->mIsArrayEnd = true;
    }
    out << '{';
    out_stackItem->mFieldDes = fieldInfo->FieldDes();
    mFieldSep = ' ';
}

void field_info_conv_traits_json::onOutputLeafField(
    std::ostream & out,
    uint32_t outputFlags,
    const field_info * fieldInfo,
    const value_obj * valObj)
{
    out << mFieldSep;
    if ( 1 == fieldInfo->MaxFieldNumber() || \
        static_cast<bool>(FIC_OUTPUT_FIELD_NUMBER & outputFlags) )
    {
        outputFieldBegin(out, outputFlags, fieldInfo);
        outputFieldValue(out, valObj);
    } else {
        if ( 1 == fieldInfo->FieldNumber() ) {
            outputFieldBegin(out, outputFlags, fieldInfo);
            out << '[';
        }
        outputFieldValue(out, valObj);
        if ( fieldInfo->FieldNumber() == fieldInfo->MaxFieldNumber() )
            out << ']';
    }
    mFieldSep = ',';
}

