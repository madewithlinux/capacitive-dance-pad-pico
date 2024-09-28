set dotenv-load

dfu:
	test -e $DEVICE
	picocom -b 1200 $DEVICE || true

default_upload_target := 'sockpad_itg8'
default_build_target := 'all'

upload TARGET=default_upload_target:
	(test -e $DEVICE && picocom -b 1200 $DEVICE) || true
	sleep 1s
	while ! test -d /media/julia/RPI-RP2; do sleep 1s; done
	# sleep 1s
	cp build/{{TARGET}}.uf2 /media/julia/RPI-RP2

build-upload-monitor TARGET=default_upload_target:
	(test -e $DEVICE && picocom -b 1200 $DEVICE) || true
	$HOME/.pico-sdk/ninja/v1.12.1/ninja -C ./build
	while ! test -d /media/julia/RPI-RP2; do sleep 1s; done
	cp build/{{TARGET}}.uf2 /media/julia/RPI-RP2
	while ! test -e $DEVICE; do sleep 1s; done
	picocom $DEVICE

build TARGET=default_build_target:
	$HOME/.pico-sdk/ninja/v1.12.1/ninja -C ./build {{TARGET}}

realclean:
	$HOME/.pico-sdk/ninja/v1.12.1/ninja -C ./build clean
	rm -f \
		./build/*.bin \
		./build/*.dis \
		./build/*.elf \
		./build/*.elf.map \
		./build/*.hex \
		./build/*.uf2 \
		./build/*.pio.h

build-clean:
	$HOME/.pico-sdk/ninja/v1.12.1/ninja -C ./build clean
	$HOME/.pico-sdk/ninja/v1.12.1/ninja -C ./build


monitor:
	while ! test -e $DEVICE; do sleep 1s; done
	picocom $DEVICE

lsusb:
	sudo lsusb -d cafe:4016 -v > lsusb_output.log

list-sources:
	ls -1 src/*.h src/*.c src/*.hpp src/*.cpp
