#pragma once


#if !defined(SQLITE3_API)
#if defined(_MSC_VER)
#define SQLITE3_API __declspec(dllimport)
#else
#define SQLITE3_API
#endif
#endif


#define SQLITE3_MAX_USER_DATA_SIZE 144