<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

工卡界面使用来自 [Google Fonts](https://github.com/google/fonts/tree/main/ofl/rajdhani) 的 `fonts/Rajdhani-Bold.ttf`，许可为 `fonts/OFL-Rajdhani.txt` 中的 SIL Open Font License。`fonts/badge_font_16.c` 和 `fonts/badge_font_28.c` 包含可打印 ASCII（U+0020–U+007E），由 `lv_font_conv` 1.5.3 以 `--bpp 2 --no-compress --format lvgl`，分别取 `--size 16`、`--size 28` 生成。重建时再传入 `--font assets/fonts/Rajdhani-Bold.ttf -r 0x20-0x7E --lv-font-name badge_font_<size> -o assets/fonts/badge_font_<size>.c`。`main/CMakeLists.txt` 将两份字库编入应用。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |
| [`images/badge-default-avatar-master.png`](images/badge-default-avatar-master.png) | 1086 × 1448，PNG RGBA | 专为本应用生成的原创喝水主题复古卡通原画，未使用用户照片或游戏素材；转换脚本从该原画制作屏幕资源。 |
| [`images/badge-default-avatar.png`](images/badge-default-avatar.png) | 112 × 132，PNG RGBA | 由 `tools/generate_public_badge_art.py` 从原创原画转换的复古卡通人物；未使用真人照片或游戏角色。 |
| [`images/badge_default_avatar.c`](images/badge_default_avatar.c) | 112 × 132，LVGL ARGB8888 C 数据 | 同一图像的固件像素数据；BGRA 字节顺序。 |
| [`images/badge-identity-preview.png`](images/badge-identity-preview.png) | 240 × 320，PNG RGB | 公开版身份页电脑预览；不代表设备实拍效果。 |
| [`images/community-cover.png`](images/community-cover.png) | 1280 × 720，PNG RGB | 社区发布原创封面；不包含游戏角色、标志或截图。 |
| [`images/badge-icon-water.png`](images/badge-icon-water.png)、[`images/badge-icon-medicine.png`](images/badge-icon-medicine.png)、[`images/badge-icon-system.png`](images/badge-icon-system.png) | 每张 80 × 80，PNG RGBA | 工卡子页面的原创荧光绿水滴、胶囊与齿轮图标。由 `python3 tools/generate_badge_page_icons.py` 生成，未使用游戏素材。 |
| [`images/badge_page_icons.c`](images/badge_page_icons.c) | 三张 80 × 80 的 LVGL ARGB8888 图像描述符，像素合计 76,800 字节 | 从上述三张图标生成的固件 BGRA 数据，由 `main/CMakeLists.txt` 编入应用。 |
| [`images/badge-secondary-pages-preview.png`](images/badge-secondary-pages-preview.png) | 720 × 320，PNG RGB | 喝水、吃药和 System 页的电脑排版预览，不代表设备实拍效果。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
