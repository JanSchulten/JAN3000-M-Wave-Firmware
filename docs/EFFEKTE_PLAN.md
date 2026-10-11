# Abgefahrene Effekte: Plan

Ziel: Effekte, die über Filter und Hall hinausgehen (Granular, Glitch, Shimmer, Ringmodulation …), angelehnt an
das OP-Z, ohne den FM-1 zu überfordern. Dieses Dokument ist der Plan; umgesetzt ist davon noch nichts
(Stand nach M2b, siehe [ROADMAP.md](ROADMAP.md)).

## 1. Was es heute schon gibt

Felucca bringt mehr mit, als man denkt. Vieles davon ist schon „abgefahren“:

| Wo | Effekt | Was er macht |
| --- | --- | --- |
| FX-Ebene (FX halten + Taste), auch auf den FX-Spuren | REPEAT 1/8 1/16 1/32 | Stutter: ein Stück des Master-Signals im Takt wiederholt |
| | REVERSE | das letzte Achtel rückwärts |
| | TAPE STOP | Bandbremse über einen Schlag |
| | FREEZE | zwei Grains frieren 186 ms Klang ein (schon granular!) |
| | OCT UP / OCT DN + SHIMMER (KNOB 4) | Harmonizer; mit SHIMMER klettert jede Runde eine Oktave höher |
| | LPF, HPF, FLANGER, PHASER | Filterfahrten, Modulation |
| | REV THROW, ECHO THROW | alles kurz voll in den Hall / ins Delay werfen |
| | KNOB 1–4: FILTER, CRUSH, THROW, DEPTH | Makros |
| Pro Spur: INSERT | SOFT, HARD, FOLD, FUZZ, CRUSH, PHASR, FLANG, CHOR | Verzerrung, Bitcrusher mit Sample-Rate, Modulation |
| Pro Spur: SLICER | Gate-Muster, STUT | rhythmisches Zerhacken, Stotter-Wiederholung |
| Sends (für alle Spuren) | CHORUS, DELAY (Tempo, punktiert), REVERB ROOM / SPRING / HALL | |
| Engines | GRAIN (granular auf Samples), PHYS (Saiten, Rings-artig), FORMANT, NOISE | granulare und physikalische Klangerzeugung |

## 2. Das Vorbild OP-Z

- **FX1 / FX2** sind Send-Effekte mit wählbarer Engine: crush, delay, dist, rymd (digitaler Hall), reverb,
  chorus-80, je zwei Parameter. Schritte auf der FX-Spur ändern die Parameter im Takt.
- **Punch-in-FX** (SHIFT + Taste, mit REC auf die Performance-Spur): Duck, Filter Sweep, Loop 1, Stereo,
  Loop 2, Pitch, Follow/Echo, Ramp Up/Fill 1, Short, Long, Ramp Down/Fill 2, Random.

| OP-Z | FM-1 heute | Plan |
| --- | --- | --- |
| delay, reverb, rymd, chorus-80 (Sends) | DELAY, ROOM / SPRING / HALL, CHORUS | ✅ vorhanden |
| crush, dist (Sends) | CRUSH, SOFT … FUZZ als INSERT pro Spur | ✅ vorhanden (pro Spur statt Send) |
| Loop 1 / Loop 2 | REPEAT 1/16, 1/32 | ✅ |
| Filter Sweep | LPF, HPF | ✅ |
| Follow / Echo | ECHO THROW | ✅ |
| Ramp Down / Fill 2 | TAPE STOP | ✅ |
| Duck | – | **neu: DUCK** (Pumpen im Viertel) |
| Stereo | – | **neu: STEREO** (schnelles Auto-Pan) |
| Pitch | OCT UP / DN | **neu: PITCH** (±5 / ±7 Halbtöne, ohne trockenes Signal) |
| Ramp Up / Fill 1 | – | **neu: RAMP UP** (Band läuft an, Gegenstück zu TAPE STOP) |
| Random | – | **neu: GLITCH** (jedes Sechzehntel ein Zufallseffekt) |
| Short / Long | – | **neu: SHORT / LONG** (Gate der Sequencer-Noten kurz / lang) |

Das OP-Z hat **keinen** Granular-Effekt. Hier gehen wir darüber hinaus.

## 3. Was der FM-1 hergibt

- **RAM:** statisch ~17 KB frei, Pool ~16 KB nutzbar (8 KB Pflichtreserve). Ein neuer großer Puffer passt nicht.
  Die Regel deshalb: **neue Effekte leihen sich vorhandene Puffer**, wie es die FX-Ebene mit dem SLICER-Puffer
  schon tut:
  - `dly_buf`: 128 KB, 1,49 s Mono bei 44,1 kHz (der Delay-Bus),
  - `sl_buf`: 32 KB, 371 ms Stereo bei 22 kHz (die SLICER-Aufnahmen, von der FX-Ebene geliehen),
  - die Hall-Puffer: ~12 KB.
