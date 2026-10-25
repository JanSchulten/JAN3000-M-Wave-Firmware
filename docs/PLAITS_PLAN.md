# Plaits-Engines: Plan und Machbarkeit (Phase 0)

Ziel: Fünf Oszillator-Modelle aus Mutable Instruments' Plaits (das Modul, auf dem Behringers Grind und Brains
aufbauen) als eigene Engines auf den FM-1 bringen, jeweils wie am Grind als Oszillator → Filter → ADSR. Dafür gehen
fünf bestehende Engines. Dieses Dokument hält die Entscheidungen fest und was Phase 0 gemessen hat.

## 1. Entscheidungen

| Raus | Rein (Plaits) | Gespeicherte Sounds der alten Engine laden als |
| --- | --- | --- |
| PHASE | **WAVESHAPE** (Waveshaping) | WAVESHAPE |
| VOICE | **WAVETABLE** | WAVETABLE |
| TRIO | **HARMONIC** (Additive) | ANALOG |
| DBOX | **PARTICLE** | DRUM (Kit-Lanes und Pattern bleiben) |
| SLICE | **SWARM** | SWARM |

- Fünf eigene Engines, keine Sammel-Engine mit MODEL-Parameter.
- Die neuen Engines werden an `ENGINES[]` angehängt (17 … 21); die alten Nummern bleiben reserviert und werden beim
  Laden umgewandelt, wie DIGITAL → FM6 (`fm4_convert.c`). Im PRESETS-Menü rücken die neuen an die Stelle der alten.
- Polyphonie: so viele Stimmen, wie die CPU schafft, pro Engine gemessen.
- Werks-Presets bringen keine Effekte mehr mit (umgesetzt, Abschnitt 4).
- PARTICLE: ein Diffusor pro Part, die Verzögerungen auf ~6 000 Samples gekürzt (12 KB, im Engine-Speicher des Parts).
- WAVETABLE: 16-Bit-Tabellen, wenn der Flash nach dem Aufräumen reicht, sonst 8 Bit.

## 2. Was Phase 0 gemessen hat

### Lizenz

Alle fünf Engines (`plaits/dsp/engine/{waveshaping,wavetable,additive,particle,swarm}_engine.cc`) und die stmlib
stehen unter MIT (Emilie Gillet). Das verträgt sich mit GPL-3.0; die MIT-Notiz muss mit in jede portierte Datei. Der
Name „Mutable Instruments“ darf nicht für das Ergebnis werben. Übrigens liegen auch die neueren Plaits-1.2-Modelle
(`plaits/dsp/engine2/`) im offenen Repository.

### Der Chip hat eine FPU

JieLis eigenes SDK baut für den WL82 (AC791N) mit `-mcpu=r3 -mfprev1` (`apps/demo/*/board/wl82/Makefile`): der Chip
hat eine Gleitkomma-Einheit. Felucca baut bisher ohne und rechnet ganzzahlig. Die Toolchain kann die FPU **pro
Funktion** einschalten: `__attribute__((target("fprev1")))`. Nur die Plaits-Funktionen bekommen sie; der Rest der
Firmware bleibt Befehl für Befehl wie er ist. Gemessen am Prototyp:

- mit dem Attribut: keine Soft-Float-Aufrufe mehr, Addition, Multiplikation, Division und Umwandlung sind
  einzelne Befehle (`r0 = r0 * r1 (f)`, `ftoi`, `itof`);
- ohne: 81 Aufrufe von Bibliotheksfunktionen (`__mulsf3` …) pro Sample-Schleife: unbrauchbar;
- Falle: jede Hilfsfunktion, die Float rechnet, braucht das Attribut auch, sonst wird sie nicht eingebettet und
  läuft als Soft-Float.

