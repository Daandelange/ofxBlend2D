#pragma once

#include "ofMain.h"
#include "ofxBlend2D.h"
#include <list>
#include "ofxGui.h"
#include "ofxFps.h"
#include "ofxTimeMeasurements.h"

class ofApp : public ofBaseApp{

	public:
        ofApp() {};
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

        void onThreadsChanged(int& value){
            if(value<1) value = 1;
            blend2d.setNumThreads(value);
        }
		
        ofxBlend2DThreadedRenderer blend2d;
        ofxFps blRendererFps;

        ofxPanel gui;
        ofxIntSlider numThreads;
        ofxToggle useBlend2DForRendering;
        ofxIntSlider rows;
        ofxIntSlider cols;
        //ofxToggle drawFilled;
        //ofxToggle drawStroked;
        ofxToggle doAnimate;
        ofxToggle showGrid;
        ofxColorSlider shapeColor;

        // Cached vector graphics
        // Both native types to the respective libs
        BLPath blShape;
        ofPath ofShape;

        bool bSaveNextFrame = false;
};
