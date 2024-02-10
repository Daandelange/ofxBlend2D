#include "ofApp.h"
#include "ofxSvgLoader.h"

//--------------------------------------------------------------
void ofApp::setup(){
    gui.setup(nullptr, true);
    loadSvg("of-logo.svg");

    blend2d.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
}

//--------------------------------------------------------------
void ofApp::update(){
    // Flush the Blend2D pipeline from update()
    blend2d.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

    // This displays the latest rendered texture, from a previous frame
    ofTexture& blTex = blend2d.getTexture();
    if(blTex.isAllocated()){
        ofFill();
        ofSetColor(ofColor::white);
        blTex.draw(0,0);
    }

    // Submit to the blend2d pipeline
    if(blend2d.begin()){
        // This scope will not be called every frame !

        // - - - - - -
        // Get the context
        BLContext ctx = blend2d.getBlContext();
        ctx.translate(20, 40); // Leave place for menu and padding

        for(auto& shapeInfo : paths){
            BLPath& blPath = shapeInfo.first;
            ofPathStyle& style = shapeInfo.second;

            if(!style.isVisible) return;

            if(!blPath.empty()){
                if(style.isFilled){
                    ctx.fillPath(blPath, toBLColor(style.fillColor));
                }

                if(style.isStroked()){
                    ctx.setStrokeWidth(style.strokeWidth);
                    ctx.strokePath(blPath, toBLColor(style.strokeColor));
                }
            }
        }
        blend2d.end();
    }

    if(bRenderBoundingboxes){
        ofPushMatrix();
        ofTranslate(20, 40); // Leave place for menu and padding
        ofPushStyle();
        ofNoFill();
        ofSetColor(ofColor::red);
        for(auto& shapeInfo : paths){
            BLPath& blPath = shapeInfo.first;
            ofPathStyle& style = shapeInfo.second;
            if(!blPath.empty()){
                BLBox bbox;
                BLResult result = blPath.getBoundingBox(&bbox);
                if(result!=BL_SUCCESS){
                    // ignore
                }
                else {
                    ofDrawRectangle(bbox.x0, bbox.y0, bbox.x1-bbox.x0, bbox.y1-bbox.y0);
                }
            }
        }
        ofPopStyle();
        ofPopMatrix();
    }

    gui.begin();
    if(ImGui::BeginMainMenuBar()){
        if(ImGui::BeginMenu("ofxBlend2D Settings")){
            ImGui::SeparatorText("OpenFrameworks");
            ImGui::Text("FPS: %.1f (target: %.0f)", ofGetFrameRate(), ofGetTargetFrameRate());
            ImGui::Text("Resolution: %i x %i", ofGetWidth(), ofGetHeight());
            ImGui::Dummy({20,20});

            ImGui::BeginDisabled();
            ImGui::TextWrapped("\nBelow, you can change some blend2D settings.\nofxBlend2D supports only the proposed pixel modes.\nYou can also set the threading to compare performance, watching the FPS monitor.");
            ImGui::EndDisabled();
            blend2d.drawImGuiSettings();
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("SVG Scene")){
            if(ImGui::MenuItem("Load another SVG...")){
                ofFileDialogResult fileQuery = ofSystemLoadDialog("SVG file to load ?", false);
                if(fileQuery.bSuccess){
                    loadSvg(fileQuery.getPath());
                }
            }
            ImGui::Text("Loaded paths: %lu", paths.size());
            ImGui::Dummy({20,20});

            ImGui::Separator();

            for(auto& shapeInfo : paths){
                BLPath& blPath = shapeInfo.first;
                ofPathStyle& style = shapeInfo.second;

                ImGui::PushID(&blPath);
                if(ImGui::CollapsingHeader( style.name.c_str() )){
                    ImGui::Text("blPath size: %lu", blPath.size());
                    ImGui::InputFloat("Stroke Width", &style.strokeWidth, 0.1, 1.0, "%.3f" );
                    ImGui::ColorEdit4("Stroke color", &style.strokeColor[0]);
                    ImGui::Checkbox("Filled", &style.isFilled);
                    ImGui::ColorEdit4("Fill Color", &style.fillColor[0]);

//                    if(ImGui::TreeNodeEx((void*)&blPath->points, _recurseChildren?ImGuiTreeNodeFlags_DefaultOpen:ImGuiTreeNodeFlags_None, "Points: %lu", _shape->points.size())){
//                        for(auto& point : _shape->points){
//                            ImGui::PushID(&point);
//                            //ImGui::BulletText("[%0.2f, %0.2f]", point.pos.x, point.pos.y);
//                            if(ImGui::DragFloat2("##Position", &point.pos.x, 0.5f)){
//                                updatePath=true;
//                            }
//                            if(point.markShapeStart){
//                                ImGui::SameLine();
//                                ImGui::Text("(Start!)");
//                            }
//                            ImGui::PopID();
//                        }
//                        ImGui::TreePop();
//                    }
                }
                ImGui::PopID();
            }
            ImGui::EndMenu();

        }

        if(ImGui::BeginMenu("Rendering")){
            ImGui::Checkbox("Render Bounding Boxes", &bRenderBoundingboxes);
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
    gui.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    blend2d.allocate(w, h);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void loadFromSvgBaseRecursive(std::vector<std::pair<BLPath, ofPathStyle> >& paths, ofxSvgBase& svg){
    if(svg.isGroup()){
        ofxSvgGroup* g = dynamic_cast<ofxSvgGroup*>(&svg);
        if(g != nullptr){
            for(std::shared_ptr<ofxSvgBase>& e : g->getElements()){
                loadFromSvgBaseRecursive(paths, *e.get());
            }
        }
    }
    else {
        // Try cast as SVG base which all have an ofPath
        ofxSvgElement* e = dynamic_cast<ofxSvgElement*>(&svg);
        if(e){
            // Use SVG layer name, or SVG type as fallback
            std::string pathName = svg.getName();
            if(strcmp(pathName.c_str(), "No Name")==0) pathName = svg.getTypeAsString();

            // Here we use the toBLPath() helper to convert an ofPath to a BLPath
            paths.push_back( {toBLPath(e->path), ofPathStyle::fromOfPath(e->path, pathName.c_str(), svg.isVisible())} );
        }
        else ofLogWarning("loadFromSvgBaseRecursive") << "Unsupported shape type : " << svg.getTypeAsString() <<" !";
    }
}


//--------------------------------------------------------------
void ofApp::loadSvg(std::string path){
    ofxSvgLoader svg;
    svg.load(path);

    paths.clear();
    for(std::shared_ptr<ofxSvgBase>& e : svg.getElements()){
        loadFromSvgBaseRecursive(paths, *e.get());
    }
}
