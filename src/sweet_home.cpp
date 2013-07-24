#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<iostream>
#include <math.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h" //load library imageloader.h
#include "vec3f.h" //load library vec3f.h

GLuint texture[2]; // inisialisai texture tpenya array
static int goParkir = 0, elbow = 0,maju =0,geserLawang = 0;//inisialisasi nilai untuk menggerakan mobil dan pintu garasi
static float spin, spin2 = 0.0; //inisialisasi nilai spin1 dan spin 2
int z=0;//inisialisasi nilai z

GLint slices = 16; //variabel
GLint stacks = 16; //variabel tumpukan

//definisi struktur data untuk image
struct Images {
unsigned long sizeX;
unsigned long sizeY;
char *data;
};
typedef struct Images Images; //struktur data untuk image

#define checkImageWidth 164
#define checkImageHeight 164

GLubyte checkImage[checkImageWidth][checkImageHeight][3]; //inisialisasi tinggi dan lebar gambar

float angle = 0; //inisialisasi nilai angle
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = 50; // nilai view x
static int viewy = 32; // nilai view y
static int viewz = 80; // nilai view z
double rx = 1.0;
double ry = 1.0;
GLUquadricObj *qobj; //inisialisasi quadrick tipenya linker
float rot = 0;

//Class untuk terrain
class Terrain {
private:
	int w; //Lebar
	int l; //Tinggi
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals;
public:
	//konstruktor terrain
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}
	// fungsi untuk menentukan lebar terrain
	int width() {
		return w;
	}
	// fungsi untuk menentukan tinggi terrain
	int length() {
		return l;
	}

	//mengatur tinggi pada titik x,z sampai y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Hasil nilai pada titik x,z
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);
				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//akhir class terrain


void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / (float)h, 1.0, 200.0);
}

//pemanggilan Terrain
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 30.0f;
//membuat tipe data terain

Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;
Terrain* _terrainJalan;

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void cleanup() {
	delete _terrain;
	delete _terrainTanah;
}

//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	float scale = 500.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Membuat OpenGL menggambar segitiga pada setiap tiga simpul berturut-turut
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

//fungsi untuk cek image
void makeCheckImage(void) {
	int i, j, c;
	for (i = 0; i < checkImageWidth; i++) {
		for (j = 0; j < checkImageHeight; j++) {
		c = ((((i & 0x8) == 0) ^ ((j & 0x1) == 0))) * 255;
		checkImage[i][j][0] = (GLubyte) c;
		checkImage[i][j][1] = (GLubyte) c;
		checkImage[i][j][2] = (GLubyte) c;
		}
	}
}

//mengambil gambar BMP
int ImageLoad(char *filename, Images *images) {
	FILE *file;
	unsigned long size; // ukuran gambar dalam bytes
	unsigned long i; // standard counter.
	unsigned short int plane; // number of planes in image

	unsigned short int bpp; // jumlah bits per pixel
	char temp; // temporary color storage for var warna sementara untuk memastikan filenya ada


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}
// mencari file header bmp
fseek(file, 18, SEEK_CUR);
// read the width
if ((i = fread(&images->sizeX, 4, 1, file)) != 1) {
	printf("Error reading width from %s.\n", filename);
	return 0;
}
printf("Width of %s: %lu\n", filename, images->sizeX);
// membaca nilai height
if ((i = fread(&images->sizeY, 4, 1, file)) != 1) {
	printf("Error reading height from %s.\n", filename);
	return 0;
}
printf("Height of %s: %lu\n", filename, images->sizeY);
//menghitung ukuran image(asumsi 24 bits or 3 bytes per pixel).

