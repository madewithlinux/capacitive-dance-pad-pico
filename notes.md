# notes.md

# dance pad sensors layout

```
- A B -
H - - C
G - - D
- F E -
```


| btn      | sensor | pin | index | pio | sm |
|----------|-------:|----:|------:|----:|---:|
| /\ Up    |      A |  14 |     3 |   0 |  3 |
| /\ Up    |      B |  15 |     7 |   1 |  3 |
| >> Right |      C |  13 |     2 |   0 |  2 |
| >> Right |      D |  12 |     6 |   1 |  2 |
| \/ Down  |      E |  11 |     1 |   0 |  1 |
| \/ Down  |      F |  10 |     5 |   1 |  1 |
| << Left  |      G |   8 |     0 |   0 |  0 |
| << Left  |      H |   9 |     4 |   1 |  0 |


# config serial
```
set threshold_factor 2.0
set usb_hid_enabled 0
set filter_type 0
set filter_type 1
set filter_type 2
```

# pump dance pad
```bash
just build-upload-monitor sockpad_pump_p1
just build-upload-monitor sockpad_pump_p2
```


# check polling rate
https://github.com/Simply-Love/Simply-Love-SM5/blob/itgmania-release/Other/Documentation/USBPollingIssues-README.md
https://github.com/geefr/stepmania-linux-goodies/wiki/evhz-usage

https://wiki.archlinux.org/title/mouse_polling_rate
check requested polling rate:
```bash
sudo grep -B3 -A12 "cafe.*4016" /sys/kernel/debug/usb/devices
```


# higher sensitivity, for use with shoes?
```
set threshold_value 10
set threshold_value 15
```