**Offen und nur am Gerät zu klären:** ob Feluccas eigener Start (`crt0.S`, `main.c`) die FPU schon nutzbar macht
(das Startup des SDK liegt nur als Bibliothek vor) und wie viele Takte ein FPU-Befehl kostet. Vorschlag: eine kleine
Probe in MENU > SYSTEM, die erst auf Tastendruck ein paar FPU-Befehle ausführt und die Zeit misst. Läuft sie nicht,
startet das Gerät nur neu (kein Boot-Problem, denn sie läuft nie beim Start).

### Waveshaping-Prototyp

Die Waveshaping-Engine nach C portiert (Float, FPU-Attribut) und Sample für Sample gegen das Original-C++ geprüft:

| Variante | Abweichung vom Original (240 000 Samples) |
| --- | --- |
| exakt | 0 (bitgleich) |
| schnell (Kehrwerte der Pulsbreite interpoliert statt dividiert) | −50 dB, Spitzen bis 0,49 während MORPH sich bewegt |

### Rechenaufwand

Statisches Kostenmaß wie `tests/target_budget.py` (Befehle in Schleifen, ×4 je Verschachtelung, Division ×9), am
pi32v2-Assembly des echten Builds gezählt. Kein Taktzähler, aber dasselbe Maß für alle:

| Engine | Kosten pro Stimme | | Bestehende Engine | Kosten |
| --- | --- | --- | --- | --- |
| WAVESHAPE (2 Oszillatoren + Formung) | ~400 | | ANALOG | 559 |
| WAVETABLE | ~380 | | FM6 (+ Operatoren) | 244 + |
| HARMONIC (Additive) | ~280 | | VOICE | 254 |
| PARTICLE (+ Diffusor) | ~360 | | TRIO | 244 + 135 |
| SWARM (8 Körner) | ~370 | | NOISE | 236 |

Die Plaits-Engines liegen damit im Bereich von ANALOG, nicht darüber. Was eine Stimme wirklich an Takten kostet,
hängt an den FPU-Latenzen (siehe oben) und wird am Gerät gemessen; danach wird die Stimmenzahl pro Engine festgelegt.
Plaits rechnet zusätzlich einen AUX-Ausgang; den brauchen wir nicht, das spart einen Teil.

### Speicher

| | Bedarf |
| --- | --- |
| Code der fünf Engines (pi32v2, `-Os`) | je 1,5 … 2,1 KB, zusammen ~9 KB, dazu die Felucca-Hülle (Parameter, Presets) |
| Tabellen | Sinus 2,5 KB, Falter 4,1 KB (als int16 2 KB), Waveshaper 2,6 KB |
| Wavetables | 25 344 Werte: **50,7 KB** in 16 Bit, 25,3 KB in 8 Bit |
| RAM: PARTICLE-Diffusor | 8 093 Samples Verzögerung → 16 KB je Instanz |
| RAM: der Rest | klein (SWARM ~0,5 KB je Stimme, im Engine-Speicher des Parts) |

Flash heute: Image 557 324 B von 581 564 B, **24 240 B frei** (die 72 KB in `hardware.md` waren veraltet).
Was das Entfernen freimacht:

| | Flash | RAM |
| --- | --- | --- |
| SLICE samt BREAK-Sample (22 050 B) und UI, gemessen (`FELUCCA_SLICE=0`) | 31 532 B | 4 960 B |
| PHASE, VOICE, TRIO, DBOX: Code, geschätzt aus `nm` (ohne Presets und Texte) | ~11 500 B | ~0,5 KB |
| **frei danach, zusammen mit den 24 240 B von heute** | **~67 KB** | |

Bedarf der fünf neuen Engines: Code und Hülle ~15 KB, Tabellen ~9 KB, dazu die Wavetables. Mit **8-Bit-Wavetables
(~50 KB zusammen) passt es mit ~17 KB Reserve**; mit 16 Bit (~75 KB) nur, wenn das Entfernen mehr bringt als
geschätzt (wird in Schritt 4 gemessen).

## 3. Offene Punkte

1. ~~WAVESHAPE exakt oder schnell~~: exakt. Die Division kostet auf dem Gerät so viel wie ein Multiplizieren-Addieren
   (Abschnitt 6), die schnelle Variante spart nichts und wäre nicht mehr bitgleich mit Plaits.
