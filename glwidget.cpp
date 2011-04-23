#include "glwidget.h"
#include "vector3d.h"
#include "matrix4.h"

#include <QPainter>

using namespace vian::geom;

#define Radians2Degrees(x) ((x)/M_PI*180.0f)
#define Degrees2Radians(x) ((x)/180.0f*M_PI)

#define ARRAY_DIM(x) (sizeof(x)/sizeof((x)[0]))

// common constants
const float m = 8;
const float beta = Degrees2Radians(16);
const float alpha = Degrees2Radians(20);
const float num_teeth = 20;
const float c_star = 0.25f;
const float ha_star = 1;
const float b = 40;
const float d = m * num_teeth / cosf(beta);
const float alpha_t = atanf(tanf(alpha) / cosf(beta));
const float Rb = d * cosf(alpha_t) / 2;
const float ro_f = c_star * m / (1.0f - sinf(alpha_t));
const float x = c_star + ha_star - ro_f / m;
const float Ra = d / 2 + (ha_star + x) * m;
const float Rf = d / 2 - (ha_star + c_star - x) * m;
const float St = (M_PI / 2 + 2 * x * tanf(alpha)) * m / cosf(beta);
const float phi1_max = 2 * b / d * tanf(beta);
const float a = d / (2 * tanf(beta));
const float alpha1 = tanf(alpha_t) - alpha_t;
const float alpha2 = St / d;
const float psi = alpha1 + alpha2;

// transition surface constants
const float alpha_tw = alpha_t;
const float LN = Rb * tanf(alpha_tw) - ro_f;
const float alpha_L = atanf(LN / Rb);
const float R_L = Rb / cosf(alpha_L);
const float phi_Lstar = sqrtf((R_L / Rb) * (R_L / Rb) - 1);
const float psi1 = alpha_tw - phi_Lstar;
const float psi2 = psi + psi1;
const float f = d / 2;
const float gamma0 = M_PI_2 + alpha_t;
const float gamma_max = M_PI;

const float max_i = 15;
const float max_j = 10;

const float max_cap_i = 5;
const float max_trans_i = 5;

// geometry data constants
const int sideSurfaceNumPoints = (max_i + 1) * (max_j + 1);
const int transitionSurfaceNumPoints = (max_trans_i + 1) * (max_j + 1);
const int capSurfaceNumPoints = (max_cap_i + 1) * (max_j + 1);
const int totalNumPoints = 2 * sideSurfaceNumPoints + 2 * transitionSurfaceNumPoints + capSurfaceNumPoints;

const int sideSurfaceNumQuads = max_i * max_j;
const int transitionSurfaceNumQuads = max_trans_i * max_j;
const int capSurfaceNumQuads = max_cap_i * max_j;
const int totalNumQuads = 2 * sideSurfaceNumQuads + 2 * transitionSurfaceNumQuads + capSurfaceNumQuads;

static vector3d side_surface_point(int i, int j, bool isLeft)
{
    //float R = Rb + (Ra - Rb) * i / max_i;
    float R = R_L + (Ra - R_L) * i / max_i;
    float RdivRb = R / Rb;
    float phi = sqrtf(RdivRb * RdivRb - 1);
    float phi1 = phi1_max * j / max_j;

    float lambda;
    vector3d res;
    if (isLeft) {
        lambda = phi + phi1 - psi;
        res = vector3d(Rb * (sinf(lambda) - phi * cosf(lambda)),
                       Rb * (cosf(lambda) - phi * sinf(lambda)),
                       -a * phi1);
    }
    else {
        lambda = phi - phi1 - psi;
        res = vector3d(-Rb * (sinf(lambda) - phi * cosf(lambda)),
                       Rb * (cosf(lambda) - phi * sinf(lambda)),
                       -a * phi1);
    }

    return res;
}

