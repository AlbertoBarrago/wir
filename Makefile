# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -Isrc
DEBUGFLAGS = -g -DDEBUG
LDFLAGS =

# Detect platform
UNAME_S := $(shell uname -s)

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)
    # macOS
    PLATFORM = macOS
else ifeq ($(UNAME_S),Linux)
    # Linux
    PLATFORM = Linux
    # Expose POSIX symbols (pid_t, strdup, kill, usleep, ...) that -std=c11
    # hides under glibc's __STRICT_ANSI__. Not needed (and harmful) on macOS,
    # where these macros would instead hide the BSD types the SDK headers use.
    CFLAGS += -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
else
    $(error Unsupported platform: $(UNAME_S))
endif

# Source files
SRCDIR = src
SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/args.c \
          $(SRCDIR)/utils.c \
          $(SRCDIR)/platform.c \
          $(SRCDIR)/output.c

# Object files
OBJDIR = obj
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Output binary
TARGET = wir

# Default target
.PHONY: all
all: $(TARGET)
	@echo "Build complete for $(PLATFORM)"
	@echo "Run './$(TARGET) --help' for usage information"

# Create object directory
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# Debug build
# Use separate sub-make invocations: chaining clean+all in one invocation
# leaves the order-only $(OBJDIR) target evaluated as up-to-date (it exists at
# startup), so clean would delete obj/ without it being recreated.
.PHONY: debug
debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(DEBUGFLAGS)" all
	@echo "Debug build complete"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "Clean complete"

# Install to /usr/local/bin (requires sudo on most systems)
.PHONY: install
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	@echo "Installation complete"

# Uninstall from /usr/local/bin
.PHONY: uninstall
uninstall:
	@echo "Removing $(TARGET) from /usr/local/bin..."
	@rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete"

# Run the program (requires arguments)
.PHONY: run
run: $(TARGET)
	@./$(TARGET)

# Display help
.PHONY: help
help:
	@echo "Wir - Port and Process Inspector"
	@echo "================================"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build the program (default)"
	@echo "  debug     - Build with debug symbols and DEBUG flag"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin (may require sudo)"
	@echo "  uninstall - Remove from /usr/local/bin (may require sudo)"
	@echo "  run       - Build and run the program"
	@echo "  help      - Display this help message"
	@echo ""
	@echo "Current platform: $(PLATFORM)"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build the program"
	@echo "  make debug        # Build with debug symbols"
	@echo "  make clean        # Clean build artifacts"
	@echo "  sudo make install # Install system-wide"

# Dependency tracking (automatically generated)
-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CFLAGS) -MM -MT $(OBJDIR)/$*.o $< > $@
