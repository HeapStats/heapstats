/*!
 * \file symbolFinder.cpp
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <limits.h>
#include <link.h>

#include "globals.hpp"
#include "symbolFinder.hpp"

/*!
 * \brief Flag of BFD libarary is already initialized.
 */
bool TSymbolFinder::initFlag = false;

/*!
 * \brief Create record stored library information.
 * \param libinfo [out] Record of library information.
 * \param libPath [in]  Part of target library path.
 * \param libName [in]  Target library name.
 */
inline void createLibInfo(TLibraryInfo *libinfo, const char *libPath,
                          const char *libName) {
  libinfo->libpath_len = strlen(libPath);
  libinfo->libpath = strdup(libPath);
  if (unlikely(libinfo->libpath == NULL)) {
    return;
  }

  libinfo->libname = strdup(libName);
  libinfo->libname_len = strlen(libName);
  if (unlikely(libinfo->libname == NULL)) {
    free(libinfo->libpath);
    libinfo->libpath = NULL;
    return;
  }
}

/*!
 * \brief Callback function of dl_iterate_phdr(3).
 * \param info [in] Information of loaded library on memory.
 * \param size [in] Size of library area on memory.
 * \param data [in] User data.
 * \return Continues callback loop if value is 0.<br>
 *         Abort callback loop if value is 1.
 */
static int libraryCallback(struct dl_phdr_info *info, size_t size, void *data) {
  TLibraryInfo *libinfo = (TLibraryInfo *)data;

  /*
   * Get real path of library.
   * "sun.boot.library.path" system property for libjvm.so will be set real
   * path.
   * See os::jvm_path() in hotspot/src/os/linux/vm/os_linux.cpp
   */
  char real_path[PATH_MAX];
  realpath(info->dlpi_name, real_path);

  /* If library is need. */
  if ((strncmp(real_path, libinfo->libpath, libinfo->libpath_len) == 0) &&
      (strncmp(basename(real_path), libinfo->libname, libinfo->libname_len) ==
       0)) {
    /* Store library information. */
    libinfo->realpath = strdup(real_path);
    libinfo->baseaddr = info->dlpi_addr;

    /* Abort callback loop. */
    return 1;
  }

  return 0;
}

/*!
 * \brief TSymbolFinder constructor.
 */
TSymbolFinder::TSymbolFinder(void) {
  if (unlikely(!initFlag)) {
    initFlag = true;

    /* Global initialize. */
    bfd_init();
  }

  /* Zero clear records. */
  memset(&libBfdInfo, 0, sizeof(libBfdInfo));
  memset(&debugBfdInfo, 0, sizeof(debugBfdInfo));

  memset(&targetLibInfo, 0, sizeof(targetLibInfo));
}

/*!
 * \brief TSymbolFinder destructor.
 */
TSymbolFinder::~TSymbolFinder() {
  /* Cleanup. */
  this->clear();
}

/*!
 * \brief Load target libaray.
 * \param pathPattern [in] Pattern of target library path.
 * \param libname     [in] Name of library.
 * \return Is success load library.<br>
 */
