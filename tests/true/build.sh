as true.s -o true.o
ld -macosx_version_min 12.0.0 -o true true.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -arch arm64
