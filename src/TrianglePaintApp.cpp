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
	void setup();
	void resize();
	void mouseDown( MouseEvent event );
	void mouseMove( MouseEvent event );
	void update();
	void draw();
	void drawGrid( float size );
	void drawAxis( float r, float b );
	void drawMover();

	Vec2f convert( Vec2f rb );
	Vec2f worldFromScreen( Vec2i xy );

	gl::VboMeshRef	mVboMesh;

	cinder::CameraOrtho mCam;
	int mR = 1;
	int mB = -1;

	int mUnit = 50;

	Vec2f touched = Vec2i::zero();
	Vec2i mouse;


	params::InterfaceGl mParams;
};

void TrianglePaintApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 220, 170 ) );
	mParams.addParam( "X", &touched.x, true );
	mParams.addParam( "Y", &touched.y, true );
	mParams.hide();


	static const int VERTICES_X = 10, VERTICES_Z = 5;

	// setup the parameters of the Vbo
	int totalVertices = VERTICES_X * VERTICES_Z;
	int totalQuads = ( VERTICES_X - 1 ) * ( VERTICES_Z - 1 );
	gl::VboMesh::Layout layout;
	layout.setStaticIndices();
	layout.setDynamicPositions();
	mVboMesh = gl::VboMesh::create( totalVertices, totalQuads * 4, layout, GL_QUADS );

	// buffer our static data - the texcoords and the indices
	vector<uint32_t> indices;
	for( int x = 0; x < VERTICES_X; ++x ) {
		for( int z = 0; z < VERTICES_Z; ++z ) {
			// create a quad for each vertex, except for along the bottom and right edges
			if( ( x + 1 < VERTICES_X ) && ( z + 1 < VERTICES_Z ) ) {
				indices.push_back( (x+0) * VERTICES_Z + (z+0) );
				indices.push_back( (x+1) * VERTICES_Z + (z+0) );
				indices.push_back( (x+1) * VERTICES_Z + (z+1) );
				indices.push_back( (x+0) * VERTICES_Z + (z+1) );
			}
		}
	}
}

// Replicate most of the defaultResize so we get a reference to the camera
// for converting screen to world.
void TrianglePaintApp::resize()
{
	Vec2i center = getWindowCenter();
	mCam.setOrtho( -center.x, center.x, center.y, -center.y, -1, 1 );
	gl::setMatrices( mCam );
}

void TrianglePaintApp::mouseDown( MouseEvent event )
{
	touched = worldFromScreen(mouse);
}

void TrianglePaintApp::mouseMove( MouseEvent event )
{
	mouse = Vec2i(event.getX(), event.getY());
}

void TrianglePaintApp::update()
{
}

void TrianglePaintApp::draw()
{
	gl::pushModelView();

	gl::clear( Color::white() );

//	drawGrid( 500 );

	// Origin
	gl::color( ColorA::black() );
	gl::drawSolidCircle( Vec2f::zero(), 5 );

	drawMover();

//	drawAxis(rb.x, rb.y);



	gl::enableWireframe();
	gl::color( Color::black() );
	gl::draw( mVboMesh );
	gl::disableWireframe();




	gl::popModelView();

	mParams.draw();
}

void TrianglePaintApp::drawGrid( float size )
{
	gl::pushModelView();

	float step = mUnit * M_SQRT_3_2;

	gl::rotate(30);

	gl::color( ColorAf(0.7, 0, 0, 0.3f) );
	for ( float i = -size * step; i <= size * step; i += step ) {
		gl::drawLine( Vec2f(-size, i), Vec2f(size, i) );
	}
	gl::drawSolidTriangle(mUnit * Vec2f(1, 0), mUnit * Vec2f(0.8, -0.2), mUnit * Vec2f(0.8, 0.2));
	gl::rotate(120);

	gl::color( ColorAf(0, 0.7, 0, 0.3f) );
	for ( float i = -size * step; i <= size * step; i += step ) {
		gl::drawLine( Vec2f(-size, i), Vec2f(size, i) );
	}
	gl::drawSolidTriangle(mUnit * Vec2f(1, 0), mUnit * Vec2f(0.8, -0.2), mUnit * Vec2f(0.8, 0.2));
	gl::rotate(120);

	gl::color( ColorAf(0, 0, 0.7, 0.3f) );
	for ( float i = -size * step; i <= size * step; i += step ) {
		gl::drawLine( Vec2f(-size, i), Vec2f(size, i) );
	}
	gl::drawSolidTriangle(mUnit * Vec2f(1, 0), mUnit * Vec2f(0.8, -0.2), mUnit * Vec2f(0.8, 0.2));
	gl::rotate(120);

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

void TrianglePaintApp::drawMover()
{
	Vec2f rb = worldFromScreen(mouse);
	int fr = floor(rb.x);
	int fb = floor(rb.y);
	int cr = ceil(rb.x);
	int cb = ceil(rb.y);
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
//	console() << vert1 << " " << vert2 << " " << vert3 << endl;

	// Surrounding points
	gl::color( Colorf( 1.0f, 0.0f, 0.0f) );
	gl::drawSolidCircle( convert(vert1), 5 );
	gl::color( Colorf( 0.0f, 1.0f, 0.0f) );
	gl::drawSolidCircle( convert(vert2), 5 );
	gl::color( Colorf( 0.0f, 0.0f, 1.0f) );
	gl::drawSolidCircle( convert(vert3), 5 );

	// Triangle
	gl::color( Colorf( 0.0f, 0.0f, 0.0f) );
	gl::drawSolidTriangle(convert(vert1), convert(vert2), convert(vert3));
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
	pos -= getWindowCenter();
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
	/*
	 // generate a ray from the camera into our world
	 float u = pos.x / (float) getWindowWidth();
	 float v = pos.y / (float) getWindowHeight();
	 // because OpenGL and Cinder use a coordinate system
	 // where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
	 Ray ray = mCam.generateRay(u , 1.0f - v, mCam.getAspectRatio() );

	 float result = 0.0f;
	 Vec3f planePos = Vec3f::zero();
	 Vec3f normal = Vec3f::zAxis();
	 if ( ray.calcPlaneIntersection( planePos, normal, &result ) ) {
	 return ray.calcPosition( result ).xy();
	 }
	 return Vec2f::zero();
	 */
}

CINDER_APP_NATIVE( TrianglePaintApp, RendererGl )
