from pathlib import Path
from PIL import Image

for path in Path("../image").glob("*.png"):
    with Image.open(path) as image:
        print(path.name, image.size)

