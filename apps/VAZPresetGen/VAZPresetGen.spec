# PyInstaller spec — cross-platform, single-file, windowed (no console / no terminal).
#   Windows :  py  -m PyInstaller --noconfirm VAZPresetGen.spec  -> dist/VAZ Preset Generator.exe
#   macOS   :  python3 -m PyInstaller --noconfirm VAZPresetGen.spec -> dist/VAZ Preset Generator.app
#   Linux   :  python3 -m PyInstaller --noconfirm VAZPresetGen.spec -> dist/VAZ Preset Generator
# PyInstaller cannot cross-compile: build the macOS .app ON a Mac, the .exe ON Windows.
import sys
from PyInstaller.utils.hooks import collect_data_files

block_cipher = None

# Bundle the rule model + the v2.0 .v2p template, plus CustomTkinter's theme assets.
datas = [
    ("data/trance_model.json", "data"),
    ("assets/init_template.v2p", "assets"),
]
datas += collect_data_files("customtkinter")

a = Analysis(
    ["run.py"],
    pathex=["src"],
    binaries=[],
    datas=datas,
    hiddenimports=["customtkinter"],
    hookspath=[],
    runtime_hooks=[],
    excludes=["numpy", "scipy", "PIL.ImageQt"],
    cipher=block_cipher,
)
pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)
exe = EXE(
    pyz, a.scripts, a.binaries, a.zipfiles, a.datas, [],
    name="VAZ Preset Generator",
    debug=False, bootloader_ignore_signals=False, strip=False, upx=(sys.platform == "win32"),
    runtime_tmpdir=None, console=False, disable_windowed_traceback=False,
    icon=None,
)

# macOS: wrap the single-file executable in a proper .app bundle.
if sys.platform == "darwin":
    app = BUNDLE(
        exe,
        name="VAZ Preset Generator.app",
        icon=None,
        bundle_identifier="com.apc.vazpresetgen",
        info_plist={"NSHighResolutionCapable": True, "CFBundleShortVersionString": "1.0.0"},
    )
