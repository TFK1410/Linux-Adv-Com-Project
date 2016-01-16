extern "C" {
#include "../src/ifconfigurator.h"
}

#include "MockSyscalls.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/netlink.h>

#include <gtest/gtest.h>

using namespace ::testing;

static const struct ifconfig *getIfconfigFromMessage(const void *buf)
{
    return (struct ifconfig *)((char *)buf + NLMSG_HDRLEN);
}

class netlink_ifconfigurator_test : public ::testing::Test
{
protected:
    static const int netlink_fd = 6;
    ifconfigurator *ifc;
    virtual void SetUp()
    {
        SocketSyscallMock socketSyscallMock;
        BindSyscallMock bindSyscallMock;

        // Create ifconfigurator
        EXPECT_FUNCTION_CALL(socketSyscallMock, (AF_NETLINK, _, _))
            .WillOnce(Return(netlink_fd));
        EXPECT_FUNCTION_CALL(bindSyscallMock, (netlink_fd, _, _))
            .WillOnce(Return(0));
        ifc = create_netlink_ifconfigurator();
     }

    virtual void TearDown()
    {
        CloseSyscallMock closeSyscallMock;

        // Destory ifconfigurator
        EXPECT_FUNCTION_CALL(closeSyscallMock, (netlink_fd))
            .WillOnce(Return(0));
        ifc->destroy(ifc);
    }
};

MATCHER_P(SockAddrInMatches, expectedAddr, "")
{
    return arg.sin_family == AF_INET && memcmp(&arg.sin_addr, expectedAddr, sizeof(sockaddr_in::sin_addr)) == 0;
}

MATCHER_P(SockAddrMatches, expectedAddr, "")
{
    return memcmp(&arg, expectedAddr, sizeof(sockaddr_in)) == 0;
}

TEST_F(netlink_ifconfigurator_test, test_get_if_config)
{
    SendToSyscallMock sendToSyscallMock;
    RecvSyscallMock recvSyscallMock;

    // Prepare response
    char responseBuffer[NLMSG_SPACE(sizeof(struct ifconfig))];
    memset(responseBuffer, 0, sizeof(responseBuffer));

    // Put pattern with marked start/end in response buffer
    // to ensure that it won't be modified after being returned
    // from kernel and copied correctly
    char *responseData = (char *)NLMSG_DATA(responseBuffer);
    memset(responseData, 0xBB, sizeof(struct ifconfig));
    responseData[0] = 0xAB;
    responseData[sizeof(struct ifconfig) - 1] = 0xBC;


    // Request interface information
    EXPECT_FUNCTION_CALL(sendToSyscallMock,
                (
                    6, // sockfd
                    ResultOf(getIfconfigFromMessage, Field(&ifconfig::message_type, Eq(LACPM_SHOW))), // buf
                    sizeof(responseBuffer), // len
                    _, _, _ // flags, dest_addr, addrlen
                )
            )
            .WillOnce(ReturnArg<2>());
    EXPECT_FUNCTION_CALL(recvSyscallMock, (6, _, sizeof(responseBuffer), _))
            .WillOnce(DoAll(
                    MemCpyArg<1>(responseBuffer, sizeof(responseBuffer)),
                    ReturnArg<2>()
            ));

    struct ifconfig out_config;
    memset(&out_config, 0xCC, sizeof(out_config));
    ifc->get_if_config(ifc, "if0", &out_config);

    // Assert that data from kernel were returned unaltered
    EXPECT_EQ(0, memcmp(&out_config, responseData, sizeof(struct ifconfig)));
}

