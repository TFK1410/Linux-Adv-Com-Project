# Paths for toolchain

# Set BUILDROOT_PATH to path to your cloned buildroot repo
# BUILDROOT_PATH = /home/xubuntu/buildroot

# Set GTEST_PATH to path to your cloned and built Google Test repo
# https://github.com/google/googletest.git
# GTEST_PATH = /home/xubuntu/googletest

BUILDROOT_HOST_PATH = $(BUILDROOT_PATH)/output/host
BUILDROOT_CC = $(BUILDROOT_HOST_PATH)/usr/bin/i686-linux-gcc



# Flags common for host and target builds
CFLAGS = -Wall -g
LDFLAGS = -lconfig

# Flags for host builds
HOST_CFLAGS =
HOST_LDFLAGS =

# Flags for target builds
TARGET_CFLAGS =
TARGET_LDFLAGS =

# Flags for test builds (These will also have target flags)
TEST_CXXFLAGS = -I$(GTEST_PATH)/googletest/include -I$(GTEST_PATH)/googlemock/include
TEST_LDFLAGS = $(GTEST_PATH)/googletest/libgtest.a $(GTEST_PATH)/googlemock/libgmock.a -lpthread



# Source files
SOURCES_FOR_TESTING = client_eh.c eh_list.c reactor.c server_eh.c
SOURCES = main.c $(SOURCES_FOR_TESTING)

TEST_SOURCES = main.cpp MockReactor.cpp server_eh_test.cpp


# Target file names
TARGET_NAME = server
HOST_NAME = server_host
TEST_NAME = server_test


# Object files
TARGET_OBJECTS = $(SOURCES:%.c=targetobjs/%.o)
HOST_OBJECTS = $(SOURCES:%.c=hostobjs/%.o)
TEST_OBJECTS = $(SOURCES_FOR_TESTING:%.c=hostobjs/%.o) $(TEST_SOURCES:%.cpp=testobjs/%.o)



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

testobjs/%.o: ../test/%.cpp
	mkdir -p testobjs
	$(CXX) -c $(CFLAGS) $(HOST_CFLAGS) $(TEST_CXXFLAGS) $< -o $@


# Clean
clean:
	-rm -rf hostobjs
	-rm -rf targetobjs
	-rm -rf testobjs
	-rm -f $(TARGET_NAME)
