
#ifndef NEW_DELETE_CONFIG_HEADER_
#define NEW_DELETE_CONFIG_HEADER_

#ifdef _MSC_VER
#  pragma once
#endif

/**
use custom new/delete or standard? (for speed testing)
*/
#ifndef OP_NEW_USE_WINNIE_ALLOC
#define OP_NEW_USE_WINNIE_ALLOC 1
#endif

/**
if you use multithreading, <b> DO NOT FORGET TO CALL </b> OperatorNewCriticalSectionInit before
creating any second thread. (for example, at start of WinMain function)
@see OperatorNewCriticalSectionInit */
#define OP_NEW_MULTITHREADING 0


#ifdef NDEBUG
#define OP_NEW_NDEBUG_NOT_ENABLED 0
#else
#define OP_NEW_NDEBUG_NOT_ENABLED 1
#endif

/**
Guards at the begin and the end of each block,
filling with 0xCCCCCCCC in `new`, and with 0xDDDDDDDD in `delete`,
catching "delete new char[10]" error (should be "delete[] new char[10]"),
invalid usage in multithread enviroment,
also add information header DebugHeader to each block,
which can retrivied with function OpNewGetHeader,
also make available many debug functions, like OpNewValidateHeap(see "new_delete.h").
By default, if defined NDEBUG, then OP_NEW_DEBUG defined as 0, otherwise OP_NEW_DEBUG defined as 1
You can force this value to 1, to set debug mode for new/delete in Release project mode
*/
#define OP_NEW_DEBUG OP_NEW_NDEBUG_NOT_ENABLED

#if OP_NEW_DEBUG
/**
assert in debug mode. Raised, only when OP_NEW_DEBUG is non-zero
*/
#define OP_NEW_ASSERT(e) assert(e)
#endif

#endif
