#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;



#define M_SQRT_3_2 0.8660254037844386 // sqrt(3) / 2
#define M_1_SQRT_3 0.5773502691896258 // 1 / sqrt(3)

class TrianglePaintApp : public AppNative {
public:
    void prepareSettings( Settings *settings ) override;
    void setup();
    void resize();
    void buildVBOMesh();
    void colorVBOMesh();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void mouseMove( MouseEvent event );
    void keyUp( KeyEvent );
    void update();
    void draw();

    void drawGrid( );
    void drawAxis( float r, float b );
    void drawOrigin();
    void drawMover();


    int toggleTriangle(int col, int row);
    void colorTriangle(int col, int row, int palette_index);

    bool inTriangle(Vec2f point, Vec2f p1, Vec2f p2, Vec2f p3);
    void getTriangle( int col, int row, Vec3f &a, Vec3f &b, Vec3f &c );
    bool getTriangleFromMouse( int &col, int &row );

    Vec2f convert( Vec2f rb );
    Vec2f worldFromScreen( Vec2i xy );

    cinder::CameraOrtho mCam;

    int mUnit = 50;

    Vec2i mouse;

    int mCols = 20;
    int mRows = 30;
    int mColor = 0;
    gl::VboMesh mMesh;
    // Colors from http://www.colourlovers.com/palette/92095/Giant_Goldfish
    // #69D2E7 and #A7DBD8
    Color mColors[4]={
        Color8u( 224,228,204 ),
        Color8u( 167,219,216 ),
        Color8u( 105,210,231 ),
        Color8u::white()
    };
    std::vector<std::vector<int> > mPainted;

    params::InterfaceGl mParams;
};

void TrianglePaintApp::prepareSettings( Settings *settings )
{
    settings->enableHighDensityDisplay();
    settings->enableMultiTouch( false );
}

void TrianglePaintApp::setup()
{
    mParams = params::InterfaceGl( "Parameters", Vec2i( 220, 170 ) );
    mParams.addParam( "Scale", &mUnit )
        .keyIncr( "s" ).keyDecr( "S" )
        .min( 1 ).max( 100 ).step( 1 )
        .updateFn( [this] { buildVBOMesh(); } );
    mParams.addParam( "Columns", &mCols )
        .keyIncr( "d" ).keyDecr( "D" )
        .min( 1 ).max( 100 ).step( 1 )
        .updateFn( [this] { buildVBOMesh(); } );
    mParams.addParam( "Rows", &mRows )
        .keyIncr( "f" ).keyDecr( "F" )
        .min( 1 ).max( 100 ).step( 1 )
        .updateFn( [this] { buildVBOMesh(); } );
    mParams.addParam( "Color", &mColor )
        .keyIncr( "c" ).keyDecr( "C" )
        .min( 0 ).max( 3 ).step( 1 );

    mParams.hide();

//    hideCursor();
    buildVBOMesh();
}

// Replicate most of the defaultResize so we get a reference to the camera
// for converting screen to world.
void TrianglePaintApp::resize()
{
    Vec2i size = getWindowSize();
    mCam.setOrtho( 0, size.x, size.y, 0, -1, 1 );
    gl::setMatrices( mCam );
}

void TrianglePaintApp::buildVBOMesh()
{
    gl::VboMesh::Layout layout;
    layout.setStaticIndices();
    layout.setStaticPositions();
    layout.setDynamicColorsRGB();

    mMesh = gl::VboMesh(mCols * mRows * 3,mCols * mRows * 3, layout, GL_TRIANGLES);
    vector<uint32_t> indices;
    vector<Vec3f> positions;

    Vec3f a;
    Vec3f b;
    Vec3f c;
    int index = -1;

    mPainted.resize(mCols);
    for (int col = 0; col < mCols; col++) {
        mPainted[col].resize(mRows);
        for (int row = 0; row < mRows; row++) {
            mPainted[col][row] = 3 ;//(col + row) % 2;

            getTriangle(col, row, a, b, c);
            positions.push_back( a );
            positions.push_back( b );
            positions.push_back( c );

            index += 3;
            indices.push_back( index - 2 );
            indices.push_back( index - 1 );
            indices.push_back( index );
        }
    }
    mMesh.bufferIndices( indices );
    mMesh.bufferPositions( positions );

    colorVBOMesh();
}

void TrianglePaintApp::colorVBOMesh()
{
    // Dynmaically color the grid
    gl::VboMesh::VertexIter iter = mMesh.mapVertexBuffer();
    for (int col = 0; col < mCols; col++) {
        for (int row = 0; row < mRows; row++) {
            Color8u color = mColors[mPainted[col][row]];
            for (int vert = 0; vert < 3; vert++) {
                iter.setColorRGB(color);
                ++iter;
            }
        }
    }
}

