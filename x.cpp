//Including libraries

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <cstring>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include "glm/glm.hpp"
#include <unistd.h>
#include <SFML/Audio.hpp>

//Background Music
sf::SoundBuffer buffer;
sf::Sound sound;
bool musicEnabled=false;

//Namespaces
using namespace std;
using namespace glm;

//Defining constants used in the code
#define TO_RADIANS 3.14/180.0
#define ssize 800
#define sx 700
#define sy 100
#define bsize 512
#define psize 5.0
#define InitScale 1.0
#define viewHeight 11.0
#define maxDist 1.0

//Struct for movement
struct Motion{
    bool Forward,Backward,Left,Right;
};

//Struct for storing object details
struct obj_t {
	//Attributes of the object
	GLuint ID = 0;               //ID of the object
	string fname = "";           //Name of the .obj file
	const char* fpath = "";      //Path of the .obj file
	bool selection = false;      //Flag to check if object is selected

	//Structures to load object details
	vector<unsigned int> indices;        //Indices
	vector<vec3> vertices;               //Vertices
	vector<vec2> uvs;                    //UVs
	vector<vec3> normals;                //Normals

	//Creating buffer objects
	GLuint vao = 0;		//Vertex Array Object
	GLuint vbo = 0;		//Vertex Buffer Object
	GLuint ubo = 0;		//UV Buffer Object
	GLuint nbo = 0;		//Normal Buffer Object
	GLuint ebo = 0;		//Element Buffer Object

	//Pointers to attributes
	GLuint vap = 0;		//Vertex attribute pointer
	GLuint uap = 1;		//UV attribute pointer
	GLuint nap = 2;		//Normal attribute pointer
};

//Struct for loading BMP files
#pragma pack(push, 1)
struct BMPHeader {
	short type;         //Type
	int size;           //Size
	short reserved1;
	short reserved2;
	int offset;         //Offset
};

//Struct to store BMP Header Info
struct BMPInfoHeader {
	int size;           //Size
	int width;          //Width
	int height;         //Height
	short planes;
	short bitsPerPixel;
	unsigned compression;
	unsigned imageSize;
	int xPelsPerMeter;
	int yPelsPerMeter;
	int clrUsed;
	int clrImportant;
};

//Struct to store 4 unsigned chars
struct uchar4 {
	unsigned char x, y, z, w;
};
#pragma pack(pop)

//Variables for camera
float pitch=-7.0,yaw= 66.0;
float camX=10.0,camZ=3.0;
Motion motion = {false,false,false,false};

//Variables for each object
obj_t* obj_house = new obj_t;       //The Room including some furniture
obj_t* obj_couch = new obj_t;            //Couch
obj_t* obj_table = new obj_t;            //Table
obj_t* obj_textwall = new obj_t;         //Text on the wall
obj_t* obj_door = new obj_t;        //The exit door
obj_t* obj_book = new obj_t;        //Book on the table
obj_t* obj_cushion = new obj_t;     //Cushion on the sofa
obj_t* obj_frame = new obj_t;    //Frame on the wall
obj_t* obj_lamp = new obj_t;		//Lamp on bedside table
obj_t* obj_laptop = new obj_t;          //Laptop on desk
obj_t* obj_pot = new obj_t;       //Pot on desk
obj_t* obj_lettercushion = new obj_t;    //Letter under cushion
obj_t* obj_letterbed = new obj_t;        //Letter on the side of the bed
obj_t* obj_letterframe = new obj_t;      //Letter behind the frame
obj_t* obj_letterlaptop = new obj_t;     //Letter on laptop screen
obj_t* obj_letterpot = new obj_t;        //Letter under pot

//Variables for password checking
char passLetter;               //Single character input to password
int passLetterCount=5;         //Count of remaining letters to password
int checkPassInput = 0;
bool passTemp = false;
bool passSolved = false;		//Variable to check if password is solved
string passPrev;                //Variable for previously entered password
string passAns="   M   U   S   I   C";         //Answer to password

//Variables for loading BMP files
int imageWidth[2], imageHeight[2];        //Size 2 for two files
GLuint imageTex[2];
uchar4* dst[2];

//More variables
GLuint mouseMode = GLUT_CURSOR_CROSSHAIR;
GLfloat shapeRatio;          //Resize ratio
float bufferDepth[bsize];    //Buffer for processing hits
float rotDoor = 0.0;         //Door rotation angle

//Declaring all functions used
//Common OpenGL functions
void display(void);         //Display function
void idle();                //Idle function
void keyboard(unsigned char key, int x, int y);         //Keyboard function
void passiveMouseFunc(int x, int y);             //Passive Mouse function
void reshape(int w, int h);                  //Reshape function

//Functions to construct OBJs
void constructOBJ(string fname, obj_t* object, GLuint ID);     //Construct OBJ
void destructOBJ(obj_t* object);         //Destruct OBJ
void drawAllObjects(GLenum mode);        //Draw all objects
void drawOBJ(obj_t* object);             //Draw a single object
void drawValidObjects(void);             //Draw valid objects
void generateVAO(obj_t* object);         //Generate VAO
void glConstructOBJs(void);              //Construct all objects
void glDestructOBJs(void);               //Destruct OBJs
void loadOBJ(obj_t* object);             //Load OBJ files

