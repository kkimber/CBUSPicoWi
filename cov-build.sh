rm *.tgz
cmake --build build --target clean
cov-build --dir cov-int cmake --build build
tar czvf CBUSPicoWi.tgz cov-int
