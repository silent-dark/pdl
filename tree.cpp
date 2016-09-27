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

#include "tree.h"

using namespace pdl;

#ifdef TREE_UT

#include <iostream>

typedef tree< tree_traits<void,int> > test_tree;
typedef test_tree::node_ptr test_node_ptr;
typedef test_tree::stack_item test_stack_item;

struct test_tree_callback: test_tree::for_each_callback {
    virtual void onPushStack(test_stack_item * io_stackTop) {
        std::cout << "  >> push {"
                  << io_stackTop->mTreeNode->GetValue()
                  << "}:"
                  << io_stackTop->mNextSubNodeIdx
                  << std::endl;
    }
    virtual void afterPopStack(test_stack_item * io_stackTop) {
        std::cout << "  << pop {"
                  << io_stackTop->mTreeNode->GetValue()
                  << "}:"
                  << io_stackTop->mNextSubNodeIdx
                  << std::endl;
    }
    virtual int onTraversal(test_stack_item * io_stackTop, int order) {
        std::cout << "trav {"
                  << io_stackTop->mTreeNode->GetValue()
                  << "}:"
                  << io_stackTop->mNextSubNodeIdx
                  << std::endl;
        return 0;
    }
};

int main() {
    test_tree testTree( test_tree::CreateNode(4) );
    test_node_ptr rootNode = testTree.GetRootNode();
    rootNode->SetSubNodeCapacity(2);
    rootNode->SetSubNode( 0, test_tree::CreateNode(2) );
    rootNode->SetSubNode( 1, test_tree::CreateNode(5) );
    rootNode = rootNode->GetSubNode(0);
    rootNode->SetSubNodeCapacity(2);
    rootNode->SetSubNode( 0, test_tree::CreateNode(1) );
    rootNode->SetSubNode( 1, test_tree::CreateNode(3) );
    test_tree_callback cb;
    testTree.ForEach(&cb, 1);
    return 0;
}

#endif // TREE_UT

