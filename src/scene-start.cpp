// Vum Suan Mang (22355071)
#include "Angel.h"

// Open Asset Importer header files (in ../../assimp--3.0.1270/include)
// This is a standard open source library for loading meshes, see gnatidread.h
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>

#ifdef LAB_PC
#include <dirent.h>
#define EXISTS opendir
#else
#include <filesystem>
#define EXISTS std::filesystem::exists
#endif
static vec3 GetOGLPos(int x, int y);
GLint windowHeight = 640, windowWidth = 960;

// start: part of Part J
bool newObjAdded, isObjSelected, delSelected, isObjectDuplicated = false;
char newObjName[248];
int newMenuId, selectedSceneObjIndex, numNewObj = 0; 
// Number of menus (not the submenus) when mouse is right-clicked
# define FIXED_MAIN_MENU_ITEMS 12 // Currently only 12 items in the main menu
# define FIXED_PRELOADED_OBJS 4 // Ground + Ligh1 + Light2 + Light3
// end: part of Part J

// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data
// Reader" code.  This file contains parts of the code that you shouldn't need
// to modify (but, you can).
#include "gnatidread.h"

using namespace std;        // Import the C++ standard functions (e.g., min)  
// Checking menu status, value is 1 if menu is used otherwise 0
int menuInUse = 0; // part of Part J

// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram; // The number identifying the GLSL shader program
GLuint vPosition, vNormal, vTexCoord; // IDs for vshader input vars (from glGetAttribLocation)
GLuint projectionU, modelViewU; // IDs for uniform variables (from glGetUniformLocation)

static float viewDist = 1.5; // Distance from the camera to the centre of the scene
static float camRotSidewaysDeg = 0.0; // rotates the camera sideways around the centre
static float camRotUpAndOverDeg = 20; // rotates the camera up and over the centre.

mat4 projection; // Projection matrix - set in the reshape function
mat4 view; // View matrix - set in the display function.

// These are used to set the window title
char lab[] = "Project1";
char *programName = NULL; // Set in main 
int numDisplayCalls = 0; // Used to calculate the number of frames per second

//------Meshes----------------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                           (numMeshes is defined in gnatidread.h)
aiMesh *meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays

// -----Textures--------------------------------------------------------------
//                           (numTextures is defined in gnatidread.h)
texture *textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures

//------Scene Objects---------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct {
    vec4 loc;
    float scale;
    float angles[3]; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    vec3 rgb;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
	int menuId; // part of Part J, for adding, deleting and duplicating
	int menuItemNumber; // part of Part J, for deleting and duplicating
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0;    // How many objects are currenly in the scene.
int currObject = -1; // The current object
int toolObj = -1;    // The object currently being modified

//----------------------------------------------------------------------------
//
// Loads a texture by number, and binds it for later use.    
void loadTextureIfNotAlreadyLoaded(int i) {
    if (textures[i] != NULL) return; // The texture is already loaded.

    textures[i] = loadTextureNum(i);
    CheckError();
    glActiveTexture(GL_TEXTURE0);
    CheckError();

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
    CheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData);
    CheckError();
    glGenerateMipmap(GL_TEXTURE_2D);
    CheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    CheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    CheckError(); // Back to default texture
}

//------Mesh loading----------------------------------------------------------//
// The following uses the Open Asset Importer library via loadMesh in 
// gnatidread.h to load models in .x format, including vertex positions, 
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

void loadMeshIfNotAlreadyLoaded(int meshNumber) {
    if (meshNumber >= numMeshes || meshNumber < 0) {
        printf("Error - no such model number");
        exit(1);
    }

    if (meshes[meshNumber] != NULL)
        return; // Already loaded

    aiMesh *mesh = loadMesh(meshNumber);
    meshes[meshNumber] = mesh;

#ifdef __APPLE__
    glBindVertexArrayAPPLE( vaoIDs[meshNumber] );
#else
    glBindVertexArray(vaoIDs[meshNumber]);
#endif

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers(1, buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (3 + 3 + 3) * mesh->mNumVertices,
                 NULL, GL_STATIC_DRAW);

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.    
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * nVerts, mesh->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 3 * nVerts, sizeof(float) * 3 * nVerts, mesh->mTextureCoords[0]);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 6 * nVerts, sizeof(float) * 3 * nVerts, mesh->mNormals);

    // Load the element index data
    //GLuint elements[mesh->mNumFaces*3];
    std::vector<GLuint> elements = std::vector<GLuint>(mesh->mNumFaces * 3, 0);

    for (GLuint i = 0; i < mesh->mNumFaces; i++) {
        elements[i * 3] = mesh->mFaces[i].mIndices[0];
        elements[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        elements[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements.data(), GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0                 
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vPosition);

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer(vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(float) * 3 * mesh->mNumVertices));
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(float) * 6 * mesh->mNumVertices));
    glEnableVertexAttribArray(vNormal);
    CheckError();
}

