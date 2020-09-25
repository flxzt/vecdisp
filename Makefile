# vecdisp Makefile

SRCDIR 		:= src
SRCS   		:= $(wildcard $(SRCDIR)/*.c)
OBJDIR 		:= obj
OBJS   		:= $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
BINDIR 		:= bin
BINS   		:= menu test clock pong asteroids
INCDIR 		:= include
LIBDIR		:= lib

CC 			:= gcc
CPPFLAGS 	:= -I$(INCDIR)
CFLAGS 		:= -std=gnu11
LDFLAGS		:= -L$(LIBDIR) -Wl,-rpath=/usr/local/lib
LDLIBS		:= -lm
DEBUG 		?= y
ifeq ($(DEBUG), y)
    CFLAGS 	+= -ggdb -Wall
else
    CFLAGS 	+= -O3
endif
OUT_METHOD 	?= SIM
ifeq ($(OUT_METHOD), SIM)
	CFLAGS 	+= -DOUT_METHOD_SIM
	LDLIBS 	+= -lSDL2
else ifeq ($(OUT_METHOD), SPIDAC)
	CFLAGS 	+= -DOUT_METHOD_SPIDAC -DOUT_METHOD_DIGITAL
	LDLIBS 	+= -lSDL2 -lbcm2835
endif
DRAW_RES 	?= 1024
CFLAGS 		+= -DDRAW_RES=$(DRAW_RES)

LD_CMD		= $(CC) $(LDFLAGS) $^ $(LDLIBS) -o $(BINDIR)/$@

.PHONY: all clean
.DEFAULT_GOAL: all

all: $(BINS)

clean:
	$(RM) $(OBJS)
	$(RM) $(wildcard $(BINDIR)/*)

$(OBJDIR):
	mkdir $@

$(BINDIR):
	mkdir $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR) $(BINDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Targets (can be changed as needed)
menu: $(OBJDIR)/menu.o $(OBJDIR)/fonts.o $(OBJDIR)/libvecdisp.o
	$(LD_CMD)

test: $(OBJDIR)/test.o $(OBJDIR)/fonts.o $(OBJDIR)/libvecdisp.o
	$(LD_CMD)

clock: $(OBJDIR)/clock.o $(OBJDIR)/fonts.o $(OBJDIR)/libvecdisp.o
	$(LD_CMD)

pong: $(OBJDIR)/pong.o $(OBJDIR)/fonts.o $(OBJDIR)/libvecdisp.o
	$(LD_CMD)

asteroids: $(OBJDIR)/asteroids.o $(OBJDIR)/fonts.o $(OBJDIR)/libvecdisp.o
	$(LD_CMD)

demo3d: $(OBJDIR)/demo3d.o $(OBJDIR)/fonts.o $(OBJDIR)/libvecdisp.o
	$(LD_CMD)