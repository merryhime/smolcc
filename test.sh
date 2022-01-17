echo expect 111
./build.sh "return 69+42;"
echo expect 111
./build.sh "return 1000-889;"
echo expect 111
./build.sh "return 13%10*23+(10- -4)/2*(34-28);"
echo expect 1
./build.sh "return 100 < 999;"
echo expect 1
./build.sh "return 10000 > -32;"
echo expect 0
./build.sh "return 10000 == -32;"
echo expect 42
./build.sh "{ if (1) { 42; } else { 69; } }"
echo expect 69
./build.sh "{ if (1 == 0) 42; else 69; }"
echo expect 69
./build.sh "{ for (0; 1; 0) { return 69; } return 42; }"
echo expect 69
./build.sh "{ while (1) { return 69; } }"
echo expect 42
./build.sh "{ int x; int y; *(&x+0) = 42; x; }"
echo expect 0
./build.sh "{ int x; int y; *(&x+0) = 42; y; }"
echo expect 0
./build.sh "{ int x; int y; *(&x+1) = 42; x; }"
echo expect 42
./build.sh "{ int x; int y; *(&x+1) = 42; y; }"