//----------------------------------------------------------------------------

void zoomIn() {
    viewDist = (viewDist < 0.0 ? viewDist : viewDist * 0.8) - 0.05;
}

void zoomOut() {
    viewDist = (viewDist < 0.0 ? viewDist : viewDist * 1.25) + 0.05;
}

static void mouseClickOrScroll(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		
        if (glutGetModifiers() != GLUT_ACTIVE_SHIFT) activateTool(button);
        else activateTool(GLUT_LEFT_BUTTON);
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) deactivateTool();
    else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) { activateTool(button); }
    else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP) deactivateTool();

    else if (button == 3) { // scroll up
       zoomIn();
	  
    } else if (button == 4) { // scroll down
       zoomOut();
	
    }
}

//----------------------------------------------------------------------------

static void mousePassiveMotion(int x, int y) {
    mouseX = x;
    mouseY = y;
	
}

//----------------------------------------------------------------------------

mat2 camRotZ() {
    return rotZ(-camRotSidewaysDeg) * mat2(10.0, 0, 0, -10.0);
}

//------callback functions for doRotate below and later-----------------------

static void adjustCamrotsideViewdist(vec2 cv) 
{
    cout << cv << endl;
    camRotSidewaysDeg += cv[0];
    viewDist += cv[1];
}

static void adjustcamSideUp(vec2 su) {
    camRotSidewaysDeg += su[0];
    camRotUpAndOverDeg += su[1];
}

static void adjustLocXZ(vec2 xz) {
    sceneObjs[toolObj].loc[0] += xz[0];
    sceneObjs[toolObj].loc[2] += xz[1];
}

static void adjustScaleY(vec2 sy) {
    sceneObjs[toolObj].scale += sy[0];
    sceneObjs[toolObj].loc[1] += sy[1];
}


//----------------------------------------------------------------------------
//------Set the mouse buttons to rotate the camera----------------------------
//------around the centre of the scene.---------------------------------------
//----------------------------------------------------------------------------

static void doRotate() {
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400, 0, 0, -2),
                     adjustcamSideUp, mat2(400, 0, 0, -90));
}

//------Add an object to the scene--------------------------------------------

static void addObject(int id) {
	
    vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
    sceneObjs[nObjects].loc[0] = currPos[0];
    sceneObjs[nObjects].loc[1] = 0.0;
    sceneObjs[nObjects].loc[2] = currPos[1];
    sceneObjs[nObjects].loc[3] = 1.0;

    if (id != 0 && id != 55)
        sceneObjs[nObjects].scale = 0.005;
	
	

    sceneObjs[nObjects].rgb[0] = 0.7;
    sceneObjs[nObjects].rgb[1] = 0.7;
    sceneObjs[nObjects].rgb[2] = 0.7;
    sceneObjs[nObjects].brightness = 1.0;

    sceneObjs[nObjects].diffuse = 1.0;
    sceneObjs[nObjects].specular = 0.5;
    sceneObjs[nObjects].ambient = 0.7;
    sceneObjs[nObjects].shine = 10.0;

    sceneObjs[nObjects].angles[0] = 0.0;
    sceneObjs[nObjects].angles[1] = 180.0;
    sceneObjs[nObjects].angles[2] = 0.0;

    sceneObjs[nObjects].meshId = id;
	
	
	// start: part of Part J, duplicating texture
	if ( isObjSelected ) 
	{
		sceneObjs[nObjects].texId = sceneObjs[selectedSceneObjIndex].texId;
	}
	// start: part of Part J, duplicating texture
    else sceneObjs[nObjects].texId = rand() % numTextures;
    sceneObjs[nObjects].texScale = 2.0;

    toolObj = currObject = nObjects++;
    setToolCallbacks(adjustLocXZ, camRotZ(),
                     adjustScaleY, mat2(0.05, 0, 0, 10.0));
    glutPostRedisplay();
}

//------The init function-----------------------------------------------------

