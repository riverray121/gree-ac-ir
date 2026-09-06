# Diagnostics

Nothing here is part of the deployed system. These files reproduce the IR reliability investigation and are kept for the next time an AC ignores commands.

## diag-ir-monitor.yaml

Receiver rig: a spare ESP32 (any ESP32 dev board) with a TL1838 receiver, OUT on GPIO14 (a second receiver on GPIO27 is configured so either pin works), VCC on 3V3, GND on GND. Tape the receiver's dome beside the AC's receiver window, facing the room. It publishes one text sensor per pin with the symbol count of every burst and dumps the full pulse timings to its log. Flash with `esphome run diagnostics/diag-ir-monitor.yaml`, then stream timings with `esphome logs diagnostics/diag-ir-monitor.yaml --device 192.168.8.7`.

## diag-ac-ram-bedroom-bitbang.yaml

Ram's unit with the library's software-timed transmit, the configuration that failed. Flash it to reproduce the old behavior; flash `ac-ram-bedroom.yaml` to return to the hardware-timed path.

## harness/

Python scripts run on the Mac from a working directory that holds the rig's log stream (`session_rx.log`):

- `diag_trial.py N out.csv`: sends N alternating off / cool commands to Ram's AC through Home Assistant, and per command records whether the blaster transmitted (IR TX Log counter), what the rig received (frame bytes, checksum, per-pulse timing), and whether the AC display is lit (laptop camera pointed at the AC; `diag_cam.py` counts green pixels in the display region, adjust `ROI` for the framing).
- `diag_stress.py HOST SECONDS PPS`: UDP flood plus repeated TCP connects against a blaster. Running it during a trial batch reproduces the software-timed failure on demand.
- `diag_irdecode.py LOG`: pulse statistics for every frame in a log; `diag_summarize.py out.csv`: batch summary.

Reference numbers from the YAP2F remote measured at the receiver (10 presses, 36 half-frames): header 8993/4490, bit mark 653, zero space 553, one space 1657, block gap 19989, all within 20 us.