//Functions to control selection
void mousefunc(int button, int state, int x, int y);     //Mouse pick function
void processHitObjects(GLuint hits, GLuint buffer[]);         //Process selections
bool checkSelected(void);                     //Check if object is selected
void updateDeselectedObject(void);                  //Deselect object
void updateSelectedObject(GLuint ID);               //Select object
bool checkMovement();                           //Check for possible movement
bool validSelected = false;                   //Variable to check selection

//Functions for password input
string checkEnteredPassword(int passAttempt);        //Check entered password
void loadPassKeypad(void);               //Load Password keypad in proper view
void printPassHintText(int x, int y);      //Print password text on screen
void displayText(int x, int y, vec3 color, char mode, const char* text);    //Print text on screen

//Functions for image loading
void loadAllBMPs(void);          //Load BMP files
void loadBMP(uchar4** dst, int* width, int* height, const char* name);   //Load a single BMP

//Functions for lighting
void glLighting0(void);    //Light 0
void glLighting1(void);    //Light 1
void glLighting2(void);		//Light 2
void glLighting3(void);		//Light 3
void glLightingOff(void);    //Turn off lights
void glLightingOpt(bool enab);   //Toggle lights

//Function to load .obj file from path
void constructOBJ(string fname,obj_t* object, GLuint ID){
	string fpath = "../object/" + fname + ".obj";     //Path of file
	object->fname = fname;
	object->fpath = fpath.c_str();
	object->ID = ID;
	loadOBJ(object);     //Load file
	generateVAO(object);    //Generate VAO
}

//Function to destruct created Object
void destructOBJ(obj_t* object){
	glDisableVertexAttribArray(object->vap);
	glDisableVertexAttribArray(object->uap);
	glDisableVertexAttribArray(object->nap);
	glDeleteBuffers(1, &object->vbo);
	glDeleteBuffers(1, &object->ubo);
	glDeleteBuffers(1, &object->nbo);
	glDeleteBuffers(1, &object->ebo);
	glDeleteVertexArrays(1, &object->vao);
	delete object;
}

//Function to bind object to buffers
void drawOBJ(obj_t* object){
	//Binding to VAO and EBO
	glBindVertexArray(object->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->ebo);
	glDrawElements(GL_TRIANGLES, (GLsizei)object->indices.size(), GL_UNSIGNED_INT, (void*)0);
	//Unbind EBO and reset VBO after drawing
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glColor3f(1.0, 1.0, 1.0);
}

