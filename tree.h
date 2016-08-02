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

#ifndef _TREE_H_
#define _TREE_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <vector>
#include "value_obj.h"

namespace pdl {

template <typename VT, uint32_t N>
class tree_node {
    template <typename NT>
    friend class tree_node_adapter;

public:
    typedef VT val_type;
    typedef tree_node * node_ptr;
    typedef const tree_node * node_ptr_c;
    typedef node_ptr sub_node_buf[N];

private:
    val_type mVal;
    sub_node_buf mSubNodes;

    val_type * getValue() {
        return &mVal;
    }
    const val_type * getValue() const {
        return &mVal;
    }
    uint32_t setSubNodeCapacity(uint32_t) {
        // do nothing.
        return N;
    }
    uint32_t getSubNodeCount() const {
        return N;
    }
    uint32_t setSubNode(uint32_t idx, node_ptr subNode) {
        if (idx < N) {
            mSubNodes[idx] = subNode;
            return idx + 1;
        }
        return 0;
    }
};

template <typename VT>
class tree_node<VT,0> {
    template <typename NT>
    friend class tree_node_adapter;

public:
    typedef VT val_type;
    typedef tree_node * node_ptr;
    typedef const tree_node * node_ptr_c;
    typedef std::vector< node_ptr,std_allocator<val_type> > sub_node_buf;

private:
    val_type mVal;
    sub_node_buf mSubNodes;

    val_type * getValue() {
        return &mVal;
    }
    const val_type * getValue() const {
        return &mVal;
    }
    uint32_t setSubNodeCapacity(uint32_t capSize) {
        mSubNodes.reserve(capSize);
        return mSubNodes.capacity();
    }
    uint32_t getSubNodeCount() const {
        return mSubNodes.size();
    }
    uint32_t setSubNode(uint32_t idx, node_ptr subNode);
};

template <typename VT>
uint32_t tree_node<VT,0>::setSubNode(uint32_t idx, node_ptr subNode) {
    if ( idx < mSubNodes.size() ) {
        mSubNodes[idx] = subNode;
        return idx + 1;
    } if ( mSubNodes.size() < mSubNodes.capacity() ) {
        mSubNodes.push_back(subNode);
        return mSubNodes.size();
    }
    return 0;
}

template <typename NT>
class tree_node_adapter {
public:
    typedef NT node_type;
    typedef typename node_type::val_type val_type;
    typedef typename node_type::node_ptr node_ptr;
    typedef typename node_type::node_ptr_c node_ptr_c;
    typedef typename node_type::sub_node_buf sub_node_buf;

private:
    typedef obj_constructor<val_type> val_constructor;

public:
    static NT * CreateTreeNode();
    static NT * CreateTreeNode(const val_type & val);
    static void DestroyTreeNode(node_ptr node);

    static bool IsValidNode(node_ptr_c node) {
        return node && node->getValue();
    }
    static val_type * GetValue(node_ptr node) {
        return node? node->getValue(): 0;
    }
    static const val_type * GetValue(node_ptr_c node) {
        return node? node->getValue(): 0;
    }
    static uint32_t SetSubNodeCapacity(node_ptr node, uint32_t capSize) {
        return node? node->setSubNodeCapacity(capSize): 0;
    }
    static uint32_t GetSubNodeCount(node_ptr_c node) {
        return node? node->getSubNodeCount(): 0;
    }
    static uint32_t SetSubNode(node_ptr node, uint32_t idx, node_ptr subNode) {
        return (node && subNode)? node->setSubNode(idx, subNode): 0;
    }
    static node_ptr * GetSubNodeAddr(node_ptr node, uint32_t idx) {
        return ( idx < GetSubNodeCount(node) )? &(node->mSubNodes[idx]): 0;
    }
    static node_ptr_c * GetSubNodeAddr(node_ptr_c node, uint32_t idx) {
        return (node_ptr_c) GetSubNodeAddr( const_cast<node_ptr>(node), idx );
    }
    static node_ptr GetSubNode(node_ptr node, uint32_t idx) {
        node_ptr * nodeAddr = GetSubNodeAddr(node, idx);
        return nodeAddr? *nodeAddr: 0;
    }
    static node_ptr_c GetSubNode(node_ptr_c node, uint32_t idx) {
        return GetSubNode( const_cast<node_ptr>(node), idx );
    }

