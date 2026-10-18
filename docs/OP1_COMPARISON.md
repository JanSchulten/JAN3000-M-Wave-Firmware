# OP-1 (erste Generation) und M-VAVE FM-1 im Vergleich

Grundlage: das OP-1 Operator's Manual 1.2 ([docs/reference/op1-manual.md](reference/op1-manual.md)), Felucca 1.5.1
und der Stand von M-Vave-OP1 mit Meilenstein M1 (TAPE). Gemeint ist immer das **originale OP-1**, nicht das OP-1 field.

> **Stand M6:** Das Noten-TAPE ist entfernt (es spielte nicht gut). An seiner Stelle steht GLO > **KEY**, eine
> Akkordfolge wie die Key-Spur des OP-Z (siehe README). Alles, was unten zum Tape steht, beschreibt den früheren Stand.

Legende für den Stand auf dem FM-1: ✅ vorhanden, 🟡 teilweise oder anders gelöst, ❌ fehlt, ⛔ mit der Hardware nicht
möglich.

## 1. Die Geräte

| | OP-1 (2012) | FM-1 |
| --- | --- | --- |
| Rechner | 400 MHz, 800 MMACS | JieLi AC791N (pi32v2) |
| RAM | 64 MB SDRAM | ~512 KB SRAM |
| Speicher | 512 MB NAND | 1 MB SPI-Flash, davon ~570 KB Firmware |
| Audio | 24 Bit / 96 kHz, Line-In, Line-Out, Mikrofon, FM-Radio | Stereo-Out, Lautsprecher, USB-Audio zum Rechner |
| Display | 320 × 160 AMOLED, 60 fps | 240 × 240 TFT (ST7789) über SPI |
| Tasten | 24 Tasten, 4 Farb-Encoder, Volume, ~30 Funktionstasten | 27 Tasten, 4 Knobs + SELECT, ALGORITHM, PRESETS, MASTER, 14 Funktionstasten |
| Sensoren | G-Force (3-Achsen), Mikrofon, Radio | keine bekannt |
| Verbindungen | USB 2.0 (MIDI, Speicher), 3,5 mm In/Out | USB-C (MIDI, Audio zum Rechner), TRS-MIDI-In |

Daraus folgt der wichtigste Unterschied: Das OP-1 ist ein **Audio-Gerät** (Tape, Album und Sampler speichern Klang).
Das FM-1 hat dafür rund 100-mal zu wenig Speicher. M-Vave-OP1 baut den Workflow deshalb auf **Noten und
Parametern** auf und spielt sie live durch die Engines.

## 2. Bedienelemente: was wird was?

| OP-1 | Aufgabe dort | FM-1 heute | Vorschlag |
| --- | --- | --- | --- |
| Volume | Master-Lautstärke | MASTER ✅ | – |
| Blau, Grün, Weiß, Orange | die vier Parameter des Bildschirms | KNOB 1–4 ✅ | in der Grafik konsequent mit diesen Farben kennzeichnen |
| SYNTH, DRUM, TAPE, MIXER (Main Modes) | die vier Hauptmodi | verteilt: HOME, GLO > TAPE, HOME > MIXER 🟡 | eine feste Modus-Zuordnung, siehe Schritt 1 |
| T1–T4 (Synth) | Engine, Hüllkurve, Effekt, LFO | **EDIT, ENV, FX, LFO** ✅ (dieselben vier Module!) | – |
| T1–T4 (Tape) | Spur 1–4 | ALGORITHM wählt die Spur 🟡 | – |
| T1–T4 (Mixer) | Mixer, EQ, Master-FX, Master-Out | HOME > MIXER, CLOCK 🟡 | Schritt 6 |
| Sound-Tasten 1–8 | Sounds wählen | PRESETS, SAVE > USER 🟡 | – |
| Tape-Tricks 1–8 | LOOP IN/OUT/TOGGLE, BREAK, REVERSE, CHOP, M1, M2 | FX halten: REPEAT, REVERSE, TAPE STOP, FREEZE … 🟡 | Schritt 2: Tape-Tricks in der FX-Ebene |
| LIFT, DROP, SPLIT (JOIN) | Takes schneiden und einfügen | ❌ | Schritt 4 |
| REC, PLAY, STOP | Transport | REC, PLAY (PLAY = STOP) ✅ | – |
| ◀ ▶ | Oktave (Synth), REW/FF (Tape), Bar-Sprung mit SHIFT | OCT− / OCT+ (Oktave) 🟡 | auf der TAPE-Seite REW/FF, Schritt 2 |
| SHIFT | zweite Ebene fast jeder Taste | keine SHIFT-Taste; Felucca nutzt gehaltene Tasten (FX, GLO, SCL, EDIT, REC, SEQ) 🟡 | bei gehaltenen Ebenen bleiben, siehe Abschnitt 6 |
| Sequencer-Taste | Sequencer an, SHIFT: Auswahl | SEQ ✅ | Auswahl der Sequencer-Art, Schritt 8 |
| HELP | Sprechblase, Tastennamen, Akku | MENU > HELP ON 🟡 | – |
| TEMPO | BPM, Tap, Metronom, Beat Match | HOME > CLOCK, Klick im MENU 🟡 | Beat Match, Schritt 7 |
| MIC/INPUT | Mikrofon, Radio, Line-In | ⛔ kein Audioeingang | – |
| ALBUM / COM | Mixdown, USB-Modi | ⛔ Album; USB-MIDI ✅, Web-Editor ✅ | Mitschnitt über USB-Audio am Rechner |

