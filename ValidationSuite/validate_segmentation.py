import cv2
import numpy as np
import struct
import argparse
import sys
import logging

file_handler = logging.FileHandler("validation_debug.log", mode='w')
file_handler.setLevel(logging.DEBUG)
console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.INFO)

logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[file_handler, console_handler]
)

class SegmParser:
    def __init__(self, filepath):
        self.filepath = filepath
        self.file = open(filepath, "rb")
        self.parse_header()

    def parse_header(self):
        self.file.seek(0)
        header_data = self.file.read(0x40)
        magic, version, self.width, self.height, self.fps, self.comp_type = struct.unpack("<4sIHHHB", header_data[:15])
        
        if magic != b'SEGM':
            raise ValueError("Invalid magic bytes. Expected SEGM.")
        
        self.total_frames, self.dict_offset, self.payload_offset = struct.unpack("<QQQ", header_data[0x20:0x38])
        logging.info(f"[+] Loaded Archive: {self.width}x{self.height} @ {self.fps} FPS | Total Frames: {self.total_frames}")

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
        
            frame_idx, timestamp, color_comp_size, comp_size, uncomp_size = struct.unpack("<QQIII", meta_bytes)
        
            color_bytes = self.file.read(color_comp_size)
            stencil_bytes = self.file.read(comp_size)

            # Decode compressed JPEG color frame
            color_frame = cv2.imdecode(np.frombuffer(color_bytes, dtype=np.uint8), cv2.IMREAD_COLOR)
            stencil_mask = self.decode_rle(stencil_bytes, uncomp_size)

            return (frame_idx, timestamp), color_frame, stencil_mask

    def close(self):
        self.file.close()

def validate_standalone_segm(segm_path, output_video, show_preview=True):
    parser = SegmParser(segm_path)
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(output_video, fourcc, parser.fps, (parser.width, parser.height))
    
    parser.file.seek(parser.payload_offset)
    processed_count = 0
    gamma_lut = np.array([((i / 255.0) ** (1.0 / 2.2)) * 255 for i in range(256)]).astype("uint8")

    logging.info("[+] Starting frame validation and overlay rendering loop...")

    while True:
        meta, color_frame, mask = parser.read_frame()
        if mask is None:
            break

        corrected_frame = cv2.LUT(color_frame, gamma_lut)
        active_pixels = np.count_nonzero(mask)

        logging.debug(f"Processing Frame {meta[0]} | Timestamp: {meta[1]}us | Active Stencil Pixels: {active_pixels}")

        if active_pixels > 0:
            # Upscale half-resolution mask smoothly using bilinear interpolation to remove vertical stripe aliasing
            if mask.shape[0] != parser.height or mask.shape[1] != parser.width:
                mask = cv2.resize(mask, (parser.width, parser.height), interpolation=cv2.INTER_LINEAR)

            color_mask = cv2.applyColorMap((mask * 19) % 255, cv2.COLORMAP_JET)
            color_mask[mask == 0] = [0, 0, 0]
            blended = cv2.addWeighted(corrected_frame, 0.7, color_mask, 0.3, 0)
        else:
            blended = corrected_frame

        cv2.putText(blended, f"Frame: {meta[0]} | TS: {meta[1]}us | Active Pixels: {active_pixels}", 
                    (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        out.write(blended)
        processed_count += 1

        if show_preview:
            cv2.imshow("Segmentation Map Standalone Validation", cv2.resize(blended, (1280, 720)))
            if cv2.waitKey(1) & 0xFF == ord('q'):
                show_preview = False
                cv2.destroyAllWindows()

    if show_preview:
        cv2.destroyAllWindows()

    out.release()
    parser.close()
    logging.info(f"[+] Validation complete. Successfully processed {processed_count} frames -> Output: {output_video}")

if __name__ == "__main__":
    cli = argparse.ArgumentParser(description="SEGM Standalone Validation Utility")
    cli.add_argument("--segm", default="capture_output.segm", help="Path to input .segm binary file")
    cli.add_argument("--out", default="validation_output.mp4", help="Path to output validation video")
    cli.add_argument("--no-preview", action="store_true", help="Disable live preview window")
    args = cli.parse_args()

    validate_standalone_segm(args.segm, args.out, show_preview=not args.no_preview)