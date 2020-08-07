#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include <GL/glew.h>
#include <freeglut/glut.h>
#include "glm.h"

#include "Matrices.h"

#define PI 3.1415926

#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "freeglut.lib")

#ifndef GLUT_WHEEL_UP
# define GLUT_WHEEL_UP   0x0003
# define GLUT_WHEEL_DOWN 0x0004
#endif

#ifndef GLUT_KEY_ESC
# define GLUT_KEY_ESC 0x001B
#endif

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Shader attributes
GLint iLocPosition;
GLint iLocColor;
GLint iLocMVP;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};

struct model
{
	GLMmodel *obj;
	GLfloat *vertices;
	GLfloat *colors;

	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
};

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
};

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};

Matrix4 view_matrix;
Matrix4 project_matrix;

project_setting proj;
camera main_camera;

int current_x, current_y;

model* models;							// store the models we load
vector<string> filenames;				// .obj filename list

int cur_idx = 0;						// represent which model should be rendered now

bool use_wire_mode = false;
bool self_rotate = false;

TransMode cur_trans_mode = GeoTranslation;
ProjMode cur_proj_mode = Orthogonal;

// new add variables
enum Movement {
	Stop = 0,
	Z = 1,
	X = 2,
};

GLfloat smooth_movement = 0;
GLfloat sample = -6;
GLfloat* x_positions;
Movement moving = Stop;

// Normalize the given vector
static GLvoid glmNormalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

// Caculate the cross product of u[3] and v[3] then output the result to n[3]
static GLvoid glmCross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{
	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}

// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
// DONE
Matrix4 translate(Vector3 vec)
{
	// P.39
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0,	 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
// DONE
Matrix4 scaling(Vector3 vec)
{
	// P.40
	Matrix4 mat;
	mat = Matrix4(
		vec.x,     0,	  0, 0,
			0, vec.y,	  0, 0,
			0,     0, vec.z, 0,
			0,     0,	  0, 1
	);
	
	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
// DONE
Matrix4 rotateX(GLfloat val)
{
	// P.41
	Matrix4 mat;
	mat = Matrix4(
		1,		  0,		 0,	0,
		0, cos(val), -sin(val), 0,
		0, sin(val),  cos(val), 0,
		0,		  0,		 0,	1
	);
	
	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-Y)
// DONE
Matrix4 rotateY(GLfloat val)
{
	// P.42
	Matrix4 mat;
	mat = Matrix4(
		 cos(val), 0, sin(val), 0,
				0, 1,		 0,	0,
		-sin(val), 0, cos(val), 0,
				0, 0,		 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-Z)
// DONE
Matrix4 rotateZ(GLfloat val)
{
	// P.43
	Matrix4 mat;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val),  cos(val), 0,	0,
			   0,		  0, 1, 0,
			   0,		  0, 0, 1
	);

	return mat;
}

// Given a rotation vector then output the Matrix4 (Rotation Matrix)
Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

void setViewingMatrix()
{
	// [TODO] compute viewing matrix according to the setting of main_camera, then assign to view_matrix
	// DONE
	// P.69
	Matrix4 T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0,					   1
	);

	// P.70
	Vector3 Rx, Ry, Rz;
	Rz = -1 * (main_camera.center - main_camera.position).normalize();
	Rx = (main_camera.center - main_camera.position).cross(main_camera.up_vector).normalize();
	Ry = Rz.cross(Rx).normalize();

	// P.72
	Matrix4 R = Matrix4(
		Rx.x, Rx.y, Rx.z, 0,
		Ry.x, Ry.y, Ry.z, 0,
		Rz.x, Rz.y, Rz.z, 0,
		   0,	 0,	   0, 1
	);
	view_matrix = R * T;
}

