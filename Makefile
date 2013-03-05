#CC := mipsel-openwrt-linux-gcc
CFLAGS := -I include -std=gnu99 -g3 -MD -MP -Werror -D_ELFDUMP_ 

# arch independent 
OBJECTS := elf.o jitfunc.o temporaries.o emitter32.o frame.o instruction.o synthetic.o 
OBJECTS += main.o table.o func.o operand.o stack.o xlogue.o 

# arch dependent
OBJECTS += arch/mips/machine.o

OBJDIR := bin
OBJS := $(patsubst %,$(OBJDIR)/%,$(OBJECTS))



$(OBJDIR)/eluajit : $(OBJS) 
	 $(CC) $(CFLAGS) $(OBJS) -o $(OBJDIR)/eluajit

$(OBJDIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $*.c -o $(OBJDIR)/$*.o

clean : 
	rm bin/*

# include the depedency file. Note the "-" that means ignore it if file doesn't exist
# because if .d doesn't exist then .o doesn't exist so its gonna get rebuilt anyway.
# Remember the whole point of .d is to command make to rebuild obj file because
# header changed
-include $(OBJS:%.o=%.d)
