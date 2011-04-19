#include "glwidget.h"

#include <QPainter>

#define M_PI 3.1415926535897932
#define PHI_MIN1	0.24f
#define PHI_MIN2	0.342f
float	PHI_MIN	  = PHI_MIN2/*0.0f*/;
#define PHI_MAX		0.55707f
#define PHI1_MIN	0.0f
#define PHI1_MAX	0.1378f
#define GAMMA_MIN	1.9326f
#define GAMMA_MAX	M_PI
#define PHI_STEPS	10
#define PHI1_STEPS	15
#define GAMMA_STEPS 5
#define Rb			77.83f
#define	a			290.28f
#define PSI			0.09467f
#define TOP_STEPS	3
#define N_TEETH		26

#define f			83.225f
#define ro_ao		3.096f

#define Radians2Degrees(x) ((x)/M_PI*180.0f)

typedef struct {
        GLdouble x;
        GLdouble y;
        GLdouble z;
} Vertex3D;

typedef Vertex3D Vector3D;

GLfloat Rdelta = 2.2;

GLUquadric *quadric;

GLfloat lightAmbient[]=  { 0.5f, 0.5f, 0.5f, 1.0f };	// Ambient Light Values
GLfloat lightDiffuse[]=	 { 1.0f, 1.0f, 1.0f, 1.0f };	// Diffuse Light Values
GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// Specular Light Values
GLfloat lightPosition[]= { 0.0f, 200.0f, 500.0f, 1.0f };	// Light Position

GLfloat matSpecular[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat matDiffuse[] = { 0.0, 1.0, 0.0, 1.0 };
GLfloat matAmbient[] = { 0.1, 0.1, 0.1, 1.0 };
GLfloat matShininess[] = { 100.0 };

int isWireframe = 1;
int isTooth = 1;
int isBgWireframe = 0;

const GLfloat identity[] = { 1, 0, 0, 0,
                             0, 1, 0, 0,
                             0, 0, 1, 0,
                             0, 0, 0, 1 };

void init(void);
void display(void);
void reshape(int, int);
void mouse(int, int, int, int);
void keyboard(unsigned char, int, int);
void arrow_keys(int, int, int);
void drawEvolvent(int);
void drawTransitionSurface(int);
void pick(GLint);

GLWidget::GLWidget(QWidget *parent) :
    QWidget(parent), lbut(false), rbut(false),
    zoom(4), tx(0), ty(0), rx(0), ry(0),
    waitForToothSelection(false), toothSelected(0), pickingMode(false),
    context(0), fbo(0)
{
    this->setMouseTracking(true);

    initializeGL();
}

GLWidget::~GLWidget()
{
    if (fbo) delete fbo;
    if (context) delete context;
}

void normalize(Vector3D *v)
{
        float length = sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);

        v->x /= length;
        v->y /= length;
        v->z /= length;
}

Vertex3D init_vertex(float x, float y, float z)
{
        Vertex3D ret;
        ret.x = x;
        ret.y = y;
        ret.z = z;

        return ret;
}

Vector3D point_sub(Vertex3D *l, Vertex3D *r)
{
        Vector3D ret;
        ret.x = l->x - r->x;
        ret.y = l->y - r->y;
        ret.z = l->z - r->z;

        return ret;
}

Vertex3D point_add_vector(Vertex3D *l, Vector3D *r)
{
        Vertex3D ret;
        ret.x = l->x + r->x;
        ret.y = l->y + r->y;
        ret.z = l->z + r->z;

        return ret;
}

Vector3D vector_mul(Vector3D *l, float r)
{
        Vector3D ret;
        ret.x = l->x * r;
        ret.y = l->y * r;
        ret.z = l->z * r;

        return ret;
}

void transform_vertex(Vertex3D v, GLdouble m[], Vertex3D &o)
{
    o.x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
    o.y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
    o.z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
}

