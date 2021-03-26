g++ -o ./build/ftpr ftpr.cpp ./libs/FileTransfer.cpp ./libs/Authorization.cpp -pthread 
g++ -o ./build/ftpc ftpc.cpp ./libs/FileTransfer.cpp ./libs/Authorization.cpp