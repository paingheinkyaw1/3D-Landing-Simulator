#include "ofApp.h"
#include "Util.h"
#include <glm/gtx/intersect.hpp>

//Pierce Kyaw, Aye Thwe Tun
void ofApp::setup() {
    // Initialize score and various state variables
    score = 0;

    bWireframe = false;
    bDisplayPoints = false;
    bAltKeyDown = false;
    bCtrlKeyDown = false;
    bRocketLoaded = false;
    bTerrainSelected = true;
    gravitationalForce = glm::vec3(0, -terrainGravity, 0);

    radius = 5;

    ofSetVerticalSync(true);
    cam.disableMouseInput();
    ofEnableSmoothing();
    ofEnableDepthTest();

    rotation = 0.0;

    // Since we are loading textures, turn off arbitrary textures
    ofDisableArbTex();

    fuel = 120;
    startFuel = 120;
    maxThrustTime = 120.0f;
    totalThrustTime = 0.0f;
    startThrustTime = 0.0f;

    // Initialize lighting and materials
    initLightingAndMaterials();

    // Load background image
    background.loadImage("images/space.jpg");

    printf("Map loaded. Creating Octree....\n");

    // Load the terrain model and create an octree for spatial queries
    if (terrain.loadModel("geo/moonterrain.obj"))
    {
        terrain.setScaleNormalization(false);
        printf("Map loaded, creating octree...\n");

        octree.create(terrain.getMesh(0), 20);
        printf("Octree created!\n");
    }
    else
    {
        printf("Map not loaded.\n");
        ofExit(0);
    }

    // Load the rocket model
    if (rocket.loadModel("geo/rocket.obj"))
    {
        printf("Rocket Loaded!\n");
        rocket.setScale(0.010, 0.01, 0.01);
        rocket.setRotation(0, 0.0, 0, 1, 0);
        rocket.setPosition(0, 30, 0);
        rocket.update();
        bRocketLoaded = true;
    }
    else
    {
        printf("Rocket not loaded.\n");
        ofExit(0);
    }

    // Load sound effects and background music
    if (backgroundMusic.load("sounds/Background.mp3") && crashSound.load("sounds/Crash.mp3")
        && thrustSound.load("sounds/Thrusters.mp3")
        && winSound.load("sounds/Win.mp3")) {

        backgroundMusic.setVolume(0.5);
        thrustSound.setVolume(1);
        crashSound.setVolume(0.5);
        winSound.setVolume(1);
        cout << "Sounds Loaded" << endl;
    }
    else
    {
        cout << "Sounds not Loaded" << endl;
    }

    // Load and setup shaders
    if ((shader.load("shaders/shader")) && (shader.load("shaders_gles/shader"))) {
        cout << "Shaders loaded" << endl;
    }
    else {
        cout << "Shaders not Loaded" << endl;
    }

    // Load particle texture
    if (!ofLoadImage(particleTex, "images/dot.png")) {
        cout << "Particle Texture File: images/dot.png not found" << endl;
        ofExit();
    }

    // Hide GUI initially
    bHide = false;

    cout << "Number of Verts: " << terrain.getMesh(0).getNumVertices() << endl;

    // Test box for debugging (not used in final game)
    testBox = Box(Vector3(3, 3, 0), Vector3(5, 5, 2));

    float time = ofGetElapsedTimef();

    // GUI setup
    gui.setup();
    gui.add(thrust.setup("Thrust", 100, 1, 1000));
    gui.add(numLevels.setup("Number of Octree Levels", 1, 1, 10));

    // Set up forces for particle systems
    // Turbulence force
    tForce = new TurbulenceForce(ofVec3f(-20, -20, -20), ofVec3f(20, 20, 20));
    // Gravity force
    gForce = new GravityForce(ofVec3f(0, -10, 0));
    // Radial force for explosions
    iForce = new ImpulseRadialForce(1000.0);

    // Create an Emitter for the rocket's engine
    emitter.setOneShot(true);
    emitter.setEmitterType(RadialEmitter);
    emitter.setGroupSize(10);
    emitter.particleRadius = .0001;
    emitter.spawn(1);
    emitter.setParticleRadius(0.5);
    emitter.setRate(0.5);
    emitter.setLifespan(1.0);
    emitter.sys->addForce(tForce);
    emitter.sys->addForce(gForce);
    emitter.setVelocity(ofVec3f(0, 5, 0));

    // Emitter for explosions
    explosion.setOneShot(true);
    explosion.setEmitterType(RadialEmitter);
    explosion.setGroupSize(100);
    explosion.setParticleRadius(0.5);
    explosion.setLifespan(2.0);
    explosion.sys->addForce(iForce);

    glm::vec3 rocketPos = rocket.getPosition();

    // Setup main camera
    cam.setPosition(rocketPos.x - 20, rocketPos.y + 30, rocketPos.z - 60);
    cam.lookAt(rocketPos);
    cam.setDistance(80);
    cam.setNearClip(0.1);
    cam.setFov(65.5);

    // Tracking camera setup
    trackingCam.setPosition(rocketPos.x + 12, rocketPos.y + 6, rocketPos.z - 80);
    trackingCam.lookAt(rocketPos);
    trackingCam.setNearClip(.1);

    // Bottom camera setup - placed under rocket looking up
    bottomCam.setPosition(rocketPos.x, rocketPos.y + 10, rocketPos.z);
    bottomCam.lookAt(rocketPos, glm::vec3(0, 0, 1));

    // TopDown camera setup - directly above rocket looking down
    TopDownCam.setPosition(rocketPos.x, rocketPos.y + 30, rocketPos.z);
    TopDownCam.lookAt(rocketPos, glm::vec3(0, 0, 1));
    TopDownCam.setNearClip(.1);

    currentCam = &cam;

    // Dynamic light setup
    dynamicLight.setup();
    dynamicLight.enable();
    dynamicLight.setSpotlight();
    dynamicLight.setScale(1);
    dynamicLight.setSpotlightCutOff(10);
    dynamicLight.setAttenuation(.2, .001, .001);
    dynamicLight.setAmbientColor(ofFloatColor(1, 1, 1));
    dynamicLight.setDiffuseColor(ofFloatColor(1, 1, 1));
    dynamicLight.setSpecularColor(ofFloatColor(1, 1, 1));
    dynamicLight.rotate(180, ofVec3f(0, 1, 0));
    dynamicLight.setPosition(rocket.getPosition() + glm::vec3(0, 10, 0));
    dynamicLight.rotate(90, ofVec3f(1, 0, 0));

    // Compute minimum terrain Y coordinate
    ofMesh terrainMesh = terrain.getMesh(0);
    float minY = std::numeric_limits<float>::max();
    for (int i = 0; i < terrainMesh.getNumVertices(); i++) {
        glm::vec3 v = terrainMesh.getVertex(i);
        if (v.y < minY) minY = v.y;
    }
    minTerrainY = minY;
    int totalVerts = terrainMesh.getNumVertices();

    // Randomly select landing zones on the terrain
    for (int i = 0; i < 3; i++) {
        int randomIndex = (int)ofRandom(0, totalVerts);
        glm::vec3 randomVertex = terrainMesh.getVertex(randomIndex);
        Ray groundRay(Vector3(randomVertex.x, randomVertex.y + 200, randomVertex.z), Vector3(0, -1, 0));
        TreeNode groundNode;

        if (octree.intersect(groundRay, octree.root, groundNode)) {
            glm::vec3 groundPoint = octree.mesh.getVertex(groundNode.points[0]);
            landingZones[i].center = groundPoint;
        }
        else {
            landingZones[i].center = randomVertex;
        }

        landingZones[i].radius = 5.0;
    }
}

