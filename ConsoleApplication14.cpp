#include "glSetup.h"
#include "mesh.h"

#include <Eigen/Dense> //헤더파일 인클루드 하고
using namespace Eigen; // 계속 쓰기 불편하니까 using namespace 해버림


#include <iostream>
using namespace std;

#ifdef _WIN32
#define _USE_MATH_DEFINES // 윈도우즈같은 경우에는 M_PI를 사용하도록 하기 위해서 math.h파일에서 _USE_MATH_DEFINES를 써워야한다
#endif

#include <math.h>

void init(const char* filename);//초기화
void setupLight(); // 라이트세팅
void setupColoredMaterial(const Vector3f& color); //material 세팅
void render(GLFWwindow* window);//렌더링
void reshape(GLFWwindow* window, int w, int h);//reshaping
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);//키보드 세팅

//camera configuration 카메라 구성을 위해서 Eigen라이브러리를 사용
Vector3f eye(2, 2, 2);//삼차원 벡터 eye의 위치를 2,2,2로
Matrix<float, 3, 1> center(0, 0.2f, 0); // vector3f와 같은데 3은 행 갯수, 1은 열 갯수
Vector3f up(0, 1, 0);//up 벡터 설정

//Light configuration 빛 구성
Vector4f light(5.0, 5.0, 0.0, 1); //호모지니어스를 썼었기때문에

//play configuration
bool pause = true;
float timeStep = 1.0f / 120;//120fps
float period = 8.0;
float theta = 0; // 버니를 회전시키기 위한 세타값

//Eigen/Opengl rotation
bool eigen = false; //회전 방법을 eigen으로 할것인가 opengl으로 할것인가

//Global coordinate frame 전역 좌표 프레임
bool axes = false;
float AXIS_LENGTH = 1.5;
float AXIS_LINE_WIDTH = 2;


//colors 뒷배경 색깔
GLfloat bgColor[4] = { 1,1,1,1 };

//어떤 매쉬파일을 읽을 것인가
const char* defaultMeshFileName = "m01_bunny.off";

//컨트롤 변수
bool aaEnabled = true; //antialiasing 에일리어싱 제거(디스플레이 화상도를 높이기 위해 에일리어싱을 제거하는 것)
bool bfcEnabled = true; // back face culling viewer에게 보이지 않는 후면을 제거하는 것. 큰 시간절약이 가능

//매쉬를 읽음
MatrixXf vertex; // vertex를 matrixXf로 읽었다. 즉 동적 크기를 가지는 XxX으로 읽을것
Matrix <float, Dynamic, Dynamic> faceNormal; //n개의 각각의 페이스에서 페이스노말 벡터를 보관하기로 했어요
MatrixXf vertexNormal; // vertexNormal vector를 보관할 장소를 마련
ArrayXXi face; // int로 이루어진 array를 만들건데 row column 모두 가변적이다 face index 를 위한 데이터 스트럭쳐
//매쉬를 읽어주는 부분에서 이 부분을 채워줄것임


//매쉬 읽은 것으로 부터 shrunken된 face를 구해야함 그래서 각각 shrunken 된 face마다 삼각형마다 vertex가 3개니까 3개의 vertex를 새로 구해야함
// 다시 말해서 n개의 삼각형으로 구성되어있다면 shrunken된 face는 3*n vertex를 가져야함 각 face마다 3개의 vertex를 옆 삼각형들과 공유를 못하니까 개별적으로 가져야함 그것을 만들어내야함
//mesh with shrunken faces
MatrixXf faceVertex;

//control variables
bool faceWithGapMesh = true; // shrunken된 mesh를 gapmesh라고 부름 shrunken된 얘들 그릴것이냐 말것이냐
bool useFaceNormal = true;// facenormal 벡터를 사용해서 렌더링을 할것이냐 vertexnormal벡터를 사용해서 렌더링을 할것이냐
float gap = 0.1f; // 그리고 그 gap 줄어드는 것은 어느정도로 할것이냐 초기는 10%줄게 해놓음 간이 슬라이드에서 알파라고 부르는 값

