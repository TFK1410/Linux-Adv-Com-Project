extern "C" {
#include "../src/client_eh.h"
}

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockSyscalls.h"
#include "MockIfconfigurator.h"
#include <net/if_arp.h>

using namespace ::testing;


TEST(client_eh_test, test_show)
{
    // Input text
    const char *incomingMessage = "show if0";
    int incomingMessageLen = strlen(incomingMessage);

    // Expected output
    const char *outcomingMessage =
        "Interface name: if0\n"
        "Status: DOWN\n"
        "IPv4 Address: 0.0.0.0\n"
        "Network Mask: 0.0.0.0\n"
        "MAC: 00:00:00:00:00:00\n"
        "IPV6: 0000:0000:0000:0000:0000:0000:0000:0000\n";

    int outcomingMessageLen = strlen(outcomingMessage);

    // Interface configuration
    struct ifconfig config = {};

    // Mocks
    MockIfconfigurator mockIfconfigurator;
    ReadSyscallMock readMock;
    WriteSyscallMock writeMock;
    CloseSyscallMock closeMock;
    {
        EXPECT_FUNCTION_CALL(readMock, (9, _, Gt(incomingMessageLen)))
            .WillOnce(MockedRead(incomingMessage, incomingMessageLen));
        EXPECT_CALL(mockIfconfigurator, get_if_config(StrEq("if0"), _))
            .WillOnce(
                DoAll(
                    SetArgPointee<1>(config),
                    Return(true)
                )
            );
        EXPECT_FUNCTION_CALL(writeMock, (9, _, _))
            .WillOnce(MockedWrite(outcomingMessage, outcomingMessageLen));
    }

    // Test logic
    struct event_handler *eh = create_client_eh(9, mockIfconfigurator.getStruct());
    eh->handle_event(eh, EPOLLIN);
    EXPECT_FUNCTION_CALL(closeMock, (9)).WillOnce(Return(0));
    eh->destroy(eh);
}

TEST(client_eh_test, test_show_two)
{
    // Input text
    const char *incomingMessage = "show if0 if1 ";
    int incomingMessageLen = strlen(incomingMessage);

    // Expected output
    const char *outcomingMessage1 = "No device with name if0 found!\n";
    const char *outcomingMessage2 = "No device with name if1 found!\n";
    int outcomingMessageLen = strlen(outcomingMessage1);

    // Mocks
    MockIfconfigurator mockIfconfigurator;
    ReadSyscallMock readMock;
    WriteSyscallMock writeMock;
    CloseSyscallMock closeMock;
    {
        EXPECT_FUNCTION_CALL(readMock, (9, _, Gt(incomingMessageLen)))
            .WillOnce(MockedRead(incomingMessage, incomingMessageLen));
        EXPECT_CALL(mockIfconfigurator, get_if_config(StrEq("if0"), _))
            .WillOnce(Return(false));
        EXPECT_CALL(mockIfconfigurator, get_if_config(StrEq("if1"), _))
            .WillOnce(Return(false));
        EXPECT_FUNCTION_CALL(writeMock, (9, _, _))
            .WillOnce(MockedWrite(outcomingMessage1, outcomingMessageLen))
            .WillOnce(MockedWrite(outcomingMessage2, outcomingMessageLen));
    }

    // Test logic
    struct event_handler *eh = create_client_eh(9, mockIfconfigurator.getStruct());
    eh->handle_event(eh, EPOLLIN);
    EXPECT_FUNCTION_CALL(closeMock, (9)).WillOnce(Return(0));
    eh->destroy(eh);
}

TEST(client_eh_test, test_set_ip)
{
    // Input text
    const char *incomingMessage = "setip if0 1.2.3.4/5";
    int incomingMessageLen = strlen(incomingMessage);

    // Expected output
    const char *outcomingMessage = "Successfully changed ip of the interface if0 to 1.2.3.4 and mask 248.0.0.0\n";
    int outcomingMessageLen = strlen(outcomingMessage);

    // Mocks
    MockIfconfigurator mockIfconfigurator;
    ReadSyscallMock readMock;
    WriteSyscallMock writeMock;
    CloseSyscallMock closeMock;
    {
        EXPECT_FUNCTION_CALL(readMock, (9, _, Gt(incomingMessageLen)))
            .WillOnce(MockedRead(incomingMessage, incomingMessageLen));
        EXPECT_CALL(mockIfconfigurator, set_ip(StrEq("if0"), Field(&in_addr::s_addr, htonl(0x01020304))))
            .WillOnce(Return(true));
        EXPECT_CALL(mockIfconfigurator, set_net_mask(StrEq("if0"), Field(&in_addr::s_addr, htonl(0xf8000000))))
            .WillOnce(Return(true));
        EXPECT_FUNCTION_CALL(writeMock, (9, _, _))
            .WillOnce(MockedWrite(outcomingMessage, outcomingMessageLen));
    }

    // Test logic
    struct event_handler *eh = create_client_eh(9, mockIfconfigurator.getStruct());
    eh->handle_event(eh, EPOLLIN);
    EXPECT_FUNCTION_CALL(closeMock, (9)).WillOnce(Return(0));
    eh->destroy(eh);
}

TEST(client_eh_test, test_set_mac)
{
    // Input text
    const char *incomingMessage = "setmac if0 12:34:56:AB:cd:EF";
    int incomingMessageLen = strlen(incomingMessage);

    // Expected output
    const char *outcomingMessage = "Successfully changed mac address of the interface if0 to 12:34:56:AB:cd:EF\n";
    int outcomingMessageLen = strlen(outcomingMessage);

    // Mocks
    MockIfconfigurator mockIfconfigurator;
    ReadSyscallMock readMock;
    WriteSyscallMock writeMock;
    CloseSyscallMock closeMock;
    {
        EXPECT_FUNCTION_CALL(readMock, (9, _, Gt(incomingMessageLen)))
            .WillOnce(MockedRead(incomingMessage, incomingMessageLen));
        EXPECT_CALL(mockIfconfigurator, set_mac(StrEq("if0"), Field(&sockaddr::sa_family, ARPHRD_ETHER)))
            .WillOnce(Return(true));
        EXPECT_FUNCTION_CALL(writeMock, (9, _, _))
            .WillOnce(MockedWrite(outcomingMessage, outcomingMessageLen));
    }

    // Test logic
    struct event_handler *eh = create_client_eh(9, mockIfconfigurator.getStruct());
    eh->handle_event(eh, EPOLLIN);
    EXPECT_FUNCTION_CALL(closeMock, (9)).WillOnce(Return(0));
    eh->destroy(eh);
}
