"""Launch the VAZ AI Preset Generator desktop app (from source).

    py run.py
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "src"))

from vazpresetgen.gui import run

if __name__ == "__main__":
    run()
