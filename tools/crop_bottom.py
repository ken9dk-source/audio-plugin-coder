from PIL import Image
SP = r"C:\Users\ken98\AppData\Local\Temp\claude\C--APC-y\702f4c83-da56-446e-9f5c-d61876c92bfd\scratchpad"
im = Image.open(SP + r"\vazclone_menu.png")
w, h = im.size
crop = im.crop((0, h - 80, w, h)).resize((w * 2, 160))
crop.save(SP + r"\vazclone_menu_bottom.png")
print("bottom strip", im.size, "->", crop.size)
