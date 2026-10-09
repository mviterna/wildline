# Seasonal tree derivatives

`prepare_seasonal_trees.py` derives these from the existing licensed Forest/Export
trees. All bark, trunks and connected wooden limbs retain their source geometry,
scale, UVs and positions. Original exports remain unchanged.

* `Thin`: removes approximately 28% of connected foliage cards, without leaving
  half cards. Blackwood, the farm's woodland and neighborhood surroundings use it.
* `Bare`: removes every leaf/needle face. Frostwood uses it with an upward-facing
  frost material, snow ground, cold winter lighting and no ground leaf/fern layer.
* `Wood`: invisible wood-only complex collision. Every gameplay tree has the
  same transform as its visual instance; foliage never becomes a platform.

`manifest.json` records source and derivative face counts. Recreate with Blender,
then build the editor and run `Scripts/import_world_upgrade.py` in Unreal's
full unattended editor with -ExecutePythonScript (the editor mesh subsystem is required). `native/VerifyWorldUpgrade.ps1` exercises real branch capsule
collision, grip movement, trunk ascent and launch on all five maps.

Frostwood preserves Classic's forage, den, perimeter and win rules. Select it in
Match Rules / Classic Reserve before entering a fresh waiting room.
