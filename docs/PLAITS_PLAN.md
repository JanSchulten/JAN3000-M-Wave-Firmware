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
| SLICE | **SWARM** | SAMPLE mit derselben Quelle (Vorschlag, noch offen) |

- Fünf eigene Engines, keine Sammel-Engine mit MODEL-Parameter.
- Die neuen Engines werden an `ENGINES[]` angehängt (17 … 21); die alten Nummern bleiben reserviert und werden beim
  Laden umgewandelt, wie DIGITAL → FM6 (`fm4_convert.c`). Im PRESETS-Menü rücken die neuen an die Stelle der alten.
- Polyphonie: so viele Stimmen, wie die CPU schafft, pro Engine gemessen.
- Werks-Presets bringen keine Effekte mehr mit (umgesetzt, Abschnitt 4).

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

## 3. Offene Entscheidungen

1. **Diffusor von PARTICLE:** 16 KB passen nicht in den Engine-Speicher eines Parts (12 888 B). Möglich: einer pro
   Part mit auf ~6 000 Samples gekürzten Verzögerungen (12 KB, passt; etwas kürzere Fahne), ein gemeinsamer für alle
   PARTICLE-Spuren (16 KB Pool), oder ganz ohne und stattdessen der Reverb-Send.
2. **Wavetables:** 16 Bit (50,7 KB) oder 8 Bit (25,3 KB); hängt an dem, was der Flash nach dem Aufräumen hergibt.
3. **WAVESHAPE exakt oder schnell:** nach einem Hörvergleich.
4. **SLICE-Sounds:** als SAMPLE mit derselben Quelle laden?
5. **FPU-Probe auf dem Gerät:** braucht eine Installation.

## 4. Umgesetzt: Werks-Presets ohne Effekte

Ein Werks-Preset bringt nur noch den Sound: die Sends der Spur (DIST, CHORUS, DELAY, REVERB) und ihr INSERT bleiben,
wie sie sind, beim PRESETS-Knopf, im Editor, bei Program Change, beim Wechsel der Engine und bei den zufälligen
Sounds eines neuen Projekts. Ein User-Preset bringt die Effekte mit, mit denen es gespeichert wurde. Nur die
Einschalt-Sounds laden ihre Sends wie bisher, damit das Einschalt-Projekt und Projekte im alten Format klingen wie
immer (`ui.c` `preset_fx`, `preset_load_to`; Test: `tests/preset_fx_test.c`).

## 5. Reihenfolge

1. ~~Werks-Presets ohne Effekte~~ (erledigt)
2. ~~Phase 0: Machbarkeit~~ (dieses Dokument)
3. FPU-Probe auf dem Gerät (Takte pro FPU-Befehl, Start ohne SDK)
4. Alte Engines umwandeln und entfernen (SLICE, PHASE, VOICE, TRIO, DBOX), Tests für das Laden alter Projekte
5. Neue Engines: WAVESHAPE → HARMONIC → PARTICLE → WAVETABLE → SWARM
6. Presets, UI, Golden-Tests, Emulator; erst dann aufs Gerät, nur mit Okay
