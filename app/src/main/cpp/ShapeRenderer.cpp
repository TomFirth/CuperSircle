#include "ShapeRenderer.h"
#include "AndroidOut.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static const char *shapeVertexShader = R"vertex(#version 300 es
in vec2 inPosition;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * vec4(inPosition, 0.0, 1.0);
}
)vertex";

static const char *shapeFragmentShader = R"fragment(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)fragment";

ShapeRenderer::ShapeRenderer()
        : program_(0), posAttrib_(-1), projUniform_(-1), colorUniform_(-1), vao_(0), vbo_(0) {}

ShapeRenderer::~ShapeRenderer() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
}

bool ShapeRenderer::init() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shapeVertexShader, nullptr);
    glCompileShader(vs);

    GLint compiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        aout << "Failed to compile Shape Vertex Shader" << std::endl;
        glDeleteShader(vs);
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shapeFragmentShader, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        aout << "Failed to compile Shape Fragment Shader" << std::endl;
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        aout << "Failed to link Shape Program" << std::endl;
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    posAttrib_ = glGetAttribLocation(program_, "inPosition");
    projUniform_ = glGetUniformLocation(program_, "uProjection");
    colorUniform_ = glGetUniformLocation(program_, "uColor");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(posAttrib_);
    glVertexAttribPointer(posAttrib_, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *) 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void ShapeRenderer::setProjectionMatrix(const float *matrix) {
    if (program_ && projUniform_ != -1) {
        glUseProgram(program_);
        glUniformMatrix4fv(projUniform_, 1, GL_FALSE, matrix);
    }
}

void ShapeRenderer::begin() {
    if (program_) {
        glUseProgram(program_);
        glBindVertexArray(vao_);
    }
}

void ShapeRenderer::end() {
    glBindVertexArray(0);
    glUseProgram(0);
}