//Pierce Kyaw, Aye Thwe Tun
void ofApp::update() {
    // If the game has started
    if (bStart) {
        // If the game is not over, check collisions
        if (!bOver) {
            checkCollisions();
        }

        // Update emitters for engine and explosions
        emitter.update();
        explosion.update();

        // If the rocket is on the ground, stop its movement
        if (bgrounded) {
            velocity = glm::vec3(0, 0, 0);
            acceleration = glm::vec3(0, 0, 0);
            force = glm::vec3(0, 0, 0);
        }

        // Manage thrust and fuel consumption
        if (bStart && bThrust && !bOver && fuel > 0) {
            float currentThrustDuration = ofGetElapsedTimef() - startThrustTime;

            totalThrustTime += currentThrustDuration;
            startThrustTime = ofGetElapsedTimef();

            fuel = (1.0f - (totalThrustTime / maxThrustTime)) * 120;

            // If fuel runs out
            if (fuel <= 0 || totalThrustTime >= maxThrustTime) {
                fuel = 0;
                velocity = glm::vec3(0, 0, 0);
                acceleration = glm::vec3(0, 0, 0);
                force = glm::vec3(0, 0, 0);
                bOver = true;
                noFuel = true;
            }
        }

        // Update the timer when thrust is applied and game is not over
        if (!bOver && bThrust) {
            int tempTime = ofGetElapsedTimeMillis() / 1000;
            timer = tempTime - startTime;
        }

        // Ensure background music plays continuously
        if (!backgroundMusic.isPlaying()) backgroundMusic.play();

        // Update camera and light positions relative to rocket
        trackingCam.lookAt(rocketPosition);
        bottomCam.setPosition(rocketPosition.x, rocketPosition.y - 5, rocketPosition.z);
        TopDownCam.setPosition(rocketPosition.x, rocketPosition.y + 10, rocketPosition.z);
        dynamicLight.setPosition(rocket.getPosition() + glm::vec3(0, 10, 0));

        // Calculate rocket's altitude
        Ray altitudeRay = Ray(Vector3(rocket.getPosition().x, rocket.getPosition().y, rocket.getPosition().z),
            Vector3(rocket.getPosition().x, rocket.getPosition().y - 200, rocket.getPosition().z));
        TreeNode altNode;
        if (octree.intersect(altitudeRay, octree.root, altNode)) {
            distanceToGround = glm::length(octree.mesh.getVertex(altNode.points[0]) - rocket.getPosition());
        }

        altitude = rocket.getPosition().y - minTerrainY;

        // Update explosion position
        glm::vec3 pos = rocket.getPosition();
        explosion.setPosition(glm::vec3(pos.x, pos.y, pos.z));

        // Update emitter position (for engine particles)
        glm::vec3 rocketMin = rocket.getSceneMin() + rocket.getPosition();
        glm::vec3 emitterPos = rocket.getPosition();
        emitterPos.y = rocketMin.y;
        emitter.setPosition(emitterPos);

        // Integrate to update physics (position, velocity, etc.)
        integrate();

        // Reset force for next frame
        force = glm::vec3(0, 0, 0);
    }
}