void init(void) {
    srand(time(NULL)); /* initialize random seed - so the starting scene varies */
    aiInit();

    // for (int i=0; i < numMeshes; i++)
    //     meshes[i] = NULL;

#ifdef __APPLE__
    glGenVertexArraysAPPLE(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes
#else
    glGenVertexArrays(numMeshes, vaoIDs);
    CheckError(); // Allocate vertex array objects for meshes
#endif

    glGenTextures(numTextures, textureIDs);
    CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    // shaderProgram = InitShader("res/shaders/vStart1.glsl", "res/shaders/fStart1.glsl"); // Used for questions Part A to Part F
	shaderProgram = InitShader("res/shaders/vStart2.glsl", "res/shaders/fStart2.glsl"); // Used for questions Part G to Part J

    glUseProgram(shaderProgram);
    CheckError();

    // Initialize the vertex position attribute from the vertex shader        
    vPosition = glGetAttribLocation(shaderProgram, "vPosition");
    vNormal = glGetAttribLocation(shaderProgram, "vNormal");
    CheckError();

    // Likewise, initialize the vertex texture coordinates attribute.    
    vTexCoord = glGetAttribLocation(shaderProgram, "vTexCoord");
    CheckError();

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");

    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[0].scale = 10.0;
    sceneObjs[0].angles[0] = 90.0; // Rotate it.
    sceneObjs[0].texScale = 5.0; // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[1].scale = 0.1;
    sceneObjs[1].texId = 0; // Plain texture
    sceneObjs[1].brightness = 0.2; // The light's brightness is 5 times this (below).
	
	// start: part of Part I 
	// A copy of the first light with slightly bigger in size
	addObject(55); // Sphere for the second light
    sceneObjs[2].loc = vec4(1.5, 1.0, 1.0, 1.0);
    sceneObjs[2].scale = 0.15;
    sceneObjs[2].texId = 0; // Plain texture
    sceneObjs[2].brightness = 0.2;
	// end: part of Part I 
	
	// start: part of Part J
	addObject(55); // Sphere for the third light, higher than the other two lights
    sceneObjs[3].loc = vec4(1.0, 3.0, 1.0, 1.0);
    sceneObjs[3].scale = 0.2;
    sceneObjs[3].texId = 0; // Plain texture
    sceneObjs[3].brightness = 0.2;
	// end: part of Part J 
	
	
    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable(GL_DEPTH_TEST);
    doRotate(); // Start in camera rotate mode.
    glClearColor(0.0, 0.0, 0.0, 1.0); /* black background */
	printf("\nPreloaded objects: %d", nObjects);
}

//----------------------------------------------------------------------------

void drawMesh(SceneObject sceneObj) {

    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

    // Texture 0 is the only texture type in this program, and is for the rgb
    // colour of the surface but there could be separate types for, e.g.,
    // specularity and normals.
    glUniform1i(glGetUniformLocation(shaderProgram, "texture"), 0);

    // Set the texture scale for the shaders
    glUniform1f(glGetUniformLocation(shaderProgram, "texScale"), sceneObj.texScale);

    // Set the projection matrix for the shaders
    glUniformMatrix4fv(projectionU, 1, GL_TRUE, projection);

    // Set the model matrix - this should combine translation, 
	// rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).
	// mat4 model = Translate(sceneObj.loc) * Scale(sceneObj.scale); // given code 
	
	// start: part of Part B
	mat4 mRotX = RotateX(sceneObj.angles[0]);
    mat4 mRotY = RotateY(sceneObj.angles[1]);
    mat4 mRotZ = RotateZ(sceneObj.angles[2]);
    mat4 mRotMatrix = mRotX * mRotY * mRotZ; 
	
    mat4 model = Translate(sceneObj.loc) * mRotMatrix * Scale(sceneObj.scale);
	// end: part of Part B

    // Set the model-view matrix for the shaders
    glUniformMatrix4fv(modelViewU, 1, GL_TRUE, view * model);

    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    CheckError();
#ifdef __APPLE__
    glBindVertexArrayAPPLE( vaoIDs[sceneObj.meshId] );
#else
    glBindVertexArray(	vaoIDs[sceneObj.meshId]	);
#endif
    CheckError();

    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3,
                   GL_UNSIGNED_INT, NULL);
    CheckError();
}

//----------------------------------------------------------------------------

