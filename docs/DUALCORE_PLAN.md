# Plan: der zweite CPU-Kern (CPU1)

Stand 2026-11-02. Der AC791N hat zwei pi32v2-Kerne; M-Vave-OP1 nutzt bisher nur CPU0 (Audio-ISR, Key-Scan-ISR und
Hauptschleife, alles nacheinander). Andere FM-1-Firmwares nutzen CPU1 schon auf dem Gerät. Dieser Plan beschreibt, wie
wir das übernehmen: was es bringt, was dafür im Code geändert werden muss, in welcher Reihenfolge und mit welchen Risiken.

**Fortschritt:** Schritt 1 und 2 (Abschnitt 5) sind erledigt: die Parts teilen beim Rendern nichts mehr, die Firmware ist
zweikernfähig gebaut (`FELUCCA_CPU2 1`), rendert auf dem Gerät aber weiter mit einem Kern, bitgleich zu vorher. Was genau
geändert wurde: 3.3 (Spalte „Umgesetzt“), Tests: 3.4, Zahlen: Abschnitt 4. Schritt 3 (HAL, Build) ist der nächste; für
Schritt 4 braucht es die Entscheidungen aus Abschnitt 8.

## 1. Was es schon gibt

| Projekt | Was | Lizenz |
| --- | --- | --- |
| Felucca 1.6.1 (unser Upstream) | CPU1 rendert pro Audio-Block einige der 4 Parts; MENU > SYSTEM > 2ND CORE (BETA, Standard OFF); [PR #247](https://github.com/hugelton/Felucca/pull/247), [Discussion #141](https://github.com/hugelton/Felucca/discussions/141) | GPL-3.0 |
| felucca-bsp `chip/fm1_cpu1.h`, `fm1_cpu1.S`, `fm1_cpu1_stop.h` | Start, Mailbox, Prüfwort, Timeout, Fehlerpfad (370 + 29 + 36 Zeilen) | MPL-2.0 |
| fm1-x0x 1.0 | erster Start von CPU1 auf dem FM-1; [Notizen](https://github.com/charlesvestal/fm1-x0x/blob/v1.0/docs/plans/2026-10-04-dual-core-and-clock.md) | GPL-3.0 |
| Melodee, Jangada | stabiler Start, Halten vor Flash-Schreiben und Reset, Host-Test mit Thread als CPU1 | MPL-2.0 (nach Absprache) / GPL-3.0 |

Gemessen von den Autoren (je ein Gerät): Felucca, 4 Spuren GRAIN/DRUM, **65–74 % → 35–51 %** Last, keine verspäteten
Halbpuffer mehr; X0X Worst Case 115 % → 81 %.

Bekanntes Hardwareproblem: auf **manchen FM-1 liest CPU1 ab und zu geteiltes RAM falsch** (0x00200000 statt 0), unter
Last häufiger (keremimo/melodee#17, charlesvestal/fm1-x0x#10). Deshalb hat jeder Auftrag ein Prüfwort, und ein Fehler
schaltet CPU1 ab.

Unsere Basis ist Felucca **1.5.1** (`FELUCCA_VERSION`); Upstream hat inzwischen in Submodule (`bsp/`, `engines/`)
umgebaut. Ein Merge geht nicht: wir **portieren von Hand**, mit den Upstream-Dateien als Vorlage.

## 2. Wie es funktioniert (Upstream-Design, das wir übernehmen)

- **Start** einmal beim Einschalten, vor Audio und Timern (später gestartet erreicht CPU1 ihren Einsprung nie):
  CPU1 halten, ihre Interrupt-Bank aus (`0x1EEF300..`), Einsprungadresse nach `0x01C7FFF8`, `SYS_DIV` (`0x10008`)
  Bit 3 während des Starts, `C1_CON` (`0x1EEE004`) Bit 3 setzen, Bit 1 löschen. Die PC-Limits sind offen, solange CPU1
  durch das ROM bootet, und werden danach wieder scharf geschaltet.
- **CPU1 wartet in einer Schleife im RAM** (`.c1_text`) und holt nie Code aus dem Flash, solange sie nichts zu tun hat.
- **Pro Audio-Block** (`fx.c` `mix_block`): `events_block` und `perf_begin` laufen wie bisher auf CPU0 (Noten,
  Voice-Stealing, alles Geteilte). Dann bekommt CPU1 per Mailbox einige Parts (`c1_parts`) und rendert sie in **eigene
  Summen** (`mix_l/r`, Sends). CPU0 rendert die übrigen, wartet, addiert die Summen und macht FX-Busse und Master wie
  bisher. Da die Summen Ganzzahlen sind, ist das Ergebnis **bitgleich** zum Rendern mit einem Kern.
- **Aufteilung:** aus den Kosten des letzten Blocks die Teilmenge der 4 Parts, die den volleren Kern am wenigsten
  belastet (16 Möglichkeiten); keine Übergabe, wenn sie weniger spart als sie kostet (`C1_HAND`).
- **Absicherung:** ein Auftrag zur Zeit, Prüfwort aus Funktion, Argument und Auftragsnummer. Ein Lesefehler, ein Fault
  auf CPU1 oder ein Auftrag, der nicht innerhalb des Timeouts zurück ist: CPU1 wird gehalten, CPU0 rendert allein weiter
  (`FM1_CPU1_FALLBACK 1`), und die Option schaltet sich auf OFF.

## 3. Was in unserem Code geändert werden muss

### 3.1 HAL und Build (`firmware/hal/`, `app.ld`, `tools/build.py`)

- `fm1_cpu1.h`, `fm1_cpu1.S`, `fm1_cpu1_stop.h` aus felucca-bsp nach `firmware/hal/`, MPL-2.0-Kopfzeilen behalten
  (MPL-2.0 gilt pro Datei und ist mit GPL-3.0 kombinierbar); `LICENSING.md` / Credits ergänzen.
- Fehlt in unserem HAL und kommt dazu: `fm1_cnum()` (welcher Kern), `FM1_DBG_C0` / `FM1_DBG_C1`, `fm1_uptime_ms()`,
  der Aufruf von `fm1_cpu1_fault_c` am Anfang von `fm1_fault_c` (`fm1_irq.h`). Vorhanden: PC-Limits,
  `FM1_C0_WR_LIMIT_EN`, `fm1__dbg_unlock`, `fm1_crash`, `fm1_ticks`.
- `app.ld`: `.c1_text` in `.ram_text`, zwischen `_c1_start` und `_c1_end`.
- `build.py`: `fm1_cpu1.S` assemblieren; prüfen, dass `.c1_text` keine Aufrufe ins XIP enthält; Schalter
  `FELUCCA_CPU2=0` baut nichts davon (wie upstream).

### 3.2 Start, Reset, Update (`main.c`, `ota.c`): Entscheidung des Nutzers

Das berührt den Boot-Pfad und den Update-Pfad, die laut Skill nur auf ausdrücklichen Wunsch angefasst werden:

- `fm1_cpu1_stop()` als Erstes in `fm1_cstart` (ein Reset, der CPU1 laufen ließ).
- `fm1_cpu1_start()` in `fm1_main` vor Audio und Timern, nur wenn 2ND CORE ON ist **und** der Boot-Guard diesen Start
  nicht mehr zählt (nicht in den ersten 30 s nach einem gescheiterten Start).
- CPU1 halten vor jedem Reset: `fm1_reboot`, `fm1_enter_uboot`, UPDATE MODE, der Update-Record des Loaders.
- `firmware/loader/` und die Paketidentität bleiben unberührt.

### 3.3 Parts unabhängig machen (der Hauptteil)

Zwei Parts dürfen beim Rendern nichts gemeinsam beschreiben. Aus der Durchsicht von `mix_part` und den Engines,
geteilter Zustand, der pro Part oder pro Kern werden muss:

| Wo | Was | Umgesetzt |
| --- | --- | --- |
| `fx.c` | `part_buf[CTL]`, die Summen `mix_l/r`, `send_c/d/r`, **`send_sh`** (SHIMMER, gibt es upstream nicht) | `mixsum_t mx[2]` (im Pool) mit allen fünf Sends, dem Part-Block und einem SHIMMER-Flag; `mix_part`, `mix_spread`, `shimmer_send` schreiben über `SM(..)` in die Summen ihres Kerns |
| `fx.c` | `shim_live = 1` in `shimmer_send` (von beiden Kernen geschrieben) | Flag `shl` pro Summe, nach den Parts in `shim_live` übernommen (`mix_parts_end`) |
| `voice.c` | `spread_on`, `spread_buf[CTL]` (SPREAD) | `t->spread_on`, `spread_buf[C1_NP][CTL]` (Pool) |
| `voice.c` | `esync_k[3]` (ENV SYNC) | `t->esync_k` im `track_t`-Pad (Größe unverändert 1824 B, Vielfaches von 16) |
| `mod.c` | `struct mod` (die Matrix des gerade rendernden Parts) | `modw_t mod_w[C1_NP]`, `MOD(t)` |
| `voice.c` `lfo_rand` | LFO-S&H zieht für Parts 1–3 aus dem gemeinsamen `rng()` | `track_lfo_adv` (Phase, beim Rendern) und `track_lfo_end` (S&H, Wert, FADE: nach allen Parts, Part für Part): Ziehreihenfolge wie mit einem Kern, Goldens unverändert |
| `eng_phys.c` | `phys_reset(..., rng())` beim Modellwechsel unter klingender Note | Seed aus einem Generator pro Part (`PHYS_RSEED`, wie upstream); der Note-on zieht weiter `rng()` |
| `eng_part.c` | `static float io[CTL]` (PRTCL-Diffusor) | auf dem Stack |
| `fm6_core.c` | `fm6_bus[2][FM6_N]` (FM6) | auf dem Stack (wie upstream) |
| `plaits_voice.c` | **`plv_out`, `plv_aux`, `plv_o`** (alle Plaits-Engines; fehlte in der Durchsicht) | `plv_buf_t plv_w[C1_NP]` (Pool), `PLV(t)` |
| `eng_acid.c` | **`acid_osc`, `acid_o`** (ACID; fehlte in der Durchsicht) | auf dem Stack von `acid_render` |
| `perform.c` | `perf_mute`, `perf_throw` | geprüft: schreiben nur `pf.mg[k]` (pro Track) und die übergebenen Summen; `pf` sonst nur gelesen (`perf_begin` vorher) |
| `voice.c` | `voice_kills`, `vage` | geprüft: nur bei Note-on/Stealing (`events_block`) und in audio.c, nicht beim Rendern |
| `fx.c` | `FX_PART_TAP` (web/bounce) | nur im Pfad mit einem Kern (`C1_ON()` 0) |

Pro Track schon getrennt und unkritisch: Engine-Speicher (`eng_mem[NPART]`), SLICER (`sl[NTRK]`), DRIVE, EQ (`eqa`),
`eng_grain` `gr_rng[NPART]`, `eng_wheel`, `eng_digital`. Die Tabelle ist eine Durchsicht, kein Beweis: was fehlt,
findet der Test in 3.4 (er hat nichts mehr gefunden: 0 Abweichungen, ThreadSanitizer ohne Meldung).

### 3.4 Tests (vor jeder Zeile im Gerät) – erledigt

Umgesetzt in `tests/c1_host.h` und `tests/dualcore_test.c` (in `tests/run_tests.sh`): 38 Szenen (4 Parts jedes Presets
jeder Engine, 6 gemischte Szenen inkl. PHYS-Modellwechsel), DRIVE, EQ (Mono-Summe und SPREAD-Seite), SLICER, SPREAD,
SHIMMER-Send, REV/ECHO THROW, Mutes (Solo), Matrix mit S&H aus dem geteilten und dem eigenen Generator, ENV SYNC. Jede
Szene mit einem Kern und mit 7 Aufteilungen (rotierend, `mix_split`, feste Masken): bitgleich. Unter ThreadSanitizer
(`TSAN=1 tests/run_tests.sh`, ~8 min): keine Meldung. `regress` mit `-DC1_HOST` und mit `-DFELUCCA_CPU2=0`: alle 139
Goldens unverändert. Gegenprobe `-DDUALCORE_BREAK` (die Kopien pro Part wieder zusammengelegt, `C1_PI` 0): alle
gemischten Szenen weichen ab, ThreadSanitizer meldet sofort `mod_begin`.

Ursprüngliche Planung:

- `tests/c1_host.h` (ein pthread spielt CPU1) und `tests/dualcore_test.c` von upstream portieren, Szenen auf
  **unsere** Engines erweitern: die Plaits-Engines (WSHAPE, HRMNC, PRTCL, WTABLE, SWARM), FM6, VDRUM, GRAIN, PHYS mit
  Modellwechsel, DRUM-Kits, dazu SPREAD, SLICER, EQ, DRIVE, SHIMMER-Send, THROW, Matrix mit S&H, ENV SYNC.
- Jede Szene einmal mit einem Kern, dann mit **jeder** Aufteilung der Parts und mit der eigenen Wahl der Firmware:
  **bitgleich**. Gebaut mit `-fsanitize=thread`: jeder Zugriff zweier Parts auf dasselbe ist ein Fehler.
- `tests/regress.c` zusätzlich mit `-DC1_HOST`; die Goldens bleiben unverändert.
- Erst einmal absichtlich etwas Geteiltes einbauen und sehen, dass der Test rot wird.

### 3.5 Aufteilung und Kosten (`fx.c`)

- Kosten pro Part messen: `fm1_ticks()` um jedes `mix_part` (billig, misst die echten Kosten, auch die
  Plaits-Engines), daraus die Aufteilung für den nächsten Block (`mix_split`, `C1_HAND`). **Stand:** `mix_split` ist da,
  mit einer Schätzung wie upstream (`ENG_VCOST`: Stimmen × Kosten pro Stimme; die FPU-Engines nach der Messung am Gerät
  geschätzt). In Schritt 4 durch die gemessenen Ticks ersetzen.
- `c1_wait` und der Abschaltpfad außer Linie (`noinline`), damit `fm1_alnk0_irq` im Budget von
  `tests/target_budget.py` bleibt.

### 3.6 FPU auf CPU1: neu bei uns, upstream nicht geklärt

Unsere Plaits-Engines rechnen mit der FPU (`FM1_FPU`). Ob die FPU auf CPU1 genauso läuft, weiß niemand. Daher:

- `fpu_probe.c` (CHECK: Ergebnisse bitgenau, Waveshaper-Hash) zusätzlich **als Auftrag auf CPU1** laufen lassen
  (MENU > SYSTEM > INFO, nur auf Anforderung, wie heute).
- Bis die Probe auf dem Gerät OK meldet: Parts mit FPU-Engines bleiben auf CPU0 (Maske in `mix_split`). Dann bringt
  CPU1 nur bei den Integer-Engines etwas; danach dürfen alle.

### 3.7 Menü, Anzeige, Speicher

- MENU > SYSTEM > 2ND CORE: OFF (Standard) / ON (BETA), wirkt ab dem nächsten Einschalten. Neues, nie benutztes Bit in
  den Settings (append-only).
- INFO: Status (an / aus / warum aus), Last von CPU1, Stack-Tiefe; dieselben Werte in der Konsole (`cpu2*`).
- Stellt sich CPU1 wegen Lesefehlern ab: einmal eine Meldung, Option auf OFF gespeichert.
- Alles neu Angezeigte in `graph_signature()`.

## 4. Budgets

Gemessen 2026-11-02 (Toolchain 20260730), vor → nach Schritt 2:

| | vorher | nachher | Grenze |
| --- | --- | --- | --- |
| RAM `.data`+`.bss` | 44 528 B | 42 624 B | 51 200 B (≥ 4 096 frei) |
| Pool | 373 652 B | 377 500 B | 391 168 B (≥ 8 192 frei): 5,4 KB nutzbar übrig |
| Image | 541 380 B | 540 820 B | 581 564 B |
| Audio-ISR `fm1_alnk0_irq` (statisch) | 40 736 | 37 478 | 44 810 (40 736 + 10 %) |

Die Summen, SPREAD-Puffer und Plaits-Puffer liegen im Pool (gleiches SRAM), daher sinkt RAM. Die Übergabe an CPU1
(`mix_parts2`, `c1_parts`, `c1_add`, `mix_split`) ist auf dem Gerät noch toter Code (`C1_ON()` 0) und nicht im Image.
Neu überwacht: `mix_parts_end` (63). Für CPU1s Stack (2,8 KB upstream) reicht der Pool-Rest knapp; sonst RAM, das
jetzt 8,6 KB frei hat (4,5 KB über der Pflichtreserve).

Ursprüngliche Schätzung (Zahlen vom 2026-10-27):

| | frei | braucht CPU1 (upstream) | Folge |
| --- | --- | --- | --- |
| RAM `.data`+`.bss` | 9,3 KB, davon 4 KB Pflichtreserve | ~5,6 KB (3 KB Stack, Summen, Kopien) | **zu knapp**: Stack und Kopien in `.pool` legen |
| Pool | 13,3 KB, davon 8 KB Pflichtreserve | 0 | 5,3 KB nutzbar, reicht für Stack + Kopien |
| Image | ~50 KB | ~2 KB | unkritisch |
| Audio-ISR `fm1_alnk0_irq` | 840 (40 412 von 41 252) | etwas für Übergabe und Addieren | Übergabe schlank, Rest `noinline` |

## 5. Reihenfolge

1. **Vorbereitung:** Toolchain, Budgets messen, Lastmessung als Referenz auf dem Gerät (die Szenen aus
   `CHECKLISTE.md`). *Erledigt bis auf die Lastmessung am Gerät.*
2. **Parts unabhängig machen** (3.3) mit dem Host-Test (3.4), **ohne** CPU1: die Firmware rendert noch mit einem Kern,
   nur die Struktur ist zweikernfähig. Bitgleich, Suite grün. Kann allein ausgeliefert werden. *Erledigt (2026-11-02).*
3. **HAL und Build** (3.1), noch ohne Start: baut, Budgets passen, `.c1_text` geprüft.
4. **Start, Halten, Aufteilung** (3.2, 3.5), Menü und INFO (3.7), FPU-Maske (3.6). Standard OFF.
5. **Gerätetest** nur mit Zustimmung, möglichst mit Wiederherstellungshardware (XIAO RP2040, Flash-Backup):
   Einschalten mit OFF (darf sich nichts ändern), dann ON; Lastszenen messen; 10 Minuten Volllast; Projekt speichern,
   Settings, USB DISK, Update mit CPU1 an; alle Reset-Wege. Ergebnisse in `CHECKLISTE.md`.
6. **FPU-Probe auf CPU1**, danach die Maske aufheben.
7. **Später, eigener Schritt:** Aufteilung pro Stimme statt pro Part, falls ein einzelner Part mit 8 teuren Stimmen
   der Engpass ist (die Teilung pro Part hilft dann nicht).

## 6. Was es bringt

- Weniger Last bei mehreren aktiven Parts; laut upstream etwa 30–45 % weniger.
- **Mehr Stimmen:** `voice_space` (PLAITS_PLAN Abschnitt 15) lässt Stimmen über 8 hinaus zu, solange die Last unter 75 %
  liegt. Mit CPU1 sinkt die gemessene Last, also kommen automatisch mehr Stimmen dazu, ohne weitere Änderung.
- Kein Gewinn bei einem einzelnen Part (siehe Schritt 7) und solange FPU-Parts auf CPU0 bleiben.

## 7. Risiken

- **Geräte mit Lesefehlern auf CPU1:** abgefangen (Prüfwort, Abschalten, Option OFF), aber auf solchen Geräten bringt
  es nichts.
- **Boot-Pfad:** ein Fehler beim Start kann das Gerät unbrauchbar machen. Abgesichert durch Standard OFF, Start erst
  nach dem Boot-Guard-Fenster, Fallback statt Neustart, Halten vor jedem Reset; trotzdem nur mit Wiederherstellung.
- **Flash:** ein Auftrag läuft aus dem XIP. CPU0 darf den Flash nur abschalten, wenn kein Auftrag offen ist; das ist
  gegeben, solange der Render auf seinen Auftrag wartet, bevor die ISR zurückkehrt. Beim Gerätetest gezielt prüfen.
- **FPU auf CPU1** ungeklärt (3.6).
- **Übersehener geteilter Zustand:** Klicks oder falsche Töne nur bei bestimmten Aufteilungen. Der bitgleiche Test
  über alle Aufteilungen plus ThreadSanitizer ist die Absicherung.

## 8. Offene Entscheidungen

1. Dürfen `main.c` (Start, Boot-Guard-Umfeld) und die Reset-/Update-Wege für das Halten von CPU1 angefasst werden?
2. Fallback: nach einem CPU1-Fehler mit einem Kern weiter (vorgeschlagen) oder Neustart (Melodee-Standard)?
3. Ist Wiederherstellungshardware vorhanden, bevor Schritt 5 aufs Gerät geht?
4. Schritt 2 zuerst allein ausliefern (vorgeschlagen), oder alles in einem Zug?
