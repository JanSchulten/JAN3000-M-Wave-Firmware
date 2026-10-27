"""Generate fonts only for behavior tests; this output is not valid for visual QA."""
import runpy
import sys
from PIL import ImageFont, ImageDraw

original_length = ImageFont.FreeTypeFont.getlength
original_text = ImageDraw.ImageDraw.text

def length(self, text, *args, **kwargs):
    kwargs.pop("features", None)
    return original_length(self, text, *args, **kwargs)

def draw_text(self, xy, text, *args, **kwargs):
    kwargs.pop("features", None)
    return original_text(self, xy, text, *args, **kwargs)

ImageFont.FreeTypeFont.getlength = length
ImageDraw.ImageDraw.text = draw_text
sys.path.insert(0, "tools")
sys.argv = ["tools/gen_aa_font.py", "build/gen/ui_fonts.h", "--preset", "inter-tight"]
runpy.run_path("tools/gen_aa_font.py", run_name="__main__")