## 3. Der OP-1-Workflow und sein Gegenstück

Das Handbuch beschreibt den Ablauf so: *Sound erstellen → sequenzieren → auf Tape aufnehmen → mischen → Album.*

| Schritt | OP-1 | FM-1 heute | Lücke |
| --- | --- | --- | --- |
| 1. Sound wählen | SYNTH oder DRUM, Sound-Taste 1–8 | ALGORITHM (Spur), PRESETS (Sound) | Synth und Drum sind auf dem FM-1 einfach Spuren mit verschiedenen Engines; das passt |
| 2. Sound formen | T1–T4 + Farb-Encoder, SHIFT+T1 tauscht nur die Engine | EDIT, ENV, FX, LFO + KNOB 1–4; EDIT halten tauscht die Engine | kaum; Grafiken im OP-1-Stil fehlen (Schritt 9) |
| 3. Sequenzieren | 4 Sequencer, Tasten transponieren die Sequenz | Step-Sequencer (64 Schritte), ARP, Orchid-Akkorde (SCL > ORCHID), SONG | ENDLESS, TOMBOLA, FINGER; Transponieren per Taste (Schritt 8) |
| 4. Aufnehmen | TAPE, Spur, REC + PLAY | GLO > TAPE, REC | ✅ seit M1 |
| 5. Tape bearbeiten | Lift, Drop, Split, Tricks, Speed, Reverse | Loop, Speed, Rückwärts, Löschen | Lift/Drop/Split, Tricks, Sprünge (Schritte 2–4) |
| 6. Mischen | Pegel/Pan, EQ, Master-FX, Drive | Pegel, Pan, Hall, Mute pro Spur, Limiter | EQ, Master-FX, Drive (Schritt 6) |
| 7. Album | Mixdown auf Seite A/B | ⛔ | über USB-Audio am Rechner aufnehmen |

### Der grundlegende Unterschied: Noten statt Audio

Auf dem OP-1 ist eine Tape-Spur Klang. Man spielt Sound 1 auf Spur 1, wechselt den Sound und spielt auf Spur 2,
wechselt wieder … Das Tape hält beliebig viele Sounds über die Zeit fest. Auf dem FM-1 spielt Tape-Spur *k* durch
die Engine von Spur *k*. Daraus folgen drei Unterschiede:

1. Ein späterer Soundwechsel ändert die ganze Aufnahme dieser Spur (Vorteil und Nachteil zugleich).
2. Knob-Bewegungen während der Aufnahme (ein Filter-Sweep) sind noch nicht auf dem Tape.
3. Mehr als vier Sounds gleichzeitig gehen nicht.

Lösung in Schritt 5: Das Tape zeichnet auch **Soundwechsel und Knob-Bewegungen** auf. Dann klingt eine Aufnahme so,
wie sie gespielt wurde, und bleibt trotzdem veränderbar.