//Pierce Kyaw, Aye Thwe Tun
void ofApp::draw() {
    // Load the VBO for particles
    loadVbo();

    glDepthMask(false);
    ofBackground(ofColor::black);

    // Draw the background image
    background.draw(0, 0, ofGetWidth(), ofGetHeight());

    glDepthMask(true);

    currentCam->begin();

    // Draw explosion particles
    explosion.draw();

    ofPushMatrix();

    // If wireframe mode is enabled
    if (bWireframe) {
        ofDisableLighting();
        ofSetColor(ofColor::slateGray);
        terrain.drawWireframe();
        if (bRocketLoaded) {
            rocket.drawWireframe();
            if (!bTerrainSelected) drawAxis(rocket.getPosition());
        }
        if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
    }
    else {
        // Draw terrain and rocket with lighting
        ofEnableLighting();
        terrain.drawFaces();
        if (bRocketLoaded) {
            rocket.drawFaces();
            if (!bTerrainSelected) drawAxis(rocket.getPosition());
            if (bDisplayBBoxes) {
                // Draw bounding boxes for rocket meshes
                ofNoFill();
                ofSetColor(ofColor::white);
                for (int i = 0; i < rocket.getNumMeshes(); i++) {
                    ofPushMatrix();
                    ofMultMatrix(rocket.getModelMatrix());
                    ofRotate(-90, 1, 0, 0);
                    Octree::drawBox(bboxList[i]);
                    ofPopMatrix();
                }
            }

            if (bRocketSelected) {
                // Draw bounding box around the rocket
                ofVec3f min = rocket.getSceneMin() + rocket.getPosition();
                ofVec3f max = rocket.getSceneMax() + rocket.getPosition();

                glm::vec3 center = (min + max) / 2.0f;
                float shrinkFactor = 0.8;
                glm::vec3 halfSize = (max - min) / 2.0f * shrinkFactor;

                min = center - halfSize;
                max = center + halfSize;

                rocketBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

                ofSetColor(ofColor::green);
                ofNoFill();
                Octree::drawBox(rocketBounds);

                ofSetColor(ofColor::lightBlue);
                for (int i = 0; i < colBoxList.size(); i++) {
                    Octree::drawBox(colBoxList[i]);
                }
            }
        }
    }
    if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));

    // If displaying points of terrain
    if (bDisplayPoints) {
        glPointSize(3);
        ofSetColor(ofColor::green);
        terrain.drawVertices();
    }

    // Draw selected point if any
    if (bPointSelected) {
        ofSetColor(ofColor::blue);
        ofDrawSphere(selectedPoint, .1);
    }

    // Draw emitter particles (engine flames, etc.)
    glDepthMask(GL_FALSE);
    ofSetColor(ofColor::orange);

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofEnablePointSprites();

    shader.begin();

    particleTex.bind();
    vbo.draw(GL_POINTS, 0, (int)emitter.sys->particles.size());
    emitter.draw();
    particleTex.unbind();

    shader.end();

    ofDisablePointSprites();
    ofDisableBlendMode();
    glDepthMask(GL_TRUE);
    ofEnableAlphaBlending();

    ofDisableLighting();
    int level = 0;

    // Display Octree if enabled
    if (bDisplayLeafNodes) {
        octree.drawLeafNodes(octree.root);
        cout << "num leaf: " << octree.numLeaf << endl;
    }
    else if (bDisplayOctree) {
        ofNoFill();
        ofSetColor(ofColor::white);
        octree.draw(numLevels, 0);
    }

    // Draw selected node if a point is selected
    if (pointSelected) {
        ofVec3f p = octree.mesh.getVertex(selectedNode.points[0]);
        ofVec3f d = p - cam.getPosition();
        ofSetColor(ofColor::lightGreen);
        ofDrawSphere(p, .02 * d.length());
    }

    ofDisableDepthTest();

    // Draw landing zones
    for (int i = 0; i < 3; i++) {
        ofPushMatrix();
        ofTranslate(landingZones[i].center);
        ofSetColor(ofColor::blue);
        ofRotateXDeg(-90);
        ofNoFill();
        ofDrawCircle(0, 0, landingZones[i].radius);
        ofPopMatrix();
    }

    currentCam->end();

    // Draw GUI if not hidden
    if (!bHide) {
        ofDisableDepthTest();
        gui.draw();
        ofEnableDepthTest();
    }

    // Instructions screen before the game starts
    if (!bStart) {
        ofSetColor(ofColor::white);
        float startX = (ofGetWindowWidth() / 2) - 120;
        float startY = (ofGetWindowHeight() / 2) - 100;
        float lineHeight = 20;

        ofDrawBitmapString("=============== INSTRUCTIONS ===============", startX, startY);
        ofDrawBitmapString("Press [Spacebar] to Start the Game", startX, startY + lineHeight * 2);

        ofDrawBitmapString("---------- MOVEMENT ----------", startX, startY + lineHeight * 4);
        ofDrawBitmapString("[W]: Forward    [S]: Backward", startX, startY + lineHeight * 5);
        ofDrawBitmapString("[A]: Left       [D]: Right", startX, startY + lineHeight * 6);
        ofDrawBitmapString("[Q]: Up         [E]: Down", startX, startY + lineHeight * 7);

        ofDrawBitmapString("-------- ROTATION & THRUST --------", startX, startY + lineHeight * 9);
        ofDrawBitmapString("[O]: Rotate Clockwise  [P]: Rotate Counter-Clockwise", startX, startY + lineHeight * 10);
        ofDrawBitmapString("[T]: Increase Thrust   [Y]: Decrease Thrust", startX, startY + lineHeight * 11);

        ofDrawBitmapString("----------- CAMERA VIEWS -----------", startX, startY + lineHeight * 13);
        ofDrawBitmapString("[1]: Free Cam      [2]: Tracking Cam", startX, startY + lineHeight * 14);
        ofDrawBitmapString("[3]: Bottom Cam    [4]: TopDown Cam", startX, startY + lineHeight * 15);
        ofDrawBitmapString("[5]: Camera to Rocket", startX, startY + lineHeight * 16);
    }

    // Display text (fuel, altitude, score) during gameplay
    if (bStart && !bOver)
    {
        drawText();
    }

    // If game over, display appropriate end game messages
    if (bOver) {
        ofSetColor(ofColor::white);

        string altitudeMsg = "Altitude: " + std::to_string(altitude);
        string scoreMessage = "Your Score: " + std::to_string(score);
        float charWidth = 8.0f;

        if (bWin && bgrounded) {
            // Successful landing scenario
            string mainMsg = "CONGRATULATIONS! You landed safely!";

            float mainWidth = mainMsg.length() * charWidth;
            float scoreWidth = scoreMessage.length() * charWidth;
            float altitudeWidth = altitudeMsg.length() * charWidth;

            float mainX = (ofGetWindowWidth() - mainWidth) / 2;
            float mainY = ofGetWindowHeight() / 2 - 40;
            float scoreX = (ofGetWindowWidth() - scoreWidth) / 2;
            float scoreY = ofGetWindowHeight() / 2 - 10;
            float altX = (ofGetWindowWidth() - altitudeWidth) / 2;
            float altY = ofGetWindowHeight() / 2 + 20;

            ofSetColor(ofColor::green);
            ofDrawBitmapString(mainMsg, mainX, mainY);
            ofSetColor(ofColor::white);
            ofDrawBitmapString(scoreMessage, scoreX, scoreY);
            ofDrawBitmapString(altitudeMsg, altX, altY);

        }
        else if (bgrounded && !bWin && !noFuel) {
            // Crash scenarios
            if (bCrashInLZ) {
                // Crashed inside landing zone
                string gameOver = "GAME OVER! Almost there!";
                string crashMessage = "You crash-landed in the landing area!";
                string impactForceMsg = "Impact Force: " + std::to_string(impactForce);

                float goWidth = gameOver.length() * charWidth;
                float crashWidth = crashMessage.length() * charWidth;
                float scoreWidth = scoreMessage.length() * charWidth;
                float altWidth = altitudeMsg.length() * charWidth;
                float impactWidth = impactForceMsg.length() * charWidth;

                float goX = (ofGetWindowWidth() - goWidth) / 2;
                float goY = ofGetWindowHeight() / 2 - 60;
                float crashX = (ofGetWindowWidth() - crashWidth) / 2;
                float crashY = ofGetWindowHeight() / 2 - 30;
                float scoreX = (ofGetWindowWidth() - scoreWidth) / 2;
                float scoreY = ofGetWindowHeight() / 2;
                float altX = (ofGetWindowWidth() - altWidth) / 2;
                float altY = ofGetWindowHeight() / 2 + 30;
                float impactX = (ofGetWindowWidth() - impactWidth) / 2;
                float impactY = ofGetWindowHeight() / 2 + 60;

                ofDrawBitmapString(gameOver, goX, goY);
                ofDrawBitmapString(crashMessage, crashX, crashY);
                ofDrawBitmapString(scoreMessage, scoreX, scoreY);
                ofDrawBitmapString(altitudeMsg, altX, altY);
                ofDrawBitmapString(impactForceMsg, impactX, impactY);
            }
            else {
                // Crashed outside the landing zone
                string gameOver = "GAME OVER!";
                string crashMessage = "You crashed!";
                string impactForceMsg = "Impact Force: " + std::to_string(impactForce);

                float goWidth = gameOver.length() * charWidth;
                float crashWidth = crashMessage.length() * charWidth;
                float scoreWidth = scoreMessage.length() * charWidth;
                float altWidth = altitudeMsg.length() * charWidth;
                float impactWidth = impactForceMsg.length() * charWidth;

                float goX = (ofGetWindowWidth() - goWidth) / 2;
                float goY = ofGetWindowHeight() / 2 - 60;
                float crashX = (ofGetWindowWidth() - crashWidth) / 2;
                float crashY = ofGetWindowHeight() / 2 - 30;
                float scoreX = (ofGetWindowWidth() - scoreWidth) / 2;
                float scoreY = ofGetWindowHeight() / 2;
                float altX = (ofGetWindowWidth() - altWidth) / 2;
                float altY = ofGetWindowHeight() / 2 + 30;
                float impactX = (ofGetWindowWidth() - impactWidth) / 2;
                float impactY = ofGetWindowHeight() / 2 + 60;

                ofDrawBitmapString(gameOver, goX, goY);
                ofDrawBitmapString(crashMessage, crashX, crashY);
                ofDrawBitmapString(scoreMessage, scoreX, scoreY);
                ofDrawBitmapString(altitudeMsg, altX, altY);
                ofDrawBitmapString(impactForceMsg, impactX, impactY);
            }
        }
        else if (noFuel) {
            // Out of Fuel scenario
            string gameOver = "GAME OVER!";
            string fuelMessage = "Out of Fuel";

            float goWidth = gameOver.length() * charWidth;
            float fuelWidth = fuelMessage.length() * charWidth;
            float scoreWidth = scoreMessage.length() * charWidth;
            float altWidth = altitudeMsg.length() * charWidth;

            float goX = (ofGetWindowWidth() - goWidth) / 2;
            float goY = ofGetWindowHeight() / 2 - 60;
            float fuelX = (ofGetWindowWidth() - fuelWidth) / 2;
            float fuelY = ofGetWindowHeight() / 2 - 30;
            float scoreX = (ofGetWindowWidth() - scoreWidth) / 2;
            float scoreY = ofGetWindowHeight() / 2;
            float altX = (ofGetWindowWidth() - altWidth) / 2;
            float altY = ofGetWindowHeight() / 2 + 30;

            ofDrawBitmapString(gameOver, goX, goY);
            ofDrawBitmapString(fuelMessage, fuelX, fuelY);
            ofDrawBitmapString(scoreMessage, scoreX, scoreY);
            ofDrawBitmapString(altitudeMsg, altX, altY);
        }
    }
}