int main(int argc, char* argv[])
{
	//mesh filename 어떤 파일을 읽을것이냐
	const char* filename; //filename을 읽고 이곳에서는 51번째 줄의 bunny 매쉬
	if (argc >= 2) filename = argv[1];//argv[0]가 실행시키는 파일, argv[0이상은] 그 코드 안에서 필요로하는 파일을 가져옴 ,초기 실행시킬때 필요한 값을 가져오는것
	else filename = defaultMeshFileName;

	//Initialize the openGL system
	GLFWwindow* window = initializeOpenGL(argc, argv, bgColor);
	if (window == NULL) return -1;

	//CALLBACKs
	glfwSetKeyCallback(window, keyboard); //callback과 키보드 callback

	//Depth test
	glEnable(GL_DEPTH_TEST); //레이트레이싱 할때 씀, 빛이 들어오면 깊이가 필요함, 빛과 관련

	//Normal vectors are normalized after transformation
	glEnable(GL_NORMALIZE); //normal vector normalized안되있으면 normalize해라

	//Viewport and perspective setting
	reshape(window, windowW, windowH);

	//Initialization -MainLoop - FInalization
	init(filename); // 매쉬 name으로 초기화 해라

	//Main loop 무한 루프!
	float previous = (float)glfwGetTime();
	float elapsed = 0;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		//컴터 그래픽스 실습과 거의 같은 부분 ->
		//Time passed during a single loop
		float now = (float)glfwGetTime();
		float delta = now - previous;
		previous = now;

		//Time passed after the previous frame
		elapsed += delta;

		//Deal with the current frame
		if (elapsed > timeStep)
		{
			if (!pause) theta += float(2.0 * M_PI) / period * elapsed;
			//pause 된게 아니라면 theta값을 정할거고

			elapsed = 0; // rest the elapsed time
		}
		//<--
		render(window); //위의 init함수와 render 함수를 살펴보자
		glfwSwapBuffers(window);

	}
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
void
buildShrunkenFaces(const MatrixXf& vertex, MatrixXf& faceVertex)
{
	faceVertex.resize(3, 3 * face.cols());
	//index가 3니까 3..... face마다 3개의 vertex가 필요 공유되지않고 각각 3개씩 가져야하니까 그래서 3*face.cols...총 몇개의 vertex? face의 개수 * 3개
	for (int i = 0; i < face.cols(); i++)// 각각의 face마다 low vector는 index 3개였고 column vector는 face.cols()를 하면 구할수 있음
	{
		//i번째 face의 center of mesh를 구히야함
		Vector3f center(0, 0, 0); //초기값
		for (int j = 0; j < 3; j++)
			center += vertex.col(face(j, i)); //3개에 대해서 더해줌,  vertex의 column을....그런데 i번째 face의 j번째 index를 가져와서 위치를 더해주고
		center /= 3.0f; //평균을 내야하니까 3분의1

		for (int j = 0; j < 3; j++)
		{
			const Vector3f& p = vertex.col(face(j, i));//원래 오리지날 포지션
			faceVertex.col((3 * i) + j) = p + gap * (center - p);//p는 현재 포지션으로부터 center로 가는 방향을 구했음, 슬라이드상에서 알파만큼 여기선gap 비율 만큼 이동시켜주면 되겠음
		} //원래 p에서 center방향으로 이동시켜주기...이것을 우리가 새로 만든 vertex의 값으로 주어줌 face index i(face마다 3개의 index를 만들어내는 녀석)와 각각의 vertex 3개니까
		//3개마다 이동시켜서 만들어주는것
	}

}



void init(const char* filename)
{
	//read a mesh 파일 이름이 주어지면 그 파일을 읽을거임
	cout << "Reading" << filename << endl;
	readMesh(filename, vertex, face, faceNormal, vertexNormal);
	//readmesh라는 유틸리티를 만들어놈

	//shrunken 하면 됨
	//shrunken face로 이루어져있는 매쉬를 만들어야할텐데 슬라이드에서 본 식 그대로 쓰면 됨
	buildShrunkenFaces(vertex, faceVertex);

	cout << endl;
	cout << "Keyboard Input :space for play/pause" << endl;
	cout << "Keyboard Input : x for axes on/off" << endl;
	cout << "Keyboard Input : q/esc for quit" << endl;
	cout << endl;
	cout << "Keyboard Input : g for face with gap mesh/mesh" << endl; //갭이 있는 mesh를 보여줄 것이냐 없는 mesh를 보여줄 것이냐
	cout << "Keyboard Input : up/down to increase/decrease the gap between faces" << endl;
	cout << "Keyboard Input : n for vertex/face normal" << endl;
	cout << "Keyboard Input : e for rotation with Eigen" << endl;
	cout << "Keyboard Input : a for antiaiasing on/off" << endl;
	cout << "Keyboard Input : b for back face culling on/off" << endl;

}

void // mesh를 그리기 위해서는 어떡해야하나요
drawMesh()
{
	setupColoredMaterial(Vector3f(0.95f, 0.95f, 0.95f)); //coloredmaterial 세팅


	glEnable(GL_POLYGON_OFFSET_FILL);

	glPolygonOffset(2.0, 2.0);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < face.cols(); i++)
	{
		glNormal3fv(vertexNormal.col(face(0, i)).data());
		glVertex3fv(vertex.col(face(0, i)).data());

		glNormal3fv(vertexNormal.col(face(1, i)).data());
		glVertex3fv(vertex.col(face(1, i)).data());

		glNormal3fv(vertexNormal.col(face(2, i)).data());
		glVertex3fv(vertex.col(face(2, i)).data());
	}
	glEnd();

	glDisable(GL_POLYGON_OFFSET_FILL);

}

