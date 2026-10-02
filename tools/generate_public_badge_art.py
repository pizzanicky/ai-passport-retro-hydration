#!/usr/bin/env python3
"""Generate original, non-personal terminal figure and community artwork."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets' / 'images'
OUT.mkdir(parents=True, exist_ok=True)
FONT = ROOT / 'assets' / 'fonts' / 'Rajdhani-Bold.ttf'
CJK = Path('/System/Library/Fonts/STHeiti Medium.ttc')
BG = (6, 18, 10)
PANEL = (18, 40, 23)
INK = (162, 255, 105)
DIM = (87, 153, 77)

def font(size, chinese=False):
    return ImageFont.truetype(str(CJK if chinese and CJK.exists() else FONT), size)

def figure(width=112, height=132):
    """Crop the original character master for the small LCD, then resample."""
    master = Image.open(OUT / 'badge-default-avatar-master.png').convert('RGBA')
    source_w, source_h = master.size
    target_ratio = width / height
    source_ratio = source_w / source_h
    if source_ratio < target_ratio:
        crop_h = int(source_w / target_ratio)
        master = master.crop((0, 0, source_w, crop_h))
    else:
        crop_w = int(source_h * target_ratio)
        left = (source_w - crop_w) // 2
        master = master.crop((left, 0, left + crop_w, source_h))
    return master.resize((width, height), Image.Resampling.LANCZOS)

avatar=figure()
avatar.save(OUT/'badge-default-avatar.png')
raw=avatar.tobytes('raw','BGRA')
chunks=[raw[i:i+12] for i in range(0,len(raw),12)]
rows=['    '+', '.join(f'0x{v:02x}' for v in chunk)+',' for chunk in chunks]
(OUT/'badge_default_avatar.c').write_text('#include "lvgl.h"\n\n/* Original retro cartoon from badge-default-avatar-master.png, converted for LVGL. */\nstatic const uint8_t avatar_pixels[] = {\n'+'\n'.join(rows)+'\n};\n\nconst lv_image_dsc_t badge_default_avatar = {\n    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_ARGB8888, .w = 112, .h = 132, .stride = 448},\n    .data_size = sizeof(avatar_pixels),\n    .data = avatar_pixels,\n};\n')

# A desktop layout preview with the same dimensions as the LCD.
id=Image.new('RGB',(240,320),BG); d=ImageDraw.Draw(id)
d.text((19,12),'IDENTITY / 01',font=font(20),fill=INK)
d.text((196,12),'98%',font=font(18),fill=INK)
d.line((18,42,222,42),fill=INK,width=2)
d.text((25,49),'PERSONNEL RECORD',font=font(13),fill=DIM)
id.paste(avatar,(64,66),avatar)
d.line((25,208,215,208),fill=DIM,width=1)
d.text((27,212),'USER-01',font=font(28),fill=INK)
d.text((27,244),'PASSIVE NFC / TAG',font=font(16),fill=DIM)
d.line((18,280,222,280),fill=INK,width=2)
d.text((27,288),'HOLD OK  BACK',font=font(16),fill=INK)
id.save(OUT/'badge-identity-preview.png')

# Original community cover, no game character, logo, screenshot, or personal image.
w,h=1280,720
cover=Image.new('RGB',(w,h),BG);d=ImageDraw.Draw(cover)
for x in range(0,w,40): d.line((x,0,x,h),fill=(12,31,15),width=1)
for y in range(0,h,40): d.line((0,y,w,y),fill=(12,31,15),width=1)
d.rounded_rectangle((48,55,1232,665),radius=28,fill=(9,25,13),outline=DIM,width=3)
d.line((84,108,1196,108),fill=INK,width=3)
d.text((88,69),'FOLOTOY / AI PASSPORT  ·  COMMUNITY APPLICATION',font=font(27),fill=DIM)
# Device and LCD
box=(105,154,444,625)
d.rounded_rectangle(box,radius=42,fill=(35,45,35),outline=(94,110,89),width=6)
d.rounded_rectangle((126,178,423,600),radius=21,fill=BG,outline=INK,width=3)
d.text((151,196),'IDENTITY / 01',font=font(26),fill=INK)
d.line((150,234,398,234),fill=DIM,width=2)
av=figure(190,224); cover.paste(av,(181,250),av)
d.text((160,489),'USER-01',font=font(38),fill=INK)
d.text((160,532),'PASSIVE NFC / TAG',font=font(19),fill=DIM)
d.text((520,168),'复古荧光终端',font=font(51,True),fill=INK)
d.text((520,241),'喝水提醒',font=font(76,True),fill=INK)
d.line((523,350,1166,350),fill=DIM,width=2)
d.text((523,372),'HYDRATION / 30 MIN',font=font(47),fill=INK)
d.text((523,439),'09:00—21:00  ·  EVERY DAY',font=font(30),fill=DIM)
d.text((523,496),'Wi-Fi TIME SYNC  /  BLUFI SETUP',font=font(29),fill=DIM)
d.rounded_rectangle((519,564,1167,624),radius=8,outline=INK,width=2)
d.text((543,576),'ORIGINAL WATER CARTOON  ·  240×320 LCD',font=font(27),fill=INK)
cover.save(OUT/'community-cover.png',optimize=True)
print('Wrote generic avatar, firmware C data, identity preview, community cover')
