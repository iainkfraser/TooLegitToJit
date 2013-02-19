#CC := mipsel-openwrt-linux-gcc
CFLAGS := -I include -std=gnu99 -g3 -MD -MP -Werror 
OBJECTS := main.o mips_emitter.o mips_setup.o table.o func.o 

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
