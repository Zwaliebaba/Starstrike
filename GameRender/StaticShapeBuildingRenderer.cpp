#include "pch.h"

#include "StaticShapeBuildingRenderer.h"
#include "staticshape.h"
#include "matrix34.h"
#include "ShapeStatic.h"

void StaticShapeBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const StaticShape& shape = static_cast<const StaticShape&>(_building);

    if (shape.m_shape)
    {
        Matrix34 mat(shape.m_front, shape.m_up, shape.m_pos);
        mat.u *= shape.m_scale;
        mat.r *= shape.m_scale;
        mat.f *= shape.m_scale;

        glEnable(GL_NORMALIZE);
        shape.m_shape->Render(_ctx.predictionTime, mat);
        glDisable(GL_NORMALIZE);
    }
}
