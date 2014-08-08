#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
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
	Vec2f convert( int r, int b );
	Vec2f worldFromScreen( Vec2i xy );

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
	mParams.addParam( "R", &mR, "keyIncr=R keyDecr=r" );
	mParams.addParam( "B", &mB, "keyIncr=B keyDecr=b" );

	mParams.addParam( "X", &touched.x, true );
	mParams.addParam( "Y", &touched.y, true );
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
	mR = round(touched.x);
	mB = round(touched.y);
	console() << mouse << "\n\t" << touched << endl;
}

void TrianglePaintApp::mouseMove( MouseEvent event )
{
	mouse = Vec2i(event.getX(), event.getY());
}

void TrianglePaintApp::update()
{
	//  mB = - (mR + mG);
}

void TrianglePaintApp::draw()
{
	gl::pushModelView();

	gl::clear( Color::white() );

	drawGrid( 500 );

	// Origin
	gl::color( Colorf( 1.0f, 0.0f, 0.0f) );
	gl::drawSolidCircle( Vec2f::zero(), 10 );

	// Mover
	gl::color( Colorf( 0.0f, 0.0f, 0.0f) );
	gl::drawSolidCircle(convert( mR, mB ), 10 );



	Vec2f parts = worldFromScreen(mouse);
	//  console() << mouse << "\n\t" << parts << endl;

	float theta = 11 * M_PI / 6;
	float slope = -tan(theta);
	Vec2f bBasis = Vec2f(0, -1);
	Vec2f rBasis = Vec2f(cos(theta), -sin(theta));

	Vec2f r = rBasis * parts.x * mUnit;
	Vec2f b = bBasis * parts.y * mUnit;

	gl::lineWidth(2);
	gl::color( Colorf( 1.0f, 0.0f, 1.0f) );
	gl::drawLine(Vec2f(0, 0), r);
	gl::color( Colorf( 0.0f, 1.0f, 1.0f) );
	gl::drawLine(r, b + r);
	gl::lineWidth(1);

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

// RGB to XY
Vec2f TrianglePaintApp::convert( int r, int b )
{
	// Red is 330 degrees
	Vec2f vr = Vec2f(M_SQRT_3_2, 0.5f) * r;
	// Blue is 90 degrees (up)
	Vec2f vb = Vec2f(0, -1) * b;

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