void display(void) {
    numDisplayCalls++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame

    // Set the view matrix. To start with this just moves the camera
    // backwards.  You'll need to add appropriate rotations.
	
	// start: Part A
    view = Translate(0.0, 0.0, -viewDist); // Camera location, translation matrix, here view is 4x4 matrix
	// std::cout << view << '\n';
	// ( 1, 0, 0, 0 )
	// ( 0, 1, 0, 0 )
	// ( 0, 0, 1, -1.5 )
	// ( 0, 0, 0, 1 )
	
	mat4 yaw_RotationAboutY = RotateY(camRotSidewaysDeg); // rotate xz-plane
	// std::cout << yaw_RotationAboutY << '\n';
	// ( 1, 0, 0, 0 )
	// ( 0, 1, 0, 0 )
	// ( -0, 0, 1, 0 )
	// ( 0, 0, 0, 1 )
	mat4 pitch_RotationAboutX = RotateX(camRotUpAndOverDeg); // rotate yz-plane
	// std::cout << pitch_RotationAboutX << '\n';
	// ( 1, 0, 0, 0 )
	// ( 0, 0.939693, -0.34202, 0 )
	// ( 0, 0.34202, 0.939693, 0 )
	// ( 0, 0, 0, 1 )
	// view = view * RotateX(camRotUpAndOverDeg) * RotateY(camRotSidewaysDeg);

	view = view * pitch_RotationAboutX * yaw_RotationAboutY ; 
	// std::cout << view << '\n'; 
	// https://learnopengl.com/Getting-started/Camera
	// ( 1, 0, 0, 0 ) // Right vector
	// ( 0, 0.939693, -0.34202, 0 ) // Up vector
	// ( 0, 0.34202, 0.939693, -1.5 ) // Direction vector
	// ( 0, 0, 0, 1 )
	// end: Part A
	
    SceneObject lightObj1 = sceneObjs[1];
    vec4 lightPosition1 = view * lightObj1.loc;

    glUniform4fv(glGetUniformLocation(shaderProgram, "LightPosition1"),
                 1, lightPosition1);
    CheckError();
	
	glUniform1f(glGetUniformLocation(shaderProgram,"brightness1"), lightObj1.brightness); // part of Part H
    CheckError(); // part of Part H
	
	glUniform3fv(glGetUniformLocation(shaderProgram, "color1"),1, lightObj1.rgb); // part of Part G
    CheckError(); // part of Part G
	
	// start: part of Part I
	SceneObject lightObj2 = sceneObjs[2];
	vec4 lightPosition2 = lightObj2.loc;
	
	glUniform4fv(glGetUniformLocation(shaderProgram, "LightPosition2"),
                 1, lightPosition2);
    CheckError();

	glUniform1f(glGetUniformLocation(shaderProgram, "brightness2"), lightObj2.brightness); 
    CheckError();
	
	glUniform3fv(glGetUniformLocation(shaderProgram, "color2"),1, lightObj2.rgb);
	CheckError();
	// end: part of Part I
	
	// start: part of Part J (third light/spot light)
	SceneObject lightObj3 = sceneObjs[3];
	vec4 lightPosition3 = view * lightObj3.loc;
	
	glUniform4fv(glGetUniformLocation(shaderProgram, "LightPosition3"),
                 1, lightPosition3);
	CheckError();
	
	glUniform1f(glGetUniformLocation(shaderProgram, "brightness3"), lightObj3.brightness);
    CheckError();
	
	glUniform3fv(glGetUniformLocation(shaderProgram, "color3"),1, lightObj3.rgb);
	CheckError();
	
	
	float light3pitch = lightObj3.angles[1]; 
	glUniform1f(glGetUniformLocation(shaderProgram, "light3pitch"), light3pitch);
    CheckError();
	
	float light3yaw = lightObj3.angles[2];
	glUniform1f(glGetUniformLocation(shaderProgram, "light3yaw"), light3yaw);
    CheckError();
	// end: part of Part J
		
    for (int i = 0; i < nObjects; i++) {
        SceneObject so = sceneObjs[i];
		
        // vec3 rgb = so.rgb * lightObj1.rgb * so.brightness * lightObj1.brightness * 2.0; // given code
		
		vec3 rgb = so.rgb * so.brightness * 2.0;
		if ( i == 1 ) rgb = so.rgb * lightObj1.rgb * so.brightness * lightObj1.brightness * 2.0;
		if ( i == 2 ) rgb = so.rgb * lightObj2.rgb * so.brightness * lightObj2.brightness * 2.0;
		if (i == 3) rgb = so.rgb * lightObj3.rgb * so.brightness * lightObj3.brightness * 2.0;
		
		
		glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * rgb);
		CheckError();
		glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, so.diffuse * rgb);
		glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, so.specular * rgb);
		glUniform1f(glGetUniformLocation(shaderProgram, "Shininess"), so.shine);
		CheckError();
	
        drawMesh(sceneObjs[i]);
    }
    glutSwapBuffers();
}

