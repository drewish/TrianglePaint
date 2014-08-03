#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TrianglePaintApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void TrianglePaintApp::setup()
{
}

void TrianglePaintApp::mouseDown( MouseEvent event )
{
}

void TrianglePaintApp::update()
{
}

void TrianglePaintApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( TrianglePaintApp, RendererGl )