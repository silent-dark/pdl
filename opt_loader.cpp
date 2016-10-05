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

#include "opt_loader.h"

namespace pdl {
    const meta_opt_loader_bool META_OPT_LOADER_BOOL;
    const meta_opt_loader_int META_OPT_LOADER_INT;
    const meta_opt_loader_float META_OPT_LOADER_FLOAT;
    const meta_opt_loader_ipv4 META_OPT_LOADER_IPV4;
    const meta_opt_loader_str META_OPT_LOADER_STR;
}

using namespace pdl;

int meta_opt_loader_bool::load(
    void * out_loadBuf, size_t bufSize,
    const char * optVal) const
{
    bool val = false;
    if (optVal) {
        static const char * const FALSE_VAL_MAP[] = {
            "0", "false", "off", "no", "F", "N"
        };
        static const size_t FALSE_VAL_MAP_SIZE = \
            sizeof(FALSE_VAL_MAP) / sizeof(const char *);

        val = true;
        for (size_t i = 0; i < FALSE_VAL_MAP_SIZE; ++i) {
            if ( 0 == strcasecmp(FALSE_VAL_MAP[i], optVal) ) {
                val = false;
                break;
            }
        }
    }
    *static_cast<bool *>(out_loadBuf) = val;
    return 0;
}

int meta_opt_loader_bool::save(
    const void * optBuf, size_t bufSize,
    std::string & out_saveBuf) const
{
    out_saveBuf = ( *static_cast<const bool *>(optBuf) )? "1": "0";
    return 0;
}

int meta_opt_loader_int::load(
    void * out_loadBuf, size_t bufSize,
    const char * optVal) const
{
    int err = 0;
    switch (bufSize) {
    case sizeof(uint8_t):
        *static_cast<uint8_t *>(out_loadBuf) = \
            static_cast<uint8_t>( atoi(optVal) );
        break;
    case sizeof(uint16_t):
        *static_cast<uint16_t *>(out_loadBuf) = \
            static_cast<uint16_t>( atoi(optVal) );
        break;
    case sizeof(uint32_t):
        *static_cast<uint32_t *>(out_loadBuf) = \
            static_cast<uint32_t>( atol(optVal) );
        break;
    case sizeof(uint64_t):
        *static_cast<uint64_t *>(out_loadBuf) = \
            static_cast<uint64_t>( atoll(optVal) );
        break;
    default:
        err = EINVAL;
        if (err > 0)
            err = -err;
    }
    return err;
}

int meta_opt_loader_int::save(
    const void * optBuf, size_t bufSize,
    std::string & out_saveBuf) const
{
    int err = 0;
    std::stringstream sst;
    switch (bufSize) {
    case sizeof(uint8_t):
        sst << static_cast<int>( *static_cast<const uint8_t *>(optBuf) );
        break;
    case sizeof(uint16_t):
        sst << static_cast<int>( *static_cast<const uint16_t *>(optBuf) );
        break;
    case sizeof(uint32_t):
        sst << static_cast<int>( *static_cast<const uint32_t *>(optBuf) );
        break;
    case sizeof(uint64_t):
        sst << std::hex << std::showbase << \
            *static_cast<const uint64_t *>(optBuf);
        break;
    default:
        err = EINVAL;
        if (err > 0)
            err = -err;
    }
    if (0 == err)
        out_saveBuf = sst.str();
    return err;
}

int meta_opt_loader_float::load(
    void * out_loadBuf, size_t bufSize,
    const char * optVal) const
{
    int err = 0;
    switch (bufSize) {
    case sizeof(float):
        *static_cast<float *>(out_loadBuf) = static_cast<float>( atof(optVal) );
        break;
    case sizeof(double):
        *static_cast<double *>(out_loadBuf) = atof(optVal);
        break;
    default:
        err = EINVAL;
        if (err > 0)
            err = -err;
    }
    return err;
}

