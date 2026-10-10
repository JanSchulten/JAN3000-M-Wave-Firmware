# M-Vave-OP1

OP-1-inspired custom firmware for the **M-VAVE FM-1** pocket synthesizer.

This project is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita
(Hügelton Instruments). It keeps Felucca's hardware layer, USB, update loader, web installer,
storage, engines and tests, and builds an OP-1-style workflow on top: distinct modes
(synth, drum, tape, mixer), playful per-engine graphics and a tape-centred way of making music.

> **Status:** early development. Nothing here has been flashed to hardware yet.
> Installing custom firmware is at your own risk. The FM-1 has a single firmware bank and no
> recovery button; if an install fails and the unit will not start, recovery needs extra
> hardware (FM-1 Transporter with an RP2040). Develop and try changes in the host tests and the
> browser emulator first.

- Roadmap: [docs/ROADMAP.md](docs/ROADMAP.md)
- Build: [BUILDING.md](BUILDING.md) (inherited from Felucca)
- Felucca's own README (feature list, controls, history): [docs/FELUCCA_README.md](docs/FELUCCA_README.md)

## Upstream

Felucca's history is merged into this repository, so upstream changes can be pulled in with:

```
git remote add upstream https://github.com/hugelton/Felucca.git
git fetch upstream
git merge upstream/main
```

## Licence

GPL-3.0-only, like Felucca. Bundled and ported parts keep their own licences; see
[LICENSING.md](LICENSING.md) and `LICENSES/`. If you distribute this firmware, you must also
offer its complete corresponding source under the same licence.
