BUILD_DIR=build
include $(N64_INST)/include/n64.mk

all: game.z64

OBJS = $(BUILD_DIR)/src/main.o \
       $(BUILD_DIR)/src/game.o \
       $(BUILD_DIR)/src/field.o \
       $(BUILD_DIR)/src/player.o \
       $(BUILD_DIR)/src/balls.o \
       $(BUILD_DIR)/src/render.o \
       $(BUILD_DIR)/src/effects.o \
       $(BUILD_DIR)/src/sfx.o

game.z64: N64_ROM_TITLE="Superbounce64"
game.z64: $(BUILD_DIR)/game.elf

$(BUILD_DIR)/game.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) game.z64

-include $(OBJS:.o=.d)
