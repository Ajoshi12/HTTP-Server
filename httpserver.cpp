#include <iostream>
#include <iomanip>
#include <sstream> 
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <deque> 
#include <vector> 
#include <unordered_map> 
#include <arpa/inet.h>

// Global variables
const size_t BUFFER_SIZE = 4096; // for the header 

int fileloc = 0;
 unsigned short port = 80;
int new_socket; // for the socket connection
char *logf = ""; // log file name if provided
const char *add; // ip address
int Enabledcache; 
std:: unordered_map<std:: string, std:: string> cache; 
std:: deque<std:: string> cachequeue; 



// function in order to print in log file
void printlog(std::string method, int fail,std::string status_code, std::string filename, int file_length, char* data, int was_in_cache)
{
    if(method == "PUT")
    {
        int fd = open(logf,  O_CREAT | O_WRONLY); 
        if(fd != -1)
        {
            if(fail) // meaning it is a failed response
            {
                /* EX: FAIL: PUT abcd HTTP/1.1 --- response 400\n
                */
                 std::string failure = "FAIL: " + std::string("PUT ") +  std::string(filename) + std::string(" HTTP/1.1 --- response ") 
               + std::string(status_code) + std::string("\n") + std::string("========\n");
                //lock then pwrite()
               pwrite(fd,failure.c_str(),  failure.length(), fileloc); //IDK what the last parameter should be 
               fileloc+= failure.length();
            }
            else // else ok 
            {
                /* EX: PUT abcdefghij0123456789abcdefg length 36
                   00000000 65 68 6c 6c 3b 6f 68 20 6c 65 6f 6c 61 20 61 67 6e 69 20 3b
                   00000020 65 68 6c 6c 20 6f 54 28 65 68 43 20 72 61 29 73
                   ========
                */
                std:: string Works;
                if(Enabledcache) // if -c is enabled
                {
                    if(was_in_cache) // file was in cache
                    {
                     // if was in cache
                     Works = std:: string("PUT ") +  filename + std:: string(" length ") 
                     + std:: to_string(file_length) + std::string(" [was in cache]") + std::string("\n"); // for the top line
                    }
                    else                  // else if not in cache 
                    {
                       Works = std:: string("PUT ") +  filename + std:: string(" length ") 
                     + std:: to_string(file_length) + std::string(" [was not in cache]") + std::string("\n"); // for the top line
                    }
                }
                else
                {
                     Works = std:: string("PUT ") +  filename + std:: string(" length ") 
               + std:: to_string(file_length) + std::string("\n"); // for the top line
                }
                
               pwrite(fd,Works.c_str(),  Works.length(), fileloc);
               fileloc+= Works.length();

                 int first = 1; 
               // now time to convert char data into hex 
                  std:: stringstream ss;
                  int hexadec = 0;
                    for(int i = 0; i<strlen(data); i+=1)
                    {
                      if(i%20 == 0)
                      {
                          if(first)
                          {
                              ss  << std::setfill('0') << std::setw(8) << (float)hexadec << " ";
                             hexadec+=20;
                          }
                          else
                          {
                             ss << "\n" << std::setfill('0') << std::setw(8) << (float)hexadec << " ";
                             hexadec+=20;
                          }
                      }
                          first = 0;
                        ss  << std::hex
                        << (int) data[i] << " ";         
                         if(i%20 == 0 && i!= 0 && i == 0)
                         ss << "\n";
                        
                    }
                    
                    std::string h = ss.str();
                    
                    int newl = 0;
                    if (h.at(h.length()-1) == '\n')
                    {
                        newl = 1;
                    }
                  pwrite(fd,h.c_str(),h.length() ,fileloc); //write down the hex data stuff
                  fileloc = fileloc+h.length();

                 std:: string endoflog;
              // printing the very end of the log
              if(newl) // if newl is 1 it means there is a newline
               endoflog = "========\n";
              else // add a newline
               endoflog = "\n========\n";

              pwrite(fd,endoflog.c_str() ,endoflog.length() ,fileloc);
              fileloc = endoflog.length()+ fileloc; 

            }
            close(fd);
        }
    }
    if(method == "GET")
    {
        int fd = open(logf, O_CREAT | O_WRONLY); 
        if(fd != -1)
        {
            if(fail) // meaning it is a failed response
            {
                // EX: FAIL: GET abcd HTTP/1.1 --- response 400\n
               std:: string failure = "FAIL: " + std::string("GET ") +  filename + std::string(" HTTP/1.1 --- response ") 
               + status_code + std::string("\n") + std::string("========\n");
               // lock and pwrite()
              pwrite(fd,failure.c_str(),  failure.length(), fileloc); //IDK what the last parameter should be
              fileloc+= failure.length();

            }
            else // else ok 
            {
                std:: string Works;
                // EX : GET abcdefghij0123456789abcdefg length 0
                if(Enabledcache)
                {
                    if(was_in_cache)
                    {
                    // if stored in cache
                        Works = std:: string("GET ") +  filename + std:: string(" length ") 
                      + std:: to_string(file_length) + std::string(" [was in cache]") + std::string("\n") + std::string("========\n");
                    }
                    else // else not stored in cache
                    {
                          Works = std:: string("GET ") +  filename + std:: string(" length ") 
                      + std:: to_string(file_length) + std::string(" [was not in cache]") + std::string("\n") + std::string("========\n"); 
                    }
                }
                else
                {
                   Works = std:: string("GET ") +  filename + std:: string(" length ") 
                  + std:: to_string(file_length) + std::string("\n") + std::string("========\n");
                }
               // lock then pwrite()
                pwrite(fd,Works.c_str(),  Works.length(), fileloc);
                fileloc+= Works.length();
            }
            close(fd);
        }
    }
    if(method != "GET" && method != "PUT") // for 500 error 
    {
         int fd = open(logf,  O_CREAT | O_WRONLY); 
        if(fd != -1)
        {
            std:: string failure = "FAIL: " + method + " " +  filename + std::string(" HTTP/1.1 --- response ") 
               + status_code + std::string("\n") + std::string("========\n");
           // pwrite()
          pwrite(fd,failure.c_str(),  failure.length(), fileloc); 
          fileloc+= failure.length();
          close(fd);
        }
    }
}





