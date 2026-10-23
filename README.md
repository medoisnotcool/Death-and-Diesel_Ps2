# Diesel and Death — native PS2 prototype

This is a clean, GitHub-ready PS2SDK/gsKit project. It produces a native MIPS R5900 **`diesel.elf`** through GitHub Actions.

> **Current scope:** this is a playable technical prototype: native title screen, temporary primitive graphics, motorcycle movement/jumping, an opponent, representative pickups, health, win conditions, and controller input. It is **not yet a finished, asset-complete commercial-game port**. No original Flash SWF, art, music, or sound assets are included in this repository package.

## Project layout — do not change it

```text
.github/workflows/build-ps2.yml  GitHub Actions build
src/game.c                       Platform-neutral gameplay rules
src/game.h                       Gameplay types/API
src/ps2_main.c                   PS2SDK + gsKit rendering/input entry point
tests/game_tests.c               Host-side gameplay test
Makefile                         PS2 ELF and host-test build rules
```

## Publish this package to a new GitHub repository

Use **Git Bash on Windows**, or Terminal on macOS/Linux. This preserves the required `.github`, `src`, and `tests` folders automatically.

1. Create a new **empty** GitHub repository. Do not add a README, license, or `.gitignore` on GitHub.
2. Download and unzip this package.
3. Open a terminal in the unzipped `diesel-death-ps2-github-ready` folder.
4. Run the following, replacing `YOUR-USERNAME`:

```sh
git init -b main
git add .
git commit -m "Initial PS2 prototype"
git remote add origin https://github.com/YOUR-USERNAME/diesel-death-ps2.git
git push -u origin main
```

GitHub will start the workflow automatically after the push.

## Download the ELF built by GitHub

1. Open the repository’s **Actions** tab.
2. Open the latest **Build PS2 prototype** run.
3. Wait for both jobs to have a green check.
4. Under **Artifacts**, download `diesel-death-ps2-elf`.

The download contains:

```text
diesel.elf
diesel.elf.sha256
```

## Controls in the current prototype

| Action | Control |
|---|---|
| Accelerate | D-pad Right |
| Brake | D-pad Left |
| Jump / confirm | Cross |
| Use power-up | Circle or R1 |
| Start / restart | Start |

## Local checks

With a regular C compiler:

```sh
make host-test
```

With Docker installed, the included helper uses the same PS2DEV image as CI:

```sh
./build-in-container.sh
```

## Before publishing a complete port

Keep the developer’s written permission, and confirm whether it covers redistribution of the game title, original artwork, audio, and any final PS2 build. Add a license only once the rights holder has approved the intended release terms.
