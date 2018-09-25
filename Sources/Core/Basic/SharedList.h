/**
 * @file Core/Basic/SharedList.h
 * Contains the header of class Core::Basic::SharedList.
 *
 * @copyright Copyright (C) 2018 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

#ifndef CORE_BASIC_SHAREDLIST_H
#define CORE_BASIC_SHAREDLIST_H

namespace Core::Basic
{

template<class CTYPE, class PTYPE=TiObject> class SharedList : public PTYPE, public virtual ListContaining<CTYPE>
{
  //============================================================================
  // Type Info

  TEMPLATE_TYPE_INFO(SharedList, PTYPE, "Core.Basic", "Core", "alusus.net", (CTYPE, PTYPE), (
    INHERITANCE_INTERFACES(ListContaining<CTYPE>)
  ));


  //============================================================================
  // Member Variables

  protected: SharedList<CTYPE, PTYPE> *base;

  /// The vector in which the object pointers will be stored.
  private: std::vector<SharedPtr<CTYPE>> list;

  private: std::vector<Bool> *inherited;


  //============================================================================
  // Signals & Slots

  public: Signal<void, SharedList<CTYPE, PTYPE>*> destroyNotifier;
  public: Signal<void, SharedList<CTYPE, PTYPE>*, ContentChangeOp, Int> changeNotifier;

  private: Slot<void, SharedList<CTYPE, PTYPE>*> baseDestroySlot = {
    this, &SharedList<CTYPE, PTYPE>::onBaseDestroyed
  };
  private: Slot<void, SharedList<CTYPE, PTYPE>*, ContentChangeOp, Int> baseChangeSlot = {
    this, &SharedList<CTYPE, PTYPE>::onBaseContentChanged
  };


  //============================================================================
  // Constructors

  public: SharedList() : inherited(0), base(0)
  {
  }

  public: SharedList(const std::initializer_list<SharedPtr<CTYPE>> &args) : inherited(0), base(0)
  {
    this->reserve(args.size());
    for (auto arg : args) this->add(arg);
  }

  public: virtual ~SharedList()
  {
    this->destroyNotifier.emit(this);
    if (this->base != 0) this->detachFromBase();
  }

  public: static SharedPtr<SharedList> create(const std::initializer_list<SharedPtr<CTYPE>> &args)
  {
    return std::make_shared<SharedList>(args);
  }


  //============================================================================
  // Member Functions

  /// @name Inheritance Functions
  /// @{

  public: void setBase(SharedList<CTYPE, PTYPE> *b)
  {
    if (this->base != 0) this->detachFromBase();
    if (b != 0) this->attachToBase(b);
  }

  public: SharedList<CTYPE, PTYPE>* getBase() const
  {
    return this->base;
  }

  private: Word getBaseDefCount() const
  {
    return this->base != 0 ? this->base->getCount() : 0;
  }

  private: void attachToBase(SharedList<CTYPE, PTYPE> *b)
  {
    ASSERT(this->base == 0);
    ASSERT(this->inherited == 0);
    this->base = b;
    this->base->changeNotifier.connect(this->baseChangeSlot);
    this->base->destroyNotifier.connect(this->baseDestroySlot);
    this->inherited = new std::vector<Bool>(this->list.size(), false);
    this->inheritFromBase();
  }

  private: void detachFromBase()
  {
    ASSERT(this->base != 0);
    ASSERT(this->inherited != 0);
    this->removeInheritted();
    this->base->changeNotifier.disconnect(this->baseChangeSlot);
    this->base->destroyNotifier.disconnect(this->baseDestroySlot);
    this->base = 0;
    delete this->inherited;
    this->inherited = 0;
  }

  private: void inheritFromBase()
  {
    ASSERT(this->base != 0);
    for (Int i = 0; static_cast<Word>(i) < this->getBaseDefCount(); ++i) this->onAdded(i);
  }

  private: void removeInheritted()
  {
    ASSERT(this->inherited != 0);
    for (Int i = this->inherited->size()-1; i >= 0; --i) {
      if (this->inherited->at(i)) {
        this->onRemoved(i);
      }
    }
  }

  private: SharedPtr<CTYPE> getFromBase(Int index)
  {
    return this->base->get(index);
  }

  private: void onAdded(Int index)
  {
    ASSERT(this->base != 0);
    ASSERT(this->inherited != 0);
    ASSERT(static_cast<Word>(index) < this->getBaseDefCount());
    this->list.insert(this->list.begin()+index, this->getFromBase(index));
    this->inherited->insert(this->inherited->begin()+index, true);
    this->changeNotifier.emit(this, ContentChangeOp::ADDED, index);
  }

  private: void onUpdated(Int index)
  {
    ASSERT(this->base != 0);
    ASSERT(this->inherited != 0);
    ASSERT(static_cast<Word>(index) < this->getBaseDefCount());
    if (this->inherited->at(index)) {
      this->list[index] = this->getFromBase(index);
      this->changeNotifier.emit(this, ContentChangeOp::UPDATED, index);
    }
  }

  private: void onRemoved(Int index)
  {
    ASSERT(this->base != 0);
    ASSERT(this->inherited != 0);
    ASSERT(static_cast<Word>(index) < this->getBaseDefCount()+1);
    if (this->inherited->at(index)) {
      this->list.erase(this->list.begin()+index);
      this->inherited->erase(this->inherited->begin()+index);
      this->changeNotifier.emit(this, ContentChangeOp::REMOVED, index);
    } else {
      SharedPtr<CTYPE> obj = this->get(index);
      this->list.erase(this->list.begin()+index);
      this->inherited->erase(this->inherited->begin()+index);
      this->changeNotifier.emit(this, ContentChangeOp::REMOVED, index);
      this->insert(this->getBaseDefCount(), obj);
    }
  }

  private: void onBaseContentChanged(SharedList<CTYPE, PTYPE> *obj, ContentChangeOp op, Int index)
  {
    if (op == ContentChangeOp::ADDED) this->onAdded(index);
    else if (op == ContentChangeOp::UPDATED) this->onUpdated(index);
    else if (op == ContentChangeOp::REMOVED) this->onRemoved(index);
  }

  private: void onBaseDestroyed(SharedList<CTYPE, PTYPE> *obj)
  {
    this->detachFromBase();
  }

  /// @}

  /// @name Data Access Functions
  /// @{

  /**
   * @brief Adds a list of new objects to the list.
   * This is more efficient than individually calling add() on each item
   * because it preallocates any needed memory in advance.
   */
  public: void add(const std::initializer_list<SharedPtr<CTYPE>> &objs)
  {
    if (this->list.capacity() < this->list.size() + objs.size()) this->list.reserve(this->list.size() + objs.size());
    for (auto obj : objs) {
      this->list.push_back(obj);
      if (this->inherited != 0) this->inherited->push_back(false);
      this->changeNotifier.emit(this, ContentChangeOp::ADDED, this->list.size() - 1);
    }
  }

  public: Int add(SharedPtr<CTYPE> const &val)
  {
    this->list.push_back(val);
    if (this->inherited != 0) this->inherited->push_back(false);
    this->changeNotifier.emit(this, ContentChangeOp::ADDED, this->list.size() - 1);
    return this->list.size() - 1;
  }

  public: void insert(Int index, SharedPtr<CTYPE> const &val)
  {
    if (static_cast<Word>(index) > this->list.size()) {
      throw EXCEPTION(InvalidArgumentException, S("index"), S("Out of range"), index);
    }
    this->list.insert(this->list.begin()+index, val);
    if (this->inherited != 0) this->inherited->insert(this->inherited->begin()+index, false);
    this->changeNotifier.emit(this, ContentChangeOp::ADDED, index);
  }

  public: void set(Int index, SharedPtr<CTYPE> const &val)
  {
    if (static_cast<Word>(index) >= this->list.size()) {
      throw EXCEPTION(InvalidArgumentException, S("index"), S("Index out of range."), index);
    }
    this->changeNotifier.emit(this, ContentChangeOp::WILL_UPDATE, index);
    this->list[index] = val;
    if (this->inherited != 0) this->inherited->at(index) = false;
    this->changeNotifier.emit(this, ContentChangeOp::UPDATED, index);
  }

  public: void remove(Int index)
  {
    if (static_cast<Word>(index) >= this->list.size()) {
      throw EXCEPTION(InvalidArgumentException, S("index"), S("Index out of range."), index);
    }
    if (this->inherited != 0 && this->inherited->at(index)) {
      throw EXCEPTION(InvalidArgumentException, S("index"), S("Given entry belongs to base."), index);
    }
    if (static_cast<Word>(index) < this->getBaseDefCount()) {
      ASSERT(this->base != 0);
      ASSERT(this->inherited != 0);
      this->changeNotifier.emit(this, ContentChangeOp::WILL_UPDATE, index);
      this->list[index] = this->getFromBase(index);
      this->inherited->at(index) = true;
      this->changeNotifier.emit(this, ContentChangeOp::UPDATED, index);
    } else {
      this->changeNotifier.emit(this, ContentChangeOp::WILL_REMOVE, index);
      this->list.erase(this->list.begin()+index);
      if (this->inherited != 0) this->inherited->erase(this->inherited->begin()+index);
      this->changeNotifier.emit(this, ContentChangeOp::REMOVED, index);
    }
  }

  public: Word getCount() const
  {
    return this->list.size();
  }

  public: SharedPtr<CTYPE> const& get(Int index) const
  {
    if (static_cast<Word>(index) >= this->list.size()) {
      throw EXCEPTION(InvalidArgumentException, S("index"), S("Index out of range."), index);
    }
    return this->list[index];
  }

  public: void clear()
  {
    Int i = 0;
    while (static_cast<Word>(i) < this->getBaseDefCount()) {
      ASSERT(this->base != 0);
      ASSERT(this->inherited != 0);
      if (!this->inherited->at(i)) {
        this->changeNotifier.emit(this, ContentChangeOp::WILL_UPDATE, i);
        this->list[i] = this->getFromBase(i);
        this->inherited->at(i) = true;
        this->changeNotifier.emit(this, ContentChangeOp::UPDATED, i);
      }
      ++i;
    }
    Int j = this->getCount() - 1;
    while (j >= i) {
      this->remove(j);
      --j;
    }
  }

  public: Bool isInherited(Int index) const
  {
    if (static_cast<Word>(index) >= this->list.size()) {
      throw EXCEPTION(InvalidArgumentException, S("index"), S("Out of range."), index);
    }
    if (this->inherited == 0) return false;
    else return this->inherited->at(index);
  }

  public: void reserve(Int size)
  {
    this->list.reserve(size);
  }

  /// @}

  /// @name Containing Implementation
  /// @{

  public: virtual void setElement(Int index, CTYPE *val)
  {
    this->set(index, getSharedPtr(val));
  }

  public: virtual void removeElement(Int index)
  {
    this->remove(index);
  }

  public: virtual Word getElementCount() const
  {
    return this->getCount();
  }

  public: virtual CTYPE* getElement(Int index) const
  {
    return this->get(index).get();
  }

  public: virtual Int addElement(CTYPE *val)
  {
    return this->add(getSharedPtr(val));
  }

  public: virtual void insertElement(Int index, CTYPE *val)
  {
    this->insert(index, getSharedPtr(val));
  }

  /// @}

}; // class

} // namespace

#endif