static vector3d side_surface_normal(int i, int j, bool isLeft)
{
    //float R = Rb + (Ra - Rb) * i / max_i;
    float R = R_L + (Ra - R_L) * i / max_i;
    float RdivRb = R / Rb;
    float phi = sqrtf(RdivRb * RdivRb - 1);
    float phi1 = phi1_max * j / max_j;

    float lambda;
    vector3d dV_dphi;
    vector3d dV_dphi1;

    if (isLeft) {
        lambda = phi + phi1 - psi;
        dV_dphi = vector3d(Rb * (cosf(lambda) + phi * sinf(lambda) - cosf(lambda)),
                           Rb * (-sinf(lambda) - phi * cosf(lambda) - sinf(lambda)),
                           0);
        dV_dphi1 = vector3d(Rb * (cosf(lambda) + phi * sinf(lambda)),
                            Rb * (-sinf(lambda) - phi * cosf(lambda)),
                            -a);
    }
    else {
        lambda = phi - phi1 - psi;
        dV_dphi = vector3d(-Rb * (cosf(lambda) + phi * sinf(lambda - cosf(lambda))),
                           Rb * (-sinf(lambda) - phi * cosf(lambda) - sinf(lambda)),
                           0);
        dV_dphi1 = vector3d(-Rb * (-cosf(lambda) - phi * sinf(lambda)),
                            Rb * (sinf(lambda) + phi * cosf(lambda)),
                            -a);
    }

    vector3d res = vector3d::cross(dV_dphi, dV_dphi1);
    res.normalize();

    return isLeft ? res : -res;
}

static vector3d transition_surface_point(int i, int j, bool isLeft)
{
    float phi1 = phi1_max * j / max_j;
    float gamma = (gamma_max - gamma0) * i / max_trans_i;
    float lambda2 = phi1 - psi2;
    float lambda3 = gamma + phi1 - psi2;
    float lambda4 = phi1 + psi2;
    float lambda5 = gamma - phi1 - psi2;

    vector3d res;
    if (isLeft) {
        res = vector3d(f * (sinf(lambda2) + ro_f * sinf(lambda3)),
                       f * (cosf(lambda2) + ro_f * cosf(lambda3)),
                       -a * phi1);
    }
    else {
        res = vector3d(f * (sinf(lambda4) - ro_f * sinf(lambda5)),
                       f * (cosf(lambda4) + ro_f * cosf(lambda5)),
                       -a * phi1);
    }

    return res;
}

static vector3d transition_surface_normal(int i, int j, bool isLeft)
{
    float phi1 = phi1_max * j / max_j;
    float gamma = (gamma_max - gamma0) * i / max_trans_i;
    float lambda2 = phi1 - psi2;
    float lambda3 = gamma + phi1 - psi2;
    float lambda4 = phi1 + psi2;
    float lambda5 = gamma - phi1 - psi2;

    vector3d dV_dgamma;
    vector3d dV_dphi1;
    if (isLeft) {
        dV_dgamma = vector3d(f * ro_f * cosf(lambda3),
                             -f * ro_f * sinf(lambda3),
                             0);
        dV_dphi1 = vector3d(f * (cosf(lambda2) + ro_f * cosf(lambda3)),
                            f * (-sinf(lambda2) - ro_f * sinf(lambda3)),
                            -a);
    }
    else {
        dV_dgamma = vector3d(-f * ro_f * cosf(lambda5),
                             -f * ro_f * sinf(lambda5),
                             0);
        dV_dphi1 = vector3d(f * (cosf(lambda4) + ro_f * cosf(lambda5)),
                            f * (-sinf(lambda4) + ro_f * sinf(lambda5)),
                            -a);
    }

    vector3d res =  vector3d::cross(dV_dgamma, dV_dphi1);
    res.normalize();
    return res;
}

static vector3d cap_surface_point(int i, int j)
{
    vector3d lpoint = side_surface_point(max_i, j, true);
    vector3d rpoint = side_surface_point(max_i, j, false);

    float lphi = acosf(lpoint.x / Ra);
    float rphi = acosf(rpoint.x / Ra);

    float phi = lphi + (rphi - lphi) * i / max_cap_i;

    return vector3d(Ra * cosf(phi), Ra * sinf(phi), lpoint.z);  // cylinder surface
}

