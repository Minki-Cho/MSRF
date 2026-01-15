#include <glCheck.h>

#include "vec3.h"
#include "color3.h"
#include "Engine.h"
#include "Texture.h" //Texture

Texture::Texture(GLTexture SDLImage)
{
    image = std::move(SDLImage);
}

Texture::Texture(const std::filesystem::path& filePath, bool enableTexel) : enableTexel(enableTexel)
{
    Load(filePath);
}

namespace
{
    std::span<const float, 3 * 3> to_span(const mat3<float>& m)
    {
        return std::span<const float, 3 * 3>(&m.elements[0][0], 9);
    }
    std::span<const float, 3> to_span(const color3& c)
    {
        return std::span<const float, 3>(&c.elements[0], 3);
    }
}

void Texture::Load(const std::filesystem::path& filePath)
{
    shader[0] = GLShader("texture shader", { {GLShader::VERTEX, "assets/shaders/texture_shader.vert"}, {GLShader::FRAGMENT, "assets/shaders/texture_shader.frag"} });
    shader[1] = GLShader("texel shader", { {GLShader::VERTEX, "assets/shaders/texel_shader.vert"}, {GLShader::FRAGMENT, "assets/shaders/texel_shader.frag"} });
    CreateFullscreenQuad();

    if (const bool loaded = image.LoadFromFileImage(filePath); !loaded)
    {
        throw std::runtime_error{ "Failed to load the image" };
    }

    glCheck(glEnable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    glCheck(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
}

//use texture shader
void Texture::Draw(mat3<float> displayMatrix)
{
    float transX = displayMatrix.column2.x * 2 + static_cast<float>(image.GetWidth());
    float transY = displayMatrix.column2.y * 2 + static_cast<float>(image.GetHeight());
    mat3 translation = mat3<float>::build_translation(transX - (1280.f - Engine::GetWindow().GetSize().x), transY - (720.f - Engine::GetWindow().GetSize().y));
    mat3 scale = mat3<float>::build_scale(static_cast<float>(image.GetWidth() * displayMatrix.column0.x), static_cast<float>(image.GetHeight() * displayMatrix.column1.y));
    mat3 to_bottom_left{ mat3<float>::build_translation(-Engine::GetWindow().GetSize().x,-Engine::GetWindow().GetSize().y) };
    const mat3 model_to_world = translation * to_bottom_left * scale;

    //mat3 extent = mat3<float>::build_scale(1 / Engine::GetWindow().GetSize().x, 1 / Engine::GetWindow().GetSize().y);
    mat3 extent = mat3<float>::build_scale(1.f / 1280.f, 1.f / 720.f);
    const mat3<float> model_to_ndc = extent * model_to_world;

    shader[0].Use();
    image.UseForSlot(0);
    shader[0].SendUniform("uTex2d", 0);
    shader[0].SendUniform("uModelToNDC", to_span(model_to_ndc));
    fullscreenQuad.Use();
    GLDrawIndexed(fullscreenQuad);
    fullscreenQuad.Use(false);
    shader[0].Use(false);
}

//use texel shader
void Texture::Draw(mat3<float> displayMatrix, vec2 texelPos, vec2 frameSize)
{
    mat3 translation{ mat3<float>::build_translation(displayMatrix.column2.x - (1280.f - Engine::GetWindow().GetSize().x) / 2.f,displayMatrix.column2.y - (720.f - Engine::GetWindow().GetSize().y) / 2.f) };
    mat3 scale = mat3<float>::build_scale(static_cast<float>(frameSize.x) * displayMatrix.column0.x, static_cast<float>(frameSize.y) * displayMatrix.column1.y);
    mat3 to_bottom_left{ mat3<float>::build_translation(-Engine::GetWindow().GetSize().x / 2,-Engine::GetWindow().GetSize().y / 2) };
    //mat3<float> model_bottom_left = mat3<float>::build_translation(static_cast<float>(image.GetWidth()), static_cast<float>(image.GetHeight()));
    const mat3<float> model_to_world = translation * to_bottom_left * scale;

    //mat3 extent = mat3<float>::build_scale(2 / Engine::GetWindow().GetSize().x, 2 / Engine::GetWindow().GetSize().y);
    mat3 extent = mat3<float>::build_scale(2.f / 1280.f, 2.f / 720.f);
    const mat3 model_to_ndc = extent * model_to_world;

    shader[1].Use();
    image.UseForSlot(0);
    shader[1].SendUniform("uTex2d", 0);
    shader[1].SendUniform("uTexelPos", texelPos.x / image.GetWidth(), texelPos.y / image.GetHeight());
    shader[1].SendUniform("uFrameSize", frameSize.x / image.GetWidth(), frameSize.y / image.GetHeight());
    shader[1].SendUniform("uModelToNDC", to_span(model_to_ndc));
    fullscreenQuad.Use();
    GLDrawIndexed(fullscreenQuad);
    fullscreenQuad.Use(false);
    shader[1].Use(false);
}

vec2 Texture::GetSize() { return { static_cast<float>(image.GetWidth()), static_cast<float>(image.GetHeight()) }; }

void Texture::CreateFullscreenQuad()
{
    std::array<vec2, 4> positions;
    switch (enableTexel)
    {
    case 0:
        positions = { vec2{-1.f,1.f}, vec2{-1.f,-1.f}, vec2{1.f,1.f}, vec2{1.f,-1.f} };
        break;
    case 1:
        positions = { vec2{0.f,1.f}, vec2{0.f,0.f}, vec2{1.f,1.f}, vec2{1.f,0.f} };
        break;
    }
    constexpr std::array<unsigned char, 4> indices = { 0, 1, 2, 3 };

    GLAttributeLayout position_layout;
    position_layout.component_type = GLAttributeLayout::Float;
    position_layout.component_dimension = GLAttributeLayout::_2;
    position_layout.normalized = false;
    position_layout.vertex_layout_location = 0;
    position_layout.stride = sizeof(vec2);
    position_layout.offset = 0;
    position_layout.relative_offset = 0;

    fullscreenQuad.AddVertexBuffer(GLVertexBuffer(std::span{ positions }), { position_layout });
    fullscreenQuad.SetPrimitivePattern(GLPrimitive::TriangleStrip);

    GLIndexBuffer index_buffer(indices);
    fullscreenQuad.SetIndexBuffer(std::move(index_buffer));
}