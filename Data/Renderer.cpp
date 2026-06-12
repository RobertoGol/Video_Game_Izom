#include "../main.hpp"
#include "Renderer.hpp"
#include <cmath>

ScreenPoint PixelsToNDC(float x, float y, float width, float height) {
    return { (x / width) * 2.0f - 1.0f, (1.0f - (y / height) * 2.0f) };
}

void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments) {
    for (int i = 0; i < segments; ++i) {
        float a1 = (i / (float)segments) * 6.2831853f;
        float a2 = ((i + 1) / (float)segments) * 6.2831853f;
        
        ScreenPoint p0 = PixelsToNDC(cx, cy, width, height);
        ScreenPoint p1 = PixelsToNDC(cx + std::cos(a1) * r, cy + std::sin(a1) * r, width, height);
        ScreenPoint p2 = PixelsToNDC(cx + std::cos(a2) * r, cy + std::sin(a2) * r, width, height);
        
        buffer.push_back({p0.x, p0.y, 0.0f, col.r, col.g, col.b, col.a});
        buffer.push_back({p1.x, p1.y, 0.0f, col.r, col.g, col.b, col.a});
        buffer.push_back({p2.x, p2.y, 0.0f, col.r, col.g, col.b, col.a});
    }
}
