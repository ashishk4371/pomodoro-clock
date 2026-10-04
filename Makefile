APP = pomodoro.app
EXEC = pomodoro
ICON = Pomodoro.icns
PLIST = Info.plist
CC = gcc
CFLAGS = -Wall -Wextra -std=c17
CPPFLAGS = -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib
LDLIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

$(APP): $(APP)/Contents/MacOS/$(EXEC) \
		$(APP)/Contents/Info.plist \
		$(APP)/Contents/Resources/$(ICON) \
		bundle-assets
		@echo "Built $(APP)"

$(APP)/Contents/MacOS/$(EXEC): main.c
		mkdir -p $(APP)/Contents/MacOS
		$(CC) $(CPPFLAGS) $(CFLAGS) main.c \
		-o $@ $(LDFLAGS) $(LDLIBS)

$(APP)/Contents/Info.plist: $(PLIST)
		mkdir -p $(APP)/Contents
		cp $(PLIST) $@

$(APP)/Contents/Resources/$(ICON): $(ICON)
	mkdir -p $(APP)/Contents/Resources
	cp $(ICON) $@

bundle-assets:
	mkdir -p $(APP)/Contents/Resources
	rm -rf $(APP)/Contents/Resources/images
	rm -rf $(APP)/Contents/Resources/sounds
	cp -R image $(APP)/Contents/Resources/images
	cp -R sound $(APP)/Contents/Resources/sounds

run: $(APP)
	./$(APP)/Contents/MacOS/$(EXEC)

open: $(APP)
		open $(APP)

install: $(APP)
	mkdir -p $(HOME)/Applications
	rm -rf $(HOME)/Applications/$(APP)
	cp -R $(APP) $(HOME)/Applications/

install-system: $(APP)
	sudo rm -rf /Applications/$(APP)
	sudo cp -R $(APP) /Applications/

.PHONY: install install-system

clean:
	rm -rf $(APP)
