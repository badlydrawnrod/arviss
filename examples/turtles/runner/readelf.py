# Read some info about an ELF file. Very much hacked together.

import struct

if __name__ == "__main__":
    filename = "examples/turtles/arviss/build/turtle"
    with open(filename, "rb") as f:
        # --- File Header ----------------------------------------------------------------------------------------------
        header_raw = f.read(64)

        # --- e_ident
        e_ident = header_raw[0:9]
        print("ELF Header")

        ei_magic = e_ident[0:4]
        ei_class = e_ident[4]
        ei_data = e_ident[5]
        ei_version = e_ident[6]
        ei_osabi = e_ident[7]
        ei_osabiversion = e_ident[8]

        print(f"Magic:                              {ei_magic}")
        print(f"Class:                              {ei_class}")
        print(f"Data:                               {ei_data}")
        print(f"Version:                            {ei_version}")
        print(f"OS/ABI:                             {ei_osabi}")
        print(f"ABI Version:                        {ei_osabiversion}")

        # At this point I'm going to assume a 32-bit little-endian RISC-V executable, because I'd have rejected
        # everything else.

        # --- e_type
        e_type = struct.unpack("<H", header_raw[0x10:0x12])[0]
        print(f"Type:                               {e_type}")

        # --- e_machine
        e_machine = struct.unpack("<H", header_raw[0x12:0x14])[0]
        print(f"Machine:                            0x{e_machine:x}")

        # --- e_version
        e_version = struct.unpack("<I", header_raw[0x14:0x18])[0]
        print(f"Version:                            {e_version}")

        # --- e_entry
        e_entry = struct.unpack("<I", header_raw[0x18:0x1c])[0]
        print(f"Entry point:                        0x{e_entry:08x}")

        # --- e_phoff
        e_phoff = struct.unpack("<I", header_raw[0x1c:0x20])[0]
        print(f"Start of program headers:           {e_phoff} bytes into file")

        # --- e_shoff
        e_shoff = struct.unpack("<I", header_raw[0x20:0x24])[0]
        print(f"Start of section headers:           {e_shoff} bytes into file")

        # --- e_flags
        e_flags = struct.unpack("<I", header_raw[0x24:0x28])[0]
        print(f"Flags:                              0x{e_flags}")

        # --- e_ehsize
        e_ehsize = struct.unpack("<H", header_raw[0x28:0x2a])[0]
        print(f"Size of this header:                {e_ehsize} bytes")

        # --- e_phentsize
        e_phentsize = struct.unpack("<H", header_raw[0x2a:0x2c])[0]
        print(f"Size of program headers:            {e_phentsize} bytes")

        # --- e_phnum
        e_phnum = struct.unpack("<H", header_raw[0x2c:0x2e])[0]
        print(f"Number of program headers:          {e_phnum}")

        # --- e_shentsize
        e_shentsize = struct.unpack("<H", header_raw[0x2e:0x30])[0]
        print(f"Size of section headers:            {e_shentsize} bytes")

        # --- e_shnum
        e_shnum = struct.unpack("<H", header_raw[0x30:0x32])[0]
        print(f"Number of section headers:          {e_shnum}")

        # --- e_shstrndx
        e_shstrnx = struct.unpack("<H", header_raw[0x32:0x34])[0]
        print(f"Section header string table index:  {e_shstrnx}")

        # --- Program headers ------------------------------------------------------------------------------------------
        f.seek(e_phoff)

        print("\nProgram Headers:")
        print(f"Type       Offset     VirtAddr   PhysAddr   FileSiz  MemSiz   Flg    Align")
        for i in range(e_phnum):
            ph = f.read(e_phentsize)
            p_type = struct.unpack("<I", ph[0x00:0x04])[0]
            p_offset = struct.unpack("<I", ph[0x04:0x08])[0]
            p_vaddr = struct.unpack("<I", ph[0x08:0x0c])[0]
            p_paddr = struct.unpack("<I", ph[0x0c:0x10])[0]
            p_filesz = struct.unpack("<I", ph[0x10:0x14])[0]
            p_memsz = struct.unpack("<I", ph[0x14:0x18])[0]
            p_flags = struct.unpack("<I", ph[0x18:0x1c])[0]
            p_align = struct.unpack("<I", ph[0x1c:0x20])[0]
            print(
                f"0x{p_type:08x} 0x{p_offset:08x} 0x{p_vaddr:08x} 0x{p_paddr:08x} 0x{p_filesz:06x} 0x{p_memsz:06x} 0x{p_flags:04x} 0x{p_align:x}")
