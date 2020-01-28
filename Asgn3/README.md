Jessica Pan - jeypan@ucsc.edu  
FALL 2019 - CSE 130 Principles of Computer System Design  
Asgn3 -- HTTP server with caching and logging  

This program is an implementation of an HTTP server that only responds to GET and PUT requests.  
To compile this program you may use the Makefile to run the program: httpserver.  
You can manually compile this program with the command: g++ httpserver.cpp -o httpserver  
To run the program use the command: ./httpserver -c -l [log filename] [IP address] [optional port #]  
Once the server is up and running, you can use curl(1) to act as a web client to the server.

To complete a GET request: curl [IP address]:[port #] (i.e curl 127.0.0.1:8080)  
To complete a PUT request: curl --upload-file [filename][ip address]:[port #] (i.e curl 127.0.0.1:8080)  
Using curl(1) will allow you to make a connection with the HTTP server.

Known Issues:  
    When the program performs GET/PUT requests, the all perform correctly in terms of:  
        - Pushing the file to the Cache  
        - Logging the file to the logfile  
        - Receiving the contents of the file in the requests  
        - Contents of files aren't changed and are written to the disk and cache properly  
        **Return the proper status codes and content lengths  

**When the user first starts the httpserver and the first GET request is performed is successful.  
    However, after the first GET request to the server, there are issues with the GET requests that follow it.  
    After the first GET request, if a new filename is used, knowing that it's a valid file, to perform the 
    next GET request and its the first time doing a GET request for that file and it's also not on the cache, 
    then the client recieves the proper status code but the content-length header is always 0 and the data is not 
    printed to user. But if you were to perform that request again any time afterwards, the proper content-length 
    is returned and the proper content is printed back to the client. This happens with all the GET requests that 
    use a new file (not in cache), except the first GET request performed after the server starts.  

****This issue doesn't affect the caching process, doesn't affect the logging process, doesn't affect the data 
    that's being cached.