//----------------------------------------------------------------------------
//------Menus-----------------------------------------------------------------
//----------------------------------------------------------------------------

static void objectMenu(int id) {
	// This id is the index of ObjectMenuEntries array in gnatidread.h
	// First index is 1 not 0 (zero), but when accessing it is 0-based indexing
    deactivateTool();
    addObject(id);
	
	// start: part of Part J
	numNewObj++;
	// Copying obj name to a new variable for later use
	strcpy(newObjName, objectMenuEntries[id-1]); 
	newObjAdded = true;
	// 600 is used to make indexing easier for deleting and duplicating
	newMenuId = 600 + nObjects - 1; // 600 is 13th in main menu item
	sceneObjs[nObjects-1].menuItemNumber == glutGet(GLUT_MENU_NUM_ITEMS) +1;
	// end: part of Part J
}

static void texMenu(int id) {
    deactivateTool();
    if (currObject >= 0) {
        sceneObjs[currObject].texId = id;
        glutPostRedisplay();
    }
}

static void groundMenu(int id) {
    deactivateTool();
    sceneObjs[0].texId = id;
    glutPostRedisplay();
}

static void adjustBrightnessY(vec2 by) {
    sceneObjs[toolObj].brightness += by[0];
    sceneObjs[toolObj].loc[1] += by[1];
}

static void adjustRedGreen(vec2 rg) {
    sceneObjs[toolObj].rgb[0] += rg[0];
    sceneObjs[toolObj].rgb[1] += rg[1];
}

static void adjustBlueBrightness(vec2 bl_br) {
    sceneObjs[toolObj].rgb[2] += bl_br[0];
    sceneObjs[toolObj].brightness += bl_br[1];
}

// start: part of Part C 
static void adjustAmbientDiffuse(vec2 mbdf){
	// mbdf is the 2nd argument, mat2, in setToolCallbacks function
	sceneObjs[toolObj].ambient += mbdf[0];
	sceneObjs[toolObj].diffuse += mbdf[1];	
}

static void adjustSpecularShine(vec2 spsh){
	// spsh is the 4th argument, mat2, in setToolCallbacks function
	sceneObjs[toolObj].specular += spsh[0];
	sceneObjs[toolObj].shine += spsh[1];	
}
// end: part of Part C



// start: part of Part J
// Adapted from adjustLocXZ function
static void rotateSpotLight(vec2 xz)
{
	sceneObjs[toolObj].angles[2]+=35*xz[0]; 
    sceneObjs[toolObj].angles[1]+=35*xz[1]; 
}
// end: part of Part J

static void lightMenu(int id) {
    deactivateTool();
    if (id == 70) {
        toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2(1.0, 0.0, 0.0, 10.0));
    } else if (id >= 71 && id <= 74) {
        toolObj = 1;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0));
    } 
	// start: part of Part I ( second light ) 
	// Adapted from given code for light 1
	else if (id == 80)
	{
		toolObj = 2;
		setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2(1.0, 0.0, 0.0, 10.0));
	}
	else if ( id == 81 ) // Same as given code for light 1
	{
        toolObj = 2; 
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0));
    } 
	// end: part of Part I 
	
	// start: part of Part J ( third light - spot light)
	// Adapted from given code for light 1
	else if ( id == 90 )
	{
		toolObj = 3;
		setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
	}
	else if (id == 91)
	{
        toolObj = 3;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0.0, 0.0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0.0, 0.0, 1.0) );        
    }
	else if ( id == 92)
	{
    	toolObj = 3;
    	setToolCallbacks(rotateSpotLight, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
    }
	// end: part of Part J
	else {
        printf("Error in lightMenu\n");
        exit(1);
    }
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int)) {
    int nSubMenus = (size - 1) / 10 + 1;
    //int subMenus[nSubMenus];
    std::vector<int> subMenus = std::vector<int>(nSubMenus, 0);

    for (int i = 0; i < nSubMenus; i++) {
        subMenus[i] = glutCreateMenu(menuFn);
        for (int j = i * 10 + 1; j <= min(i * 10 + 10, size); j++)
            glutAddMenuEntry(menuEntries[j - 1], j);
        CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for (int i = 0; i < nSubMenus; i++) {
        char num[6];
        sprintf(num, "%d-%d", i * 10 + 1, min(i * 10 + 10, size));
        glutAddSubMenu(num, subMenus[i]);
        CheckError();
    }
    return menuId;
}

static void materialMenu(int id) {
    deactivateTool();
    if (currObject < 0) return;
    if (id == 10) {
        toolObj = currObject;
        setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1),
                         adjustBlueBrightness, mat2(1, 0, 0, 1));
    }
    // You'll need to fill in the remaining menu items here.
	
	// start: part of Part C 
	else if (id == 21) {
		toolObj = currObject;
		setToolCallbacks(adjustAmbientDiffuse, mat2(1, 0, 0, 1),
                         adjustSpecularShine, mat2(1, 0, 0, 1));
	}
	// end: part of Part C
    else {
        printf("Error in materialMenu\n");
    }
}

