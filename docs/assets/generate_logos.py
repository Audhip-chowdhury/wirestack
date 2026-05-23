"""Generate WireStack brand PNG assets for PDF and web."""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ASSETS = Path(__file__).resolve().parent
NAVY = (11, 31, 58)
BLUE = (30, 111, 255)
TEAL = (0, 201, 167)
WHITE = (255, 255, 255)
SLATE = (148, 163, 184)


def _font(size: int, bold: bool = False):
    names = (
        ["arialbd.ttf", "Arial Bold.ttf", "DejaVuSans-Bold.ttf"]
        if bold
        else ["arial.ttf", "Arial.ttf", "DejaVuSans.ttf"]
    )
    for name in names:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def draw_icon(size: int = 512) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    pad = size // 7
    w = size - 2 * pad
    layers = 5
    step = max(14, (w - 40) // layers)
    for i in range(layers):
        y = pad + i * (step + 6)
        bar_w = w - i * (w // 14)
        hue = BLUE if i < 3 else TEAL
        d.rounded_rectangle(
            [pad, y, pad + bar_w, y + step],
            radius=max(4, size // 64),
            fill=(*hue, 255 - i * 18),
        )
    return img


def draw_wordmark(width: int = 1400, height: int = 300) -> Image.Image:
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    icon_sz = int(height * 0.72)
    icon = draw_icon(icon_sz)
    img.paste(icon, (0, (height - icon_sz) // 2), icon)
    ix = icon_sz + int(width * 0.04)
    font = _font(int(height * 0.38), bold=True)
    tag = _font(int(height * 0.11))
    d.text((ix, int(height * 0.22)), "Wire", fill=WHITE, font=font)
    wire_bbox = d.textbbox((0, 0), "Wire", font=font)
    wire_w = wire_bbox[2] - wire_bbox[0]
    d.text((ix + wire_w, int(height * 0.22)), "Stack", fill=BLUE, font=font)
    d.text((ix, int(height * 0.62)), "SEE THE WIRE. OWN THE STACK.", fill=SLATE, font=tag)
    return img


def draw_wordmark_dark_bg(width: int = 1400, height: int = 320) -> Image.Image:
    img = Image.new("RGBA", (width, height), (*NAVY, 255))
    overlay = draw_wordmark(width, height)
    img.paste(overlay, (0, (height - overlay.height) // 2), overlay)
    return img


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    draw_icon(1024).save(ASSETS / "wirestack-icon.png", optimize=True)
    draw_wordmark(1400, 300).save(ASSETS / "wirestack-wordmark-light.png", optimize=True)
    draw_wordmark_dark_bg(1400, 320).save(
        ASSETS / "wirestack-wordmark-dark.png", optimize=True
    )
    print("Wrote logos to", ASSETS)


if __name__ == "__main__":
    main()
