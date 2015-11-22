#ifndef MOCK_SYSCALLS_H
#define MOCK_SYSCALLS_H

extern "C" {
#include <unistd.h>
}

#include <cmock/cmock.h>

DECLARE_FUNCTION_MOCK3(ReadSyscallMock, read, ssize_t(int, void *, size_t)); 
ACTION_P2(MockedRead, data, len)
{
    memcpy(arg1, data, len);
    return len;
}

DECLARE_FUNCTION_MOCK3(WriteSyscallMock, write, ssize_t(int, const void *, size_t)); 
ACTION_P2(MockedWrite, data, len)
{
    EXPECT_EQ(len, arg2);
    EXPECT_EQ(0, memcmp(arg1, data, len))
        << "Expected written text: " << (const char *) data << "\n"
        << "Actual written text: " << (const char *) arg1;
    return arg2;
}

DECLARE_FUNCTION_MOCK1(CloseSyscallMock, close, int(int)); 

#endif
