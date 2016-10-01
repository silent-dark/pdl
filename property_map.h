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

#ifndef _PROPERTY_MAP_H_
#define _PROPERTY_MAP_H_

#ifndef __cplusplus
#error The module is NOT compatible with C codes.
#endif

#include <vector>
#include "tree.h"

namespace pdl {

template <
    typename VT = obj_ptr<value_obj>,
    typename KT = char,
    uint32_t BUFFER_EXTEND_SIZE = 16>
class property_map: public obj_base {
public:
    typedef KT key_type;
    typedef VT val_type;

    struct for_each_callback {
        // refer tree::for_each_callback::onTraversal for more informations
        // about 'order' parameter and return value.
        virtual int Callback(
            const key_type * key, val_type & val, int order
        ) = 0;
    };

    virtual const char * ContextName() const {
        return "property_map";
    }
    virtual uint32_t ContextSize() const {
        return sizeof(property_map);
    }

private:
    struct key_val_pair {
        key_type mKeyItem;
        val_type mVal;

        key_val_pair() {}
        key_val_pair(const key_val_pair & src)
            : mKeyItem(src.mKeyItem)
            , mVal(src.mKeyVal)
        {}
        key_val_pair & operator =(const key_val_pair & src) {
            mKeyItem = src.mKeyItem;
            mVal = src.mVal;
        }
    };
    typedef tree_traits<void,key_val_pair,3> map_traits;
    typedef tree<map_traits,BUFFER_EXTEND_SIZE> map_tree;
    typedef typename map_tree::node_type node_type;
    typedef typename map_tree::node_ptr node_ptr;
    typedef typename map_tree::stack_item stack_item;
    typedef typename map_tree::for_each_callback for_each_node_callback;
    typedef std::vector< key_type,std_allocator<key_type> > key_buf;

    class for_each_callback_invoker: public for_each_node_callback {
        property_map * mMap;
        for_each_callback * mCb;

    public:
        for_each_callback_invoker(property_map * map, for_each_callback * cb) {
            mMap = map;
            mCb = cb;
        }

        virtual void onPushStack(stack_item * io_stackTop) {
            if (1 == io_stackTop->mNextSubNodeIdx)
                mMap->pushKeyItem(io_stackTop->mTreeNode->GetValue().mKeyItem);
        }
        virtual void afterPopStack(stack_item * io_stackTop) {
            if (1 == io_stackTop->mNextSubNodeIdx)
                mMap->popKeyItem();
        }
        virtual int onTraversal(stack_item * io_stackTop, int order) {
            return mMap->invokeCallback(
                mCb, &( io_stackTop->mTreeNode->GetValue() ), order
            );
        }
    };
    friend class property_map::for_each_callback_invoker;

    map_tree mMapTree;
    key_buf mKeyBuf;
    uint32_t mValCount;

    void pushKeyItem(key_type keyItem);
    void popKeyItem();
    int invokeCallback(
        for_each_callback * cb, key_val_pair * kvPair, int order
    );