bool TSymbolFinder::loadLibrary(const char *pathPattern, const char *libname) {
  /* Sanity check. */
  if (unlikely(pathPattern == NULL)) {
    logger->printWarnMsg("Library path is not set.");
    return false;
  }

  if (unlikely(libname == NULL)) {
    logger->printWarnMsg("Library name is not set.");
    return false;
  }

  /* Initialize. */
  createLibInfo(&targetLibInfo, pathPattern, libname);
  targetLibInfo.realpath = NULL;
  targetLibInfo.baseaddr = ((ptrdiff_t)0);

  /* If failure allocate memory. */
  if (unlikely((targetLibInfo.libpath == NULL) ||
               (targetLibInfo.libname == NULL))) {
    logger->printWarnMsg("Cannot allocate memory for searching symbols.");
    return false;
  }

  /* Search target library on memory. */
  if (unlikely(dl_iterate_phdr(&libraryCallback, &targetLibInfo) == 0)) {
    logger->printWarnMsg("Cannot find library: %s", libname);
    free(targetLibInfo.libname);
    targetLibInfo.libname = NULL;
    free(targetLibInfo.libpath);
    targetLibInfo.libpath = NULL;
    return false;
  }

  /* Load library with bfd record. */
  loadLibraryInfo(targetLibInfo.realpath, &libBfdInfo);

  /* If bfd record has the symbols, no need to search debuginfo */
  if (libBfdInfo.hasSymtab && libBfdInfo.staticSymCnt > 0 &&
      libBfdInfo.dynSymCnt > 0) {
    return true;
  }

  char dbgInfoPath[PATH_MAX];
  dbgInfoPath[0] = '\0';

  /* Try to get debuginfo path from .note.gnu.build-id */
  asection *buid_id_section =
      bfd_get_section_by_name(libBfdInfo.bfdInfo, ".note.gnu.build-id");
  if (buid_id_section != NULL) {
    TBuildIdInfo *buildID;

    if (bfd_malloc_and_get_section(libBfdInfo.bfdInfo, buid_id_section,
                                   (bfd_byte **)&buildID)) {
      sprintf(dbgInfoPath, DEBUGINFO_DIR "/.build-id/%02hhx/",
              *(&(buildID->contents) + buildID->name_size));

      for (unsigned int Cnt = buildID->name_size + 1;
           Cnt < buildID->name_size + buildID->hash_size; Cnt++) {
        char digitChars[3];
        sprintf(digitChars, "%02hhx", *(&(buildID->contents) + Cnt));
        strcat(dbgInfoPath, digitChars);
      }

      strcat(dbgInfoPath, DEBUGINFO_SUFFIX);
    }

    if (buildID != NULL) {
      free(buildID);
    }
  }

  if (dbgInfoPath[0] == '\0') {
    /* Try to get debuginfo path from .gnu_debuglink */
    char *buf = bfd_follow_gnu_debuglink(libBfdInfo.bfdInfo, DEBUGINFO_DIR);

    if (buf != NULL) {
      strcpy(dbgInfoPath, buf);
      free(buf);
    }
  }

  if (dbgInfoPath[0] != '\0') {
    /* Load library's debuginfo with bfd record. */
    struct stat st = {0};
    if (unlikely(stat(&(dbgInfoPath[0]), &st) != 0)) {
      logger->printWarnMsg("Cannot read debuginfo from ", dbgInfoPath);
    } else {
      logger->printDebugMsg("Try to read debuginfo from ", dbgInfoPath);
      loadLibraryInfo(dbgInfoPath, &debugBfdInfo);
    }

  } else {
    logger->printDebugMsg("The same version of debuginfo not found: ",
                          libBfdInfo.bfdInfo->filename);
  }

  /* If failure both load process. */
  if (unlikely(libBfdInfo.staticSymCnt == 0 && debugBfdInfo.staticSymCnt == 0 &&
               libBfdInfo.dynSymCnt == 0 && debugBfdInfo.dynSymCnt == 0)) {
    logger->printWarnMsg("Cannot load library information.");
    free(targetLibInfo.libname);
    targetLibInfo.libname = NULL;
    free(targetLibInfo.libpath);
    targetLibInfo.libpath = NULL;
    free(targetLibInfo.realpath);
    targetLibInfo.realpath = NULL;
    return false;
  }

  return true;
}

/*!
 * \brief Find symbol in target library.
 * \param symbol [in] Symbol string.
 * \return Address of designated symbol.<br>
 *         value is null if process is failure.
 */
void *TSymbolFinder::findSymbol(char const *symbol) {
  void *result = NULL;

  /* Search symbol in library as static symbol. */

  if (likely(libBfdInfo.staticSymCnt != 0)) {
    result = doFindSymbol(&libBfdInfo, symbol, false);
  }

  /* If not found symbol. */
  if (unlikely(result == NULL)) {
    /* Search symbol in library's debuginfo. */

    if (likely(debugBfdInfo.staticSymCnt != 0)) {
      result = doFindSymbol(&debugBfdInfo, symbol, false);
    }
  }

  /* Search symbol in library as dynamic symbol. */

  if (unlikely(result == NULL)) {
    /* Search symbol in library. */

    if (likely(libBfdInfo.dynSymCnt != 0)) {
      result = doFindSymbol(&libBfdInfo, symbol, true);
    }
  }

  if (unlikely(result == NULL)) {
    /* Search symbol in library's debuginfo. */

    if (likely(debugBfdInfo.dynSymCnt != 0)) {
      result = doFindSymbol(&debugBfdInfo, symbol, true);
    }
  }

  if (likely(result != NULL)) {
    /* Convert absolute symbol address. */
    result = getAbsoluteAddress(result);
  }

  return result;
}

/*!
 * \brief Load target libaray information.
 * \param path    [in]  Target library path.
 * \param libInfo [out] BFD information record.<br>
 *                      Value is null if process is failure.
 */