- **Rechenzeit:** Auf dem Gerät noch nicht gemessen (es ist noch unterwegs). Auf dem Rechner kostet ein voller
  Mix mit 8 Stimmen ~1 600 Instruktionen pro Sample. Jeder neue Effekt bekommt eine eigene Kostenzeile in
  `tests/target_budget.py` und läuft nur, wenn er an ist (aus = bitgenau wie vorher, wie bei der FX-Ebene).
  Sobald das Gerät da ist: MENU > SYSTEM > INFO zeigt die CPU-Last; Effekte mit Grains bekommen eine
  Qualitätsstufe (4 oder 8 Grains).
- **Speicher der Einstellungen:** Felluccas globale Parameter sind fest (Projektformat, Editor-Protokoll). Neue
  Send-Parameter kommen in die Projekt-Extras (`opstore.c`), die es seit M2 gibt. INSERT-Typen und Punch-in-Effekte
  werden hinten angehängt (beide Listen sind dafür gebaut: Platz für 14 weitere Punch-ins).

## 4. Der Katalog

Kosten: geschätzte Instruktionen pro Sample auf dem Rechner (zum Vergleich: ein voller Mix ~1 600).

### A. FX1 wird wählbar: der Delay-Bus bekommt Typen (wie FX1 beim OP-Z)

Alle Spuren schicken über ihren DLY-Send hinein. Die Seite DLY wird zu **FX1** mit KNOB 1 TYPE. Jeder Typ nutzt
`dly_buf`, kein neuer Speicher.

| Typ | Was er macht | Regler | Kosten |
| --- | --- | --- | --- |
| DELAY | das heutige Tempo-Delay | TIME, FDBK, COLOR, MIX | – |
| **CLOUD** (Granular) | bis zu 8 Grains aus den letzten 1,5 s des Sends, wie Mutable Clouds: Klangwolken, Texturen, eingefrorene Flächen | SIZE, DENSITY, PITCH (inkl. Zufall bis ±1 Oktave, rückwärts), SPREAD; FREEZE als Punch-in | ~150 |
| **SHIMMER** | Delay mit Oktave nach oben in der Rückkopplung: jede Wiederholung höher, Engelschor | TIME, FDBK, PITCH (+12, +7, +5, −12), MIX | ~40 |
| **REVERSE** | Delay, das jedes Stück rückwärts zurückspielt | TIME, FDBK, COLOR, MIX | ~15 |
| **TAPE ECHO** | Bandecho: Gleichlaufschwankungen, Sättigung, dunkler werdende Wiederholungen | TIME, FDBK, WOW, MIX | ~25 |

Dazu passt die FX-Ebene: ECHO THROW wirft dann in die Wolke, ein neuer Punch-in **FREEZE CLOUD** hält sie an.
Auf den FX-Spuren lässt sich beides im Takt sequenzieren.

### B. Neue Punch-ins (FX-Ebene, Master, auf FX-Spuren aufnehmbar)

| Punch-in | Was er macht | Puffer | Kosten |
| --- | --- | --- | --- |
| **DUCK** | Sidechain-Pumpen im Viertel (OP-Z Duck) | – | ~5 |
| **STEREO** | schnelles Auto-Pan links/rechts im Sechzehntel (OP-Z Stereo) | – | ~8 |
| **PITCH** | Master ±5 / ±7 Halbtöne verstimmt (OP-Z Pitch) | Harmonizer (`sl_buf`) | ~40 |
| **RAMP UP** | Band läuft aus dem Stillstand an (OP-Z Ramp Up) | `sl_buf` | ~20 |
| **GLITCH** | jedes Sechzehntel zufällig: Repeat 1/16–1/64, rückwärts, Oktave, Stille (OP-Z Random) | `sl_buf` | wie der gewählte |
| **SCATTER** | granular: das letzte Stück in Grains zerstreut, versetzt und verstimmt | `sl_buf` | ~60 |
| **SHORT / LONG** | Noten der Sequencer kurz abgehackt / lang gehalten (OP-Z Short/Long) | – | ~0 |
| **FREEZE CLOUD** | hält die CLOUD von FX1 an (nur mit Typ CLOUD) | – | ~0 |

### C. Neue INSERT-Typen pro Spur

Pro Spur, also verschieden auf jeder der vier Spuren. Speicher: der 256-Sample-Puffer, den FLANG und CHOR
schon haben.

