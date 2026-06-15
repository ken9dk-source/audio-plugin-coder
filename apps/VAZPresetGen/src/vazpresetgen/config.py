"""Runtime paths + constants. Resolves bundled data whether running from source or as a
PyInstaller-frozen .exe (sys._MEIPASS)."""
from __future__ import annotations
import sys, os
from pathlib import Path

APP_NAME = "VAZ AI Preset Generator"
APP_VERSION = "1.0.0"
CATEGORIES = ["lead", "supersaw", "pad", "pluck", "bass", "fx"]


def _base() -> Path:
    if getattr(sys, "frozen", False):                       # PyInstaller: data bundled into the app
        return Path(getattr(sys, "_MEIPASS", os.path.dirname(sys.executable)))
    return Path(__file__).resolve().parents[2]              # apps/VAZPresetGen/


BASE = _base()
DATA_DIR = BASE / "data"
ASSETS_DIR = BASE / "assets"
MODEL_PATH = DATA_DIR / "trance_model.json"
TEMPLATE_PATH = ASSETS_DIR / "init_template.v2p"


def default_output_dir() -> Path:
    return Path.home() / "Desktop" / "VAZ Generated Presets"
