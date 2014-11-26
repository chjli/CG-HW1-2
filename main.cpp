#include <GL/glew.h>
#include <GL/glut.h>
#include <FreeImage.h>
#include <fstream>
#include <string>
#include "mesh.h"

using namespace std;
void display();

void LoadTexture(const char* pFilename,unsigned int id){
	FIBITMAP* pImage = FreeImage_Load(FreeImage_GetFileType(pFilename, 0), pFilename);
	FIBITMAP *p32BitsImage = FreeImage_ConvertTo32Bits(pImage);
	int iWidth = FreeImage_GetWidth(p32BitsImage);
	int iHeight = FreeImage_GetHeight(p32BitsImage);

	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iWidth, iHeight,
		0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage));
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	FreeImage_Unload(p32BitsImage);
	FreeImage_Unload(pImage);
}
//////////////////////////////
////	VIEW				
//////////////////////////////
class View{
public:
	void loadfile(const char* file);
	void set() const;
	void scaling(float w,float h){ 
		viewport[2] = w;
		viewport[3] = h;
	}
	void zoom(int n){
		fovy += n;
	}
	void rotate(int n){
		angle += n;
	}
	float width(){
		return viewport[2];
	}
	float height(){
		return viewport[3];
	}
private:
	float angle;
	float eye[3];
	float vat[3];
	float vup[3];
	float fovy;
	float dnear;
	float dfar;
	float viewport[4];
};
void View::loadfile(const char* file){
	ifstream ifile(file);
	angle = 0;
	string kind;
	while (ifile >> kind)
	{
		if (kind == "eye")
			ifile >> eye[0] >> eye[1] >> eye[2];
		else if (kind == "vat")
			ifile >> vat[0] >> vat[1] >> vat[2];
		else if (kind == "vup")
			ifile >> vup[0] >> vup[1] >> vup[2];
		else if (kind == "fovy")
			ifile >> fovy;
		else if (kind == "dnear")
			ifile >> dnear;
		else if (kind == "dfar")
			ifile >> dfar;
		else if (kind == "viewport")
			ifile >> viewport[0] >> viewport[1] >> viewport[2] >> viewport[3];
		else
			break;
	}
}
void View::set() const{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eye[0], eye[1], eye[2],    // eye
		vat[0], vat[1], vat[2],    // center
		vup[0], vup[1], vup[2]);   // up
		
	glTranslatef(vat[0], vat[1], vat[2]);
	glRotatef(angle, vup[0], vup[1], vup[2]);
	glTranslatef(-vat[0], -vat[1], -vat[2]);	
	// projection transformation
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy, viewport[2] / viewport[3], dnear, dfar);
	// viewport transformation
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);	
}
//////////////////////////////
////	VIEW				
//////////////////////////////

//////////////////////////////
////	LIGHT				
//////////////////////////////
class Light{
public:
	void loadfile(const char* file);
	void set() const;
private:
	float ambient[4];
	struct source{
		float position[4];
		float ambient[4];
		float diffuse[4];
		float specular[4];
	}light[8];
	int count;
};
void Light::loadfile(const char* file){
	ifstream ifile(file);
	string kind;
	count = 0;
	while (ifile >> kind)
	{
		if (kind == "light")
		{
			ifile >> light[count].position[0] >> light[count].position[1] >> light[count].position[2]
				>> light[count].ambient[0] >> light[count].ambient[1] >> light[count].ambient[2]
				>> light[count].diffuse[0] >> light[count].diffuse[1] >> light[count].diffuse[2]
				>> light[count].specular[0] >> light[count].specular[1] >> light[count].specular[2];
			light[count].position[3] = 1.0f;
			light[count].ambient[3] = 1.0f;
			light[count].diffuse[3] = 1.0f;
			light[count].specular[3] = 1.0f;
			count++;
		}
		else if (kind == "ambient")
		{
			ambient[3] = 1.0f;
			ifile >> ambient[0] >> ambient[1] >> ambient[2];
		}
		else
			break;
	}
}
void Light::set() const{
	glEnable(GL_LIGHTING);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	int i;
	GLenum times[8] = {
		GL_LIGHT0,
		GL_LIGHT1,
		GL_LIGHT2,
		GL_LIGHT3,
		GL_LIGHT4,
		GL_LIGHT5,
		GL_LIGHT6,
		GL_LIGHT7
	};
	for (i = 0; i < count; i++)
	{
		glEnable(times[i]);
		glLightfv(times[i], GL_POSITION, light[i].position);
		glLightfv(times[i], GL_AMBIENT, light[i].ambient);
		glLightfv(times[i], GL_DIFFUSE, light[i].diffuse);
		glLightfv(times[i], GL_SPECULAR, light[i].specular);

	}
}
//////////////////////////////
////	LIGHT				
//////////////////////////////

