TARGET ?= libledgersigner.so
SRC_DIRS ?= ./

SRCS := ledger_signer.c device_apdu_commands.c  
OBJS := $(addsuffix .o,$(basename $(SRCS)))
DEPS := $(OBJS:.o=.d)
LDFLAGS := -shared
LDLIBS := -lhidapi

INC_FLAGS := -I/usr/local/include/hidapi -I./

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)