#include <array>

#include <glCheck.h>
#include "angles.h"

#include "Collision.h" //Collision
#include "GameObject.h" //GameObject
#include "color3.h"
#include "Engine.h"

//for test
#include<iostream>

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

RectCollision::RectCollision(rect3 rect, GameObject* objectPtr) : rect(rect), objectPtr(objectPtr)
{
    shader = GLShader("collisionBox shader", { {GLShader::VERTEX, "assets/shaders/model_to_ndc_pos2d_clr.vert"}, {GLShader::FRAGMENT, "assets/shaders/basic_vtx_clr_attribute.frag"} });

    //constexpr std::array positions = { vec2{0.f,1.f}, vec2{0.f,0.f}, vec2{1.f,1.f}, vec2{1.f,0.f} };
    constexpr std::array positions = { vec2{0.f,0.f}, vec2{1.f,0.f}, vec2{1.f,1.f}, vec2{0.f,1.f} };
    constexpr std::array colors = { color3{0, 0, 0}, color3{0, 0, 0}, color3{0, 0, 0}, color3{0, 0, 0} };
    constexpr std::array<unsigned char, 5> indices = { 0, 1, 2, 3, 0 };

    GLAttributeLayout position_layout;
    position_layout.component_type = GLAttributeLayout::Float;
    position_layout.component_dimension = GLAttributeLayout::_2;
    position_layout.normalized = false;
    position_layout.vertex_layout_location = 0;
    position_layout.stride = sizeof(vec2);
    position_layout.offset = 0;
    position_layout.relative_offset = 0;

    GLAttributeLayout color_layout;
    color_layout.component_type = GLAttributeLayout::Float;
    color_layout.component_dimension = GLAttributeLayout::_3;
    color_layout.normalized = false;
    color_layout.vertex_layout_location = 1;
    color_layout.stride = sizeof(color3);
    color_layout.offset = 0;
    color_layout.relative_offset = 0;

    collisionBox.AddVertexBuffer(GLVertexBuffer(std::span{ positions }), { position_layout });
    collisionBox.AddVertexBuffer(GLVertexBuffer(std::span{ colors }), { color_layout });
    //collisionBox.SetPrimitivePattern(GLPrimitive::TriangleStrip);
    collisionBox.SetPrimitivePattern(GLPrimitive::LineStrip);

    GLIndexBuffer index_buffer(indices);
    collisionBox.SetIndexBuffer(std::move(index_buffer));

    //glCheck(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    glCheck(glLineWidth(5));
}

void RectCollision::Draw(mat3<float>)
{
    mat3<float> translation = mat3<float>::build_translation(GetWorldCoorRect().Left() - (1280.f - Engine::GetWindow().GetSize().x) / 2.f, GetWorldCoorRect().Bottom() - (720.f - Engine::GetWindow().GetSize().y) / 2.f);
    mat3<float> scale = mat3<float>::build_scale(GetWorldCoorRect().Size().x, GetWorldCoorRect().Size().y);
    mat3 to_bottom_left{ mat3<float>::build_translation(-Engine::GetWindow().GetSize().x / 2,-Engine::GetWindow().GetSize().y / 2) };
    //mat3<float> model_bottom_left = mat3<float>::build_translation(rect.Size().width, rect.Size().height);
    const mat3<float> model_to_world = to_bottom_left * translation * scale;

    //mat3<float> extent = mat3<float>::build_scale(2 / Engine::GetWindow().GetSize().x, 2 / Engine::GetWindow().GetSize().y);
    mat3 extent = mat3<float>::build_scale(2.f / 1280.f, 2.f / 720.f);
    const mat3 model_to_ndc = extent * model_to_world;

    shader.Use();
    shader.SendUniform("uModelToNDC", to_span(model_to_ndc));
    collisionBox.Use();
    GLDrawIndexed(collisionBox);
    collisionBox.Use(false);
    shader.Use(false);
}

rect3 RectCollision::GetWorldCoorRect()
{
    return
    {
        objectPtr->GetMatrix() * rect.point1,
        objectPtr->GetMatrix() * rect.point2
    };
}

bool RectCollision::DoesCollideWith(GameObject* objectB)
{
    if (objectB->GetGOComponent<Collision>() != nullptr && objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Rect_Collide)
    {
        rect3 objectB_Rect = objectB->GetGOComponent<RectCollision>()->GetWorldCoorRect();
        if (GetWorldCoorRect().Right() > objectB_Rect.Left() && GetWorldCoorRect().Left() < objectB_Rect.Right())
        {
            if (GetWorldCoorRect().Top() > objectB_Rect.Bottom() && GetWorldCoorRect().Bottom() < objectB_Rect.Top())
            {
                return true;
            }
        }
    }
    return false;
}

