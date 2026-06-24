from PIL import Image
import itertools
import collections
import pathlib

def delta(seq):
    prev = 0
    for it in seq:
        yield it-prev
        prev = it

OPAQUE_PALETTE_TEXTURE             = 0b00
ONE_BIT_ALPHA_PALETTE_TEXTURE      = 0b01
TWO_BIT_ALPHA_PALETTE_TEXTURE      = 0b10

def lzss_compress(data: bytes, window_size: int = 256, min_match: int = 3, max_match: int = 256) -> bytes:
    if isinstance(data, str):
        data = data.encode()

    flags = bytearray()
    tokens = bytearray()
    pos = 0
    n = len(data)

    while pos < n:
        flag_byte = 0

        for bit in range(8):
            if pos >= n:
                break

            window_start = max(0, pos - window_size)
            best_len = 0
            best_offset = 0

            for i in range(window_start, pos):
                length = 0
                while (length < max_match and
                       pos + length < n and
                       data[i + length] == data[pos + length]):
                    length += 1
                if length > best_len:
                    best_len = length
                    best_offset = pos - i

            if best_len >= min_match:
                flag_byte |= (1 << bit)
                tokens.append((best_offset - 1) & 0xFF)
                tokens.append((best_len - 1) & 0xFF)
                pos += best_len
            else:
                tokens.append(data[pos])
                pos += 1

        flags.append(flag_byte)

    # 4-byte header: length of flag stream, so decompressor knows where data begins
    flag_len = len(flags).to_bytes(4, 'little')
    return bytes(flag_len + flags + tokens)

def palettize(cnt, pixels):
    palette = {}
    for pix in cnt:
        palette[pix] = len(palette)
    res = []
    for pix in pixels:
        res.append(palette[pix])

    return (palette, res)


def compress_alpha_texture(f, rgbas, count_alphas):
    num_pix = len(rgbas)
    combined_16bpp_colors = []
    res = []
    if len(count_alphas) == 2 and 0 in count_alphas and 255 in count_alphas:
        print("generating one bit alpha texture")
        res.append(ONE_BIT_ALPHA_PALETTE_TEXTURE)
        for (r,g,b,a) in rgbas:
            r>>=3
            g>>=3
            b>>=3
            a>>=7
            combined_16bpp_colors.append((r,g,b,a))
    else:
        res.append(TWO_BIT_ALPHA_PALETTE_TEXTURE)
        print("generating two bit alpha texture")
        # generate two bit
        for (r,g,b,a) in rgbas:
            r>>=3
            g>>=3
            b>>=4
            a>>=6
            combined_16bpp_colors.append((r,g,b,a))
        
    cnt = collections.Counter(combined_16bpp_colors)
    print("unique colors {}".format(len(cnt)))
    if len(cnt) <= 256 and len(cnt)*2 + num_pix < num_pix*2:
        print("generating palettized alpha texture")
        print("output bytes {}".format(len(cnt)*2+num_pix))        
        palette, idxs = palettize(cnt, combined_16bpp_colors)
        lzss_comp_idxs = lzss_compress(idxs)
        print("num index bytes uncompressed: ", len(idxs))
        print("num index bytes compressed: ", len(lzss_comp_idxs))
        if True: #len(lzss_comp_idxs) < len(idxs):
            print("OUTPUT COMPRESSED")

            return len(palette)*2+len(lzss_comp_idxs)

        else:
            print("OUTPUT UNCOMPRESSED")
            return len(cnt)*2+num_pix

        return len(cnt)*2+num_pix
    else:
        assert False
        #print("generating 16bpp alpha texture")
        #print("output bytes {}".format(num_pix*2))
        #return num_pix*2

def swizzle_rgbas(rgbas):
    rs = []
    gs = []
    bs = []
    alphs = []
    for (r,g,b,a) in rgbas:
        rs.append(r)
        gs.append(g)
        bs.append(b)
        alphs.append(a)
    return (rs,gs,bs,alphs)