static void adjustAngleYX(vec2 angle_yx) {
    sceneObjs[currObject].angles[1] += angle_yx[0];
    sceneObjs[currObject].angles[0] += angle_yx[1];
}

static void adjustAngleZTexscale(vec2 az_ts) {
    sceneObjs[currObject].angles[2] += az_ts[0];
    sceneObjs[currObject].texScale += az_ts[1];
}

static void mainmenu(int id) {
    deactivateTool();
    if (id == 41 && currObject >= 0) {
        toolObj = currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustScaleY, mat2(0.05, 0, 0, 10));
    }
    if (id == 50)
        doRotate();
    if (id == 55 && currObject >= 0) {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400),
                         adjustAngleZTexscale, mat2(400, 0, 0, 15));
    }
    if (id == 99) exit(0);
	
	// start: part of Part J
	// delete menu 
	if (id == 102) {
		deactivateTool();
		delSelected = true;
		// printf("\ncurrent selected menu Item Number: %d", sceneObjs[selectedSceneObjIndex].menuItemNumber);
	}
	if ( id  == 103 ) 
	{
		deactivateTool();
		// Adapted from init function addObject(rand() % numMeshes); 
		addObject(sceneObjs[selectedSceneObjIndex].meshId); 
		strcpy(newObjName, objectMenuEntries[sceneObjs[selectedSceneObjIndex].meshId-1]); 
		newMenuId = nObjects - 1 + 600;
		
		isObjectDuplicated = true;
		glutPostRedisplay();
	}
	
	if ( id == 599 ){} // Used only for showing text in main menu
	
	if (id >= FIXED_PRELOADED_OBJS + 600) {
		deactivateTool();
		isObjSelected = true;
		
		if ( newObjAdded ) selectedSceneObjIndex = id - 600 - numNewObj;
		else selectedSceneObjIndex = id - 600;
		
		glutPostRedisplay();
	}
	// end: part of Part J	
}

// start: part of Part J
// adapted from https://www.lighthouse3d.com/tutorials/glut-tutorial/modifying-a-menu/
// Reshuffling menu item number.
// This menu item number is used to delete when a user select any of 
// the objects below "Select Object below" on the menu when mouse is right-clicked
static void menuShuffler(){
	// Extra menus starts from 13 because there are 12 fixed main menu items
	// Extra menus are added to or deleleted from the menu when objects 
	// are added/duplicated or deleted respectively.
	// Reshuffling is needed because the glut menu item sequential numbering works // only for what is in the menu, it does not remember a menu item 
	// that has been removed
	int shuffling = 13; 
	// int shuffling = FIXED_MAIN_MENU_ITEMS + 1; // i.e. 12 + 1 = 13
	for (int j = 4; j < nObjects; j++)
	{
		if (sceneObjs[j].menuItemNumber != -1) 
		{
			sceneObjs[j].menuItemNumber = shuffling;
			shuffling++;
		}		
	}
}

