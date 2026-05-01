# Velox File Manager 🚀

Velox is a lightning-fast, lightweight, and customizable file manager for Linux, written in standard C and GTK+ 3. Built with simplicity in mind, it provides an easy-to-use user interface without the bloated features found in other file managers.

## Features ✨
- **Blazing Fast**: Written in pure C with the GTK3 framework for native performance.
- **Minimal UI**: Focuses on what matters the most – your files. Simple toolbar, clean lists.
- **Easy Extension**: Custom sort functions, straightforward codebase (`main.c` is very readable).
- **System Integration**: Instantly double-click files to open them with your system's default applications (`xdg-open` integration).

## Installation

### Prerequisites
Make sure you have GCC, Make, GTK+ 3, and GLib installed on your system.
On Debian/Ubuntu based systems:
```bash
sudo apt install build-essential libgtk-3-dev libglib2.0-dev
```

On Arch Linux:
```bash
sudo pacman -S base-devel gtk3 glib2
```

### Building from Source
Simply run `make` in the root of the project to build the file manager!

```bash
git clone https://github.com/yourusername/velox.git
cd velox
make
```

### Installing System-wide
If you'd like to install Velox and its icon:
```bash
sudo make install
```
Then you can run `velox` from your terminal or application launcher!

## Customization 🎨
The file manager's CSS styling and sorting methods are fully exposed in `src/main.c`. You can tweak the appearance or change the way directories and files are sorted very easily by editing the source code and typing `make`.


