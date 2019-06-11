#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//#define _CRT_SECURE_NO_WARNINGS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <direct.h>
#include <io.h>

#define getcwd _getcwd
#define mkdir(a, b) _mkdir(a)

#ifndef S_IRWXU
#define S_IRWXU 0
#endif

#else
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pwd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>
//#include "net/stat.h"
#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
