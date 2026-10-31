Design a 3D-printable enclosure for the **receiver** of my DIY LED-strip controller, using the connected Autodesk Fusion tools. Model it parametrically in Fusion, then export print-ready files.

## Context
The receiver is a Raspberry Pi Pico that gets commands over an nRF24L01+ radio from a handheld remote and drives a 12 V WS2811 LED strip on my ceiling. The box will hang near the ceiling (wall or ceiling surface), close to the start of the strip. It's a purely functional box: the parts can **lie loose inside** with no special mounts, as long as everything fits together with its cables.

## What goes inside
| Part | Size (verify with me) | Notes |
|---|---|---|
| Raspberry Pi Pico (RP2040, original, no WiFi) | PCB 51 × 21 × 1 mm; micro-USB connector on one short edge, overhanging ~1.3 mm | May have pin headers soldered on with Dupont jumper wires plugged in. Then it needs much more height (male pins ~8.5 mm below the board + Dupont housings ~14 mm). **Ask me**. |
| nRF24L01+ module (PCB-trace antenna version) | ~29 × 15.2 mm | 2×4 pin header (2.54 mm) at one end, with Dupont jumpers to the Pico (8 wires). A 100 µF electrolytic capacitor (~6.3 mm Ø × 11 mm) sits on its VCC/GND pins. Its **antenna end should face an outer wall**, away from wires; plastic only near it. |
| "Pixel Boost" level shifter (MSX Elektronika) | 18 × 8 mm board | 4 pins: GND, VCC, DIN, DOUT. 3 wires from the Pico, 2 wires out to the strip. |
| Small resistor (330 Ω) | in-line in the data wire | Just needs space in the wire run. |

## Cables leaving the box
1. **A micro-USB cable** powers the Pico. Its plug goes into the Pico inside the box. The cable passes through the wall. I'd prefer a **slot/notch open to the lid edge** so I can lay the cable in without threading the plug through a hole (typical plug overmold ~11 × 7 mm, cable Ø 3–4 mm).
2. **The wires to the LED strip:** GND + DATA (2 wires, maybe a 3-core cable, Ø up to ~5 mm). Same treatment: a notch plus some **strain relief** (e.g. a zip-tie anchor bar inside).

The 12 V supply goes straight to the strip, *not* through this box.

## Requirements
- **One colour, one material.** No multi-colour or AMS features. Print without supports if at all possible.
- **Layout:** separate compartments for Pico, radio and Pixel Boost, **connected by wire channels** (low dividers or gaps), so the parts don't slide on top of each other while the wires run freely. Compact overall, but leave real room for Dupont connectors and wire bends.
- **Lid:** either snap-fit or 2–4 screws. Ask me which fasteners I have (I might have M3 screws; no heat-set inserts unless I confirm).
- **Mounting:** 2 screw holes or keyhole slots on the back for a wall/ceiling, *and* a flat back that also works with double-sided tape.
- **Optional, nice to have:** a small hole or thin window over the Pico's onboard LED (it sits near the USB end, ~2–3 mm from the edge on the top side). It blinks on every received radio packet, which is handy for diagnosis.
- I don't need access to the Pico's BOOTSEL button.

## Printer & materials
- **Printer:** Bambu Lab A1 (build volume 256 × 256 × 256 mm, 0.4 mm nozzle).
- **Filaments I have:** red, dark green, cyan-green gradient, white, black, transparent, brown. Some are PLA and some PETG. **Ask me which is which** before recommending one. White or black is probably most discreet on a ceiling.

## How I'd like you to work
1. **Before modelling, ask me (all in one message):**
   - Do the Picos have soldered headers?
   - Are the Dupont jumpers plugged in, or are the wires soldered?
   - Which cable diameters do I have?
   - Which fasteners do I have?
   - Which filaments are PLA and which are PETG?
   - Wall or ceiling mount?
2. **Propose the concept:** outer dimensions, compartment layout sketch, lid type and mount type. Wait for my OK.
3. **Model it in Fusion:**
   - Parametric, with **user parameters** for wall thickness, the general clearance (default 0.3 mm), the compartment sizes and the cable notch sizes, so I can tweak them without remodelling.
   - Name the bodies and components clearly.
4. **Show me screenshots** of the assembly and of each part.
5. **Export each printable part** as 3MF (preferred, for Bambu Studio) or STL. Give me a short print table for each part: filament, orientation, layer height, walls, infill and supports.
