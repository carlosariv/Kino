#include <assert.h>
#include <fstream>
#include <filesystem>

#include <stb/stb_truetype.h>
#include <stb/stb_rect_pack.h>

#include "os.h"
#include "array.h"
#include "font.h"
#include "texture.h"

FontProvider *font_provider;

GlyphMetrics *font_get_glyph_metrics(Font *font, u32 code_point) {
    GlyphMetrics *glyph = font->default_glyph;
    auto it = font->metrics_table.find(code_point);
    if (it != font->metrics_table.end()) {
        glyph = &it->second;
    }
    return glyph;
}

Font *font_create(String file_name, int size) {
    u32 start_code_point = 0;
    u32 end_code_point = '~';
    u32 *code_points = new u32[end_code_point - start_code_point+1];

    for (u32 code_point = start_code_point; code_point <= end_code_point; code_point++) {
        code_points[code_point-start_code_point] = code_point;
    }

    Font *font = font_create(file_name, size, code_points, end_code_point - start_code_point);

    delete [] code_points;
    return font;
}

Font *font_create(String file_name, int size, u32 *code_points, int code_point_count) {
    if (!font_provider) {
        Arena *arena = make_arena(cu_megabytes(4));
        font_provider = New(FontProvider, arena_allocator(arena));
        font_provider->arena = arena;
    }

    // FILE* File = fopen("C:\\windows\\Fonts\\arial.ttf", "rb");
    // [...]
    // stbtt_fontinfo stbFont;
    // stbtt_InitFont(&stbFont, (u8*)FileContent, 0);
    // r32 stbScale = stbtt_ScaleForPixelHeight(&stbFont, 128);
    //
    // const u32 StartCodePoint = '!';
    // const u32 EndCodePoint = 'z';
    // const u32 CodePointCount = 1 + EndCodePoint - StartCodePoint;
    // r32* HorizontalAdvance = malloc(sizeof(r32) * Font->CodePointCount * Font->CodePointCount));
    //
    // //Get the image for every code-point we need
    // for(u32 CodePoint = StartCodePoint; CodePoint <= EndCodePoint; ++CodePoint) {
    //   s32 AdvanceWidth;
    //   stbtt_GetCodepointHMetrics(&stbFont, CodePoint, &AdvanceWidth, 0);
    //
    //   s32 Width, Height;
    //   s32 xOffset, yOffset;
    //   u8* MonoBitmap = stbtt_GetCodepointBitmap(&stbFont, stbScale, stbScale, CodePoint, &Width, &Height, &xOffset, &yOffset);
    //   loaded_bitmap Bitmap = MakeEmptyBitmap(&TransientState->Arena, Width, Height);
    //   Bitmap->Memory = MonoBitmap;
    //   //Bitmap->AlignPercentage = {0.5f - ((r32)xOffset/(r32)Width), 0.5};
    //   //Bitmap->AlignPercentage = {(r32)xOffset /(r32)Width , 0.5f};
    //
    //   //Kerning
    //   for(u32 PrevCodePoint = StartCodePoint; PrevCodePoint <= EndCodePoint; ++PrevCodePoint) {
    //     s32 Kerning = stbtt_GetCodepointKernAdvance(&stbFont, PrevCodePoint, CodePoint);
    //     HorizontalAdvance[PrevCodePoint*CodePointCount + CodePoint] = (stbScale * AdvanceWidth) + (stbScale * Kerning);
    //   }
    // }
    //

    Font *font = new Font();
    font->size = size;
    font->file_name = file_name;

    os::Handle file_handle = os::open_file(file_name, os::FileAccessFlag_Read|os::FileAccessFlag_Open);
    if (!os::is_valid_handle(file_handle)) {
        fprintf(stderr, "Error: Could not find file %s", (char *)file_name.text);
        return nullptr;
    }

    String contents = os::read_entire_file(file_handle);
    os::close_handle(file_handle);

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, contents.text, 0)) {
        fprintf(stderr, "Error: Could not read font %s", (char *)file_name.text);
    }

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

    f32 line_height = floorf(96.f/72.f * size);
    f32 scale = stbtt_ScaleForMappingEmToPixels(&info, line_height);

    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    font->scale = scale;
    font->ascent = ascent;
    font->descent = descent;
    font->line_gap = line_gap;

    stbrp_node nodes[256];
    Array<stbrp_rect> rects;
    rects.reserve(100);

    int atlas_offset = 5;

    // TODO: Make bitmap dimensions depend on scale of font and line height
    int bytes_per_pixel = 1;
    int bitmap_width = 512;
    int bitmap_height = 512;
    u8 *bitmap = cu_alloc_array(u8, arena_allocator(font_provider->arena), bitmap_width * bitmap_height * bytes_per_pixel);
    memset(bitmap, 0, bitmap_width * bitmap_height * bytes_per_pixel);

    stbrp_context rp_ctx;
    stbrp_init_target(&rp_ctx, bitmap_width - atlas_offset, bitmap_height - atlas_offset, nodes, 256);

    int gap_size = 5;

    //NOTE: Get glyph metrics
    u32 start_code_point = 0;
    u32 end_code_point = '~';
    for (int i = 0; i < code_point_count; i++) {
        u32 code_point = code_points[i];
        if (stbtt_FindGlyphIndex(&info, code_point) == 0) {
            continue;
        }

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&info, code_point, scale, scale, &x0, &y0, &x1, &y1);

        GlyphMetrics metrics;
        metrics.y_off = y0;
        font->metrics_table.insert({code_point, metrics});

        int width = x1 - x0;
        int height = y1 - y0;

        stbrp_rect rect;
        rect.id = code_point;
        rect.w = width + gap_size;
        rect.h = height + gap_size;
        rects.add(rect);
    }

    stbrp_pack_rects(&rp_ctx, rects.data, rects.count);

    //NOTE: White rectangle at 0,0
    {
        u8 *p = bitmap;
        for (int y = 0; y < 2; y++) {
            memset(p, 255, 2);
            p += bitmap_width * bytes_per_pixel;
        }
    }

    for (int i = 0; i < rects.count; i++) {
        stbrp_rect rect = rects[i];
        assert(rect.was_packed);

        u32 code_point = (u32)rect.id;

        auto glyph_it = font->metrics_table.find(code_point);
        if (glyph_it == font->metrics_table.end()) {
            continue;
        }

        int advance_width;
        int left_side_bearing;
        stbtt_GetCodepointHMetrics(&info, code_point, &advance_width, &left_side_bearing);

        int width = rect.w - gap_size;
        int height = rect.h - gap_size;

        int x = rect.x + atlas_offset;
        int y = rect.y + atlas_offset;


        GlyphMetrics &metrics = glyph_it->second;
        metrics.bl = x;
        metrics.bt = y;
        metrics.bw = width;
        metrics.bh = height;
        metrics.advance = advance_width * font->scale;
        metrics.lsb = left_side_bearing * font->scale;

        if (metrics.advance > font->max_advance) {
            font->max_advance = metrics.advance;
        }

        int byte_offset = x + (y * bitmap_width * bytes_per_pixel);
        stbtt_MakeCodepointBitmap(&info, bitmap + byte_offset, width, height, bitmap_width * bytes_per_pixel, scale, scale, code_point);

        //   //Kerning
        //   for(u32 PrevCodePoint = StartCodePoint; PrevCodePoint <= EndCodePoint; ++PrevCodePoint) {
        //     s32 Kerning = stbtt_GetCodepointKernAdvance(&stbFont, PrevCodePoint, CodePoint);
        //     HorizontalAdvance[PrevCodePoint*CodePointCount + CodePoint] = (stbScale * AdvanceWidth) + (stbScale * Kerning);
        //   }
    }

    font->width = bitmap_width;
    font->height = bitmap_height;
    font->line_skip = line_height;

    Texture *texture = texture_create(bitmap, bitmap_width, bitmap_height, TextureFormat::R8_UNorm);
    font->texture = texture;

    return font;
}