void TrianglePaintApp::getTriangle( int col, int row, Vec3f &a, Vec3f &b, Vec3f &c) {
    float w = M_SQRT_3_2 * mUnit * 0.5f;
    float h = mUnit * 0.5f;
    float x = col * 2 * w + w;
    float y = row * h;

    int direction = (col + row) % 2 ? 1 : -1;

    a = Vec3f( x - w * direction, y - h, 0 );
    b = Vec3f( x + w * direction, y + 0, 0 );
    c = Vec3f( x - w * direction, y + h, 0 );
}

bool TrianglePaintApp::inTriangle(Vec2f p, Vec2f p0, Vec2f p1, Vec2f p2)
{
    // Based on http://stackoverflow.com/a/14382692/203673
    float Area = 0.5*(-p1.y*p2.x + p0.y*(-p1.x + p2.x) + p0.x*(p1.y - p2.y) + p1.x*p2.y);
    float s = 1/(2.0*Area)*(p0.y*p2.x - p0.x*p2.y + (p2.y - p0.y)*p.x + (p0.x - p2.x)*p.y);
    float t = 1/(2.0*Area)*(p0.x*p1.y - p0.y*p1.x + (p0.y - p1.y)*p.x + (p1.x - p0.x)*p.y);
    return (s>0.0 && t>0.0 && 1.0-s-t>0.0);
}


int TrianglePaintApp::toggleTriangle(int col, int row)
{
    return mPainted[col][row] = (mPainted[col][row] + 1) % 4;
}

void TrianglePaintApp::colorTriangle(int col, int row, int palette_index)
{
    mPainted[col][row] = palette_index;
}


void TrianglePaintApp::mouseDown( MouseEvent event )
{
    int col, row;
    if (getTriangleFromMouse(col, row)) {
//        toggleTriangle(col, row);
        colorTriangle(col, row, mColor);
        colorVBOMesh();
    }
}

void TrianglePaintApp::mouseDrag( MouseEvent event )
{
    mouse = Vec2i(event.getX(), event.getY());

    int col, row;
    if (getTriangleFromMouse(col, row)) {
        colorTriangle(col, row, mColor);
        colorVBOMesh();
    }

    console() << event.getX() << " " << event.getY() << endl;
}

void TrianglePaintApp::mouseMove( MouseEvent event )
{
    mouse = Vec2i(event.getX(), event.getY());
}

void TrianglePaintApp::keyUp( KeyEvent event )
{
// TODO this is needlessly verbose.
    if( event.getChar() == '1' ) {
        mColor = 0;
    } else if( event.getChar() == '2' ) {
        mColor = 1;
    } else if( event.getChar() == '3' ) {
        mColor = 2;
    } else if( event.getChar() == '4' ) {
        mColor = 3;
    }
}


void TrianglePaintApp::update()
{
}

void TrianglePaintApp::draw()
{

    gl::enableAlphaBlending();
    gl::pushModelView();

    gl::clear( Color(0.9f, 0.9f, 0.9f) );

    gl::draw(mMesh);

    drawGrid( );
//    drawOrigin();
    drawMover();

//    gl::enableWireframe();
//    gl::color( Color::black() );
//    gl::draw( mVboMesh );
//    gl::disableWireframe();

    gl::popModelView();

    mParams.draw();
}

void TrianglePaintApp::drawGrid()
{
    gl::pushModelView();

    gl::lineWidth(1);
    gl::color(0, 0, 0, 0.2);

    int width = getWindowWidth();
    int height = getWindowHeight();
    float x, y;
    float step = mUnit;
    // slope isn't the right word...
    float slope = width * M_1_SQRT_3;

    int heightInTriangles = ceil(height / (float)mUnit) * mUnit;

    // red  gl::color( ColorAf(0.7, 0, 0, 0.3f) );
    for ( y = -heightInTriangles; y <= heightInTriangles; y += mUnit ) {
        gl::drawLine( Vec2f(0, y), Vec2f(width, slope + y) );
    }
    // green  gl::color( ColorAf(0, 0.7, 0, 0.3f) );
    for ( y = 0; y <= heightInTriangles * 2; y += mUnit ) {
        gl::drawLine( Vec2f(0, y), Vec2f(width, -slope + y) );
    }
    // blue  gl::color( ColorAf(0, 0, 0.7, 0.3f) );
    step = mUnit * M_SQRT_3_2;
    for ( x = 0; x <= width; x += step ) {
        gl::drawLine( Vec2f(x, 0), Vec2f(x, heightInTriangles) );
    }

    gl::popModelView();
}