int meta_opt_loader_float::save(
    const void * optBuf, size_t bufSize,
    std::string & out_saveBuf) const
{
    int err = 0;
    std::stringstream sst;
    switch (bufSize) {
    case sizeof(float):
        sst << *static_cast<const float *>(optBuf);
        break;
    case sizeof(double):
        sst << *static_cast<const double *>(optBuf);
        break;
    default:
        err = EINVAL;
        if (err > 0)
            err = -err;
    }
    if (0 == err)
        out_saveBuf = sst.str();
    return err;
}

int meta_opt_loader_ipv4::load(
    void * out_loadBuf, size_t bufSize,
    const char * optVal) const
{
    int err = EINVAL;
    if (err > 0)
        err = -err;
    if ( sizeof(uint32_t) == bufSize ) {
        uint32_t ipv4[4] = {0};
        if ( 4 == sscanf(
            optVal, "%d.%d.%d.%d", ipv4, ipv4 + 1, ipv4 + 2, ipv4 + 3) )
        {
            *static_cast<uint32_t *>(out_loadBuf) = \
                (ipv4[0] << 24) | (ipv4[1] << 16) | (ipv4[2] << 8 ) | ipv4[3];
            err = 0;
        }
    }
    return err;
}

int meta_opt_loader_ipv4::save(
    const void * optBuf, size_t bufSize,
    std::string & out_saveBuf) const
{
    int err = EINVAL;
    if (err > 0)
        err = -err;
    if ( sizeof(uint32_t) == bufSize ) {
        uint32_t ipv4 = *static_cast<const uint32_t *>(optBuf);
        char ipv4Buf[] = "255.255.255.255";
        sprintf(
            ipv4Buf, "%d.%d.%d.%d",
            ipv4 >> 24,
            (ipv4 >> 16) & 0xff,
            (ipv4 >> 8)  & 0xff,
            ipv4 & 0xff
        );
        out_saveBuf = ipv4Buf;
        err = 0;
    }
    return err;
}

int meta_opt_loader_str::load(
    void * out_loadBuf, size_t bufSize,
    const char * optVal) const
{
    int err = 0;
    switch (bufSize) {
    case sizeof(std::string):
        *static_cast<std::string *>(out_loadBuf) = optVal;
        break;
    case sizeof(str_val):
        {
            str_val * strBuf = static_cast<str_val *>(out_loadBuf);
            strBuf->Resize( strlen(optVal) + 1 );
            strcpy(strBuf->Str(), optVal);
        }
        break;
    case sizeof(char *):
        *static_cast<char **>(out_loadBuf) = strdup(optVal);
        break;
    default:
        err = EINVAL;
        if (err > 0)
            err = -err;
    }
    return err;
}

int meta_opt_loader_str::save(
    const void * optBuf, size_t bufSize,
    std::string & out_saveBuf) const
{
    int err = 0;
    switch (bufSize) {
    case sizeof(std::string):
        out_saveBuf = *static_cast<const std::string *>(optBuf);
        break;
    case sizeof(str_val):
        out_saveBuf = static_cast<const str_val *>(optBuf)->Str();
        break;
    case sizeof(char *):
        out_saveBuf = *static_cast<char * const *>(optBuf);
        break;
    default:
        err = EINVAL;
        if (err > 0)
            err = -err;
    }
    return err;
}

#ifdef OPT_LOADER_UT

struct prog_args {
    uint16_t _threadCount;
    std::string _testUsr;
    std::string _testPswd;
    std::string _testUtkn;
    std::string _testMedID;
    uint32_t _testAuthCode;

    prog_args() {
        _threadCount = 0;
        _testAuthCode = 0;
    }
};
prog_args gProgArgs;

struct meta_arg_thread_count: pdl::meta_opt {
    meta_arg_thread_count()
        : pdl::meta_opt(pdl::META_OPT_LOADER_INT)
    {}
    virtual const char * optKey() const {
        return "-t";
    }
    virtual bool isNoneValueOpt() const {
        return false;
    }
    virtual size_t optSize() const {
        return sizeof(uint16_t);
    }
    virtual void * optAddr(void * baseAddr) const {
        return &( static_cast<prog_args *>(baseAddr)->_threadCount );
    }
    virtual const char * defaultVal() const {
        return "1";
    }
};

