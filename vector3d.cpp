#include "vector3d.h"
#include "matrix4.h"

namespace vian {
    namespace geom {
        vector3d vector3d::transform(const matrix4_t &m)
        {
            return vector3d(m.m[0] * x + m.m[4] * y + m.m[8] * z + m.m[12],
                            m.m[1] * x + m.m[5] * y + m.m[9] * z + m.m[13],
                            m.m[2] * x + m.m[6] * y + m.m[10] * z + m.m[14]);
        }
    }
}