void TrianglePaintApp::drawAxis( float _r, float _b )
{
    // Isolate the two vectors so we can draw the components
    Vec2f r = convert(Vec2f(_r, 0));
    Vec2f b = convert(Vec2f(0, _b));
    gl::lineWidth(2);
    gl::color( Colorf( 1.0f, 0.0f, 1.0f) );
    gl::drawLine(Vec2f(0, 0), r);
    gl::color( Colorf( 0.0f, 1.0f, 1.0f) );
    gl::drawLine(r, b + r);
    gl::lineWidth(1);
}

void TrianglePaintApp::drawOrigin()
{
    // Origin
    gl::color( Color8u( 250,105,0 ) );
    gl::drawSolidCircle( Vec2f::zero(), 5 );

    gl::rotate(30);
    for (int i = 0; i < 3; i++) {
        gl::drawSolidTriangle(mUnit * Vec2f(1, 0), mUnit * Vec2f(0.8, -0.2), mUnit * Vec2f(0.8, 0.2));
        gl::rotate(120);
    }
}

void TrianglePaintApp::drawMover()
{
    Vec2f rb = worldFromScreen(mouse);
//    drawAxis(rb.x, rb.y);

    int cr = ceil(rb.x);
    int fr = floor(rb.x);
    int cb = ceil(rb.y);
    int fb = floor(rb.y);
    Vec2f vert1 = Vec2f( fr, fb );
    Vec2f vert2 = Vec2f(  0, cb );
    Vec2f vert3 = Vec2f( cr,  0 );
    // Triangles can be in two orientations, we want the vertexes to always be
    // in clockwise order so we have to do a little more work.
    if (rb.x - fr < rb.y - fb ) {
        vert2.x = fr;
        vert3.y = cb;
    } else {
        vert2.x = cr;
        vert3.y = fb;
    }

    // Surrounding points
    //  gl::color( Color8u( 243,134,48 ) );
    gl::color(mColors[mColor]);
    //  gl::color( Color8u( 167,219,216 ) );
    gl::drawSolidCircle( convert(vert1), 2 );
    gl::drawSolidCircle( convert(vert2), 2 );
    gl::drawSolidCircle( convert(vert3), 2 );
    gl::lineWidth(10);
    gl::drawStrokedTriangle(convert(vert1), convert(vert2), convert(vert3));
}

bool TrianglePaintApp::getTriangleFromMouse( int &col, int &row )
{
    Vec2f pos = mouse;
    Vec3f p0, p1, p2;
    for (col = 0; col < mCols; col++) {
        for (row = 0; row < mRows; row++) {
            getTriangle(col, row, p0, p1, p2);
            if (inTriangle(pos, p0.xy(), p1.xy(), p2.xy())) {
                console() << p0 << p1 << p2 << endl;
                return true;
            }
        }
    }
    console() << "No hit" << endl;
    return false;
}

// RGB to XY
Vec2f TrianglePaintApp::convert( Vec2f rb )
{
    // Red is 330 degrees
    Vec2f vr = Vec2f(M_SQRT_3_2, 0.5f) * rb.x;
    // Blue is 90 degrees (up)
    Vec2f vb = Vec2f(0, -1) * rb.y;

    return (vb + vr) * mUnit;
}

Vec2f TrianglePaintApp::worldFromScreen( Vec2i pos )
{
    //  pos -= getWindowCenter();
    // I'm sure I'll have no idea how I derived these. I spent a lot of time
    // looking at the unit circle in grapher. The red axis has an angle of 330
    // degrees (11*pi/6) and slope is tan(11*pi/6) or -1/sqrt(3). Eventually I
    // realized that the red axis got you over to the X position but introduced
    // a change in the blue/Y axis that needed to be canceled out there.
    //
    // This needs to return the component along each axis. Once I figured out
    // I needed to use unit vectors for the basis (length 1) blue was easy. It's
    // pointed up the (inverted thanks to cinder) y-axis so I just need to
    // subtract out the X component (slope * x). The red axis was trickier but
    // eventually I found that I needed to use the secant to get it scaled
    // correctly and that: x/cos(theta) did it. Wolfram Alpha was helpful in
    // determining transformations to get the trig functions out.
    float red = pos.x / M_SQRT_3_2;
    float blue = -(pos.y - M_1_SQRT_3 * pos.x);
    return Vec2f(red, blue) / mUnit;
}

CINDER_APP_NATIVE( TrianglePaintApp, RendererGl )
