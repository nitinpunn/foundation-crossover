"""List every 'mov dword ptr [reg+disp], imm32' with the given immediate in a PE's .text, with context."""
import sys
import pefile
import capstone

path, imm = sys.argv[1], int(sys.argv[2], 0)
pe = pefile.PE(path, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
code, va0, raw0 = text.get_data(), base + text.VirtualAddress, text.PointerToRawData
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
needle = imm.to_bytes(4, "little")
i = code.find(needle)
while i >= 0:
    for back in range(2, 9):
        off = i - back
        ins = next(md.disasm(code[off:off + 15], va0 + off, count=1), None)
        if ins and ins.mnemonic == "mov" and ins.size == back + 4 and ins.op_str.startswith("dword ptr [") and ins.op_str.endswith(hex(imm)):
            print(f"VA {ins.address:#x} file {raw0 + off:#x}: {ins.bytes.hex()}  {ins.mnemonic} {ins.op_str}")
            break
    i = code.find(needle, i + 1)
