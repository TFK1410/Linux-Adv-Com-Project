extern "C" {
#include "../src/ifconfigurator.h"
}

#include "MockSyscalls.h"

#include <sys/ioctl.h>
#include <net/if.h>

#include <gtest/gtest.h>

using namespace ::testing;

static FILE *fixtureFopen(const char *, const char *) {
    return FopenCallMock::real("../test/if_inet6_fixture.txt", "r");
}

TEST(ifconfigurator_test, test_get_if_config)
{
    SocketSyscallMock socketSyscallMock;
    IoctlSyscallMock ioctlSyscallMock;
    FopenCallMock fopenCallMock;
    CloseSyscallMock closeSyscallMock;

    // Create ifconfigurator
    EXPECT_FUNCTION_CALL(socketSyscallMock, (AF_INET, _, _))
        .WillOnce(Return(6));
    ifconfigurator *ifc = create_ifconfigurator();

    // Request we expect
    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");

    // Return values
    struct ifreq flagsResult = expectedRequest;
    flagsResult.ifr_flags = IFF_RUNNING;

    struct ifreq hwAddrResult = expectedRequest;
    struct ifreq addrResult = expectedRequest;
    struct ifreq brdAddrResult = expectedRequest;
    struct ifreq netmaskResult = expectedRequest;

    // Request interface information
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCGIFFLAGS, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                MemCpyArg<2>(&flagsResult, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCGIFHWADDR, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                MemCpyArg<2>(&hwAddrResult, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCGIFADDR, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                MemCpyArg<2>(&addrResult, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCGIFBRDADDR, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                MemCpyArg<2>(&brdAddrResult, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCGIFNETMASK, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                MemCpyArg<2>(&netmaskResult, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_FUNCTION_CALL(fopenCallMock, (StrEq("/proc/net/if_inet6"), StrEq("r")))
        .WillOnce(Invoke(fixtureFopen));

    struct ifconfig out_config;
    memset(&out_config, 0xCC, sizeof(out_config));
    ifc->get_if_config(ifc, "if0", &out_config);

    EXPECT_EQ(IFF_RUNNING, out_config.flags);
    EXPECT_EQ(0, memcmp(out_config.ipv6, "\xfe\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x30\x5f", 16));

    // Destory ifconfigurator
    EXPECT_FUNCTION_CALL(closeSyscallMock, (6))
        .WillOnce(Return(0));
    ifc->destroy(ifc);
}

TEST(ifconfigurator_test, test_set_ip)
{
    SocketSyscallMock socketSyscallMock;
    IoctlSyscallMock ioctlSyscallMock;
    CloseSyscallMock closeSyscallMock;

    // Create ifconfigurator
    EXPECT_FUNCTION_CALL(socketSyscallMock, (AF_INET, _, _))
        .WillOnce(Return(6));
    ifconfigurator *ifc = create_ifconfigurator();

    // Request we expect
    struct in_addr newAddr = {};
    memset(&newAddr, 0xA1, sizeof(newAddr));

    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");
    memcpy(&((struct sockaddr_in *) &expectedRequest.ifr_addr)->sin_addr, &newAddr, sizeof(newAddr));

    // Request interface information
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCSIFADDR, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_EQ(true, ifc->set_ip(ifc, "if0", &newAddr));

    // Destory ifconfigurator
    EXPECT_FUNCTION_CALL(closeSyscallMock, (6))
        .WillOnce(Return(0));
    ifc->destroy(ifc);
}

TEST(ifconfigurator_test, test_set_netmask)
{
    SocketSyscallMock socketSyscallMock;
    IoctlSyscallMock ioctlSyscallMock;
    CloseSyscallMock closeSyscallMock;

    // Create ifconfigurator
    EXPECT_FUNCTION_CALL(socketSyscallMock, (AF_INET, _, _))
        .WillOnce(Return(6));
    ifconfigurator *ifc = create_ifconfigurator();

    // Request we expect
    struct in_addr newAddr = {};
    memset(&newAddr, 0xA1, sizeof(newAddr));

    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");
    memcpy(&((struct sockaddr_in *) &expectedRequest.ifr_netmask)->sin_addr, &newAddr, sizeof(newAddr));

    // Request interface information
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCSIFNETMASK, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_EQ(true, ifc->set_net_mask(ifc, "if0", &newAddr));

    // Destory ifconfigurator
    EXPECT_FUNCTION_CALL(closeSyscallMock, (6))
        .WillOnce(Return(0));
    ifc->destroy(ifc);
}

TEST(ifconfigurator_test, test_set_mac)
{
    SocketSyscallMock socketSyscallMock;
    IoctlSyscallMock ioctlSyscallMock;
    CloseSyscallMock closeSyscallMock;

    // Create ifconfigurator
    EXPECT_FUNCTION_CALL(socketSyscallMock, (AF_INET, _, _))
        .WillOnce(Return(6));
    ifconfigurator *ifc = create_ifconfigurator();

    // Request we expect
    struct sockaddr newAddr = {};
    memset(&newAddr, 0xA1, sizeof(newAddr));

    struct ifreq expectedRequest = {};
    expectedRequest.ifr_addr.sa_family = AF_INET;
    strcpy(expectedRequest.ifr_name, "if0");
    memcpy(&expectedRequest.ifr_hwaddr, &newAddr, sizeof(newAddr));

    // Request interface information
    EXPECT_FUNCTION_CALL(ioctlSyscallMock, (6, SIOCSIFHWADDR, _))
        .WillOnce(DoAll(
                MemCmpArg<2>(&expectedRequest, sizeof(struct ifreq)),
                Return(0)
        ));
    EXPECT_EQ(true, ifc->set_mac(ifc, "if0", &newAddr));

    // Destory ifconfigurator
    EXPECT_FUNCTION_CALL(closeSyscallMock, (6))
        .WillOnce(Return(0));
    ifc->destroy(ifc);
}

