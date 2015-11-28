extern "C" {
#include "../src/server_eh.h"
#include "../src/ifconfigurator.h"
}
#include "MockReactor.h"
#include "MockEventHandler.h"
#include "MockSyscalls.h"
#include "cmock/cmock.h"

using namespace ::testing;

// C-Mock requires method existing in library so cannot be used here
class MockCreateClientEh {
public:
    MOCK_METHOD2(create_client_eh, event_handler* (int, ifconfigurator*));
};
static MockCreateClientEh *gMockCreateClientEh = NULL;

static event_handler *create_client_eh_mock(int cli_fd, ifconfigurator *ifc)
{
    return gMockCreateClientEh->create_client_eh(cli_fd, ifc);
}



TEST(server_eh_test, test_lifecycle)
{
    MockReactor mockReactor;
    MockCreateClientEh mockCreateClientEh;
    gMockCreateClientEh = &mockCreateClientEh;

    // Mock syscalls
    SocketSyscallMock socketMock;
    BindSyscallMock bindMock;
    ListenSyscallMock listenMock;
    AcceptSyscallMock acceptMock;

    // This unit shouldn't dereference these, cause SEGV if it tries
    ifconfigurator *dummyIfconfigurator = reinterpret_cast<ifconfigurator *>(100);
    event_handler *dummyClientEh = reinterpret_cast<event_handler *>(200);


    // Create server
    EXPECT_FUNCTION_CALL(socketMock, (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, _))
        .WillOnce(Return(8));
    EXPECT_FUNCTION_CALL(bindMock, (8, _, _))
        .WillOnce(Return(0));
    EXPECT_FUNCTION_CALL(listenMock, (8, _))
        .WillOnce(Return(0));
    event_handler *server = create_server_eh(mockReactor.getStruct(), 5533, dummyIfconfigurator, create_client_eh_mock);

    // Verify server's fd
    EXPECT_EQ(8, server->get_fd(server));

    // Accept new connection
    EXPECT_FUNCTION_CALL(acceptMock, (8, NULL, 0))
        .WillOnce(Return(13));
    EXPECT_CALL(mockCreateClientEh, create_client_eh(13, dummyIfconfigurator))
        .WillOnce(Return(dummyClientEh));
    EXPECT_CALL(mockReactor, add_eh(dummyClientEh))
        .WillOnce(Return(true));
    server->handle_event(server, EPOLLIN);

    // Destroy server
    server->destroy(server);
    gMockCreateClientEh = NULL;
}
