# Effektkette neu: Plan

Ziel: eine klare, feste Kette ohne doppelte Effekte. Pro Spur ein Drive, danach **eine** gemeinsame serielle Kette
Chorus → Shimmer → Delay → Reverb. Der Pumper (und alles andere) am Ende der Kette fällt weg. Grundlage: die Skizze
vom 28.10. und das Interview dazu.

## 1. Ziel-Signalfluss

```
Spur 1 ─ SLICER (GATE / STUT / PUMP) ─ DRIVE ─ LEVEL / PAN ─┬────────────────────────────── dry (stereo) ─┐
Spur 2 ─ SLICER ─ DRIVE ─ LEVEL / PAN ─────────────────────┤                                             │
Spur 3 ─ …                                                  │ Sends pro Spur: CHR  SHR  DEL  REV          │
Spur 4 ─ …                                                  │                  │    │    │    │           │
                                                            │                  ▼    ▼    ▼    ▼           │
                                                            └──────────────► [CHO]→[SHIM]→[DLY]→[REV] ─ wet ┤
                                                                                                         MASTER
                                                                                                    → FX-Ebene
```

- **Stufe n bekommt:** die Sends aller Spuren für diese Stufe **plus** das, was aus Stufe n−1 kommt (deren Wet).
  Was in den Chorus geht, läuft also weiter durch Shimmer, Delay und Reverb. Was nur ins Delay geht, hört man im
  Delay und im Reverb. So arbeitet SHIM → DLY → REV heute schon; neu ist, dass CHORUS vorne in die Kette kommt
  (bisher lief er parallel).
- Jedes Wet geht wie bisher auch direkt auf den Master.
- **Stereo (hybrid):** Der Dry-Anteil bleibt stereo (PAN, SPREAD). Der Chorus bekommt einen zweiten, versetzten
  Abgriff für die Breite, HALL und SHIMMER behalten ihr Seitensignal, das Delay bleibt mono.
- **Hinter dem Reverb kommt nichts mehr** außer MASTER und der FX-Ebene (Punch-ins, Throws, FX TRACKS).

## 2. Entscheidungen aus dem Interview

| Thema | Entscheidung |
| --- | --- |
| Pumper | Nur noch am Anfang: SLICER-Modi OFF / GATE / STUT / PUMP, pro Spur **vor** dem Drive. GATE END und PUMP END entfallen. |
| Inserts | Das INSERT wird zum **DRIVE**. Es bleiben SOFT, HARD, FOLD, FUZZ und CRUSH. PHASR, FLANG und CHOR fallen weg (redundant zum Chorus-Bus mit CHORUS / FLANGER / VIBRATO und zur FX-Ebene). Der alte DIST-Regler (Pre-Drive) fällt weg. |
| Drive-Regler | K1 TYPE · K2 DRIVE · K3 TONE · K4 LEVEL. Kein MIX: Der Drive ist immer voll drin, TYPE OFF = aus. Bei CRUSH heißen die Regler BITS · RATE · LPF (wie bisher). |
| Mix-Fenster | **Pro Spur** (gewählte Spur): 4 Fader CHR / SHR / DEL / REV = wie viel von dieser Spur in die jeweilige Stufe geht. |
| Stereo | Hybrid (siehe oben). |
| REV / ECHO THROW | Unverändert: Sie ziehen weiter die REV- bzw. DLY-Sends aller Spuren hoch. |
| Reverb-Typ SHIMMER | Entfällt. REVERB hat nur noch ROOM / SPRING / HALL. Der Shimmer ist die eigene Stufe. |
| Alte Projekte | Keine Rücksicht nötig. Gespeicherte Werte, die es nicht mehr gibt, werden beim Laden auf OFF bzw. HALL gesetzt (damit nichts Ungültiges läuft). Parameter-IDs und Enum-Werte bleiben trotzdem reserviert (Projektformat, Editor-Protokoll: append-only). |

## 3. Die Fenster (FX-Taste, der Reihe nach)

| # | Seite | Bereich | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **MIX** (bisher „FX“) | Spur | CHR | SHR | DEL | REV |
| 2 | **DRIVE** (bisher INSERT + INSERT 2) | Spur | TYPE | DRIVE | TONE | LEVEL |
| 3 | CHORUS | global | TYPE | RATE | DEPTH | FDBK |
| 4 | SHIMMER | global | SIZE | DAMP | PITCH | FDBK |
| 5 | DELAY (FX1, mit CLOUD) | global | wie heute | | | |
| 6 | REVERB | global | TYPE | SIZE | DAMP | *(frei, s. offene Punkte)* |
| 7 | SLICER | Spur | MODE (OFF/GATE/STUT/PUMP) | PAT | RATE | DEPTH |
| 8 | FX TRACKS | global | wie heute | | | |

