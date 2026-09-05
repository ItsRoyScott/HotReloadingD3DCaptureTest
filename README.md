# Real-Time Gameplay Segmentation Extractor

A practical prototype for pulling pixel-aligned object segmentation masks out of shipped DirectX 11 Unreal Engine games (*The Baby In Yellow*, UE 4.27.2). The tool uses MinHook to intercept D3D11 render targets, transfers depth-stencil buffers asynchronously without dropping frame rates, and hooks Unreal reflection to resolve runtime actor identities.

---

## Architecture

The system splits into a C++ DLL injected into the target process and an offline Python pipeline for extraction, cleaning, and overlay rendering:

```text
+-------------------------------------------------------------------+
|                        Injected C++ DLL                           |
|                                                                   |
|  [MinHook] ----> [D3D11 SwapChain Hook] ---> [Async Ring Buffer]  |
|                         |                         |               |
|                 [GUObjectArray /]         [RLE Stencil &]         |
|                 [ ProcessEvent ]          [ JPEG Frame  ]         |
+-------------------------|-------------------------|---------------+
                          |                         |
                          v                         v
               [SEGM Binary Container (.segm)]
                                |
                                v
         [ValidationSuite/validate_segmentation.py Pipeline]
                                |
                                v
             [Annotated H.264 MP4 Validation Video]
```

### Core Pipeline

* **DXGI & D3D11 Hooks**: Intercepts `IDXGISwapChain::Present` and `ID3D11DeviceContext::OMSetRenderTargets` via MinHook to capture the backbuffer RGB texture and active stencil buffer.
* **Non-Blocking GPU Readback**: Avoids render-thread locks by streaming texture data through a 3-stage staging buffer pool (`ID3D11Texture2D`, `D3D11_USAGE_STAGING`) backed by GPU fences.
* **Unreal Engine Reflection**: Traverses `GUObjectArray` at startup to map `FName` identifiers and hooks `UFunction::ProcessEvent` to safely interrogate properties on the game thread.
* **Custom Serialization**: Stores sync headers, timestamps, JPEG color frames, and RLE-compressed 8-bit stencil masks in a lightweight `.segm` binary file.
* **Validation & Re-encoding**: `ValidationSuite/validate_segmentation.py` parses the binary stream, applies morphological cleanup, draws high-contrast contour perimeters with class badges, and converts output files to web-compatible H.264 (`yuv420p`) using FFmpeg.

---

## File Structure

```text
├── src/
│   ├── dllmain.cpp              # DLL entrypoint & injection lifecycle
│   ├── d3d11_hook.cpp           # MinHook detours for Present & OMSetRenderTargets
│   ├── ring_buffer.cpp          # 3-frame asynchronous staging buffer queue
│   ├── engine_reflection.cpp    # GUObjectArray walker & ProcessEvent hook
│   └── segm_writer.cpp          # Binary container serializer (.segm)
├── ValidationSuite/
│   └── validate_segmentation.py # RLE decoder, OpenCV mask renderer, & FFmpeg pass
├── README.md                    # Project overview & build instructions
└── DEBUG_JOURNAL.md             # Technical post-mortem, debugging log, & AI usage
```

---

## Setup & Running

### Dependencies
* **C++**: Visual Studio 2022/2026 (MSVC x64), CMake 3.20+, DirectX SDK.
* **Python**: Python 3.9+, OpenCV (`opencv-python`), NumPy (`numpy`).
* **Tooling**: System FFmpeg installation (required for web H.264 re-encoding pass).

### 1. Build Injected DLL
```bash
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

### 2. Capture Gameplay Data
Inject `GameplaySegmentation.dll` into the game process using standard injection tools (e.g., Xenos). Toggle recording in-game with **F8**. Output streams directly to `capture_output.segm`.

### 3. Generate Overlays & Validate
Process the captured binary stream to generate annotated video:

```bash
# Render with interactive preview window
python ValidationSuite/validate_segmentation.py --segm capture_output.segm --out validation_output.mp4

# Run headless batch pass
python ValidationSuite/validate_segmentation.py --segm capture_output.segm --out validation_output.mp4 --no-preview
```

---

## Practical AI Usage & Overrides

AI tools accelerated boilerplate work but required manual overrides when handling real-time engine constraints:

* **Where AI Worked Well**: Writing initial DXGI vtable offsets, generating C++ and Python RLE compression routines, and scaffolding the FFmpeg `subprocess` pipeline.
* **Where AI Failed & Needed Manual Engineering**:
  * **GPU Sync Lockups**: AI recommended immediate `ID3D11DeviceContext::Map` calls right after `CopyResource`, which tanked frame rates by stalling the render thread. Fixed by building a fence-guarded 3-frame staging queue.
  * **Engine State Changes**: AI assumed CustomStencil values were static. Diagnostic logs revealed UE turns off `bRenderCustomDepth` during object pick-ups, requiring a `ProcessEvent` hook to force stencil flags back on during actor attachments.