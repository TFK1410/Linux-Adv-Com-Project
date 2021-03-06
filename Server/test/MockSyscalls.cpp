#include "MockSyscalls.h"

IMPLEMENT_FUNCTION_MOCK3(ReadSyscallMock, read, ssize_t(int, void *, size_t));
IMPLEMENT_FUNCTION_MOCK3(WriteSyscallMock, write, ssize_t(int, const void *, size_t));
IMPLEMENT_FUNCTION_MOCK1(CloseSyscallMock, close, int(int));
IMPLEMENT_FUNCTION_MOCK1(EpollCreateSyscallMock, epoll_create, int(int));
IMPLEMENT_FUNCTION_MOCK4(EpollCtlSyscallMock, epoll_ctl, int(int, int, int, struct epoll_event *));
IMPLEMENT_FUNCTION_MOCK4(EpollWaitSyscallMock, epoll_wait, int(int, struct epoll_event *, int, int));
IMPLEMENT_FUNCTION_MOCK3(SocketSyscallMock, socket, int(int, int, int));
IMPLEMENT_FUNCTION_MOCK3(BindSyscallMock, bind, int(int, const struct sockaddr *, socklen_t));
IMPLEMENT_FUNCTION_MOCK2(ListenSyscallMock, listen, int(int, int));
IMPLEMENT_FUNCTION_MOCK3(AcceptSyscallMock, accept, int(int, struct sockaddr *, socklen_t *));
IMPLEMENT_FUNCTION_MOCK3(IoctlSyscallMock, ioctl, int(int, long, void *));
IMPLEMENT_FUNCTION_MOCK2(FopenCallMock, fopen, FILE* (const char *, const char *));
IMPLEMENT_FUNCTION_MOCK6(SendToSyscallMock, sendto, ssize_t(int, const void *, size_t, int, const struct sockaddr *, socklen_t));
IMPLEMENT_FUNCTION_MOCK4(RecvSyscallMock, recv, ssize_t(int, void *, size_t, int));