TEST_F(netlink_ifconfigurator_test, test_set_ip)
{
    SendToSyscallMock sendToSyscallMock;
    RecvSyscallMock recvSyscallMock;

    // Prepare response
    char responseBuffer[NLMSG_SPACE(sizeof(struct ifconfig))];
    memset(responseBuffer, 0, sizeof(responseBuffer));

    // Request we expect
    struct in_addr newAddr = {};
    memset(&newAddr, 0xA1, sizeof(newAddr));

    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");
    memcpy(&((struct sockaddr_in *) &expectedRequest.ifr_addr)->sin_addr, &newAddr, sizeof(newAddr));

    // Request interface information
    EXPECT_FUNCTION_CALL(sendToSyscallMock,
                (
                    6, // sockfd
                    // buf
                    ResultOf(getIfconfigFromMessage, AllOf(
                        Field(&ifconfig::message_type, Eq(LACPM_SETIP)),
                        Field(&ifconfig::name, StrEq("if0")),
                        Field(&ifconfig::ipv4, SockAddrInMatches(&newAddr))
                    )),
                    sizeof(responseBuffer), // len
                    _, _, _ // flags, dest_addr, addrlen
                )
            )
            .WillOnce(ReturnArg<2>());
    EXPECT_FUNCTION_CALL(recvSyscallMock, (6, _, sizeof(responseBuffer), _))
            .WillOnce(DoAll(
                    MemCpyArg<1>(responseBuffer, sizeof(responseBuffer)),
                    ReturnArg<2>()
            ));
    EXPECT_EQ(true, ifc->set_ip(ifc, "if0", &newAddr));
}

TEST_F(netlink_ifconfigurator_test, test_set_netmask)
{
    SendToSyscallMock sendToSyscallMock;
    RecvSyscallMock recvSyscallMock;

    // Prepare response
    char responseBuffer[NLMSG_SPACE(sizeof(struct ifconfig))];
    memset(responseBuffer, 0, sizeof(responseBuffer));

    // Request we expect
    struct in_addr newAddr = {};
    memset(&newAddr, 0xA1, sizeof(newAddr));

    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");
    memcpy(&((struct sockaddr_in *) &expectedRequest.ifr_addr)->sin_addr, &newAddr, sizeof(newAddr));

    // Request interface information
    EXPECT_FUNCTION_CALL(sendToSyscallMock,
                (
                    6, // sockfd
                    // buf
                    ResultOf(getIfconfigFromMessage, AllOf(
                        Field(&ifconfig::message_type, Eq(LACPM_SETMASK)),
                        Field(&ifconfig::name, StrEq("if0")),
                        Field(&ifconfig::ipv4_netmask, SockAddrInMatches(&newAddr))
                    )),
                    sizeof(responseBuffer), // len
                    _, _, _ // flags, dest_addr, addrlen
                )
            )
            .WillOnce(ReturnArg<2>());
    EXPECT_FUNCTION_CALL(recvSyscallMock, (6, _, sizeof(responseBuffer), _))
            .WillOnce(DoAll(
                    MemCpyArg<1>(responseBuffer, sizeof(responseBuffer)),
                    ReturnArg<2>()
            ));
    EXPECT_EQ(true, ifc->set_net_mask(ifc, "if0", &newAddr));
}

TEST_F(netlink_ifconfigurator_test, test_set_mac)
{
    SendToSyscallMock sendToSyscallMock;
    RecvSyscallMock recvSyscallMock;

    // Prepare response
    char responseBuffer[NLMSG_SPACE(sizeof(struct ifconfig))];
    memset(responseBuffer, 0, sizeof(responseBuffer));

    // Request we expect
    struct sockaddr newAddr = {};
    memset(&newAddr, 0xA1, sizeof(newAddr));
    newAddr.sa_family = AF_INET;

    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");
    memcpy(&((struct sockaddr_in *) &expectedRequest.ifr_addr)->sin_addr, &newAddr, sizeof(newAddr));

    // Request interface information
    EXPECT_FUNCTION_CALL(sendToSyscallMock,
                (
                    6, // sockfd
                    // buf
                    ResultOf(getIfconfigFromMessage, AllOf(
                        Field(&ifconfig::message_type, Eq(LACPM_SETMAC)),
                        Field(&ifconfig::name, StrEq("if0")),
                        Field(&ifconfig::mac, SockAddrMatches(&newAddr))
                    )),
                    sizeof(responseBuffer), // len
                    _, _, _ // flags, dest_addr, addrlen
                )
            )
            .WillOnce(ReturnArg<2>());
    EXPECT_FUNCTION_CALL(recvSyscallMock, (6, _, sizeof(responseBuffer), _))
            .WillOnce(DoAll(
                    MemCpyArg<1>(responseBuffer, sizeof(responseBuffer)),
                    ReturnArg<2>()
            ));
    EXPECT_EQ(true, ifc->set_mac(ifc, "if0", &newAddr));
}