// Draw XYZ axis for reference
void ofApp::drawAxis(ofVec3f location) {
    ofPushMatrix();
    ofTranslate(location);

    ofSetLineWidth(1.0);

    // X Axis (Red)
    ofSetColor(ofColor(255, 0, 0));
    ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));

    // Y Axis (Green)
    ofSetColor(ofColor(0, 255, 0));
    ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

    // Z Axis (Blue)
    ofSetColor(ofColor(0, 0, 255));
    ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

    ofPopMatrix();
}

//Pierce Kyaw, Aye Thwe Tun
void ofApp::keyPressed(int key) {
    glm::vec3 rocketPosition = rocket.getPosition();
    switch (key) {
    case '1':
        // Switch to main camera
        currentCam = &cam;
        break;
    case '2':
        // Switch to tracking camera
        currentCam = &trackingCam;
        break;
    case '3':
        // Switch to bottom camera
        currentCam = &bottomCam;
        break;
    case '4':
        // Switch to top-down camera
        currentCam = &TopDownCam;
        break;
    case '5':
        // Adjust main camera to look at rocket
        cam.lookAt(rocketPosition);
        break;
    case 'c':
    case 'C':
        // Toggle camera mouse input
        if (cam.getMouseInputEnabled())
            cam.disableMouseInput();
        else
            cam.enableMouseInput();
        break;
    case 'f':
    case 'F':
        // Toggle fullscreen mode
        ofToggleFullscreen();
        break;
    case 'h':
    case 'H':
        // Toggle GUI visibility
        bHide = !bHide;
        break;
    case 'n':
    case 'N':
        // Display leaf nodes of octree
        bDisplayLeafNodes = !bDisplayLeafNodes;
        break;
    case 'b':
    case 'B':
        // Display octree
        bDisplayOctree = !bDisplayOctree;
        break;
    case 'r':
        // Reset the camera
        cam.reset();
        break;
    case 'v':
        // Toggle points display
        togglePointsDisplay();
        break;

        // Movement and thrust controls
    case 'w':
    case 'W':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            emitter.sys->reset();
            emitter.start();
            bThrust = true;
            force = float(thrust) * ofVec3f(0, 0, 1); // Forward
            if (!thrustSound.isPlaying()) thrustSound.play();
        }
        break;
    case 's':
    case 'S':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            emitter.sys->reset();
            emitter.start();
            bThrust = true;
            force = float(thrust) * ofVec3f(0, 0, -1); // Backward
            if (!thrustSound.isPlaying()) thrustSound.play();
        }
        break;
    case 'a':
    case 'A':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            emitter.sys->reset();
            emitter.start();
            bThrust = true;
            force = float(thrust) * ofVec3f(1, 0, 0); // Left
            if (!thrustSound.isPlaying()) thrustSound.play();
        }
        break;
    case 'd':
    case 'D':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            emitter.sys->reset();
            emitter.start();
            bThrust = true;
            force = float(thrust) * ofVec3f(-1, 0, 0); // Right
            if (!thrustSound.isPlaying()) thrustSound.play();
        }
        break;
    case 'q':
    case 'Q':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            emitter.sys->reset();
            emitter.start();
            bThrust = true;
            force = float(thrust) * ofVec3f(0, 1, 0); // Up
            if (!thrustSound.isPlaying()) thrustSound.play();
        }
        break;
    case 'e':
    case 'E':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            emitter.sys->reset();
            emitter.start();
            bThrust = true;
            force = float(thrust) * ofVec3f(0, -1, 0); // Down
            if (!thrustSound.isPlaying()) thrustSound.play();
        }
        break;

        // Rotation controls
    case 'o':
    case 'O':
        if (fuel > 0) {
            if (!thrustSound.isPlaying()) thrustSound.play();
            bThrust = true;
            angularForce += -10.0f; // Rotate clockwise
        }
        break;
    case 'p':
    case 'P':
        if (fuel > 0) {
            if (!bThrust) startThrustTime = ofGetElapsedTimef();
            if (!thrustSound.isPlaying()) thrustSound.play();
            bThrust = true;
            angularForce += 10.0f; // Rotate counter-clockwise
        }
        break;
    case ' ':
        if (bOver) {
            // Reset the game if it ended
            bOver = false;
            bWin = false;
            noFuel = false;
            bgrounded = false;
            bThrust = false;

            // Reset fuel and thrust
            fuel = startFuel;
            startThrustTime = ofGetElapsedTimef();

            // Reset rocket state
            rocket.setPosition(0, 30, 0);
            rocket.setRotation(0, 180.0, 0, 1, 0);
            rocket.update();

            // Reset physics
            velocity = glm::vec3(0, 0, 0);
            acceleration = glm::vec3(0, 0, 0);
            force = glm::vec3(0, 0, 0);

            // Reset particles and sounds
            explosion.stop();
            explosion.sys->reset();
            crashSound.stop();
            winSound.stop();

            // Re-randomize landing zones
            ofMesh terrainMesh = terrain.getMesh(0);
            int totalVerts = terrainMesh.getNumVertices();
            for (int i = 0; i < 3; i++) {
                int randomIndex = (int)ofRandom(0, totalVerts);
                glm::vec3 randomVertex = terrainMesh.getVertex(randomIndex);
                landingZones[i].center = randomVertex;
                landingZones[i].radius = 5.0;
            }

            // Start game again
            bStart = true;

        }
        else if (!bStart) {
            // Start the game initially
            bStart = true;
        }
        break;
    case 'x':
    case 'X':
        // Toggle altitude display
        bDisplayAltitude = !bDisplayAltitude;
        break;
    case 't':
    case 'T':
        // Increase thrust
        thrust = thrust + 5;
        break;
    case 'y':
    case 'Y':
        // Decrease thrust
        thrust = thrust - 5;
        break;

    default:
        break;
    }
}