2. ~~Was die FPU auf dem Gerät kostet~~: gemessen (Abschnitt 6).

## 4. Umgesetzt: Werks-Presets ohne Effekte

Ein Werks-Preset bringt nur noch den Sound: die Sends der Spur (DIST, CHORUS, DELAY, REVERB) und ihr INSERT bleiben,
wie sie sind, beim PRESETS-Knopf, im Editor, bei Program Change, beim Wechsel der Engine und bei den zufälligen
Sounds eines neuen Projekts. Ein User-Preset bringt die Effekte mit, mit denen es gespeichert wurde. Nur die
Einschalt-Sounds laden ihre Sends wie bisher, damit das Einschalt-Projekt und Projekte im alten Format klingen wie
immer (`ui.c` `preset_fx`, `preset_load_to`; Test: `tests/preset_fx_test.c`).

## 5. Reihenfolge

1. ~~Werks-Presets ohne Effekte~~ (erledigt)
2. ~~Phase 0: Machbarkeit~~ (dieses Dokument)
3. ~~FPU-Probe auf dem Gerät~~ (gemessen 2026-10-24, Abschnitt 6)
4. ~~Alte Engines entfernen~~ (SLICE, PHASE, VOICE, TRIO, DBOX; Abschnitt 7)
5. ~~Neue Engines~~: WAVESHAPE (Abschnitt 8) → HARMONIC (9) → PARTICLE (10) → WAVETABLE (11) → SWARM (12); Pegel (13)
6. Presets, UI, Golden-Tests, Emulator; erst dann aufs Gerät, nur mit Okay

## 6. Die FPU-Probe (MENU > SYSTEM > INFO)

`firmware/src/fpu_probe.c` misst am Gerät, was der Host nicht kann. Unter den drei Zeilen von INFO steht ein zweiter
Block: **FPU**, **F I DIV NS**, **WS VOICE**. OCT+ startet die Probe, jeder Druck einmal:

- **FPU:** `OK`, wenn ein paar Ergebnisse bitgenau stimmen (Multiplizieren-Addieren, Division, Float ↔ Int) und
  16 Blöcke des Waveshapers denselben Hash ergeben wie auf dem Host (`FPB_WS_HASH`); `WS DIFFERS`, wenn nur der
  Waveshaper abweicht; `WRONG`, wenn schon die einfachen Ergebnisse falsch sind.
- **F I DIV NS:** Nanosekunden für ein Float-Multiplizieren-Addieren, ein Integer-Multiplizieren-Addieren und eine
  Float-Division (je eine abhängige Kette, die kürzeste von 16 Messungen, TIMER4 mit 24 MHz).
- **WS VOICE:** eine Stimme des Waveshapers (`plaits_ws.c`, bitgleich mit Plaits), ein 32-Sample-Block, in Prozent
  der Zeit, die das Audio dafür hat (32 Samples bei 44,1 kHz). Daraus folgt die Stimmenzahl.

Sicherheit: Die Probe läuft nur auf Tastendruck in der Hauptschleife, nie beim Start. In den ersten 30 s nach dem
Einschalten verweigert sie („FPU TEST AFTER 30 S“), solange der Boot-Schutz den Start noch mitzählt. Ein Absturz
kann also nie als fehlgeschlagener Start zählen. Stürzt sie ab, weil die FPU aus ist, zeigt das Gerät den
Crash-Bildschirm und startet nach 4 s neu. Vorher das Projekt speichern: Nach einem Absturz lädt RESTORE LAST nicht
automatisch.

Kosten: 12,2 KB Flash (vor allem die Plaits-Tabellen, die WAVESHAPE ohnehin braucht), 336 B RAM. Das Image hat
keine Soft-Float-Hilfsfunktion (`__mulsf3` …) gelinkt.

### Gemessen am Gerät (2026-10-24, Firmware 2f426e6)

