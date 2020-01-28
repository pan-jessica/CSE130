Jessica Pan - jeypan@ucsc.edu
FALL 2019 - CSE 130 Principles of Computer System Design
Asgn2 -- Multithreading HTTP server with logging/////////////////////////////////////////////////////////////////////

This program is an implementation of an HTTP server that only responds to GET and PUT requests.  
To compile this program you may use the Makefile to run the program: httpserver.  
You can manually compile this program with the command: g++ httpserver.cpp -o httpserver  
To run the program use the command: ./httpserver -N [# of threads] -l [log filename] [IP address] [optional port #]  
Once the server is up and running, you can use curl(1) to act as a web client to the server.

To complete a GET request: curl [IP address]:[port #] (i.e curl 127.0.0.1:8080)  
To complete a PUT request: curl --upload-file [filename][ip address]:[port #] (i.e curl 127.0.0.1:8080)  
Using curl(1) will allow you to make a connection with the HTTP server.
