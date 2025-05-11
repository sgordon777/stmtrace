import serial
import struct
import time
import sys
from datetime import datetime
import re

# Configuration
PORT = 'COM3'
BAUD = 921600
PAGE_SIZE = 256
FLASH_SIZE = 8 * 1024 * 1024
HEADER_SIZE = 160

# Magic IDs
TRACE_ID_1 = 0x4a455355  # 'JESU'
TRACE_ID_2 = 0x53444945  # 'SDIE'
TRACE_ID_3 = 0x44344f55  # 'D4OU'
TRACE_ID_4 = 0x5253494e  # 'RSIN'

HEADER_FMT = "<IIIIIIHHI128s"  # Matches trace_header_t in C

ser = serial.Serial(PORT, BAUD, timeout=0.5)

def flush_input():
    ser.reset_input_buffer()

def send_cmd(cmd):
    flush_input()
    ser.write((cmd + '').encode())
    time.sleep(0.01)

def read_data(length):
    buf = bytearray()
    while len(buf) < length:
        chunk = ser.read(length - len(buf))
        if not chunk:
            break
        buf.extend(chunk)
    return buf

def check_flash_connection():
    send_cmd("idb,\r")
    id_bytes = ser.read(3)
    if len(id_bytes) != 3:
        print(f"Flash chip not responding (no JEDEC ID), bytes read: { len(id_bytes) } \n")
        return False
    print(f"Flash JEDEC ID: {id_bytes.hex().upper()}")
    return True

def read_flash(addr, length):
    send_cmd(f"rdb, 0x{addr:X}, {length}\r")
    return read_data(length)

def parse_header(data):
    fields = struct.unpack(HEADER_FMT, data)
    return {
        'id1': fields[0],
        'id2': fields[1],
        'id3': fields[2],
        'id4': fields[3],
        'len_pages': fields[4],
        'len_bytes': fields[5],
        'checksum': fields[6],
        'header_len_b': fields[7],
        'ver': fields[8],
        'filename': fields[9].split(b'\x00')[0].decode(errors='ignore')
    }

def is_valid_header(h):
    return (h['id1'] == TRACE_ID_1 and
            h['id2'] == TRACE_ID_2 and
            h['id3'] == TRACE_ID_3 and
            h['id4'] == TRACE_ID_4 and
            h['len_bytes'] > 0 and
            h['len_pages'] > 0)

def dump_trace_file(start_addr, length, filename):
    #print(f"Dumping {length} bytes from 0x{start_addr:06X} → {filename}")
    with open(filename, "wb") as f:
        remaining = length
        addr = start_addr
        while remaining > 0:
            chunk = min(4096, remaining)
            data = read_flash(addr, chunk)
            if not data:
                print("Read failed or timed out.")
                return
            f.write(data)
            addr += chunk
            remaining -= chunk


def scan_and_dump():
    addr = 0
    header_valid = True
    while addr + HEADER_SIZE <= FLASH_SIZE and header_valid:
        data = read_flash(addr, HEADER_SIZE)
        if len(data) != HEADER_SIZE:
            print(f"Incomplete header at 0x{addr:X}, len={len(data)}")
            print(data)
            addr += PAGE_SIZE
            return
        header = parse_header(data)
        if is_valid_header(header):
            # Sanitize the filename
            if not header['filename'] or header['filename'].strip() == "":
                header['filename'] = f"trace_{datetime.now().strftime('%Y%m%d_%H%M%S')}.bin"
            else:
                # Remove invalid characters from the filename
                header['filename'] = re.sub(r'[<>:"/\\|?*\x00-\x1F]', '_', header['filename'])
            
            print(f"{header['filename']}, {header['len_bytes']} bytes @ 0x{addr:06X}")
            payload_start = addr + PAGE_SIZE
            dump_trace_file(payload_start, header['len_bytes'], header['filename'])            
            addr += (header['len_pages'] * PAGE_SIZE + PAGE_SIZE)
        else:
            header_valid = False

    print("Done!")

if __name__ == "__main__":
    if not check_flash_connection():
        sys.exit(1)
    scan_and_dump()