| Zeile | Wert | Was es heißt |
| --- | --- | --- |
| FPU | OK | Feluccas eigener Start lässt die FPU nutzbar; die Ergebnisse sind bitgleich mit dem Host, also IEEE-Single wie erwartet, und der Waveshaper rechnet auf dem Gerät exakt wie Plaits |
| F I DIV NS | 33,7 / 28,2 / 30,9 | ein Glied der Kette (mit Schleife): Float-Multiplizieren-Addieren kaum langsamer als Integer, eine Float-Division nicht teurer als ein Multiplizieren-Addieren |
| WS VOICE | 4,3 % | eine Waveshaper-Stimme (32 Samples, die kürzeste von 64 Messungen, Cache warm) |

Folgerung: acht WAVESHAPE-Stimmen kosten ~35 % der Audio-Zeit, dazu Filter und Hüllkurve des Stimmenpfads und, im
echten Interrupt, Cache-Fehlgriffe auf die Tabellen im Flash. Die anderen vier Modelle liegen im statischen Kostenmaß
gleichauf (Abschnitt 2): **alle fünf können mit der vollen Stimmenzahl (NVOICE 8) geplant werden**; jede Engine wird
nach dem Einbau mit INFO > CPU bei acht gehaltenen Noten nachgemessen.

## 7. Schritt 4: die fünf Engines entfernt

Entschieden mit dem Nutzer: Es gibt keine wichtigen Daten, also werden alte Sounds nicht umgerechnet, sondern laden
als Standard-Sound eines Nachfolgers. Umgesetzt:

- `engines.c`: PHASE (2), VOICE (5), TRIO (6), SLICE (13) und DBOX (16) sind `ENG_GONE` (stumm, keine Parameter, keine
  Presets, nie angeboten). Ihre Nummern bleiben reserviert, denn Projekte, User-Presets und das Editor-Protokoll
  speichern Engine-Nummern.
- `core.h` `eng_retired`, `eng_successor`: DBOX → DRUM, die anderen vier → ANALOG (bis WAVESHAPE, WAVETABLE und SWARM
  ihre Plätze einnehmen: dann ändert sich nur diese Zeile). Wer eine der Nummern nennt, bekommt das erste Preset des
  Nachfolgers: `set_engine_of` (Editor, Engine-Wechsel), `project.c` `proj_retired` (Projekte, Autosave; die Bewegung
  auf den E-Werten fällt weg, der Rest des Sounds und die Steps bleiben), `upreset.c` (User-Presets: Liste, Name, Laden).
- Entfernt: `eng_phase.c`, `eng_formant.c`, `eng_trio.c`, `eng_dbox.c`, `ui_dbox.c`, `eng_slice.c`, `ui_slice.c`,
  `slice_store.c`, das BREAK-Sample und die Slice-Tabellen in `tools/gen_samples.py`, der Schalter `FELUCCA_SLICE`, die
  DBOX-Lanes im Raster, die DBOX- und SLICES-Seiten, der Credit für CrispyZebra (PHASE) und klattsch (VOICE).
- Tests: `tests/dbox_test.c`, `tests/slice_test.c` entfernt; neu `tests/retired_test.c` (nie angeboten, stumm, Wählen,
  Projekte, User-Presets; schlägt an, wenn `proj_retired` fehlt). Was TRIO, PHASE und VOICE in anderen Tests prüften,
  prüfen jetzt ANLG2, LOFI und PHYS (`retrig_test`, `mod_test`, `regress`: die Stimmengrenze einer Engine). Golden:
  0 Renders verändert, nur die der fünf entfernt; `union_test`: 546 Engine-Wechsel, keiner weicht ab.

| | vorher | nachher |
| --- | --- | --- |
| Image | 569 516 B | **522 836 B** (58 728 B frei) |
| RAM `.data`+`.bss` | 56 616 B | 51 640 B |
| Pool | 340 188 B | 339 548 B |

