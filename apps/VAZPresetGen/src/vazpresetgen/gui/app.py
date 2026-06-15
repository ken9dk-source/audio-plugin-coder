"""Modern desktop GUI (CustomTkinter) for the VAZ AI Preset Generator.

Type a prompt ("uplifting trance lead"), pick a batch size, Generate -> a list of valid presets
appears; tick the ones you like and Save (.v2p) or Evolve into variations. All generation runs
through the validated engine; nothing here invents parameters."""
from __future__ import annotations
import os
from pathlib import Path

import customtkinter as ctk
from tkinter import filedialog, messagebox

from ..engine import PresetService, Preset
from ..config import APP_NAME, APP_VERSION, CATEGORIES, default_output_dir

SUBGENRES = ["uplifting", "asot", "classic", "progressive", "psy"]
BATCHES = ["1", "10", "50", "100"]
BADGE = {"lead": "#3b8eea", "supersaw": "#2e6fd6", "pad": "#7a5cff", "pluck": "#e0883a",
         "bass": "#d94f4f", "fx": "#36b37e"}


class VAZApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")
        self.title(f"{APP_NAME}  v{APP_VERSION}")
        self.geometry("1040x680")
        self.minsize(900, 560)

        self.service = PresetService()
        self.presets: list[Preset] = []
        self._rows: list[tuple[Preset, ctk.BooleanVar, ctk.CTkFrame]] = []
        self.out_dir = ctk.StringVar(value=str(default_output_dir()))

        self._build()

    # ---------------------------------------------------------------- layout
    def _build(self):
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)

        side = ctk.CTkFrame(self, width=300, corner_radius=0)
        side.grid(row=0, column=0, sticky="nsw")
        side.grid_propagate(False)
        ctk.CTkLabel(side, text="VAZ AI\nPreset Generator", font=("Segoe UI", 22, "bold"),
                     justify="left").pack(anchor="w", padx=20, pady=(20, 4))
        ctk.CTkLabel(side, text="Uplifting / ASOT / classic trance\nfor the VAZ 2010 clone",
                     text_color="#9aa4b2", justify="left").pack(anchor="w", padx=20, pady=(0, 16))

        ctk.CTkLabel(side, text="Prompt", anchor="w").pack(fill="x", padx=20)
        self.prompt = ctk.CTkEntry(side, placeholder_text="e.g. uplifting trance lead")
        self.prompt.pack(fill="x", padx=20, pady=(2, 12))
        self.prompt.bind("<Return>", lambda _e: self.on_generate())

        ctk.CTkLabel(side, text="Category", anchor="w").pack(fill="x", padx=20)
        self.category = ctk.CTkOptionMenu(side, values=CATEGORIES)
        self.category.set("lead")
        self.category.pack(fill="x", padx=20, pady=(2, 10))

        ctk.CTkLabel(side, text="Trance style", anchor="w").pack(fill="x", padx=20)
        self.subgenre = ctk.CTkOptionMenu(side, values=SUBGENRES)
        self.subgenre.set("uplifting")
        self.subgenre.pack(fill="x", padx=20, pady=(2, 10))

        ctk.CTkLabel(side, text="Batch size", anchor="w").pack(fill="x", padx=20)
        self.batch = ctk.CTkSegmentedButton(side, values=BATCHES)
        self.batch.set("10")
        self.batch.pack(fill="x", padx=20, pady=(2, 16))

        ctk.CTkButton(side, text="Generate", height=40, font=("Segoe UI", 15, "bold"),
                      command=self.on_generate).pack(fill="x", padx=20, pady=(0, 8))
        ctk.CTkButton(side, text="Evolve selected", fg_color="#3a3f4b", hover_color="#4a505e",
                      command=self.on_evolve).pack(fill="x", padx=20, pady=(0, 16))

        ctk.CTkLabel(side, text="Output folder", anchor="w").pack(fill="x", padx=20)
        row = ctk.CTkFrame(side, fg_color="transparent")
        row.pack(fill="x", padx=20, pady=(2, 8))
        ctk.CTkEntry(row, textvariable=self.out_dir).pack(side="left", fill="x", expand=True)
        ctk.CTkButton(row, text="…", width=34, command=self.on_browse).pack(side="left", padx=(6, 0))
        ctk.CTkButton(side, text="Save selected (.v2p)", command=lambda: self.on_save(False)).pack(fill="x", padx=20, pady=(0, 6))
        ctk.CTkButton(side, text="Save all (.v2p)", command=lambda: self.on_save(True)).pack(fill="x", padx=20, pady=(0, 8))

        # main results area
        main = ctk.CTkFrame(self, corner_radius=0, fg_color="#1a1d23")
        main.grid(row=0, column=1, sticky="nsew")
        main.grid_columnconfigure(0, weight=1)
        main.grid_rowconfigure(1, weight=1)
        head = ctk.CTkFrame(main, fg_color="transparent")
        head.grid(row=0, column=0, sticky="ew", padx=16, pady=(14, 6))
        ctk.CTkLabel(head, text="Generated presets", font=("Segoe UI", 16, "bold")).pack(side="left")
        self.count_lbl = ctk.CTkLabel(head, text="", text_color="#9aa4b2")
        self.count_lbl.pack(side="right")
        self.list = ctk.CTkScrollableFrame(main, fg_color="transparent")
        self.list.grid(row=1, column=0, sticky="nsew", padx=8, pady=4)
        self.list.grid_columnconfigure(0, weight=1)

        self.status = ctk.CTkLabel(self, text="Ready.", anchor="w", text_color="#9aa4b2")
        self.status.grid(row=1, column=0, columnspan=2, sticky="ew", padx=16, pady=(0, 6))

    # ---------------------------------------------------------------- actions
    def _set_status(self, msg: str):
        self.status.configure(text=msg)
        self.update_idletasks()

    def on_generate(self):
        prompt = self.prompt.get().strip()
        if prompt:
            from ..engine import classify_prompt
            cat, sub = classify_prompt(prompt)
            self.category.set(cat)
            self.subgenre.set(sub)
        cat, sub, n = self.category.get(), self.subgenre.get(), int(self.batch.get())
        self._set_status(f"Generating {n} {sub} {cat} preset(s)…")
        try:
            self.presets = self.service.batch(cat, sub, n)
        except Exception as e:                                    # pragma: no cover
            messagebox.showerror(APP_NAME, f"Generation failed:\n{e}")
            return
        self._render()
        self._set_status(f"Generated {len(self.presets)} preset(s). Tick some and Save, or Evolve.")

    def on_evolve(self):
        sel = [p for p, v, _ in self._rows if v.get()]
        if not sel:
            messagebox.showinfo(APP_NAME, "Tick a preset first, then Evolve.")
            return
        base = sel[0]
        self._set_status(f"Evolving '{base.name}'…")
        variations = self.service.evolve(base, count=8)
        self.presets = variations + self.presets
        self._render()
        self._set_status(f"Created {len(variations)} variations of '{base.name}'.")

    def on_browse(self):
        d = filedialog.askdirectory(initialdir=self.out_dir.get() or os.path.expanduser("~"))
        if d:
            self.out_dir.set(d)

    def on_save(self, all_: bool):
        targets = self.presets if all_ else [p for p, v, _ in self._rows if v.get()]
        if not targets:
            messagebox.showinfo(APP_NAME, "Nothing to save — generate, then tick presets (or use Save all).")
            return
        try:
            paths = self.service.save(targets, self.out_dir.get())
        except Exception as e:                                    # pragma: no cover
            messagebox.showerror(APP_NAME, f"Save failed:\n{e}")
            return
        self._set_status(f"Saved {len(paths)} .v2p file(s) to {self.out_dir.get()}")
        messagebox.showinfo(APP_NAME, f"Saved {len(paths)} preset(s) to:\n{self.out_dir.get()}")

    # ---------------------------------------------------------------- rendering
    def _render(self):
        for _p, _v, frame in self._rows:
            frame.destroy()
        self._rows.clear()
        for ps in self.presets:
            f = ctk.CTkFrame(self.list, fg_color="#22262e", corner_radius=8)
            f.pack(fill="x", padx=6, pady=4)
            f.grid_columnconfigure(2, weight=1)
            var = ctk.BooleanVar(value=False)
            ctk.CTkCheckBox(f, text="", variable=var, width=24).grid(row=0, column=0, rowspan=2, padx=(10, 4), pady=8)
            ctk.CTkLabel(f, text=ps.category.upper(), width=72, corner_radius=6,
                         fg_color=BADGE.get(ps.category, "#3b8eea"), text_color="white",
                         font=("Segoe UI", 10, "bold")).grid(row=0, column=1, rowspan=2, padx=4)
            ctk.CTkLabel(f, text=ps.name, anchor="w", font=("Segoe UI", 13, "bold")).grid(row=0, column=2, sticky="w", padx=8, pady=(8, 0))
            ctk.CTkLabel(f, text=ps.summary(), anchor="w", text_color="#9aa4b2",
                         font=("Consolas", 11)).grid(row=1, column=2, sticky="w", padx=8, pady=(0, 8))
            ok = "✓ valid" if ps.qc_ok else "✗ " + "; ".join(ps.qc_notes)
            ctk.CTkLabel(f, text=ok, text_color="#36b37e" if ps.qc_ok else "#d94f4f").grid(row=0, column=3, rowspan=2, padx=12)
            self._rows.append((ps, var, f))
        self.count_lbl.configure(text=f"{len(self.presets)} preset(s)")


def run():
    VAZApp().mainloop()
