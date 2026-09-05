# Engineering Debug Journal

## Overview
Technical log detailing bug fixes, engine quirks, and performance bottlenecks encountered while building the DirectX 11 gameplay segmentation system.

---

## Issue 1: Game Frame Rate Tanking During Capture

### Symptom
Enabling capture caused game performance to drop from 60 FPS down to ~10-12 FPS with severe visual hitching.

### Root Cause
Directly calling `ID3D11DeviceContext::Map` on a staging texture right after `CopyResource` forces the CPU thread to block until the GPU finishes executing all pending command buffers. Doing this every frame locks Unreal Engine's main rendering thread.

### Resolution
Replaced immediate map calls with an asynchronous 3-frame staging buffer ring:
1. Created a ring buffer containing 3 staging textures (`D3D11_USAGE_STAGING` with `D3D11_CPU_ACCESS_READ`).
2. On frame N, queue `CopyResource` into ring slot `N % 3`.
3. Read back data from ring slot `(N - 2) % 3`. This gives the GPU two full frame cycles to finish the DMA transfer without holding up the render thread.

---

## Issue 2: Stencil Mask Vanishing on Picked-up Objects

### Symptom
The baby actor (Stencil ID 62) registered clean mask coverage when standing still (e.g., Frame 89: 9,643 active pixels). However, picking up the baby caused the pixel count to drop to zero (e.g., Frame 149), despite the object occupying most of the viewport.

| Frame # | Active Pixels | Detected IDs | Context |
|---|---|---|---|
| Frame 29 | 0 | [] | Pre-pickup / distant target |
| Frame 89 | 9,643 | [np.uint8(62)] | Close-up stationary actor |
| Frame 149 | 0 | [] | Baby picked up; CustomStencil disabled |
| Frame 209 | 321 | [np.uint8(62)] | Partial mesh recovery |
| Frame 329 | 1,422 | [np.uint8(62)] | Re-acquired tracking |
| Frame 389 | 1,478 | [np.uint8(62)] | Stable tracking |
| Frame 449 | 0 | [] | Fully occluded / state toggle |

### Root Cause
Unreal Engine dynamically mutates primitive render flags when actors attach to player sockets. During pickup events, the engine temporarily disables `bRenderCustomDepth` on attached skeletal components, causing them to bypass the stencil pass entirely.

### Resolution
1. **Engine Reflection Override**: Intercepted `AActor::Tick` via `ProcessEvent` to force `bRenderCustomDepth = true` and `CustomDepthStencilValue = 62` across all `UPrimitiveComponent` instances attached to the actor.
2. **Morphological Closing**: Added `cv2.MORPH_CLOSE` operations to `ValidationSuite/validate_segmentation.py` to fill single-frame holes caused by transient state swaps.

---

## Issue 3: Low-Contrast Mask Overlays & Web Video Playback Errors

### Symptom
Applying raw OpenCV colormaps (`COLORMAP_JET`) resulted in washed-out highlights that lacked clear perimeters. Additionally, raw output MP4 files failed to play back in web browsers.

### Root Cause
Flat colormaps don't provide explicit class contrast or structural vector edges. Meanwhile, OpenCV's default `VideoWriter` encoders output non-standard FourCC headers or `yuv444p` pixel layouts that HTML5 video players refuse to stream.

### Resolution
1. **Explicit Vector Overlays**: Updated `ValidationSuite/validate_segmentation.py` to use explicit BGR color palettes per class ID, trace sharp contour boundaries (`cv2.drawContours`), and render anchored text badges at the object's center of mass.
2. **Automated H.264 Re-encoding**: Added a post-process step that runs an automated FFmpeg conversion pass to guarantee web-standard `yuv420p` encoding:
   `ffmpeg -i input_temp.mp4 -c:v libx264 -pix_fmt yuv420p -preset fast -crf 18 output.mp4`

---

## Core Findings

* **MinHook Detours**: Intercepting `Present` and `OMSetRenderTargets` provides reliable access to DirectX 11 swapchains without disrupting game execution.
* **Engine Introspection**: Walking `GUObjectArray` at runtime bypasses the need for hardcoded memory offsets across different engine builds.
* **RLE Compression**: Stencil masks compress efficiently with Run-Length Encoding, keeping serialization overhead light enough to record high-resolution frames without disk bottlenecks.