void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	// [TODO] compute orthogonal projection matrix, then assign to project_matrix
	// DONE
	// P.123
	/*
	GLfloat x_max = proj.right;
	GLfloat x_min = proj.left;
	GLfloat y_max = proj.top;
	GLfloat y_min = proj.bottom;
	// two version, use versino P.123
	GLfloat z_far = proj.farClip;
	GLfloat z_near = proj.nearClip;

	// P.89, 125
	GLfloat t_x = -((x_max + x_min) / (x_max - x_min));
	GLfloat t_y = -((y_max + y_min) / (y_max - y_min));
	GLfloat t_z = -((z_far + z_near) / (z_far - z_near));
	// since using version P.123 of z_far and z_near, matrix is version P.125
	Matrix4 M_orthonorm = Matrix4(
		2/(x_max - x_min),					0,					 0, t_x,
						 0, 2/(y_max - y_min),	        		 0, t_y,
						 0,					0, -2/(z_far - z_near), t_z,
						 0,					0,					 0,	  1
	);
	*/
	GLfloat x_length = proj.right - proj.left;
	GLfloat y_length = proj.top - proj.bottom;
	GLfloat z_length = proj.farClip - proj.nearClip;
	GLfloat x_center = (proj.right + proj.left) / 2;
	GLfloat y_center = (proj.top + proj.bottom) / 2;
	GLfloat z_center = (proj.farClip + proj.nearClip) / 2;
	project_matrix = Matrix4(
		2 / x_length,			 0,				0, -x_center * 2 / x_length,
				   0, 2 / y_length,				0, -y_center * 2 / y_length,
				   0,			 0, -2 / z_length, -z_center * 2 / z_length,
				   0,			 0,				0,						  1
	);
}

void setPerspective()
{
	cur_proj_mode = Perspective;
	// [TODO] compute persepective projection matrix, then assign to project_matrix
	// DONE
	// P.95, 130
	// use version P.130
	/*
	GLfloat f = 1 / tan(proj.fovy / 2 / 180 * PI);
	GLfloat z_far = proj.farClip;
	GLfloat z_near = proj.nearClip;

	Matrix4 M_persnorm = Matrix4(
		f / proj.aspect, 0,								 0,										  0,
					  0, f,								 0,										  0,
					  0, 0, (z_far+z_near)/(z_near- z_far), (2 * z_far * z_near) / (z_near - z_far),
					  0, 0,								-1,										  0
	);
	*/
	project_matrix = Matrix4(
		1 / tan(proj.fovy / 2 / 180 * PI) / proj.aspect,								 0,																  0,																 0,
													  0, 1 / tan(proj.fovy / 2 / 180 * PI),																  0,																 0,
													  0,								 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), 2 * proj.farClip * proj.nearClip / (proj.nearClip - proj.farClip),
													  0,								 0,																 -1,																 0
	);
}

void modelMovement(model *m)
{
	// [TODO] Apply the movement on current frame
	smooth_movement = 1 / (exp(-sample) + 1);
	sample += 0.005;
}

