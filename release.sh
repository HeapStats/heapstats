#!/bin/bash -ue

# Interactive script for release
# You should `dnf upgrade` before run it

# Check icedtea repository
if [[ "$(hg branch)" != "default" ]]; then
  echo "You must be on the default branch of mercurial SCM to release"
  exit 1
fi
if [[ ! "$(hg paths default)" =~ .*icedtea\.classpath\.org/hg/release/.* ]]; then
  echo "You must be on the icedtea release repository (/hg/release/*) to release"
  exit 1
fi

# Check Author
AUTHOR=$(hg config ui.username)
read -p "Is release author: ${AUTHOR} ? (y/n) > " _check
if [[ ${_check} != "y" && ${_check} != "Y" ]]; then
  echo "Modify hg config ui.username"
  exit 1
fi

# Check Date
DATE=$(LANG=C date "+%Y-%m-%d")
read -p "Is release date: ${DATE} ? (y/n) > " _check_date
if [[ ${_check_date} != "y" && ${_check_date} != "Y" ]]; then
  read -p "Enter release date (YYYY-MM-DD) > " DATE
  if [[ ! ${DATE} =~ [2-9][0-9][0-9][0-9]\-[0-1][0-9]\-[0-3][0-9] ]]; then
    echo "Enter correct date as YYYY-MM-DD"
    exit 1
  fi
  DATE=$(LANG=C date --date=${DATE} "+%Y-%m-%d")
fi

# Input new version
CURRENT_VERSION=`sed -n 's#^\s\+<version>\(.\+\)</version>#\1#p' pom.xml`
echo "Current version is ${CURRENT_VERSION}"
read -p "Enter release version > " VERSION
if [[ ! ${VERSION} =~ [0-9]\.[0-9]\.[0-9] ]]; then
  echo "Enter correct version as [0-9].[0-9].[0-9]"
  exit 1
fi

# analyzer
## pom
sed -i -e "/^\s\+<version>/s#${CURRENT_VERSION}#${VERSION}#g" pom.xml
## Dialog
sed -i -e "/Version/s#text=\"Version\s.\+\"#text=\"Version ${VERSION}\"#g" analyzer/fx/src/main/resources/jp/co/ntt/oss/heapstats/aboutDialog.fxml

# configure.ac
sed -i -e "/AC_INIT(\[HeapStats\]/s#[0-9]\.[0-9]\.[0-9]#${VERSION}#g" configure.ac
sed -i -e "/HEAPSTATS_MAJOR_VERSION/s#[0-9]\.[0-9]#${VERSION:0:3}#g" configure.ac
## remake configure
autoconf

# heapstats.spec
SPECFILE=specs/heapstats.spec
sed -i -e "/Version:/s#[0-9]\.[0-9]\.[0-9]#${VERSION}#g" ${SPECFILE}
## Copyright
YEAR=$(LANG=C date "+%Y")
TO_YEAR=`sed -n 's#^Copyright\s(C)\s2011-\(.\+\)\sNippon.\+#\1#p' ${SPECFILE}`
TO_YEAR=${TO_YEAR:0:4}
if [[ ${TO_YEAR} != ${YEAR} ]]; then
  sed -i "/^Copyright/s#${TO_YEAR}#${YEAR}#g" ${SPECFILE}
fi
## changelog
LINE=$(($(sed -n "/%changelog/=" ${SPECFILE})+1))
SPECDATE=$(LANG=C date --date=${DATE} "+%a %b %d %Y")
sed -i "${LINE}i\* ${SPECDATE} ${AUTHOR}\n- Bump to ${VERSION}" ${SPECFILE}

# ChangeLog
sed -i "1i${DATE}  ${AUTHOR}\n\n\t\* Bump to ${VERSION}\n" ChangeLog

# NEWS
sed -i "1iNew in release ${VERSION} (${DATE})\n\n* Fix some bugs\n" NEWS
## Edit to add more detailed history
read -p "Press ENTER to edit NEWS ..."
if [ -z ${EDITOR+x} ]; then
  vim NEWS
else
  ${EDITOR} NEWS
fi

# remake Makefile by latest automake
automake
bash configure

# hg tag
read -p "Do you want 'hg tag' automatically? (y/n) > " _check_tag
if [[ ${_check_tag} == "y" || ${_check_tag} == "Y" ]]; then
  #TODO: hg status / add
  hg tag -m "Bump to ${VERSION}" ${VERSION}
fi

cat << _EOF_

==============================================================

All done. Ready to release.

You should check
 * hg status to check non-committed files
 * hg tags to check tag for new version
 * To check changelogs
  * NEWS
  * ChangeLog
  * spec/heapstats.spec

This script also changes the below files
 * pom.xml (<version>)
 * analyzer/fx/src/main/resources/jp/co/ntt/oss/heapstats/aboutDialog.fxml (Version: XXX)
 * configure.ac

==============================================================

_EOF_

