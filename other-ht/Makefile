CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra
TOMMYDIR ?= tommyds/tommyds
PROJECT_ROOT := ..
TOMMY_CFLAGS = -I$(TOMMYDIR) -include string.h -include $(TOMMYDIR)/tommylist.h

.PHONY: all clean run

all: ht_tommy

ht_tommy: ht_tommy.c $(PROJECT_ROOT)/common/randstring.c $(PROJECT_ROOT)/common/lineio.c
	$(CC) $(CFLAGS) $(TOMMY_CFLAGS) -I$(PROJECT_ROOT)/common \
		ht_tommy.c $(PROJECT_ROOT)/common/randstring.c $(PROJECT_ROOT)/common/lineio.c \
		$(TOMMYDIR)/tommy.c \
		-o $@

run: ht_tommy
	./ht_tommy

clean:
	rm -f ht_tommy newdata.txt
