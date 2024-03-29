g++ $(pkg-config --libs --cflags fmt) -std=c++20 main.cpp lexer.cpp parser.cpp -o build/main -g
./build/main "$1" > test.s
as test.s -o test.o
ld -macosx_version_min 12.0.0 -o test test.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -arch arm64
./test
echo $?
