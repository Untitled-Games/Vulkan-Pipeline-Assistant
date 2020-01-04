#include "spirvresource.h"

const QString vpa::ShaderStageStrings[size_t(ShaderStage::count_)] = { "Vertex", "Fragment", "TessControl", "TessEval", "Geometry" };
const QString vpa::SpvGroupnameStrings[size_t(SpvGroupname::count_)] = { "InputAttribute", "UniformBuffer", "StorageBuffer", "PushConstant", "Image" };
const QString SpvTypenameStrings[size_t(SpvTypename::count_)] = { "Image", "Array", "Vector", "Matrix", "Struct" };
const QString SpvImageTypenameStrings[size_t(SpvImageTypename::count_)] = { "Texture1D", "Texture2D", "Texture3D", "TextureCube", "UnknownTexture" };