## 4. Komponente für Komponente

### Synth-Engines

| OP-1 | Bestes Gegenstück auf dem FM-1 | Stand |
| --- | --- | --- |
| FM (4 Operatoren: Topology, Freq, FM Amount, Detune) | FM6 (6 Operatoren, 32 Algorithmen, DX7-artig) | 🟡 mehr als das OP-1, aber keine 4-Regler-Makroseite |
| Cluster (bis 6 Oszillatoren) | TRIO (3 Oszillatoren), ANALOG mit UNISON | 🟡 |
| Digital (Wave Shaper, Octave, Ring Mod, Digitalness) | LOFI (Chip-Wellen) | 🟡 |
| String (Waveguide) | PHYS (Saiten, Modal, Rings-Saiten) | ✅ |
| Pulse (zwei Pulszüge) | ANALOG (Puls), LOFI PULSE | 🟡 |
| Phase (Phase Distortion) | PHASE (CrispyZebra-Oszillator) | ✅ |
| Dr Wave (Frequenzbereich) | WHEEL (additiv, 9 Teiltöne) | 🟡 |
| Synth-Sampler (6 s) | SAMPLE (3 Nutzer-Slots, ~7 s bei 22 kHz) | 🟡 Samples kommen per USB, nicht über ein Mikrofon |
| Drum-Sampler (12 s, Teile auf Tasten) | SLICE (Slices auf den Tasten) | ✅ das Prinzip ist dasselbe |
| DBOX (Drum-Synth) | DRUM (synthetische Kits, GM-Belegung) | ✅ |
| – | VOICE (Formant), GRAIN, NOISE | Extras des FM-1 |

### Hüllkurve, Play Mode, Effekte, LFO

| OP-1 | FM-1 | Stand |
| --- | --- | --- |
| ADSR auf T2, Farben Blau/Grün/Weiß/Orange = A/D/S/R | ENV: KNOB 1–4 = A/D/S/R | ✅ |
| Play Mode: POLY, MONO, LEGATO, UNISON, Portamento | EDIT > VOICE: POLY, MONO, LEGATO, UNISON, GLIDE | ✅ |
| Dynamic Envelope (Drums) | DRUM-Engine mit eigenen Hüllkurven | 🟡 |
| ein Effekt pro Sound (T3): Delay, Grid, Nitro, Phone, Punch, Spring | pro Spur DIST, INSERT (Drive, Crush, Phaser, Flanger, Chorus), SLICER; Sends Chorus, Delay, Reverb (ROOM/SPRING/HALL) | 🟡 Spring ✅, Delay ✅ (Send), Punch/Nitro ≈ Filter/Drive, Grid ≈ HALL, Phone ≈ CRUSH |
| LFO VALUE (ein Parameter) | LFO + LFO DEST | ✅ |
| LFO TREMOLO (Tonhöhe + Lautstärke) | LFO mit Ziel PIT und AMP | ✅ |
| LFO RANDOM (alle Parameter eines Moduls) | MOD-Matrix mit S&H | 🟡 nur einzelne Ziele |
| LFO MIDI (bis 4 CCs) | MOD-Matrix mit CC-Quellen, MIDI LEARN | ✅ |
| LFO ELEMENT (G-Force, Mikro, Radio) | ⛔ keine Sensoren | – |
| LFO CRANK, BEND (Zubehör) | ❌ | ein freier Regler als „Kurbel“ wäre möglich |
| Free LFO („F“) | LFO 2: TRIG FREE | ✅ |
| Takt-synchroner LFO (Uhr mit Zahl) | LFO 2: SYNC | ✅ |

### Sequencer

