# System architecture

Three ESPHome ESP32 units transmit infrared to Gree GSE-50CI air conditioners (Kelvinator protocol, 128-bit frames). Control flows from Home Assistant on the Pi 5 (192.168.8.2). IR is one-way: an entity's state in HA means "command transmitted", never "appliance obeyed".

## Devices

| Unit | Node | IP | Entity | Emitter | tx_delay |
|---|---|---|---|---|---|
| Living room | `ac-living-room` | 192.168.8.4 | `climate.living_room_ac` | Grove module, GPIO4; TL1838 receiver GPIO14 | 0s |
| Elijah's bedroom | `ac-elijah-bedroom` | 192.168.8.5 | `climate.elijah_s_bedroom_ac` | Grove module, GPIO4 | 1s |
| Ram's bedroom | `ac-ram-bedroom` | 192.168.8.6 | `climate.ram_s_bedroom_ac` | KN2222A + two 940nm LEDs in series (5V→LED→LED→22Ω→collector, base via 1kΩ from GPIO4) | 2s |

Entity ids derive from the device friendly-name slug (climate `name: ""` inherits it), not the node name.

## Firmware behavior (components/kelvinator_ac)

Wraps IRremoteESP8266's `IRKelvinatorAC`; the library bit-bangs the 38 kHz carrier on the pin, so no `remote_transmitter` may claim that pin.

- **Set-implies-on**: a target-temperature command while the unit is off switches it to cool and on. Any command that names a mode uses that mode. Consequence: any bare `climate.set_temperature` call (voice agent, dashboard slider, script) powers the AC on.
- **Frame repeat**: every transmission sends the frame twice back-to-back (~0.8s on air). The AC beeps per accepted frame; one beep still means the command executed.
- **Transmit slots (`tx_delay`)**: each unit delays its transmission by its slot. Two units transmitting simultaneously corrupt each other at any receiver that can see both emitters, so a command targeting several ACs staggers on the air. 1s spacing is the floor while the repeat is on.
- **Coalescing**: commands are scheduled through one named timeout; a command arriving before the pending transmission replaces it, and only the final state is transmitted. Consequence: commands to the same unit spaced closer than its tx_delay can drop the earlier one. Real usage is unaffected; rapid test loops must space commands beyond the unit's slot.

Interaction with `script.control_ac` (mode call, then temperature call 1s later): depending on the unit's slot the two calls transmit as one merged frame or two frames. Final state is correct in all cases.

## Voice path

Pipeline "Local" (preferred): faster-whisper STT → `conversation.claude_conversation` (Anthropic integration, entry title "Claude") → piper TTS. `prefer_local_intents` is **false** and must stay false: the built-in intent agent is not used, by explicit decision.

The conversation subentry prompt (stored in `.storage/core.config_entries` on the Pi; editing requires stopping the container) directs the agent: replies as short as possible, exactly "Done." after a successful action, no lists or markdown, one-sentence answers unless detail is requested; for any request to run an AC in a mode or at a temperature, call `script.control_ac` picking the matching ac option ("my bedroom" means elijah bedroom), the mode, and the temperature; use the normal turn-off tool to switch off; never claim success without a successful tool call this turn.

STT: wyoming faster-whisper on the Pi runs `--model small-int8` with an `--initial-prompt` carrying the AC vocabulary (biases short-command transcription of "Ram"/"Elijah"). The Voice PE's finished-speaking detection is set to relaxed; the default cut the mic during mid-sentence pauses.

Known hazard: the LLM's spoken reply can assert success it never executed. Debug voice issues by reproducing with the websocket command `conversation/process` against `conversation.claude_conversation` and diffing entity states before and after, never from the reply text.

## script.control_ac

Fields: `ac` (select: "living room" / "elijah bedroom" / "ram bedroom"), `hvac_mode` (off/cool/heat/fan_only/dry/heat_cool), `temperature` (optional, 16-30).

Sequence: map `ac` to its entity through a static dict → `climate.set_hvac_mode` → if a temperature was given and mode is not off, wait 1s → `climate.set_temperature`. Powering on before setting temperature is therefore explicit and does not rely on set-implies-on.

The AC is a fixed select so the LLM does all language interpretation and the script does none: the tool schema constrains the choice to the three real units, which removes the free-text name-matching failure class entirely (substring matching once routed "Ram bedroom" to the wrong AC because area "Bedroom" is a substring). The script description tells the agent that "my bedroom" means Elijah's bedroom. A fourth AC requires adding one select option and one dict entry, and mirroring it in this file.

The live script is mirrored in `docs/control_ac.script.json`; update the snapshot when the script changes.

## HA structure

- Areas: `living_room`, `bedroom` (Elijah), `rams_bedroom`, one AC device each.
- Aliases on each climate entity cover phrasing and STT variants (e.g. "Rom bedroom AC" for "Ram").
- Dashboard `ac-hub`: one thermostat card per AC (hvac modes + fan modes; no swing, swing stays off).
- Geofence: zone `near_home` (1 km). Rules: any member arriving → living room cool 25 (only if off); a member arriving → their own bedroom cool 25 (only if off); a member leaving → their own bedroom off regardless of who stays; the last member leaving → all ACs off. Automations: `ac_geofence_on` (living room, all listed persons), `ac_geofence_off` (all ACs, fires when every person is outside home and near_home), `ac_elijah_bedroom_on`/`ac_elijah_bedroom_off`. Ram's bedroom pair and his presence in the shared trigger lists require Ram's person entity.

## Physical constraints

- Power the ESP32s from wall USB adapters. Power banks auto-shut off on the ESP32's low draw.
- The ESP32 has 512 RMT symbols total shared between remote_receiver and remote_transmitter; a receiver configured with all 512 makes a transmitter fail init with `ESP_ERR_NOT_FOUND`.
- The AC's receiver can be blinded by direct sunlight; frame acceptance at marginal signal is probabilistic. The repeat covers single-frame loss.
