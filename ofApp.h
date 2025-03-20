#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include  "ofxAssimpModelLoader.h"
#include "Octree.h"
#include "Particle.h"
#include "ParticleEmitter.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent2(ofDragInfo dragInfo);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	void drawAxis(ofVec3f);
	void savePicture();
	void toggleWireframeMode();
	void togglePointsDisplay();
	void toggleSelectTerrain();
	bool doPointSelection();
	void setCameraTarget();
	void initLightingAndMaterials();
	bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f& point);
	bool raySelectWithOctree(ofVec3f& pointRet);
	glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 p, glm::vec3 n);

	ofEasyCam cam;
	ofCamera bottomCam, TopDownCam, trackingCam, topCam, * currentCam;


	float rotation = 0.0;
	glm::vec3 velocity = glm::vec3(0, 0, 0);
	glm::vec3 acceleration = glm::vec3(0, 0, 0);
	glm::vec3 force = glm::vec3(0, 0, 0);
	glm::vec3 gravitationalForce;
	float terrainGravity = 1.62;
	float mass = 1.0;
	float damping = .99;

	float angularForce = 0;
	float angularVelocity = 0.0;
	float angularAcceleration = 0.0;
	bool bThrust = false;

	bool bStart = false;
	bool bOver = false;

	ofxAssimpModelLoader terrain, rocket;

	ofLight light;

	Box rocketBounds;
	Box testBox;
	vector<Box> colBoxList;
	bool bRocketSelected = false;
	Octree octree;
	TreeNode selectedNode;
	glm::vec3 mouseDownPos, mouseLastPos;
	bool bInDrag = false;

	ofxIntSlider numLevels;

	bool bAltKeyDown;
	bool bCtrlKeyDown;
	bool bWireframe;
	bool bDisplayPoints;
	bool bPointSelected;
	bool bHide;
	bool pointSelected = false;
	bool bDisplayLeafNodes = false;
	bool bDisplayOctree = false;
	bool bDisplayBBoxes = false;
	bool bgrounded = false;
	bool bRocketLoaded;
	bool bWin = false;
	bool bTerrainSelected;
	bool bWinSoundPlayed = false;
	bool bCrashSoundPlayed = false;
	bool bNoFuelSoundPlayed = false;
	float startFuel;
	float startThrustTime;
	float totalThrustTime;
	float maxThrustTime;

	float distanceToGround = 0.0;
	float altitude = 0.0;
	float minTerrainY = 0.0;

	ofVec3f selectedPoint;
	ofVec3f intersectPoint;

	vector<Box> bboxList;

	const float selectionRange = 4.0;

	int startTime;
	int timer;

	int i = 0;
	int landingZone = 0;

	int maxRotations = 4;
	int currentRotations = 0;
	glm::vec3 initialHeading = glm::vec3(0, 0, 1);

	ofxPanel gui;
	ofxFloatSlider thrust, camDist, camNearClip, camSetFOV;


	glm::vec3 rocketPosition;

	bool rotateX = false;
	bool rotateY = false;
	bool rotateZ = false;
	void ofApp::integrate() {
		float framerate = 60;
		float dt = 1.0 / framerate;
		rocketPosition = rocket.getPosition();


		rocketPosition += velocity * dt;


		glm::vec3 accel = acceleration;
		accel += ((force * 1.0) + gravitationalForce / mass);
		velocity += accel * dt;
		velocity *= damping;

		rotation += (angularVelocity * dt);

		rocket.setRotation(0, rotation, 0, 1, 0);
		float a = angularAcceleration;
		a += (angularForce / mass);
		angularVelocity += a * dt;
		angularVelocity *= damping;

		rocket.setPosition(rocketPosition.x, rocketPosition.y, rocketPosition.z);
	}

	void checkCollisions();
	void drawText();

	int impactForce = 0;

	int fuel = 0;
	bool noFuel = false;

	ParticleEmitter emitter;
	ParticleEmitter explosion;

	TurbulenceForce* tForce;
	GravityForce* gForce;
	ImpulseRadialForce* iForce;

	ofLight keyLight, rimLight, fillLight, dynamicLight;
	vector<ofLight*> Lights;

	ofSoundPlayer backgroundMusic;
	ofSoundPlayer thrustSound;
	ofSoundPlayer crashSound;
	ofSoundPlayer winSound;


	ofImage background;

	ofVbo vbo;
	ofShader shader;
	ofTexture particleTex;
	void loadVbo();
	int radius = 0;

	ofSpherePrimitive sphere;
	ofTexture spaceTexture;

	struct LandingZone {
		glm::vec3 center;
		float radius;
	};

	LandingZone landingZones[3];
	bool bCrashInLZ = false;

	int score;

	bool bDisplayAltitude = true;

};