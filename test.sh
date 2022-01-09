echo expect 111
./build.sh "69+42"
echo expect 111
./build.sh "1000-889"
echo expect 111
./build.sh "13%10*23+(10- -4)/2*(34-28)"
echo expect 1
./build.sh "100 < 999"
echo expect 1
./build.sh "10000 > -32"
echo expect 0
./build.sh "10000 == -32"
