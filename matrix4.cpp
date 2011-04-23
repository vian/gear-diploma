#include "matrix4.h"
#include <memory.h>

namespace vian {
    namespace geom {
        matrix4_t::matrix4_t(const float *mm)
        {
            memcpy(m, mm, sizeof(m));
        }

        matrix4_t::matrix4_t(const double *mm)
        {
            m[0] = mm[0];
            m[1] = mm[1];
            m[2] = mm[2];
            m[3] = mm[3];

            m[4] = mm[4];
            m[5] = mm[5];
            m[6] = mm[6];
            m[7] = mm[7];

            m[8] = mm[8];
            m[9] = mm[9];
            m[10] = mm[10];
            m[11] = mm[11];

            m[12] = mm[12];
            m[13] = mm[13];
            m[14] = mm[14];
            m[15] = mm[15];
        }
    }
}