void ofApp::toggleWireframeMode() {
    bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
    bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
    bDisplayPoints = !bDisplayPoints;
}

//Pierce Kyaw, Aye Thwe Tun
void ofApp::keyReleased(int key) {

    switch (key)
    {
        // Stop thrust when keys are released
    case 'w':
    case 'W':
        bThrust = false;
        force = glm::vec3(0, 0, 0);
        thrustSound.stop();
        break;
    case 's':
    case 'S':
        bThrust = false;
        force = glm::vec3(0, 0, 0);
        thrustSound.stop();
        break;
    case 'a':
    case 'A':
        bThrust = false;
        force = glm::vec3(0, 0, 0);
        thrustSound.stop();
        break;
    case 'd':
    case 'D':
        bThrust = false;
        force = glm::vec3(0, 0, 0);
        thrustSound.stop();
        break;
    case 'q':
    case 'Q':
        bThrust = false;
        force = glm::vec3(0, 0, 0);
        thrustSound.stop();
        break;
    case 'e':
    case 'E':
        bThrust = false;
        force = glm::vec3(0, 0, 0);
        thrustSound.stop();
        break;

    case OF_KEY_ALT:
        cam.disableMouseInput();
        bAltKeyDown = false;
        break;

    case 'o':
    case 'O':
        thrustSound.stop();
        bThrust = false;
        angularForce = 0;
        break;
    case 'p':
    case 'P':
        bThrust = false;
        thrustSound.stop();
        angularForce = 0;
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
    // Currently unused
}

//Pierce Kyaw, Aye Thwe Tun
void ofApp::mousePressed(int x, int y, int button) {
    // If camera mouse input is enabled, do not select
    if (cam.getMouseInputEnabled()) return;

    if (bRocketLoaded) {
        // Raycast from mouse to see if rocket is selected
        glm::vec3 origin = cam.getPosition();
        glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
        glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

        ofVec3f min = rocket.getSceneMin() + rocket.getPosition();
        ofVec3f max = rocket.getSceneMax() + rocket.getPosition();

        Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
        bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
        if (hit) {
            bRocketSelected = true;
            mouseDownPos = getMousePointOnPlane(rocket.getPosition(), cam.getZAxis());
            mouseLastPos = mouseDownPos;
            bInDrag = true;
        }
        else {
            bRocketSelected = false;
        }
    }
    else {
        // If rocket not loaded, attempt to select a point on the terrain
        ofVec3f p;
        raySelectWithOctree(p);
    }
}

//Pierce Kyaw, Aye Thwe Tun
bool ofApp::raySelectWithOctree(ofVec3f& pointRet) {
    // Convert screen coordinates to world ray
    ofVec3f mouse(mouseX, mouseY);
    ofVec3f rayPoint = cam.screenToWorld(mouse);
    ofVec3f rayDir = rayPoint - cam.getPosition();
    rayDir.normalize();
    Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
        Vector3(rayDir.x, rayDir.y, rayDir.z));

    // Check intersection with octree
    pointSelected = octree.intersect(ray, octree.root, selectedNode);

    if (pointSelected) {
        pointRet = octree.mesh.getVertex(selectedNode.points[0]);
        cout << "POINT RET:" << pointRet << endl;
    }
    return pointSelected;
}