void traverseColorModel(model &m)
{
	GLfloat maxVal[3];
	GLfloat minVal[3];


	m.vertices = new GLfloat[m.obj->numtriangles * 9];
	m.colors = new GLfloat[m.obj->numtriangles * 9];

	// The center position of the model 
	m.obj->position[0] = 0;
	m.obj->position[1] = 0;
	m.obj->position[2] = 0;


	for (int i = 0; i < (int)m.obj->numtriangles; i++)
	{
		// the index of each vertex
		int indv1 = m.obj->triangles[i].vindices[0];
		int indv2 = m.obj->triangles[i].vindices[1];
		int indv3 = m.obj->triangles[i].vindices[2];

		// the index of each color
		int indc1 = indv1;
		int indc2 = indv2;
		int indc3 = indv3;

		// assign vertices
		GLfloat vx, vy, vz;
		vx = m.obj->vertices[indv1 * 3 + 0];
		vy = m.obj->vertices[indv1 * 3 + 1];
		vz = m.obj->vertices[indv1 * 3 + 2];

		m.vertices[i * 9 + 0] = vx;
		m.vertices[i * 9 + 1] = vy;
		m.vertices[i * 9 + 2] = vz;

		vx = m.obj->vertices[indv2 * 3 + 0];
		vy = m.obj->vertices[indv2 * 3 + 1];
		vz = m.obj->vertices[indv2 * 3 + 2];

m.vertices[i * 9 + 3] = vx;
m.vertices[i * 9 + 4] = vy;
m.vertices[i * 9 + 5] = vz;

vx = m.obj->vertices[indv3 * 3 + 0];
vy = m.obj->vertices[indv3 * 3 + 1];
vz = m.obj->vertices[indv3 * 3 + 2];

m.vertices[i * 9 + 6] = vx;
m.vertices[i * 9 + 7] = vy;
m.vertices[i * 9 + 8] = vz;

// assign colors
GLfloat c1, c2, c3;
c1 = m.obj->colors[indv1 * 3 + 0];
c2 = m.obj->colors[indv1 * 3 + 1];
c3 = m.obj->colors[indv1 * 3 + 2];

m.colors[i * 9 + 0] = c1;
m.colors[i * 9 + 1] = c2;
m.colors[i * 9 + 2] = c3;

c1 = m.obj->colors[indv2 * 3 + 0];
c2 = m.obj->colors[indv2 * 3 + 1];
c3 = m.obj->colors[indv2 * 3 + 2];

m.colors[i * 9 + 3] = c1;
m.colors[i * 9 + 4] = c2;
m.colors[i * 9 + 5] = c3;

c1 = m.obj->colors[indv3 * 3 + 0];
c2 = m.obj->colors[indv3 * 3 + 1];
c3 = m.obj->colors[indv3 * 3 + 2];

m.colors[i * 9 + 6] = c1;
m.colors[i * 9 + 7] = c2;
m.colors[i * 9 + 8] = c3;
	}
	// Find min and max value
	GLfloat meanVal[3];

	meanVal[0] = meanVal[1] = meanVal[2] = 0;
	maxVal[0] = maxVal[1] = maxVal[2] = -10e20;
	minVal[0] = minVal[1] = minVal[2] = 10e20;

	for (int i = 0; i < m.obj->numtriangles * 3; i++)
	{
		maxVal[0] = max(m.vertices[3 * i + 0], maxVal[0]);
		maxVal[1] = max(m.vertices[3 * i + 1], maxVal[1]);
		maxVal[2] = max(m.vertices[3 * i + 2], maxVal[2]);

		minVal[0] = min(m.vertices[3 * i + 0], minVal[0]);
		minVal[1] = min(m.vertices[3 * i + 1], minVal[1]);
		minVal[2] = min(m.vertices[3 * i + 2], minVal[2]);

		meanVal[0] += m.vertices[3 * i + 0];
		meanVal[1] += m.vertices[3 * i + 1];
		meanVal[2] += m.vertices[3 * i + 2];
	}
	GLfloat scale = max(maxVal[0] - minVal[0], maxVal[1] - minVal[1]);
	scale = max(scale, maxVal[2] - minVal[2]);

	// Calculate mean values
	for (int i = 0; i < 3; i++)
	{
		meanVal[i] /= (m.obj->numtriangles * 3);
	}

	// Normalization
	for (int i = 0; i < m.obj->numtriangles * 3; i++)
	{
		m.vertices[3 * i + 0] = 1.0*((m.vertices[3 * i + 0] - meanVal[0]) / scale);
		m.vertices[3 * i + 1] = 1.0*((m.vertices[3 * i + 1] - meanVal[1]) / scale);
		m.vertices[3 * i + 2] = 1.0*((m.vertices[3 * i + 2] - meanVal[2]) / scale);
	}
}