    static NT * Separate(node_ptr * io_nodeAddr, uint32_t nextNodeIdx);
    static void Rotate(
        node_ptr * io_nodeAddr, uint32_t nextNodeIdx, uint32_t destNodeIdx
    );
};

template <typename NT>
NT * tree_node_adapter<NT>::CreateTreeNode() {
    node_type * newNode = static_cast<node_type *>(
        val_constructor::Allocator()->Allocate( sizeof(node_type) )
    );
    return new (newNode) node_type;
}

template <typename NT>
NT * tree_node_adapter<NT>::CreateTreeNode(const val_type & val) {
    node_type * newNode = static_cast<node_type *>(
        val_constructor::Allocator()->Allocate( sizeof(node_type) )
    );
    new ( newNode->getValue() ) val_type(val);
    new ( &(newNode->mSubNodes) ) sub_node_buf;
    return newNode;
}

template <typename NT>
void tree_node_adapter<NT>::DestroyTreeNode(node_ptr node) {
    if (node) {
        node->~node_type();
        val_constructor::Allocator()->Recycle(node);
    }
}

template <typename NT>
NT * tree_node_adapter<NT>::Separate(
    node_ptr * io_nodeAddr, uint32_t nextNodeIdx)
{
    if ( io_nodeAddr && nextNodeIdx < GetSubNodeCount(*io_nodeAddr) ) {
        node_ptr origNode = *io_nodeAddr;
        *io_nodeAddr = origNode->mSubNodes[nextNodeIdx];
        origNode->mSubNodes[nextNodeIdx] = 0;
        return origNode;
    }
    return 0;
}

template <typename NT>
void tree_node_adapter<NT>::Rotate(
    node_ptr * io_nodeAddr, uint32_t nextNodeIdx, uint32_t destNodeIdx)
{
    node_ptr target = Separate(io_nodeAddr, nextNodeIdx);
    if (target) {
        if (*io_nodeAddr) {
            node_ptr destNode = *io_nodeAddr;
            node_ptr subNode;
            while (  ( subNode = GetSubNode(destNode, destNodeIdx) )  )
                destNode = subNode;
            destNode->setSubNode(destNodeIdx, target);
        } else
            *io_nodeAddr = target;
    }
}

template <typename NT, typename DT>
struct tree_stack_item {
    typedef NT tree_node;
    typedef DT item_data;
    typedef typename tree_node::node_ptr tree_node_ptr;

    item_data mData;
    tree_node_ptr mTreeNode;
    uint32_t mNextSubNodeIdx;
    uint32_t mSubNodeCount;

    void Reset(tree_node_ptr treeNode) {
        new (&mData) item_data;
        mTreeNode = treeNode;
        mNextSubNodeIdx = 0;
    }
};

template <typename NT>
struct tree_stack_item<NT,void> {
    typedef NT tree_node;
    typedef typename tree_node::node_ptr tree_node_ptr;

    tree_node_ptr mTreeNode;
    uint32_t mNextSubNodeIdx;
    uint32_t mSubNodeCount;

    void Reset(tree_node_ptr treeNode) {
        mTreeNode = treeNode;
        mNextSubNodeIdx = 0;
    }
};

template <typename DT, typename VT = value_obj, uint32_t MAX_SUBNODE_COUNT = 0>
struct tree_traits {
    typedef tree_node<VT,MAX_SUBNODE_COUNT> node_type;
    typedef typename node_type::val_type val_type;
    typedef typename node_type::node_ptr node_ptr;
    typedef typename node_type::node_ptr_c node_ptr_c;
    typedef tree_node_adapter<node_type> node_adapter;
    typedef tree_stack_item<node_type,DT> stack_item;
    typedef std::vector< stack_item,std_allocator<val_type> > stack_buf;
};

template <typename T, uint32_t BUFFER_EXTEND_SIZE = 16>
class tree: obj_base {
public:
    typedef T traits_type;
    typedef typename traits_type::val_type val_type;
    typedef typename traits_type::node_type node_type;
    typedef typename traits_type::node_ptr node_ptr;
    typedef typename traits_type::node_ptr_c node_ptr_c;
    typedef typename traits_type::node_adapter node_adapter;
    typedef typename traits_type::stack_item stack_item;

    struct for_each_callback {
        virtual void onPushStack(stack_item * io_stackTop) = 0;
        virtual void afterPopStack(stack_item * io_stackTop) = 0;
        // For 'order' parameter, the value 0 means pre-order tree-traversal,
        // and the value 1 means in-order tree-traversal which access the root-
        // node after the 1st. sub-node, and the value 2 means in-order tree-
        // traversal which access the root-node after the 2nd. sub-node, and
        // so on, if the value is negative or larger than the count of sub-
        // nodes, then it means post-order tree-traversal.
        // Return value:
        //   < 0 - break from ForEach() function;
        //   > 0 - ignore sub-nodes in pre-order mode or re-enter sub-nodes in
        //         post-order mode.
        virtual int onTraversal(stack_item * io_stackTop, int order) = 0;
    };

