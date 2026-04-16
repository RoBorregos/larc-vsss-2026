"""Tk sidebar controls for the VSSS simulator (field-colored strip)."""

from __future__ import annotations

import tkinter as tk
import tkinter.font as tkfont

# Same fill as the playable rectangle in strategy.field.field.draw_field
FIELD_SURFACE = "#000000"
FIELD_LINE = "#3d3d3d"


class TeamSidebar(tk.Frame):
    """
    Left vertical strip: same black as the pitch, team choice as two tall buttons.

    If ``run_var`` is passed, it tracks strategy halt: True = stopped, False = running.
    """

    def __init__(
        self,
        parent: tk.Misc,
        *,
        initial_team: str = "YELLOW",
        width: int = 200,
        run_var: tk.BooleanVar | None = None,
    ):
        super().__init__(
            parent,
            bg=FIELD_SURFACE,
            width=width,
            highlightthickness=1,
            highlightbackground=FIELD_LINE,
        )
        self.pack_propagate(False)
        self._team = initial_team
        self._run_var = run_var

        title_f = tkfont.Font(font=tkfont.nametofont("TkDefaultFont"))
        title_f.configure(size=15, weight="bold")
        label_f = tkfont.Font(font=tkfont.nametofont("TkDefaultFont"))
        label_f.configure(size=9)
        btn_f = tkfont.Font(font=tkfont.nametofont("TkDefaultFont"))
        btn_f.configure(size=11, weight="bold")
        hint_f = tkfont.Font(font=tkfont.nametofont("TkDefaultFont"))
        hint_f.configure(size=8)

        pad = tk.Frame(self, bg=FIELD_SURFACE)
        pad.pack(fill=tk.BOTH, expand=True, padx=14, pady=16)

        tk.Label(
            pad,
            text="VSSS",
            font=title_f,
            fg="#f3f4f6",
            bg=FIELD_SURFACE,
            anchor="w",
        ).pack(fill=tk.X)
        tk.Label(
            pad,
            text="Simulator",
            font=label_f,
            fg="#9ca3af",
            bg=FIELD_SURFACE,
            anchor="w",
        ).pack(fill=tk.X, pady=(0, 6))

        tk.Frame(pad, bg=FIELD_LINE, height=1).pack(fill=tk.X, pady=(8, 14))

        tk.Label(
            pad,
            text="YOUR TEAM",
            font=hint_f,
            fg="#6b7280",
            bg=FIELD_SURFACE,
            anchor="w",
        ).pack(fill=tk.X, pady=(0, 10))

        self._btn_yellow = self._team_button(
            pad,
            "YELLOW",
            "Yellow",
            btn_f,
            selected_bg="#2a2314",
            selected_fg="#f5d547",
            accent="#d4a012",
            dim_fg="#6b5f2a",
        )
        self._btn_blue = self._team_button(
            pad,
            "BLUE",
            "Blue",
            btn_f,
            selected_bg="#0f1f33",
            selected_fg="#93c5fd",
            accent="#2563eb",
            dim_fg="#3d5b82",
        )

        self._btn_yellow.pack(fill=tk.X, pady=(0, 8))
        self._btn_blue.pack(fill=tk.X)

        if self._run_var is not None:
            tk.Label(
                pad,
                text="STRATEGY",
                font=hint_f,
                fg="#6b7280",
                bg=FIELD_SURFACE,
                anchor="w",
            ).pack(fill=tk.X, pady=(20, 10))
            ctrl_f = tkfont.Font(font=tkfont.nametofont("TkDefaultFont"))
            ctrl_f.configure(size=10, weight="bold")
            run_row = tk.Frame(pad, bg=FIELD_SURFACE)
            run_row.pack(fill=tk.X)
            self._btn_stop = tk.Button(
                run_row,
                text="■  Stop",
                font=ctrl_f,
                relief=tk.FLAT,
                cursor="hand2",
                borderwidth=0,
                padx=8,
                pady=11,
                command=self._on_stop_click,
            )
            self._btn_resume = tk.Button(
                run_row,
                text="▶  Resume",
                font=ctrl_f,
                relief=tk.FLAT,
                cursor="hand2",
                borderwidth=0,
                padx=8,
                pady=11,
                command=self._on_resume_click,
            )
            self._btn_stop.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))
            self._btn_resume.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(5, 0))
            self._paint_run_buttons()

        tk.Label(
            pad,
            text="Click the pitch to place the ball\nand stop your robots.",
            font=hint_f,
            fg="#525a66",
            bg=FIELD_SURFACE,
            anchor="w",
            justify=tk.LEFT,
        ).pack(fill=tk.X, side=tk.BOTTOM, pady=(18, 0))

        self._paint_buttons()

    @property
    def team(self) -> str:
        return self._team

    def _on_stop_click(self) -> None:
        if self._run_var is None or self._run_var.get():
            return
        self._run_var.set(True)
        self._paint_run_buttons()

    def _on_resume_click(self) -> None:
        if self._run_var is None or not self._run_var.get():
            return
        self._run_var.set(False)
        self._paint_run_buttons()

    def _paint_run_buttons(self) -> None:
        if self._run_var is None or not hasattr(self, "_btn_stop"):
            return
        stopped = self._run_var.get()
        dim_fg = "#5c6370"
        dim_bg = "#0d0d0d"
        dim_hi = "#252525"

        if stopped:
            self._btn_stop.config(
                cursor="arrow",
                fg=dim_fg,
                bg=dim_bg,
                activeforeground=dim_fg,
                activebackground=dim_bg,
                highlightthickness=1,
                highlightbackground=dim_hi,
                highlightcolor=dim_hi,
            )
            self._btn_resume.config(
                cursor="hand2",
                fg="#bbf7d0",
                bg="#0f1f14",
                activeforeground="#d1fae5",
                activebackground="#14291c",
                highlightthickness=2,
                highlightbackground="#22c55e",
                highlightcolor="#22c55e",
            )
        else:
            self._btn_stop.config(
                cursor="hand2",
                fg="#fecaca",
                bg="#1f1212",
                activeforeground="#fecaca",
                activebackground="#2a1818",
                highlightthickness=2,
                highlightbackground="#ef4444",
                highlightcolor="#ef4444",
            )
            self._btn_resume.config(
                cursor="arrow",
                fg=dim_fg,
                bg=dim_bg,
                activeforeground=dim_fg,
                activebackground=dim_bg,
                highlightthickness=1,
                highlightbackground=dim_hi,
                highlightcolor=dim_hi,
            )

    def _team_button(
        self,
        parent: tk.Misc,
        value: str,
        title: str,
        font: tkfont.Font,
        *,
        selected_bg: str,
        selected_fg: str,
        accent: str,
        dim_fg: str,
    ) -> tk.Button:
        btn = tk.Button(
            parent,
            text=title,
            font=font,
            relief=tk.FLAT,
            cursor="hand2",
            borderwidth=0,
            padx=12,
            pady=14,
            activeforeground=selected_fg,
            command=lambda: self._set_team(value),
        )
        btn._sidebar_meta = {  # type: ignore[attr-defined]
            "value": value,
            "selected_bg": selected_bg,
            "selected_fg": selected_fg,
            "accent": accent,
            "dim_fg": dim_fg,
        }
        return btn

    def _set_team(self, value: str) -> None:
        if self._team == value:
            return
        self._team = value
        self._paint_buttons()

    def _paint_buttons(self) -> None:
        for btn in (self._btn_yellow, self._btn_blue):
            meta = btn._sidebar_meta  # type: ignore[attr-defined]
            sel = meta["value"] == self._team
            if sel:
                btn.config(
                    bg=meta["selected_bg"],
                    fg=meta["selected_fg"],
                    highlightthickness=2,
                    highlightbackground=meta["accent"],
                    highlightcolor=meta["accent"],
                    activebackground=meta["selected_bg"],
                )
            else:
                btn.config(
                    bg="#0a0a0a",
                    fg=meta["dim_fg"],
                    highlightthickness=1,
                    highlightbackground="#1f1f1f",
                    highlightcolor="#1f1f1f",
                    activebackground="#141414",
                )
