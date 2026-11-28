"""Offline desktop prototype. Run python -m tools.teacher_studio."""
import logging
import os
from pathlib import Path
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, simpledialog
from uuid import uuid4

from tools.teacher_accounts import AccountStore
from tools.teacher_sessions import TeacherSessions
from tools.teacher_decks import TeacherDecks
from tools.flashcard_deck import import_csv, MAX_BYTES

LOG = logging.getLogger(__name__)


class Studio:
    def __init__(self, root, directory):
        self.root = root
        self.accounts = AccountStore(directory / "accounts.sqlite")
        self.sessions = TeacherSessions(self.accounts)
        self.decks = TeacherDecks(directory / "decks.sqlite", self.sessions)
        self.token = None
        self.root.title("Pocket Learning Studio")
        self.root.geometry("850x620")
        self.login_screen()

    def clear(self):
        for child in self.root.winfo_children():
            child.destroy()

    def run_action(self, action):
        try:
            action()
        except PermissionError:
            LOG.warning("Studio permission denied")
            messagebox.showerror("Access denied", "Permission denied or session expired.")
        except Exception:
            LOG.exception("Studio operation failed")
            messagebox.showerror("Operation failed", "Check the input or local database.")

    def login_screen(self):
        self.sessions.logout(self.token)
        self.token = None
        self.clear()
        ttk.Label(self.root, text="Pocket Learning Studio", font=("Segoe UI", 24)).pack(pady=25)
        ttk.Label(self.root, text="Username").pack()
        user = ttk.Entry(self.root)
        user.pack(pady=5)
        ttk.Label(self.root, text="Password").pack()
        password = ttk.Entry(self.root, show="*")
        password.pack(pady=5)

        def sign_in():
            try:
                self.token = self.sessions.login(user.get(), password.get())
            finally:
                password.delete(0, tk.END)
            if self.token:
                self.workspace()
            else:
                messagebox.showerror("Sign in", "Invalid credentials or account locked.")

        def bootstrap():
            try:
                self.accounts.create(user.get(), password.get(), "admin", bootstrap=True)
                messagebox.showinfo("Setup", "Administrator created. Sign in to continue.")
            finally:
                password.delete(0, tk.END)

        ttk.Button(self.root, text="Sign in", command=lambda: self.run_action(sign_in)).pack(pady=10)
        ttk.Button(self.root, text="First-time administrator setup",
                   command=lambda: self.run_action(bootstrap)).pack()

    def workspace(self):
        self.sessions.require(self.token)
        self.clear()
        ttk.Label(self.root, text="Flashcard Studio", font=("Segoe UI", 22)).pack(pady=12)
        title = ttk.Entry(self.root, width=55)
        title.pack(pady=5)
        title.insert(0, "New deck")
        ttk.Label(self.root, text="One card per row (CSV: front,back)").pack()
        editor = tk.Text(self.root, height=12, width=75)
        editor.pack(fill="both", expand=True, padx=20)
        editor.insert("1.0", "front,back\n")
        library = tk.Listbox(self.root, height=5)
        library.pack(fill="x", padx=20, pady=8)

        def refresh():
            library.delete(0, tk.END)
            for deck in self.decks.list_decks(self.token):
                library.insert(tk.END, f'{deck["title"]} — {len(deck["cards"])} cards')

        def load():
            self.sessions.require(self.token)
            path = filedialog.askopenfilename(filetypes=[("CSV", "*.csv")])
            if path:
                with open(path, "rb") as file:
                    raw = file.read(MAX_BYTES + 1)
                if len(raw) > MAX_BYTES:
                    raise ValueError("CSV too large")
                editor.delete("1.0", tk.END)
                editor.insert("1.0", raw.decode("utf-8-sig"))

        def save():
            self.sessions.require(self.token)
            deck = import_csv(editor.get("1.0", "end-1c"), uuid4().hex, title.get())
            if messagebox.askyesno("Save deck", f'Save {len(deck["cards"])} cards?'):
                self.decks.save(self.token, deck)
                refresh()

        def add_teacher():
            self.sessions.require(self.token, "admin")
            name = simpledialog.askstring("New teacher", "Username")
            if not name:
                return
            secret = simpledialog.askstring("New teacher", "Password (15+ characters)", show="*")
            if secret:
                self.sessions.create_account(self.token, name, secret, "teacher")

        buttons = ttk.Frame(self.root)
        buttons.pack(pady=10)
        for label, action in [("Import CSV", load), ("Validate and save", save),
                              ("Refresh decks", refresh), ("Add teacher", add_teacher),
                              ("Sign out", self.login_screen)]:
            ttk.Button(buttons, text=label, command=lambda a=action: self.run_action(a)).pack(side="left", padx=3)
        refresh()


def main():
    try:
        logging.basicConfig(level=logging.INFO)
        directory = Path(os.environ.get("LOCALAPPDATA", str(Path.home()))) / "PocketLearningStudio"
        directory.mkdir(parents=True, exist_ok=True)
        root = tk.Tk()
        Studio(root, directory)
        root.mainloop()
    except Exception:
        LOG.exception("Teacher Studio startup failed")
        raise


if __name__ == "__main__":
    main()