void ShapeRenderer::drawFilledCircle(float cx, float cy, float radius, const Color &color, int segments) {
    if (!program_) return;

    std::vector<float> vertices;
    vertices.reserve((segments + 2) * 2);

    // Center point
    vertices.push_back(cx);
    vertices.push_back(cy);

    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        vertices.push_back(cx + radius * std::cos(angle));
        vertices.push_back(cy + radius * std::sin(angle));
    }

    glUniform4f(colorUniform_, color.r, color.g, color.b, color.a);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShapeRenderer::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color &color) {
    if (!program_) return;

    float vertices[6] = {x1, y1, x2, y2, x3, y3};

    glUniform4f(colorUniform_, color.r, color.g, color.b, color.a);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShapeRenderer::drawRingArc(float cx, float cy, float innerRadius, float outerRadius,
                                float startAngle, float endAngle, const Color &color, int segments) {
    if (!program_) return;

    if (endAngle < startAngle) {
        endAngle += 2.0f * M_PI;
    }

    std::vector<float> vertices;
    vertices.reserve((segments + 1) * 4);

    float angleSpan = endAngle - startAngle;

    for (int i = 0; i <= segments; ++i) {
        float angle = startAngle + (angleSpan * i) / segments;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Inner vertex
        vertices.push_back(cx + innerRadius * cosA);
        vertices.push_back(cy + innerRadius * sinA);

        // Outer vertex
        vertices.push_back(cx + outerRadius * cosA);
        vertices.push_back(cy + outerRadius * sinA);
    }

    glUniform4f(colorUniform_, color.r, color.g, color.b, color.a);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (segments + 1) * 2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShapeRenderer::drawRect(float x, float y, float width, float height, const Color &color) {
    if (!program_) return;

    float x2 = x + width;
    float y2 = y + height;

    float vertices[12] = {
            x, y,
            x2, y,
            x, y2,
            x2, y,
            x2, y2,
            x, y2
    };

    glUniform4f(colorUniform_, color.r, color.g, color.b, color.a);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShapeRenderer::drawDigit(int digit, float x, float y, float w, float h, const Color &color) {
    float t = std::min(w, h) * 0.15f;

    bool segA = (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 7 || digit == 8 || digit == 9);
    bool segB = (digit == 0 || digit == 1 || digit == 2 || digit == 3 || digit == 4 || digit == 7 || digit == 8 || digit == 9);
    bool segC = (digit == 0 || digit == 1 || digit == 3 || digit == 4 || digit == 5 || digit == 6 || digit == 7 || digit == 8 || digit == 9);
    bool segD = (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 8 || digit == 9);
    bool segE = (digit == 0 || digit == 2 || digit == 6 || digit == 8);
    bool segF = (digit == 0 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9);
    bool segG = (digit == 2 || digit == 3 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9);

    float halfH = h * 0.5f;

    if (segA) drawRect(x, y + h - t, w, t, color);
    if (segB) drawRect(x + w - t, y + halfH, t, halfH, color);
    if (segC) drawRect(x + w - t, y, t, halfH, color);
    if (segD) drawRect(x, y, w, t, color);
    if (segE) drawRect(x, y, t, halfH, color);
    if (segF) drawRect(x, y + halfH, t, halfH, color);
    if (segG) drawRect(x, y + halfH - t * 0.5f, w, t, color);
}

void ShapeRenderer::drawNumber(int number, float x, float y, float digitW, float digitH, const Color &color) {
    if (number < 0) number = 0;
    std::string str = std::to_string(number);
    float spacing = digitW * 0.3f;
    float currentX = x;

    for (char c : str) {
        int d = c - '0';
        drawDigit(d, currentX, y, digitW, digitH, color);
        currentX += digitW + spacing;
    }
}

void ShapeRenderer::drawChar(char c, float x, float y, float w, float h, const Color &color) {
    if (c >= '0' && c <= '9') {
        drawDigit(c - '0', x, y, w, h, color);
        return;
    }
    if (c == ' ' || c == '\0') {
        return;
    }

    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }

    float t = std::min(w, h) * 0.15f;
    float halfH = h * 0.5f;

    switch (c) {
        case 'C':
            drawRect(x, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y, w, t, color);
            break;
        case 'U':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x, y, w, t, color);
            break;
        case 'P':
            drawRect(x, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x + w - t, y + halfH, t, halfH, color);
            drawRect(x, y + halfH - t * 0.5f, w, t, color);
            break;
        case 'E':
            drawRect(x, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y + halfH - t * 0.5f, w, t, color);
            drawRect(x, y, w, t, color);
            break;
        case 'R':
            drawRect(x, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x + w - t, y + halfH, t, halfH, color);
            drawRect(x, y + halfH - t * 0.5f, w, t, color);
            drawRect(x + w * 0.5f, y, t, halfH, color);
            break;
        case 'S':
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y + halfH, t, halfH, color);
            drawRect(x, y + halfH - t * 0.5f, w, t, color);
            drawRect(x + w - t, y, t, halfH, color);
            drawRect(x, y, w, t, color);
            break;
        case 'I':
            drawRect(x + w * 0.5f - t * 0.5f, y, t, h, color);
            break;
        case 'L':
            drawRect(x, y, t, h, color);
            drawRect(x, y, w, t, color);
            break;
        case 'A':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y + halfH - t * 0.5f, w, t, color);
            break;
        case 'D':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x, y + h - t, w * 0.8f, t, color);
            drawRect(x, y, w * 0.8f, t, color);
            break;
        case 'N':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x + w * 0.3f, y + halfH, t, halfH, color);
            break;
        case 'G':
            drawRect(x, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y, w, t, color);
            drawRect(x + w - t, y, t, halfH, color);
            drawRect(x + w * 0.4f, y + halfH - t * 0.5f, w * 0.6f, t, color);
            break;
        case 'M':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x + w * 0.4f, y + halfH, t, halfH, color);
            break;
        case 'O':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y, w, t, color);
            break;
        case 'W':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x + w * 0.4f, y, t, halfH, color);
            break;
        case 'T':
            drawRect(x, y + h - t, w, t, color);
            drawRect(x + w * 0.5f - t * 0.5f, y, t, h - t, color);
            break;
        case 'Q':
            drawRect(x, y, t, h, color);
            drawRect(x + w - t, y, t, h, color);
            drawRect(x, y + h - t, w, t, color);
            drawRect(x, y, w, t, color);
            drawRect(x + w * 0.5f, y, t * 1.5f, t * 1.5f, color);
            break;
        case 'V':
            drawRect(x, y + halfH, t, halfH, color);
            drawRect(x + w - t, y + halfH, t, halfH, color);
            drawRect(x + w * 0.3f, y, t, halfH, color);
            break;
        case 'Y':
            drawRect(x + w * 0.5f - t * 0.5f, y, t, halfH, color);
            drawRect(x, y + halfH, t, halfH, color);
            drawRect(x + w - t, y + halfH, t, halfH, color);
            break;
        default:
            break;
    }
}

void ShapeRenderer::drawText(const std::string &text, float x, float y, float charW, float charH, const Color &color) {
    float spacing = charW * 0.35f;
    float currentX = x;
    for (char c : text) {
        drawChar(c, currentX, y, charW, charH, color);
        currentX += charW + spacing;
    }
}
