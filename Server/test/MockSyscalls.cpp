#include "MockSyscalls.h"

IMPLEMENT_FUNCTION_MOCK3(ReadSyscallMock, read, ssize_t(int, void *, size_t)); 
IMPLEMENT_FUNCTION_MOCK3(WriteSyscallMock, write, ssize_t(int, const void *, size_t)); 
IMPLEMENT_FUNCTION_MOCK1(CloseSyscallMock, close, int(int)); 