//Generate VAO
void generateVAO(obj_t* object){
	//Create VAO
	glGenVertexArrays(1, &object->vao);
	glBindVertexArray(object->vao);      //Bind
	//Create VBO
	glGenBuffers(1, &object->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, object->vbo);      //Bind
	glBufferData(GL_ARRAY_BUFFER, object->vertices.size() * sizeof(vec3), &object->vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//Create UBO
	glGenBuffers(1, &object->ubo);
	glBindBuffer(GL_ARRAY_BUFFER, object->ubo);       //Bind
	glBufferData(GL_ARRAY_BUFFER, object->uvs.size() * sizeof(vec2), &object->uvs[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	///Create NBO
	glGenBuffers(1, &object->nbo);
	glBindBuffer(GL_ARRAY_BUFFER, object->nbo);         //Bind
	glBufferData(GL_ARRAY_BUFFER, object->normals.size() * sizeof(vec3), &object->normals[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//Generate EBO
	glGenBuffers(1, &object->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->ebo);  //Bind
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, object->indices.size() * sizeof(unsigned int), &object->indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//Enable the attribute arrays
	glEnableVertexAttribArray(object->vap);
	glBindBuffer(GL_ARRAY_BUFFER, object->vbo);  //Bind vbo
	glVertexAttribPointer(object->vap, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(object->uap);
	glBindBuffer(GL_ARRAY_BUFFER, object->ubo);  //Bind ubo
	glVertexAttribPointer(object->uap, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(object->nap);
	glBindBuffer(GL_ARRAY_BUFFER, object->nbo);   //Bind nbo
	glVertexAttribPointer(object->nap, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindVertexArray(0);
}

//Construct all the objects
void glConstructOBJs(void){
	//Call construct OBJ for each object
	constructOBJ("house", obj_house,1);   //House
	constructOBJ("couch",obj_couch,2);	//Couch
	constructOBJ("table",obj_table,3);	//Table
	constructOBJ("textwall",obj_textwall,4);	//Wall text
	constructOBJ("door", obj_door, 5);	//Exit Door
	constructOBJ("book", obj_book, 6);	//Book on table
	constructOBJ("cushion", obj_cushion, 7);	//Cushion
	constructOBJ("frame", obj_frame, 8);	//Frame
	constructOBJ("lamp", obj_lamp, 9);	//Lamp
	constructOBJ("laptop", obj_laptop, 10);	//Laptop
	constructOBJ("pot", obj_pot, 11);	//Pot
	constructOBJ("lettercushion",obj_lettercushion,12);	//Letter under cushion
	constructOBJ("letterbed",obj_letterbed,13);	//Letter on bedside
	constructOBJ("letterframe",obj_letterframe,14);	//Letter behind frame
	constructOBJ("letterlaptop",obj_letterlaptop,15);	//Letter on laptop screen
	constructOBJ("letterpot",obj_letterpot,16);	//Letter under pot
}

//Destruct created objects
void glDestructOBJs(void){
	//Call destructOBJ for each object
	destructOBJ(obj_house);  //House
	destructOBJ(obj_couch);	//Couch
	destructOBJ(obj_table);	//Table
	destructOBJ(obj_textwall);	//textwall
	destructOBJ(obj_door);	//door
	destructOBJ(obj_book);	//book
	destructOBJ(obj_cushion);	//cushion
	destructOBJ(obj_frame);	//frame
	destructOBJ(obj_lamp);	//lamp
	destructOBJ(obj_laptop);	//laptop
	destructOBJ(obj_pot);	//pot
	destructOBJ(obj_lettercushion);	//letter under cushion
	destructOBJ(obj_letterbed);	//letter on bedside
	destructOBJ(obj_letterframe);	//letter behind frame
	destructOBJ(obj_letterlaptop);	//letter on laptop screen
	destructOBJ(obj_letterpot);	//letter under pot
}

//Load an object from .obj file
void loadOBJ(obj_t* object){
	//Create arrays to store data
	vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	vector<vec3> temp_vertices;
	vector<vec2> temp_uvs;
	vector<vec3> temp_normals;
	FILE* file = fopen(object->fpath, "r");
	//Check file exists
	if (file == NULL) {
		printf("Impossible to open the file.\n");
		exit(1);
	}
	while (1) {
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;
		//Vertex
		if (strcmp(lineHeader, "v") == 0) {
			vec3 vertex;
			int lineData = fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);  //Store vertex
		}
		//Texture coordinate
		else if (strcmp(lineHeader, "vt") == 0) {
			vec2 uv;
			int lineData = fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);     //Store UV
		}
		//Normal
		else if (strcmp(lineHeader, "vn") == 0) {
			vec3 normal;
			int lineData = fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);   //Store normal
		}
		//Face
		else if (strcmp(lineHeader, "f") == 0) {
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
			                     &vertexIndex[0], &uvIndex[0], &normalIndex[0],
				                 &vertexIndex[1], &uvIndex[1], &normalIndex[1],
				                 &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			//Check formatting
			if (matches != 9) {
				printf("File can't be read by loader.\nmatches : %d", matches);
				exit(1);
			}
			//Store face data
			for (int i = 0; i < 3; i++) {
				vertexIndices.push_back(vertexIndex[i]);
				uvIndices    .push_back(uvIndex[i]);
				normalIndices.push_back(normalIndex[i]);
			}
		}
		else continue;
	}
	//Assign to object structure
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex  = vertexIndices[i];
		unsigned int uvIndex      = uvIndices[i];
		unsigned int normalIndex  = normalIndices[i];
		unsigned int elementIndex = i;
		//finding index
		vec3 vertex = temp_vertices[vertexIndex - 1];
		vec2 uv     = temp_uvs[uvIndex - 1];
		vec3 normal = temp_normals[normalIndex - 1];
		//assign
		object->vertices.push_back(vertex);
		object->uvs     .push_back(uv);
		object->normals .push_back(normal);
		object->indices .push_back(elementIndex);
	}
	//close the file
	fclose(file);
}

//function to load bmp file
void loadBMP(uchar4 **dst, int *width, int *height, const char *name){
	//structures to store data
	BMPHeader hdr;
	BMPInfoHeader infoHdr;
	int x, y;
	FILE *fd;
	//Error checking
	//Size error
	if(sizeof(uchar4) != 4){
		printf("Bad uchar4 size\n");
		exit(0);
	}
	//Access error
	if( !(fd = fopen(name,"rb")) ){
		printf("File access denied\n");
		exit(0);
	}
	//format error
	fread(&hdr, sizeof(hdr), 1, fd);
	if(hdr.type != 0x4D42){
		printf("Bad file format\n");
		exit(0);
	}
	fread(&infoHdr, sizeof(infoHdr), 1, fd);
	//color depth error
	if(infoHdr.bitsPerPixel != 24){
		printf("Invalid color depth\n");
		exit(0);
	}
	//compression error
	if(infoHdr.compression){
		printf("Compressed image\n");
		exit(0);
	}
	//setting pointers
	*width  = infoHdr.width;
	*height = infoHdr.height;
	*dst    = (uchar4 *)malloc(*width * *height * 4);
	fseek(fd, hdr.offset - sizeof(hdr) - sizeof(infoHdr), SEEK_CUR);
	//set values
	for(y = 0; y < infoHdr.height; y++){
		for(x = 0; x < infoHdr.width; x++){
			(*dst)[(y * infoHdr.width + x)].z = fgetc(fd);
			(*dst)[(y * infoHdr.width + x)].y = fgetc(fd);
			(*dst)[(y * infoHdr.width + x)].x = fgetc(fd);
		}
		for(x = 0; x < (4 - (3 * infoHdr.width) % 4) % 4; x++)
			fgetc(fd);
	}
	//loading error
	if(ferror(fd)){
		printf("Unknown load error.\n");
		free(*dst);
		exit(0);
	}else 
		printf("BMP file loaded.\n");       //successful load
	fclose(fd);
}

//load all bmp files
void loadAllBMPs(void){
	//load keypad
	loadBMP(&dst[0], &imageWidth[0], &imageHeight[0], "../keypad.bmp");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &imageTex[0]);
	//load congrats
	loadBMP(&dst[1], &imageWidth[1], &imageHeight[1], "../banner.bmp");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &imageTex[1]);
}

//camera function
void camera(){
	//cam position variables
	float tempX=camX,tempZ=camZ;
	//forward motion
    if(motion.Forward)
    {
        camX += cos((yaw+90)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90)*TO_RADIANS)/5.0;
		if(!checkMovement()){
			camX=tempX;
			camZ=tempZ;
		}
    }
    //backward motion
    if(motion.Backward)
    {
        camX += cos((yaw+90+180)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90+180)*TO_RADIANS)/5.0;
		if(!checkMovement()){
			camX=tempX;
			camZ=tempZ;
		}
    }
    //left motion
    if(motion.Left)
    {
        camX += cos((yaw+90+90)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90+90)*TO_RADIANS)/5.0;
		if(!checkMovement()){
			camX=tempX;
			camZ=tempZ;
		}
    }
    //right motion
    if(motion.Right)
    {
        camX += cos((yaw+90-90)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90-90)*TO_RADIANS)/5.0;
		if(!checkMovement()){
			camX=tempX;
			camZ=tempZ;
		}
    }
    //limit value of pitch
    if(pitch>=70){
        pitch = 70;
    }
    if(pitch<=-60){
        pitch=-60;
    }
    //apply transformations
    glRotatef(-pitch,1.0,0.0,0.0);
    glRotatef(-yaw,0.0,1.0,0.0);
    glTranslatef(-camX,-11.0,-camZ);
}

