USERMODULES_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $(USERMODULES_DIR)/lsm303dlhc.c

CFLAGS_USERMOD += -I$(USERMODULES_DIR)



#LDFLAGS_USERMOD += $(BUILD)/py/malloc.o

#LDFLAGS_USERMOD += -lc -lm -lnosys 

