#pragma once

#include "ofMain.h"
#include "ofAppGlutWindow.h"

#include "ofxAssimpModelLoader.h"
#include "ofxCubeMap.h"
#include "ofxUI.h"
#include "ofxFenster.h"

#include "turntableCam.h"
#include "icosohedron.h"
<<<<<<< HEAD
//#include "radomeSyphonClient.h"
=======
#include "radomeSyphonClient.h"
>>>>>>> 7072e5b1eb2648c23ccb5ba0530a60c77004eed2
#include "radomeProjector.h"
#include "radomeModel.h"

using std::list;
using std::vector;
using std::string;


enum DisplayMode {
    DisplayScene = 0,
    DisplayCubeMap,
    DisplayDome,
    DisplayProjectorOutput,
    LastDisplayMode,
};


class radomeApp : public ofBaseApp {
public:
    radomeApp();
    ~radomeApp();
    
    void setup();
    void update();
    void draw();
    void drawScene();
    void drawDome();
    void drawGroundPlane();
    void updateCubeMap();
    void updateProjectorOutput();
    
    void loadFile();
    void showProjectorWindow();

    DisplayMode getDisplayMode() const { return _displayMode; }
    void changeDisplayMode(DisplayMode mode);
    
    void keyPressed(int key);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseDragged(int x, int y, int button);
    void mouseMoved(int x, int y);

protected:
    void initGUI();
    void guiEvent(ofxUIEventArgs &e);
    void beginShader();
    void endShader();
    
    void prepDrawList();
    
    ofxUICanvas* _pUI;
    ofxUICanvas* _pCalibrationUI;
    
    ofxCubeMap _cubeMap;
    ofShader _shader;
    ofxTurntableCam _cam;
    unsigned int domeDrawIndex;

    list<radomeModel*> _modelList;
    vector<radomeProjector*> _projectorList;
    ofxFenster* _projectorWindow;
    
<<<<<<< HEAD
    //    radomeSyphonClient _vidOverlay;
=======
    radomeSyphonClient _vidOverlay;
>>>>>>> 7072e5b1eb2648c23ccb5ba0530a60c77004eed2
    ofImage _blankImage;
    
    bool _fullscreen;
    
    float _animationTime;
    unsigned long long _lastSystemTime;
    
    int _mixMode;
    int _mappingMode;
    
    enum DisplayMode _displayMode;
    vector<string> _displayModeNames;
    vector<string> _mixModeNames;
    vector<string> _mappingModeNames;

<<<<<<< HEAD
    vector<icosohedron::Triangle> _triangle;
=======
    vector<icosohedron::Triangle> _triangles;
>>>>>>> 7072e5b1eb2648c23ccb5ba0530a60c77004eed2
};
