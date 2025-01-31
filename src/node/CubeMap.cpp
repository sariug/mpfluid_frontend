/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "../../include/CubeMap.h"

#include <Corrade/Containers/Optional.h>
#include <Corrade/Utility/Directory.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/CubeMapTexture.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/MeshTools/FlipNormals.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>

#include "../../include/CubeMapShader.h"

namespace Magnum { namespace Examples {

CubeMap::CubeMap(CubeMapResourceManager& resourceManager, const std::string* cubefaces, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group) {
    /* Cube mesh */
    if(!(_cube = resourceManager.get<GL::Mesh>("cube"))) {
        Trade::MeshData3D cubeData = Primitives::cubeSolid();
        MeshTools::flipFaceWinding(cubeData.indices());

        GL::Buffer* buffer = new GL::Buffer;
        buffer->setData(MeshTools::interleave(cubeData.positions(0)), GL::BufferUsage::StaticDraw);

        Containers::Array<char> indexData;
        MeshIndexType indexType;
        UnsignedInt indexStart, indexEnd;
        std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(cubeData.indices());

        GL::Buffer* indexBuffer = new GL::Buffer;
        indexBuffer->setData(indexData, GL::BufferUsage::StaticDraw);

        GL::Mesh* mesh = new GL::Mesh;
        mesh->setPrimitive(cubeData.primitive())
            .setCount(cubeData.indices().size())
            .addVertexBuffer(*buffer, 0, CubeMapShader::Position{})
            .setIndexBuffer(*indexBuffer, 0, indexType, indexStart, indexEnd);

        resourceManager.set("cube-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set("cube-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set(_cube.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Cube map texture */
    if(!(_texture = resourceManager.get<GL::CubeMapTexture>("texture"))) {
        GL::CubeMapTexture* cubeMap = new GL::CubeMapTexture;

        cubeMap->setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear);

        Resource<Trade::AbstractImporter> importer = resourceManager.get<Trade::AbstractImporter>("jpeg-importer");

        /* Configure texture storage using size of first image */
        importer->openFile(cubefaces[0]);
        Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
        CORRADE_INTERNAL_ASSERT(image);
        Vector2i size = image->size();
        Debug{}<<size;
        cubeMap->setStorage(Math::log2(size.min())+1, GL::TextureFormat::RGB8, size)
            .setSubImage(GL::CubeMapCoordinate::PositiveX, 0, {}, *image);

        importer->openFile(cubefaces[1]);
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(GL::CubeMapCoordinate::NegativeX, 0, {}, *image);
              size = image->size();
        Debug{}<<size;
        importer->openFile(cubefaces[2]);
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(GL::CubeMapCoordinate::PositiveY, 0, {}, *image);

        importer->openFile(cubefaces[3]);
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(GL::CubeMapCoordinate::NegativeY, 0, {}, *image);

        importer->openFile(cubefaces[4]);
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(GL::CubeMapCoordinate::PositiveZ, 0, {}, *image);

        importer->openFile(cubefaces[5]);
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(GL::CubeMapCoordinate::NegativeZ, 0, {}, *image);

        cubeMap->generateMipmap();
        resourceManager.set(_texture.key(), cubeMap, ResourceDataState::Final, ResourcePolicy::Manual);
    }
    /* Shader */
    if(!(_shader = resourceManager.get<GL::AbstractShaderProgram, CubeMapShader>("shader")))
        resourceManager.set<GL::AbstractShaderProgram>(_shader.key(), new CubeMapShader, ResourceDataState::Final, ResourcePolicy::Manual);
            

}

void CubeMap::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader->setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .setTexture(*_texture);

    _cube->draw(*_shader);
}

}}
