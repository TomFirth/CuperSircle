#ifndef CUPERSIRCLE_SHAPERENDERER_H
#define CUPERSIRCLE_SHAPERENDERER_H

#include <GLES3/gl3.h>
#include <vector>
#include <cmath>

struct Color {
    float r, g, b, a;
    constexpr Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
            : r(r), g(g), b(b), a(a) {}

    static Color fromHex(uint32_t hex, float alpha = 1.0f) {
        float red = ((hex >> 16) & 0xFF) / 255.0f;
        float green = ((hex >> 8) & 0xFF) / 255.0f;
        float blue = (hex & 0xFF) / 255.0f;
        return Color(red, green, blue, alpha);
    }
};

class ShapeRenderer {
public:
    ShapeRenderer();
    ~ShapeRenderer();

    bool init();
    void setProjectionMatrix(const float *matrix);
    void begin();
    void end();

    void drawFilledCircle(float cx, float cy, float radius, const Color &color, int segments = 48);
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color &color);
    void drawRingArc(float cx, float cy, float innerRadius, float outerRadius,
                     float startAngle, float endAngle, const Color &color, int segments = 64);
    void drawRect(float x, float y, float width, float height, const Color &color);
    void drawDigit(int digit, float x, float y, float w, float h, const Color &color);
    void drawNumber(int number, float x, float y, float digitW, float digitH, const Color &color);
    void drawChar(char c, float x, float y, float w, float h, const Color &color);
    void drawText(const std::string &text, float x, float y, float charW, float charH, const Color &color);

private:
    GLuint program_;
    GLint posAttrib_;
    GLint projUniform_;
    GLint colorUniform_;

    GLuint vao_;
    GLuint vbo_;
};

#endif // CUPERSIRCLE_SHAPERENDERER_H