| OP-1 | FM-1 | Stand |
| --- | --- | --- |
| ENDLESS: Noten der Reihe nach eingeben (bis 99), per Taste transponiert abspielen, Richtung, Notenlänge, Swing, Rhythmusmuster, Crank | ❌ | Schritt 8 |
| PATTERN: 16-Step-Raster, Live-Mode, Länge, Abschnitt verschieben | Step-Sequencer (64 Schritte, Akkorde, Chance, Ratchets, Locks, Automation) | ✅ deutlich mehr als das OP-1 |
| TOMBOLA: Noten springen in einem drehenden Sechseck | ❌ | Schritt 8 |
| FINGER: zwei Spieler, Muster pro weißer Taste, JOIN/REPLACE/FILL IN | ❌ | Schritt 8 |
| Transponieren einer Sequenz mit der Tastatur | 🟡 SCL > TRN, nicht per Taste | Schritt 8 |
| – | ARP mit vielen Modi; SCL > ORCHID, die Akkord-Tastatur des Orchid (nach ChoralRoot FM-1: Akkordblock F3 .. C#4, Grundtöne D4 .. G5, VOICE, LOCK); Akkord-Tasten (CHRD), SONG-Kette | Extras des FM-1 |
| (OP-Z) Pattern-Ketten, nächstes Pattern vormerken | SONG: REC + A–D tippt eine Kette, LOOP, Änderungen im laufenden Song | ✅ seit M2b |
| (OP-Z) 16 Patterns pro Projekt | SZENEN: SEQ halten + weiße Taste, OCT+ / OCT− die nächste / vorige; im Spiel am Ende des Patterns | ✅ seit M2c |
| (OP-Z) FX-Spuren FX1/FX2 und aufgenommene Punch-in-FX | FX > FX TRACKS: zwei Effekt-Spuren, REC + FX halten + Tasten nimmt auf | ✅ seit M2b |

### Tape (entfernt in M6, durch GLO > KEY ersetzt)

| OP-1 | M-Vave-OP1 | Stand |
| --- | --- | --- |
| 4 Spuren, 6 Minuten Audio | 4 Spuren, 128 Takte Noten (~4 min bei 120 BPM), ~2000 Noten | ✅ als Noten-Tape |
| REC + PLAY, Overdub | REC armt, PLAY läuft; Overdub | ✅ |
| Rewind / Fast Forward (◀ ▶) | KNOB 1 POS | 🟡 Schritt 2 |
| STOP + ◀ / ▶: an den Anfang / ans Ende | GLO + PLAY: Neustart am Loop-Anfang | 🟡 Schritt 2 |
| SHIFT + ◀ ▶: Takt-Sprung, Loop verschieben | ❌ | Schritt 2 |
| Loop In / Out / Toggle | KNOB 2 LOOP (1–32 Takte ab dem aktuellen Takt) | 🟡 Schritt 2: freie Punkte |
| Rückwärts (SHIFT + PLAY, Trick 5) | KNOB 4 DIR | ✅ |
| Bandgeschwindigkeit, auch beim Aufnehmen | KNOB 3 SPEED −12…+12 Halbtöne | ✅ |
| BREAK, CHOP | FX-Ebene: TAPE STOP, REPEAT (auf das Master-Audio) | 🟡 Schritt 2 |
| MEMO 1/2 (Parameter merken, abrufen) | ❌ | Schritt 2 |
| LIFT, DROP, SPLIT, JOIN, LIFT ALL, Region-Lift | ❌ | Schritt 4 |
| Erase: SHIFT + TAPE, alle 4 Spurtasten | TAPE 2: ERASE Spur oder alles, mit Rückfrage | ✅ |
| Aufnahmepegel (Orange) | ❌ | Velocity-Skalierung, Schritt 2 |
| Takt-Markierungen (Beat Match) | Takt-Striche über den Spuren | ✅ |
| Manuelles Bandziehen (SHIFT + REC, Blau) | ❌ | denkbar mit KNOB 1, später |
| Bounce (Out-to-In) | 🟡 Spuren zusammenführen | Schritt 4 |
| Tape bleibt nach dem Ausschalten | ein Tape wie beim OP-1, im Flash des früheren Sample-Slots USR3 (zwei Kopien) | ✅ seit M2 |
| Sounds als Daten aufs Tape (Lift/Drop eines Sounds) | ❌ | Schritt 5 |

### Mixer, Tempo, Sonstiges

| OP-1 | FM-1 | Stand |
| --- | --- | --- |
| Pegel 0–99 und Pan pro Spur (Farb-Encoder, SHIFT = Pan) | MIXER: LEVEL, PAN, REVERB, MUTE der gewählten Spur; MENU > DISPLAY > HOME LEVELS: alle vier Pegel auf HOME | ✅ |
| Master-EQ (Low, Mid, High, Amount) | ❌ | Schritt 6 |
| Master-Effekt (T3) | 🟡 FX-Ebene, Sends | Schritt 6 |
| Master-Out: Balance, DRIVE, RELEASE | 🟡 Limiter | Schritt 6 |
| Signal-Path-Ansicht (SHIFT + MIXER) | ❌ | Schritt 10 |
| BPM, Tap-Tempo | HOME > CLOCK, GLO halten auf CLOCK | ✅ |
| Metronom | MENU > CLICK, Count-in | ✅ |
| FREE / BEAT MATCH / SYNC | INT / externe MIDI-Clock | 🟡 Beat Match fehlt, Schritt 7 |
| Hilfe-Sprechblase, Tastennamen | MENU > HELP ON: Hinweise pro Seite | 🟡 |
| Album, Disk-Modus, Mikrofon, Radio | ⛔ | – |
| MIDI-Ausgabe der Sequencer | 🟡 die Tasten senden MIDI, Sequencer nicht | später |

## 5. Schritt-für-Schritt-Plan

Die Reihenfolge folgt dem OP-1-Workflow: zuerst was beim Aufnehmen und Bearbeiten am meisten fehlt, dann Mischen und
Sequencer, zuletzt die Optik. Jeder Schritt kommt mit Host-Tests und einem neuen Workflow im Emulator
(`web/lab`).

| # | Schritt | OP-1-Vorbild | Bedienung auf dem FM-1 (Vorschlag) | Aufwand |
| --- | --- | --- | --- | --- |
| 1 | **Modus-Zuordnung** | die vier Main Modes | HOME = Synth (Spur, Sound), GLO = Tape, HOME HOME = Mixer, SEQ = Sequencer; Seitentitel und Farben nach OP-1-Art | klein |
| 2 | **Tape-Transport und Tricks** | ◀ ▶, STOP + ◀ ▶, Tape-Tricks 1–8 | auf der TAPE-Seite OCT− / OCT+ = REW / FF (halten spult), PLAY halten + OCT− = an den Anfang; FX halten auf der TAPE-Seite: LOOP IN, LOOP OUT, LOOP TOGGLE, BREAK, REVERSE, CHOP, M1, M2 auf den weißen Tasten | mittel |
| 3 | **Tape speichern (M2)** ✅ | Tape bleibt beim Ausschalten | erledigt: ein Tape im Flash von USR3, nach 3 s Ruhe gesichert | – |
| 4 | **Tape-Schnitt** | LIFT, DROP, SPLIT, JOIN, Region-Lift | TAPE 2: LIFT (Loop-Bereich in die Zwischenablage), DROP (am Kopf einfügen), SPLIT/JOIN über Lücken; LIFT ALL | mittel |
| 5 | **Tape nimmt Klang mit** | Audio enthält jeden Sound und jeden Sweep | Soundwechsel (PRESETS) und Knob-Bewegungen werden Tape-Events; beim Abspielen gesetzt | groß |
| 6 | **Master wie beim OP-1** | EQ (T2), Master-FX (T3), Drive + Release (T4) | MIXER-Seiten 2–4: EQ LOW MID HIGH AMOUNT, MASTER FX, DRIVE RELEASE BALANCE | mittel, CPU prüfen |
| 7 | **Beat Match** | Tempo an die Bandgeschwindigkeit koppeln | CLOCK: LINK; dann ändert SPEED das Tempo ohne Tonhöhe, Sequencer folgen | klein |
| 8 | **Sequencer** | ENDLESS, dann TOMBOLA, FINGER; Transponieren per Taste | SEQ halten: Art wählen; ENDLESS zuerst (einfach, sehr OP-1-typisch) | mittel bis groß |
| 9 | **OP-1-Optik** | Engine-Grafiken, farbige Werte | pro Engine eine verspielte Grafik (FM-Sechsecke, Saiten-Kurven, Phase-Wellen), Werte in den Encoder-Farben | mittel |
| 10 | **Kleinkram** | Signal Path, Hilfe, LFO-Arten | Signal-Path-Seite, RANDOM-LFO für ein ganzes Modul, „Kurbel“ auf SELECT | klein |

Grenzen, die bei jedem Schritt mitgeprüft werden: statisches RAM (zurzeit 56 152 von 73 728 Byte), Pool-RAM (~23 KB frei,
davon 8 KB Pflichtreserve), statische ISR-Kosten (zurzeit 39 032 von erlaubten 41 252), Flash für Code (~71 KB frei).

## 6. Offene Entscheidungen

1. ~~**Wo wird das Tape gespeichert?**~~ Entschieden: Noten und Einstellungen statt eines dritten Sample-Slots. USR3 (80 KB)
   hält das Tape (ein Tape, zwei Kopien) und die FX-Spuren jedes Projekts.
2. **SHIFT-Taste ja oder nein?** Das OP-1 lebt von SHIFT. Felucca löst dasselbe mit gehaltenen Tasten, die nur auf
   bestimmten Seiten eine Ebene öffnen. Empfehlung: bei gehaltenen Tasten bleiben und auf der TAPE-Seite die
   FX-Ebene für die Tape-Tricks nutzen, statt eine Taste umzuwidmen.
3. **Soll SPEED die Sequencer mitziehen?** Beim OP-1 nur mit Beat Match. Empfehlung: wie beim OP-1, Schritt 7.

## 7. OP-Z-Anleihen (M2b)

Gewünscht waren sechs gleichwertige Spuren. Gemessen: jede weitere Synth-Spur kostet ~23 KB RAM (Engine-Speicher,
Stimmen, SLICER-Puffer), zwei zusammen ~52 KB, frei sind ~16 KB. Daher das OP-Z-Modell: **vier Instrument-Spuren** (jede
mit jeder Engine, wie bisher) und **zwei FX-Spuren**, die nur Schritte speichern (66 Byte je Spur).

| OP-Z | M-Vave-OP1 | Stand |
| --- | --- | --- |
| Spuren 1–8 (Drums, Synths) | vier Spuren, jede mit beliebiger Engine (DRUM, FM6, SAMPLE, SLICE …) | ✅ |
| FX1 / FX2 (Send-Effekte mit Schritten) | FX TRACKS: zwei Spuren mit den Effekten der FX-Ebene, eigene Länge 1–64 | ✅ |
| Punch-in-FX (SHIFT + Tasten) | FX halten + weiße Tasten (Effekte), schwarze 1–4 (Mutes) | ✅ (aus Felucca) |
| Punch-in-FX mit REC aufnehmen | REC auf FX TRACKS armt eine Spur; quantisiert, gehalten = TIE, Overdub, Punch | ✅ |
| Pattern-Ketten (Projekt-Taste + Pattern-Tasten) | SONG: REC, dann A3 B3 C4 D4 = Pattern A–D; gleiche Taste = noch einmal | ✅ |
| Kette läuft in Schleife | LOOP (bei getippten Ketten an), END auf + ADD: LOOP / STOP | ✅ |
| Pattern während des Spielens wechseln | neue Kette ab dem Ende des laufenden Patterns, Änderungen am Abschnittsende | ✅ |
| 16 Patterns pro Projekt, sofort umschaltbar | SZENEN: 16 pro Projekt (Schritte, Timing, Automation, FX-Spuren), SEQ + weiße Taste; gestoppt sofort, im Spiel am Ende des Patterns | ✅ seit M2c |
| (KO-II) Punch-in halten und dabei die Sequenz wechseln | FX + Effekt-Taste, FX loslassen (der Effekt bleibt), SEQ + weiße Taste | ✅ seit M2c |
| Weitere Punch-ins: Duck, Ramp, Randomize, Stereo | FX MAP B (FX + ALGORITHM): DUCK, STEREO, PITCH, RAMP UP, GLITCH | ✅ seit E2 |

Speicher: Projekte haben Felluccas Format (ein Sektor, voll). Die FX-Spuren liegen daneben im Flash von USR3, die Szenen
im Flash von USR2, beide mit der Prüfsumme des Projekts, zu dem sie gehören. Ein Felucca-Projekt oder eines, das der
Web-Editor geschrieben hat, lädt mit leeren FX-Spuren und nur seiner eigenen Szene (als Szene 1).