Was die fünf Plaits-Engines noch brauchen (die Tabellen von Sinus, Faltern und Waveshaper sind mit der FPU-Probe schon im
Image): Code und Hülle ~15 KB (geschätzt), dazu die Wavetables. Mit **16 Bit** (50,7 KB) sind das ~66 KB: **~7 KB mehr,
als frei ist**; mit 8 Bit (25,3 KB) ~41 KB, das passt mit ~18 KB Reserve. Die genaue Rechnung kommt mit den ersten
Engines (Code gemessen statt geschätzt); reicht es nicht für 16 Bit, gilt die Entscheidung „sonst 8 Bit“.

## 8. Schritt 5a: WAVESHAPE (Engine 17, `WSHAPE`)

Die erste spielbare Plaits-Engine, wie Grind sie spielt: Plaits-Oszillator → Filter → ADSR.

- `eng_wshape.c` um `plaits_ws.c` (das Modell der FPU-Probe, bitgleich mit Plaits): ein Modell pro Stimme im
  Engine-Speicher des Parts (`wsh_mem_t`, 56 B pro Stimme), der Float-Teil in `wsh_model` (`FM1_FPU`, auf der FPU),
  der Rest ganzzahlig wie jede Engine: DRV, ANALOGs Filter mit Knie, ein DC-Blocker (Plaits' Kurven tragen
  Gleichanteil, das Modul koppelt seinen Ausgang kapazitiv aus), `voice_amp`.
- Regler: SHAPE-Seite HARM TIMB MORPH AUX (Plaits' HARMONICS, TIMBRE, MORPH; AUX blendet in den zweiten Ausgang),
  FLT-Seite CUT RES DRV KTR. HOME: HARM TIMB MORPH CUT. Die SHP-Routings bewegen TIMB.
- Eine Stimme aus der Ruhe startet auf ihrem Ton (kein Gleiten hinein), ihre Phase aus `ph[0]` (UNISON spreizt sie);
  ein Retrigger einer klingenden Stimme lässt das Modell weiterlaufen.
- Acht Presets: SOFT FOLD, FOLD BASS, GLASS LEAD, WARM PAD, BUZZ SAW, SOFT KEYS, METAL PLUCK, SWEEP. PHASE-Sounds laden
  als SOFT FOLD (`eng_successor`: PHASE → WSHAPE).
- In der Liste nach FM6 (`ENGINE_ORDER`), wo PHASE stand; EDIT-Tasten jetzt 12.
- Test `tests/wshape_test.c`: alle Presets C1..C7 ohne DC und begrenzt, die Ecken innerhalb des Filterknies, C4
  gestimmt ab den ersten 10 ms, MONO-Retrigger, UNISON-Phasen, SHP, Determinismus, Engine-Wechsel mit GRAIN PHYS DRUM
  FM6 VDRUM wie vom Start; Demos in `build/wshape_demo/`. Golden: 8 neue Renders, keiner verändert.

| | vorher | nachher |
| --- | --- | --- |
| Image | 522 836 B | **524 740 B** (+1 904 B; 56 824 B frei) |
| RAM `.data`+`.bss` | 51 640 B | 52 024 B (+384 B: die Puffer des Modells) |

Der Code einer Engine kostet also ~2 KB statt der geschätzten ~3 KB. Für die vier übrigen: ~8–10 KB Code plus die
Wavetables. Mit 16 Bit (50,7 KB) sind das ~60 KB bei 56,8 KB frei: **knapp zu viel**; die Entscheidung fällt mit
WAVETABLE, wenn HARMONIC und PARTICLE gemessen sind.

Gemessen am Gerät (2026-10-24, INFO > CPU, Firmware 9e149ba): 4 Töne 39 %, 8 Töne 64 %, also **~6,3 % pro Stimme**
(das Modell 4,3 %, Filter, Drive, Hüllkurve und Umrechnung ~2 %) auf ~14 % Grundlast. Die 8 Stimmen sind das ganze
gemeinsame Budget aller Parts (`NVOICE`), ab 85 % nimmt `audio.c` Stimmen weg: WSHAPE braucht keine Grenze.

Danach sparsamer gemacht: Bei AUX 0 rechnet `pws_render` (`need` 1) Plaits' zweiten Ausgang nicht (Dreieck, Sinus,
zweiter Falter), OUT bleibt bitgleich (`tests/wshape_test.c`); das Dreieck läuft auf seiner Bahn weiter
(`pws_tri_follow`), AUX kommt danach innerhalb von 0,001 wieder. Bei AUX 127 fällt der erste Falter weg. Statisch
kostet das Modell bei AUX 0 ~224 statt ~385 pro Sample (~40 % weniger): erwartet ~4,5 % pro Stimme, 8 Töne ~50 %.
Noch einmal am Gerät messen.