//Pierce Kyaw, Aye Thwe Tun
void ofApp::mouseDragged(int x, int y, int button) {
    // If camera mouse input is enabled, do not drag
    if (cam.getMouseInputEnabled()) return;

    if (bInDrag) {
        // Dragging the rocket
        glm::vec3 rocketPos = rocket.getPosition();
        glm::vec3 mousePos = getMousePointOnPlane(rocketPos, cam.getZAxis());
        glm::vec3 delta = mousePos - mouseLastPos;

        rocketPos += delta;
        rocket.setPosition(rocketPos.x, rocketPos.y, rocketPos.z);
        rocket.setRotation(0, rotation, 0, 1, 0);
        mouseLastPos = mousePos;

        ofVec3f min = rocket.getSceneMin() + rocket.getPosition();
        ofVec3f max = rocket.getSceneMax() + rocket.getPosition();

        Box rocketBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

        colBoxList.clear();

        octree.intersect(rocketBounds, octree.root, colBoxList);

        if (rocketBounds.overlap(testBox)) {
            cout << "overlap" << endl;
        }
        else {
            cout << "OK" << endl;
        }
    }
    else {
        // If not dragging rocket, try to select a point
        ofVec3f p;
        raySelectWithOctree(p);
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    bInDrag = false;
}

//--------------------------------------------------------------
void ofApp::setCameraTarget() {
    // Not used
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
    // Not used
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
    // Not used
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
    // Not used
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
    // Not used
}

// Save a screenshot
void ofApp::savePicture() {
    ofImage picture;
    picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
    picture.save("screenshot.png");
    cout << "picture saved" << endl;
}

void ofApp::dragEvent2(ofDragInfo dragInfo) {

    ofVec3f point;
    mouseIntersectPlane(ofVec3f(0, 0, 0), cam.getZAxis(), point);
    if (rocket.loadModel(dragInfo.files[0])) {
        rocket.setScaleNormalization(false);
        rocket.setScale(.1, .1, .1);
        rocket.setPosition(1, 1, 0);

        bRocketLoaded = true;
        for (int i = 0; i < rocket.getMeshCount(); i++) {
            bboxList.push_back(Octree::meshBounds(rocket.getMesh(i)));
        }

        cout << "Mesh Count: " << rocket.getMeshCount() << endl;
    }
    else cout << "Error: Can't load model" << dragInfo.files[0] << endl;
}

// Check if the mouse intersects a given plane
bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f& point) {
    ofVec2f mouse(mouseX, mouseY);
    ofVec3f rayPoint = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
    ofVec3f rayDir = rayPoint - cam.getPosition();
    rayDir.normalize();
    return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}

