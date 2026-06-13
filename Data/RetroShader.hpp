#pragma once

namespace bunker {

// Высокопроизводительный HLSL шейдерный код ретро-люминофора v15
static const char* g_BunkerRetroHLSL = R"(
struct VS_INPUT 
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct PS_INPUT 
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float4 screenPos : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input) 
{
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    output.screenPos = float4(input.pos, 1.0f); // Передаем NDC для эффекта линий развертки
    return output;
}

float4 PS(PS_INPUT input) : SV_Target 
{
    float4 finalColor = input.col;
    
    // Аппаратный расчет полос развертки ЭЛТ монитора на пиксельном конвейере ГПУ
    // Переводим NDC координаты [-1, 1] в пиксельное пространство экрана 720p
    float pixelY = (input.screenPos.y * 0.5f + 0.5f) * 720.0f;
    
    // Каждые 3 пикселя накладываем горизонтальную тень интерлейсинга
    if (fmod(pixelY, 3.0f) < 1.0f) 
    {
        finalColor.rgb *= 0.88f; // Мягкое затемнение строки люминофора
    }

    return finalColor;
}
)";

} // namespace bunker