static vector3d cap_surface_normal(int i, int j)
{
    vector3d res = cap_surface_point(i, j);
    res.z = 0;

    res.normalize();

    return res;
}

static void computeSideVerticesWithNormals(GLfloat *verticesWithNormals, bool isLeft)
{
    GLfloat *pf = verticesWithNormals;
    for (int i = 0; i <= max_i; ++i) {
        for (int j = 0; j <= max_j; ++j) {
            vector3d v = side_surface_point(i, j, isLeft);
            vector3d n = side_surface_normal(i, j, isLeft);
            pf[0] = v.x;
            pf[1] = v.y;
            pf[2] = v.z;
            pf[3] = n.x;
            pf[4] = n.y;
            pf[5] = n.z;
            pf += 6;
        }
    }
}

static void computeCapVerticesWithNormals(GLfloat *verticesWithNormals)
{
    GLfloat *pf = verticesWithNormals;
    for (int i = 0; i <= max_cap_i; ++i) {
        for (int j = 0; j <= max_j; ++j) {
            vector3d v = cap_surface_point(i, j);
            vector3d n = cap_surface_normal(i, j);
            pf[0] = v.x;
            pf[1] = v.y;
            pf[2] = v.z;
            pf[3] = n.x;
            pf[4] = n.y;
            pf[5] = n.z;
            pf += 6;
        }
    }
}

static void computeTransitionVerticesWithNormals(GLfloat *verticesWithNormals, bool isLeft)
{
    GLfloat *pf = verticesWithNormals;
    for (int i = 0; i <= max_trans_i; ++i) {
        for (int j = 0; j <= max_j; ++j) {
            vector3d v = transition_surface_point(i, j, isLeft);
            vector3d n = transition_surface_normal(i, j, isLeft);
            pf[0] = v.x;
            pf[1] = v.y;
            pf[2] = v.z;
            pf[3] = n.x;
            pf[4] = n.y;
            pf[5] = n.z;
            pf += 6;
        }
    }
}

static void computeQuadIndices(GLushort *indices, int _max_i, int _max_j, int offset = 0)
{
    GLushort *ind = indices;
    for (int i = 0; i < _max_i; ++i) {
        for (int j = 0; j < _max_j; ++j) {
            ind[0] = (i + 0) * (_max_j + 1) + j + 0 + offset;
            ind[1] = (i + 0) * (_max_j + 1) + j + 1 + offset;
            ind[2] = (i + 1) * (_max_j + 1) + j + 1 + offset;
            ind[3] = (i + 1) * (_max_j + 1) + j + 0 + offset;

            ind += 4;
        }
    }
}

GLUquadric *quadric;

int isWireframe = 1;
int isTooth = 1;
int isBgWireframe = 0;

const GLfloat identity[] = { 1, 0, 0, 0,
                             0, 1, 0, 0,
                             0, 0, 1, 0,
                             0, 0, 0, 1 };

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