//keyboard function
void keyboard(unsigned char key,int x,int y){
	//toggle music
	if(key=='m'||key=='M'){
		if(musicEnabled){
            sound.stop();
            musicEnabled = false;
        }
        else
        {	
        	sound.play();
            musicEnabled = true;
        }
	}
	//close the game
	if(key==27){
		exit(0);
	}
	//deselect an object
	if(validSelected==true && key=='q'){
		validSelected=false;
		updateDeselectedObject();
	}
	//take password input
	if(validSelected==true && obj_door->selection==true){
		if (key == 'r' || key == 'R') {
			passSolved = false;
			passTemp = false;
			passLetterCount = 5;
			passPrev = "";
			rotDoor = 0.0;
		}
		//uppercase
		else if(key>=65 && key<=90){
			passLetter=key;
			passLetterCount--;
			checkPassInput=1;
		}
		//lowercase
		else if(key>=97 && key<=122){
			passLetter=char(key-'a'+65);
			passLetterCount--;
			checkPassInput=1;
		}
	}
	//movement
	if(obj_door->selection==false){
		if(key=='w'||key=='W'){
	        motion.Forward = true;     //forward
	    }
	    else if(key=='a'||key=='A'){
	    	motion.Left = true;			//left
	    }
	    else if(key=='s'||key=='S'){
	    	motion.Backward = true;		//backward
	    }
	    else if(key=='d'||key=='D'){
	    	motion.Right = true;		//right
	    }
	}
	glutPostRedisplay();
}

//function to handle key release
void keyRelease(unsigned char key,int x,int y){
    if(key=='w'||key=='W'){
        motion.Forward = false;    //forward
    }
    else if(key=='a'||key=='A'){
    	motion.Left = false;		//left
    }
    else if(key=='s'||key=='S'){
    	motion.Backward = false;	//backward
    }
    else if(key=='d'||key=='D'){
    	motion.Right = false;		//right
    }
}

//Resize function
void resizeScene(int Width, int Height){
    if (Height==0){
        Height=1;
    }
    //set values
    glViewport(0, 0, Width, Height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);
    glMatrixMode(GL_MODELVIEW);
}

