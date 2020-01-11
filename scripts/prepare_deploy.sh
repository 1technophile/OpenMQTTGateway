echo "renaming bin files with the board name"
rename -v 's:/:-:g' .pio/build/*/*.bin
mkdir toDeploy
rename 's/.pio-build-//' .*.bin
cd .pio/libdeps
echo "zipping libraries per board"
for i in */; do zip -r "${i%/}-libraries.zip" "$i"; done
ls -la
mv *.zip ../../toDeploy
cd ../..
echo "zipping code and licence"
zip -r OpenMQTTGateway_sources.zip main LICENSE.txt
mv *.zip toDeploy
mv *.bin toDeploy
cd toDeploy
ls -la
cd ..
