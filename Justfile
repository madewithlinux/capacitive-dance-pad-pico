
upload:
	(test -e /dev/ttyACM0 && picocom -b 1200 /dev/ttyACM0) || true
	sleep 1s
	while ! test -d /media/julia/RPI-RP2; do sleep 1s; done
	sleep 1s
	cp build/capacitive-dance-pad-pico.uf2 /media/julia/RPI-RP2

build-upload-monitor:
	$HOME/.pico-sdk/ninja/v1.12.1/ninja -C ./build
	just upload
	while ! test -e /dev/ttyACM0; do sleep 1s; done
	sleep 1s
	picocom /dev/ttyACM0