size = images->sizeX * images->sizeY * 3;
// read the planes
if ((fread(&plane, 2, 1, file)) != 1) {
	printf("Error reading planes from %s.\n", filename);
	return 0;
}
if (plane != 1) {
	printf("Planes from %s is not 1: %u\n", filename, plane);
	return 0;
}
// read the bitsperpixel
if ((i = fread(&bpp, 2, 1, file)) != 1) {
	printf("Error reading bpp from %s.\n", filename);
	return 0;
}
if (bpp != 24) {
	printf("Bpp from %s is not 24: %u\n", filename, bpp);
	return 0;
}
// seek past the rest of the bitmap header.
fseek(file, 24, SEEK_CUR);
// membaca data gambar.
images->data = (char *) malloc(size);
if (images->data == NULL) {
	printf("Error allocating memory for color-corrected image data");
	return 0;
}
if ((i = fread(images->data, size, 1, file)) != 1) {
	printf("Error reading image data from %s.\n", filename);
	return 0;
}
	for (i = 0; i < size; i += 3) { // membalikan semuan nilai warna (gbr - > rgb)
		temp = images->data[i];
		images->data[i] = images->data[i + 2];
		images->data[i + 2] = temp;
	}
return 1;
}

//mengambil tekstur
Images * loadTexture() {
Images *image1;
// lokasi memmory untuk tekstur
image1 = (Images *) malloc(sizeof(Images));
	if(image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
//pic.bmp ukuranya 64x64 picture
	if(!ImageLoad("genteng.bmp", image1)) {
		exit(1);
	}
	return image1;
}

//fungsi untuk membuat pilar rumah
void pilar(){
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj,GLU_FILL);
	gluCylinder(qobj,0.5,0.5,10,45,2);//membuat bentuk silinder atau tabung
	glDisable(GL_TEXTURE_2D);
}

//fungsi untuk membuat mobil
void mobil(void){
	//Body Mobil bawah
	glPushMatrix();//memasukan matriks pada stack.
	glScaled(1, 0.37, 2);//mengatur besar kecilnya ukuran objek
	glTranslatef(-3, 2.5, 0);//mengatur posisi objek berdasarkan koordinat
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);//membuat warna depan dan belakang pada tiap sisi objek
	glColor3f(1, 0, 0);//untuk memberi warna
	glutSolidCube(5.0);//membuat objek kubus atau balok
	glPopMatrix();//menghentikan matriks.

	//Body Mobil Atas
	glPushMatrix();
	glScaled(1, 0.35, 1);
	glTranslatef(-3, 7.75, -1);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 0);
	glutSolidCube(5.0);
	glPopMatrix();

	//body kaca depan
	glPushMatrix();
	glScaled(1, 0.45, 0.3);
	glTranslatef(-3, 4.4, 4.7);
	glRotatef(50, 1, 0, 0);//fungsi untuk memutar objek
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 0);
	glutSolidCube(5.0);
	glPopMatrix();

	//body kaca belakang
	glPushMatrix();
	glScaled(1, 0.45, 0.3);
	glTranslatef(-3, 4.4, -11.4);
	glRotatef(40, 1, 0, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 0);
	glutSolidCube(5.0);
	glPopMatrix();

	//kaca depan
	glPushMatrix();
	glScaled(0.8, 0.4, 0.3);
	glTranslatef(-3.75, 4.8, 5.4);
	glRotatef(50, 1, 0, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//kaca samping kanan
	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-50, 9.5, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-50, 9.5, -6);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//kaca samping kiri
	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-550, 9.5, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-550, 9.5, -6);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//ban kanan belakang
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-0.5, 0.4, -3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);//membuat objek donat
	glPopMatrix();

	//ban kanan depan
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-0.5, 0.4, 2.7);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//ban kiri depan
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-5.5, 0.4, 2.7);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//ban kiri belakang
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-5.5, 0.4, -3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//Lampu depan
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-1.5, 1.2, 5.2);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glRotatef(180, 0, 1, 0);
	glutSolidCone(0.5,1,45,1); //fungsi untuk membuat bentuk kerucut
	glPopMatrix();

	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-4.5, 1.2, 5.2);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glRotatef(180, 0, 1, 0);
	glutSolidCone(0.5,1,45,1);
	glPopMatrix();

	//bemper
	glPushMatrix();
	glScaled(1.05, 0.05, 0.25);
	glTranslatef(-2.8, 5, 18);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(1.05, 0.05, 0.22);
	glTranslatef(-2.8, 5, -21);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

}
//fungsi untuk membuat mobil 2
void mobil2(void){
	//Body Mobil bawah
	glPushMatrix();
	glScaled(1, 0.37, 2);
	glTranslatef(-3, 2.5, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//Body Mobil Atas
	glPushMatrix();
	glScaled(1, 0.35, 1);
	glTranslatef(-3, 7.75, -1);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//body kaca depan
	glPushMatrix();
	glScaled(1, 0.45, 0.3);
	glTranslatef(-3, 4.4, 4.7);
	glRotatef(50, 1, 0, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//body kaca belakang
	glPushMatrix();
	glScaled(1, 0.45, 0.3);
	glTranslatef(-3, 4.4, -11.4);
	glRotatef(40, 1, 0, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 0, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//kaca depan
	glPushMatrix();
	glScaled(0.8, 0.4, 0.3);
	glTranslatef(-3.75, 4.8, 5.4);
	glRotatef(50, 1, 0, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//kaca samping kanan
	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-50, 9.5, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-50, 9.5, -6);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//kaca samping kiri
	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-550, 9.5, 0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(0.01, 0.27, 0.4);
	glTranslatef(-550, 9.5, -6);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//ban kanan belakang
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-0.5, 0.4, -3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//ban kanan depan
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-0.5, 0.4, 2.7);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//ban kiri depan
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-5.5, 0.4, 2.7);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//ban kiri belakang
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-5.5, 0.4, -3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glRotatef(90, 0, 90, 0);
	glutSolidTorus(0.4,0.6,20,40);
	glPopMatrix();

	//Lampu depan
	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-1.5, 1.2, 5.2);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glRotatef(180, 0, 1, 0);
	glutSolidCone(0.5,1,45,1);
	glPopMatrix();

	glPushMatrix();
	glScaled(1, 1, 1);
	glTranslatef(-4.5, 1.2, 5.2);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glRotatef(180, 0, 1, 0);
	glutSolidCone(0.5,1,45,1);
	glPopMatrix();

	//bemper
	glPushMatrix();
	glScaled(1.05, 0.05, 0.25);
	glTranslatef(-2.8, 5, 18);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(1.05, 0.05, 0.22);
	glTranslatef(-2.8, 5, -21);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

}

