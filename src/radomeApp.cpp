#include "radomeApp.h"
#include "radomeUtils.h"
#include "ofxFensterManager.h"

#define SIDEBAR_WIDTH 180
#define CALIBRATIONUI_WIDTH 210
#define DEFAULT_SYPHON_APP "Arena"
#define DEFAULT_SYPHON_SERVER "Composition"
#define DOME_DIAMETER 300
#define DOME_HEIGHT 110
#define NUM_PROJECTORS 3

#define PROJECTOR_INITIAL_HEIGHT 147.5
#define PROJECTOR_INITIAL_DISTANCE DOME_DIAMETER*1.5

radomeApp::radomeApp() {
  _pUI = NULL;
  _projectorWindow = NULL;
}

radomeApp::~radomeApp() {
  if (_pUI)
    delete (_pUI);
    
  if (_projectorWindow) {
    _projectorWindow->destroy();
    ofxFensterManager::get()->deleteFenster(_projectorWindow);
  }
    
  deletePointerCollection(_projectorList);
  deletePointerCollection(_modelList);
}

void radomeApp::setup() {
  //configure window manager
  ofxFensterManager::get()->setWindowTitle("radome");
  
  ofSetFrameRate(45);
  ofEnableSmoothing();

  //set the display mode (see enum in header)
  _displayMode = DisplayScene;
  //seems to be related to the shader?
  _mixMode = 0;
  //shader variable
  _mappingMode = 0;
  //animation timer for the modelList
  _animationTime = 0.0;
  _lastSystemTime = ofGetSystemTime();
  _fullscreen = false;
  
  _shader.load("radome");
  
  //initialize cubemap in the FBO
  _cubeMap.initEmptyTextures(1024);
  _cubeMap.setNearFar(ofVec2f(0.01, 8192.0));
  
  //setup turntable cam
  _cam.setTarget(ofVec3f(0.0, DOME_HEIGHT*0.25, 0.0));
  _cam.setRotation(0.66, 0.5);
  _cam.setupPerspective(false);
  
  //icosohedron class? My guess is that this creates the dome mesh.
  _triangles = icosohedron::createsphere(4);
  
  //syphon client
  //  _vidOverlay.initialize(DEFAULT_SYPHON_APP, DEFAULT_SYPHON_SERVER);
  //_vidOverlay.setFaderValue(0.75);
  
  //setup a blank  ofImage
  unsigned char p[3] = {0, 0, 0};
  _blankImage.setUseTexture(true);
  //set it to a 1x1 black pixel?
  _blankImage.setFromPixels(p, 1, 1, OF_IMAGE_COLOR);
  
  //create list of projectors and their positions.
  for (int ii = 0; ii < NUM_PROJECTORS; ii++) {
    _projectorList.push_back(new radomeProjector(ii*360.0/(NUM_PROJECTORS*1.0)+60.0, PROJECTOR_INITIAL_DISTANCE, PROJECTOR_INITIAL_HEIGHT));
  }    
  
  //initialize the GUI!
  initGUI();
  
  glEnable(GL_DEPTH_TEST);
  //some more dome geometry setup
  prepDrawList();
}