bool RectCollision::DoesCollideWith(vec2 point)
{
    if (point.x >= GetWorldCoorRect().Left() && point.x <= GetWorldCoorRect().Right())
    {
        if (point.y >= GetWorldCoorRect().Bottom() && point.y <= GetWorldCoorRect().Top())
        {
            return true;
        }

        if (point.y + point.height >= GetWorldCoorRect().Bottom() && point.y + point.height <= GetWorldCoorRect().Top())
        {
            return true;
        }
    }
    return false;
}

CircleCollision::CircleCollision(double radius, GameObject* objectPtr) : radius(radius), objectPtr(objectPtr)
{
    shader = GLShader("collisionCircle shader", { {GLShader::VERTEX, "assets/shaders/model_to_ndc_pos2d_clr.vert"}, {GLShader::FRAGMENT, "assets/shaders/basic_vtx_clr_attribute.frag"} });

    constexpr int slices = 30;
    std::vector<vec2> pos_vtx(slices + 1);
    for (int i = 0; i <= slices; i++)
    {
        pos_vtx[i] = vec2{ cos(util::to_radians(360.0f / slices) * i),sin(util::to_radians(360.0f / slices) * i) };
    }

    std::vector<color3> color(slices + 1);
    for (int i = 0; i <= slices; i++)
    {
        color[i] = color3{ 1,0,0 };
    }

    collisionCircle.SetVertexCount(static_cast<int>(pos_vtx.size()));

    GLAttributeLayout position;
    position.component_dimension = GLAttributeLayout::_2;
    position.component_type = GLAttributeLayout::Float;
    position.normalized = false;
    position.offset = 0;
    position.relative_offset = 0;
    position.stride = sizeof(vec2);
    position.vertex_layout_location = 0;

    GLAttributeLayout color_layout;
    color_layout.component_type = GLAttributeLayout::Float;
    color_layout.component_dimension = GLAttributeLayout::_3;
    color_layout.normalized = false;
    color_layout.vertex_layout_location = 1;
    color_layout.stride = sizeof(color3);
    color_layout.offset = 0;
    color_layout.relative_offset = 0;

    collisionCircle.AddVertexBuffer(GLVertexBuffer(std::span{ pos_vtx }), { position });
    collisionCircle.AddVertexBuffer(GLVertexBuffer(std::span{ color }), { color_layout });

    collisionCircle.SetPrimitivePattern(GLPrimitive::LineStrip);

    glCheck(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    glCheck(glLineWidth(3));
}

void CircleCollision::Draw(mat3<float> cameraMatrix)
{
    mat3<float> scale = mat3<float>::build_scale(static_cast<float>(radius * 2));
    mat3<float> translation = mat3<float>::build_translation(cameraMatrix.column2.x, cameraMatrix.column2.y);
    const mat3<float> model_to_world = translation * scale;

    mat3 extent = mat3<float>::build_scale(1 / Engine::GetWindow().GetSize().x, 1 / Engine::GetWindow().GetSize().y);
    mat3 to_bottom_left{ mat3<float>::build_translation(-Engine::GetWindow().GetSize().x / 2,-Engine::GetWindow().GetSize().y / 2) };
    const mat3 model_to_ndc = extent * to_bottom_left * model_to_world;

    shader.Use();
    shader.SendUniform("uModelToNDC", to_span(model_to_ndc));
    collisionCircle.Use();
    GLDrawVertices(collisionCircle);
    collisionCircle.Use(false);
    shader.Use(false);
}

double CircleCollision::GetRadius()
{
    return (mat3<float>::build_scale(objectPtr->GetScale()) * vec3 { static_cast<float>(radius), 0, 1.0 }).x;
}

bool CircleCollision::DoesCollideWith(GameObject* objectB)
{
    if (objectB->GetGOComponent<Collision>() != nullptr && objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Circle_Collide)
    {
        double d_x = (objectPtr->GetPosition().x - objectB->GetPosition().x);
        double d_y = (objectPtr->GetPosition().y - objectB->GetPosition().y);
        double distance = (d_x * d_x) + (d_y * d_y);
        double d_r = GetRadius() + objectB->GetGOComponent<CircleCollision>()->GetRadius();
        if (distance < d_r * d_r)
        {
            return true;
        }
    }
    return false;
}

bool CircleCollision::DoesCollideWith(vec2 point)
{
    double d_x = (objectPtr->GetPosition().x - point.x);
    double d_y = (objectPtr->GetPosition().y - point.y);
    double distance = (d_x * d_x) + (d_y * d_y);
    if (distance <= GetRadius() * GetRadius())
    {
        return true;
    }
    return false;
}