#include "cocFlyCapture.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace coc{
	
using namespace FlyCapture2;

// <GLOBAL IF THREADED>

std::mutex	imageGrabberMutex;

void onImageGrabbed(Image* pImage, const void* pCallbackData)
{

	imageGrabberMutex.lock();
	coc::FlyCapture* cam = (coc::FlyCapture*) pCallbackData;
	cam->update(pImage);
	imageGrabberMutex.unlock();
}

// </GLOBAL IF THREADED>

void FlyCapture::setup(int _w, int _h, bool _isCol, bool _threaded, int _serial)
{
	isThreaded = false;
	isCol = _isCol;
    width  = _w;
    height = _h;
    if (!isCol) {
		channel = Channel(width,height);
	} else {
		surface = Surface( width, height, false, SurfaceChannelOrder::RGB );
	}

    PrintBuildInfo();

    BusManager busMgr;
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK) PrintError( error );

    printf( "Number of cameras detected: %u:\n", numCameras );

	for (int i=0; i<numCameras; i++) {
		unsigned int num;
		busMgr.GetCameraSerialNumberFromIndex(i, &num);
		console() << "\t" << num << endl;
	}

	PGRGuid guid;
	if (_serial) {
		error = busMgr.GetCameraFromSerialNumber(_serial, &guid);
	}
	else {
		unsigned int num;
		busMgr.GetCameraSerialNumberFromIndex(0, &num);
		error = busMgr.GetCameraFromSerialNumber(num, &guid);
	}
    if (error != PGRERROR_OK) PrintError( error );

	// Connect to a camera
	error = cam.Connect(&guid);
	if (error != PGRERROR_OK) PrintError(error);

	// Get the camera information
	CameraInfo camInfo;
	error = cam.GetCameraInfo(&camInfo);
	if (error != PGRERROR_OK) PrintError(error);
	PrintCameraInfo(&camInfo);

	if (!isThreaded) {
		error = cam.StartCapture();
	}
	else {
		error = cam.StartCapture(onImageGrabbed, (void*)this);
	}
	if (error != PGRERROR_OK) PrintError(error);
}


void FlyCapture::setup( int _serial, bool _isCol, Format7ImageSettings _fmt7ImageSettings, float _speed, bool _threaded )
{
	isThreaded = _threaded;
	isCol = _isCol;
    width  = _fmt7ImageSettings.width;
    height = _fmt7ImageSettings.height;
    if (!isCol) {
		channel = Channel(width,height);
	} else {
		surface = Surface( width, height, false, SurfaceChannelOrder::RGB );
	}

    PrintBuildInfo();

    BusManager busMgr;
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK) PrintError( error );

    printf( "Number of cameras detected: %u:\n", numCameras );

	for (int i=0; i<numCameras; i++) {
		unsigned int num;
		busMgr.GetCameraSerialNumberFromIndex(i, &num);
		console() << "\t" << num << endl;
	}
	
	PGRGuid guid;
	error = busMgr.GetCameraFromSerialNumber(_serial, &guid);

    if (error != PGRERROR_OK) PrintError( error );

    // Connect to a camera
    error = cam.Connect(&guid);
    if (error != PGRERROR_OK) PrintError( error );

    // Get the camera information
    CameraInfo camInfo;
    error = cam.GetCameraInfo(&camInfo);
    if (error != PGRERROR_OK) PrintError( error );

    PrintCameraInfo(&camInfo); 

	cam.SetFormat7Configuration( &_fmt7ImageSettings, _speed);

	if (!_threaded) {
		error = cam.StartCapture();
	} else {
		error = cam.StartCapture(onImageGrabbed, (void*)this);
	}
	if (error != PGRERROR_OK) PrintError( error );

}


void FlyCapture::update() {
	
	if (isThreaded)
	{
		//CI_LOG_E("Threaded, no need to call update()!");
	}
	else
	{
		update( nullptr );
	}

}


void FlyCapture::update( Image *rawImage ) //threaded function for flea
{

	if (rawImage == nullptr) {
		Image rawImage;
		error = cam.RetrieveBuffer(&rawImage);
		if (error != PGRERROR_OK)PrintError(error);


		Image convertedImage;
		if (!isCol) {//make mono
			error = rawImage.Convert(PIXEL_FORMAT_MONO8, &convertedImage);
			if (error != PGRERROR_OK) PrintError(error);
			memcpy(channel.getData(), convertedImage.GetData(), convertedImage.GetCols() * convertedImage.GetRows());
			isNewFrame = true;
		}
		else {//RGB

			error = rawImage.Convert(PIXEL_FORMAT_RGB8, &convertedImage);
			if (error != PGRERROR_OK) PrintError(error);

			memcpy(surface.getData(), convertedImage.GetData(), convertedImage.GetCols() * convertedImage.GetRows() * 3);
			isNewFrame = true;

		}
	}
	else {
		Image convertedImage;
		if (!isCol) {//make mono
			error = rawImage->Convert(PIXEL_FORMAT_MONO8, &convertedImage);
			if (error != PGRERROR_OK) PrintError(error);

			imageMutex.lock();
			memcpy(channel.getData(), convertedImage.GetData(), convertedImage.GetCols() * convertedImage.GetRows());
			isNewFrame = true;
			imageMutex.unlock();

		}
		else {//RGB

			error = rawImage->Convert(PIXEL_FORMAT_RGB, &convertedImage);
			if (error != PGRERROR_OK) PrintError(error);

			imageMutex.lock();
			memcpy(surface.getData(), convertedImage.GetData(), convertedImage.GetCols() * convertedImage.GetRows() * 3);
			isNewFrame = true;
			imageMutex.unlock();


		}
	}
	

	
}