static void checkMenuStatus(int status, int x, int y) {
	deactivateTool();

			
	int numMenu = glutGet(GLUT_MENU_NUM_ITEMS);
	if (status == GLUT_MENU_IN_USE) 
	{
		menuInUse = 1;
		// printf("\nMenu in use");
		// isObjSelected = false;
	}
	else if ( !isObjSelected ){
		menuInUse = 0;
	}
	else 
	{
		menuInUse = 0;
		// isObjSelected = false;	
	}
	
	if (!menuInUse) {
		
		if ( delSelected  && isObjSelected ) 
		{
			glutRemoveMenuItem(sceneObjs[selectedSceneObjIndex].menuItemNumber);
			
			// Adapted from addObject function, move deleted object out of scene 
			vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
			sceneObjs[selectedSceneObjIndex ].loc[0] = currPos[0]+ 10000.0; 
			sceneObjs[selectedSceneObjIndex ].loc[2] = currPos[1] + 10000.0; 

			toolObj = currObject = selectedSceneObjIndex;
			setToolCallbacks(adjustLocXZ, camRotZ(),
						 adjustScaleY, mat2(0.05, 0, 0, 10.0) );
						 
			// Since the deleted object is still in the sceneObjs struct,
			// -1 below is just a flag to indicate that it is deleted
			// that will be useful for the menuShuffler function below
			sceneObjs[selectedSceneObjIndex].menuItemNumber = -1; 
			menuShuffler();
			
			delSelected = false;
			isObjSelected = false; 
			glutPostRedisplay();
			
		}
		if ( isObjectDuplicated && isObjSelected )
		{
			// Start adding duplicate obj name to the menu 
			glutAddMenuEntry(newObjName, 600 + nObjects - 1); 
			// printf("\nduplicated menu id, %d", newMenuId );
			menuShuffler();
			isObjectDuplicated = false;
			isObjSelected = false;
			glutPostRedisplay();
		}
		if ( newObjAdded ) 
		{
			// start adding new obj to the menu
			glutAddMenuEntry(newObjName, newMenuId);  
			menuShuffler();
			// Reset after name of new obj is added to the menu 
			newObjAdded = false; 
		}
	}
	glutPostRedisplay(); 		
}

// end: part of Part J

// This function is modified for Part C,  Part I, Part J
static void makeMenu() {
    int objectId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu);
	
	
	
    int materialMenuId = glutCreateMenu(materialMenu);
    glutAddMenuEntry("R/G/B/All", 10);
    // glutAddMenuEntry("UNIMPLEMENTED: Ambient/Diffuse/Specular/Shine", 20);
	
	// start: part of Part C
	glutAddMenuEntry("IMPLEMENTED: Ambient/Diffuse/Specular/Shine", 21);
	// end: part of Part C
	
    int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
    int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);
	
    int lightMenuId = glutCreateMenu(lightMenu);
    glutAddMenuEntry("Move Light 1", 70);
    glutAddMenuEntry("R/G/B/All Light 1", 71);
    glutAddMenuEntry("Move Light 2", 80); // part of Part I
    glutAddMenuEntry("R/G/B/All Light 2", 81); // part of Part I
	glutAddMenuEntry("Move Light 3 (Spot light)", 90); // part of Part J
    glutAddMenuEntry("R/G/B/All Light 3 (Spot light)", 91); // part of Part J
	glutAddMenuEntry("Rotate (Spot Light)", 92); // part of Part J
	

    glutCreateMenu(mainmenu);
    glutAddMenuEntry("Rotate/Move Camera", 50);
    glutAddSubMenu("Add object", objectId);
    glutAddMenuEntry("Position/Scale", 41);
    glutAddMenuEntry("Rotation/Texture Scale", 55);
    glutAddSubMenu("Material", materialMenuId);
    glutAddSubMenu("Texture", texMenuId);
    glutAddSubMenu("Ground Texture", groundMenuId);
    glutAddSubMenu("Lights", lightMenuId);
    glutAddMenuEntry("EXIT", 99);
	glutAddMenuEntry("Delete", 102); // part of Part J
	glutAddMenuEntry("Duplicate", 103); // part of Part J
	glutAddMenuEntry("Select Object below", 599); // part of Part J
	
	
	// code block below, part of Part J
	//  FIXED_PRELOADED_OBJS = Ground object + three lights objects = 4 objects
	if (nObjects > FIXED_PRELOADED_OBJS ) {
		strcpy(newObjName, objectMenuEntries[sceneObjs[4].meshId-1]);
		glutAddMenuEntry(newObjName, 600 + FIXED_PRELOADED_OBJS );
		sceneObjs[4].menuItemNumber = 13;
		// sceneObjs[4].menuItemNumber = FIXED_MAIN_MENU_ITEMS +1;
	}
    glutAttachMenu(GLUT_RIGHT_BUTTON);
	glutMenuStateFunc(checkMenuStatus); // part of Part J, prevent error when updating menu
}