// function that does the get or put part (same logic as assignment 1)
void GetOrPut()
{
    char* token; 
    int fd;
    std::vector<std::string> header; 
    char buffer[BUFFER_SIZE] = {0}; 
    int valread;
    valread = read( new_socket , buffer, BUFFER_SIZE);
      char* rest = buffer; 
      while((token = strtok_r(rest, " \n", &rest)))
      {
        header.push_back(token);
      }
      std::string method;
      int contentl = 0;
      // now I have to loop through the vector and check if it's a PUT or GET and get the content length for PUT
      for(unsigned long i = 0; i< header.size() - 1; i++)
      {
        if(header.at(i) == "GET")
        {
          method = header.at(i);
        }
        if(header.at(i) == "PUT")
        {
          method = header.at(i);
        }
        if(header.at(i) == "Content-Length:")
        {
          contentl = i+1;
        }
      }
      std::string file = header.at(1); // getting the file name
      
      
      if (method == "GET")
      {
        if(file.at(0) == '/') // have to do this for get 
        file = file.substr (1,file.length());
        if(file.size() == 27 && file.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890-_") == std::string::npos) //if file is 27 ascii characters
        {
          fd = open(file.c_str(), O_RDONLY );
          if(fd != -1)
          {
           struct stat fileStat;
           stat(file.c_str(), &fileStat);
           const int size = fileStat.st_size; // file size
           // in case file does not have read or write permission
           if(stat(file.c_str(), &fileStat) != -1 && fileStat.st_mode & !S_IRUSR & !S_IWUSR) 
           {
             char *un = (char*)malloc(10 * sizeof(char));
             std::string FORBIDDEN = std::string("HTTP/1.1 403 Forbidden\r\n") + std::string("Content-Length: 0") + std::string("\r\n\r\n");
             send(new_socket, FORBIDDEN.c_str(), FORBIDDEN.length(), 0); // send the 403 response back
             if (strlen(logf) != 0) {
             printlog("GET",1,"403", file, 0, un, 0);
             }
           }
           char *data = (char*)malloc(size * sizeof(char));
           std::string OK = std::string("HTTP/1.1 200 OK\r\n") + std::string("Content-Length: ") + std::to_string(size) + std::string("\r\n\r\n");
           send(new_socket, OK.c_str(), OK.length(), 0); // send the 200 response back 
           
           if(Enabledcache)
           {
           ///////////////////////////////////// check cache
               if (cache.find(file) == cache.end()) //if element is not present in cache
           {
               while(read(fd, data , size) > 0)
               {
                 send(new_socket, data, size, 0);
               }
                  std::string data_in_string = std::string(data);
                   if(cache.size() >= 4)
                   {
                   // remove the first element in cache
                   cache.erase(cachequeue.front()); 
                   cachequeue.pop_front(); // now remove the first element
                   }
                   // add the file to cache
                   cache.insert({file,data_in_string});
                   cachequeue.push_back(file);
                              close(fd);
                   if (strlen(logf) != 0) {
              printlog("GET",0,"200", file, size, data,0);
            }

           } 
           else  // send from via cache since element is inside map
           {
               // may need a while loop
               send(new_socket,cache.at(file).c_str(), size, 0);
               close(fd);
            if (strlen(logf) != 0) {
              printlog("GET",0,"200", file, size, data,1);
            }
           }
           
         } // end of if cache is enabled
           else
           {
                while(read(fd, data , size) > 0)
               {
                 send(new_socket, data, size, 0);
               }
                          close(fd);
                if (strlen(logf) != 0) {
              printlog("GET",0,"200", file, size, data,0);
            }
           }
         }
       else // fd == -1 meaning the file does not exist
         {
           std::string NOTFOUND = std::string("HTTP/1.1 404 Not Found\r\n") + std::string("Content-Length: 0\r\n\r\n");
           // file does not exist send error back 404
           send(new_socket,NOTFOUND.c_str(),  NOTFOUND.length(), 0);
           char *un = (char*)malloc(10 * sizeof(char));
           // calling printlog function ////////////////////////////////////////////
           if (strlen(logf) != 0) {
           printlog("GET",1,"404", file, 0, un,0);
           }
         }
       }
       else // file is not 27 characters so 400 reply
       {
        char *un = (char*)malloc(10 * sizeof(char));
        std:: string BADREQUEST = std::string("HTTP/1.1 400 Bad Request\r\n") + std::string("Content-Length: 0\r\n\r\n");
        send(new_socket, BADREQUEST.c_str(), BADREQUEST.length(), 0);
        if (strlen(logf) != 0) {
         printlog("GET",1,"400", file, 0, un, 0);
        }
       }
     }
     
     
     
      if(method == "PUT")
     {
       struct stat fileStat;
       stat(file.c_str(), &fileStat);
       std::string cl = header.at(contentl);
       int contentlength = std::atoi(cl.c_str());
       char *data = (char*)malloc(contentlength * sizeof(char));    // the buffer array with size of content length - data that might 
       // have been read if header was less than 4kb 
       if(file.size() == 27 && file.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890-_") == std::string::npos) // if file is 27 ascii characters
       {
         if(stat(file.c_str(),&fileStat) == -1) // if file does not exist
         {
           fd = open(file.c_str(), O_CREAT | O_WRONLY); // create the file 
           if(fd != -1)
           {
             // file does not exist send create header 201 after recieving data
             std::string CREATED = std::string("HTTP/1.1 201 Created\r\n") + std::string("Content-Length: ") + std::to_string(contentlength) + std::string("\r\n\r\n");
             send(new_socket, CREATED.c_str(), CREATED.length(), 0);
             read(new_socket,data , contentlength);
             write(fd, data, contentlength);

             if(Enabledcache)
             {
             // add to cache
                std::string data_in_string = std::string(data);
                   if(cache.size() >= 4)
                   {
                      // remove the first element in cache
                     cache.erase(cachequeue.front()); 
                     cachequeue.pop_front(); // now remove the first element
                   }
                    // add the file to cache and deque
                   cache.insert({file,data_in_string});
                   cachequeue.push_back(file);
             }
               close(fd);
             if (strlen(logf) != 0) {
                printlog("PUT",0,"201", file, contentlength, data,0);
             }
           }
         }
         else // file exists and so update file and return  ok 200 header
         {
          if(stat(file.c_str(), &fileStat) != -1 && fileStat.st_mode & !S_IRUSR & !S_IWUSR) // if file exists but does not have read or write permissions 
          {
            std::string FORBIDDEN = std::string("HTTP/1.1 403 Forbidden\r\n") + std::string("Content-Length: 0") + std::string("\r\n\r\n");
            send(new_socket, FORBIDDEN.c_str(), FORBIDDEN.length(), 0); // send the 403 response back 
            // printlog /////////////////////////////////////////////////////////////////////////////////////
            if (strlen(logf) != 0) {
            printlog("PUT",1,"201", file, 0, data, 0);
            }
          }
           fd = open(file.c_str(), O_TRUNC);
          if(fd != -1)
          {
           std::string OK = std::string("HTTP/1.1 200 OK\r\n") + std::string("Content-Length: ") + std::to_string(contentlength) + std::string("\r\n\r\n");
           send(new_socket, OK.c_str(), OK.length(), 0); // send the 200 response back
            read(new_socket,data , contentlength);
            write(fd, data, contentlength);
            
            if(Enabledcache)
            {
            //checking if file exists in cache in which case we update else just add in cache
             std::string data_in_string = std::string(data);
            if (cache.find(file) == cache.end()) //if element is not present in cache
           {
                   if(cache.size() >= 4)
                   {
                     cache.erase(cachequeue.front()); 
                     cachequeue.pop_front(); // now remove the first element
                   }
                   // add the file to cache
                   cache.insert({file,data_in_string});
                   cachequeue.push_back(file);
                    // printlog ///////////////////////////////////////////////////////////////
             if (strlen(logf) != 0) {
             printlog("PUT",0,"200", file, contentlength, data,0);
             }

           } 
           else // file is in cache 
           {
                  int index = 0;
                   // search the position of file in deque
                    for (unsigned long i = 0; i < cachequeue.size(); ++i)
                    { 
                        if (cachequeue.at(i) == file)
                        { 
                            index = i;
                        } 
                    } 
                   // find the file in map and delete it 
                   cache.erase(cachequeue.at(index)); 
                   // now remove file in deque 
                   cachequeue.erase(cachequeue.begin() + index -1);
                   // add the updated file to cache
                   cache.insert({file,data_in_string});
                   // add the same file in deque at the end
                   cachequeue.push_back(file);
                    // printlog ///////////////////////////////////////////////////////////////
             if (strlen(logf) != 0) {
             printlog("PUT",0,"200", file, contentlength, data,1);
             }
           }
         }
         else
         {
            if (strlen(logf) != 0) {
             printlog("PUT",0,"200", file, contentlength, data,0);
             } 
         }
           close(fd);
          }
         }
       }
           else // file is not 27 characters 400 reply
           {
            std:: string BADREQUEST = std::string("HTTP/1.1 400 Bad Request\r\n") + std::string("Content-Length: 0\r\n\r\n");
            send(new_socket, BADREQUEST.c_str(), BADREQUEST.length(), 0);
            // printlog ///////////////////////////////////////////////////////////////////
            if (strlen(logf) != 0) {
            printlog("PUT",1,"400", file, 0, data,0);
            }
       }
     }
     
     
       if(method != "PUT" && method != "GET") // internal service error
        {
          std:: string INTERNAL_ERROR = std::string("HTTP/1.1 500 Internal Server Error\r\n") + std::string("Content-Length: 0\r\n\r\n");
          send(new_socket, INTERNAL_ERROR.c_str(), INTERNAL_ERROR.length(), 0);
          char *data = (char*)malloc(0 * sizeof(char));    // the buffer array with size of content length - data that might 
          if (strlen(logf) != 0) {
          printlog(method,1,"500", file, 0, data, 0);
          }
        }
    header.clear(); // clearing the header 
   // close(new_socket);
   }







// main function
int main(int argc, char *argv[]) {
  Enabledcache = 0; 
    int opt;
        while((opt = getopt(argc, argv, "c:l:A:P:")) != -1)  
     {  
        switch(opt)  
        {  
            case 'c':
                break;
            case 'l':
               logf = optarg;
                break;
            case 'A': 
                add = optarg;
                break;
            case 'P': 
                port = atoi(optarg);
                break;  
        }  
   }
   
   for(int i = 0; i<argc;i++)
   {
       if (argv[i] == std::string("-c"))
       {
           Enabledcache = 1; // if there is a -c
       }
   }

    int server_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
    // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    perror("In socket");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  inet_aton( argv[1], &address.sin_addr);
  address.sin_port = htons(port);
   
   // forcefully attaching socket to the port 
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
  {
    perror("In bind");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0)
  {
    perror("In listen");
    exit(EXIT_FAILURE);
  }
    
 
  while(1)
  {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))>=0)
    {
        GetOrPut();
    }
  }
   close(new_socket); // close socket connection
   return 0;
}