void TSymbolFinder::loadLibraryInfo(char const *path, TLibBFDInfo *libInfo) {
  bfd *bfdDesc = NULL;

  /* Open library. */
  bfdDesc = bfd_openr(path, NULL);
  if (unlikely(bfdDesc == NULL)) {
    return;
  }

  /* If illegal format. */
  if (!bfd_check_format(bfdDesc, bfd_object)) {
    bfd_close(bfdDesc);
    return;
  }

  /* If library don't have symbol. */
  if (!(bfd_get_file_flags(bfdDesc) & HAS_SYMS)) {
    bfd_close(bfdDesc);
    return;
  }

  /* Library is legal binary file. */
  asymbol *syms = NULL;

  /* Create empty symbol as temporary working space. */
  syms = bfd_make_empty_symbol(bfdDesc);
  if (unlikely(syms == NULL)) {
    bfd_close(bfdDesc);
    return;
  }

  libInfo->bfdInfo = bfdDesc;
  /* Load static symbols. */
  libInfo->staticSymCnt = bfd_read_minisymbols(
      bfdDesc, 0, (void **)&libInfo->staticSyms, &libInfo->staticSymSize);
  /* Load dynamic symbols. */
  libInfo->dynSymCnt = bfd_read_minisymbols(
      bfdDesc, 1, (void **)&libInfo->dynSyms, &libInfo->dynSymSize);
  /* Check .symtab section. If there are no symbols in the BFD,
   * below function returns the size of terminal Null pointer or 0 */
  libInfo->hasSymtab = ((size_t)bfd_get_symtab_upper_bound(bfdDesc)
                                                            > sizeof(NULL));
  libInfo->workSym = syms;
}

/*!
 * \brief Find symbol in target library to use BFD.
 * \param libInfo  [in] BFD information record.
 * \param symbol   [in] Symbol string.
 * \param isDynSym [in] Symbol is dynamic symbol.
 * \return Address of designated symbol.<br>
 *         value is null if process is failure.
 */
void *TSymbolFinder::doFindSymbol(TLibBFDInfo *libInfo, char const *symbol,
                                  bool isDynSym) {
  bfd *bfdDesc = libInfo->bfdInfo;
  asymbol *syms = libInfo->workSym;
  char *miniSymsEntry = NULL;
  int miniSymCnt = 0;
  int dyn = ((isDynSym) ? 1 : 0);
  void *result = NULL;
  unsigned int size = 0;

  if (isDynSym) {
    miniSymCnt = libInfo->dynSymCnt;
    miniSymsEntry = libInfo->dynSyms;
    size = libInfo->dynSymSize;
  } else {
    miniSymCnt = libInfo->staticSymCnt;
    miniSymsEntry = libInfo->staticSyms;
    size = libInfo->staticSymSize;
  }

  /* Search symbol. */
  for (int idx = 0; idx < miniSymCnt; idx++) {
    /* Convert symbol to normally symbol. */
    asymbol *symEntry =
        bfd_minisymbol_to_symbol(bfdDesc, dyn, miniSymsEntry, syms);

    if (strcmp(bfd_asymbol_name(symEntry), symbol) == 0) {
      /* Found target symbol. */
      result = (void *)bfd_asymbol_value(symEntry);
      break;
    }

    /* Move next symbol entry. */
    miniSymsEntry += (ptrdiff_t)size;
  }

  return result;
}

/*!
 * \brief Clear loaded libaray data.
 */
void TSymbolFinder::clear(void) {
  /* Cleanup. */
  free(targetLibInfo.libname);
  targetLibInfo.libname = NULL;
  free(targetLibInfo.libpath);
  targetLibInfo.libpath = NULL;
  free(targetLibInfo.realpath);
  targetLibInfo.realpath = NULL;

  /* Deallocate and null-clear bfd object. */
  if (unlikely(libBfdInfo.bfdInfo != NULL)) {
    bfd_close(libBfdInfo.bfdInfo);
    libBfdInfo.bfdInfo = NULL;
  }

  if (unlikely(debugBfdInfo.bfdInfo != NULL)) {
    bfd_close(debugBfdInfo.bfdInfo);
    debugBfdInfo.bfdInfo = NULL;
  }

  /* Deallocate and null-clear each symbol object. */
  free(libBfdInfo.staticSyms);
  libBfdInfo.staticSyms = NULL;
  libBfdInfo.staticSymCnt = 0;

  free(debugBfdInfo.staticSyms);
  debugBfdInfo.staticSyms = NULL;
  debugBfdInfo.staticSymCnt = 0;

  free(libBfdInfo.dynSyms);
  libBfdInfo.dynSyms = NULL;
  libBfdInfo.dynSymCnt = 0;

  free(debugBfdInfo.dynSyms);
  debugBfdInfo.dynSyms = NULL;
  debugBfdInfo.dynSymCnt = 0;
}
