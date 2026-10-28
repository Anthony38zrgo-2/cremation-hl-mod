// Runtime GDI rasterization keeps the OFL fonts readable in GoldSrc's HUD.
// The launcher loads the bundled files into the current Windows session before
// starting Half-Life; this module only caches their glyphs as GL textures.
#include <vector>
#include <cstdint>

struct CremationFontTexture {
    std::string key;
    GLuint texture = 0;
    int width = 0;
    int height = 0;
    unsigned int used = 0;
};

static std::vector<CremationFontTexture> cremationFontTextures;
static unsigned int cremationFontClock = 0;

static int cremationNextPowerOfTwo(int value) {
    int result = 1;
    while (result < value && result < 4096) result <<= 1;
    return result;
}

static CremationFontTexture* cremationFontTexture(const char* family, const char* text, int pixels) {
    if (!family || !text || !*text || pixels < 1 || !wglGetCurrentContext()) return nullptr;
    std::string key(family);
    key.push_back('\n');
    key += std::to_string(pixels);
    key.push_back('\n');
    key += text;
    for (auto& item : cremationFontTextures) {
        if (item.key == key) {
            item.used = ++cremationFontClock;
            return &item;
        }
    }

    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return nullptr;
    HFONT font = CreateFontA(-pixels, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, family);
    if (!font) { DeleteDC(dc); return nullptr; }
    HGDIOBJ oldFont = SelectObject(dc, font);
    SIZE extent{};
    TEXTMETRICA metrics{};
    if (!GetTextExtentPoint32A(dc, text, static_cast<int>(std::strlen(text)), &extent) ||
        !GetTextMetricsA(dc, &metrics)) {
        SelectObject(dc, oldFont); DeleteObject(font); DeleteDC(dc); return nullptr;
    }
    const int pad = 2;
    const int width = std::max(1, static_cast<int>(extent.cx) + pad * 2);
    const int height = std::max(1, static_cast<int>(metrics.tmHeight) + 3);
    const int textureWidth = cremationNextPowerOfTwo(width);
    const int textureHeight = cremationNextPowerOfTwo(height);
    if (textureWidth > 4096 || textureHeight > 4096) {
        SelectObject(dc, oldFont); DeleteObject(font); DeleteDC(dc); return nullptr;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* dibBits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &dibBits, nullptr, 0);
    if (!bitmap || !dibBits) {
        if (bitmap) DeleteObject(bitmap);
        SelectObject(dc, oldFont); DeleteObject(font); DeleteDC(dc); return nullptr;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    std::memset(dibBits, 0, static_cast<size_t>(width) * height * 4);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutA(dc, pad, 0, text, static_cast<int>(std::strlen(text)));

    std::vector<unsigned char> rgba(static_cast<size_t>(textureWidth) * textureHeight * 4, 0);
    const auto* source = static_cast<const unsigned char*>(dibBits);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t src = (static_cast<size_t>(y) * width + x) * 4;
            const unsigned char alpha = std::max(source[src], std::max(source[src + 1], source[src + 2]));
            const size_t dst = (static_cast<size_t>(y) * textureWidth + x) * 4;
            rgba[dst] = rgba[dst + 1] = rgba[dst + 2] = 255;
            rgba[dst + 3] = alpha;
        }
    }
    SelectObject(dc, oldBitmap);
    SelectObject(dc, oldFont);
    DeleteObject(bitmap);
    DeleteObject(font);
    DeleteDC(dc);

    if (cremationFontTextures.size() >= 64) {
        auto oldest = std::min_element(cremationFontTextures.begin(), cremationFontTextures.end(),
            [](const CremationFontTexture& a, const CremationFontTexture& b) { return a.used < b.used; });
        if (oldest->texture) glDeleteTextures(1, &oldest->texture);
        *oldest = cremationFontTextures.back();
        cremationFontTextures.pop_back();
    }

    CremationFontTexture entry;
    entry.key = key;
    entry.width = width;
    entry.height = height;
    entry.used = ++cremationFontClock;
    glGenTextures(1, &entry.texture);
    if (!entry.texture) return nullptr;
    glBindTexture(GL_TEXTURE_2D, entry.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    cremationFontTextures.push_back(entry);
    return &cremationFontTextures.back();
}

static void drawCremationFontText(const char* text, const char* family, int pixels,
    int centerX, int y, int r, int g, int b) {
    SCREENINFO screen{};
    screen.iSize = sizeof(screen);
    if (!eng.pfnGetScreenInfo(&screen) || screen.iWidth <= 0 || screen.iHeight <= 0) return;
    CremationFontTexture* item = cremationFontTexture(family, text, pixels);
    if (!item) {
        int width = 0, height = 0;
        eng.pfnDrawConsoleStringLen(text, &width, &height);
        eng.pfnDrawSetTextColor(r / 255.f, g / 255.f, b / 255.f);
        eng.pfnDrawConsoleString(centerX - width / 2, y, const_cast<char*>(text));
        return;
    }

    GLint oldMatrixMode = GL_MODELVIEW;
    glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, screen.iWidth, screen.iHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, item->texture);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screen.iWidth, screen.iHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const int x = centerX - item->width / 2;
    const float u = static_cast<float>(item->width) /
        static_cast<float>(cremationNextPowerOfTwo(item->width));
    const float v = static_cast<float>(item->height) /
        static_cast<float>(cremationNextPowerOfTwo(item->height));
    auto quad = [&](int left, int top) {
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2i(left, top);
        glTexCoord2f(u, 0); glVertex2i(left + item->width, top);
        glTexCoord2f(u, v); glVertex2i(left + item->width, top + item->height);
        glTexCoord2f(0, v); glVertex2i(left, top + item->height);
        glEnd();
    };
    glColor4ub(0, 0, 0, 190);
    quad(x + 2, y + 2);
    glColor4ub(static_cast<GLubyte>(r), static_cast<GLubyte>(g), static_cast<GLubyte>(b), 255);
    quad(x, y);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(oldMatrixMode);
    glPopAttrib();
}

static void clearCremationFontCache() {
    if (wglGetCurrentContext()) {
        for (auto& item : cremationFontTextures)
            if (item.texture) glDeleteTextures(1, &item.texture);
    }
    cremationFontTextures.clear();
    cremationFontClock = 0;
}
