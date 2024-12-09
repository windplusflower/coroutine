#ifndef HOOK_H
#define HOOK_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
ssize_t co_read(int fd, void *buf, size_t nbyte);
ssize_t co_write(int fd, const void *buf, size_t nbyte);
#endif