#define PHI_I(i) (i) * (PHI_MAX - PHI_MIN) / PHI_STEPS + PHI_MIN
#define PHI1_I(i) (i) * (PHI1_MAX - PHI1_MIN) / PHI1_STEPS + PHI1_MIN
#define GAMMA_I(i) (i) * (GAMMA_MAX - GAMMA_MIN) / GAMMA_STEPS + GAMMA_MIN
#define R_EVOL_IJ(i, j) r_evolventa(PHI_I(i), PHI1_I(j))
#define R_EVOLN_IJ(i, j) r_evolventa_normal(PHI_I(i), PHI1_I(j))
#define L_EVOL_IJ(i, j) l_evolventa(PHI_I(i), PHI1_I(j))
#define L_EVOLN_IJ(i, j) l_evolventa_normal(PHI_I(i), PHI1_I(j))
#define R_TRANS_IJ(i, j) r_transition_surface(GAMMA_I(i), PHI1_I(j))
#define R_TRANSN_IJ(i, j) r_transition_surface_normal(GAMMA_I(i), PHI1_I(j))
#define L_TRANS_IJ(i, j) l_transition_surface(GAMMA_I(i), PHI1_I(j))
#define L_TRANSN_IJ(i, j) l_transition_surface_normal(GAMMA_I(i), PHI1_I(j))

Vertex3D r_evolventa(float phi, float phi1)
{
        Vertex3D ret;
        ret.x = Rb*((sinf(phi) - phi * cosf(phi)) * cosf(phi1) + (cosf(phi) + phi * sinf(phi)) * sinf(phi1));
        ret.y = -Rb*((sinf(phi) - phi * cosf(phi)) * sinf(phi1) - (cosf(phi) + phi * sinf(phi)) * cosf(phi1));
        ret.z = -a * phi1;

        return ret;
}

Vertex3D l_evolventa(float phi, float phi1)
{
        Vertex3D ret;
        ret.x = -Rb*((sinf(phi) - phi * cosf(phi)) * cosf(phi1) - (cosf(phi) + phi * sinf(phi)) * sinf(phi1));
        ret.y = Rb*((sinf(phi) - phi * cosf(phi)) * sinf(phi1) + (cosf(phi) + phi * sinf(phi)) * cosf(phi1));
        ret.z = -a * phi1;

        return ret;
}

Vector3D r_evolventa_normal(float phi, float phi1)
{
        Vector3D ret;
        float dxu = Rb*((cosf(phi) - cosf(phi) + phi * sinf(phi)) * cosf(phi1) + (-sinf(phi) + sinf(phi) + phi * cosf(phi)) * sinf(phi1));
        float dxv = Rb*(-(sinf(phi) - phi * cosf(phi)) * sinf(phi1) + (cosf(phi) + phi * sinf(phi)) * cosf(phi1));
        float dyu = -Rb*((cosf(phi) - cosf(phi) + phi * sinf(phi)) * sinf(phi1) - (-sinf(phi) + sinf(phi) + phi * cosf(phi)) * cosf(phi1));
        float dyv = -Rb*((sinf(phi) - phi * cosf(phi)) * cosf(phi1) + (cosf(phi) + phi * sinf(phi)) * sinf(phi1));
        float dzu = 0;
        float dzv = -a;

        ret.x = dxu * dyv - dxv * dyu;
        ret.y = dyu * dzv - dyv * dzu;
        ret.z = dzu * dxv - dzv * dxu;

        normalize(&ret);

        return ret;
}

Vector3D l_evolventa_normal(float phi, float phi1)
{
        Vector3D ret;
        float dxu = -Rb*((cosf(phi) - cosf(phi) + phi * sinf(phi)) * cosf(phi1) - (-sinf(phi) + sinf(phi) + phi * cosf(phi)) * sinf(phi1));
        float dxv = -Rb*(-(sinf(phi) - phi * cosf(phi)) * sinf(phi1) - (cosf(phi) + phi * sinf(phi)) * cosf(phi1));
        float dyu = Rb*((cosf(phi) - cosf(phi) + phi * sinf(phi)) * sinf(phi1) + (-sinf(phi) + sinf(phi) + phi * cosf(phi)) * cosf(phi1));
        float dyv = Rb*((sinf(phi) - phi * cosf(phi)) * cosf(phi1) - (cosf(phi) + phi * sinf(phi)) * sinf(phi1));
        float dzu = 0;
        float dzv = -a;

        ret.x = dxu * dyv - dxv * dyu;
        ret.y = dyu * dzv - dyv * dzu;
        ret.z = dzu * dxv - dzv * dxu;

        normalize(&ret);

        return ret;
}