void loadOBJModel()
{
	models = new model[filenames.size()];
	int idx = 0;
	for (string filename : filenames)
	{

		models[idx].obj = glmReadOBJ((char*)filename.c_str());
		cout << "Read : " << filename << " OK " << endl;
		traverseColorModel(models[idx++]);
	}
}

void onIdle()
{
	glutPostRedisplay();
}

void drawModel(model* model, int index)
{
	// [TODO] Add an offset before caculate the model position matrix
	// e.x.: offset = index * 2
	// Or you can set the offset value yourself
	int num = filenames.size();
	if (cur_idx == 0 && moving == Stop) {
		model->position.x = 2 * index;
	}
	else if (moving == Z) {
		if (cur_idx == num - 1) {
			model->position.x = 2 * (index - num + 1) - 2 * (num - 1) * (smooth_movement - 1) + x_positions[index];
		}
		else {
			model->position.x = 2 * (index - cur_idx - 1 + smooth_movement) + x_positions[index];
		}
	}
	else if (moving == X) {
		if (cur_idx == 0) {
			model->position.x = 2 * index + 2 * (num - 1) * (smooth_movement - 1) + x_positions[index];
		}
		else {
			model->position.x = 2 * (index - cur_idx + 1 - smooth_movement) + x_positions[index];
		}
	}
	if (moving == Stop) {
		model->position.x += x_positions[index];
	}

	Matrix4 MVP;
	// [TODO] Assign MVP correct value
	// [HINT] MVP = projection_matrix * view_matrix * ??? * ??? * ???
	// DONE
	Matrix4 T = translate(model->position);
	Matrix4 R = rotate(model->rotation);
	Matrix4 S = scaling(model->scale);	
	MVP = project_matrix * view_matrix * T * R * S;

	// Since the glm matrix is row-major, so we change the matrix to column-major for OpenGL shader.
	// row-major ---> column-major
	GLfloat mvp[16];
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	// Link vertex and color matrix to shader paremeter.
	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, model->vertices);
	glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, model->colors);

	// Drawing command.
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glDrawArrays(GL_TRIANGLES, 0, model->obj->numtriangles * 3);
}

void drawPlane()
{
	GLfloat vertices[18]{ 1.0, -0.6, -1.0,
						   1.0, -0.6,  1.0,
						  -1.0, -0.6, -1.0,
						   1.0, -0.6,  1.0,
						  -1.0, -0.6,  1.0,
						  -1.0, -0.6, -1.0 };

	GLfloat colors[18]{ 0.0,1.0,0.0,
						0.0,0.5,0.8,
						0.0,1.0,0.0,
						0.0,0.5,0.8,
						0.0,0.5,0.8,
						0.0,1.0,0.0 };


	Matrix4 MVP = project_matrix * view_matrix;
	// Since the glm matrix is row-major, so we change the matrix to column-major for OpenGL shader.
	// row-major ---> column-major
	GLfloat mvp[16];
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	// [TODO] Draw the plane
	// DONE
	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, colors);

	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void onDisplay(void)
{
	// clear canvas
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// clear canvas to color(0,0,0)->black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawPlane();

	if (self_rotate)
	{
		// [TODO] Implement self rotation on current model
		models[cur_idx].rotation.y += (models[cur_idx].rotation.y >= 2 * PI) ? (PI / 180.0) - 2 * PI : (PI / 180.0);
	}

	for (int i = 0; i < filenames.size(); i++)
	{
		// Draw all models
		modelMovement(&models[i]);
		drawModel(&models[i], i);
	}

	glutSwapBuffers();
}

void showShaderCompileStatus(GLuint shader, GLint *shaderCompiled)
{
	glGetShaderiv(shader, GL_COMPILE_STATUS, shaderCompiled);
	if (GL_FALSE == (*shaderCompiled))
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character.
		GLchar *errorLog = (GLchar*)malloc(sizeof(GLchar) * maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
		fprintf(stderr, "%s", errorLog);

		glDeleteShader(shader);
		free(errorLog);
	}
}

