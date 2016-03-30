#pragma once

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/Thread.h"
#include "cinder/ConcurrentCircularBuffer.h"

	
//POINT GREY
// #if defined(WIN32) || defined(WIN64)

#ifndef _WIN32_WINNT            // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of Windows.
#endif          

#define _CRT_SECURE_NO_WARNINGS                                 

#include <stdio.h>
#include <tchar.h>

#include "FlyCapture2.h"


namespace coc {

	class FlyCapture {
	public:


        FlyCapture() : isNewFrame(false) {}
        bool setup( bool _isCol = false );//todo: test return in windows


		void setup( int _serial, int _w, int _h, bool _isCol );
		void setupThreaded( int _serial, int _w, int _h, bool _isCol );
		void setup( int _serial, bool _isCol, FlyCapture2::Format7ImageSettings _fmt7ImageSettings, float _speed, bool _threaded );

		bool		isThreaded;
		std::mutex	imageMutex;
		ci::Surface surface;
		
		void generateTexture();
		void update();
		void update( FlyCapture2::Image *rawImage );

		void printPropertyInfo( FlyCapture2::PropertyType _type, std::string _name);
		void printProperty( FlyCapture2::PropertyType _type, std::string _name);
		void setProperty( FlyCapture2::PropertyType _type, float _absVal);


		void draw(glm::vec2 _pos = glm::vec2(0, 0));
        void draw( ci::Rectf _bounds );
		void stop();

		int getWidth() { return width; }
		int getHeight() { return height; }
		ci::Channel getChannel() { return channel; }
		ci::Surface getSurface() { return surface; }
		unsigned char * getData() {
			unsigned char * data;
			isCol ? data = surface.getData() : data = channel.getData() ; 
			return data;
		} 

        bool isNewFrame;
        bool checkNewFrame() {
            bool tmp = isNewFrame;
            isNewFrame = false;
            return tmp;
        }


	private:

        std::string makeString(int name);
        float       makeFloat( int name);

        ci::Channel                     channel;
        int                             width;
        int                             height;
        ci::gl::TextureRef              tex;
        bool                            isCol;
        

        FlyCapture2::Camera				cam;
        FlyCapture2::Image              rawImage;
        FlyCapture2::Image              convertedImage;
        FlyCapture2::Error              error;

        void setupSingleCamera( FlyCapture2::PGRGuid guid );

        void PrintBuildInfo();
        void PrintCameraInfo( FlyCapture2::CameraInfo* pCamInfo );
        void PrintError( FlyCapture2::Error error );



	};//class FlyCapture

}//namespace coc