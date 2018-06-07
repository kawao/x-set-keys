# Change Log

## Unreleased

## 1.0.1

* Changed not to get the current state of fcitx at program startup, because fcitx may not return the correct value at automatic startup.
* Improved autorpeat of keyboard, so that it get delay and interval of autorepeat from system.
* Added emacs-like backward-kill-word command to emacslike.conf.
* Added processing to handle when the keyboard modifier map is changed.
* Modified so that the keymap, modifiers and controls of keyboard are not reset at program startup.
