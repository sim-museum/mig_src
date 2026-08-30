# Tacview: installing it, and opening a sortie recorded by this port

This port records flights as **ACMI** files, which Tacview reads. Nothing in the repo said how to
get Tacview onto a Linux box or where the exports land, so this is that (PO-87). BoB has the twin
of this file (`R15`); the two ports differ only in where the file is written.

## What this port writes, and where

The recorder writes **one file, always the same name**, into the game's run directory:

    $HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig/acmi_current.txt

- The name is fixed (`ma_acmi.cpp:73`), so **every sortie overwrites the previous one.** If a
  recording matters, copy it out before flying again. `port/tacview_export.sh` archives each run's
  file next to its log for exactly this reason — an earlier session lost a control arm's evidence
  to the second run.
- The extension is `.txt`, not `.acmi`. Tacview opens it regardless (the format is identified by
  content), but renaming the copy to `.acmi` makes it obvious what it is.

## Installing Tacview

Tacview is a Windows application; on this box it runs under Wine in **its own prefix**, kept
separate from the game prefixes so a Wine change for one cannot disturb the other.

**The maintained way — a self-contained launcher that installs on first run:**

    /home/admin/sgl/SAT/tacview/tacview.sh

It sets `WINEPREFIX=$PWD/WP` and `WINEARCH=win64`, installs **Wine Mono** if absent (Tacview's
addons need .NET, and without it the install appears to work and then misbehaves), runs the newest
`INSTALL/Tacview*Setup*.exe` it finds, and afterwards just launches. It starts Tacview inside
`wine explorer /desktop=Tacview,1280x800`, which keeps it in its own virtual desktop rather than
letting it fight the host window manager.

Installers already present on this machine (newest wins):

    /home/admin/sgl/SAT/tacview/INSTALL/Tacview187Setup.exe
    /home/admin/sgl/SAT/INSTALL/Tacview176Setup.exe
    /home/admin/Downloads/Tacview195Setup.exe          (1.9.5, newest)
    /home/admin/Documents/260825/Tacview195Setup.exe

To install a newer one, drop the `.exe` into `sgl/SAT/tacview/INSTALL/` and run `tacview.sh` — it
picks the highest version by `sort -V`.

**The other install already on this box** is a separate 1.9.5 in `/home/admin/Documents/260825/WP`,
launched by the desktop entry `~/Desktop/Tacview.desktop`:

    env WINEPREFIX="/home/admin/Documents/260825/WP" wine-stable \
        "C:\Program Files (x86)\Tacview\Tacview64.exe"

Either works. They are independent prefixes; preferences set in one do not appear in the other.

## Opening a recording

1. Fly a sortie (or run `port/tacview_export.sh`, which flies one and checks the file's structure).
2. Copy the export somewhere stable before it is overwritten:

       cp "$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig/acmi_current.txt" ~/Documents/Tacview/sortie.acmi

3. Start Tacview (`tacview.sh`, or the desktop entry) and use **File > Open**. Wine maps `Z:` to
   the Linux root, so `~/Documents/...` appears under `Z:\home\admin\Documents\...`.

## Two things to know before you read anything into a replay

**1. Tacview orients the model from `Yaw` ALONE, and in the OPPOSITE rotational sense from compass
heading.** (S276/S327.) The ACMI transform is
`T=Lon|Lat|Alt|Roll|Pitch|Yaw|U|V|Heading` — Tacview draws the aircraft using `Yaw`, and the
trailing `Heading` field does not orient it. This cost this port two sessions and a confident,
wrong "the aircraft is flying backwards" diagnosis before a four-variant synthetic control settled
it. If a replay shows aircraft pointing away from their own track, suspect this convention before
suspecting the flight model.

**2. A replay that stops early is a known, fixed bug — but check your build.** BoB's exporter used
to truncate every sortie at 20.48 s (its R10, a block-wrap in the frame counter). If you see a
recording end at a suspiciously round time, compare against that.

## Checking the export without opening Tacview

    port/tacview_export.sh

flies a short sortie and asserts the file's structure — that it recorded at all, that the header is
well formed, and that the frames are present — so a broken export is caught without a human loading
it. `port/acmi_orientation.sh` checks the yaw convention specifically, and returns **INCONCLUSIVE**
rather than PASS when a sortie has no east-west leg to judge it by.
