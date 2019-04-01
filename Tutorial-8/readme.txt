To compile the proxy server:
g++ proxy-caching.cpp -o proxy-caching

To run the proxy server:
./proxy-caching

To compile the client:
g++ client.cpp -o client

To run the client:
./client /files/home/IFS.pdf 192.168.162.24

or
./client /files/home/MCM.pdf 192.168.162.24
./client /files/home/SC_ST.pdf 192.168.162.24
./client /files/home/List_of_Holidays_2019.pdf 192.168.162.24
./client /files/home/List_of_Holidays_2018.pdf 192.168.162.24
./client /files/home/Draft_Senate_Manual_IITH.pdf 192.168.162.24

or
./client /AU%20Prospectus.pdf 185.62.238.50
./client /app_graduate.pdf 185.62.238.50
./client /undergrad_app.pdf 185.62.238.50

or
./client /sites/default/files/logo.png 164.100.161.63
./client /sites/all/themes/mospi/images/footer-gov-logo.png 164.100.161.63
./client /sites/default/files/vkg.jpg 164.100.161.63