//fungsi untuk membuat pagar rumah
void pagar(){
	//Patok pager1
	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(35, 0.5, 105);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//Patok Pager arep
	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(-170, 0.5, 105);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(-250, 0.5, 105);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//Patok pager2
	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(-77, 0.5, 105);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//Patok pager3
	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(-315, 0.5, 105);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//Patok pager4
	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(-315, 0.5, -285);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	//Patok pager5
	glPushMatrix();
	glScaled(0.1, 0.7, 0.1);
	glTranslatef(-315, 0.5, -143.3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	// Pager sing Miring kiri
	//Patok pager8
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(-315, 0.5, -220);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager8
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(-315, 0.5, -71.25);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager8
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(-315, 0.5, 10);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Pager Sing Miring Kiri
		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(-1590, 0, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//pager miring 1
		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(-1590, 0, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//pager miring 2
		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(-1590, 8, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//pager miring 2
		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(-1590, 16, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager6 kanan
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(35, 0.5, -285);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager7
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(35, 0.5, -143.3);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager8
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(35, 0.5, -220);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager8
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(35, 0.5, -71.25);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//Patok pager8
		glPushMatrix();
		glScaled(0.1, 0.7, 0.1);
		glTranslatef(35, 0.5, 10);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//pager sing horisontal
		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(190, 0, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(190, 8, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(0.02, 0.1, 7.9);
		glTranslatef(190, 16, -1.13);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		//pager ngarep
		glPushMatrix();
		glScaled(4.9, 0.1, 0.02);
		glTranslatef(-4, 0, 540);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(4.9, 0.1, 0.02);
		glTranslatef(-4, 8, 540);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(4.9, 0.1, 0.02);
		glTranslatef(-4, 16, 540);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1, 1, 1);
		glutSolidCube(5.0);
		glPopMatrix();
}

//fungsi untuk membuat objek pohon
void pohon(void){
	//batang pohon
	GLUquadricObj *pObj; //inisialisasi
	pObj =gluNewQuadric();
	gluQuadricNormals(pObj, GLU_SMOOTH);

	glPushMatrix();
	glColor3ub(104,70,14);
	glRotatef(270,1,0,0);
	gluCylinder(pObj, 4, 0.7, 30, 25, 25);
	glPopMatrix();
}

void LampuTaman(void){
 //tiang lampu
	GLUquadricObj *pObj;
	pObj =gluNewQuadric();
	gluQuadricNormals(pObj, GLU_SMOOTH); //
	glPushMatrix();
	glColor3f(0.4613, 0.4627, 0.4174);
	glTranslatef(10,0,21);
	glRotatef(270,1,0,0);
	gluCylinder(pObj, 0.3, 0.2, 7, 25, 25);
	glPopMatrix();

	glPushMatrix();
	glColor3f(0.4613, 0.4627, 0.4174);
	glTranslatef(10,6.85,21.05);
	glRotatef(220,1,0,0);
	gluCylinder(pObj, 0.2, 0.1, 3, 25, 25);
	glPopMatrix();

	//lampunya
	glPushMatrix();
	glScaled(0.18, 0.085, 0.2);
	glTranslatef(55.5, 104, 92);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glutSolidCube(5.0);
	glPopMatrix();

	 glPushMatrix();
	 glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	 glColor3ub(153, 223, 255);
	 glTranslatef(10,8.6,18.4);
	 glutSolidSphere(0.4, 20, 20);//fungsi membuat bola
	 glPopMatrix();
}

//membuat ranting dan daun
void ranting(void){
	GLUquadricObj *pObj;
	pObj =gluNewQuadric();
	gluQuadricNormals(pObj, GLU_SMOOTH); //
	glPushMatrix();
	glColor3ub(104,70,14);
	glTranslatef(0,27,0);
	glRotatef(330,1,0,0);
	gluCylinder(pObj, 0.6, 0.1, 15, 25, 25);
	glPopMatrix();

	//daun
	glPushMatrix();
	glColor3ub(18,118,13);
	glScaled(5, 5, 5);
	glTranslatef(0,7,3);
	glutSolidDodecahedron();
	glPopMatrix();
}

//fungsi membuat bentuk pohon dengan parameter (skalax, skalay, skalaz,translasix,translasiy,translasiz)
void uwit(GLfloat skalax,GLfloat skalay,GLfloat skalaz,GLfloat tx,GLfloat ty,GLfloat tz){
	glPushMatrix();
			glTranslatef(tx,ty,tz);
			glScalef(skalax, skalay, skalaz);
			glRotatef(90,0,1,0);
			pohon();//pemanggilan fungsi pohon
			//ranting1
			ranting();

			//ranting2
			glPushMatrix();
			glScalef(1.5, 1.5, 1.5);
			glTranslatef(0,25,25);
			glRotatef(250,1,0,0);
			ranting();//pemanggilan fungsi ranting
			glPopMatrix();

			//ranting3
			glPushMatrix();
			glScalef(1.8, 1.8, 1.8);
			glTranslatef(0,-6,21.5);
			glRotatef(-55,1,0,0);
			ranting();//pemanggilan fungsi ranting
			glPopMatrix();

		glPopMatrix();
}

//fungsi membuat bentuk jendela dengan parameter (skalax, skalay, skalaz,translasix,translasiy,translasiz)
void jendela(GLfloat skalax, GLfloat skalay, GLfloat skalaz, GLfloat tx, GLfloat ty, GLfloat tz){
	glPushMatrix();
	glScaled( skalax, skalay, skalaz);
	glTranslatef(tx, ty, tz);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(0, 0, 0);
	glutSolidCube(5.0);
	glPopMatrix();
}

//fungsi membuat bangku
void bangkukosong(){
	//korsi nang arep kolam
	glPushMatrix();
	glScaled( 0.3, 0.1, 0.7);
	glTranslatef(-75, 5, 40);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled( 0.3, 0.3, 0.1);
	glTranslatef(-75, -0.2, 260);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
	glScaled( 0.3, 0.3, 0.1);
	glTranslatef(-75, -0.2, 295);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 0);
	glutSolidCube(5.0);
	glPopMatrix();
}
//fungsi membuat pintu garasi
void lawangGarasi(){
	glPushMatrix();
	glScaled(1.5, 1.25, 0.15);
	glTranslatef(-2.2, 2, -70);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(97, 91, 91);
	glutSolidCube(5.0);
	glPopMatrix();
}
//fungsi membuat awan
void awan(){
     glPushMatrix();
     glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3ub(153, 223, 255);
     glTranslatef(3,12,2);
     glutSolidSphere(3, 30, 30);//fungsi membuat bola
     glPopMatrix();

     glPushMatrix();
     glTranslatef(0,11,2);
     glutSolidSphere(2, 30, 30);
     glPopMatrix();

     glPushMatrix();
     glTranslatef(6,11.5,2);
     glutSolidSphere(2.5, 30, 30);
     glPopMatrix();

     glPushMatrix();
     glTranslatef(8,11,2);
     glutSolidSphere(1.75, 30, 30);
     glPopMatrix();
}

//fungsi membuat rumah
void rumah(void){
		//Lampu Taman
		glPushMatrix();
		glTranslatef(-50.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-40.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-30.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-20.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-10.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(0.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(10.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(20.7, -1,0.5);
		LampuTaman();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(30.7, -1,0.5);
		LampuTaman();
		glPopMatrix();
	    //pilar1
		glPushMatrix();
		glScaled(1,0.8 ,1);
		glTranslatef(-10.7, 9.4,3.2);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glRotatef(90, 90, 0, 0);
		pilar();//pemanggilan fungsi pilar
		glPopMatrix();
		//pilar2
		glPushMatrix();
		glScaled(1,0.8 ,1);
		glTranslatef(-21.9, 9.4,3.2);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glRotatef(90, 90, 0, 0);
		pilar();//pemanggilan fungsi pilar
		glPopMatrix();

		//atap rumah tengah
		glPushMatrix();
		glScaled(4, 3, 3);
		glTranslatef(-5, 2.85, -5.2);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		//glColor3d memberi warna lebih detail
		glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
		glRotatef(45, 1, 0, 0);
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glutSolidCube(5.0);
		glPopMatrix();

		//atap rumah kanan
		glPushMatrix();
		glScaled(4, 2, 2);
		glTranslatef(-2, 4.4, -8);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
		glRotatef(45, 1, 0, 0);
		glutSolidCube(5.0);
		glPopMatrix();

		// didnding kiri
		glPushMatrix();
		glScaled(4.01, 3.5, 3.9);
		glTranslatef(-4.99, -0.4, -4);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		//Hiasan dinding kiri
		glPushMatrix();
		glScaled(0.5, 0.4, 0.15);
		glTranslatef(-58.5, 2, -38.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

		glColor3ub(97, 91, 91);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(0.5, 0.4, 0.15);
		glTranslatef(-58.5, 8, -38.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(97, 91, 91);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(0.5, 0.4, 0.15);
		glTranslatef(-58.5, 14, -38.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(97, 91, 91);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(0.3, 2, 2.2);
		glTranslatef(4.25, 1.4, -7);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(0.6, 2, 2.2);
		glTranslatef(-14.3, 1.4, -7);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(2.5, 0.5, 2.2);
		glTranslatef(-1.69, 12.5, -7);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		//pintu garasi
		glPushMatrix();
		glTranslatef((GLfloat) geserLawang, 0.0, 0.0);
		lawangGarasi();
		glPopMatrix();

		//dalem garasi
		glPushMatrix();
		glScaled(1.5, 1.5, 0.3);
		glTranslatef(-2.2, 1.75, -37);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0, 0, 0);
		glutSolidCube(5.0);
		glPopMatrix();

        //dinding kanan bag blkg
		glPushMatrix();
		glScaled(2.5, 2, 2.2);
		glTranslatef(-1.69, 1.4, -9);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		//dinding depan
		glPushMatrix();
		glScaled(2.51, 1.65, 1);
		glTranslatef(-6.475, 1.99, -5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		//Pintu
		glPushMatrix();
		glScaled( 0.6, 1.2, 0.1);
		glTranslatef(-27, 2.3, -25);
 		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
 		glColor3ub(76, 38, 10);
 		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.4, 1, 0.1);
		glTranslatef(-40, 2.8, -24.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(138,77,25);
		glutSolidCube(5.0);
		glPopMatrix();
		//gagang pintu
		glPushMatrix();
		glScaled( 0.1, 0.1, 0.1);
		glTranslatef(-174, 26, -21);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0,0,0);
		glutSolidSphere(2,20,10);
		glPopMatrix();


		glPushMatrix();
		jendela(0.5, 1, 0.1,-25, 3.3, -27);
		glPopMatrix();

		// atap teras tengah
		glPushMatrix();
		glScaled(3.3, 0.049, 2.6);
		glTranslatef(-5, 155.2, -1);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();

		// atap teras kiri
		glPushMatrix();
		glScaled(2.5, 0.049, 1);
		glTranslatef(-9.49, 155.2, -6.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();

		// atap teras kanan
		glPushMatrix();
		glScaled(2.5, 0.049, 1);
		glTranslatef(-1.71, 155.2, -10.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();
		// latar
		glPushMatrix();
		glScaled(6.4, 0.049, 5);
		glTranslatef(-2.2, -14, -0.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(1.75, 0.049, 3);
		glTranslatef(-1.4, -14.1, 1.27);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.4613, 0.4627, 0.4174);
		glutSolidCube(5.0);
		glPopMatrix();

		// lantai tengah
		glPushMatrix();
		glScaled(2.5, 0.049, 2.6);
		glTranslatef(-6.5, -13, -1);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();
		// atap teras tengah atas
		glPushMatrix();
		glScaled(2, 0.05, 2.6);
		glTranslatef(-8.1, -10, -1.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(0.73, 0.108, 0.203);
		glutSolidCube(5.0);
		glPopMatrix();

		// penghias atap teras kanan
		glPushMatrix();
		glScaled(2.1, 0.15, 0.05);
		glTranslatef(-1.49, 51.6, -156.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();

		// penghias atap teras tengah
		glPushMatrix();
		glScaled(3.3, 0.15, 0.05);
		glTranslatef(-5, 51.6, 75.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();

		// penghias atap teras kiri
		glPushMatrix();
		glScaled(1.05, 0.15, 0.05);
		glTranslatef(-26, 51.6, -77.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();

		// penghias atap teras samping kanan
		glPushMatrix();
		glScaled(0.05, 0.15,2.35);
		glTranslatef(-165, 51.6, -0.84);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();

		// penghias atap teras samping kanan
		glPushMatrix();
		glScaled(0.05, 0.15,1.55);
		glTranslatef(-495, 51.6,0.01);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(5.0);
		glPopMatrix();


		glPushMatrix();
		glTranslatef(0.0, 0.0, (GLfloat) elbow);
		glRotatef((GLfloat) goParkir, 0.0, 1.0, 0.0);
		glTranslatef(0.0, 0.0, (GLfloat) maju);
		mobil();
		glPopMatrix();

		//mobil2
		glPushMatrix();
		glTranslatef(-30, 0.0, 12.5);
		glRotatef(90, 0.0, 1.0, 0.0);
		mobil2();
		glPopMatrix();

		glPushMatrix();
		pagar();
		glPopMatrix();

		//memanggil fungsi pohon1
		glPushMatrix();
		uwit(0.25,0.25,0.25, 7,-1.5,5);
		glPopMatrix();

		glPushMatrix();
		uwit(0.2,0.2,0.2, -40,-1.5,5);
		glPopMatrix();

		glPushMatrix();
		uwit(0.1,0.1,0.1, 30,-1.5,6);
		glPopMatrix();

		glPushMatrix();
		uwit(0.1,0.1,0.1, 39,-1.5,6);
		glPopMatrix();

		glPushMatrix();
		uwit(0.1,0.1,0.1, 47,-1.5,6);
		glPopMatrix();

		glPushMatrix();
		glRotatef(90, 0, 1, 0);
		uwit(0.2,0.2,0.2, -28,-1.5,-25);
		glPopMatrix();

		glPushMatrix();
		glRotatef(90, 0, 1, 0);
		uwit(0.2,0.2,0.2,-5,-1.20,25);
		glPopMatrix();

		glPushMatrix();
		glRotatef(90, 0, 1, 0);
		uwit(0.1,0.1,0.1,2,-1.20,25);
		glPopMatrix();

		glPushMatrix();
		glRotatef(90, 0, 1, 0);
		uwit(0.17,0.17,0.17,10,-1.20,25);
		glPopMatrix();

		glPushMatrix();
		glRotatef(90, 0, 1, 0);
		uwit(0.1,0.1,0.1,20,-1.20,25);
		glPopMatrix();

		glPushMatrix();
		glRotatef(90, 0, 1, 0);
		uwit(0.1,0.1,0.1,30,-1.20,25);
		glPopMatrix();

		//kaca jendela
		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-65.5, 5, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-59.5, 5, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-65.5, 11, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-59.5, 11, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		jendela(0.5, 1, 0.1,-40, 3.3, -27);
		glPopMatrix();

		//kaca jendela
		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-103, 5, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-97, 5, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-103, 11, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-97, 11, -26.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		jendela(0.5, 1, 0.1,-50, 3.3, -60);
		glPopMatrix();

		//kaca jendela
		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-128, 5, -59.8);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-121.5, 5, -59.8);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-128, 11, -59.8);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled( 0.2 ,0.4, 0.1);
		glTranslatef(-121.5, 11, -59.8);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(84, 198, 231);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		bangkukosong();//fungsi objek bangku
		glPopMatrix();

        //Awan
		glPushMatrix();
		glTranslatef(0,17,-10);
		awan();//fungsi objek awan
		glPopMatrix();

		glPushMatrix();
		glTranslatef(10,27,-20);
		awan();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(30,27,-20);
		awan();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-15,14,-15);
		awan();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-30,16,0);
		awan();
		glPopMatrix();

		//Matahari
		glPushMatrix();
		 glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		 glColor3f(1, 1, 0);
		 glTranslatef(-15,35,-20);
		 glutSolidSphere(4, 35, 35);
		 glPopMatrix();

	glutSwapBuffers();
}



//untuk menampilkan semua objek
void display(void) {
	glClearStencil(0); //menghapus buffer stensil
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);
	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrain, 0.3f, 0.9f, 0.0f);
	glPopMatrix();

	glPushMatrix();
	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainTanah, 0.7f, 0.2f, 0.1f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

	glPushMatrix();

		glBindTexture(GL_TEXTURE_3D, texture[0]);//memanggil texture
		drawSceneTanah(_terrainJalan, 0.4902f, 0.4683f,0.4594f);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(0,5,-10);
		glScalef(5, 5, 5);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		rumah();//fungsi rumah
		glPopMatrix();

		glPushMatrix();
		glTranslatef(0,5,10);
		glScalef(5, 5, 5);
		//glBindTexture(GL_TEXTURE_2D, texture[0]);
		glPopMatrix();

	glutSwapBuffers();//menghapus buffer pembaca
	glFlush();
	rot++;
	angle++;
}

//inisialisasi semua objek yang ada
void init(void) {
	glEnable(GL_DEPTH_TEST);//Depth test digunakan untuk menghindari polygon yang tumpang tindih.
	glEnable(GL_LIGHTING);//GL_LIGHTING digunakan untuk pencahayaan lampu
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);// Set mode gradasi warna halus (Smooth)
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);
	//glShadeModel(GL_FLAT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.0, 0.0, 0.0);
		gluLookAt (0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	//panggil terrain
	_terrain = loadTerrain("heightmap02.bmp", 10);
	_terrainTanah = loadTerrain("heightmapTanah.bmp", 30);
	_terrainAir = loadTerrain("heightmapAir.bmp", 20);
	_terrainJalan = loadTerrain("heightmapJalan.bmp", 20);
	//binding texture

	Images *image1 = loadTexture();//memanggil image texture
		if (image1 == NULL) {
			printf("Image was not returned from loadTexture\n");
			exit(0);
		}
		makeCheckImage();//cek gambar

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// membuat texture
	glGenTextures(2, texture);
	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	//menyesuaikan ukuran textur ketika image lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //
	//menyesuaikan ukuran textur ketika image lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB,GL_UNSIGNED_BYTE, image1->data);

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	//menyesuaikan ukuran textur ketika image lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //
	//menyesuaikan ukuran textur ketika image lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //

	//glTexImage2D(GL_TEXTURE_2D, 0, 3, image2->sizeX, image2->sizeY, 0, GL_RGB,
	//GL_UNSIGNED_BYTE, image2->data);

	//baris tekstur buatan #belang
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	//binding texture baru
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, checkImageWidth, checkImageHeight, 0,	GL_RGB, GL_UNSIGNED_BYTE, &checkImage[0][0][0]);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
}

//fungsi keyboard
static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME: //tombol Home melihat keatas
		viewy++;
		break;
	case GLUT_KEY_END://tombol end melihat kebawah
		viewy--;
		break;
	case GLUT_KEY_UP://tombol arah atas
		viewz--;
		break;
	case GLUT_KEY_DOWN://tombol arah bawah
		viewz++;
		break;

	case GLUT_KEY_RIGHT://tombol arah kanan melihat view ke kanan
		viewx++;
		break;
	case GLUT_KEY_LEFT://tombol arah kiri melihat view ke kiri
		viewx--;
		break;
	//tombol F1 menyalakan lampu
	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);//mengkonfigurasi sumber cahaya
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);//mengkonfigurasi sumber cahaya
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
		//F2 Mematikan lampu
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	default:
		break;
	}
}

//fungsi keyboard 2
void keyboard(unsigned char key, int x, int y) {
	if (key == 'd') {//tombol d
		spin = spin - 720;
		if (spin < 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {//tombol a
		spin = spin + 720;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'q') {//tombol q megarahkan kamera ke samping kiri
		viewz++;
	}
	if (key == 'e') {//tombol e megarahkan kamera ke samping kanan
		viewz--;
	}
	if (key == 's') {//tombol s megarahkan kamera ke bawah
		viewy--;
	}
	if (key == 'w') {//tombol w megarahkan kamera ke atas
		viewy++;
	}
	if(key =='z'){ //tombol z menggerakan mobil kedepan
		elbow = (elbow + 1) % 360;
		glutPostRedisplay();
	}
	if(key == 'x'){ //tombol x menggerakan mobil kebelakang
		elbow = (elbow - 1) % 360;
		glutPostRedisplay();
	}
	if(key == 'c'){ //tombol c memutar mobil kekiri
		goParkir = (goParkir + 1) % 360;
		glutPostRedisplay();
	}
	if(key == 'v'){ //tombol c memutar mobil kekanan
		goParkir = (goParkir - 1) % 360;
		glutPostRedisplay();
	}
	if(key =='b'){ //tombol b menggerakan mobil maju kekanan
		maju = (maju + 1) % 360;
		glutPostRedisplay();
	}
	if(key == 'n'){ //tombol n menggerakan mobil maju kekiri
		maju = (maju - 1) % 360;
		glutPostRedisplay();
	}
	if(key =='f'){ //tombol f membuka pintu garasi
		geserLawang = (geserLawang - 1) % 360;
		glutPostRedisplay();
	}
	if(key == 'g'){ //tombol g meutup pintu garasi
		geserLawang = (geserLawang + 1) % 360;
		glutPostRedisplay();
	}

}



void reshape(int w, int h) {
	// Set viewport ke resolusi 900x600 viewport bisa diibaratkan layar monitor anda
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, 0.0);//translasi objek
	gluLookAt (0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);//tempat kita melihat objek
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //menambah stencil buffer ke tampilan window
	glutInitWindowSize(900, 600);//mengatur ukuran window sebanyak 900x600
	glutInitWindowPosition(100, 100);//posisi window padasaat running
	glutCreateWindow("Sweet Home IF-9");//judul windows
	init();//fungsi init
	glutDisplayFunc(display);//fungsi dari display object yang menggabungkan objek sate lighting
	glutIdleFunc(display);
	glutReshapeFunc(reshape);//reshape memanggil fungsi reshape
	glutSpecialFunc(kibor);//memanggil fungsi keyboard

	glutKeyboardFunc(keyboard);//memanggil fungsi keyboard

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); //mengatur cahaya kilau pada objek
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);/// mengkonfigurasi sumber cahaya
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);///mengatur cahaya kilau pada objek
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);// mengatur definisi kekilauan
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	glutMainLoop();// Looping fungsi main
	return 0;
}