Vertex3D r_transition_surface(float gamma, float phi1)
{
        Vertex3D ret;
        ret.x = f * sinf(phi1) + ro_ao * sinf(gamma + phi1);
        ret.y = f * cosf(phi1) + ro_ao * cosf(gamma + phi1);
        ret.z = -a * phi1;

        return ret;
}

Vector3D r_transition_surface_normal(float gamma, float phi1) {
        Vector3D ret;

        float dxu = ro_ao * cosf(gamma + phi1);
        float dxv = f * cosf(phi1) + ro_ao * cosf(gamma + phi1);
        float dyu = -ro_ao * sinf(gamma + phi1);
        float dyv = -f * sinf(phi1) - ro_ao * sinf(gamma + phi1);
        float dzu = 0;
        float dzv = -a;

        ret.x = dxu * dyv - dxv * dyu;
        ret.y = dyu * dzv - dyv * dzu;
        ret.z = dzu * dxv - dzv * dxu;

        normalize(&ret);

        return ret;
}

Vertex3D l_transition_surface(float gamma, float phi1)
{
        Vertex3D ret;
        ret.x = f * sinf(phi1) + ro_ao * sinf(-gamma + phi1);
        ret.y = f * cosf(phi1) + ro_ao * cosf(-gamma + phi1);
        ret.z = -a * phi1;

        return ret;
}

Vector3D l_transition_surface_normal(float gamma, float phi1) {
        Vector3D ret;

        float dxu = -ro_ao * cosf(-gamma + phi1);
        float dxv = f * cosf(phi1) + ro_ao * cosf(-gamma + phi1);
        float dyu = ro_ao * sinf(-gamma + phi1);
        float dyv = -f * sinf(phi1) - ro_ao * sinf(-gamma + phi1);
        float dzu = 0;
        float dzv = -a;

        ret.x = dxu * dyv - dxv * dyu;
        ret.y = dyu * dzv - dyv * dzu;
        ret.z = dzu * dxv - dzv * dxu;

        normalize(&ret);

        return ret;
}

/* Draw axes */

void GLWidget::drawAxes(void)
{
    /* Name-stack manipulation for the purpose of
         selection hit processing when mouse button
         is pressed.  Names are ignored in normal
         OpenGL rendering mode.                    */

        glPushMatrix();
        /* No name for grey sphere */

        glScalef(10.0f, 10.0f, 10.0f);

        glColor3f(0.3,0.3,0.3);
        gluSphere(quadric, 0.7, 20, 20);

        //glutSolidSphere(0.7, 20, 20);

        glPushMatrix();
        glColor3f(1,0,0);
        glRotatef(90,0,1,0);
        gluCylinder(quadric, 0.6, 0, 4.0, 20, 20);
        //glutSolidCone(0.6, 4.0, 20, 20);
        glPopMatrix();

        //this->renderText(4.5, 0, 0, "X");

        glPushMatrix();
        glColor3f(0,1,0);
        glRotatef(-90,1,0,0);
        gluCylinder(quadric, 0.6, 0, 4.0, 20, 20);
        //glutSolidCone(0.6, 4.0, 20, 20);
        glPopMatrix();

        //this->renderText(0, 4.5, 0, "Y");

        glColor3f(0,0,1);
        gluCylinder(quadric, 0.6, 0, 4.0, 20, 20);
        //glutSolidCone(0.6, 4.0, 20, 20);

        //this->renderText(0, 0, 4.5, "Z");

        glPopMatrix();
}

