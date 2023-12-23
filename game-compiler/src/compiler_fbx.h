#pragma once

#include <mengine/mengine.h>

mengine::GeometryCreate createResourceFromFBX(const bx::FilePath& _filePath, U16 _meshIndex, const bx::FilePath& _vfp);