//load Shader
const char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	const char **srcp = new const char*[1];
	srcp[0] = src;
	return srcp;
}

void setShaders()
{
	GLuint v, f, p;
	const char **vs = NULL;
	const char **fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = loadShaderSource("shader.vert");
	fs = loadShaderSource("shader.frag");

	glShaderSource(v, 1, vs, NULL);
	glShaderSource(f, 1, fs, NULL);

	free(vs);
	free(fs);

	// compile vertex shader
	glCompileShader(v);
	GLint vShaderCompiled;
	showShaderCompileStatus(v, &vShaderCompiled);
	if (!vShaderCompiled) system("pause"), exit(123);

	// compile fragment shader
	glCompileShader(f);
	GLint fShaderCompiled;
	showShaderCompileStatus(f, &fShaderCompiled);
	if (!fShaderCompiled) system("pause"), exit(456);

	p = glCreateProgram();

	// bind shader
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);

	iLocPosition = glGetAttribLocation(p, "av4position");
	iLocColor = glGetAttribLocation(p, "av3color");
	iLocMVP = glGetUniformLocation(p, "mvp");

	glUseProgram(p);

	glEnableVertexAttribArray(iLocPosition);
	glEnableVertexAttribArray(iLocColor);
}

void onMouse(int who, int state, int x, int y)
{
	printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);

	switch (who)
	{
	case GLUT_LEFT_BUTTON:
		current_x = x;
		current_y = y;
		break;
	case GLUT_MIDDLE_BUTTON: printf("middle button "); break;
	case GLUT_RIGHT_BUTTON:
		current_x = x;
		current_y = y;
		break;
	case GLUT_WHEEL_UP:
		printf("wheel up      \n");
		// [TODO] complete all switch case
		// DONE, still need to be adjusted
		switch (cur_trans_mode)
		{
		case ViewEye:
			main_camera.position.z -= 0.125;
			setViewingMatrix();
			// printf("Current camera position: (%f, %f, %f)\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
			break;
		case ViewCenter:
			main_camera.center.z += 0.25;
			setViewingMatrix();
			// printf("Current camera viewing direction: (%f, %f, %f)\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
			break;
		case ViewUp:
			main_camera.up_vector.z += 0.33;
			setViewingMatrix();
			// printf("Current camera up vector: (%f, %f, %f)\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
			break;
		case GeoTranslation:
			models[cur_idx].position.z += 0.25;
			break;
		case GeoScaling:
			models[cur_idx].scale.z += 0.125;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z += (PI / 180.0) * 10;
			break;
		}
		break;
	case GLUT_WHEEL_DOWN:
		printf("wheel down    \n");
		// [TODO] complete all switch case
		// DONE, still need to be adjusted
		switch (cur_trans_mode)
		{
		case ViewEye:
			main_camera.position.z += 0.125;
			setViewingMatrix();
			// printf("Current camera position: (%f, %f, %f)\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
			break;
		case ViewCenter:
			main_camera.center.z -= 0.5;
			setViewingMatrix();
			// printf("Current camera viewing direction: (%f, %f, %f)\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
			break;
		case ViewUp:
			main_camera.up_vector.z -= 0.33;
			setViewingMatrix();
			// printf("Current camera up vector: (%f, %f, %f)\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
			break;
		case GeoTranslation:
			models[cur_idx].position.z -= 0.5;
			break;
		case GeoScaling:
			models[cur_idx].scale.z -= 0.125;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z -= (PI / 180.0) * 10;
			break;
		}
		break;
	default:
		printf("0x%02X          ", who); break;
	}

	switch (state)
	{
	case GLUT_DOWN: printf("start "); break;
	case GLUT_UP:   printf("end   "); break;
	}

	printf("\n");
}