//////////////////////////////
////	SCENE				
//////////////////////////////
class Scene{
public:
	void loadfile(const char* file);
	void set() const;
	void shift(float x, float y){
		input[num].translate[0] += x;
		input[num].translate[1] += y;
	}
	void select(int n){
		num = n;
	}
private:
    struct model{
		float scale[3];
		float rotate[4];
		float translate[3];
		int kind;
    	mesh* object;
	}input[1000];
	struct method{
		unsigned int texture_id[2];
		int kind;
	}texture[100];
    int count_input,count_texture,num;
};
void Scene::loadfile(const char* file){
	ifstream ifile(file);
	string m,name;
	string image;
	count_input = 0;
	count_texture = 0;
	int x = 0;
	FreeImage_Initialise();
	while (ifile >> m)
	{
		if (m == "no-texture")
		{
			texture[count_texture].kind = 1;
			count_texture++;
		}
		else if (m == "single-texture")
		{
			ifile >> image;
			
			glGenTextures(1, texture[count_texture].texture_id);
			LoadTexture(image.c_str(), texture[count_texture].texture_id[0]);

			texture[count_texture].kind = 2;
			count_texture++;
		}
		else if (m == "multi-texture")
		{
			glGenTextures(2, texture[count_texture].texture_id);
			for (int i = 0; i < 2; i++)
			{
				ifile >> image; 
				LoadTexture(image.c_str(), texture[count_texture].texture_id[i]);
			}
			texture[count_texture].kind = 3;
			count_texture++;
		}
		else if (m == "cube-map")
		{
			glGenTextures(1, texture[count_texture].texture_id);
			FIBITMAP* pImage[6];
			FIBITMAP *p32BitsImage[6];
			int iWidth[6];
			int iHeight[6];

			for (int i = 0; i < 6; i++)
			{	
				ifile >> image;
				pImage[i] = FreeImage_Load(FreeImage_GetFileType(image.c_str(), 0), image.c_str());
				p32BitsImage[i] = FreeImage_ConvertTo32Bits(pImage[i]);
				iWidth[i] = FreeImage_GetWidth(p32BitsImage[i]);
				iHeight[i] = FreeImage_GetHeight(p32BitsImage[i]);
			}

			glBindTexture(GL_TEXTURE_CUBE_MAP, texture[count_texture].texture_id[0]);
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, iWidth[0], iHeight[0],
				0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage[0]));
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, iWidth[1], iHeight[1],
				0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage[1]));
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, iWidth[2], iHeight[2],
				0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage[2]));
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, iWidth[3], iHeight[3],
				0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage[3]));
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, iWidth[4], iHeight[4],
				0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage[4]));
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, iWidth[5], iHeight[5],
				0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(p32BitsImage[5]));

			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
			FreeImage_Unload(p32BitsImage[0]);
			FreeImage_Unload(pImage[0]);
			FreeImage_Unload(p32BitsImage[1]);
			FreeImage_Unload(pImage[1]);
			FreeImage_Unload(p32BitsImage[2]);
			FreeImage_Unload(pImage[2]);
			FreeImage_Unload(p32BitsImage[3]);
			FreeImage_Unload(pImage[3]);
			FreeImage_Unload(p32BitsImage[4]);
			FreeImage_Unload(pImage[4]);
			FreeImage_Unload(p32BitsImage[5]);
			FreeImage_Unload(pImage[5]);

			texture[count_texture].kind = 4;
			count_texture++;
		}
		else if (m == "model")
		{
			ifile >> name;
			ifile >> input[count_input].scale[0] >> input[count_input].scale[1] >> input[count_input].scale[2]
				>> input[count_input].rotate[0] >> input[count_input].rotate[1] >> input[count_input].rotate[2] >> input[count_input].rotate[3]
				>> input[count_input].translate[0] >> input[count_input].translate[1] >> input[count_input].translate[2];
			input[count_input].object = new mesh(name.c_str());
			input[count_input].kind = count_texture -1;
			count_input++;
		}		
	}
	FreeImage_DeInitialise();
}
void Scene::set() const{
	int k;
	glMatrixMode(GL_MODELVIEW);	
	for (k = 0; k<count_input; k++){
		glPushMatrix();
		glTranslatef(input[k].translate[0],input[k].translate[1],input[k].translate[2]);
		glRotatef(input[k].rotate[0],input[k].rotate[1],input[k].rotate[2],input[k].rotate[3]);
		glScalef(input[k].scale[0],input[k].scale[1],input[k].scale[2]);
		int lastMaterial = -1;
		
		if (texture[input[k].kind].kind == 1)
		{
			for (size_t i = 0; i < input[k].object->fTotal; ++i)
			{
				if (lastMaterial != input[k].object->faceList[i].m)
				{
					lastMaterial = (int)input[k].object->faceList[i].m;
					glMaterialfv(GL_FRONT, GL_AMBIENT, input[k].object->mList[lastMaterial].Ka);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, input[k].object->mList[lastMaterial].Kd);
					glMaterialfv(GL_FRONT, GL_SPECULAR, input[k].object->mList[lastMaterial].Ks);
					glMaterialfv(GL_FRONT, GL_SHININESS, &input[k].object->mList[lastMaterial].Ns);
				}
				glBegin(GL_TRIANGLES);
				for (size_t j = 0; j<3; ++j)
				{
					glNormal3fv(input[k].object->nList[input[k].object->faceList[i][j].n].ptr);
					glVertex3fv(input[k].object->vList[input[k].object->faceList[i][j].v].ptr);
				}
				glEnd();
			}
			glPopMatrix(); 
		}
		else if (texture[input[k].kind].kind == 2)
		{
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture[input[k].kind].texture_id[0]);
			for (size_t i = 0; i < input[k].object->fTotal; ++i)
			{
				if (lastMaterial != input[k].object->faceList[i].m)
				{
					lastMaterial = (int)input[k].object->faceList[i].m;
					glMaterialfv(GL_FRONT, GL_AMBIENT, input[k].object->mList[lastMaterial].Ka);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, input[k].object->mList[lastMaterial].Kd);
					glMaterialfv(GL_FRONT, GL_SPECULAR, input[k].object->mList[lastMaterial].Ks);
					glMaterialfv(GL_FRONT, GL_SHININESS, &input[k].object->mList[lastMaterial].Ns);
					//////
					glBindTexture(GL_TEXTURE_2D, texture[input[k].kind].texture_id[0]);
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
					glEnable(GL_ALPHA_TEST);
					glAlphaFunc(GL_GREATER, 0.5f);
					////
				}
				glBegin(GL_TRIANGLES);
				for (size_t j = 0; j<3; ++j)
				{
					glTexCoord2fv(input[k].object->tList[input[k].object->faceList[i][j].t].ptr);
					glNormal3fv(input[k].object->nList[input[k].object->faceList[i][j].n].ptr);
					glVertex3fv(input[k].object->vList[input[k].object->faceList[i][j].v].ptr);
				}
				glEnd();
			}
			glActiveTexture(GL_TEXTURE0);
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			glPopMatrix(); 
		}
		else if (texture[input[k].kind].kind == 3)
		{
			
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture[input[k].kind].texture_id[0]);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);

			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture[input[k].kind].texture_id[1]);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);

			for (size_t i = 0; i < input[k].object->fTotal; ++i)
			{
				if (lastMaterial != input[k].object->faceList[i].m)
				{
					lastMaterial = (int)input[k].object->faceList[i].m;
					glMaterialfv(GL_FRONT, GL_AMBIENT, input[k].object->mList[lastMaterial].Ka);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, input[k].object->mList[lastMaterial].Kd);
					glMaterialfv(GL_FRONT, GL_SPECULAR, input[k].object->mList[lastMaterial].Ks);
					glMaterialfv(GL_FRONT, GL_SHININESS, &input[k].object->mList[lastMaterial].Ns);
				}
				glBegin(GL_TRIANGLES);
				for (size_t j = 0; j<3; ++j)
				{
					glMultiTexCoord2fv(GL_TEXTURE0, input[k].object->tList[input[k].object->faceList[i][j].t].ptr);
					glMultiTexCoord2fv(GL_TEXTURE1, input[k].object->tList[input[k].object->faceList[i][j].t].ptr);
					glNormal3fv(input[k].object->nList[input[k].object->faceList[i][j].n].ptr);
					glVertex3fv(input[k].object->vList[input[k].object->faceList[i][j].v].ptr);
				}
				glEnd();
			}
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			glPopMatrix();			
		}

		else if (texture[input[k].kind].kind ==  4)
		{
			glActiveTexture(GL_TEXTURE0);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
			glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glEnable(GL_TEXTURE_GEN_R);
			glEnable(GL_TEXTURE_CUBE_MAP);

			glBindTexture(GL_TEXTURE_CUBE_MAP, texture[input[k].kind].texture_id[0]);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);

			for (size_t i = 0; i < input[k].object->fTotal; ++i)
			{
				if (lastMaterial != input[k].object->faceList[i].m)
				{
					lastMaterial = (int)input[k].object->faceList[i].m;
					glMaterialfv(GL_FRONT, GL_AMBIENT, input[k].object->mList[lastMaterial].Ka);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, input[k].object->mList[lastMaterial].Kd);
					glMaterialfv(GL_FRONT, GL_SPECULAR, input[k].object->mList[lastMaterial].Ks);
					glMaterialfv(GL_FRONT, GL_SHININESS, &input[k].object->mList[lastMaterial].Ns);
				}
				glBegin(GL_TRIANGLES);
				for (size_t j = 0; j<3; ++j)
				{
					glTexCoord2fv(input[k].object->tList[input[k].object->faceList[i][j].t].ptr);
					glNormal3fv(input[k].object->nList[input[k].object->faceList[i][j].n].ptr);
					glVertex3fv(input[k].object->vList[input[k].object->faceList[i][j].v].ptr);
				}
				glEnd();
			}
			glActiveTexture(GL_TEXTURE0);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
			glDisable(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			glPopMatrix();
		}
	}
}
//////////////////////////////
////	SCENE				
//////////////////////////////

