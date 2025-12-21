#pragma once
#include<string>
#include<cstdint>

int open_file_append(const std::string& path);
int open_file_readonly(const std::string& path);
void write_file(int fd, const char* data, size_t size);
size_t read_file(int fd, char* data, size_t size);
void fsync_file(int fd);
void close_file(int fd);