## 9. Schritt 5b: HARMONIC (Engine 18, `HRMNC`)

- `plaits_add.c` (MIT): Plaits' Additive-Engine, bitgleich mit Plaits (`tools/plaits_proto/ref_add.cc`: Differenz 0).
  24 Sinus-Teiltöne (zwei Batches zu 12 aus einer Tschebyscheff-Rekursion), AUX die Orgel-Teiltöne 1 2 3 4 6 8 10 12.
  Die drei Oszillatoren von Plaits haben dieselbe Phase (gleiche Frequenz, gleicher Start), deshalb nur eine.
- Das Spektrum wird einmal pro Block geglättet. Plaits: 0,001 pro 12-Sample-Block bei 48 kHz; bei 32 Samples und
  44,1 kHz ist es für dieselbe Zeit 0,0029 (`HAR_LP`).
- `plaits_voice.c`: der Felucca-Teil beider Engines (Parameter, Mischung OUT/AUX, Drive, Filter, DC-Blocker, die acht
  Regler). WSHAPE nutzt ihn jetzt auch, bitgleich wie vorher (Golden: 0 verändert).
- Regler wie WSHAPE: HARM (Plaits HARMONICS: Nebenbuckel), TIMB (Lage des Spektrum-Gipfels, Grundton bis Teilton 24),
  MORPH (Breite: 0 breit und hell, 127 ein einziger Teilton), AUX (Orgel). Bei AUX 0 wird die Orgel nicht gerechnet.
- Acht Presets: DRAWBARS, AIR PAD, BRIGHT LEAD, SUB BASS, GLASS BELL, VOWEL, COMB KEYS, SPECTRAL. TRIO-Sounds laden
  als DRAWBARS (`eng_successor`: TRIO → HRMNC). In der Liste nach WSHAPE; EDIT-Tasten jetzt 13. Angezeigt als
  „HRMNC“: „HARMON“ ist auf der PRESETS-Seite zu breit (der Layout-Test schneidet es ab).
- Hohe Töne mit hellem Spektrum werden leiser: Plaits blendet Teiltöne über der halben Abtastrate aus (bei C7 sind
  GLASS BELL und SPECTRAL still). Der Test prüft deshalb „klingt“ nur bis C6.
- Test `tests/harm_test.c` (wie der von WSHAPE). Golden: 8 neue Renders.
- `tests/target_budget.py`: objdump kennt die 4-Byte-FPU-Befehle nicht (`3f e5`/`3f f5`) und las ihre zweite Hälfte
  als eigenen Befehl, manchmal als Sprung: daher die scheinbar verschachtelten Schleifen. Jetzt zählt ein
  FPU-Befehl einmal; die Ganzzahl-Funktionen behalten ihre Werte.

| | vorher | nachher |
| --- | --- | --- |
| Image | 525 316 B | **527 544 B** (+2 228 B; 54 020 B frei) |
| RAM `.data`+`.bss` | 52 024 B | 52 024 B |

