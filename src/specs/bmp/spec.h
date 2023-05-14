#pragma once

// BMP image decoder/encoder
// References:
//  - https://en.wikipedia.org/wiki/BMP_file_format
//  - https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
//  - http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
//  - http://www.martinreddy.net/gfx/2d/BMP.txt

#include <karm-base/vec.h>
#include <karm-gfx/buffer.h>
#include <karm-text/emit.h>

#include "../bscan.h"

namespace Bmp {

struct Image {
    /* --- Loading ---------------------------------------------------------- */

    static bool isBmp(Bytes slice) {
        return slice.len() >= 2 and
               slice[0] == 0x42 and
               slice[1] == 0x4D;
    }

    static Res<Image> load(Bytes slice) {
        if (!isBmp(slice)) {
            return Error::invalidData("invalid signature");
        }

        Image image{};
        BScan s{slice};

        return Ok(image);
    }

    /* --- Header ----------------------------------------------------------- */

    usize _width;
    usize _height;
    usize _bpp;

    enum Compression {
        RGB = 0,
        RLE8 = 1,
        RLE4 = 2,
    } _compression;

    usize _numsColors;

    Res<> readHeader(BScan &s) {
        if (s.rem() < 54) {
            return Error::invalidData("image too small");
        }

        s.skip(2); // signature
        s.skip(4); // file size
        s.skip(4); // reserved
        s.skip(4); // data offset

        _width = s.nextI32le();
        _height = s.nextI32le();

        auto planes = s.nextI16le();
        if (planes != 1) {
            return Error::invalidData("invalid number of planes");
        }

        _bpp = s.nextI16le();

        auto comporession = s.nextI32le();
        if (comporession != RGB and comporession != RLE8 and comporession != RLE4) {
            return Error::invalidData("invalid compression");
        }

        s.skip(4); // image size
        s.skip(4); // x pixels per meter
        s.skip(4); // y pixels per meter
        _numsColors = s.nextI32le();
        s.skip(4); // important colors

        return Ok();
    }

    /* --- Palette ---------------------------------------------------------- */

    Vec<Gfx::Color> _palette;

    Res<> readPalette(BScan &s) {
        for (usize i = 0; i < _numsColors; ++i) {
            auto b = s.nextU8le();
            auto g = s.nextU8le();
            auto r = s.nextU8le();
            s.skip(1); // reserved

            _palette.pushBack(Gfx::Color{r, g, b});
        }

        return Ok();
    }

    /* --- Pixels ----------------------------------------------------------- */

    /* --- Encoding --------------------------------------------------------- */

    static Res<> encode(Gfx::Pixels pixels, BEmit &e) {
        e.writeU16le(0x424D); // signature
        e.writeU32le(0);      // file size
        e.writeU32le(0);      // reserved
        e.writeU32le(54);     // data offset

        e.writeU32le(pixels.width());
        e.writeU32le(pixels.height());
        e.writeU16le(1);  // planes
        e.writeU16le(24); // bpp
        e.writeU32le(0);  // compression
        e.writeU32le(0);  // image size
        e.writeU32le(0);  // x pixels per meter
        e.writeU32le(0);  // y pixels per meter
        e.writeU32le(0);  // nums colors
        e.writeU32le(0);  // important colors

        for (isize y = 0; y < pixels.height(); ++y) {
            for (isize x = 0; x < pixels.width(); ++x) {
                auto color = pixels.load({x, y});
                e.writeU8le(color.blue);
                e.writeU8le(color.green);
                e.writeU8le(color.red);
            }
        }

        return Ok();
    }

    /* --- Decoding --------------------------------------------------------- */

    Res<> decode(Gfx::MutPixels dest) {
    }

    /* --- Dumping ---------------------------------------------------------- */

    void dump(Text::Emit &e) {
        e("BMP image\n");
        e.indentNewline();
        e("width: {}\n", _width);
        e("height: {}\n", _height);
        e("bpp: {}\n", _bpp);
        e("compression: {}\n", _compression);
        e("numsColors: {}\n", _numsColors);

        e("palette:\n");
        e.indentNewline();
        for (auto &color : _palette) {
            e("{}\n", color);
        }
        e.deindent();

        e.deindent();
    }
};

} // namespace Bmp