def compress_opaque_texture(f, rgbas):
    num_pix = len(rgbas)
    print("generating opaque texture")
    # generate 565
    output = []
    output.append(OPAQUE_TEXTURE)
    combined_16b_colors = []
    for (r,g,b,_) in rgbas:
        r >>= 3
        g >>= 2
        b >>= 3
        combined_16b_colors.append((r,g,b))
    
    cnt = collections.Counter(combined_16b_colors)
    print("unique colors {}".format(len(cnt)))

    if len(cnt)*2 + num_pix < num_pix*2:
        print("generating palettized opaque texture")
        print("output bytes {}".format(len(cnt)*2+num_pix))
        palettized = palettize(cnt, combined_16b_colors)
        palette, idxs = palettized
        lzss_comp_idxs = lzss_compress(idxs)
        print("num index bytes uncompressed: ", len(idxs))
        print("num index bytes compressed: ", len(lzss_comp_idxs))
        if True: #len(lzss_comp_idxs) < len(idxs):
            print("OUTPUT COMPRESSED")
            return len(palette)*2+len(lzss_comp_idxs)
        else:
            print("OUTPUT UNCOMPRESSED")
            return len(cnt)*2+num_pix
    else:
        assert False
        print("generating 16bpp opaque texture")
        print("output bytes {}".format(num_pix*2))
        print("num rgba bytes uncompressed: ", len(combined_16b_colors*2))
        (rs,gs,bs,alphs) = swizzle_rgbas(rgbas)
        crs = lzss_compress(rs)
        cgs = lzss_compress(gs)
        cbs = lzss_compress(bs)
        cas = lzss_compress(alphs)

        print("num rgba bytes compressed: ", len(crs) + len(cgs) + len(cbs) + len(cas))
        return num_pix*2


def compress(f):
    img = Image.open(f)

    # Get raw bytes
    raw_bytes = img.tobytes()

    #print(f"Type: {type(raw_bytes)}")
    #print(f"Length of raw bytes: {len(raw_bytes)}")
    groups = list(itertools.batched(raw_bytes, 4))
    #print(groups)
    rs = []
    gs = []
    bs = []
    rgba_5bpp = []
    rs5bpp = []
    gs5bpp = []
    bs5bpp = []
    alphs = []
    for (r,g,b,a) in groups:
        rs.append(r)
        rs5bpp.append(r>>3)
        gs.append(g)
        gs5bpp.append(g>>3)
        bs.append(b)
        bs5bpp.append(b>>3)
        alphs.append(a)
        rgba_5bpp.append((r>>3,g>>4,b>>3,a>>6))


    prev_r = 0
    delta_rs = list(delta(rs))
    delta_gs = list(delta(gs))
    delta_bs = list(delta(bs))
    delta_alphs = list(delta(alphs))
    cnt_rgbs = collections.Counter(groups)
    cnt_rs = collections.Counter(rs)
    cnt_gs = collections.Counter(gs)
    cnt_bs = collections.Counter(bs)
    cnt_as = collections.Counter(alphs)
    cnt_rs5bpp = collections.Counter(rs5bpp)
    cnt_gs5bpp = collections.Counter(gs5bpp)
    cnt_bs5bpp = collections.Counter(bs5bpp)
    cnt_rgbas5bpp = collections.Counter(rgba_5bpp)


    print(f)
    #print("unique colors: {}".format(len(cnt_rgbs)))
    #print("unique quantized colors: {}".format(len(cnt_rgbas5bpp)))
    #print("unique alphas: {}".format(len(cnt_as)))

    if len(cnt_as) == 1:
        return compress_opaque_texture(f, groups)
    else:
        return compress_alpha_texture(f, groups, cnt_as)
    #print("whoa")

import os

if __name__ == '__main__':
    print(os.getcwd())
    total_bytes = 0
    for f in pathlib.Path("./out/resources/").iterdir():
        if ".tga" in f.name:
            total_bytes += compress(f)#"./out/resources/flat_tex0.tga")
    print(total_bytes)