void drawEvolvent(int r)
{
        Vertex3D v;
        Vector3D n;
        int i, j;

        glPushName(1);

        if (r) {
                glBegin(GL_QUADS);
                for (i = 0; i < PHI_STEPS; ++i) {
                        for (j = 0; j < PHI1_STEPS; ++j) {
                                n = R_EVOLN_IJ(i, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_EVOL_IJ(i, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = R_EVOLN_IJ(i+1, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_EVOL_IJ(i+1, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = R_EVOLN_IJ(i+1, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_EVOL_IJ(i+1, j+1);
                                glVertex3f(v.x, v.y, v.z);

                                n = R_EVOLN_IJ(i, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_EVOL_IJ(i, j+1);
                                glVertex3f(v.x, v.y, v.z);
                        }
                }
                glEnd();
        } else {
                glBegin(GL_QUADS);
                for (i = 0; i < PHI_STEPS; ++i) {
                        for (j = 0; j < PHI1_STEPS; ++j) {
                                n = L_EVOLN_IJ(i, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_EVOL_IJ(i, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = L_EVOLN_IJ(i+1, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_EVOL_IJ(i+1, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = L_EVOLN_IJ(i+1, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_EVOL_IJ(i+1, j+1);
                                glVertex3f(v.x, v.y, v.z);

                                n = L_EVOLN_IJ(i, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_EVOL_IJ(i, j+1);
                                glVertex3f(v.x, v.y, v.z);
                        }
                }
                glEnd();
        }

        glPopName();
}

void drawTooth()
{
        int i, j;
        Vertex3D vl, vr, vx;
        Vector3D v;

        glPushMatrix();
        glRotatef(Radians2Degrees(PSI), 0.0f, 0.0f, 1.0f);
        drawEvolvent(1);	// draw left side surface of the tooth
        glPopMatrix();

        glPushMatrix();
        glRotatef(Radians2Degrees(-PSI), 0.0f, 0.0f, 1.0f);
        drawEvolvent(0);	// draw right side surface of the tooth
        glPopMatrix();

        // draw tooth top surface
        for (i = 0; i < TOP_STEPS; ++i) {
                glBegin(GL_QUAD_STRIP);
                for (j = 0; j <= PHI1_STEPS; ++j) {
                        vl = l_evolventa(PHI_MAX, PHI1_I(j));
                        vr = r_evolventa(PHI_MAX, PHI1_I(j));

                        vl = init_vertex(vl.x*cosf(PSI) + vl.y*sinf(PSI), -vl.x*sinf(PSI) + vl.y*cosf(PSI), vl.z);
                        vr = init_vertex(vr.x*cosf(PSI) - vr.y*sinf(PSI), vr.x*sinf(PSI) + vr.y*cosf(PSI), vr.z);

                        v = point_sub(&vl, &vr);
                        v = vector_mul(&v, (float)i / TOP_STEPS);
                        vx = point_add_vector(&vr, &v);

                        glNormal3f(0.0f, -1.0f, 0.0f);
                        glVertex3f(vx.x, vx.y, vx.z);

                        v = point_sub(&vl, &vr);
                        v = vector_mul(&v, (float)(i + 1) / TOP_STEPS);
                        vx = point_add_vector(&vr, &v);

                        glNormal3f(0.0f, -1.0f, 0.0f);
                        glVertex3f(vx.x, vx.y, vx.z);
                }
                glEnd();
        }

        // draw tooth front and back surfaces
        /*
        for (i = 0; i < TOP_STEPS; ++i) {
                glBegin(GL_QUAD_STRIP);
                for (j = 0; j <= PHI_STEPS; ++j) {
                        vl = l_evolventa(PHI_I(j), PHI1_I(0));
                        vr = r_evolventa(PHI_I(j), PHI1_I(0));

                        vl = init_vertex(vl.x*cosf(PSI) + vl.y*sinf(PSI), -vl.x*sinf(PSI) + vl.y*cosf(PSI), vl.z);
                        vr = init_vertex(vr.x*cosf(PSI) - vr.y*sinf(PSI), vr.x*sinf(PSI) + vr.y*cosf(PSI), vr.z);

                        v = point_sub(&vl, &vr);
                        v = vector_mul(&v, (float)i / TOP_STEPS);
                        vx = point_add_vector(&vr, &v);

                        glNormal3f(0.0f, 0.0f, -1.0f);
                        glVertex3f(vx.x, vx.y, vx.z);

                        v = point_sub(&vl, &vr);
                        v = vector_mul(&v, (float)(i + 1) / TOP_STEPS);
                        vx = point_add_vector(&vr, &v);

                        glNormal3f(0.0f, 0.0f, -1.0f);
                        glVertex3f(vx.x, vx.y, vx.z);
                }
                glEnd();
        }

        for (i = 0; i < TOP_STEPS; ++i) {
                glBegin(GL_QUAD_STRIP);
                for (j = 0; j <= PHI_STEPS; ++j) {
                        vl = l_evolventa(PHI_I(j), PHI1_MAX);
                        vr = r_evolventa(PHI_I(j), PHI1_MAX);

                        vl = init_vertex(vl.x*cosf(PSI) + vl.y*sinf(PSI), -vl.x*sinf(PSI) + vl.y*cosf(PSI), vl.z);
                        vr = init_vertex(vr.x*cosf(PSI) - vr.y*sinf(PSI), vr.x*sinf(PSI) + vr.y*cosf(PSI), vr.z);

                        v = point_sub(&vl, &vr);
                        v = vector_mul(&v, (float)i / TOP_STEPS);
                        vx = point_add_vector(&vr, &v);

                        glNormal3f(0.0f, 0.0f, 1.0f);
                        glVertex3f(vx.x, vx.y, vx.z);

                        v = point_sub(&vl, &vr);
                        v = vector_mul(&v, (float)(i + 1) / TOP_STEPS);
                        vx = point_add_vector(&vr, &v);

                        glNormal3f(0.0f, 0.0f, 1.0f);
                        glVertex3f(vx.x, vx.y, vx.z);
                }
                glEnd();
        }
        */

        glPushMatrix();
        glRotatef(Radians2Degrees(PSI + 0.024), 0.0f, 0.0f, 1.0f);
        drawTransitionSurface(1);
        glPopMatrix();

        glPushMatrix();
        glRotatef(Radians2Degrees(-PSI - 0.024), 0.0f, 0.0f, 1.0f);
        drawTransitionSurface(0);
        glPopMatrix();
}

void drawToothTriangles()
{
        PHI_MIN = PHI_MIN1;
        glPushMatrix();
        drawEvolvent(1);	// draw left side surface of the tooth
        glPopMatrix();

        PHI_MIN = PHI_MIN2;
        glPushMatrix();
        glRotatef(Radians2Degrees(-PSI), 0.0f, 0.0f, 1.0f);
        drawEvolvent(0);	// draw right side surface of the tooth
        glPopMatrix();

        glPushMatrix();
        glRotatef(Radians2Degrees(PSI), 0.0f, 0.0f, 1.0f);
        drawEvolvent(1);	// draw left side surface of the tooth
        glPopMatrix();

        PHI_MIN = PHI_MIN1;
        glPushMatrix();
        drawEvolvent(0);	// draw right side surface of the tooth
        glPopMatrix();

        PHI_MIN = PHI_MIN2;

        glPushMatrix();
        glRotatef(Radians2Degrees(PSI + 0.024), 0.0f, 0.0f, 1.0f);
        drawTransitionSurface(1);
        glPopMatrix();

        glPushMatrix();
        glRotatef(Radians2Degrees(-PSI - 0.024), 0.0f, 0.0f, 1.0f);
        drawTransitionSurface(0);
        glPopMatrix();
}

void drawTransitionSurface(int r)
{
        Vertex3D v;
        Vector3D n;
        int i, j;

        if (r) {
                glBegin(GL_QUADS);
                for (i = 0; i < GAMMA_STEPS; ++i) {
                        for (j = 0; j < PHI1_STEPS; ++j) {
                                n = R_TRANSN_IJ(i, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_TRANS_IJ(i, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = R_TRANSN_IJ(i+1, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_TRANS_IJ(i+1, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = R_TRANSN_IJ(i+1, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_TRANS_IJ(i+1, j+1);
                                glVertex3f(v.x, v.y, v.z);

                                n = R_TRANSN_IJ(i, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = R_TRANS_IJ(i, j+1);
                                glVertex3f(v.x, v.y, v.z);
                        }
                }
                glEnd();
        } else {
                glBegin(GL_QUADS);
                for (i = 0; i < GAMMA_STEPS; ++i) {
                        for (j = 0; j < PHI1_STEPS; ++j) {
                                n = L_TRANSN_IJ(i, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_TRANS_IJ(i, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = L_TRANSN_IJ(i+1, j);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_TRANS_IJ(i+1, j);
                                glVertex3f(v.x, v.y, v.z);

                                n = L_TRANSN_IJ(i+1, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_TRANS_IJ(i+1, j+1);
                                glVertex3f(v.x, v.y, v.z);

                                n = L_TRANSN_IJ(i, j+1);
                                glNormal3f(n.x, n.y, n.z);
                                v = L_TRANS_IJ(i, j+1);
                                glVertex3f(v.x, v.y, v.z);
                        }
                }
                glEnd();
        }
}


void keyboard(unsigned char key, int x, int y)  // Create Keyboard Function
{
        switch (key) {
                case 27:        // When Escape Is Pressed...
                        exit(0);   // Exit The Program
                        break;        // Ready For Next Case
                case 'W':
                case 'w':
                        isWireframe = !isWireframe;
                        break;
                case 'C':
                case 'c':
                        isTooth = !isTooth;
                        PHI_MIN = PHI_MIN2;
                        break;
                default:        // Now Wrap It Up
                        break;
        }
}

void GLWidget::initializeGL()
{
    context = new QGLContext(QGLFormat::defaultFormat(), this);
    context->create();
    context->makeCurrent();

    glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
    glClearDepth(1.0f);									// Depth Buffer Setup

    //glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
    glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
    glEnable(GL_COLOR_MATERIAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    quadric = gluNewQuadric();			// Create A Pointer To The Quadric Object ( NEW )
    gluQuadricNormals(quadric, GLU_SMOOTH);	// Create Smooth Normals ( NEW )
    gluQuadricTexture(quadric, GL_TRUE);		// Create Texture Coords ( NEW )

    memcpy(rotAccum, identity, sizeof(GLfloat) * 16);
}

void GLWidget::resizeGL(int width, int height)
{
    context->makeCurrent();

    /* window reshape function */
    if (height==0)										// Prevent A Divide By Zero By
    {
            height=1;										// Making Height Equal One
    }

    if (fbo) delete fbo;
    fbo = new QGLFramebufferObject(width, height, QGLFramebufferObject::Depth);
    fbo->bind();

    glViewport(0,0,width,height);						// Reset The Current Viewport

    glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
    glLoadIdentity();                                                         // Reset The Projection Matrix

    // Calculate The Aspect Ratio Of The Window
    //gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,1000.0f);
    glOrtho(-width/2, width/2, -height/2, height/2, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
    glLoadIdentity();
}

void GLWidget::paintGL()
{
    int i, k;

    context->makeCurrent();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

    if (isBgWireframe && !pickingMode) {
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);

        glColor3f(0.8f, 0.8f, 0.8f);

        glBegin(GL_LINES);

        GLint viewport[4];					// Where The Viewport Values Will Be Stored
        glGetIntegerv(GL_VIEWPORT, viewport);			// Retrieves The Viewport Values (X, Y, Width, Height)
        int w2 = width()/2;
        int h2 = height()/2;

        for (float x = -w2; x < w2; x += width()/60.0)
        {
            glVertex2f(x, -h2);
            glVertex2f(x, h2);
        }

        for (float y = -h2; y < h2; y += height()/40.0)
        {
            glVertex2f(-w2, y);
            glVertex2f(w2, y);
        }

        glEnd();

        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

    glLoadIdentity();

    glTranslatef(tx, ty - Rb*zoom, 0);
    glScalef(zoom, zoom, zoom);

    glRotatef(-ry, 1.0f, 0.0f, 0.0f);
    glRotatef(rx, 0.0f, 1.0f, 0.0f);
    glMultMatrixf(rotAccum);

    glDisable(GL_LIGHTING);
    if (!pickingMode)
    {
        drawAxes();
        glEnable(GL_LIGHTING);
    }

    for (k = 0; k < ((isWireframe && !pickingMode) ? 2 : 1); ++k) {
            if (k == 0) {
                    glPolygonOffset(1.0, 1.0);
                    glEnable(GL_POLYGON_OFFSET_FILL);
            }
            else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            if (!pickingMode) // draw main gear cylinder
            {
                glPushMatrix();
                glTranslatef(0.0f, 0.0f, -a * PHI1_MAX);
                if (k == 0)
                        glColor3f(0.0f, 0.0f, 1.0f);
                else
                        glColor3f(1.0f, 1.0f, 1.0f);

                gluCylinder(quadric, Rb + Rdelta, Rb + Rdelta, a * PHI1_MAX, 60, 6);
                glPopMatrix();
            }

            // draw evolvent
            if (!pickingMode)
            {
                if (k == 0) {
                        glColor3f(0.5f, 0.0f, 0.0f);
                } else
                        glColor3f(1.0f, 1.0f, 1.0f);
            }

            if (isTooth) {
                    for (i = 0; i < N_TEETH; ++i) {
                            glPushMatrix();
                            glRotatef(i * 360.0f / N_TEETH, 0.0f, 0.0f, 1.0f);

                            if (pickingMode)
                                glColor3ub(i+1, 0, 0);
                            else if (waitForToothSelection && k == 0)
                            {
                                if (toothSelected == i+1)
                                    glColor3f(1.0f, 0.5f, 0.5f);    // Hilite selected tooth
                                else
                                    glColor3f(0.5f, 0.0f, 0.0f);
                            }

                            drawTooth();
                            glPopMatrix();
                    }
            } else {
                    drawToothTriangles();
            }

            if (k == 0) {
                    glDisable(GL_POLYGON_OFFSET_FILL);
            }
            else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
    }

    context->swapBuffers();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!waitForToothSelection)
    {
        int dx = prevPos.x() - event->pos().x();
        int dy = prevPos.y() - event->pos().y();

        context->makeCurrent();

        if (lbut) {
            rx += dx;
            ry += dy;
        }

        if (rbut) {
            tx -= dx;
            ty += dy;
        }

        if (lbut || rbut) {
            this->updateGL();
            prevPos = event->pos();
        }
    } else
    {
        context->makeCurrent();
        pickingMode = true;
        paintGL();
        pickingMode = false;

        // read from back buffer
        GLint viewport[4];
        GLubyte pixel[3];

        glGetIntegerv(GL_VIEWPORT, viewport);

        glReadPixels(event->pos().x(), viewport[3]-event->pos().y(), 1, 1,
                GL_RGB, GL_UNSIGNED_BYTE, (void *)pixel);

        int prevToothSelected = toothSelected;
        toothSelected = pixel[0];

        if (prevToothSelected != toothSelected)
            updateGL();
    }
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (!waitForToothSelection)
    {
        prevPos = event->pos();
        if (event->button() == Qt::LeftButton)
            lbut = true;

        if (event->button() == Qt::RightButton)
            rbut = true;
    } else
    {
        if (event->button() == Qt::LeftButton)
        {
            if (toothSelected != 0)
                zoomTo(toothSelected);

            waitForToothSelection = false;
        }
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && lbut) {
        lbut = false;

        context->makeCurrent();

        glLoadIdentity();

        glRotatef(-ry, 1.0f, 0.0f, 0.0f);
        glRotatef(rx, 0.0f, 1.0f, 0.0f);
        glMultMatrixf(rotAccum);

        glGetFloatv(GL_MODELVIEW_MATRIX, rotAccum);

        rx = ry = 0;
    }

    if (event->button() == Qt::RightButton)
        rbut = false;
}

void GLWidget::zoomIn()
{
    zoom *= 1.4;
    updateGL();
}

void GLWidget::zoomOut()
{
    zoom /= 1.4;
    updateGL();
}

void GLWidget::restoreView()
{
    context->makeCurrent();

    glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
    glLoadIdentity();                                                         // Reset The Projection Matrix
    glOrtho(-width()/2, width()/2, -height()/2, height()/2, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
    glLoadIdentity();

    memcpy(rotAccum, identity, sizeof(GLfloat) * 16);
    rx = 0; ry = 0;
    tx = 0; ty = 0;
    zoom = 4;
    updateGL();
}

void GLWidget::toggleTooth()
{
    isTooth = !isTooth;
    updateGL();
}

void GLWidget::toggleWireframe()
{
    isWireframe = !isWireframe;
    updateGL();
}

void GLWidget::toggleBgWireframe()
{
    isBgWireframe = !isBgWireframe;
    updateGL();
}

void GLWidget::zoomToTooth()
{
    if (isTooth)
        waitForToothSelection = true;
    else
        zoomTo(1);
}

void GLWidget::zoomTo(int toothNum)
{
    if (toothNum > 0 && toothNum <= N_TEETH)
    {
        context->makeCurrent();

        glLoadIdentity();

        glTranslatef(tx, ty - Rb*zoom, 0);
        glScalef(zoom, zoom, zoom);

        glRotatef(-ry, 1.0f, 0.0f, 0.0f);
        glRotatef(rx, 0.0f, 1.0f, 0.0f);
        glMultMatrixf(rotAccum);

        glRotatef((toothNum-1) * 360.0f / N_TEETH, 0.0f, 0.0f, 1.0f);

        // ---

        glPushMatrix();
        glRotatef(Radians2Degrees(PSI), 0.0f, 0.0f, 1.0f);

        GLdouble mm1[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, mm1);

        glPopMatrix();

        // ----

        glPushMatrix();
        glRotatef(Radians2Degrees(-PSI), 0.0f, 0.0f, 1.0f);

        GLdouble mm2[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, mm2);

        glPopMatrix();

        // ----
        Vertex3D v1 = R_EVOL_IJ(0, 0);
        Vertex3D v2 = R_EVOL_IJ(0, PHI1_STEPS-1);

        Vertex3D v3 = L_EVOL_IJ(0, 0);
        Vertex3D v4 = L_EVOL_IJ(0, PHI1_STEPS-1);

        Vertex3D v1t, v2t, v3t, v4t;
        transform_vertex(v1, mm1, v1t);
        transform_vertex(v2, mm1, v2t);
        transform_vertex(v3, mm2, v3t);
        transform_vertex(v4, mm2, v4t);

        float r1 = sqrtf((v1t.x - v4t.x) * (v1t.x - v4t.x) + (v1t.y - v4t.y) * (v1t.y - v4t.y)/* + (v1t.z - v4t.z) * (v1t.z - v4t.z)*/);
        float r2 = sqrtf((v2t.x - v3t.x) * (v2t.x - v3t.x) + (v2t.y - v3t.y) * (v2t.y - v3t.y)/* + (v2t.z - v3t.z) * (v2t.z - v3t.z)*/);

        float diam = (r1 > r2) ? r1 : r2;

        float cx = (v2t.x + v3t.x) / 2;
        float cy = (v2t.y + v3t.y) / 2;

        GLdouble left = cx - diam;
        GLdouble right = cx + diam;
        GLdouble bottom = cy - diam;
        GLdouble top = cy + diam;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(left, right, bottom, top, -1000, 1000);
        glMatrixMode(GL_MODELVIEW);

        updateGL();
    }
}

void GLWidget::resizeEvent(QResizeEvent *event)
{
    resizeGL(width(), height());
}

void GLWidget::updateGL()
{
    update();
}

void GLWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (fbo) {
        paintGL();
        QImage img = fbo->toImage();
        painter.drawImage(0, 0, img);
    }
}