//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 033: {
            exit(EXIT_SUCCESS);
            break;
        }
        case 'w': {
            if (glutGetModifiers() == GLUT_ACTIVE_ALT) { // up + alt
                zoomIn();
            }
            break;
        }
        case 's': {
            if (glutGetModifiers() == GLUT_ACTIVE_ALT) { // down + alt
                zoomOut();
            }
            break;
        }
    }
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: {
            if (glutGetModifiers() == GLUT_ACTIVE_ALT) { // up + alt
                zoomIn();
            }
            break;
        }
        case GLUT_KEY_DOWN: {
            if (glutGetModifiers() == GLUT_ACTIVE_ALT) { // down + alt
                zoomOut();
            }
            break;
        }
    }
}
//----------------------------------------------------------------------------

void idle(void) {
    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void reshape(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    glViewport(0, 0, width, height);

    // You'll need to modify this so that the view is similar to that in the
    // sample solution.
    // In particular: 
    //     - the view should include "closer" visible objects (slightly tricky)
    //     - when the width is less than the height, the view should adjust so
    //         that the same part of the scene is visible across the width of
    //         the window.
	
	// // Given
    // GLfloat nearDist = 0.2;
    // projection = Frustum(-nearDist * (float) width / (float) height,
                         // nearDist * (float) width / (float) height,
                         // -nearDist, nearDist,
                         // nearDist, 100.0);
	
	// start: Part D  and Part E
	GLfloat nearDist = 0.001;
	GLfloat farDist = 100.0;
	
	if(width < height){
     
        projection = Frustum(-nearDist, nearDist,
                    -nearDist*(float)height/(float)width,
                    nearDist*(float)height/(float)width,
                    nearDist, farDist);
    }

    else{
		projection = Frustum(-nearDist * (float) width / (float) height,
                         nearDist * (float) width / (float) height,
                         -nearDist, nearDist,
                         nearDist, farDist);
	}
	// end: Part D and Part E
}

//----------------------------------------------------------------------------

void timer(int unused) {
    char title[256];
    sprintf(title, "%s %s: %d Frames Per Second @ %d x %d",
            lab, programName, numDisplayCalls, windowWidth, windowHeight);

    glutSetWindowTitle(title);

    numDisplayCalls = 0;
    glutTimerFunc(1000, timer, 1);
}

//----------------------------------------------------------------------------

char dirDefault1[] = "res/models-textures";
char dirDefault3[] = "/tmp/models-textures";
char dirDefault4[] = "/d/models-textures";
char dirDefault2[] = "/cslinux/examples/CITS3003/project-files/models-textures";

void fileErr(char *fileName) {
    printf("Error reading file: %s\n", fileName);
    printf("When not in the CSSE labs, you will need to include the directory containing\n");
    printf("the models on the command line, or put it in the res folder next to the executable.");
    exit(1);
}

//----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    // Get the program name, excluding the directory, for the window title
    programName = argv[0];
    for (char *cpointer = argv[0]; *cpointer != 0; cpointer++)
        if (*cpointer == '/' || *cpointer == '\\') programName = cpointer + 1;

    // Set the models-textures directory, via the first argument or some handy defaults.
    if (argc > 1)
        strcpy(dataDir, argv[1]);
    else if (EXISTS(dirDefault1)) strcpy(dataDir, dirDefault1);
    else if (EXISTS(dirDefault2)) strcpy(dataDir, dirDefault2);
    else if (EXISTS(dirDefault3)) strcpy(dataDir, dirDefault3);
    else if (EXISTS(dirDefault4)) strcpy(dataDir, dirDefault4);
    else fileErr(dirDefault1);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);

#if !defined(__APPLE__) && !defined(LAB_PC)
    glutInitContextVersion(3, 2);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#endif

    glutCreateWindow("Initialising...");

#ifndef __APPLE__
    glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
#endif

    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

    init();
    CheckError();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);
	
    glutMouseFunc(mouseClickOrScroll);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc(doToolUpdateXY);
	
	
	
    glutReshapeFunc(reshape);
    glutTimerFunc(1000, timer, 1);
    CheckError();
    makeMenu();
    CheckError();

    glutMainLoop();
    return 0;
}