void onMouseMotion(int x, int y)
{
	int diff_x = x - current_x;
	int diff_y = y - current_y;
	current_x = x;
	current_y = y;

	// [TODO] complete all switch case
	// DONE, still need to be adjusted
	switch (cur_trans_mode)
	{
	case ViewEye:
		main_camera.position.x += diff_x * (5.0 / 400.0);
		main_camera.position.y += diff_y * (5.0 / 400.0);
		setViewingMatrix();
		// printf("Current camera position: (%f, %f, %f)\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
		break;
	case ViewCenter:
		main_camera.center.x += diff_x * (5.0 / 400.0);
		main_camera.center.y += diff_y * (5.0 / 400.0);
		setViewingMatrix();
		// printf("Current camera viewing direction: (%f, %f, %f)\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
		break;
	case ViewUp:
		main_camera.up_vector.x += diff_x * 0.1;
		main_camera.up_vector.y += diff_y * 0.1;
		setViewingMatrix();
		// printf("Current camera up vector: (%f, %f, %f)\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
		break;
	case GeoTranslation:
		// models[cur_idx].position.x += diff_x * (1.0 / 400.0);
		x_positions[cur_idx] += diff_x * (1.0 / 400.0);
		models[cur_idx].position.y -= diff_y * (1.0 / 400.0);
		break;
	case GeoScaling:
		models[cur_idx].scale.x += diff_x * 0.01;
		models[cur_idx].scale.y += diff_y * 0.01;
		break;
	case GeoRotation:
		models[cur_idx].rotation.x += (PI / 180.0) * diff_y * (60.0 / 400.0);
		models[cur_idx].rotation.y += (PI / 180.0) * diff_x * (60.0 / 400.0);
		break;
	}
	printf("%18s(): (%d, %d) mouse move\n", __FUNCTION__, x, y);
}

