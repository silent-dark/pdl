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
class tree_node_base {
public:
    typedef VT val_type;

protected:
    typedef tree_node_base * sub_node_buf[N];

    val_type mVal;
    sub_node_buf mSubNodes;

    tree_node_base() {
        memset( mSubNodes, 0, sizeof(sub_node_buf) );
    }

public:
    uint32_t SetSubNodeCapacity(uint32_t) {
        // do nothing.
        return N;
    }
    uint32_t GetSubNodeCount() const {
        return N;
    }
    uint32_t SetSubNode(uint32_t idx, tree_node_base * subNode) {
        if (idx < N) {
            mSubNodes[idx] = subNode;
            return idx + 1;
        }
        return 0;
    }
};
template <typename VT>
class tree_node_base<VT,0> {
public:
    typedef VT val_type;

protected:
    typedef std_allocator<tree_node_base *,val_type> node_ptr_allocator;
    typedef std::vector<tree_node_base *,node_ptr_allocator> sub_node_buf;

    val_type mVal;
    sub_node_buf mSubNodes;

public:
    uint32_t SetSubNodeCapacity(uint32_t capSize) {
        if (capSize)
            mSubNodes.reserve(capSize);
        return mSubNodes.capacity();
    }
    uint32_t GetSubNodeCount() const {
        return mSubNodes.size();
    }
    uint32_t SetSubNode(uint32_t idx, tree_node_base * subNode);
};

template <typename VT>
uint32_t tree_node_base<VT,0>::SetSubNode(
    uint32_t idx, tree_node_base * subNode)
{
    if ( idx < mSubNodes.size() ) {
        mSubNodes[idx] = subNode;
        return idx + 1;
    } if ( mSubNodes.size() < mSubNodes.capacity() ) {
        mSubNodes.push_back(subNode);
        return mSubNodes.size();
    }
    return 0;
}

template <typename VT, uint32_t N = 0>
class tree_node: public tree_node_base<VT,N> {
public:
    typedef tree_node_base<VT,N> my_base;
    typedef typename my_base::val_type val_type;
    using my_base::SetSubNodeCapacity;
    using my_base::GetSubNodeCount;
    using my_base::SetSubNode;

private:
    using my_base::mVal;
    using my_base::mSubNodes;

public:
    tree_node() {}
    tree_node(const val_type & val) {
        mVal = val;
    }

    val_type & GetValue() {
        return mVal;
    }
    const val_type & GetValue() const {
        return mVal;
    }
    tree_node ** GetSubNodeAddr(uint32_t idx) {
        if ( idx < GetSubNodeCount() )
            return reinterpret_cast<tree_node **>( &(mSubNodes[idx]) );
        return 0;
    }
    tree_node * const * GetSubNodeAddr(uint32_t idx) const {
        if ( idx < GetSubNodeCount() )
            return reinterpret_cast<tree_node * const *>( &(mSubNodes[idx]) );
        return 0;
    }
    tree_node * GetSubNode(uint32_t idx) {
        if ( idx < GetSubNodeCount() )
            return static_cast<tree_node *>(mSubNodes[idx]);
        return 0;
    }
    const tree_node * GetSubNode(uint32_t idx) const {
        if ( idx < GetSubNodeCount() )
            return static_cast<const tree_node *>(mSubNodes[idx]);
        return 0;
    }
    static tree_node * Separate(tree_node ** io_target, uint32_t nextNodeIdx) {
        tree_node * target = *io_target; // de-ref.
        if (target) {
            *io_target = target->GetSubNode(nextNodeIdx); // cover ref-val.
            target->SetSubNode(nextNodeIdx, 0);
        }
        return target;
    }
    static void Rotate(
        tree_node ** io_target, uint32_t nextNodeIdx, uint32_t destNodeIdx
    );
};

template <typename VT, uint32_t N>
void tree_node<VT,N>::Rotate(
    tree_node ** io_target, uint32_t nextNodeIdx, uint32_t destNodeIdx)
{
    tree_node * target = Separate(io_target, nextNodeIdx);
    tree_node * nextNode = *io_target; // de-ref.
    while (nextNode) {
        io_target = nextNode->GetSubNodeAddr(destNodeIdx); // ref to next.
        nextNode = *io_target; // de-ref.
    }
    *io_target = target; // cover ref-val.
}

template <typename NT, typename DT>
struct tree_stack_item {
    typedef NT node_type;
    typedef DT item_data;
    typedef node_type * node_ptr;

    item_data mData;
    node_ptr mTreeNode;
    uint32_t mNextSubNodeIdx;
    uint32_t mSubNodeCount;

    void Reset(node_ptr treeNode) {
        new (&mData) item_data;
        mTreeNode = treeNode;
        mNextSubNodeIdx = 0;
    }
};

template <typename NT>
struct tree_stack_item<NT,void> {
    typedef NT node_type;
    typedef node_type * node_ptr;

    node_ptr mTreeNode;
    uint32_t mNextSubNodeIdx;
    uint32_t mSubNodeCount;

    void Reset(node_ptr treeNode) {
        mTreeNode = treeNode;
        mNextSubNodeIdx = 0;
    }
};

template <typename DT, typename VT = value_obj, uint32_t MAX_SUBNODE_COUNT = 0>
struct tree_traits {
    typedef VT val_type;
    typedef tree_node<VT,MAX_SUBNODE_COUNT> node_type;
    typedef node_type * node_ptr;
    typedef const node_type * node_ptr_c;
    typedef obj_constructor<node_type> node_constructor;
    typedef tree_stack_item<node_type,DT> stack_item;
    typedef std_allocator<stack_item,val_type> stack_item_allocator;
    typedef std::vector<stack_item,stack_item_allocator> stack_buf;
};

template <typename T, uint32_t BUFFER_EXTEND_SIZE = 16>
class tree: public obj_base {
public:
    typedef T traits_type;
    typedef typename traits_type::val_type val_type;
    typedef typename traits_type::node_type node_type;
    typedef typename traits_type::node_ptr node_ptr;
    typedef typename traits_type::node_ptr_c node_ptr_c;
    typedef typename traits_type::node_constructor node_constructor;
    typedef typename traits_type::stack_item stack_item;
    typedef typename traits_type::stack_buf stack_buf;

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

    static node_ptr CreateNode() {
        node_ptr newNode = static_cast<node_ptr>(
            node_constructor::Allocator()->Allocate( sizeof(node_type) )
        );
        new (newNode) node_type();
        return newNode;
    }
    static node_ptr CreateNode(const val_type & val) {
        node_ptr newNode = static_cast<node_ptr>(
            node_constructor::Allocator()->Allocate( sizeof(node_type) )
        );
        new (newNode) node_type(val);
        return newNode;
    }
    static void DestroyNode(node_ptr node) {
        if (node) {
            node->~node_type();
            node_constructor::Allocator()->Recycle(node);
        }
    }

protected:
    struct node_deleter: for_each_callback {
        virtual void onPushStack(stack_item *) {
            // Do nothing.
        }
        virtual void afterPopStack(stack_item *) {
            // Do nothing.
        }
        virtual int onTraversal(stack_item * io_stackTop, int order) {
            DestroyNode(io_stackTop->mTreeNode);
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
            stackTop.mSubNodeCount = stackTop.mTreeNode->GetSubNodeCount();
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
                subNode = stackTop.mTreeNode->GetSubNode(i);
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

