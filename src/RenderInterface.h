#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include "RmlUi_Renderer_GL3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


class RenderInterface : public Rml::RenderInterface {
    public:


    Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
    {
        constexpr GLenum draw_usage = GL_STATIC_DRAW;

        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ibo = 0;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Rml::Vertex) * vertices.size(), (const void*)vertices.data(), draw_usage);

        glEnableVertexAttribArray((GLuint)Gfx::VertexAttribute::Position);
        glVertexAttribPointer((GLuint)Gfx::VertexAttribute::Position, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
            (const GLvoid*)(offsetof(Rml::Vertex, position)));

        glEnableVertexAttribArray((GLuint)Gfx::VertexAttribute::Color0);
        glVertexAttribPointer((GLuint)Gfx::VertexAttribute::Color0, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex),
            (const GLvoid*)(offsetof(Rml::Vertex, colour)));

        glEnableVertexAttribArray((GLuint)Gfx::VertexAttribute::TexCoord0);
        glVertexAttribPointer((GLuint)Gfx::VertexAttribute::TexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
            (const GLvoid*)(offsetof(Rml::Vertex, tex_coord)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), (const void*)indices.data(), draw_usage);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        Gfx::CheckGLError("CompileGeometry");

        Gfx::CompiledGeometryData* geometry = new Gfx::CompiledGeometryData;
        geometry->vao = vao;
        geometry->vbo = vbo;
        geometry->ibo = ibo;
        geometry->draw_count = (GLsizei)indices.size();

        return (Rml::CompiledGeometryHandle)geometry;
    }
    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override {

        int width, height, channels;
        stbi_uc* image_data = stbi_load(source.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!image_data)
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, ("Failed to load image: " + source).c_str());
            return 0; // invalid texture handle
        }

        // STBI_rgb_alpha ensures 4 channels (RGBA)
        size_t image_size = width * height * 4;

        Rml::UniquePtr<Rml::byte[]> buffer(new Rml::byte[image_size]);
        memcpy(buffer.get(), image_data, image_size);

        stbi_image_free(image_data);

        texture_dimensions.x = width;
        texture_dimensions.y = height;

        return GenerateTexture({buffer.get(), image_size}, texture_dimensions);
    };
};