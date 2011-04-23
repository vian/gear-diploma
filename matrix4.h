#ifndef MATRIX4_H
#define MATRIX4_H

namespace vian {
    namespace geom {
        class matrix4_t
        {
        public:
            union {
                struct {
                    float _11, _12, _13, _14;
                    float _21, _22, _23, _24;
                    float _31, _32, _33, _34;
                    float _41, _42, _43, _44;
                };
                float m[16];
            };

            matrix4_t() : _11(1), _12(0), _13(0), _14(0),
                          _21(0), _22(1), _23(0), _24(0),
                          _31(0), _32(0), _33(1), _34(0),
                          _41(0), _42(0), _43(0), _44(1) {};
            matrix4_t(const float *mm);
            matrix4_t(const double *mm);
        };

    }
}

#endif // MATRIX4_H