void FlyCapture::generateTexture() {
	
	if (!isCol) {//make mono
		imageMutex.lock();
        tex = gl::Texture::create( channel );
		imageMutex.unlock();
	}
	else {//RGB
		imageMutex.lock();
		if (surface.getData()) tex = gl::Texture::create( surface );
		imageMutex.unlock();
	}
}

void FlyCapture::draw( glm::vec2 _pos)
{
	generateTexture();
    if (tex) gl::draw( tex, _pos );
}

void FlyCapture::draw( Rectf _bounds )
{
	generateTexture();
    if (tex) gl::draw( tex, _bounds);
}

void FlyCapture::stop() {
        // Stop capturing images
    error = cam.StopCapture();
    if (error != PGRERROR_OK) PrintError( error );   
    // Disconnect the camera
    error = cam.Disconnect();
    if (error != PGRERROR_OK) PrintError( error );
}

void FlyCapture::printPropertyInfo( PropertyType _type, string _name) {
	PropertyInfo propInfo;
	propInfo.type = _type;
	cam.GetPropertyInfo( &propInfo );
	
	printf(
		"\nCamera Property Type: %s\n"
		"\tpresent: %i\n"
		"\tautoSupported: %i\n"
		"\tmanualSupported: %i\n"
		"\tonOffSupported: %i\n"
		"\tonePushSupported: %i\n"
		"\tabsValSupported: %i\n"
		"\treadOutSupported: %i\n"
		"\tmin: %i\n"
		"\tmax: %i\n"
		"\tabsMin: %f\n"
		"\tabsMax: %f\n\n",
		_name.c_str(),
		(int)propInfo.present,
		(int)propInfo.autoSupported,
		(int)propInfo.manualSupported,
		(int)propInfo.onOffSupported,
		(int)propInfo.onePushSupported,
		(int)propInfo.absValSupported,
		(int)propInfo.readOutSupported,
		propInfo.min,
		propInfo.max,
		propInfo.absMin,
		propInfo.absMax
		);
		
} 

void FlyCapture::printProperty( PropertyType _type, string _name) {
	Property propInfo;
	propInfo.type = _type;
	cam.GetProperty( &propInfo );
	
	printf(
		"\nCamera Property: %s\n"
		"\tpresent: %i\n"
		"\tabsControl: %i\n"
		"\tonePush: %i\n"
		"\tonOff: %i\n"
		"\tautoManualMode: %i\n"
		"\tvalueA: %i\n"
		"\tvalueB: %i\n"
		"\tabsValue: %f\n\n",
		_name.c_str(),
		(int)propInfo.present,
		(int)propInfo.absControl,
		(int)propInfo.onePush,
		(int)propInfo.onOff,
		(int)propInfo.autoManualMode,
		propInfo.valueA,
		propInfo.valueB,
		propInfo.absValue
		);
		
} 

void FlyCapture::setProperty( PropertyType _type, float _absVal) {
	Property prop;
	prop.type = _type;
	cam.GetProperty( &prop );

	prop.absControl = true;
	prop.autoManualMode = false;
	prop.absValue = _absVal;
	cam.SetProperty( &prop );
}

void FlyCapture::PrintBuildInfo()
{
    FC2Version fc2Version;
    Utilities::GetLibraryVersion( &fc2Version );
    char version[128];
    sprintf( 
        version, 
        "FlyCapture2 library version: %d.%d.%d.%d\n", 
        fc2Version.major, fc2Version.minor, fc2Version.type, fc2Version.build );

    printf( version );

    char timeStamp[512];
    sprintf( timeStamp, "Application build date: %s %s\n\n", __DATE__, __TIME__ );

    printf( timeStamp );
}

void FlyCapture::PrintCameraInfo( CameraInfo* pCamInfo )
{
    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n\n",
        pCamInfo->serialNumber,
        pCamInfo->modelName,
        pCamInfo->vendorName,
        pCamInfo->sensorInfo,
        pCamInfo->sensorResolution,
        pCamInfo->firmwareVersion,
        pCamInfo->firmwareBuildTime );
}

void FlyCapture::PrintError( Error error )
{
    error.PrintErrorTrace();
}


}//namespace coc