void onKeyboard(unsigned char key, int x, int y)
{
	printf("%18s(): (%d, %d) key: %c(0x%02X) ", __FUNCTION__, x, y, key, key);
	switch (key)
	{
	case GLUT_KEY_ESC: /* the Esc key */
		exit(0);
		break;
	case 'z':
		// [TODO] Change model and trigger the model moving animation
		cur_idx = (cur_idx + filenames.size() - 1) % filenames.size();
		moving = Z;
		sample = -6;
		break;
	case 'x':		
		// [TODO] Change model and trigger the model moving animation
		cur_idx = (cur_idx + 1) % filenames.size();
		moving = X;
		sample = -6;
		break;
	case 'w':
		// Change polygon mode
		glPolygonMode(GL_FRONT_AND_BACK, use_wire_mode ? GL_FILL : GL_LINE);
		use_wire_mode = !use_wire_mode;
		break;
	case 'd':
		self_rotate = !self_rotate;
		break;
	case 'o':
		// [TODO] Change to Orthogonal
		// DONE
		setOrthogonal();
		break;
	case 'p':
		// [TODO] Change to Perspective
		// DONE
		setPerspective();
		break;
	case 't':
		cur_trans_mode = GeoTranslation;
		break;
	case 's':
		cur_trans_mode = GeoScaling;
		break;
	case 'r':
		cur_trans_mode = GeoRotation;
		break;
	case 'e':
		cur_trans_mode = ViewEye;
		break;
	case 'c':
		cur_trans_mode = ViewCenter;
		break;
	case 'u':
		cur_trans_mode = ViewUp;
		break;
	case 'i':
		// [TODO] print the model name, mode, active operation etc...
		printf("\n\n");
		printf("========================INFO========================\n");
		printf("current model: %d\n", cur_idx);
		printf("	position: (%f, %f, %f)\n", models[cur_idx].position.x, models[cur_idx].position.y, models[cur_idx].position.z);
		printf("	rotation: (%f, %f, %f)\n", models[cur_idx].rotation.x, models[cur_idx].rotation.y, models[cur_idx].rotation.z);
		printf("	scale: (%f, %f, %f)\n", models[cur_idx].scale.x, models[cur_idx].scale.y, models[cur_idx].scale.z);
		printf("\n");
		printf("camera:\n");
		printf("	position: (%f, %f, %f)\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
		printf("	center: (%f, %f, %f)\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
		printf("	up vector: (%f, %f, %f)\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
		printf("\n");
		if (cur_proj_mode == Perspective) {
			printf("current projection mode: Perspective\n");
		}
		else {
			printf("current projection mode: Orthogonal\n");
		}
		printf("\n");
		switch (cur_trans_mode) {
			case GeoTranslation:
				printf("now in geometrical transformation mode: translation\n");
				break;
			case GeoRotation:
				printf("now in geometrical transformation mode: rotation\n");
				break;
			case GeoScaling:
				printf("now in geometrical transformation mode: scaling\n");
				break;
			case ViewCenter:
				printf("now in viewing transformation mode: center coordinate \n");
				break;
			case ViewEye:
				printf("now in viewing transformation mode: eye coordinate\n");
				break;
			case ViewUp:
				printf("now in viewing transformation mode: up vector\n");
				break;
		}
		printf("====================================================\n");
		/*
		printf("	left: %f\n", proj.left);
		printf("	right: %f\n", proj.right);
		printf("	top: %f\n", proj.top);
		printf("	bottom: %f\n", proj.bottom);
		printf("	nearClip: %f\n", proj.nearClip);
		printf("	farClip: %f\n", proj.farClip);
		printf("	fov: %f\n", proj.fovy);
		printf("	aspect ratio: %f\n", proj.aspect);
		*/
		break;
	}
	printf("\n");
}

void onKeyboardSpecial(int key, int x, int y) {
	printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);
	switch (key)
	{
	case GLUT_KEY_LEFT:
		printf("key: LEFT ARROW");
		break;

	case GLUT_KEY_RIGHT:
		printf("key: RIGHT ARROW");
		break;

	default:
		printf("key: 0x%02X      ", key);
		break;
	}
	printf("\n");
}

void onWindowReshape(int width, int height)
{
	proj.aspect = width / height;

	printf("%18s(): %dx%d\n", __FUNCTION__, width, height);
}

void loadConfigFile()
{
	ifstream fin;
	string line;
	fin.open("../../config.txt", ios::in);
	if (fin.is_open())
	{
		while (getline(fin, line))
		{
			filenames.push_back(line);
		}
		fin.close();
	}
	else
	{
		cout << "Unable to open the config file!" << endl;
	}
	for (int i = 0; i < filenames.size(); i++)
		printf("%s\n", filenames[i].c_str());
}

// you can setup your own camera setting when testing
void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.01;
	proj.farClip = 1000.0;
	proj.fovy = 45;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setOrthogonal();	//set default projection matrix as orthogonal matrix

	// new add
	x_positions = new GLfloat[filenames.size()]();
}

int main(int argc, char **argv)
{
	loadConfigFile();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// create window
	glutInitWindowPosition(500, 100);
	glutInitWindowSize(800, 800);
	glutCreateWindow("CG HW2");

	glewInit();
	if (glewIsSupported("GL_VERSION_2_0")) {
		printf("Ready for OpenGL 2.0\n");
	}
	else {
		printf("OpenGL 2.0 not supported\n");
		system("pause");
		exit(1);
	}

	// initialize the camera parameter
	initParameter();

	// load obj models through glm
	loadOBJModel();

	// register glut callback functions
	glutDisplayFunc(onDisplay);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onKeyboardSpecial);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouseMotion);
	glutReshapeFunc(onWindowReshape);

	// set up shaders here
	setShaders();

	glEnable(GL_DEPTH_TEST);

	// main loop
	glutMainLoop();

	// delete glm objects before exit
	for (int i = 0; i < filenames.size(); i++)
	{
		glmDelete(models[i].obj);
	}

	// new add
	delete[]x_positions;

	return 0;
}

