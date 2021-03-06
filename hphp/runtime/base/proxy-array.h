/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2014 Facebook, Inc. (http://www.facebook.com)     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/
#ifndef incl_HPHP_PROXY_ARRAY_H
#define incl_HPHP_PROXY_ARRAY_H

#include "hphp/runtime/vm/name-value-table.h"
#include "hphp/runtime/base/array-data.h"

namespace HPHP {

//////////////////////////////////////////////////////////////////////

/*
 * A proxy for an underlying ArrayData. The Zend compatibility layer needs
 * this since functions like zend_hash_update only take a pointer to the
 * ArrayData and don't expect it to change location.
 *
 * Other functionality specific to the Zend compatibility layer is also
 * implemented here, such as the need to store arbitrary non-zval data. This
 * feature is implemented by wrapping the arbitrary data block in a
 * ResourceData.
 *
 * TODO: rename to ZendArray
 */
struct ProxyArray : public ArrayData {
  explicit ProxyArray(ArrayData* ad)
    : ArrayData(kProxyKind)
    , m_ad(ad)
  {}

  static ProxyArray* Make(ArrayData*);

public:
  //////////////////////////////////////////////////////////////////////
  // Non-static interface for zend_hash.cpp

  typedef void (*DtorFunc)(void *pDest);

  /**
   * Initialize a ProxyArray using the parameters provided to a
   * zend_hash_init() call.
   */
  void proxyInit(uint32_t nSize, DtorFunc pDestructor, bool persistent);

  /**
   * Get a pointer to the data for an array element identified by a given
   * string key as a void*. If the array holds zvals, this will be a zval**,
   * i.e. RefData**. If it holds arbitrary data, a pointer to that data will
   * be returned.
   */
  void * proxyGet(StringData* k) const;

  /**
   * Get a pointer to the data for an array element identified by a given
   * integer key as a void*. If the array holds zvals, this will be a zval**,
   * i.e. RefData**. If it holds arbitrary data, a pointer to that data will
   * be returned.
   */
  void * proxyGet(int64_t k) const;

  /**
   * Get a pointer to the data for an array element identified by its position,
   * as returned by iter_begin() etc. This is used to implement the
   * HashPosition interface.
   */
  void * proxyGetValueRef(ssize_t pos) const;

  /**
   * Set an element by StringData* or integer key, and return the new data
   * location in the dest parameter.
   */
  template<class K>
  void proxySet(K k, void* data, uint32_t data_size, void** dest);

  /**
   * Append an element, and return the new data location in the dest parameter.
   */
  void proxyAppend(void* data, uint32_t data_size, void** dest);

private:
  /**
   * Returns true if the array contains zvals. The caller conventionally
   * indicates this to us by setting the destructor to ZVAL_PTR_DTOR in the
   * zend_hash_init() call.
   */
  bool hasZvalValues() const {
    return m_destructor == ZvalPtrDtor;
  }

  /**
   * Convert a TypedValue retrieved from the array to the void* expected by the
   * Zend compat caller. This will retrieve the underlying data pointer from
   * the ZendCustomElement resource, if applicable.
   */
  void * elementToData(TypedValue * tv) const;

  /**
   * Make a ZendCustomElement resource wrapping the given data block. If pDest
   * is non-null, it will be set to the newly-allocated location for the block.
   */
  ResourceData * makeElementResource(void *pData, uint nDataSize,
                                     void **pDest) const;

  DtorFunc m_destructor;

  static DtorFunc ZvalPtrDtor;

public:
  //////////////////////////////////////////////////////////////////////
  // ArrayData implementation
  static void Release(ArrayData*);

  static size_t Vsize(const ArrayData*);
  static void NvGetKey(const ArrayData* ad, TypedValue* out, ssize_t pos);
  static const Variant& GetValueRef(const ArrayData*, ssize_t pos);

  static bool ExistsInt(const ArrayData* ad, int64_t k);
  static bool ExistsStr(const ArrayData* ad, const StringData* k);

  static const TypedValue* NvGetInt(const ArrayData*, int64_t k);
  static const TypedValue* NvGetStr(const ArrayData*, const StringData* k);

  static ArrayData* LvalInt(ArrayData*, int64_t k, Variant*& ret, bool copy);
  static ArrayData* LvalStr(ArrayData*, StringData* k, Variant*& ret,
                            bool copy);
  static ArrayData* LvalNew(ArrayData*, Variant*& ret, bool copy);

  static ArrayData* SetInt(ArrayData*, int64_t k, Cell v, bool copy);
  static ArrayData* SetStr(ArrayData*, StringData* k, Cell v, bool copy);
  static ArrayData* SetRefInt(ArrayData*, int64_t k, Variant& v, bool copy);
  static ArrayData* SetRefStr(ArrayData*, StringData* k, Variant& v, bool copy);
  static ArrayData* RemoveInt(ArrayData*, int64_t k, bool copy);
  static ArrayData* RemoveStr(ArrayData*, const StringData* k, bool copy);
  static constexpr auto AddInt = &SetInt;
  static constexpr auto AddStr = &SetStr;

  static ArrayData* Copy(const ArrayData* ad);

  static ArrayData* Append(ArrayData*, const Variant& v, bool copy);
  static ArrayData* AppendRef(ArrayData*, Variant& v, bool copy);
  static ArrayData* AppendWithRef(ArrayData*, const Variant& v, bool copy);

  static ArrayData* PlusEq(ArrayData*, const ArrayData* elems);
  static ArrayData* Merge(ArrayData*, const ArrayData* elems);
  static ArrayData* Pop(ArrayData*, Variant &value);
  static ArrayData* Dequeue(ArrayData*, Variant &value);
  static ArrayData* Prepend(ArrayData*, const Variant& v, bool copy);
  static void Renumber(ArrayData*);
  static void OnSetEvalScalar(ArrayData*);
  static ArrayData* Escalate(const ArrayData* ad);

  static ssize_t IterBegin(const ArrayData*);
  static ssize_t IterEnd(const ArrayData*);
  static ssize_t IterAdvance(const ArrayData*, ssize_t prev);
  static ssize_t IterRewind(const ArrayData*, ssize_t prev);

  static bool ValidMArrayIter(const ArrayData*, const MArrayIter & fp);
  static bool AdvanceMArrayIter(ArrayData*, MArrayIter&);
  static bool IsVectorData(const ArrayData*);

  static ArrayData* EscalateForSort(ArrayData*);
  static void Ksort(ArrayData*, int sort_flags, bool ascending);
  static void Sort(ArrayData*, int sort_flags, bool ascending);
  static void Asort(ArrayData*, int sort_flags, bool ascending);
  static bool Uksort(ArrayData*, const Variant& cmp_function);
  static bool Usort(ArrayData*, const Variant& cmp_function);
  static bool Uasort(ArrayData*, const Variant& cmp_function);

  static ArrayData* ZSetInt(ArrayData* ad, int64_t k, RefData* v);
  static ArrayData* ZSetStr(ArrayData* ad, StringData* k, RefData* v);
  static ArrayData* ZAppend(ArrayData* ad, RefData* v, int64_t* key_ptr);

  static ArrayData* CopyWithStrongIterators(const ArrayData*);
  static ArrayData* NonSmartCopy(const ArrayData*);

private:
  static ProxyArray* asProxyArray(ArrayData* ad);
  static const ProxyArray* asProxyArray(const ArrayData* ad);
  static ProxyArray* reseatable(ArrayData* oldArr, ArrayData* newArr);
  static ArrayData* innerArr(ArrayData* ad);
  static ArrayData* innerArr(const ArrayData* ad);

private:
  ArrayData* m_ad;
};

//////////////////////////////////////////////////////////////////////

template<class K>
void ProxyArray::proxySet(K k,
    void* data, uint32_t data_size, void** dest) {
  ArrayData * r;
  if (hasZvalValues()) {
    assert(data_size == sizeof(void*));
    r = m_ad->zSet(k, *(RefData**)data);
    if (dest) {
      *dest = (void*)(&m_ad->nvGet(k)->m_data.pref);
    }
  } else {
    ResourceData * elt = makeElementResource(data, data_size, dest);
    r = m_ad->set(k, elt, false);
  }
  reseatable(this, r);
}

//////////////////////////////////////////////////////////////////////

}

#endif
