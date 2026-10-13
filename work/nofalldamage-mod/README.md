# No Fall Damage — Android native-mod source scaffold

This archive contains the project files and a GitHub Actions workflow template. It does **not** contain a compiled `.so` or a tested `.levipack`.

## Important compatibility notes

- The workflow expects a matching Android SDK package that provides `LeviLaminaConfig.cmake`, `LeviLamina::LeviLamina`, and the `<pl/...>` headers. The repository workflow does not install that SDK for you, so the build will fail until the matching SDK/setup is supplied.
- The `pl::memory::Patch`, `PatchHandle`, and `getModuleBaseAddress` symbols must match the exact LeviLauncher/LeviLamina SDK version in use. Their compatibility has not been verified against your installed launcher build.
- `TARGET_OFFSET` (`0x0f28e710`) is version-specific and has not been independently verified. Patching an incorrect offset/instruction can crash the game. The `RET` patch is not guaranteed safe merely because a patch helper can restore bytes.
- This source uses the Mod Menu registration API pattern documented by LeviLaunchroid, but the complete Android build/link/package pipeline still needs validation against the target SDK.

## Contents

- `manifest.json` — native mod metadata
- `CMakeLists.txt` — Android CMake template
- `src/main.cpp` — toggle and patch scaffold
- `.github/workflows/build.yml` — GitHub Actions template

## Expected output

When the SDK is correctly configured and the project successfully builds, the workflow is intended to package `manifest.json` and `libnofalldamage.so` into `NoFallDamageMod.levipack`.
