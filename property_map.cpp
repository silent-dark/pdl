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

#include "property_map.h"

using namespace pdl;

#ifdef PROPERTY_MAP_UT

#include <stdio.h>
#include <iostream>

struct test_for_each: property_map<>::for_each_callback {
    virtual int Callback(
        const char * key, obj_ptr<value_obj> & val, int order)
    {
        switch ( val->GetValType() ) {
        case value_obj::INT_VAL:
            {
                int_val * intVal = \
                    val_itf_selector<int_val>::GetInterface(val);
                std::cout << key << ":" << intVal->Val() << std::endl;
            }
            break;
        case value_obj::STR_VAL:
            {
                str_val * strVal = \
                    val_itf_selector<str_val>::GetInterface(val);
                std::cout << key << ":" << strVal->Str() << std::endl;
            }
            break;
        case value_obj::WCS_VAL:
            {
                wcs_val * wcsVal = \
                    val_itf_selector<wcs_val>::GetInterface(val);
                printf("%s:%ls\n", key, wcsVal->Str() );
            }
            break;
        }
        return 0;
    }
};

int main() {
    obj_constructor<value_obj> valObjConstructor;
    property_map<> propMap;
    obj_ptr<value_obj> propVal;
    test_for_each cb;
    setlocale(LC_ALL,"");

    propVal = propMap["name-zh"] = obj_ptr<value_obj>(&valObjConstructor);
    wcs_val * nameZh = val_itf_selector<wcs_val>::GetInterface(propVal);
    nameZh->Resize( sizeof(L"黎青") );
    wcscpy(nameZh->Str(), L"黎青");

    propVal = propMap["name-en"] = obj_ptr<value_obj>(&valObjConstructor);
    str_val * nameEn = val_itf_selector<str_val>::GetInterface(propVal);
    nameEn->Resize( sizeof("Qing Li") );
    strcpy(nameEn->Str(), "Qing Li");

    propVal = propMap["age"] = obj_ptr<value_obj>(&valObjConstructor);
    int_val * age = val_itf_selector<int_val>::GetInterface(propVal);
    age->Val() = 35;

    propVal = propMap["mobile"] = obj_ptr<value_obj>(&valObjConstructor);
    str_val * mobile = val_itf_selector<str_val>::GetInterface(propVal);
    mobile->Resize( sizeof("18611924492") );
    strcpy(mobile->Str(), "18611924492");

    propVal = propMap["email"] = obj_ptr<value_obj>(&valObjConstructor);
    str_val * email = val_itf_selector<str_val>::GetInterface(propVal);
    email->Resize( sizeof("liqing@imobiletv.cn") );
    strcpy(email->Str(), "liqing@imobiletv.cn");

    propMap.ForEach(&cb);
    if ( propMap.GetValue("name-zh") )
        propMap.Remove("name-zh");
    propMap.ForEach(&cb);
    return 0;
}

#endif // PROPERTY_MAP_UT