| Typ | Was er macht | Regler A B C | Kosten |
| --- | --- | --- | --- |
| **RING** | Ringmodulator, Frequenz fest oder zur gespielten Note (Glocken, Metall) | FREQ / RATIO, TRACK, MIX | ~10 |
| **SHIFT** | Frequenzverschieber (Bode): unharmonisch, schwebend, Barber-Pole | HZ, FDBK, MIX | ~40 |
| **COMB** | Resonator auf die gespielte Note gestimmt: alles klingt wie Saiten / Metall (ähnlich OP-1 „Grid“) | TUNE, DECAY, MIX | ~15 |
| **VOWEL** | Formantfilter a-e-i-o-u, vom LFO bewegt: der Synth „spricht“ | VOWEL, Q, MIX | ~30 |
| **WOW** | Kassette: Leiern, Flattern, Rauschen, Sättigung | WOW, FLUTTER, AGE | ~20 |

## 5. Reihenfolge (Vorschlag)

| # | Schritt | Warum zuerst | Aufwand |
| --- | --- | --- | --- |
| E1 ✅ | **FX1-Typen mit CLOUD** (+ FREEZE CLOUD, THROW in die Wolke) | der größte „Wow“-Effekt, kein neuer Speicher, OP-Z-Prinzip FX1 | erledigt |
| E2 | **OP-Z-Punch-ins** DUCK, STEREO, PITCH, RAMP UP, GLITCH | machen die FX-Spuren sofort lebendiger, klein und sicher | klein bis mittel |
| E3 | **INSERT** RING, COMB, VOWEL, WOW, SHIFT | Charakter pro Spur | mittel |
| E4 | **FX1-Typen** SHIMMER, REVERSE, TAPE ECHO; SCATTER, SHORT / LONG | Ausbau | mittel |
| E5 | **FX-Spuren steuern Parameter** (wie die Schritte der OP-Z-FX-Spuren): z. B. CLOUD SIZE pro Schritt | OP-Z-Feeling | mittel |

Jeder Schritt bringt mit:

- Host-Tests: aus = bitgenau, keine Klicks beim Ein- und Ausschalten, kein Gleichanteil, keine Übersteuerung,
  Kosten pro Sample; Speichern und Laden der Einstellungen.
- **Hörbeispiele als WAV** aus den Tests (wie heute schon für FLANGER oder FREEZE), damit man vor dem Gerät hört,
  wie es klingt.
- einen Workflow auf der Emulator-Seite, Budgets (RAM, Pool, ISR) vorher und nachher.

## 6. Stand E1 (CLOUD)

Umgesetzt in `firmware/src/fx1.c`, entschieden: ein Typ zur Zeit (volle 1,5 s), vier Knobs.

- Seite **FX1**: KNOB 1 TYPE (DELAY / CLOUD), mit CLOUD KNOB 2 RAND, 3 FDBK, 4 MIX. Seite **CLOUD** (statt DLY): SIZE
  (10 ms – 1 s), DENS (0,5 – 64 Grains/s), PITCH (±24 Halbtöne), SPREAD (wie weit zurück, wie breit im Stereobild).
- Bis zu 8 Grains, Hann-Fenster, Zufallsverstimmung und Rückwärts-Grains (RAND), Stereo über das Seitensignal des HALL.
- **CLOUD FREEZE** auf F5 der FX-Ebene (auch auf den FX-Spuren aufnehmbar); ECHO THROW wirft in die Wolke.
- Typwechsel: der alte Typ blendet aus, der Puffer wird in 23 ms geleert, der neue beginnt leer.
- Gespeichert in den Projekt-Extras und im Autosave; ein Felucca-Projekt lädt mit DELAY.
- Kosten: statischer Audio-ISR-Wert 39 169 von 41 252 (vorher 39 032); RAM +32 Byte, Pool +380 Byte.
- Tests: `tests/cloud_test.c` (Grains lesen nie, wo geschrieben wird, Tonhöhe, Pegel, Rückkopplung, FREEZE, Seiten,
  Speichern) und Hörbeispiele: `build/cloud_demo/*.wav` (trocken, Wash, Shimmer, Down, Glitch, Freeze).

## 7. Offene Fragen

1. ~~Womit anfangen?~~ E1 (CLOUD), erledigt.
2. ~~FX1: ein Typ zur Zeit oder beides?~~ Ein Typ zur Zeit (volle 1,5 s).
3. ~~Wie viele Regler?~~ Vier.
4. **Als Nächstes:** E2 (OP-Z-Punch-ins DUCK, STEREO, PITCH, RAMP UP, GLITCH) oder E3 (Spur-Effekte)?