Rechenzeit (statisch, Befehle pro Sample, ohne Filter): ~400 bei AUX 0 (nur OUT), ~244 bei AUX 127 (nur die Orgel),
~556 dazwischen (beide); WSHAPE ~224 bei AUX 0, ~385 mit beiden (gemessen ~6,3 % pro Stimme mit beiden). Erwartet für
HRMNC pro Stimme: **~6,5 % bei AUX 0, ~4,7 % bei AUX 127, ~8 % dazwischen**; 8 Töne auf ~14 % Grundlast also ~66 %,
~52 %, ~80 %. Mit AUX zwischen 1 und 126 an der Grenze, ab der `audio.c` Stimmen wegnimmt (85 %). Am Gerät messen
(je 8 Töne: BRIGHT LEAD, AUX 0; DRAWBARS, AUX 127; AIR PAD, AUX 30); danach entscheiden, ob HRMNC eine Stimmengrenze
braucht (`engine_t.poly`, wie FM6 6).

## 10. Schritt 5c: PARTICLE (Engine 19, `PRTCL`)

- `plaits_part.c` (MIT): Plaits' Particle-Engine mit ihren 6 Partikeln (Zufallsimpulse durch je einen Bandpass), dem
  Tiefpass am Ton und dem Diffusor; dazu stmlibs SVF, Zufallsgenerator und `SemitonesToRatio` (mit seinen Tabellen,
  `stmlib_tables.h`, 2 KB). Bitgleich mit Plaits (`tools/plaits_proto/ref_part.cc` mit Plaits' Delay-Längen: Differenz
  0, auch Diffusor und Zufallsfolge).
- Kein `sqrtf` in der Firmware (keine libc): `pun_sqrt` rechnet die Wurzel ganzzahlig, korrekt gerundet wie IEEE
  (geprüft: 57 Mio. Werte, alle gleich `sqrtf`). Sie läuft nur, wenn ein Partikel feuert, und einmal pro Block.
- **Diffusor einmal pro Part** (wie besprochen), hinter den Stimmen (`engine_t.post`, wie der Resonator von VDRUM);
  Delays auf **7/10** gekürzt: 5670 Samples (11,3 KB) statt 8100, damit Partikel (8 Stimmen × 164 B) und Diffusor in
  den Engine-Speicher eines Parts passen (12 888 B). Der Ring ist kein Zweierpotenz-Puffer mehr (Vergleich statt Maske).
  Bei MORPH ab 64 läuft er nicht; still und ohne Nachhall auch nicht.