//draw all objects in the scene
void drawAllObjects(GLenum mode){
	//draw house
	if (mode == GL_SELECT) glLoadName(obj_house->ID);
	glPushMatrix();
	glPushName(obj_house->ID);
	{
		glColor3f(0.9, 0.9, 0.9);   //house color
		drawOBJ(obj_house);
	}
	glPopName();
	glPopMatrix();
	//draw couch
	if (mode == GL_SELECT) glLoadName(obj_couch->ID);
	glPushMatrix();
	glPushName(obj_couch->ID);
	{
		glColor3f(0.78, 0.04, 0.04);	//couch color
		drawOBJ(obj_couch);
	}
	glPopName();
	glPopMatrix();
	//draw text on wall
	if (mode == GL_SELECT) glLoadName(obj_textwall->ID);
	glPushMatrix();
	glPushName(obj_textwall->ID);
	{
		glColor3f(0.78, 0.04, 0.04);	//textwall color
		drawOBJ(obj_textwall);
	}
	glPopName();
	glPopMatrix();
	//draw table
	if (mode == GL_SELECT) glLoadName(obj_table->ID);
	glPushMatrix();
	glPushName(obj_table->ID);
	{
		glColor3f(0.04, 0.2, 0.78);		//table color
		drawOBJ(obj_table);
	}
	glPopName();
	glPopMatrix();
	//draw door
	if (mode == GL_SELECT) glLoadName(obj_door->ID);
	glPushMatrix();
	glPushName(obj_door->ID);
	{
		if (passSolved) {			//check password solved to open the door
			if (rotDoor > -90)
				rotDoor -= 3.0;
			glTranslatef(33.46, 0, -18.80);
			glRotatef(rotDoor, 0, 1, 0);
			glTranslatef(-33.46, 0, 18.80);
		}
		glColor3f(0.5, 0.5, 0.6);	//color of door
		drawOBJ(obj_door);
	}
	glPopName();
	glPopMatrix();
	//draw book
	if (mode == GL_SELECT) glLoadName(obj_book->ID);
	glPushMatrix();
	glPushName(obj_book->ID);
	{
		if(obj_book->selection==true){			//check if selected
			glTranslatef(0.0,0.0,4.0);
		}
		glColor3f(0.8, 0.7, 1.0);			//color of book
		drawOBJ(obj_book);
	}
	glPopName();
	glPopMatrix();
	//draw cushion
	if (mode == GL_SELECT) glLoadName(obj_cushion->ID);
	glPushMatrix();
	glPushName(obj_cushion->ID);
	{
		if(obj_cushion->selection==true){ 	//check if selected
			glTranslatef(0.0,0.0,-4.0);
		}
		glColor3f(0.3, 0.5, 0.6);		//color of cushion
		drawOBJ(obj_cushion);
	}
	glPopName();
	glPopMatrix();
	//draw frame
	if (mode == GL_SELECT) glLoadName(obj_frame->ID);
	glPushMatrix();
	glPushName(obj_frame->ID);
	{
		if(obj_frame->selection==true){		//check if selected
			glTranslatef(-9.0,0.0,22.0);
			glRotatef(45.0,0,1,0);
			glTranslatef(8.0,0.0,-14.0);
		}
		glColor3f(0.6, 1.0, 0.8);			//color of frame
		drawOBJ(obj_frame);
	}
	glPopName();
	glPopMatrix();
	//draw lamp
	if (mode == GL_SELECT) glLoadName(obj_lamp->ID);
	glPushMatrix();
	glPushName(obj_lamp->ID);
	{
		if(obj_lamp->selection==true){		//check if selected
			glTranslatef(0.0,4.0,0.0);	
		}
		glColor3f(1.0, 1.0, 0.6);		//lamp color
		drawOBJ(obj_lamp);
	}
	glPopName();
	glPopMatrix();
	//draw laptop
	if (mode == GL_SELECT) glLoadName(obj_laptop->ID);
	glPushMatrix();
	glPushName(obj_laptop->ID);
	{
		if(obj_laptop->selection==true){		//check if selected
			glColor3f(1.0, 1.0, 0.0);
			drawOBJ(obj_letterlaptop);
		}
		glColor3f(0.7, 0.7, 1.0);			//laptop color
		drawOBJ(obj_laptop);
	}
	glPopName();
	glPopMatrix();
	//draw pot
	if (mode == GL_SELECT) glLoadName(obj_pot->ID);
	glPushMatrix();
	glPushName(obj_pot->ID);
	{
		if(obj_pot->selection==true){		//check if selected
			glTranslatef(0.0,2.5,0.0);
		}
		glColor3f(0.3, 0.3, 0.3);		//pot color
		drawOBJ(obj_pot);
	}
	glPopName();
	glPopMatrix();
	//letter under cushion
	if (mode == GL_SELECT) glLoadName(obj_lettercushion->ID);
	glPushMatrix();
	glPushName(obj_lettercushion->ID);
	{
		glColor3f(0.0, 1.0, 0.0);		//color
		drawOBJ(obj_lettercushion);
	}
	glPopName();
	glPopMatrix();
	//letter on bedside
	if (mode == GL_SELECT) glLoadName(obj_letterbed->ID);
	glPushMatrix();
	glPushName(obj_letterbed->ID);
	{
		glColor3f(0.0, 0.0, 1.0);	//color
		drawOBJ(obj_letterbed);
	}
	glPopName();
	glPopMatrix();
	//letter behind frame
	if (mode == GL_SELECT) glLoadName(obj_letterframe->ID);
	glPushMatrix();
	glPushName(obj_letterframe->ID);
	{
		glColor3f(1.0, 0.0, 0.0);	//color
		drawOBJ(obj_letterframe);
	}
	glPopName();
	glPopMatrix();
	//letter under pot
	if (mode == GL_SELECT) glLoadName(obj_letterpot->ID);
	glPushMatrix();
	glPushName(obj_letterpot->ID);
	{
		glColor3f(1.0, 0.64, 0.0);		//color
		drawOBJ(obj_letterpot);
	}
	glPopName();
	glPopMatrix();
}

