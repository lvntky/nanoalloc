# ===========================================================================
#  Makefile — nanoalloc
#
#  Targets:
#    make          build libnanoalloc.so
#    make clean    remove build artifacts
#    make help     show this message
#
#  Variables:
#    CC=<compiler>   compiler to use (default: gcc)
# ===========================================================================

CC       := clang
SRCDIR   := src
INCDIR   := include
BUILDDIR := build

CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O2 -g \
          -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
          -shared -fPIC -I$(INCDIR)

LIB_SO := $(BUILDDIR)/libnanoalloc.so

# --------------------------------------------------------------------------

.PHONY: all
all: $(BUILDDIR) $(LIB_SO)
	@printf "\033[32m\033[1mDone.\033[0m\n"
	@printf "  lib : $(LIB_SO)\n"

$(BUILDDIR):
	@mkdir -p $@

$(LIB_SO): $(SRCDIR)/nanoalloc.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)

.PHONY: help
help:
	@printf "\033[1mTargets:\033[0m\n"
	@printf "  make          build libnanoalloc.so\n"
	@printf "  make clean    remove build artifacts\n"
	@printf "  make CC=clang use clang instead of gcc\n"
