# Vox Clean — PS5 Cache & Temp Cleaner 🧹

A PS5 payload that cleans the console's WebKit **browser cache**, **cookies**,
**local storage** and useless **temporary data**. Built with the
[ps5-payload-sdk (prospero)](https://github.com/ps5-payload-dev/ps5-payload-sdk).

- ✅ x86-64 PS5 payload (same format as `kstuff.elf` / `etaHEN-2.6.elf`)
- ✅ Deletes cache/cookies/databases/localstorage/tmp under
  `/document/common/webbrowser` and `/user/system/webkit/webbrowser`
- ✅ Shows a TV notification when finished (`Vox Clean — PS5 cache & temp data cleared`)
- ✅ Works on jailbroken PS5 via the standard ELF loader (port `9021`)

## Usage

1. Jailbreak your PS5 (e.g. `etaHEN` / `pldmgr`).
2. Open the ELF loader on port `9021`.
3. Send the payload:

   ```bash
   prospero-deploy -h <PS5_IP> -p 9021 clear_cache_Vox.elf
   ```

Or just load `clear_cache_Vox.elf` from your favourite loader (Vox Manager,
WebMAN, pldmgr toolbox, etc.). You will see a notification on the TV screen
once the cleanup is done.

## Build

Requirements:

- [ps5-payload-sdk](https://github.com/ps5-payload-dev/ps5-payload-sdk)
- LLVM / clang for x86_64 (used through the SDK toolchain)

```bash
make PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
```

Output: `clear_cache_Vox.elf`

## What it cleans

| Path |
|------|
| `/document/common/webbrowser/cache` |
| `/document/common/webbrowser/cookies` |
| `/document/common/webbrowser/cookie` |
| `/document/common/webbrowser/databases` |
| `/document/common/webbrowser/localstorage` |
| `/document/common/webbrowser/tmp` |
| `/user/system/webkit/webbrowser/cache` |
| `/user/system/webkit/webbrowser/cookies` |
| `/user/system/webkit/webbrowser/databases` |
| `/user/system/webkit/webbrowser/localstorage` |
| `/user/system/webkit/webbrowser/tmp` |

> Files that are currently locked by the running browser are skipped safely;
> directories are left in place (only their contents are removed).

## Disclaimer

This is an unofficial homebrew tool for **your own jailbroken console only**.
It is not affiliated with or endorsed by Sony Interactive Entertainment. Use
at your own risk.