/* Draw axes */
void GLWidget::drawAxes(void)
{
        glPushMatrix();

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

void GLWidget::drawTooth()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), toothData);
    glNormalPointer(GL_FLOAT, 6 * sizeof(GLfloat), toothData + 3);

    glDrawElements(GL_QUADS, 4 * totalNumQuads, GL_UNSIGNED_SHORT, toothIndices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
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

    GLfloat lightAmbient[]=  { 0.5f, 0.5f, 0.5f, 1.0f };	// Ambient Light Values
    GLfloat lightDiffuse[]=	 { 1.0f, 1.0f, 1.0f, 1.0f };	// Diffuse Light Values
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// Specular Light Values
    GLfloat lightPosition[]= { 0.0f, 200.0f, 500.0f, 1.0f };	// Light Position

    GLfloat matSpecular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat matDiffuse[] = { 0.0, 1.0, 0.0, 1.0 };
    GLfloat matAmbient[] = { 0.1, 0.1, 0.1, 1.0 };
    GLfloat matShininess[] = { 100.0 };

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

    // alloc and init tooth surface data
    const int vertexSize = 2 * 3;
    toothData = new GLfloat[totalNumPoints * vertexSize];
    GLfloat *fp = toothData;
    computeSideVerticesWithNormals(fp, true); fp += sideSurfaceNumPoints * vertexSize;
    computeSideVerticesWithNormals(fp, false); fp += sideSurfaceNumPoints * vertexSize;
    computeTransitionVerticesWithNormals(fp, true); fp += transitionSurfaceNumPoints * vertexSize;
    computeTransitionVerticesWithNormals(fp, false); fp += transitionSurfaceNumPoints * vertexSize;
    computeCapVerticesWithNormals(fp); fp = 0;

    // alloc and init tooth indices
    toothIndices = new GLushort[4 * totalNumQuads];

    int offset = 0;
    int offset2 = 0;
    computeQuadIndices(toothIndices + offset, max_i, max_j, offset2);
    offset += 4 * sideSurfaceNumQuads;
    offset2 += sideSurfaceNumPoints;
    computeQuadIndices(toothIndices + offset, max_i, max_j, offset2);
    offset += 4 * sideSurfaceNumQuads;
    offset2 += sideSurfaceNumPoints;
    computeQuadIndices(toothIndices + offset, max_trans_i, max_j, offset2);
    offset += 4 * transitionSurfaceNumQuads;
    offset2 += transitionSurfaceNumPoints;
    computeQuadIndices(toothIndices + offset, max_trans_i, max_j, offset2);
    offset += 4 * transitionSurfaceNumQuads;
    offset2 += transitionSurfaceNumPoints;
    computeQuadIndices(toothIndices + offset, max_cap_i, max_j, offset2);
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
                //glTranslatef(0.0f, 0.0f, -a * PHI1_MAX);
                if (k == 0)
                        glColor3f(0.0f, 0.0f, 1.0f);
                else
                        glColor3f(1.0f, 1.0f, 1.0f);

                //gluCylinder(quadric, Rb + Rdelta, Rb + Rdelta, a * PHI1_MAX, 60, 6);
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
                    for (i = 0; i < num_teeth; ++i) {
                            glPushMatrix();
                            glRotatef(i * 360.0f / num_teeth, 0.0f, 0.0f, 1.0f);

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
                    drawTooth();
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
    if (toothNum > 0 && toothNum <= num_teeth)
    {
        context->makeCurrent();

        glLoadIdentity();

        glTranslatef(tx, ty - Rb*zoom, 0);
        glScalef(zoom, zoom, zoom);

        glRotatef(-ry, 1.0f, 0.0f, 0.0f);
        glRotatef(rx, 0.0f, 1.0f, 0.0f);
        glMultMatrixf(rotAccum);

        glRotatef((toothNum-1) * 360.0f / num_teeth, 0.0f, 0.0f, 1.0f);

        // ---

        //glPushMatrix();
        //glRotatef(Radians2Degrees(PSI), 0.0f, 0.0f, 1.0f);

        GLdouble mm1[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, mm1);

        //glPopMatrix();

        // ----

        //glPushMatrix();
        //glRotatef(Radians2Degrees(-PSI), 0.0f, 0.0f, 1.0f);

        //GLdouble mm2[16];
        //glGetDoublev(GL_MODELVIEW_MATRIX, mm2);

        //glPopMatrix();

        // ----
        vector3d v1 = side_surface_point(0, 0, true);
        vector3d v2 = side_surface_point(0, max_j, true);

        vector3d v3 = side_surface_point(0, 0, false);
        vector3d v4 = side_surface_point(0, max_j, false);

        matrix4_t mm1_t = matrix4_t((double*)mm1);
        //matrix4_t mm2_t = matrix4_t((double*)mm2);

        vector3d v1t = v1.transform(mm1_t);
        vector3d v2t = v2.transform(mm1_t);
        vector3d v3t = v3.transform(mm1_t);
        vector3d v4t = v4.transform(mm1_t);

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
