#pragma once

#include <string>
#include <unordered_map>

#include "base_types.h"

struct Texture;

struct GlyphMetrics {
    f32 bt;
    f32 bl;
    f32 bw;
    f32 bh;
    f32 advance;
    f32 lsb;
    f32 y_off;
};

struct Font {
    String file_name;
    std::unordered_map<u32, GlyphMetrics> metrics_table;
    Texture *texture;
    GlyphMetrics *default_glyph;

    f32 width;
    f32 height;
    f32 scale;

    int ascent;
    int descent;
    int line_gap;
    int line_skip;
};


Font *font_create(String file_name, int line_height);
GlyphMetrics *font_get_glyph_metrics(Font *font, u32 code_point);