void
drawShrunkenFaces()
{
	setupColoredMaterial(Vector3f(0.f, 0.f, 0.95f));

	glBegin(GL_TRIANGLES);
	for (int i = 0; i < face.cols(); i++)
	{
		if (useFaceNormal) glNormal3fv(faceNormal.col(i).data()); //faceNormal Vector를 사용한다면 그 face마다 normal 한번만 설정하면 됨
		for (int j = 0; j < 3; j++)
		{
			if (!useFaceNormal) glNormal3fv(vertexNormal.col(face(j, i)).data());//vertex normal을 사용한다면 3개의 값을 줘야함
			glVertex3fv(faceVertex.col((3 * i) + j).data());//그 위치는 facevertex에 설정해놨으니까 i번째 face에서 j(3개의 위치)를 주면 됨
		}
	}
	glEnd();
}
void
wireFrame()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	setupColoredMaterial(Vector3f(0.0f, 0.0f, 0.0f)); //coloredmaterial 세팅

	glBegin(GL_TRIANGLES);//삼각형을 그리는데
	for (int i = 0; i < face.cols(); i++)
	{
		glNormal3fv(vertexNormal.col(face(0, i)).data());
		glVertex3fv(vertex.col(face(0, i)).data());

		glNormal3fv(vertexNormal.col(face(1, i)).data());
		glVertex3fv(vertex.col(face(1, i)).data());

		glNormal3fv(vertexNormal.col(face(2, i)).data());
		glVertex3fv(vertex.col(face(2, i)).data());
	}
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//위에서 GL_Line을 사용해서 wireframe을 그리고 다시 삭제하기위해선 GL_FILL을 사용해서 채움

}

void
render(GLFWwindow* window)
{
	//antialiasing 할꺼나 말꺼냐
	if (aaEnabled) glEnable(GL_MULTISAMPLE);
	else glDisable(GL_MULTISAMPLE);

	//back face culling 할거냐 말거냐
	if (bfcEnabled)
	{
		glEnable(GL_CULL_FACE); //back face 와 front face는 어떻게 구별할거임?
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);//front face는 counter clover wize order로 준거다
	}
	else glDisable(GL_CULL_FACE);

	//Background color
	glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
	//------------------8페이지
	if (axes) //그릴거니 말거니?
	{
		glDisable(GL_LIGHTING);
		drawAxes(AXIS_LENGTH, AXIS_LINE_WIDTH);
	}
	//Lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	setupLight();

	if (eigen)
	{
		//4X4 homogeneous matrix
		Matrix4f T; T.setIdentity(); //T를 identity matrix 로 함

		T.block<3, 3>(0, 0) = Matrix3f(AngleAxisf(theta, Vector3f::UnitY()));
		//T의 4X4에서 3X3matrix를 얻어오는데(0,0)을 시작으로 3X3를 뽑아낸다...............AngleAxisf(theta, Vector3f::UnitY())
		// (AngleAxisf회전시켜주는 <-theta만큼 <-Y축을 중심으로)
		//Y축을 중심으로 theta만큼 회전시켜주는 angleaxis회전으로부터 Matrix3f(3X3matrix)를 얻어내는것
		//angleaxis로 부터 3X3 matrix를 얻어낼수가 있구나! 그것으로 L Value 3X3만큼 얻어내는거예요
		//T.block<3, 3>(0, 0) = 여기가 L Value ....Matrix3f(AngleAxisf(theta, Vector3f::UnitY()));여기가 R Value
		//L Value는 대입이 될수 있는 값 R Value는 값

		T.block<3, 1>(0, 3) = Vector3f(0.0f, 0.0f, 0.0f); //3X1 Vector를 뽑아냈어. 어디서? 0번째 row의 3번째 column 즉 맨 오른쪽 끝의 vector 그중에 x,y,z에 해당하는 값 translation 부분을  0,0,0으로 설정

		glMultMatrixf(T.data());//위에 있는 값들에 우리들이 지금 설정한 바로 위의 값을 multiply 하겠다
	}
	else
	{
		glRotatef(theta / float(M_PI) * 180.0f, 0, 1, 0);//기본적으로 eigen이 꺼져있어서 opengl을 사용해서 rotation을 해줄거임
	}
	//--------------9페이지
	//if (faceWithGapMesh)
	drawShrunkenFaces();
	//else
	{
		drawMesh();
		//wireFrame();
	}
}

void
setupColoredMaterial(const Vector3f& color)
{
	GLfloat mat_ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat mat_diffuse[4] = { color[0], color[1], color[2], 1.0f };
	GLfloat mat_specular[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat mat_shininess = 100.0f;

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

}

void
setupLight()
{
	GLfloat ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light.data());
}

void
keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
		case GLFW_KEY_Q:
		case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;

		case GLFW_KEY_SPACE: pause = !pause; break;

		case GLFW_KEY_E: eigen = !eigen; break;

		case GLFW_KEY_G: faceWithGapMesh = !faceWithGapMesh; break;

		case GLFW_KEY_UP:
			gap = min(gap + 0.05f, 0.5F);
			buildShrunkenFaces(vertex, faceVertex);
			break;
		case GLFW_KEY_DOWN:
			gap = max(gap - 0.05f, 0.05f);
			buildShrunkenFaces(vertex, faceVertex);
			break;

		case GLFW_KEY_N: useFaceNormal = !useFaceNormal; break;

		case GLFW_KEY_A: aaEnabled = !aaEnabled; break;

		case GLFW_KEY_B: bfcEnabled = !bfcEnabled; break;

		case GLFW_KEY_X: axes = !axes; break;
		}
	}
}