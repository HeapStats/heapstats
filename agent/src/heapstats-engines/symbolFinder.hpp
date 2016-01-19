/*!
 * \file symbolFinder.hpp
 * \brief This file is used by search symbol in library.
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

#ifndef _SYMBOL_FINDER_HPP
#define _SYMBOL_FINDER_HPP

#include <bfd.h>
#include <sys/stat.h>
#include <stddef.h>

#include "util.hpp"

#ifndef DEBUGINFO_DIR

/*!
 * \brief Debuginfo parent directory.
 */
#define DEBUGINFO_DIR "/usr/lib/debug"
#endif

#ifndef DEBUGINFO_SUFFIX

/*!
 * \brief Debuginfo binary name suffix.
 */
#define DEBUGINFO_SUFFIX ".debug"
#endif

/*!
 * \brief Data container of shared library name and loaded virtual address.
 */
typedef struct {
  char *libname;      /*!< Name of target library.          */
  size_t libname_len; /*!< Length of target library name.   */
  char *libpath;      /*!< Part of library path.            */
  size_t libpath_len; /*!< Length of library path.          */
  char *realpath;     /*!< Real path of library.            */
  ptrdiff_t baseaddr; /*!< Pointer of library base address. */
} TLibraryInfo;

/*!
 * \brief BFD symbol information structure.
 */
typedef struct {
  bfd *bfdInfo;               /*!< BFD descriptor.                     */
  char *staticSyms;           /*!< List stored static symbol.          */
  int staticSymCnt;           /*!< Count of static symbols.            */
  unsigned int staticSymSize; /*!< Size of single static symbols.      */
  char *dynSyms;              /*!< List stored dynamic symbol.         */
  int dynSymCnt;              /*!< Count of dynamic symbols.           */
  unsigned int dynSymSize;    /*!< Size of single dynamic symbols.     */
  bool hasSymtab;             /*!< Flag of BFD has .symtab section.    */
  asymbol *workSym;           /*!< Empty symbol for working temporary. */
} TLibBFDInfo;

/*!
 * \brief .note.gnu.build-id information structure.
 */
typedef struct {
  unsigned int name_size;  /*!< size of the name.              */
  unsigned int hash_size;  /*!< size of the hash.              */
  unsigned int identifier; /*!< NT_GNU_BUILD_ID == 0x3         */
  char contents;           /*!< name ("GNU") & hash (build id) */
} TBuildIdInfo;

/*!
 * \brief C++ class for symbol search.
 */
class TSymbolFinder {
 public:
  /*!
   * \brief TSymbolFinder constructor.
   */
  TSymbolFinder(void);

  /*!
   * \brief TSymbolFinder destructor.
   */
  virtual ~TSymbolFinder();

  /*!
   * \brief Load target libaray.
   * \param pathPattern [in] Pattern of target library path.
   * \param libname     [in] Name of library.
   * \return Is success load library.<br>
   */
  virtual bool loadLibrary(const char *pathPattern, const char *libname);

  /*!
   * \brief Find symbol in target library.
   * \param symbol [in] Symbol string.
   * \return Address of designated symbol.<br>
   *         value is null if process is failure.
   */
  virtual void *findSymbol(char const *symbol);

  /*!
   * \brief Clear loaded libaray data.
   */
  virtual void clear(void);

  /*!
   * \brief Get target library name.
   * \return String of library name.
   */
  const inline char *getLibraryName(void) {
    return this->targetLibInfo.libname;
  };

  /*!
   * \brief Get target library base address.
   * \return Pointer of library base address.
   */
  const inline void *getLibraryAddress(void) {
    return (void *)this->targetLibInfo.baseaddr;
  };

  /*!
   * \brief Get absolute address.
   * \param addr [in] Address of library symbol.
   * \return Absolute symbol address in library.
   */
  inline void *getAbsoluteAddress(void *addr) {
    return (void *)((ptrdiff_t) this->targetLibInfo.baseaddr + (ptrdiff_t)addr);
  };

 protected:
  /*!
   * \brief Load target libaray information.
   * \param path    [in]  Target library path.
   * \param libInfo [out] BFD information record.<br>
   *                      Value is null if process is failure.
   */
  virtual void loadLibraryInfo(char const *path, TLibBFDInfo *libInfo);

  /*!
   * \brief Find symbol in target library to use BFD.
   * \param libInfo  [in] BFD information record.
   * \param symbol   [in] Symbol string.
   * \param isDynSym [in] Symbol is dynamic symbol.
   * \return Address of designated symbol.<br>
   *         value is null if process is failure.
   */
  virtual void *doFindSymbol(TLibBFDInfo *libInfo, char const *symbol,
                             bool isDynSym);

  RELEASE_ONLY(private :)
  /*!
   * \brief Flag of BFD libarary is already initialized.
   */
  static bool initFlag;

  /*!
   * \brief Record of target library information.
   */
  TLibraryInfo targetLibInfo;
  /*!
   * \brief BFD record of target library information.
   */
  TLibBFDInfo libBfdInfo;
  /*!
   * \brief BFD record of target library's debug information.
   */
  TLibBFDInfo debugBfdInfo;
};

#endif  // _SYMBOL_FINDER_HPP