## 4. Umsetzung in Schritten

Jeder Schritt einzeln: Host-Test zuerst (rot sehen), dann Code, `./build.sh`, Budgets, Suite.

1. **SLICER am Ende raus** (`slicer.c`, `fx.c slicer_end`, `ui_graph.c`, `params.c N_SLCR`):
   MODE-Bereich bis PUMP, `sle`-Zustand und `slicer_end()` entfernen, beim Laden ≥ GATE END → OFF.
   Tests: `tests/fxchain_test.c` (END-Teile raus, START-Teile bleiben), `tests/slicer_test.c`.
2. **INSERT → DRIVE** (`fx.c track_insert`, `params.c` Seiten und `INS_DESC`):
   TYPE bis CRUSH, `ins_swept` und die 256-Sample-Linie pro Spur entfernen (Pool wird frei), `track_dist` aus der
   Kette, MIX fest auf 100 %. Seiten INSERT + INSERT 2 → eine Seite DRIVE. Grafik: Kennlinie bzw. Pegel als Balken.
   Tests: `tests/insert_test.c` (Swept-Typen raus, „TYPE OFF = bitgenau wie ohne“, kein Klick bei Typwechsel).
3. **Chorus in die Kette** (`fx.c fx_buses`, `fx1.c fx1_block`, `dly_glide`, `mix_block`):
   Chorus aus der Delay-Schleife lösen und als erste Stufe vor `shimmer_bus` rechnen; `send_sh += cho_wet`;
   `serial_reverb` zieht `cho_wet` nicht mehr ab (er kommt jetzt über die Kette). `fx_serial` entfällt, die Kette
   ist immer seriell.
   Tests: neuer Test „nur CHR-Send → Signal kommt in Shimmer, Delay und Reverb an“, „nur REV-Send → nur Reverb“,
   kein Überlauf mit 4 lauten Spuren und vollen Sends; `tests/shimmer_send_test.c`, `tests/cloud_test.c` anpassen.
4. **Hybrid-Stereo für den Chorus**: zweiter Abgriff (um 90° versetzt) als Seitensignal auf `mix_l` / `mix_r`,
   wie es HALL schon macht. Test: Mitte unverändert, Seite > 0, keine Kosten bei Chorus-Send 0.
5. **Reverb-Typ SHIMMER raus** (`params.c N_RTYPE`, `fx.c` Typwahl): Bereich bis HALL, beim Laden 3 → HALL.
   Test: `tests/reverb_test.c`.
6. **Seiten und UI** (`params.c PAGES`, `ui.c fx_pedal`, `ui_help.c`, `ui_graph.c`, `icons.c`):
   Reihenfolge wie in Abschnitt 3, „FX“ → „MIX“, Pedal-Grafiken anpassen, `graph_signature()` für alle neuen
   Werte. Screenshots: `tests/ui_render.c`, `tests/golden.txt` neu erzeugen.
7. **Web**: `web/editor.html`, `web/EDITOR_PROTOCOL.md` (IDs gleich, Bereiche kleiner), `web/manual/` aktualisieren.
8. **Prüfen**: ganze Suite (`tests/run_tests.sh`), Budgets vorher → nachher (`budget_report.sh`,
   `tests/target_budget.py`: Audio-ISR höchstens +10 %), Emulator (Seiten durchklicken, Hörprobe), WAV-Hörbeispiele
   der Kette. Installation aufs Gerät erst danach und nur nach deinem Go.

Erwartung beim Budget: Schritt 2 spart Pool (4 × ~580 B Insert-Zustand werden kleiner) und ISR-Zeit, Schritt 1
spart den END-Pfad, Schritt 3 kostet fast nichts (eine Addition pro Sample), Schritt 4 einen zweiten Lesezugriff
pro Sample im Chorus.

## 5. Offene Punkte (Vorschlag in Klammern)

1. **Wet-Lautstärke der Stufen:** Delay hat MIX, Chorus, Shimmer und Reverb haben keinen eigenen Rückweg-Regler.
   (Vorschlag: REVERB K4 = MIX; Chorus und Shimmer ohne, die Sends reichen.)
2. **Soll der Wet einer Stufe auch einzeln auf den Master gehen** oder nur über die nächste Stufe weiter?
   (Vorschlag: beides, wie heute. Sonst hört man den Chorus nur noch verhallt.)
