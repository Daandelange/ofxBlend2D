#pragma once

#include "ofMain.h"
#include "ofxBlend2D.h"
#include "ofxImGui.h"

struct ofPathStyle {
		std::string name = "No Name";

		float strokeWidth = 0.f;
		ofFloatColor strokeColor;

		bool isFilled = false;
		ofFloatColor fillColor;

		bool isVisible = false;

		bool isStroked(){
			return strokeWidth>0.f;
		};

		ofPathStyle(std::string _name, float _pathWidth, ofColor _pathColor, bool _isFilled, ofColor _fillColor, bool _isVisible) :
			name(_name), strokeWidth(_pathWidth), strokeColor(_pathColor), isFilled(_isFilled), fillColor(_fillColor), isVisible(_isVisible)
		{}

		static ofPathStyle fromOfPath(ofPath const& _path, std::string _name="No Name", bool _isVisible=false){
			return {
				_name,
				_path.getStrokeWidth(),
				_path.getStrokeColor(),
				_path.isFilled(),
				_path.getFillColor(),
				_isVisible
			};
		}
};

class ofApp : public ofBaseApp{

	public:
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

		void loadSvg(std::string path);
		
		std::vector<std::pair<BLPath, ofPathStyle> > paths;
		ofxBlend2DThreadedRenderer blend2d;
		ofxImGui::Gui gui;
		bool bRenderBoundingboxes = true;
};
