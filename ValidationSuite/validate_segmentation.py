import argparse
import os
import shutil
import struct
import subprocess
import sys
import cv2
import numpy as np


class SegmParser:

    def __init__(self, filepath):
        self.filepath = filepath
        self.file = open(filepath, "rb")
        self.parse_header()

    def parse_header(self):
        self.file.seek(0)
        header_data = self.file.read(0x40)
        magic, version, self.width, self.height, self.fps, self.comp_type = (
            struct.unpack("<4sIHHHB", header_data[:15])
        )

        if magic != b"SEGM":
            raise ValueError("Invalid magic bytes. Expected SEGM.")

        self.total_frames, self.dict_offset, self.payload_offset = (
            struct.unpack("<QQQ", header_data[0x20:0x38])
        )

    def decode_rle(self, compressed_bytes, expected_size):
        decompressed = bytearray()
        idx = 0
        comp_len = len(compressed_bytes)
        while idx < comp_len:
            run_len = compressed_bytes[idx]
            run_val = compressed_bytes[idx + 1]
            decompressed.extend([run_val] * run_len)
            idx += 2

        arr = np.frombuffer(decompressed, dtype=np.uint8)
        return arr.reshape((self.height, self.width))

    def read_frame(self):
        meta_bytes = self.file.read(28)
        if not meta_bytes or len(meta_bytes) < 28:
            return None, None, None

        frame_idx, timestamp, color_comp_size, comp_size, uncomp_size = (
            struct.unpack("<QQIII", meta_bytes)
        )

        color_bytes = self.file.read(color_comp_size)
        stencil_bytes = self.file.read(comp_size)

        color_frame = cv2.imdecode(
            np.frombuffer(color_bytes, dtype=np.uint8), cv2.IMREAD_COLOR
        )
        stencil_mask = self.decode_rle(stencil_bytes, uncomp_size)

        return (frame_idx, timestamp), color_frame, stencil_mask

    def close(self):
        self.file.close()


def convert_to_web_h264(input_video_path):
    """Converts the video to standard web-compatible H.264 (yuv420p) if ffmpeg is available."""
    ffmpeg_bin = shutil.which("ffmpeg")
    if not ffmpeg_bin:
        print(
            "[!] FFmpeg not found on system PATH. Skipping web H.264 conversion pass."
        )
        return

    temp_path = input_video_path.replace(".mp4", "_temp.mp4")
    shutil.move(input_video_path, temp_path)

    cmd = [
        ffmpeg_bin,
        "-y",
        "-i",
        temp_path,
        "-c:v",
        "libx264",
        "-pix_fmt",
        "yuv420p",
        "-preset",
        "fast",
        "-crf",
        "18",
        input_video_path,
    ]

    try:
        subprocess.run(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True
        )
        os.remove(temp_path)
        print(
            f"[+] Successfully re-encoded {input_video_path} to web-playable H.264 (yuv420p)."
        )
    except Exception as e:
        print(f"[!] FFmpeg conversion failed: {e}")
        if not os.path.exists(input_video_path) and os.path.exists(temp_path):
            shutil.move(temp_path, input_video_path)


