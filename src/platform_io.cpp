#include "platform_io.h"

#ifdef _WIN32
#include<io.h>
#include<fcntl.h>



int open_file_append(const std::string& path){
    return _open(path.c_str(),
             _O_CREAT | _O_BINARY | _O_APPEND | _O_RDWR,
             0666);

}

int open_file_readonly(const std::string& path){
    return _open(path.c_str(), _O_BINARY | _O_RDONLY);
}


void write_file(int fd,const char* data, size_t size){
    _write(fd, data, (unsigned)size);
}

size_t read_file(int fd,char* data,size_t size){
    return _read(fd,data,(unsigned)size);
}

void fsync_file(int fd){
    _commit(fd);
}

void close_file(int fd){
    _close(fd);
}

#else 
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>

int open_file_append(const std::string& path){
    return open(path.c_str(), O_CREAT | O_RDWR | O_APPEND, 0644);
}

int open_file_readonly(const std:: string& path){
    return open(path.c_str(), O_RDONLY);
}

void write_file(int fs, const char* data, size_t size){
    ::write(fc,data,size);
}

size_t read_file(int fd,char* data,size_t size){
    return ::read(fd,data,size);
}

void fsync_file(int fd){
    ::fsync(fd);
}

void close_file(int fd){
    ::close(fd);
}
#endif