void radomeApp::initGUI() {
  _pUI = new ofxUICanvas(5, 0, SIDEBAR_WIDTH, ofGetHeight());
  _pUI->setWidgetSpacing(5.0);
  _pUI->setDrawBack(true);
   
  //setup Projector parameters
  _pCalibrationUI = new ofxUICanvas(SIDEBAR_WIDTH + 5, 0, CALIBRATIONUI_WIDTH, ofGetHeight());
  _pCalibrationUI->setWidgetSpacing(5.0);
  _pCalibrationUI->setDrawBack(true);
  _pCalibrationUI->setFont("GUI/Exo-Regular.ttf", true, true, false, 0.0, OFX_UI_FONT_RESOLUTION);
  _pCalibrationUI->addWidgetDown(new ofxUILabel("CALIBRATION", OFX_UI_FONT_MEDIUM));
  _pCalibrationUI->addSpacer(0, 12);
  _pCalibrationUI->addSlider("PROJECTOR 1 HEIGHT", 5.0, 20.0, PROJECTOR_INITIAL_HEIGHT/10.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 1 HEADING", 0.0, 120.0, 60.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 1 DISTANCE", DOME_DIAMETER/20.0, DOME_DIAMETER/5.0, PROJECTOR_INITIAL_DISTANCE/10.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 1 FOV", 20.0, 90.0, 30.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 1 TARGET", 0.0, DOME_HEIGHT/10.0, 2.0, 200, 25);
  _pCalibrationUI->addSpacer(0, 12);
  _pCalibrationUI->addSlider("PROJECTOR 2 HEIGHT", 5.0, 20.0, PROJECTOR_INITIAL_HEIGHT/10.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 2 HEADING", 120.0, 240.0, 180.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 2 DISTANCE", DOME_DIAMETER/20.0, DOME_DIAMETER/5.0, PROJECTOR_INITIAL_DISTANCE/10.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 2 FOV", 20.0, 90.0, 30.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 2 TARGET", 0.0, DOME_HEIGHT/10.0, 2.0, 200, 25);
  _pCalibrationUI->addSpacer(0, 12);
  _pCalibrationUI->addSlider("PROJECTOR 3 HEIGHT", 5.0, 20.0, PROJECTOR_INITIAL_HEIGHT/10.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 3 HEADING", 240.0, 360.0, 300.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 3 DISTANCE", DOME_DIAMETER/20.0, DOME_DIAMETER/5.0, PROJECTOR_INITIAL_DISTANCE/10.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 3 FOV", 20.0, 90.0, 30.0, 200, 25);
  _pCalibrationUI->addSlider("PROJECTOR 3 TARGET", 0.0, DOME_HEIGHT/10.0, 2.0, 200, 25);
  _pCalibrationUI->setVisible(false);
    
  //label for Radome
  _pUI->setFont("GUI/Exo-Regular.ttf", true, true, false, 0.0, OFX_UI_FONT_RESOLUTION);
  _pUI->addWidgetDown(new ofxUILabel("RADOME 0.2", OFX_UI_FONT_LARGE));
  _pUI->addSpacer(0, 12);
    
  //dropdown menu for display modes
  _displayModeNames.push_back("3D Scene");
  _displayModeNames.push_back("Cube Map");
  _displayModeNames.push_back("Dome Preview");
  _displayModeNames.push_back("Output Preview");
  addRadioAndSetFirstItem(_pUI, "DISPLAY MODE", _displayModeNames, OFX_UI_ORIENTATION_VERTICAL, 16, 16);
  _pUI->addSpacer(0, 12);

  //more projector options
  _pUI->addWidgetDown(new ofxUILabel("PROJECTORS", OFX_UI_FONT_MEDIUM));
  _pUI->addWidgetDown(new ofxUILabelButton("Calibrate...", false, 0, 30, 0, 0, OFX_UI_FONT_SMALL));
  _pUI->addWidgetDown(new ofxUILabelButton("Show Window", false, 0, 30, 0, 0, OFX_UI_FONT_SMALL));
  _pUI->addSpacer(0, 12);

  //Content
  _pUI->addWidgetDown(new ofxUILabel("CONTENT", OFX_UI_FONT_MEDIUM));
  _pUI->addWidgetDown(new ofxUILabelButton("Add 3D Model...", false, 0, 30, 0, 0, OFX_UI_FONT_SMALL));
  _pUI->addWidgetDown(new ofxUILabelButton("2D Input...", false, 0, 30, 0, 0, OFX_UI_FONT_SMALL));
  _pUI->addWidgetDown(new ofxUILabelButton("Plugins...", false, 0, 30, 0, 0, OFX_UI_FONT_SMALL));
  _pUI->addSpacer(0, 12);

  //mix slider
  _pUI->addWidgetDown(new ofxUILabel("MIXER", OFX_UI_FONT_MEDIUM));
  //  _pUI->addWidgetDown(new ofxUIBiLabelSlider(0, 0, SIDEBAR_WIDTH-10, 30, 0, 100, _vidOverlay.getFaderValue()*100.0, "XFADE", "2D", "3D", OFX_UI_FONT_MEDIUM));
    
  //mix modes
  _mixModeNames.push_back("Underlay");
  _mixModeNames.push_back("Overlay");
  _mixModeNames.push_back("Mask");
  addRadioAndSetFirstItem(_pUI, "BLEND MODE", _mixModeNames, OFX_UI_ORIENTATION_VERTICAL, 16, 16);
  _pUI->addSpacer(0, 12);

  //map modes
  _mappingModeNames.push_back("Lat/Long");
  _mappingModeNames.push_back("Quadrants");
  _mappingModeNames.push_back("Fisheye");
  _mappingModeNames.push_back("Geodesic");
  _mappingModeNames.push_back("Cinematic");
  addRadioAndSetFirstItem(_pUI, "MAPPING MODE", _mappingModeNames, OFX_UI_ORIENTATION_VERTICAL, 16, 16);
  _pUI->addSpacer(0, 12);
    
  ofBackground(40, 20, 32);

  ofAddListener(_pUI->newGUIEvent, this, &radomeApp::guiEvent);
  ofAddListener(_pCalibrationUI->newGUIEvent, this, &radomeApp::guiEvent);
}

void radomeApp::prepDrawList()
{
  domeDrawIndex = glGenLists(1);
  std::vector<icosohedron::Triangle>::iterator i = _triangles.begin();

  glNewList(domeDrawIndex, GL_COMPILE);
  glBegin(GL_TRIANGLES);
  int sx = DOME_DIAMETER/2.0, sy = DOME_HEIGHT, sz = DOME_DIAMETER/2.0;
  while (i != _triangles.end())
    {
      icosohedron::Triangle& t = *i++;

      float dx, dy, dz;

      dx = t.v0[0] * sx;
      dy = t.v0[1] * sy;
      dz = t.v0[2] * sz;
      glNormal3f(dx, dy, dz);
      glVertex3f(dx, dy, dz);

      dx = t.v1[0] * sx;
      dy = t.v1[1] * sy;
      dz = t.v1[2] * sz;
      glNormal3f(dx, dy, dz);
      glVertex3f(dx, dy, dz);

      dx = t.v2[0] * sx;
      dy = t.v2[1] * sy;
      dz = t.v2[2] * sz;
      glNormal3f(dx, dy, dz);
      glVertex3f(dx, dy, dz);
    }
  glEnd();
  glEndList();
}

void radomeApp::loadFile() {
  ofFileDialogResult result = ofSystemLoadDialog("Load Model", false, "/Users/dewb/dev/of_v0073_osx_release/apps/video/radome/content");
    
  // Workaround for ofxFenster modal mouse event bug
  ofxFenster* pWin = ofxFensterManager::get()->getActiveWindow();
  ofxFenster* pDummy = ofxFensterManager::get()->createFenster(0,0,1,1);
  ofxFensterManager::get()->setActiveWindow(pDummy);
  ofxFensterManager::get()->setActiveWindow(pWin);
  ofxFensterManager::get()->deleteFenster(pDummy);
    
  if (result.bSuccess) {
    auto model = new radomeModel();
    model->loadModel(result.getPath());
    _modelList.push_back(model);
  }
}

void radomeApp::showProjectorWindow() {
  if (_projectorWindow && _projectorWindow->id != 0) {
    _projectorWindow->destroy();
    ofxFensterManager::get()->deleteFenster(_projectorWindow);
  }
  _projectorWindow = ofxFensterManager::get()->createFenster(400, 300, 750, 200, OF_WINDOW);
  _projectorWindow->addListener(new radomeProjectorWindowListener(&_projectorList));
  _projectorWindow->setWindowTitle("Projector Output");
}

void radomeApp::update() {
  //track clock time, update time, and update cube map according to 3d models
  unsigned long long time = ofGetSystemTime();
  _animationTime += (time - _lastSystemTime)/1000.0;
  _lastSystemTime = time;
    
  if (_animationTime >= 1.0) {
    _animationTime = 0.0;
  }
  for (auto iter = _modelList.begin(); iter != _modelList.end(); ++iter) {
    (*iter)->update(_animationTime);
  }
    
  updateCubeMap();
  //then update the projector
  updateProjectorOutput();
}

void radomeApp::updateCubeMap() {
  glEnable(GL_DEPTH_TEST);
  for(int i = 0; i < 6; i++) {
    _cubeMap.beginDrawingInto3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    ofClear(0,0,0,0);
    drawScene();
    _cubeMap.endDrawingInto3D();
  }
}

void radomeApp::updateProjectorOutput() {
  glEnable(GL_DEPTH_TEST);
  for (auto iter = _projectorList.begin(); iter != _projectorList.end(); ++iter) {
    (*iter)->renderBegin();

    beginShader();
    drawDome();
    endShader();

    (*iter)->renderEnd();
  }
}

void radomeApp::beginShader() {
  _shader.begin();
  _cubeMap.bind();

  _shader.setUniform1i("EnvMap", 0);
  _shader.setUniform1i("mixMode", _mixMode);
  _shader.setUniform1i("mappingMode", _mappingMode);
  _shader.setUniform1f("domeDiameter", DOME_DIAMETER*1.0);
  _shader.setUniform1f("domeHeight", DOME_HEIGHT*1.0);
    
  /*  if (_vidOverlay.maybeBind()) {
    _shader.setUniform1f("videoMix", _vidOverlay.getFaderValue());
    _shader.setUniform2f("videoSize", _vidOverlay.getWidth(), _vidOverlay.getHeight());
    _shader.setUniformTexture("video", _vidOverlay.getTexture(), _vidOverlay.getTextureId());
  } else {
    _shader.setUniform1f("videoMix", -1.0);
    _shader.setUniform2f("videoSize", 0.0, 0.0);
    _shader.setUniformTexture("video", _blankImage.getTextureReference(),
			      _blankImage.getTextureReference().getTextureData().textureID);
  }
  */
}

void radomeApp::endShader() {
  //  _vidOverlay.unbind();
  _cubeMap.unbind();
  _shader.end();
}

void radomeApp::draw() {
  switch (_displayMode) {
  case DisplayScene: {
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    _cam.setDistance(DOME_DIAMETER*1.6);
    _cam.begin();
                        
    ofPushMatrix();
    drawScene();
    ofPopMatrix();

    ofSetColor(128,128,255,128);
            
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    ofSetLineWidth(1);
    drawDome();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
    ofSetColor(80,80,192,128);
    drawGroundPlane();

    _cam.end();
            
    ofPopStyle();
  }
    break;
  case DisplayCubeMap: {
    ofSetColor(200,220,255);
    int margin = 2;
    int w = (ofGetWindowWidth() - SIDEBAR_WIDTH - margin*4) / 3;
    int h = (ofGetWindowHeight() - margin*3) / 2;
    for (int i = 0; i < 6; i++) {
      int x = margin + i%3 * (w + margin) + SIDEBAR_WIDTH;
      int y = margin + i/3 * (h + margin);
      ofDrawBitmapString(_cubeMap.getDescriptiveStringForFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), x+margin*1.5, y+10+margin*1.5);
      _cubeMap.drawFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , x, y, w, h);
      ofRect(x-1, y-1, w + margin, h + margin);
    }
  }
    break;
  case DisplayDome: {

    ofClear(20, 100, 50);
            
    _cam.setDistance(DOME_DIAMETER*1.10);
    _cam.begin();
            
    beginShader();
    drawDome();
    drawGroundPlane();
    endShader();
            
    for (auto iter = _projectorList.begin(); iter != _projectorList.end(); ++iter)
      {
	(*iter)->drawSceneRepresentation();
      }
            
    _cam.end();
  }
    break;
  case DisplayProjectorOutput:
  case LastDisplayMode: {
    ofSetColor(200,220,255);
    int margin = 2;
    int w = (ofGetWindowWidth() - SIDEBAR_WIDTH - margin*4) / 2;
    int h = (ofGetWindowHeight() - margin*3) / 2;
    auto iter = _projectorList.begin();
    for (int i = 0; iter != _projectorList.end(); i++, ++iter) {
      int x = margin + i%2 * (w + margin) + SIDEBAR_WIDTH;
      int y = margin + i/2 * (h + margin);
      (*iter)->drawFramebuffer(x, y, w, h);
      ofRect(x-1, y-1, w + margin, h + margin);
    }
  }
    break;            
  }
    
  glDisable(GL_DEPTH_TEST);
  _pUI->draw();
  if (_pCalibrationUI->isVisible())
    _pCalibrationUI->draw();
  glEnable(GL_DEPTH_TEST);
}

