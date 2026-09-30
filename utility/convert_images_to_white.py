from pathlib import Path
from PIL import Image

INPUT_DIR = Path("../image")
OUTPUT_DIR = Path("../image")

OUTPUT_DIR.mkdir(exist_ok=True)

for input_path in INPUT_DIR.glob("*.png"):
    output_path = OUTPUT_DIR / input_path.name

    with Image.open(input_path) as image:
        image = image.convert("RGBA")

        # Preserve the original alpha channel
        alpha = image.getchannel("A")

        # Create a white image using the original transparency
        white_image = Image.new("RGBA", image.size, (255, 255, 255, 255))
        white_image.putalpha(alpha)

        white_image.save(output_path)

    print(f"Converted: {input_path} -> {output_path}")
