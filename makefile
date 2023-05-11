# Makefile

# Directories
SRCDIR := src
BUILDDIR := build

# Source files
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

# Targets
TARGET := $(BUILDDIR)/net367
DEPS := $(OBJS:.o=.d)

# Compiler settings
CC := gcc
CFLAGS := -g
LDFLAGS :=

# Rules
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

clean:
	$(RM) $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

