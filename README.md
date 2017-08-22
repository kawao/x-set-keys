# x-set-keys

x-set-key is a key remapper for X Window System on Linux.
It allows you to change key sequence to another key sequence.

## Features

- Supports emacs-like Cut/Copy and Paste keybindings.
- Supports multiple storoke keybindings.
For example, you can map key sequence Control+x and then Control+c to Alt+F4 (Quit application).
For another example, you can map Control+k to key sequence Shift+End and then Constol+x (Kill line).
- You can send any key to application.
For the above example you can send Control+k to application directly, not only used for kill-line.
- You can specify imput method of fcitx to exclude, for example mozc (Disable key remapping while specified fcitx input method is active).

By using sample configuration file emacslike.conf, you can use any applications with Emacs-like keybindings running on X Window System on Linux.

## Installation

### Build dependencies

- GNU C compiler (gcc package for Debian/Ubuntu)
- X11 client-side library (libx11-dev package for Debian/Ubuntu)
- Development files for the GLib library (libglib2.0-dev package for Debian/Ubuntu)

### Acquire source code

Choose one of the following method:

- Download and extract source code archive from GitHub
- Cloning Git repository to local machine using:

```sh
$ git clone https://github.com/kawao/x-set-keys
```

### Build and Install

```sh
$ cd x-set-keys
$ make
$ sudo make install
```

By default, make install will install x-se-keys command to /usr/local/bin directory.
If you want to install in $HOME/bin directory, you could do:

```sh
$ make PREFIX=$HOME install
```

Or:

```sh
$ make INSTALLBIN=$HOME/bin install
```

## Configuration File

Sample configuration file emacslike.conf provides Emacs-like keybindings.
This section explains the contents of this file.

### Basic key mappings

```
C-i :: Tab
C-m :: Return
C-g :: Escape
C-h :: BackSpace
C-d :: Delete
```

The above meanings are as follows.

- Control+i maps to Tab
- Control+m maps to Enter
- Control+h maps to BackSpace
- Control+d maps to Delete

Modifier keys are written as follow.:

- **A-** : Alt
- **C-** : Control
- **M-** : Meta
- **S-** : Shift
- **s-** : super

Key names are found in the header file [X11/keysymdef.h](https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h) (remove the `XK_` prefix).

### Cursor navigation

```
C-a :: Home
C-e :: End
C-b :: Left
C-f :: Right
C-p :: Up
C-n :: Down
A-v :: Page_Up
C-v :: Page_Down
A-b :: C-Left
A-f :: C-Right
C-bracketleft S-bracketleft :: C-Up
C-bracketleft S-bracketright :: C-Down
C-bracketleft S-comma :: C-Home
C-bracketleft S-period :: C-End
```

The above meanings are as follows.

- Control+a maps to Home
- Control+e maps to End
- Control+b maps to Left arrow
- Control+f maps to Right arrow
- Control+p maps to Up arrow
- Control+n maps to Down arrow
- Alt+v maps to Page Up
- Control+v maps to Page Down
- Alt+b maps to Control+Left (Go to previous word)
- Alt+f maps to Control+Right (Go to next word)
- Control+\[ and then \{ maps to Control+Up (Go to previous line break)
- Control+\[ and then \} maps to Control+Down (Go to next line break)
- Control+\[ and then < maps to Control+Home (Go to start of document)
- Control+\[ and then > maps to Control+End (Go to end of document)

### Cut/Copy and Paste

```
C-space :: $select
A-w :: C-c
C-w :: C-x
C-y :: C-v
A-d :: S-C-Right C-x
C-k :: S-End C-x
```

*$select* is the special notation that means start/end selection.

To copy(cut) text, type Constrl+space and then move text cursor ( type cursor navigation key ), and then type Alt+w(Control+w).

Other meanings are as follows.

- Control+y maps to Control+v (Paste)
- Alt+d maps to Shift+Control+Right and then Constol+x (Kill word)
- Control+k maps to Shift+End and then Constol+x (Kill line)

### Undo, Find and File(Window) commands

```
C-slash :: C-z
C-s :: C-f
C-r :: C-S-g
C-x C-f :: C-o
C-x C-s :: C-s
C-x k :: C-w
C-x C-c :: A-F4
```

The above meanings are as follows.

- Control+/ maps to Control+z (Undo)
- Control+s maps to Control+f (Find)
- Control+r maps to Control+Shift+g (Find previous)
- Control+x and then Control+f maps to Control+o (Open file)
- Control+x and then Control+s maps to Control+s (Save file)
- Control+x and then k maps to Control+w (Close file/tab)
- Control+x and then Control+c maps to Alt+F4 (Quit application)

### Send any key to application

```
C-q C-q :: C-q
```

If a key that is not defined in the configuration file is typed, then the key is sent to application directly.
So any key type next of Control+q is sent to application directly.

## Usage

```
# x-set-keys [OPTION...] <configuration-file>
```

x-set-keys command requires you to have root privileges to run.

### Options

#### -h, --help

Print usage statement.

#### -d, --device-file=`<devicefile>`

Specify keyboard device file.
If this option is omited then x-set-keys will search keyboard device from /dev/input/event\* and use the first found.

#### -e, --exclude-focus-class=`<classname>`

Specify excluded class of input focus window.
The `xprop WM_CLASS` commands are useful in determining the class of a window.
This option can be specified multiple times.

#### -f, --exclude-fcitx-im=`<inputmethod>`

Specify excluded input method of fcitx, for example mozc.
To use this option you must run x-set-keys with sudo, and need to take over the environment variable `DBUS_SESSION_BUS_ADDRESS` from before sudo (See Example section below).
This option can be specified multiple times.

### Example

```sh
$ sudo -b /usr/local/bin/x-set-keys \
     --exclude-focus-class=emacs --exclude-focus-class=Gnome-terminal \
     /usr/local/etc/x-set-keys/emacslike.conf
```

The above example run x-set-keys with configuration file /usr/local/etc/x-set-keys/emacslike.conf, excluding emacs and gnome-terminal.

```sh
$ sudo -b G_MESSAGES_PREFIXED=all /usr/local/bin/x-set-keys \
     --device-file=/dev/input/by-id/usb-Topre_Corporation_Realforce-event-kbd \
     --exclude-focus-class=emacs24 /usr/local/etc/x-set-keys/emacslike.conf
```

The above example specify Realforce of Topre corporation as keyboard device.
Environment variable `G_MESSAGES_PREFIXED=all` means that output messages should be prefixed by the program name and PID of x-set-keys.

```sh
$ sudo -b G_MESSAGES_PREFIXED=all \
     DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS \
     /usr/local/bin/x-set-keys \
       --exclude-focus-class=Emacs \
       --exclude-fcitx-im=mozc \
       /usr/local/etc/x-set-keys/emacslike.conf
```

In the above example key remapping is disabled while the input method mozc of fcitx is active.
To do this, the environment variable `DBUS_SESSION_BUS_ADDRESS` must be taken over from before sudo.

### Run x-set-keys without password

To run x-set-keys without password, add following line to the bottom of the file /etc/sudoers by visudo command:

```
yourname ALL=(ALL) SETENV:NOPASSWD: /usr/local/bin/x-set-keys
```

## Related Works

- [autokey](https://code.google.com/archive/p/autokey/)
- [x11keymacs](http://yashiromann.sakura.ne.jp/x11keymacs/index-en.html)
- [chromemacs](https://github.com/zenozeng/chromemacs)

## Licence

[GPLv3](http://www.gnu.org/licenses/gpl.html)

## Author

Tomoyuki Kawao
