/*!
 * \file sorter.hpp
 * \brief This file is used sorting class datas.
 * Copyright (C) 2011-2015 Nippon Telegraph and Telephone Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef SORTER_HPP
#define SORTER_HPP

/*!
 * \brief This structure use stored sorting data.
 */
template <typename T>
struct Node {
  struct Node<T> *prev; /*!< Previoue node data.     */
  struct Node<T> *next; /*!< Next     node data.     */
  T value;              /*!< Pointer of sort target. */
};

/*!
 * \brief Comparator for TSorter.
 * \param arg1 [in] Target of comparison.
 * \param arg2 [in] Target of comparison.
 * \return Difference of arg1 and arg2.
 */
typedef int (*TComparator)(const void *arg1, const void *arg2);

/*!
 * \brief This class is used sorting data defined by <T>.
 */
template <typename T>
class TSorter {
 private:
  /*!
   * \brief Max sorted data count.
   */
  int _max;
  /*!
   * \brief Now tree having node count.<br>
   *        This value is never large more than '_max'.<br>
   *        Tree will remove smallest data in everytime,
   *        if add many data to tree beyond limit.
   */
  int _nowIdx;

  /*!
   * \brief Sorter holding objects.
   */
  struct Node<T> *container;
  /*!
   * \brief The node is held node first.
   */
  struct Node<T> *_top;

  /*!
   * \brief Comparator used by tree.
   */
  TComparator cmp;

 public:
  /*!
   * \brief TSorter constructor.
   * \param max        [in] Max array size.
   * \param comparator [in] Comparator used compare sorting data.
   */
  TSorter(int max, TComparator comparator)
      : _max(max), _nowIdx(-1), _top(NULL), cmp(comparator) {
    /* Allocate sort array. */
    this->container = new struct Node<T>[this->_max];
  }

  /*!
   * \brief TSorter destructor.
   */
  virtual ~TSorter() {
    /* Free allcated array. */
    delete[] this->container;
  }

  /*!
   * \brief Add data that need sorting.
   * \param val [in] add target data.
   */
  virtual void push(T val) {
    struct Node<T> *tmpTop, *newNode;

    /* If count is less than 1. */
    if (unlikely(this->_max <= 0)) {
      return;
    }

    /* Push data. */
    if (this->_nowIdx < (this->_max - 1)) {
      /* If count of holding element is smaller than array size. */
      this->_nowIdx++;
      this->container[this->_nowIdx].prev = NULL;
      this->container[this->_nowIdx].next = NULL;
      this->container[this->_nowIdx].value = val;

      /* If holds none. */
      if (this->_nowIdx == 0) {
        this->_top = this->container;
        return;
      }

      tmpTop = this->_top;
      newNode = &this->container[this->_nowIdx];
    } else if ((*this->cmp)(&this->_top->value, &val) < 0) {
      /* If added element is bigger than smallest element in array. */

      /* If holds only 1 element. */
      if (this->_top->next == NULL) {
        this->_top->value = val;
        return;
      }

      /* Setting element. */
      tmpTop = this->_top->next;
      tmpTop->prev = NULL;

      this->_top->prev = NULL;
      this->_top->next = NULL;
      this->_top->value = val;
      newNode = this->_top;
      this->_top = tmpTop;
    } else {
      /* If don't need to refresh array. */
      return;
    }

    /* Rebuild node array. */
    struct Node<T> *tmpNode = tmpTop;
    while (true) {
      /* Compare element in array. */
      if ((*this->cmp)(&newNode->value, &tmpNode->value) < 0) {
        /* If added element is smaller than certain element in array. */

        /* Setting chain. */
        newNode->prev = tmpNode->prev;
        newNode->next = tmpNode;
        tmpNode->prev = newNode;

        /* If first node. */
        if (newNode->prev == NULL) {
          this->_top = newNode;
        } else {
          newNode->prev->next = newNode;
        }

        return;
      } else if (tmpNode->next == NULL) {
        /* If added element is bigger than biggest element in array. */

        /* Insert top. */
        tmpNode->next = newNode;
        newNode->prev = tmpNode;
        return;
      } else {
        /* If added element is bigger than a certain element in array. */

        /* Move next element. */
        tmpNode = tmpNode->next;
      }
    }
  }

  /*!
   * \brief Get sorted first data.
   * \return Head object in sorted array.
   */
  inline struct Node<T> *topNode(void) { return this->_top; };

  /*!
   * \brief Get sorted last data.
   * \return Last object in sorted array.
   */
  inline struct Node<T> *lastNode(void) {
    /* Get array head. */
    struct Node<T> *node = this->_top;

    /* If array is empty. */
    if (node == NULL) {
      return NULL;
    }

    /* Move to last. */
    while (node->next != NULL) {
      node = node->next;
    }

    /* Return last node. */
    return node;
  }

  /*!
   * \brief Get count of element in array.
   * \return element count.
   */
  inline int
  getCount(void) {
    return _nowIdx + 1;
  }
};

#endif  // SORTER
