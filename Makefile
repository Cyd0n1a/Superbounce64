BUILD_DIR=build
include $(N64_INST)/include/n64.mk
include tiny3d/t3d.mk

all: game.z64

OBJS = $(BUILD_DIR)/src/main.o \
       $(BUILD_DIR)/src/game.o \
       $(BUILD_DIR)/src/field.o \
       $(BUILD_DIR)/src/player.o \
       $(BUILD_DIR)/src/balls.o \
       $(BUILD_DIR)/src/render.o \
       $(BUILD_DIR)/src/effects.o \
       $(BUILD_DIR)/src/sfx.o \
       $(BUILD_DIR)/src/save.o \
       $(BUILD_DIR)/src/title3d.o \
       $(BUILD_DIR)/src/play3d.o \
       $(BUILD_DIR)/src/bg3d.o

filesystem/mozartku.xm64: assets/mozartku.xm
	@mkdir -p filesystem
	@echo "    [AUDIO] $@"
	$(N64_AUDIOCONV) -o filesystem "$<"

filesystem/ld-logo.sprite: assets/ld-logo.png
	@mkdir -p filesystem
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) -f RGBA16 -o filesystem "$<"

filesystem/cydonis-logo.sprite: assets/cydonis-logo.png
	@mkdir -p filesystem
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) -f RGBA16 -o filesystem "$<"

$(BUILD_DIR)/game.dfs: filesystem/mozartku.xm64 filesystem/ld-logo.sprite filesystem/cydonis-logo.sprite

game.z64: N64_ROM_TITLE="Superbounce64"
game.z64: $(BUILD_DIR)/game.elf $(BUILD_DIR)/game.dfs

$(BUILD_DIR)/game.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) game.z64 filesystem

-include $(OBJS:.o=.d)
