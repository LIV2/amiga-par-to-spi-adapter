PROJECT=spisd
INCLUDE=$$NDK32/Include_I
AS=vasmm68k_mot
ASFLAGS=-Fhunk -I$(INCLUDE) -quiet -align -DROM
LINKER=vlink
LINKFLAGS=-brawbin1 -s -sc -sd -mrel
OBJDIR=obj

.PHONY: all clean rom

SRCS = bootldr.S \
       loadseg.S \
       endcode.S

OBJS = $(SRCS:%.S=$(OBJDIR)/%.o)

all:	$(PROJECT).rom

# Nibble-wide boot loader with Byte-wide driver
$(OBJDIR)/%.o: %.S
	@mkdir -p $(OBJDIR)
	$(AS) $(ASFLAGS) -DBYTEWIDE -o $@ $<

$(OBJDIR)/bootldr:	$(OBJDIR)/bootldr.o $(OBJDIR)/loadseg.o $(OBJDIR)/endcode.o
	$(LINKER) $(LINKFLAGS) -o $@ $^

$(OBJDIR)/bootnibbles: $(OBJDIR)/bootldr mungerom.py
	@mkdir -p $(OBJDIR)
	./mungerom.py

$(OBJDIR)/assets.o: assets.S $(OBJDIR)/bootnibbles ../spisd.device

$(PROJECT).rom: $(OBJDIR)/bootnibbles $(OBJDIR)/assets.o
	$(LINKER) $(LINKFLAGS) -Trom.ld -o $@ $(OBJDIR)/assets.o

clean:
	rm -f $(OBJDIR)/*
