#pragma once

#include <string_view>

namespace UI
{
    struct Color
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
    };

    struct Rect
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        bool Contains(float px, float py) const
        {
            return px >= x && px <= (x + w) && py >= y && py <= (y + h);
        }
    };

    struct Theme
    {
        Color panelBg{ 0.08f, 0.11f, 0.16f };
        Color panelBorder{ 0.35f, 0.55f, 0.80f };
        Color buttonIdle{ 0.14f, 0.20f, 0.30f };
        Color buttonHover{ 0.22f, 0.30f, 0.43f };
        Color buttonPressed{ 0.35f, 0.47f, 0.62f };
        Color buttonDanger{ 0.55f, 0.18f, 0.18f };
        Color text{ 0.92f, 0.94f, 0.98f };
        Color textMuted{ 0.70f, 0.74f, 0.80f };
        Color accent{ 0.23f, 0.70f, 0.34f };
    };

    class Framework
    {
    public:
        void BeginFrame();
        void EndFrame();

        const Theme& GetTheme() const { return theme; }

        void FillRect(const Rect& rect, const Color& color);
        void Panel(const Rect& rect, const Color& bg, const Color& border, float borderPx = 2.0f);
        void Label(float x, float y, std::string_view text, float pixelSize, const Color& color);
        void LabelCentered(const Rect& rect, std::string_view text, float pixelSize, const Color& color);
        void ProgressBar(const Rect& rect, float ratio, const Color& fill, const Color& bg, const Color& border);
        bool Button(const Rect& rect, std::string_view text, bool danger = false);

        bool IsMouseReleasedIn(const Rect& rect) const;
        bool IsMouseHovering(const Rect& rect) const;

    private:
        void EnsureRenderer();
        void EmitRectPx(float x, float y, float w, float h, const Color& color);
        void EmitTextPx(float x, float y, std::string_view text, float pixelSize, const Color& color);
        float MeasureTextWidthPx(std::string_view text, float pixelSize) const;
        static const unsigned char* GetGlyphRows(char c);
        static char ToUpperAscii(char c);

    private:
        struct Impl;
        Impl* impl = nullptr;
        Theme theme{};
    };

    Framework& Get();
}
