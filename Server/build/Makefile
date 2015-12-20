# Paths for toolchain

# Set BUILDROOT_PATH to path to your cloned buildroot repo
BUILDROOT_PATH = /home/xubuntu/buildroot

# Set GTEST_PATH to path to your cloned and built Google Test repo
# https://github.com/google/googletest.git
GTEST_PATH = /home/xubuntu/googletest

# Set CMOCK_PATH to path to your cloned C-Mock repo
# https://github.com/hjagodzinski/C-Mock.git
CMOCK_PATH = /home/xubuntu/C-Mock

BUILDROOT_HOST_PATH = $(BUILDROOT_PATH)/output/host
BUILDROOT_CC = $(BUILDROOT_HOST_PATH)/usr/bin/i686-linux-gcc



# Flags common for host and target builds
CFLAGS = -Wall -g -I../module/
LDFLAGS = -lconfig

# Flags for host builds
HOST_CFLAGS =
HOST_LDFLAGS = -lgcov

# Flags for target builds
TARGET_CFLAGS =
TARGET_LDFLAGS =

# Flags for test builds (These will also have target flags)
TEST_CXXFLAGS = -I$(GTEST_PATH)/googletest/include -I$(GTEST_PATH)/googlemock/include -I$(CMOCK_PATH)/include
TEST_LDFLAGS = $(GTEST_PATH)/googletest/lib/.libs/libgtest.a $(GTEST_PATH)/googlemock/lib/.libs/libgmock.a -lpthread -ldl -lgcov



# Source files
SOURCES_FOR_TESTING = client_eh.c reactor.c server_eh.c ifconfigurator.c utils.c
SOURCES = main.c $(SOURCES_FOR_TESTING)

TEST_SOURCES = \
	client_eh_test.cpp \
	ifconfigurator_test.cpp \
	main.cpp \
	MockEventHandler.cpp \
	MockIfconfigurator.cpp \
	MockReactor.cpp \
	MockSyscalls.cpp \
	reactor_test.cpp \
	server_eh_test.cpp


# Target file names
TARGET_NAME = server
HOST_NAME = server_host
TEST_NAME = server_test


# Object files
TARGET_OBJECTS = $(SOURCES:%.c=targetobjs/%.o)
HOST_OBJECTS = $(SOURCES:%.c=hostobjs/%.o)
TEST_OBJECTS = $(SOURCES_FOR_TESTING:%.c=hostcovobjs/%.o) $(TEST_SOURCES:%.cpp=testobjs/%.o)

# Default target
all: $(TARGET_NAME) $(HOST_NAME) $(TEST_NAME)


# Build executables
$(TARGET_NAME): $(TARGET_OBJECTS)
	$(BUILDROOT_CC) $^ $(LDFLAGS) $(TARGET_LDFLAGS) -o $@

$(HOST_NAME): $(HOST_OBJECTS)
	$(CC) $^ $(LDFLAGS) $(HOST_LDFLAGS) -o $@

$(TEST_NAME): $(TEST_OBJECTS)
	$(CXX) $^ $(LDFLAGS) $(HOST_LDFLAGS) $(TEST_LDFLAGS) -o $@


# Build objects
targetobjs/%.o: ../src/%.c
	mkdir -p targetobjs
	$(BUILDROOT_CC) -c $(CFLAGS) $(TARGET_CFLAGS) $< -o $@

hostobjs/%.o: ../src/%.c
	mkdir -p hostobjs
	$(CC) -c $(CFLAGS) $(HOST_CFLAGS) $< -o $@

hostcovobjs/%.o: ../src/%.c
	mkdir -p hostcovobjs
	$(CC) -c $(CFLAGS) -fprofile-arcs -ftest-coverage $(HOST_CFLAGS) $< -o $@

testobjs/%.o: ../test/%.cpp
	mkdir -p testobjs
	$(CXX) -c $(CFLAGS) $(HOST_CFLAGS) $(TEST_CXXFLAGS) $< -o $@


# Testing
test: $(TEST_NAME)
	./$(TEST_NAME)

coverage: test
	mkdir -p coverage
	lcov -b hostcovobjs -d ../.. -c -o coverage/coverage.info
	lcov -r coverage/coverage.info /usr/\* tst/* -o coverage/coverage.info
	genhtml coverage/coverage.info --output-directory coverage/html/ --ignore-errors source

# Clean
clean:
	-rm -rf hostobjs
	-rm -rf hostcovobjs
	-rm -rf targetobjs
	-rm -rf testobjs
	-rm -rf coverage
	-rm -f $(TARGET_NAME)
	make -C ../module/ clean


.PHONY: test coverage clean
