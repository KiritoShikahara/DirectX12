#pragma once

#include<vector>
#include<string>
#include<memory>
#include<filesystem>
#include<Utility/Export/Export.h>

#include<graphics/IndexBuffer/IndexBuffer.h>
#include<graphics/VertexBuffer/VertexBuffer.h>
#include<graphics/Model/ModelData.h>

namespace graphics
{
	class ENGINE_API ModelResource
	{
    public:
        ModelResource() = default;
        ~ModelResource() = default;

        ModelResource(const ModelResource&) = delete;
        ModelResource& operator=(const ModelResource&) = delete;
        ModelResource(ModelResource&&) = delete;
        ModelResource& operator=(ModelResource&&) = delete;



	};
}