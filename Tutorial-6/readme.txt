To compile:
g++ client-4.cpp -std=c++11 -lpthread

To run:
./a.out 192.168.162.24 intranet-objects.txt
./a.out 180.179.170.86 bjp-objects.txt
./a.out 185.62.238.50 africa-objects.txt

------To get IP------
dig +noall +answer intranet.iith.ac.in
dig +noall +answer bjp.org
dig +noall +answer africau.edu