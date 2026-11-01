#!/usr/bin/env python3
# Приёмник для e2e-теста печати: окно с Gtk.Entry, через заданное время
# сохраняет содержимое поля в /tmp/golos_atspi_result.txt.
import sys
import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib

OUT = sys.argv[1] if len(sys.argv) > 1 else "/tmp/golos_atspi_result.txt"


def dump_and_quit(entry):
    with open(OUT, "w") as f:
        f.write(entry.get_text())
    Gtk.main_quit()
    return False


win = Gtk.Window(title="golos-atspi-test")
entry = Gtk.Entry()
win.add(entry)
win.connect("destroy", Gtk.main_quit)
win.show_all()
win.present()
GLib.timeout_add(5000, dump_and_quit, entry)
Gtk.main()
