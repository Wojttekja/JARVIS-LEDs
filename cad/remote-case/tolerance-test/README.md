# Remote case: tolerance test print

Six small test pieces that dial in the fits before the main print. Every piece is
exported in print orientation (as it lies on the bed), so load them flat and
**don't rotate them**.

| File | Filament | Notes |
|---|---|---|
| `t1_switch_plate_test.3mf` | black PLA (the case filament) | the plain face (on the bed) is the visible top; the stepped pockets face up |
| `t2_panel_and_fastener_test.3mf` | black PLA | bosses face up |
| `t3_keycap_stem_test.3mf` | black PLA, plus a second copy in red PETG if you can | the caps come in both materials |
| `t4_knob_shaft_test.3mf` | white PLA (the knob filament) | hubs face up |
| `t5_pico_usb_corner_test.3mf` | black PLA | the 12 mm bridge over the USB opening prints fine without supports |
| `t6_light_pipe_test.3mf` | transparent PLA | collar on the bed |

Settings for all pieces: 0.2 mm layers, 3 walls, 20 % infill, **no supports**, and a
textured PEI plate. Leave "X-Y hole compensation" at 0, because the test exists to
measure the real holes.

## What to check (the number on each test is the size it was modelled at)

- **T1 switch cutouts (14.0 / 14.1 / 14.2):** which one does a switch snap into firmly,
  with both clips engaged and no wobble?
- **T3 keycap stems (4.05 / 4.15 / 4.25):** press each one onto a switch stem.
  Which holds firmly but can still be pulled off by hand? Is any of them cracked?
- **T2 encoder hole (7.3):** does the bushing pass through, and does the nut
  tighten down flat?
- **T4 knob D-hole (6.1 / 6.2 / 6.3):** which one is snug with no rotational play?
  Does the shaft reach the bottom of the hole?
- **T2 light pipe holes (4.05 / 4.15 / 4.25):** which one holds the T6 rod from below
  with a light press?
- **T2 nut pockets (5.60 / 5.70 / 5.80):** which one holds an M3 nut so it doesn't
  fall out when you turn the piece upside down?
- **T5 Pico corner:** set the Pico-on-perfboard on the rails, pushed back against the
  stops. Does the micro-USB plug go all the way in? Is the opening centred on the
  connector, or too high or too low (by how much)?
