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
        "IfInfo"
        "=ifname=if0"
        "=status=DOWN"
        "=ipv4=0.0.0.0"
        "=netmask=0.0.0.0"
        "=mac=00:00:00:00:00:00"
        "=ipv6=0000:0000:0000:0000:0000:0000:0000:0000"
        "\n";

    int outcomingMessageLen = strlen(outcomingMessage);
    const char *outcomingEndMessage = "EndOfList\n";
    int outcomingEndMessageLen = strlen(outcomingEndMessage);

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
            .WillOnce(MockedWrite(outcomingMessage, outcomingMessageLen))
            .WillOnce(MockedWrite(outcomingEndMessage, outcomingEndMessageLen));
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
    const char *outcomingMessage1 = "Error=name=noiffound=ifname=if0\n";
    const char *outcomingMessage2 = "Error=name=noiffound=ifname=if1\n";
    int outcomingMessageLen = strlen(outcomingMessage1);
    const char *outcomingEndMessage = "EndOfList\n";
    int outcomingEndMessageLen = strlen(outcomingEndMessage);

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
            .WillOnce(MockedWrite(outcomingMessage2, outcomingMessageLen))
            .WillOnce(MockedWrite(outcomingEndMessage, outcomingEndMessageLen));
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
    const char *outcomingMessage = "SetIpOkay\n";
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
    const char *outcomingMessage = "SetMacOkay\n";
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