// Drag event for loading models
void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (rocket.loadModel(dragInfo.files[0])) {
        bRocketLoaded = true;
        rocket.setScaleNormalization(false);
        rocket.setPosition(0, 0, 0);
        cout << "number of meshes: " << rocket.getNumMeshes() << endl;
        bboxList.clear();
        for (int i = 0; i < rocket.getMeshCount(); i++) {
            bboxList.push_back(Octree::meshBounds(rocket.getMesh(i)));
        }

        glm::vec3 origin = cam.getPosition();
        glm::vec3 camAxis = cam.getZAxis();
        glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
        glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
        float distance;

        bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);
        if (hit) {
            glm::vec3 intersectPoint = origin + distance * mouseDir;

            glm::vec3 min = rocket.getSceneMin();
            glm::vec3 max = rocket.getSceneMax();
            float offset = (max.y - min.y) / 2.0;
            rocket.setPosition(intersectPoint.x, intersectPoint.y - offset, intersectPoint.z);

            rocketBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
        }
    }
}

// Get the mouse point on a given plane in world space
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
    glm::vec3 origin = cam.getPosition();
    glm::vec3 camAxis = cam.getZAxis();
    glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
    glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
    float distance;

    bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

    if (hit) {
        glm::vec3 intersectPoint = origin + distance * mouseDir;

        return intersectPoint;
    }
    else return glm::vec3(0, 0, 0);
}

// Check collisions between rocket and terrain or landing zones
//Pierce Kyaw, Aye Thwe Tun
void ofApp::checkCollisions() {
    ofVec3f min = rocket.getSceneMin() + rocket.getPosition();
    ofVec3f max = rocket.getSceneMax() + rocket.getPosition();
    Box rocketBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

    colBoxList.clear();

    if (octree.intersect(rocketBounds, octree.root, colBoxList)) {
        // Check if rocket is within any landing zone
        bool inAnyLandingZone = false;
        for (int i = 0; i < 3; i++) {
            float distanceToLandingZone = glm::distance(rocket.getPosition(), landingZones[i].center);
            if (distanceToLandingZone < landingZones[i].radius) {
                inAnyLandingZone = true;
                break;
            }
        }

        float verticalSpeed = velocity.y;
        float landingSpeedThreshold = 8.0f; // Speed threshold
        bool gentleVerticalSpeed = fabs(verticalSpeed) < landingSpeedThreshold;

        if (inAnyLandingZone) {
            // Rocket inside landing zone
            if (gentleVerticalSpeed && verticalSpeed <= 0) {
                // Gentle landing = success
                bgrounded = true;
                bWin = true;
                bOver = true;
                winSound.play();

                // Stop movement
                velocity = glm::vec3(0);
                acceleration = glm::vec3(0);
                force = glm::vec3(0);

                // Add score
                score += 500;
            }
            else {
                // Crash landing inside LZ
                explosion.sys->reset();
                explosion.start();
                bgrounded = true;
                bOver = true;
                velocity = glm::vec3(0);
                acceleration = glm::vec3(0);
                force = glm::vec3(0);
                bCrashInLZ = true;
                impactForce = (int)(fabs(verticalSpeed));

                score += 250;

                crashSound.play();
            }
        }
        else {
            // Rocket outside landing zone
            if (gentleVerticalSpeed) {
                // Hover scenario: Apply upward force to avoid penetrating terrain
                Ray downwardRay(Vector3(rocket.getPosition().x, rocket.getPosition().y, rocket.getPosition().z),
                    Vector3(0, -1, 0));
                TreeNode groundNode;
                if (octree.intersect(downwardRay, octree.root, groundNode)) {
                    glm::vec3 groundPoint = octree.mesh.getVertex(groundNode.points[0]);
                    force = glm::vec3(0, 10, 0);
                }
            }
            else {
                // High-speed impact outside LZ = crash
                explosion.sys->reset();
                explosion.start();
                bgrounded = true;
                bOver = true;
                bCrashInLZ = false;
                impactForce = (int)(fabs(verticalSpeed));

                velocity = glm::vec3(0);
                acceleration = glm::vec3(0);
                force = glm::vec3(0);

                crashSound.play();
            }
        }
    }
}

