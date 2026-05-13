#!/usr/bin/env python3
import os, re, glob

DAPLINK_FAMILY = "/home/zed/code/rtos_app/DAPLink/source/family"
OUTPUT_DIR = "/home/zed/code/rtos_app/daplink-rtos/src/flash_algo"
VENDOR_MAP = {"st":"stm32","freescale":"freescale","nxp":"nxp","maxim":"maxim","nuvoton":"nuvoton","nordic":"nordic","renesas":"renesas","arm":"arm","realtek":"realtek","ambiq":"ambiq"}
os.makedirs(OUTPUT_DIR, exist_ok=True)

def safename(s):
    return re.sub(r'_+', '_', re.sub(r'[^a-z0-9]', '_', s.lower())).strip('_')

def convert(fp):
    with open(fp) as f: text = f.read()
    name = safename(os.path.basename(os.path.dirname(fp)))
    m = re.search(r'static\s+const\s+uint32_t\s+\w+\[\]\s*=\s*\{([^}]+)\}', text, re.DOTALL)
    if not m: return None, None
    nums = re.findall(r'0x[0-9a-fA-F]+', re.sub(r'/\*.*?\*/', '', m.group(1)))
    m3 = re.search(r'program_target_t\s+\w+\s*=\s*\{([^}]+)\}', text, re.DOTALL)
    if not m3: return None, None
    pt = m3.group(1)
    def gv(k):
        p = re.search(rf'{k}\s*=\s*(0x[0-9a-fA-F]+)', pt)
        return p.group(1) if p else "0"
    blob = f'static const uint32_t {name}_algo[] = {{\n'
    for i in range(0, len(nums), 8):
        blob += '  ' + ', '.join(nums[i:i+8]) + ',\n'
    blob += '};\n\n'
    blob += f'const program_target_t {name}_flash = {{\n'
    blob += f'  .init = {gv("init")}, .uninit = {gv("uninit")},\n'
    blob += f'  .erase_chip = {gv("erase_chip")}, .erase_sector = {gv("erase_sector")},\n'
    blob += f'  .program_page = {gv("program_page")}, .verify = {gv("verify")},\n'
    blob += f'  .sys_s = {{.breakpoint = {gv("breakpoint")}, .static_base = {gv("static_base")}, .stack_pointer = {gv("stack_pointer")}}},\n'
    blob += f'  .program_buffer = {gv("program_buffer")}, .algo_start = {gv("algo_start")},\n'
    blob += f'  .algo_size = {len(nums)*4}, .algo_blob = {name}_algo, .flash_algo_count = 1,\n'
    blob += '};\n'
    return name, blob

by_vendor = {}
for f in sorted(glob.glob(f"{DAPLINK_FAMILY}/*/*/flash_blob.c")):
    v = f.replace(DAPLINK_FAMILY, '').strip('/').split('/')[0]
    by_vendor.setdefault(v, []).append(f)

for vendor, flist in sorted(by_vendor.items()):
    vname = VENDOR_MAP.get(vendor, vendor)
    outfile = os.path.join(OUTPUT_DIR, f"vendor_{vname}.c")
    parts = ['#include "flash_blob.h"\n']
    targets = []
    for f in flist:
        name, blob = convert(f)
        if blob: parts.append(blob); targets.append(name)
    with open(outfile, 'w') as fout: fout.write('\n'.join(parts))
    print(f"{outfile}: {len(targets)} targets ({', '.join(targets[:3])}...)")