View view;
Light light;
Scene scene;
void keyboard(unsigned char key, int x, int y){
	if(key == 'w')
		view.zoom(-10);
	else if(key == 's')
		view.zoom(10);
	else if(key == 'a')
		view.rotate(-10);
	else if(key == 'd')
		view.rotate(10);
	else if(key <= '9' && key >= '0')
		scene.select(key - '1');
	glutPostRedisplay();
}
void mouse(int button, int state, int x, int y){
	static int startx, starty;
	if(state == GLUT_DOWN){
		startx = x;
		starty = y;
	}
	else if(state == GLUT_UP){
		scene.shift(x - startx, y - starty);
		glutPostRedisplay();
	}
}
void reshape(GLsizei w, GLsizei h){
	view.scaling(w, h);
}

int main(int argc, char** argv)
{	
	view.loadfile("Chess.view");
	light.loadfile("Chess.light");

	glutInit(&argc, argv);
	glutInitWindowSize(view.width(), view.height());
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("OpenGL assignment1-1");
	glewInit();
	scene.loadfile("Chess.scene");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMainLoop();

	return 0;
}

void display()
{
	// clear the buffer
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

////view////
	view.set();
////light////
	light.set();
////scene////
	scene.set();
	
	glutSwapBuffers();
	glFlush();
}