def render_overlay(
    color_frame: np.ndarray, stencil_mask: np.ndarray
) -> np.ndarray:
    """Blends semi-transparent class colors, draws outlines, and places centroid labels."""
    CLASS_MAP = {
        62: {"label": "Baby", "color": (0, 255, 255)},  # Yellow / Cyan
        4: {"label": "Environment", "color": (255, 0, 0)},  # Blue 
        24: {"label": "Prop", "color": (0, 255, 0)},  # Green 
        56: {"label": "Interactive", "color": (255, 0, 255)},  # Magenta 
        102: {"label": "Special Actor", "color": (0, 165, 255)},  # Orange 
        170: {"label": "Actor", "color": (128, 0, 128)},  # Purple 
    }

    # Match dimensions if stencil resolution differs from color backbuffer
    if stencil_mask.shape[:2] != color_frame.shape[:2]:
        stencil_mask = cv2.resize(
            stencil_mask,
            (color_frame.shape[1], color_frame.shape[0]),
            interpolation=cv2.INTER_NEAREST,
        )

    output = color_frame.copy()
    color_overlay = color_frame.copy()

    unique_ids = np.unique(stencil_mask[stencil_mask > 0])

    for stencil_id in unique_ids:
        bin_mask = (stencil_mask == stencil_id).astype(np.uint8)
        if np.sum(bin_mask) == 0:
            continue

        info = CLASS_MAP.get(
            int(stencil_id),
            {"label": f"ID_{stencil_id}", "color": (0, 255, 0)},
        )
        fill_color = info["color"]
        label = info["label"]

        # 1. Fill mask pixels with mapped class color
        color_overlay[bin_mask == 1] = fill_color

        # 2. Draw high-contrast contour outlines around stencil geometry 
        contours, _ = cv2.findContours(
            bin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )
        cv2.drawContours(output, contours, -1, (0, 255, 255), 3)

        # 3. Compute centroid and draw object badge 
        for cnt in contours:
            if cv2.contourArea(cnt) < 100:
                continue
            M = cv2.moments(cnt)
            if M["m00"] != 0:
                cX = int(M["m10"] / M["m00"])
                cY = int(M["m01"] / M["m00"])

                text_str = f"{label} ({stencil_id})"
                (tw, th), _ = cv2.getTextSize(
                    text_str, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2
                )
                cv2.rectangle(
                    output,
                    (cX - 5, cY - th - 5),
                    (cX + tw + 5, cY + 5),
                    (0, 0, 0),
                    -1,
                )
                cv2.putText(
                    output,
                    text_str,
                    (cX, cY),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.6,
                    (255, 255, 255),
                    2,
                    cv2.LINE_AA,
                )

    # Blend transparent overlay over original frame 
    alpha = 0.45
    return cv2.addWeighted(color_overlay, alpha, output, 1 - alpha, 0)


def validate_standalone_segm(segm_path, output_video, show_preview=True):
    parser = SegmParser(segm_path)

    fourcc = cv2.VideoWriter_fourcc(*"avc1")
    out = cv2.VideoWriter(
        output_video, fourcc, parser.fps, (parser.width, parser.height)
    )
    if not out.isOpened():
        fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        out = cv2.VideoWriter(
            output_video, fourcc, parser.fps, (parser.width, parser.height)
        )

    parser.file.seek(parser.payload_offset)
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 3))

    while True:
        meta, color_frame, mask = parser.read_frame()
        if mask is None:
            break

        # Filter noise and perform morphological closing 
        binary_mask = (mask > 0).astype(np.uint8) * 255
        cleaned_binary = cv2.morphologyEx(binary_mask, cv2.MORPH_CLOSE, kernel)
        cleaned_mask = np.where(cleaned_binary > 0, mask, 0).astype(np.uint8)

        active_pixels = np.count_nonzero(cleaned_mask)
        unique_ids = np.unique(cleaned_mask[cleaned_mask > 0])

        # Apply rich overlay rendering 
        if active_pixels > 0:
            blended = render_overlay(color_frame, cleaned_mask)
        else:
            blended = color_frame.copy()

        hud_text = f"Frame: {meta[0]} | Active Pixels: {active_pixels} | Detected IDs: {list(unique_ids)}"
        cv2.putText(
            blended,
            hud_text,
            (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (0, 255, 0),
            2,
            cv2.LINE_AA,
        )

        out.write(blended)

        if show_preview:
            cv2.imshow(
                "Segmentation Map Standalone Validation",
                cv2.resize(blended, (1280, 720)),
            )
            if cv2.waitKey(1) & 0xFF == ord("q"):
                show_preview = False
                cv2.destroyAllWindows()

    if show_preview:
        cv2.destroyAllWindows()

    out.release()
    parser.close()
    print(f"[+] Video writing finished: {output_video}")

    convert_to_web_h264(output_video)


if __name__ == "__main__":
    cli = argparse.ArgumentParser()
    cli.add_argument("--segm", default="capture_output.segm")
    cli.add_argument("--out", default="validation_output.mp4")
    cli.add_argument("--no-preview", action="store_true")
    args = cli.parse_args()

    validate_standalone_segm(
        args.segm, args.out, show_preview=not args.no_preview
    )