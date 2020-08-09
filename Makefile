CXX = g++
CXXFLAGS = -g -O0 -I include -I src
LDFLAGS = -lfmt -pthread

TARGET = kthxvm

SRCS = \
src/main.cpp \

HDRS = \
$(wildcard src/kvm/virtio/*.h) \
$(wildcard src/kvm/device/*.h) \
$(wildcard src/kvm/*.h) \
$(wildcard src/elf/*.h) \

OBJS = $(addsuffix .o, build/$(basename $(SRCS)))

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

build/src:
	@mkdir -p build/src

$(OBJS): $(SRCS) $(HDRS) build/src
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS)
