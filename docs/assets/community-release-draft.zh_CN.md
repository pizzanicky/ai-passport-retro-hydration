<p align="right"><a href="community-release-draft.md">English</a> · <strong>简体中文</strong></p>

# 社区发布草稿

状态：仅在本地准备。公开源码仓库尚未上线；本草稿不代表已批准上传。

| 字段 | 预备内容 |
| --- | --- |
| 中文标题 | 复古荧光终端喝水提醒 |
| 英文标题 | Retro Terminal Hydration Reminder |
| 封面 | [`../../assets/images/community-cover.png`](../../assets/images/community-cover.png)，原创 1280 × 720 PNG |
| 公开源码 | `https://github.com/pizzanicky/ai-passport-retro-hydration`，待独立公开仓库建立并审阅后才成立；当前私有仓库不能作为公开源码地址 |
| 固件 | `build/FoloToy-AI-Passport-full.bin`，经校验的单个合并镜像；正式上传前从最终干净构建记录准确的 SHA-256 和源码提交号 |

## 中文简介

适用于 FoloToy AI Passport 的原创复古绿色终端喝水提醒应用。原创绿色终端界面和复古卡通人物，身份页默认显示 `USER-01`。每天 09:00–21:00，打开总开关后以固定 30 分钟间隔提醒喝水；到点亮屏并发出四声提示音，每声约 0.5 秒、声间约 0.2 秒。提醒待确认时，30 秒无操作会息屏，按键可唤醒；按 OK 确认喝水后重新计时。通过独立的蓝牙 BLUFI 配网小程序配置 2.4 GHz Wi-Fi 并校时，成功校时后关闭 Wi-Fi。时区固定为亚洲/上海；时间未在本次启动后同步时不会提醒。插画为原创素材，未使用第三方游戏人物、标志或截图。

## English description

A retro green terminal hydration reminder for the FoloToy AI Passport. It uses original green terminal graphics and an original retro cartoon character, with `USER-01` as the default identity label. When the master switch is on, water alerts run daily from 09:00 to 21:00 at a fixed 30-minute interval. An alert lights the screen and plays four 0.5-second beeps separated by 0.2 seconds; after 30 seconds without input the backlight goes off while the alert remains pending. Wake with a button and press OK to acknowledge, which starts the next 30-minute interval. A separate Bluetooth BLUFI provisioning mini-program supplies 2.4 GHz Wi-Fi credentials for time sync; Wi-Fi turns off after sync. The medicine page is a placeholder with no medicine alerts. Timezone is fixed to Asia/Shanghai, and reminders require a fresh sync after boot. Access and attendance compatibility of the passive NFC tag requires separate testing. All illustrations are original and do not use third-party game characters, logos, or screenshots.

## 正式发布前检查

- 在公开版最终提交上运行完整 `./tools/validate.sh`，校验单个合并固件归档，并记录合并镜像、应用和 ELF 哈希；源码变更后重新核对。
- 核对封面格式和大小（PNG、JPEG 或 WebP，≤10 MiB），目视检查全部预览图。
- 扫描公开版全部文件和 Git 历史，排除私人头像像素、私人交接信息、凭据、设备私有链接和原始串口日志。
- 实机检查公开固件的复古卡通人物、到点亮屏与提示音、待确认提醒 30 秒息屏、按键唤醒和 OK 确认；配网与校时、电量分别检查。合并镜像从 `0x0` 写入可能重置 NVS 和 Wi-Fi 配置；说明影响后单独取得刷机同意。
- 实机验收后审阅全部字段；保留旧仓库私有且不改名，创建另命名的公开仓库，核查公开源码与固件对应，再单独授权官方社区发布助手上传。公开 Git 或社区上传任一步失败均停止并报告。