- Ein Zufallsgenerator pro Part (in seinem Speicher): Ein Part aus der Ruhe spielt eine Note immer gleich.
- Dichte pro Sekunde wie am Modul (`PRT_RATE`: 47 872 Hz des Moduls gegen unsere 44 100 Hz).
- Ein Ton feuert alle Partikel (Plaits' Trigger) im zweiten Block: im ersten steigt die Hüllkurve noch von 0 an und
  würde den Impuls verschlucken (0,7 ms später, nicht hörbar).
- Regler: HARM (Verstimmung der Bandpässe, bis ±4 Oktaven), TIMB (Dichte), MORPH (unter 64 Diffusor, über 64 Resonanz),
  AUX (die rohen Impulse). Acht Presets: DUST CLOUD, RAIN, RESO PING, CRACKLE, GLITTER PAD, WOOD TAPS, GEIGER, SHIMMER.
  In der Liste nach HRMNC; EDIT-Tasten jetzt 14. Kein Nachfolger eines alten Engines (DBOX bleibt DRUM).
- Test `tests/part_test.c`; Golden: 8 neue Renders.

| | vorher | nachher |
| --- | --- | --- |
| Image | 527 544 B | **533 248 B** (+5 704 B; 48 316 B frei) |
| RAM `.data`+`.bss` | 52 024 B | 52 152 B |

Rechenzeit (statisch): ~255 Befehle pro Sample und Stimme (WSHAPE voll ~385), also erwartet **~5 % pro Stimme**; der
Diffusor ~200 pro Sample und Part (~2 %), nur bei MORPH unter 64. 8 Töne DUST CLOUD erwartet ~55 %. Am Gerät messen.

Platz für den Rest: 48 KB frei. WAVETABLE mit 8-Bit-Tabellen (25,3 KB) plus Code passt, 16 Bit (50,7 KB) nicht mehr;
nach der Entscheidung „16 Bit wenn es passt, sonst 8 Bit“ also **8 Bit**.

## 11. Schritt 5d: WAVETABLE (Engine 20, `WTABLE`)

- `plaits_wt.c` (MIT): Plaits' Wavetable-Engine; bitgleich mit Plaits bei Plaits' 16-Bit-Tabellen
  (`tools/plaits_proto/ref_wt.cc`, `PWT_FULL`: Differenz 0).
- **8 Bit** (die Regel „16 Bit wenn es passt, sonst 8 Bit“: 16 Bit hätten 50,7 KB gebraucht, frei waren 48 KB). Plaits'
  Tabellen sind *integrierte* Wellen (beim Spielen differenziert: das ist ihr Anti-Aliasing); deren Schritte sind 11 Bit
  breit, 8 Bit direkt auf die integrierte Welle wären nur ~27 dB sauber. Deshalb (`tools/plaits_proto/gen_wt8.py`):
  das Integral auf einem Raster von 9, gespeichert als erster Wert und 131 Schritte zu 8 Bit; das Raster wird bei jedem
  Sample neu gerundet, der Fehler summiert sich nicht (jeder Wert ±4 von Plaits', jede Periode schließt wie bei Plaits).
  Beim Spielen **49 dB** Abstand zu Plaits' 16 Bit. 25,5 KB statt 50,7.
- Dekodiert wird eine Welle, wenn eine Stimme sie liest, in einen Cache pro Part (44 Wellen à 264 B im Engine-Speicher;
  die Stimmen eines Parts brauchen höchstens 32: TIMB pro Stimme, MORPH und HARM pro Part). Im Test: 8 Stimmen auf
  eigenen Reihen, MORPH über die ganze Tabelle: 26 Dekodierungen in 2000 Blöcken.
- Regler: HARM die Bank (ab der Mitte rückwärts und in Stufen), TIMB die Reihe, MORPH die Spalte, AUX die 5-Bit-Kopie.
  VOICE-Sounds laden als WAVE SCAN.

## 12. Schritt 5e: SWARM (Engine 21, `SWARM`)

- `plaits_swarm.c` (MIT): Plaits' Swarm-Engine (8 Sägezähne und 8 Sinus pro Stimme unter Grain-Hüllkurven); bitgleich
  mit Plaits, mit und ohne Trigger (`tools/plaits_proto/ref_swarm.cc`).
- Jeder Ton startet einen **Burst** (Plaits mit gestecktem Trigger, wie die Tasten des Grind): der Schwarm startet
  schnell und wird dann ruhiger. (Ohne Trigger läuft er gleichmäßig; das wäre eine Option, wenn dir der Burst bei
  langen Flächen nicht gefällt.)
- SLICE-Sounds laden als SAW CLOUD.

## 13. Pegel wie in Plaits

Plaits gleicht seine Engines in `voice.cc` aus: Waveshaping 0,7 (AUX 0,6), Additiv 0,8, Wavetable 0,6, Particle ×2
und Swarm ×3 jeweils in stmlibs Limiter. Das machen jetzt alle fünf (`plaits_voice.c` `plv_blend`); WSHAPE, HRMNC und
PRTCL klingen dadurch etwas anders laut als im ZIP davor (die Golden-Renders der fünf neu, alle anderen unverändert).

## 14. Stand

| | vor Schritt 4 | jetzt |
| --- | --- | --- |
| Image | 569 516 B | **575 672 B** (5 892 B frei; mit SCL > CSET aus `claude/cset-chords`: +8 876 B) |
| RAM `.data`+`.bss` | 56 616 B | 52 152 B |
| Engines | ANALOG LOFI SAMPLE WHEEL GRAIN PHYS DRUM NOISE FM6 ANLG2 VDRUM + PHASE VOICE TRIO SLICE DBOX | dieselben elf + WSHAPE HRMNC PRTCL WTABLE SWARM |

Am Gerät zu messen: siehe `docs/CHECKLISTE.md`.
