!/bin/bash
clear
echo Deleting existing os.c
rm -f os.c
echo "Writing new os.c (without main())"
cat os_test.c > os.c
echo Compiling...
gcc -O3 -Wall os.c pt.c os_a1_test.c
echo "Running the tester"
echo ""

./a.out
sleep 10
