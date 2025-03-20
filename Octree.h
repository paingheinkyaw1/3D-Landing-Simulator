
//--------------------------------------------------------------
//
//  Kevin M. Smith
//
//  Simple Octree Implementation 11/10/2020
// 
//  Copyright (c) by Kevin M. Smith
//  Copying or use without permission is prohibited by law.
//
#pragma once
#include "ofMain.h"
#include "box.h"
#include "ray.h"



class TreeNode {
public:
	Box box;
	vector<int> points;
	vector<TreeNode> children;
};

class Octree {
public:

	void create(const ofMesh& mesh, int numLevels);
	void subdivide(const ofMesh& mesh, TreeNode& node, int numLevels, int level);
	bool intersect(const Ray&, const TreeNode& node, TreeNode& nodeRtn);
	bool intersect(const Box&, TreeNode& node, vector<Box>& boxListRtn);
	void draw(TreeNode& node, int numLevels, int level);
	void draw(int numLevels, int level) {
		draw(root, numLevels, level);
	}
	void drawLeafNodes(TreeNode& node);
	static void drawBox(const Box& box);
	static Box meshBounds(const ofMesh&);
	int getMeshPointsInBox(const ofMesh& mesh, const vector<int>& points, Box& box, vector<int>& pointsRtn);
	int getMeshFacesInBox(const ofMesh& mesh, const vector<int>& faces, Box& box, vector<int>& facesRtn);
	void subDivideBox8(const Box& b, vector<Box>& boxList);

	ofMesh mesh;
	TreeNode root;
	bool bUseFaces = false;

	vector<ofColor> colors;

	// debug;
	//
	int strayVerts = 0;
	int numLeaf = 0;
};