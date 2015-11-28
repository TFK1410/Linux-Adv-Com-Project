extern "C" {
#include "../src/reactor.h"
}

#include "MockEventHandler.h"
#include "MockSyscalls.h"

#include <errno.h>

#include <gtest/gtest.h>

using namespace ::testing;

TEST(reactor_test, handle_then_rm_eh)
{
    MockEventHandler eh;
    EpollCreateSyscallMock mockEpollCreate;
    EpollCtlSyscallMock mockEpollCtl;
    EpollWaitSyscallMock mockEpollWait;
    CloseSyscallMock mockClose;

    struct epoll_event event;
    
    EXPECT_CALL(eh, get_fd()).WillRepeatedly(Return(4));

    // Create reactor
    EXPECT_FUNCTION_CALL(mockEpollCreate, (_)).WillOnce(Return(7));

    struct reactor *reactor = create_reactor(5);

    // Register event handler
    EXPECT_FUNCTION_CALL(mockEpollCtl, (7, EPOLL_CTL_ADD, 4, _))
        .WillOnce(DoAll(
            SaveArgPointee<3>(&event),
            Return(0)
        ));

    EXPECT_TRUE(reactor->add_eh(reactor, eh.getStruct()));

    event.events = EPOLLIN;

    // Run event loop
    EXPECT_FUNCTION_CALL(mockEpollWait, (7, _, _, -1))
        .WillOnce(DoAll(
            SetArgPointee<1>(event),
            Return(1)
        ))
        .WillOnce(
            SetErrnoAndReturn(-EAGAIN, -1) // Cause error that doesn't exit loop
        )
        .WillOnce(
            SetErrnoAndReturn(-EBADF, -1) // Fail in order to leave event loop
        );
    EXPECT_CALL(eh, handle_event(EPOLLIN)).WillOnce(Return(false));

    reactor->event_loop(reactor);

    // Remove handler
    EXPECT_FUNCTION_CALL(mockEpollCtl, (7, EPOLL_CTL_DEL, 4, _))
        .WillOnce(Return(0));
    reactor->rm_eh(reactor, eh.getStruct());

    // Destroy reactor
    EXPECT_FUNCTION_CALL(mockClose, (7)).WillOnce(Return(0));
    reactor->destroy(reactor);
}

TEST(reactor_test, handle_and_rm_via_return)
{
    MockEventHandler eh;
    EpollCreateSyscallMock mockEpollCreate;
    EpollCtlSyscallMock mockEpollCtl;
    EpollWaitSyscallMock mockEpollWait;
    CloseSyscallMock mockClose;

    struct epoll_event event;
    
    EXPECT_CALL(eh, get_fd()).WillRepeatedly(Return(4));

    // Create reactor
    EXPECT_FUNCTION_CALL(mockEpollCreate, (_)).WillOnce(Return(7));

    struct reactor *reactor = create_reactor(5);

    // Register event handler
    EXPECT_FUNCTION_CALL(mockEpollCtl, (7, EPOLL_CTL_ADD, 4, _))
        .WillOnce(DoAll(
            SaveArgPointee<3>(&event),
            Return(0)
        ));

    EXPECT_TRUE(reactor->add_eh(reactor, eh.getStruct()));

    event.events = EPOLLIN;

    // Run event loop
    EXPECT_FUNCTION_CALL(mockEpollWait, (7, _, _, -1))
        .WillOnce(DoAll(
            SetArgPointee<1>(event),
            Return(1)
        ))
        .WillOnce(
            SetErrnoAndReturn(-EBADF, -1) // Fail in order to leave event loop 
        );
    EXPECT_FUNCTION_CALL(mockEpollCtl, (7, EPOLL_CTL_DEL, 4, _))
        .WillOnce(Return(0));
    EXPECT_CALL(eh, handle_event(EPOLLIN)).WillOnce(Return(true));
    EXPECT_CALL(eh, destroy()).Times(1);

    reactor->event_loop(reactor);

    // Destroy reactor
    EXPECT_FUNCTION_CALL(mockClose, (7)).WillOnce(Return(0));
    reactor->destroy(reactor);
}
