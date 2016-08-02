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

#include "table.h"

using namespace pdl;

#ifdef TABLE_UT

#include <iostream>

int main() {
    std::cout << "// test index_map." << std::endl;
    index_map<char> idxMap;
    char c;
    for (c = 'C'; c >= 'A'; --c)
        std::cout << "insert '" << c << "' with index " << idxMap[c] << std::endl;

    std::cout << "// test table." << std::endl;
    table<char,int> tbl;
    tbl.Insert('A', 1);
    tbl.Insert('A', 2);
    tbl.Insert('B', 1);
    tbl.Insert('C', 2);
    tbl.Remove('A', 1);

    char cBuf[2] = {0};
    int iBuf[2] = {0};
    uint32_t i, j, n;
    for (c = 'A'; c <= 'C'; ++c) {
        std::cout << "Find '" << c << "'";
        n = tbl.FindWithA(c, iBuf);
        for (i = 0; i < n; ++i)
            std::cout << ":" << iBuf[i];
        std::cout << std::endl;
    }
    for (j = 1; j <= 2; ++j) {
        std::cout << "Find " << j << "";
        n = tbl.FindWithB(j, cBuf);
        for (i = 0; i < n; ++i)
            std::cout << ":" << cBuf[i];
        std::cout << std::endl;
    }
    return 0;
}

#endif // TABLE_UT

