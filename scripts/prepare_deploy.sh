#!/bin/bash
set -e
echo "renaming bin files with the board name"
rename -v 's:/:-:g' .pio/build/*/*.bin
mkdir toDeploy
rename 's/.pio-build-//' .*.bin
# don't publish binaries for *-all and *-test env
rm *-all.bin *-test.bin
(
  cd .pio/libdeps
  echo "replace space by _ in folder names"
  find . -type d -name "* *" | while read FNAME; do mv "$FNAME" "${FNAME// /_}"; done
  echo "zipping libraries per board"
  for i in */
  do
   zip -r "${i%/}-libraries.zip" "$i"
  done
  # don't publish libraries for *-test env
  rm *-test.zip
  ls -lA
  mv *.zip ../../toDeploy
)

echo "zipping code and licence"
zip -r OpenMQTTGateway_sources.zip main LICENSE.txt
mv *.zip toDeploy
mv *.bin toDeploy

ls -lA toDeploy