//draw valid objects
void drawValidObjects(void){
	//set mouse mode
	mouseMode = GLUT_CURSOR_NONE;
	//draw all valid objects
	//draw door
	if (obj_door->selection) { 		//if selected
		glPushMatrix();
		{
			camX=30.02;				//change cam angle when door is selected
			camZ=-5.0;
			yaw=-0.36;
			pitch=-1.42;
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
		//activate password checking
		printPassHintText(ssize/2 - 115, ssize/2 + 150);
	}
	//draw book
	else if (obj_book->selection) {		//if selected
		glPushMatrix();
		{
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
	}
	//draw cushion
	else if (obj_cushion->selection) {		//if selected
		glPushMatrix();
		{
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
	}
	//draw frame
	else if (obj_frame->selection) {		//if selected
		glPushMatrix();
		{
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
	}
	//draw lamp
	else if (obj_lamp->selection) {			//if selected
		glPushMatrix();
		{
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
	}
	//draw laptop
	else if (obj_laptop->selection) {			//if selected
		glPushMatrix();
		{
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
	}
	//draw pot
	else if (obj_pot->selection) {		//if selected
		glPushMatrix();
		{
			glLightingOff();
			glLightingOpt(false);
			drawAllObjects(GL_RENDER);
		}
		glPopMatrix();
	}
}

//display function
void display(void){
	//set initial values
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);    //set color
	glClearDepth(1.0f);			//clear depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//clear buffers
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();			
	glShadeModel(GL_SMOOTH);
	glScalef(InitScale, InitScale, InitScale);  //scale scene
	glInitNames();			//initialise names
	glutInitDisplayMode(GLUT_DEPTH);			//display mode
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//call camera function
	camera();
	//draw all objects
	if (validSelected == false) {
		glLightingOpt(false);
		drawAllObjects(GL_RENDER);
	}
	//draw selected object after transformation
	else {
		glLightingOpt(true);
		drawValidObjects();
	}
	glLightingOff();
	//print help text
	//normal movement
	if (!checkSelected()&&!passSolved) {
		displayText(10, 770, vec3(0.0, 0.0, 0.0), 'b', "Movement: WASD");		//move
		displayText(10, 745, vec3(0.0, 0.0, 0.0), 'b', "Interact: Left Click");	//interact
		displayText(10, 720, vec3(0.0, 0.0, 0.0), 'b', "Exit game: Esc");		//quit
		displayText(10, 695, vec3(0.0, 0.0, 0.0), 'b', "Toggle BGM: M");		//toggle music
	}
	//when object is interacted with
	else if (checkSelected()) {
		//if door is selected
		if (obj_door->selection) {
			displayText(10, 770, vec3(0.0, 0.0, 0.0), 'b', "Enter letter: Keyboard"); 	//enter password
			displayText(10, 745, vec3(0.0, 0.0, 0.0), 'b', "Reset: R");		//reset
			displayText(10, 720, vec3(0.0, 0.0, 0.0), 'b', "Quit: Q");	//return to scene
		}
		//normal object selected
		else {
			displayText(10, 770, vec3(0.0, 0.0, 0.0), 'b', "Return object: Q");   //return to original position
		}
	}
	//display congrats banner when room solved
	if (passSolved) {
		glBindTexture(GL_TEXTURE_2D, imageTex[1]);  //bind texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  //tex parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	//tex parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	//tex parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	//tex parameter
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth[1], imageHeight[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, dst[1]);
		//draw the banner
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
			glTexCoord2d(1.0, 0.0); glVertex3d(33.5, viewHeight - 1.0, -30);	//coordinates
			glTexCoord2d(0.0, 0.0); glVertex3d(24.5, viewHeight - 1.0, -30);	//coordinates
			glTexCoord2d(0.0, 1.0); glVertex3d(24.5, viewHeight + 1.0, -30);	//coordinates
			glTexCoord2d(1.0, 1.0); glVertex3d(33.5, viewHeight + 1.0, -30);	//coordinates
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
	glFlush();
	glutSwapBuffers();
}

//idle function
void idle(){
	glutPostRedisplay();
}

//passive mouse movement function
void passiveMouseFunc(int x, int y){
	int dev_x,dev_y;
    dev_x = 400-x;
    dev_y = 400-y;
	yaw+=(float)dev_x/1000.0;   //update yaw
    pitch+=(float)dev_y/1000.0;		//update pitch
	glutPostRedisplay();
}

//check if movement is possible
bool checkMovement(){
	bool result;   //variable to store result
	bool roomCheck = (camX <  34   && camX >  15  && camZ > -17 && camZ <  4)  ||   //check if movement is possible
				  (camX <  28.3 && camX >  23  && camZ >  4  && camZ <  18) ||
				  (camX <  15   && camX > -8.5 && camZ > -3  && camZ <  18) ||
				  (camX < -1.6  && camX > -14  && camZ > -17 && camZ < -3)  ||
				  (camX < -8.5  && camX > -26  && camZ >  14 && camZ <  18) ||
				  (camX <  15   && camX > -1.6 && camZ > -8  && camZ < -3);

	if (passSolved) 		//region changes if room is open
		result = roomCheck || (camX < 32 && camX > 26 && camZ < -15) || 
				 camX > 37 || camX < -33 || camZ > 21 || camZ < -22;
	else 
		result = roomCheck;
	//return result of checks
	return result;
}

//reshape function
void reshape(int w, int h){
	shapeRatio = (GLfloat)w / (GLfloat)h;   //ratio of screen
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);		//set viewport
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40.0, shapeRatio, 1.0, 300.0);		//set perspective
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -20.0);
}

//mouse pick function
void mousefunc(int button, int state, int x, int y){
	GLuint selectBuf[bsize];      //buffer for object selection
	GLint hits;						//check hits
	GLint viewport[4];		//viewport
	//if not left click return
	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
		return;
	//get viewport
	glGetIntegerv(GL_VIEWPORT, viewport);
	//select the buffer
	glSelectBuffer(bsize, selectBuf);
	(void)glRenderMode(GL_SELECT);
	//initialise names
	glInitNames();
	glPushName(0);
	//we send a ray from camera angle and check all objects that are hit by the ray
	//then take the object with minimum z-depth
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	{
		glLoadIdentity();
		//Save the matrix of the picking area to the select buffer
		gluPickMatrix((GLdouble)x, (GLdouble)(viewport[3] - y), psize, psize, viewport);
		gluPerspective(40.0, shapeRatio, 1.0, 300.0);     //ray
		drawAllObjects(GL_SELECT);
		//Draw selected object with object name
		glMatrixMode(GL_PROJECTION);
	}
	glPopMatrix();
	glFlush();
	//process the hit objects
	hits = glRenderMode(GL_RENDER);
	processHitObjects(hits, selectBuf);
	glutPostRedisplay();
}

//process the objects hit by camera ray
void processHitObjects(GLuint hits, GLuint buffer[]){
	//initialise variables
	float zFront, zBack;
	float zMin = maxDist;
	GLuint objectID, selectionID, hitCounts;
	GLuint* ptr = (GLuint*)buffer;
	selectionID = 0;
	for (int i = 0; i < bsize; i++)
		bufferDepth[i] = maxDist;
	//detect objects
	if (hits != 0){
		for (unsigned int i = 0; i < hits; i++){
			//save each object hit
			hitCounts = *ptr;
			ptr++;
			//find zfront depth value
			zFront = (float)*ptr / 0x7fffffff;
			ptr++;
			//find zback depth value
			zBack = (float)*ptr / 0x7fffffff;
			ptr++;
			//save ID of the hit object
			objectID = *ptr;
			//skip buffer pointer
			for (unsigned int j = 0; j < hitCounts; j++)
				ptr++;
			//save zfront depth value for checking
			bufferDepth[objectID] = zFront - 1.0f;
		}
		//find minimum zdepth value as selected object
		for (int i = 0; i < bsize; i++){
			if (zMin >= bufferDepth[i]) {
				zMin = bufferDepth[i];
				selectionID = i;
			}
		}
	}
	//update the selected object in struct
	//and render as needed
	if(hits!=0){
		updateSelectedObject(selectionID);
	}
}

//check if any interactive object is selected
bool checkSelected(void){
	if (obj_door->selection)              return true;		//check door
	else if (obj_book->selection)         return true;		//check book
	else if (obj_cushion->selection)      return true;		//check cushion
	else if (obj_frame->selection) return true;		//check frame
	else if (obj_lamp->selection)  return true;		//check lamp
	else if (obj_laptop->selection)       return true;		//check laptop
	else if (obj_pot->selection)      return true;		//check pot
	return false;		//else return false no object selected
}

//update selected object
void updateSelectedObject(GLuint ID){
	//update the object struct that is selected
	if (ID == obj_house->ID) {		//if house is selected
		obj_house->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_house->fname.c_str()); //house is selected
	}
	else if (ID == obj_door->ID) {		//if door is selected
		obj_door->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_door->fname.c_str());		//door is selected
	}
	else if (ID == obj_book->ID) {		//if book is selected
		obj_book->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_book->fname.c_str());		//book is selected
	}
	else if (ID == obj_cushion->ID) {		//if cushion is selected
		obj_cushion->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_cushion->fname.c_str());	//cushion is selected
	}
	else if (ID == obj_frame->ID) {		//if frame is selected
		obj_frame->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_frame->fname.c_str());		//frame is selected
	}
	else if (ID == obj_lamp->ID) {		//if lamp is selected
		obj_lamp->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_lamp->fname.c_str());		//lamp is selected
	}
	else if (ID == obj_laptop->ID) {		//if laptop is selected
		obj_laptop->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_laptop->fname.c_str());	//laptop is selected
	}
	else if (ID == obj_pot->ID) {		//if pot is selected
		obj_pot->selection = true;		//update as selected in struct
		printf("Selected object : %s\n", obj_pot->fname.c_str());		//pot is selected
	}
	//check if any object is selected
	if (checkSelected()) {
		validSelected = true;
	}
}