void radomeApp::drawScene() {
  ofSetColor(180, 192, 192);
  for (auto iter = _modelList.begin(); iter != _modelList.end(); ++iter)
    {
      (*iter)->draw();
    }
}

void radomeApp::drawDome() {
  double clipPlane[4] = { 0.0, 1.0, 0.0, 0.0 };
  glEnable(GL_CLIP_PLANE0);
  glClipPlane(GL_CLIP_PLANE0, clipPlane);
  glCallList(domeDrawIndex);
  glDisable(GL_CLIP_PLANE0);
}

void radomeApp::drawGroundPlane() {
  float size = DOME_DIAMETER * 5;
  float ticks = 40.0;
    
  float step = size / ticks;
  float major =  step * 2.0f;
    
  for (float k =- size; k <= size; k += step) {
    if (fabs(k) == size || k == 0)
      ofSetLineWidth(4);
    else if (k / major == floor(k / major))
      ofSetLineWidth(2);
    else
      ofSetLineWidth(1);
        
    ofLine(k, 0, -size, k, 0, size);
    ofLine(-size, 0, k, size, 0, k);
  }
}

void radomeApp::keyPressed(int key) {
  float accel = 3.0;
  auto model = *(_modelList.rbegin());
    
  switch (key) {
  case 'w': if (model) model->_origin.y += accel; break;
  case 's': if (model) model->_origin.y -= accel; break;
  case 'a': if (model) model->_origin.x -= accel; break;
  case 'd': if (model) model->_origin.x += accel; break;
  case 'z': if (model) model->_origin.z += accel; break;
  case 'x': if (model) model->_origin.z -= accel; break;
  case 'W': if (model) model->_origin.y += accel * 4; break;
  case 'S': if (model) model->_origin.y -= accel * 4; break;
  case 'A': if (model) model->_origin.x -= accel * 4; break;
  case 'D': if (model) model->_origin.x += accel * 4; break;
  case 'Z': if (model) model->_origin.z += accel * 4; break;
  case 'X': if (model) model->_origin.z -= accel * 4; break;
  case 'l': loadFile(); break;
  case 'm':
    {
      DisplayMode mode = getDisplayMode();
      mode++;
      changeDisplayMode(mode);
    }
    break;
  case 'f':
    {
      _fullscreen = !_fullscreen;
      ofSetFullscreen(_fullscreen);
    }
    break;
  case 'p':
    {
      showProjectorWindow();
    }
    break;
  case 'c':
    {
      if (_pCalibrationUI)
	_pCalibrationUI->setVisible(!_pCalibrationUI->isVisible());
    }
    break;
  case 'C':
    {
      if (_modelList.size() > 0)
	{
	  auto m = _modelList.back();
	  _modelList.pop_back();
	  if (m) delete(m);
	}
    }
    break;
  case 'r':
    {
      if (model) {
	if (model->getRotationIncrement() == 0) {
	  model->setRotationOrigin(model->getOrigin());
	  model->setRotation(ofVec4f(0.0,
				     frand_bounded(),
				     frand_bounded(),
				     frand_bounded()));
	}
	model->setRotationIncrement(model->getRotationIncrement() + 2.0);
	if (model->getRotationIncrement() == 10.0) {
	  model->setRotationIncrement(0.0);
	}
      }
    }
    break;
  }
}

