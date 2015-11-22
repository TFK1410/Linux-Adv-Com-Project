#include "MockSyscalls.h"

IMPLEMENT_FUNCTION_MOCK3(ReadSyscallMock, read, ssize_t(int, void *, size_t)); 
IMPLEMENT_FUNCTION_MOCK3(WriteSyscallMock, write, ssize_t(int, const void *, size_t)); 
IMPLEMENT_FUNCTION_MOCK1(CloseSyscallMock, close, int(int)); 
IMPLEMENT_FUNCTION_MOCK1(EpollCreateSyscallMock, epoll_create, int(int)); 
IMPLEMENT_FUNCTION_MOCK4(EpollCtlSyscallMock, epoll_ctl, int(int, int, int, struct epoll_event *)); 
IMPLEMENT_FUNCTION_MOCK4(EpollWaitSyscallMock, epoll_wait, int(int, struct epoll_event *, int, int)); 