//update deselection of an object
void updateDeselectedObject(void){
	//Reset variable
	validSelected = false;
	mouseMode = GLUT_CURSOR_CROSSHAIR;
	//Set all objects as deselected
	obj_house->selection = false;	//deselect house
	obj_door->selection = false;	//deselect door
	obj_book->selection = false;	//deselect book
	obj_cushion->selection = false;		//deselect cushion
	obj_frame->selection = false;		//deselect frame
	obj_lamp->selection = false;		//deselect lamp
	obj_laptop->selection = false;		//deselect laptop
	obj_pot->selection = false;		//deselect pot
}

//load the keypad bmp in correct position
void loadPassKeypad(void){
	glPushMatrix();
	{
		glBindTexture(GL_TEXTURE_2D, imageTex[0]);		//bind texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);		//tex parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);		//tex parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	//tex parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	//tex parameter
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth[0], imageHeight[0], 0, GL_RGBA, GL_UNSIGNED_BYTE, dst[0]);
		//display the keypad
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0); glVertex3f(26.6,7,-17);	//coordinates
			glTexCoord2d(1.0, 0.0); glVertex3f(33.6,7,-17);	//coordinates
			glTexCoord2d(1.0, 1.0); glVertex3f(33.6,14,-17);	//coordinates
			glTexCoord2d(0.0, 1.0); glVertex3f(26.6,14,-17);	//coordinates
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
	glPopMatrix();
}

//function to check password attempt
string checkEnteredPassword(int passAttempt){
	//string to store text to print
	string passCheck;
	if(passAttempt == 4)		//4 letters left to type
		passCheck = passPrev + "   _   _   _   _";
	else if (passAttempt>=1 && passAttempt<=3)		//between 1 and 3 letters left to type
		passCheck = passPrev + "   _";
	else if (passAttempt == 0){				//all letters typed, check password
		passCheck = passPrev;
		//if correct password
		if (passPrev==passAns){
			passSolved = true;		//password solved
		}
		//incorrect password
		else{
			passLetterCount--;		//decrease count to -1 to reset
		}
	}
	//return string to print
	return passCheck;
}