struct meta_arg_test_usr: pdl::meta_opt {
    meta_arg_test_usr()
        : pdl::meta_opt(pdl::META_OPT_LOADER_STR)
    {}
    virtual const char * optKey() const {
        return "-u";
    }
    virtual bool isNoneValueOpt() const {
        return false;
    }
    virtual size_t optSize() const {
        return sizeof(std::string);
    }
    virtual void * optAddr(void * baseAddr) const {
        return &( static_cast<prog_args *>(baseAddr)->_testUsr );
    }
    virtual const char * defaultVal() const {
        return "test_http_drm";
    }
};

struct meta_arg_test_pswd: pdl::meta_opt {
    meta_arg_test_pswd()
        : pdl::meta_opt(pdl::META_OPT_LOADER_STR)
    {}
    virtual const char * optKey() const {
        return "-p";
    }
    virtual bool isNoneValueOpt() const {
        return false;
    }
    virtual size_t optSize() const {
        return sizeof(std::string);
    }
    virtual void * optAddr(void * baseAddr) const {
        return &( static_cast<prog_args *>(baseAddr)->_testPswd );
    }
    virtual const char * defaultVal() const {
        return "123456";
    }
};

struct meta_arg_test_utkn: pdl::meta_opt {
    meta_arg_test_utkn()
        : pdl::meta_opt(pdl::META_OPT_LOADER_STR)
    {}
    virtual const char * optKey() const {
        return "--user-token";
    }
    virtual bool isNoneValueOpt() const {
        return false;
    }
    virtual size_t optSize() const {
        return sizeof(std::string);
    }
    virtual void * optAddr(void * baseAddr) const {
        return &( static_cast<prog_args *>(baseAddr)->_testUtkn );
    }
    virtual const char * defaultVal() const {
        return "abcdef";
    }
};

struct meta_arg_test_med_id: pdl::meta_opt {
    meta_arg_test_med_id()
        : pdl::meta_opt(pdl::META_OPT_LOADER_STR)
    {}
    virtual const char * optKey() const {
        return "-m";
    }
    virtual bool isNoneValueOpt() const {
        return false;
    }
    virtual size_t optSize() const {
        return sizeof(std::string);
    }
    virtual void * optAddr(void * baseAddr) const {
        return &( static_cast<prog_args *>(baseAddr)->_testMedID );
    }
    virtual const char * defaultVal() const {
        return "test_video";
    }
};

struct meta_arg_test_auth_code: pdl::meta_opt {
    meta_arg_test_auth_code()
        : pdl::meta_opt(pdl::META_OPT_LOADER_INT)
    {}
    virtual const char * optKey() const {
        return "-a";
    }
    virtual bool isNoneValueOpt() const {
        return false;
    }
    virtual size_t optSize() const {
        return sizeof(uint32_t);
    }
    virtual void * optAddr(void * baseAddr) const {
        return &( static_cast<prog_args *>(baseAddr)->_testAuthCode );
    }
    virtual const char * defaultVal() const {
        return "654321";
    }
};

struct prog_args_loader: pdl::opt_loader<> {
    prog_args_loader()
        : pdl::opt_loader<>(6)
    {
        regMetaOpt(new meta_arg_thread_count);
        regMetaOpt(new meta_arg_test_usr);
        regMetaOpt(new meta_arg_test_pswd);
        regMetaOpt(new meta_arg_test_utkn);
        regMetaOpt(new meta_arg_test_med_id);
        regMetaOpt(new meta_arg_test_auth_code);
    }
};

int main(int argc, char ** argv)
{
    static prog_args_loader gProgArgsLoader;
    pdl::opt_parser progArgsParser(argc, argv);
    gProgArgsLoader.load( progArgsParser, &gProgArgs, sizeof(gProgArgs) );

    return 0;
}

#endif // OPT_LOADER_UT