    static void * findNodeWithKeyItem(
        node_ptr * rootMapNode, key_type keyItem, bool autoInsert
    );
    static void * findNodeWithKey(
        node_ptr * rootMapNode, const key_type * key, bool autoInsert
    );
    node_ptr * findNode(const key_type * key, bool autoInsert) {
        return key? static_cast<node_ptr *>(
            findNodeWithKey( mMapTree.GetRootNodeAddr(), key, autoInsert )
        ): 0;
    }

public:
    void Clear() {
        mMapTree.Clear();
        mValCount = 0;
    }
    uint32_t ValCount() const {
        return mValCount;
    }
    val_type GetValue(const key_type * key) const {
        node_ptr * mapNode = \
            const_cast<property_map *>(this)->findNode(key, false);
        return (mapNode && *mapNode)? (*mapNode)->GetValue().mVal: val_type();
    }
    void Remove(const key_type * key) {
        node_ptr * mapNode = findNode(key, false);
        if (mapNode && *mapNode) {
            (*mapNode)->GetValue().mVal = val_type();
            --mValCount;
        }
    }
    val_type & operator [](const key_type * key) {
        ++mValCount;
        return (* findNode(key, true) )->GetValue().mVal;
    }
    // refer tree::ForEach for more informations about 'order' parameter.
    int ForEach(for_each_callback * cb, int order = 0) {
        for_each_callback_invoker cbInvoker(this, cb);
        return mMapTree.ForEach(&cbInvoker, order);
    }
};

template <typename VT, typename KT, uint32_t BUFFER_EXTEND_SIZE>
void property_map<VT,KT,BUFFER_EXTEND_SIZE>::pushKeyItem(key_type keyItem) {
    if ( mKeyBuf.size() == mKeyBuf.capacity() )
        mKeyBuf.reserve(mKeyBuf.size() + BUFFER_EXTEND_SIZE);
    if ( mKeyBuf.size() )
        mKeyBuf.back() = keyItem;
    else
        mKeyBuf.push_back(keyItem);
    mKeyBuf.push_back(0);
}

template <typename VT, typename KT, uint32_t BUFFER_EXTEND_SIZE>
void property_map<VT,KT,BUFFER_EXTEND_SIZE>::popKeyItem() {
    if ( mKeyBuf.size() > 1 ) {
        mKeyBuf.pop_back();
        mKeyBuf.back() = 0;
    }
};

template <typename VT>
inline bool is_valid(const VT & val) {
    return static_cast<bool>(val);
}
inline bool is_valid(const std::string & str) {
    return !str.empty();
}

template <typename VT, typename KT, uint32_t BUFFER_EXTEND_SIZE>
int property_map<VT,KT,BUFFER_EXTEND_SIZE>::invokeCallback(
    for_each_callback * cb, key_val_pair * kvPair, int order)
{
    int cbResult = 0;
    if ( is_valid(kvPair->mVal) ) {
        pushKeyItem(kvPair->mKeyItem);
        cbResult = cb->Callback( &(mKeyBuf[0]), kvPair->mVal, order );
        if (cbResult < 0)
            mKeyBuf.resize(0);
        else
            popKeyItem();
    }
    return cbResult;
}

template <typename VT, typename KT, uint32_t BUFFER_EXTEND_SIZE>
void * property_map<VT,KT,BUFFER_EXTEND_SIZE>::findNodeWithKeyItem(
    node_ptr * rootMapNode, key_type keyItem, bool autoInsert)
{
    node_ptr * curMapNode = rootMapNode;
    node_ptr * prevMapNode = 0;
    uint32_t rotationIdx;
    while (curMapNode) {
        if (! *curMapNode) {
            if (autoInsert) {
                *curMapNode = map_tree::CreateNode();
                (*curMapNode)->GetValue().mKeyItem = keyItem;
            }
            break;
        }
        if ( keyItem == (*curMapNode)->GetValue().mKeyItem )
            break;
        prevMapNode = curMapNode;
        if ( keyItem < (*curMapNode)->GetValue().mKeyItem ) {
            curMapNode = (*curMapNode)->GetSubNodeAddr(1);
            rotationIdx = 2;
        } else {
            curMapNode = (*curMapNode)->GetSubNodeAddr(2);
            rotationIdx = 1;
        }
    }
    if (prevMapNode && *curMapNode) {
        node_type::Rotate(prevMapNode, 3 - rotationIdx, rotationIdx);
        return prevMapNode;
    }
    return curMapNode;
}

template <typename VT, typename KT, uint32_t BUFFER_EXTEND_SIZE>
void * property_map<VT,KT,BUFFER_EXTEND_SIZE>::findNodeWithKey(
    node_ptr * rootMapNode, const key_type * key, bool autoInsert)
{
    node_ptr * curMapNode = 0;
    node_ptr * nextMapNode = rootMapNode;
    uint32_t i = 0;
    while (key[i]) {
        curMapNode = nextMapNode;
        if (*curMapNode) {
            curMapNode = static_cast<node_ptr *>(
                findNodeWithKeyItem(curMapNode, key[i], autoInsert)
            );
        } else {
            if (!autoInsert)
                break;
            *curMapNode = map_tree::CreateNode();
            (*curMapNode)->GetValue().mKeyItem = key[i];
        }
        if (! *curMapNode)
            break;
        nextMapNode = (*curMapNode)->GetSubNodeAddr(0);
        ++i;
    }
    return curMapNode;
}

} // namespace pdl

#endif // _PROPERTY_MAP_H_

