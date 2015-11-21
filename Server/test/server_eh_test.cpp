extern "C" {
#include "../src/server_eh.h"
}
#include "MockReactor.h"

using namespace ::testing;

TEST(server_eh_test, construct_destruct_dont_add_events)
{
    MockReactor reactor;

    EXPECT_CALL(reactor, add_eh(_))
        .Times(0);
    EXPECT_CALL(reactor, rm_eh(_))
        .Times(0);

    event_handler *server = create_server_eh(reactor.getStruct(), 5533, 10);
    server->destroy(server);
}
