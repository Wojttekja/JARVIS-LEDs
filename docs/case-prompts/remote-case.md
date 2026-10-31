Design a 3D-printable **remote control** (a Stream Deck-style desk controller) for my DIY LED-strip system, using the connected Autodesk Fusion tools. Include **custom keycaps** and a **knob**. Model it parametrically in Fusion, then export print-ready files. This one should **look good**: it sits on my desk.

## Context
The remote is a Raspberry Pi Pico with 9 mechanical key switches in a 3×3 grid, a KY-040 rotary encoder (turn + push) and an nRF24L01+ radio. It's powered by a micro-USB cable and sends commands to a receiver that drives the LED strip on my ceiling.

### What the keys do (for keycap icons)
| | Left | Middle | Right |
|---|---|---|---|
| **Top row** | K1 power on/off (long press: 30-min sleep timer) | K2 rainbow | K3 fire / flames |
| **Middle row** | K4 breathe (slow pulse) | K5 nebula (purple clouds) | K6 warm white |
| **Bottom row** | K7 colour slot 1 (red by default) | K8 colour slot 2 (green) | K9 colour slot 3 (blue) |

K7–K9 are user-customisable colour memories. **Knob:** turn changes brightness. Press cycles the knob mode: brightness → speed → hue.

### Status LED
The Pico's **onboard LED** is the remote's only visual feedback: a short flash when a command is sent, 1–3 blinks for the knob mode, and fast blinking when the receiver is unreachable. It sits on the top side of the Pico near the USB end, ~2–3 mm from the edge. **Please make it visible from the outside:** a light pipe or window printed in my transparent filament would be great.

## Components
| Part | Size (verify with me) | Notes |
|---|---|---|
| Raspberry Pi Pico (RP2040) | PCB 51 × 21 × 1 mm; micro-USB on one short edge (overhang ~1.3 mm) | May have soldered headers. **Ask me** how it will be wired: soldered wires or headers with Dupont jumpers. Needs a cutout/notch for the micro-USB plug (overmold ~11 × 7 mm), ideally at the back. |
| nRF24L01+ (PCB-trace antenna version) | ~29 × 15.2 mm | 2×4 header at one end, plus a 100 µF capacitor (~6.3 mm Ø × 11 mm) on its power pins. Keep the antenna end clear of wires; plastic only near it. |
| 9 × key switches | **Ask me the exact type first.** Assume Cherry MX-compatible until I confirm | MX defaults: plate cutout 14.0 × 14.0 mm in a **1.5 mm thick plate section** (the clips need that), 19.05 mm pitch (a larger, Stream Deck-like pitch is fine if it looks better), ~5 mm body below the plate + 3.3 mm pins, plus room for wires. |
| KY-040 rotary encoder module | PCB ~32 × 19 mm with 2 mounting holes Ø ~3 mm; threaded bushing M7 × ~7 mm; 6 mm shaft (D-flat or knurled, **ask me**) ~12–15 mm above the bushing; 5-pin header at one PCB end (often right-angled) | Mounts through a ~7.2 mm panel hole with its nut, or on standoffs. The **shaft must still push down** (it's a button), so the knob must not bottom out on the case. |

**Wiring:** every switch has 2 wires (one to its own GPIO, the other to a shared GND), the encoder has 5 and the radio 8. Leave a sensible routing space under the plate.

## Design wishes
- **Shape:** desk-friendly, with a slight **tilt toward the user** (~10–15°) like a Stream Deck. Use rubber-feet recesses or a non-slip bottom. Place the knob where it feels natural, e.g. to the right of the grid or centred above it. Propose options.
- **Construction:** a top part (with the switch plate) plus a bottom shell, joined with screws. Ask me what fasteners I have (maybe M3; heat-set inserts only if I confirm). The Pico must be easy to reach for re-plugging the USB.
- **Keycaps:**
  - MX cross stem: cross ~4.0 × 4.0 mm, arms ~1.3 mm, with parametric FDM clearance.
  - Clean sculpted or flat-topped design, ~18 mm or larger for a Stream Deck look.
  - Each key gets an **embossed or debossed icon** for its function from the table above.
  - **Single colour per keycap.** No multi-material printing within a part, but different keys can use different filaments. Suggest a colour scheme from my filaments, for example red for K7, dark green for K8, the cyan-green gradient for rainbow or K9, red or brown for fire.
- **Knob:** ~30–35 mm Ø, grippy (knurled or ribbed) with an indicator mark, fitting the encoder shaft with a parametric clearance.
- **Printing:** everything should print **without supports** wherever possible, so think about the orientation of the plate, the shell, the caps and the knob.

## Printer & materials
- **Printer:** Bambu Lab A1 (256 × 256 × 256 mm, 0.4 mm nozzle). **No multi-colour within one part.**
- **Filaments:** red, dark green, cyan-green gradient, white, black, transparent and brown. Some are PLA and some PETG. **Ask me which is which.** I don't need to use them all.

## How I'd like you to work
1. **Before modelling, ask me (all in one message):**
   - What type are the switches, and what are their dimensions?
   - What does the encoder shaft look like?
   - How is the Pico wired (soldered wires, or headers with Dupont jumpers)?
   - Which fasteners do I have?
   - Which filaments are PLA and which are PETG?
   - Do I prefer the knob position right of the grid or above it?
2. **Propose 2–3 concepts** (shape, knob position, colour scheme), each with rough dimensions. Wait for me to pick one.
3. **Design a small tolerance test print first:**
   - one switch cutout in a 1.5 mm plate;
   - one keycap stem fit;
   - the knob-to-shaft fit;
   - a corner of the Pico pocket.

   I'll print it and report back, so we can tune the clearances before the big print.
4. **Model it in Fusion:**
   - Parametric, with **user parameters** for clearances (default 0.2–0.3 mm), key pitch, tilt angle, wall thickness, keycap size and the stem fit.
   - Clearly named components.
5. **Show me renders/screenshots** of the assembly (with the caps in their colours) and of each part.
6. **Export each printable part** as 3MF (preferred) or STL. Give me a print table for each part: filament, orientation, layer height, walls, infill and supports. Then write short assembly notes (what goes in first, wire routing).
