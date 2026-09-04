# HotReloadingD3DCapture

A high-performance C++ and Python research prototype that extracts
synchronized per-pixel object segmentation maps and video streams
from shipped Unreal Engine games running on DirectX 11.
Built to demonstrate advanced systems programming, runtime hooking,
GPU-to-CPU async readbacks, engine introspection, and live DLL
hot-reloading.

---

## Architecture Overview

The solution (`HotReloadingD3DCapture.sln`) is organized into
four core modules:

1. **`Injector` (C++20 Console Executable)**
* Locates the target game process (`Game-Win64-Shipping.exe`),
allocates remote virtual memory, writes the payload DLL path,
and injects `D3DHooksCore.dll` via `CreateRemoteThread`.




2. **`D3DHooksCore` (C++20 Dynamic Link Library)**
* Persistent anchor DLL inside the game process. Allocates
a live debug console, initializes **MinHook** for safe Vtable
detouring (`Present` and `ClearDepthStencilView`), and runs a
background file watcher thread (`ReadDirectoryChangesW`) to
enable seamless runtime hot-reloading of the segmentation logic.




3. **`SegmentationMap` (C++20 Dynamic Link Library)**
* Hot-swappable logic payload. Performs signature scanning
for Unreal Engine structures (`GUObjectArray`), latches onto
active depth-stencil views (`ClearDepthStencilView`), manages a
3-stage asynchronous GPU-to-CPU staging ring buffer
(`CopySubresourceRegion` with non-blocking `Map`), compresses
custom stencil masks via Run-Length Encoding (RLE), and
serializes synchronized RGB frames and masks into a custom
`.segm` container format.




4. **`ValidationSuite` (Python 3.11 Tooling)**
* Standalone verification suite that parses `.segm` binary streams,
decodes RLE mask payloads, blends colorized JET colormap overlays
directly onto captured gameplay frames, displays a live
interactive OpenCV preview, and exports verified validation
videos (`validation_output.mp4`).





---

## Quick Start Guide

### 1. Build the Solution

Open `HotReloadingD3DCapture.sln` in Visual Studio 2022/2026 and
build the entire solution in **Release | x64** configuration.

### 2. Launch the Game

Start *The Baby In Yellow* enforcing DirectX 11 mode:

```cmd
Game-Win64-Shipping.exe -d3d11

```

### 3. Inject the Core Hook

Run the injector executable from your build output directory:

```cmd
Injector.exe Game-Win64-Shipping.exe D3DHooksCore.dll

```

### 4. Load Payload & Record

1. Copy `SegmentationMap.dll` into the game binary directory next
to `Game-Win64-Shipping.exe`. The hot-reload bridge will
automatically shadow-copy and bind the payload.


2. Press **F8** in-game to toggle synchronized RGB backbuffer and
CustomStencil recording (`capture_output.segm`). Press **F8**
again to stop recording.



### 5. Validate Results

Run the validation script to generate visual proof and a
rendered MP4 overlay:

```bash
cd ValidationSuite
pip install -r requirements.txt
python validate_segmentation.py --segm capture_output.segm --out validation_output.mp4

```