// Draw overlay text (fuel, altitude, fps, score)
//Pierce Kyaw, Aye Thwe Tun
void ofApp::drawText()
{
    ofSetColor(ofColor::white);

    int windowWidth = ofGetWindowWidth();
    int windowHeight = ofGetWindowHeight();

    float margin = 20;
    float xPos = windowWidth - margin;

    float yPos = 15;
    float lineHeight = 20;

    int framerate = ofGetFrameRate();
    string fpsText = "Frame Rate: " + std::to_string(framerate);
    string timerText = "Fuel Remaining: " + std::to_string(fuel) + " seconds";
    string scoreText = "Score: " + std::to_string(score);

    if (bDisplayAltitude) {
        string altitudeMsg = "Altitude: " + std::to_string(altitude);
        ofDrawBitmapString(altitudeMsg, xPos - altitudeMsg.size() * 8, yPos);
        yPos += lineHeight;
    }

    ofDrawBitmapString(timerText, xPos - timerText.size() * 8, yPos); yPos += lineHeight;
    ofDrawBitmapString(fpsText, xPos - fpsText.size() * 8, yPos); yPos += lineHeight;
    ofDrawBitmapString(scoreText, xPos - scoreText.size() * 8, yPos);
}

// Initialize lighting and materials for the scene
//Pierce Kyaw
void ofApp::initLightingAndMaterials()
{
    keyLight.setup();
    keyLight.enable();
    keyLight.setAreaLight(1, 1);

    keyLight.setAmbientColor(ofFloatColor(1, 1, 1));
    keyLight.setDiffuseColor(ofFloatColor(1, 1, 1));
    keyLight.setSpecularColor(ofFloatColor(1, 1, 1));

    keyLight.rotate(45, ofVec3f(0, 1, 0));
    keyLight.rotate(-45, ofVec3f(1, 0, 0));
    keyLight.setSpotlightCutOff(75);
    keyLight.setPosition(100, 50, 100);

    fillLight.setup();
    fillLight.enable();
    fillLight.setSpotlight();
    fillLight.setScale(0.2);
    fillLight.setSpotlightCutOff(30);
    fillLight.setAttenuation(1, 0.005, 0.001);
    fillLight.setAmbientColor(ofFloatColor(0.5, 0.5, 0.5));
    fillLight.setDiffuseColor(ofFloatColor(0.7, 0.7, 0.7));
    fillLight.setSpecularColor(ofFloatColor(0.8, 0.8, 0.8));
    fillLight.rotate(-20, ofVec3f(1, 0, 0));
    fillLight.rotate(-60, ofVec3f(0, 1, 0));
    fillLight.setPosition(500, 50, 50);

    rimLight.setup();
    rimLight.enable();
    rimLight.setSpotlight();
    rimLight.setScale(0.1);
    rimLight.setSpotlightCutOff(20);
    rimLight.setAttenuation(0.5, 0.002, 0.001);
    rimLight.setAmbientColor(ofFloatColor(0.2, 0.2, 0.2));
    rimLight.setDiffuseColor(ofFloatColor(0.4, 0.4, 0.6));
    rimLight.setSpecularColor(ofFloatColor(1, 1, 1));
    rimLight.rotate(160, ofVec3f(0, 1, 0));
    rimLight.setPosition(100, 30, -200);
    rimLight.rotate(45, ofVec3f(1, 0, 0));
}

// Load particle data into a VBO for rendering
void ofApp::loadVbo()
{
    // Return early if there are no particles in the system.
    if (emitter.sys->particles.size() < 1) return;

    // Create vectors to store particle positions and sizes.
    vector<ofVec3f> sizes;
    vector<ofVec3f> points;

    // Populate the points and sizes vectors with particle data.
    for (int i = 0; i < emitter.sys->particles.size(); i++)
    {
        points.push_back(emitter.sys->particles[i].position); // Store particle positions.
        sizes.push_back(ofVec3f(20)); // Assign a default size (20) to each particle.
    }

    // Get the total number of particles.
    int total = (int)points.size();

    // Clear any existing data in the VBO (Vertex Buffer Object).
    vbo.clear();

    // Set vertex data (particle positions) in the VBO.
    vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);

    // Set normal data (particle sizes) in the VBO.
    vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);
}

