<p align="right"><a href="community-release-draft.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Community release draft

Status: local preparation only. The public source repository does not yet exist online; this draft is not an upload approval.

| Field | Prepared value |
| --- | --- |
| Chinese title | See the exact approved title in the [Chinese draft](community-release-draft.zh_CN.md) |
| English title | Retro Terminal Hydration Reminder |
| Cover | [`../../assets/images/community-cover.png`](../../assets/images/community-cover.png), original 1280 × 720 PNG |
| Public source | `https://github.com/pizzanicky/ai-passport-retro-hydration` **after** the separate public repository is created and reviewed; do not use the current private repository URL as a publication claim |
| Firmware | `build/FoloToy-AI-Passport-full.bin`, verified merged image; exact SHA-256 and source commit must be copied from the final clean build manifest before upload |

## Chinese description

See the approved Chinese text in the [Chinese draft](community-release-draft.zh_CN.md).

## English description

A retro green terminal hydration reminder for the FoloToy AI Passport. It uses original green terminal graphics and an original retro cartoon character, with `USER-01` as the default identity label. When the master switch is on, water alerts run daily from 09:00 to 21:00 at a fixed 30-minute interval. An alert lights the screen and plays a tone; after 30 seconds without input the backlight goes off while the alert remains pending. Wake with a button and press OK to acknowledge, which starts the next 30-minute interval. A separate Bluetooth BLUFI provisioning mini-program supplies 2.4 GHz Wi-Fi credentials for time sync; Wi-Fi turns off after sync. The medicine page is a placeholder with no medicine alerts. Timezone is fixed to Asia/Shanghai, and reminders require a fresh sync after boot. Access and attendance compatibility of the passive NFC tag requires separate testing. All illustrations are original and do not use third-party game characters, logos, or screenshots.

## Checks before publication

- Complete `./tools/validate.sh` on the final public commit; verify the single merged image archive and record its full image, app, and ELF hashes. Recheck after any source change.
- Confirm cover type and size (PNG, JPEG, or WebP; at most 10 MiB) and visually inspect all previews.
- Scan the entire public Git history and files for private portrait pixels, personal handoff data, credentials, device links, and raw serial logs.
- On the device, verify the public firmware's retro cartoon identity art, alert onset and tone, screen timeout after 30 seconds while pending, button wake, and OK acknowledgment. Check provisioning and time sync, battery use, and NFC enrollment separately. A merged image written from `0x0` may reset NVS and stored Wi-Fi; obtain separate flashing consent after explaining this effect.
- After device acceptance, review every field, keep the old repository private, create a separately named public repository, check the public source and firmware match, then authorize the official community publisher separately. Stop and report if either public Git or community upload fails.
