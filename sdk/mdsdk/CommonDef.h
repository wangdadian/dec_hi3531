#ifndef COMMON_DEFINE_H_
#define  COMMON_DEFINE_H_
#ifdef _WIN32

#define THREADHANDLE HANDLE

#define DEBUG_( info ) \
{\
	char *szOutInfo = new char[strlen(info) + 1024 ]; \
	sprintf( szOutInfo, "[DEBUG:%s] %s:%d\n", info, __FILE__, __LINE__ ); \
OutputDebugString( szOutInfo );  \
delete []szOutInfo;}

#define ERROR_( info ) \
{\
	char *szOutInfo = new char[strlen(info) + 1024 ]; \
	sprintf( szOutInfo, "[ERROR:%s] %s:%d\n", info, __FILE__, __LINE__ ); \
	OutputDebugString( szOutInfo ); \
delete []szOutInfo;}
 


#define FATAL_( info ) \
{\
	char *szOutInfo = new char[strlen(info) + 1024 ]; \
	sprintf( szOutInfo, "[FATAL:%s] %s:%d\n", info, __FILE__, __LINE__ ); \
	OutputDebugString( szOutInfo );  \
delete []szOutInfo; }


#define WARN_( info ) \
	{\
	char *szOutInfo = new char[strlen(info) + 1024 ]; \
	sprintf( szOutInfo, "[WARN:%s] %s:%d\n", info, __FILE__, __LINE__ ); \
	OutputDebugString( szOutInfo );  \
delete []szOutInfo; }


#define INFO_( info ) \
	{ \
	char *szOutInfo = new char[strlen(info) + 1024 ]; \
	sprintf( szOutInfo, "[INFO:%s] %s:%d\n", info, __FILE__, __LINE__ ); \
	OutputDebugString( szOutInfo );  \
	delete []szOutInfo; }
#endif

#endif 