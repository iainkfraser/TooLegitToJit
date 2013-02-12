#CC := mipsel-openwrt-linux-gcc

# TODO: use gcc dependecy feature 
HEADERS := mc_emitter.h mips_mapping.h mips_emitter.h user_memory.h mips_opcodes.h bit_manip.h 
HEADERS += lua.h luaconf.h lopcodes.h llimits.h 
HEADERS += list.h  

# TODO use objs to speed up compilation on qemu 

HDRDIR := include
HDRS = $(HEADERS:%.h=$(HDRDIR)/%.h)

FLAGS := -I include -std=gnu99 -g3

eluajit	: main.c mips_emitter.c mips_setup.c $(HDRS)  
	 $(CC) $(FLAGS) main.c mips_emitter.c mips_setup.c -o eluajit

clean : 
	rm eluajit