void radomeApp::mousePressed(int x, int y, int button) {
  if ((x > SIDEBAR_WIDTH + 5) &&
      (!_pCalibrationUI || !_pCalibrationUI->isVisible() || x > (SIDEBAR_WIDTH + CALIBRATIONUI_WIDTH + 10)))
    _cam.mousePressed(x, y, button);
}

void radomeApp::mouseReleased(int x, int y, int button) {
  _cam.mouseReleased(x, y, button);
}

void radomeApp::mouseDragged(int x, int y, int button) {
  _cam.mouseDragged(x, y, button);
}

void radomeApp::mouseMoved(int x, int y) {
}

void radomeApp::changeDisplayMode(DisplayMode mode) {
  _displayMode = mode;
  if (_displayMode == LastDisplayMode)
    _displayMode = DisplayScene;
    
  ofxUIRadio* pRadio = dynamic_cast<ofxUIRadio*>(_pUI->getWidget("DISPLAY MODE"));
  if (pRadio) {
    pRadio->activateToggle(_displayModeNames[_displayMode]);
  }
}

void radomeApp::guiEvent(ofxUIEventArgs &e) {
  string name = e.widget->getName();
    
  int radio;
  if(matchRadioButton(name, _displayModeNames, &radio)) {
    changeDisplayMode((DisplayMode)radio);
    return;
  }
    
  if (matchRadioButton(name, _mixModeNames, &_mixMode))
    return;

  if (matchRadioButton(name, _mappingModeNames, &_mappingMode))
    return;
            
  if (name == "XFADE") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider) {
      //      _vidOverlay.setFaderValue(slider->getValue());
    }
  } else if (name == "Add 3D Model...") {
    auto pButton = dynamic_cast<ofxUIButton*>(e.widget);
    if (pButton && !pButton->getValue())
      {
	loadFile();
      }
  } else if (name == "Show Window") {
    auto pButton = dynamic_cast<ofxUIButton*>(e.widget);
    if (pButton && !pButton->getValue())
      {
	showProjectorWindow();
      }
  } else if (name == "Calibrate...") {
    auto pButton = dynamic_cast<ofxUIButton*>(e.widget);
    if (pButton && !pButton->getValue())
      {
	if (_pCalibrationUI) {
	  bool bVis = _pCalibrationUI->isVisible();
	  _pCalibrationUI->setVisible(!bVis);
	}
      }
  } else if (name == "PROJECTOR 1 HEIGHT") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 0 && _projectorList[0]) {
      _projectorList[0]->setHeight(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 1 HEADING") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 0 && _projectorList[0]) {
      _projectorList[0]->setHeading(slider->getScaledValue());
    }
  } else if (name == "PROJECTOR 1 DISTANCE") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 0 && _projectorList[0]) {
      _projectorList[0]->setDistance(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 1 FOV") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 0 && _projectorList[0]) {
      _projectorList[0]->setFOV(slider->getScaledValue());
    }
  } else if (name == "PROJECTOR 1 TARGET") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 0 && _projectorList[0]) {
      _projectorList[0]->setTargetHeight(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 2 HEIGHT") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 1 && _projectorList[1]) {
      _projectorList[1]->setHeight(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 2 HEADING") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 1 && _projectorList[1]) {
      _projectorList[1]->setHeading(slider->getScaledValue());
    }
  } else if (name == "PROJECTOR 2 DISTANCE") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 1 && _projectorList[1]) {
      _projectorList[1]->setDistance(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 2 FOV") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 1 && _projectorList[1]) {
      _projectorList[1]->setFOV(slider->getScaledValue());
    }
  } else if (name == "PROJECTOR 2 TARGET") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 1 && _projectorList[1]) {
      _projectorList[1]->setTargetHeight(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 3 HEIGHT") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 2 && _projectorList[2]) {
      _projectorList[2]->setHeight(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 3 HEADING") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 2 && _projectorList[2]) {
      _projectorList[2]->setHeading(slider->getScaledValue());
    }
  } else if (name == "PROJECTOR 3 DISTANCE") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 2 && _projectorList[2]) {
      _projectorList[2]->setDistance(slider->getScaledValue() * 10);
    }
  } else if (name == "PROJECTOR 3 FOV") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 2 && _projectorList[2]) {
      _projectorList[2]->setFOV(slider->getScaledValue());
    }
  } else if (name == "PROJECTOR 3 TARGET") {
    auto slider = dynamic_cast<ofxUISlider*>(e.widget);
    if (slider && _projectorList.size() > 2 && _projectorList[2]) {
      _projectorList[2]->setTargetHeight(slider->getScaledValue() * 10);
    }
  }
    
}
