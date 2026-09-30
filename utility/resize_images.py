from pathlib import Path
from PIL import Image

INPUT_DIR = Path("../image")
OUTPUT_DIR = Path("../image")

OUTPUT_DIR.mkdir(exist_ok=True)

for input_path in INPUT_DIR.glob("*.png"):
    output_path = OUTPUT_DIR / input_path.name

    with Image.open(input_path) as image:
        image = image.convert("RGBA")
        image = image.resize((120, 120), Image.Resampling.LANCZOS)
        image.save(output_path)

    print(f"Saved {output_path}")