//function to display hint
void printPassHintText(int x, int y){
	string printStr;
	//Load keypad
	glPushMatrix();
	glDisable(GL_LIGHTING);
	loadPassKeypad();
	glPopMatrix();
	//if correct password is entered
	if (passSolved) {
		//deselect door
		updateDeselectedObject();
		if (passTemp == false) {
			sleep(1);		//wait for 1 second to appear as if we are checking password
			passTemp = true;
		}
		//print text for correct password
		displayText(x-130, y+70, vec3(1.0, 1.0, 1.0), 's', "Correct Password!");
		return;
	}
	//if incorrect password is entered
	else if (passLetterCount >= 0) {
		printStr = "     Hint:Sa Re Ga Ma...";	//hint to be printed
		//if some letters are remaining
		if (passLetterCount < 5) {
			if (checkPassInput) {
				passPrev = passPrev + "   " + passLetter;	//add space between letters
				checkPassInput = 0;
			}
			//check the entered password
			printStr = checkEnteredPassword(passLetterCount);
		}
		//print hint text or entered password
		displayText(x-190, y+70, vec3(1.0, 1.0, 1.0), 's', printStr.c_str());
	}
	//reset password when incorrect
	else {
		sleep(1);
		passLetterCount = 5;
		passPrev = "";
	}
}

//function to display text on the screen
void displayText(int x, int y, vec3 color, char mode, const char* text){
	//function to display any text on the screen at 
	//position x,y with specified color
	const unsigned char* str = (const unsigned char*)text;
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	{
		glLoadIdentity();
		gluOrtho2D(0.0, (double)ssize, 0.0, (double)ssize);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		{
			glLoadIdentity();
			glColor3f(color.x, color.y, color.z);	//color of text to be printed
			//instructions for moving through room
			if (mode == 'b') {
				string s = text;
				glRasterPos2i(x, y);		//position of text
				glutBitmapString(GLUT_BITMAP_HELVETICA_18, str);	//font
			}
			//while entering password in keypad
			else if (mode == 's') {
				glLineWidth(2.0);		//width of text
				glTranslated(x, y, 0);	//position of text
				glScaled(0.25, 0.25, 0);	//scale text
				glutStrokeString(GLUT_STROKE_ROMAN, str);	//font
			}
			//width and scale
			glLineWidth(InitScale);
			glScaled(InitScale, InitScale, InitScale);
			glMatrixMode(GL_PROJECTION);
		}
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

//functions for lighting
//light 1
void glLighting1(void){
	glEnable(GL_LIGHT1);	//enable light
	GLfloat light_ambient[] = {0.5, 0.5, 0.5, 0.5};  //ambient parameters
	GLfloat light_diffuse[] = {0.5, 0.5, 0.5, 0.5};		//diffuse parameters
	GLfloat light_specular[] = {0.5, 0.5, 0.5, 0.5};	//specular parameters
	GLfloat light_position[] = {0,0,8.868,1};		//position of light
	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);	//set ambient
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);	//set diffuse
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);	//set specular
	glLightfv(GL_LIGHT1, GL_POSITION, light_position);	//set position
}

//light 2
void glLighting2(void){
	glEnable(GL_LIGHT2);	//enable light
	GLfloat light_ambient[] = {0.1, 0.1, 0.1, 1.0};  //ambient parameters
	GLfloat light_diffuse[] = {1.0, 1.0, 0.7, 1.0};		//diffuse parameters
	GLfloat light_specular[] = {1.0, 1.0, 1.0, 0.1};	//specular parameters
	GLfloat light_position[] = {-19.0,17.0,13.0,0.5};		//position of light
	glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient);	//set ambient
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);	//set diffuse
	glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);	//set specular
	glLightfv(GL_LIGHT2, GL_POSITION, light_position);	//set position
}

//light 3
void glLighting3(void){
	glEnable(GL_LIGHT3);
	GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};	//ambient
	GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};	//diffuse
	GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};	//specular
	glLightfv(GL_LIGHT3, GL_AMBIENT, light_ambient);	//set ambient
	glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse);	//set diffuse
	glLightfv(GL_LIGHT3, GL_SPECULAR, light_specular);	//set specular
}

//turn off lights
void glLightingOff(void){
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glDisable(GL_LIGHT3);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
}

//check if lights are on or off
void glLightingOpt(bool enab){
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	//if enabled
	if (enab == false) {
		glLighting1();
		// glLighting2();
	}
	//if disabled
	else
		glLighting3();
}

//driver code
int main(int argc, char** argv){
	//set window characteristics
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(ssize, ssize);
	glutInitWindowPosition(sx, sy);
	glutCreateWindow("Escape Room");

	//load background music in sound buffer
	if(!buffer.loadFromFile("./music.wav")){
    	return 1;
	}
	sound.setBuffer(buffer);

	//initialise display window, objects and bmps
	glewInit();
	glConstructOBJs();
	loadAllBMPs();
	
	//callback functions
    glutKeyboardFunc(keyboard);		//keyboard
	glutKeyboardUpFunc(keyRelease);		//keyboard release
	glutMouseFunc(mousefunc);		//mouse click
	glutReshapeFunc(reshape);		//reshape function
	glutDisplayFunc(display);		//display function
	glutIdleFunc(idle);				//idle function
	glutPassiveMotionFunc(passiveMouseFunc);	//passive mouse movement

	//main callback loop
	glutMainLoop();

	//destruct objects when done
	glDestructOBJs();
	return 0;
}