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

#ifndef _OPT_LOADER_H_
#define _OPT_LOADER_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cctype>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include "value_obj.h"

namespace pdl {

class opt_parser {
    char ** mArgBegin;
    char ** mArgEnd;

public:
    opt_parser(int argc, char ** argv) {
        mArgBegin = argv;
        mArgEnd = argv + argc;
    }
    // for none-value option.
    const char * isOptExisted(const std::string & option) const {
        char ** opt = std::find(mArgBegin, mArgEnd, option);
        return (opt != mArgEnd)? *opt: 0;
    }
    const char * getOpt(const std::string & option) const {
        char ** opt = std::find(mArgBegin, mArgEnd, option);
        if (opt != mArgEnd) {
            if (++opt != mArgEnd)
                return *opt;
        }
        return 0;
    }
};

struct meta_opt_loader {
    virtual int load(
        void * out_loadBuf, size_t bufSize,
        const char * optVal
    ) const = 0;
    virtual int save(
        const void * optBuf, size_t bufSize,
        std::string & out_saveBuf
    ) const = 0;
};

struct meta_opt_loader_bool: meta_opt_loader {
    virtual int load(
        void * out_loadBuf, size_t bufSize,
        const char * optVal
    ) const;
    virtual int save(
        const void * optBuf, size_t bufSize,
        std::string & out_saveBuf
    ) const;
};
extern const meta_opt_loader_bool META_OPT_LOADER_BOOL;

struct meta_opt_loader_int: meta_opt_loader {
    virtual int load(
        void * out_loadBuf, size_t bufSize,
        const char * optVal
    ) const;
    virtual int save(
        const void * optBuf, size_t bufSize,
        std::string & out_saveBuf
    ) const;
};
extern const meta_opt_loader_int META_OPT_LOADER_INT;

struct meta_opt_loader_float: meta_opt_loader {
    virtual int load(
        void * out_loadBuf, size_t bufSize,
        const char * optVal
    ) const;
    virtual int save(
        const void * optBuf, size_t bufSize,
        std::string & out_saveBuf
    ) const;
};
extern const meta_opt_loader_float META_OPT_LOADER_FLOAT;

struct meta_opt_loader_ipv4: meta_opt_loader {
    virtual int load(
        void * out_loadBuf, size_t bufSize,
        const char * optVal
    ) const;
    virtual int save(
        const void * optBuf, size_t bufSize,
        std::string & out_saveBuf
    ) const;
};
extern const meta_opt_loader_ipv4 META_OPT_LOADER_IPV4;

struct meta_opt_loader_str: meta_opt_loader {
    virtual int load(
        void * out_loadBuf, size_t bufSize,
        const char * optVal
    ) const;
    virtual int save(
        const void * optBuf, size_t bufSize,
        std::string & out_saveBuf
    ) const;
};
extern const meta_opt_loader_str META_OPT_LOADER_STR;

class meta_opt {
public:
    virtual const char * optKey() const = 0;
    virtual bool isNoneValueOpt() const = 0;
    virtual size_t optSize() const = 0;
    virtual const char * defaultVal() const = 0;

protected:
    const meta_opt_loader & mOptLoader;

    meta_opt(const meta_opt_loader & loader)
        : mOptLoader(loader)
    {}

public:
    virtual ~meta_opt() {}

    const meta_opt_loader & optLoader() const {
        return mOptLoader;
    }

    size_t _optOffset; // runtime value after load or save.
};

template <typename PT = opt_parser>
class opt_loader {
public:
    typedef PT opt_parser_t;

private:
    typedef obj_ptr<void *,meta_opt> meta_opt_ptr;
    typedef std_allocator<meta_opt_ptr,void *> meta_opt_ptr_allocator;
    typedef std::vector<meta_opt_ptr,meta_opt_ptr_allocator> meta_opt_set;

    meta_opt_set mMetaOptSet;

public:
    opt_loader(size_t maxOptCount) {
        mMetaOptSet.reserve(maxOptCount);
    }
    void regMetaOpt(meta_opt * metaOpt) {
        meta_opt_ptr::constructor_type objConstructor(metaOpt);
        mMetaOptSet.push_back( meta_opt_ptr(&objConstructor) );
    }
    int load(opt_parser_t & optParser, void * out_optBuf, size_t bufSize);
    int save(std::ostream & ost, const void * optBuf, size_t bufSize);
};

template <typename PT>
int opt_loader<PT>::load(
    opt_parser_t & optParser, void * out_optBuf, size_t bufSize)
{
    int err = 0;
    uint8_t * optAddr = static_cast<uint8_t *>(out_optBuf);
    uint8_t * optEnd = optAddr + bufSize;
    meta_opt * metaOpt;
    const char * optVal;
    size_t i, n;
    for (i = 0, n = mMetaOptSet.size(); i < n && optAddr < optEnd; ++i) {
        metaOpt = mMetaOptSet[i].ObjAt(0);
        optVal = metaOpt->isNoneValueOpt()? \
            optParser.isOptExisted( metaOpt->optKey() ): \
            optParser.getOpt( metaOpt->optKey() );
        err = metaOpt->optLoader().load(
            optAddr, metaOpt->optSize(), optVal? optVal: metaOpt->defaultVal()
        );
        if (err < 0)
            break;
        metaOpt->_optOffset = optAddr - static_cast<uint8_t *>(out_optBuf);
        optAddr += metaOpt->optSize();
    }
    return err;
}

template <typename PT>
int opt_loader<PT>::save(
    std::ostream & ost, const void * optBuf, size_t bufSize)
{
    int err = 0;
    const uint8_t * optAddr = static_cast<const uint8_t *>(optBuf);
    const uint8_t * optEnd = optAddr + bufSize;
    meta_opt * metaOpt;
    std::string optVal;
    size_t i, n;
    for (i = 0, n = mMetaOptSet.size(); i < n && optAddr < optEnd; ++i) {
        metaOpt = mMetaOptSet[i].ObjAt(0);
        err = metaOpt->optLoader().save(
            optAddr, metaOpt->optSize(), optVal
        );
        if (err < 0)
            break;
        ost << metaOpt->optKey() << '=' << optVal << std::endl;
        metaOpt->_optOffset = optAddr - static_cast<const uint8_t *>(optBuf);
        optAddr += metaOpt->optSize();
    }
    return err;
}

} // namespace pdl

#endif // _OPT_LOADER_H_
