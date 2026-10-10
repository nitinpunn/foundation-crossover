"""Locate Mesa's d3d12_wgl_framebuffer_resize in libgallium_wgl.dll by its DXGI_SWAP_CHAIN_DESC1.Flags
constant (0x840) and dump the surrounding disassembly so the AlphaMode selection can be patched."""
import sys
import pefile
import capstone

path = sys.argv[1]
pe = pefile.PE(path, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
code = text.get_data()
va0 = base + text.VirtualAddress
raw0 = text.PointerToRawData

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
md.detail = False

# find "mov dword ptr [...], 0x840" occurrences by scanning for the imm32 bytes and disassembling around them
needle = (0x840).to_bytes(4, "little")
hits = []
start = 0
while True:
    i = code.find(needle, start)
    if i < 0:
        break
    for back in range(3, 12):
        off = i - back
        if off < 0:
            continue
        insns = list(md.disasm(code[off:off + 16], va0 + off, count=1))
        if insns and insns[0].mnemonic == "mov" and "0x840" in insns[0].op_str and insns[0].size == back + 4:
            hits.append(off)
            break
    start = i + 1

print(f"{len(hits)} candidate(s)")
for off in hits:
    lo = max(0, off - 0x140)
    print(f"\n=== hit at VA {va0 + off:#x} (file offset {raw0 + off:#x}) ===")
    for ins in md.disasm(code[lo:off + 0x200], va0 + lo):
        mark = "  <==" if ins.address == va0 + off else ""
        print(f"{ins.address:#x} [{raw0 + ins.address - va0:#08x}]: {ins.bytes.hex():<24} {ins.mnemonic} {ins.op_str}{mark}")