    virtual const char * ContextName() const {
        return "tree";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(tree);
    }

protected:
    typedef typename traits_type::stack_buf stack_buf;

    struct node_deleter: for_each_callback {
        virtual void onPushStack(stack_item *) {
            // Do nothing.
        }
        virtual void afterPopStack(stack_item *) {
            // Do nothing.
        }
        virtual int onTraversal(stack_item * io_stackTop, int order) {
            node_adapter::DestroyTreeNode(io_stackTop->mTreeNode);
            return 0;
        }
    };

    node_ptr mRootNode;
    stack_buf mStackBuf;

    void pushStackItem(const stack_item * stackItem);
    void popStackItem(stack_item * out_stackItem);

public:
    explicit tree(node_ptr rootNode = 0) {
        mRootNode = rootNode;
    }
    explicit tree(node_ptr_c rootNode) {
        mRootNode = const_cast<node_ptr>(rootNode);
    }
    ~tree() {
        Clear();
    }

    node_ptr * GetRootNodeAddr() {
        return &mRootNode;
    }
    node_ptr_c * GetRootNodeAddr() const {
        return &mRootNode;
    }
    node_ptr GetRootNode() {
        return mRootNode;
    }
    node_ptr_c GetRootNode() const {
        return mRootNode;
    }

    // For 'order' parameter, the value 0 means pre-order tree-traversal,
    // and the value 1 means in-order tree-traversal which access the root-
    // node after the 1st. sub-node, and the value 2 means in-order tree-
    // traversal which access the root-node after the 2nd. sub-node, and
    // so on, if the value is negative or larger than the count of all sub-
    // nodes, then it means post-order tree-traversal.
    int ForEach(for_each_callback * cb, int order = 0);

    void Clear() {
        if (mRootNode) {
            node_deleter nodeDeleter;
            ForEach(&nodeDeleter, -1);
            mRootNode = 0;
        }
    }
};

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void tree<T,BUFFER_EXTEND_SIZE>::pushStackItem(const stack_item * stackItem) {
    if ( mStackBuf.size() == mStackBuf.capacity() )
        mStackBuf.reserve( mStackBuf.size() + BUFFER_EXTEND_SIZE );
    mStackBuf.push_back(*stackItem);
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
void tree<T,BUFFER_EXTEND_SIZE>::popStackItem(stack_item * out_stackItem) {
    if ( mStackBuf.size() ) {
        *out_stackItem = mStackBuf.back();
        mStackBuf.pop_back();
    } else
        out_stackItem->mTreeNode = 0;
}

template <typename T, uint32_t BUFFER_EXTEND_SIZE>
int tree<T,BUFFER_EXTEND_SIZE>::ForEach(for_each_callback * cb, int order) {
    int cbResult = 0;
    if (cb && mRootNode) {
        stack_item stackTop;
        node_ptr subNode;
        uint32_t i;
        stackTop.Reset(mRootNode);
        while (stackTop.mTreeNode) {
            stackTop.mSubNodeCount = \
                node_adapter::GetSubNodeCount(stackTop.mTreeNode);
            for (i = stackTop.mNextSubNodeIdx; i < stackTop.mSubNodeCount; ++i)
            {
                if (i == order) {
                    cbResult = cb->onTraversal(&stackTop, order);
                    if (cbResult < 0)
                        break;
                    if (cbResult > 0) {
                        // Ignore sub-nodes in pre-order mode.
                        i = stackTop.mSubNodeCount;
                        cbResult = 0;
                        break;
                    }
                }
                subNode = node_adapter::GetSubNode(stackTop.mTreeNode, i);
                if (subNode) {
                    stackTop.mNextSubNodeIdx = i + 1;
                    cb->onPushStack(&stackTop);
                    pushStackItem(&stackTop);
                    stackTop.Reset(subNode);
                    break;
                }
            }
            if (cbResult < 0)
                break;
            if (i >= stackTop.mSubNodeCount) {
                if (order < 0 || order >= stackTop.mSubNodeCount) {
                    cbResult = cb->onTraversal(&stackTop, order);
                    if (cbResult < 0)
                        break;
                }
                popStackItem(&stackTop);
                if (stackTop.mTreeNode)
                    cb->afterPopStack(&stackTop);
                if (cbResult > 0) {
                    // Re-enter sub-nodes in post-order mode.
                    stackTop.mNextSubNodeIdx = 0;
                    cbResult = 0;
                }
            }
        }
        if ( mStackBuf.size() )
            mStackBuf.resize(0);
    }
    return cbResult;
}

} // namespace pdl

#endif // _TREE_H_

