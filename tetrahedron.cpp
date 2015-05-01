#include <windows.h>
#include <math.h>
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

#include <D3dx9core.h>
#pragma comment(lib,"D3dx9.lib")


#define IDC_OX_BUTTON 101  // Идентификаторы кнопок в окне управления
#define IDC_OY_BUTTON 102
#define IDC_OZ_BUTTON 103
#define IDC_STOP_BUTTON 104
#define IDC_EXIT_BUTTON 105

struct vertex   // Класс для вершин: местоположение, нормаль грани, цвет грани
{
	D3DXVECTOR3 v;
	D3DXVECTOR3 normal;
	unsigned long color;
};


LRESULT _stdcall WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
void main();
void init(HINSTANCE);
void shutdown();

void rotateX();
void rotateY();
void rotateZ();
void (*rotate)(); // Указатель хранит адрес функции текущего вращения
D3DXVECTOR3 Cross(D3DXVECTOR3,D3DXVECTOR3); // Функция векторного произведения векторов
HWND hWnd;        // Главное окно
HWND hWnd2;       // Окно управления
MSG msg;

D3DPRESENT_PARAMETERS pp;
IDirect3D9* d3d = NULL;
IDirect3DDevice9* dev = NULL;
IDirect3DVertexBuffer9* vb = NULL;
IDirect3DIndexBuffer9* ib = NULL;
void* vb_vertices;
void* ib_indexes;

vertex vertices[4];
unsigned short indexes[12];
D3DXMATRIX matWorld, matCam, matProj;
float xAngle = 0, yAngle = 0, zAngle = 0; // углы вращения вокруг каждой оси

int _stdcall WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	init(hInstance);
	while(true)
	{
			if(PeekMessage(&msg,0,0,0,PM_REMOVE))
			{
				if(msg.message == WM_QUIT)
					break;  
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			main();
	}
	shutdown();
	return 0;
}

LRESULT _stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
		return 0;
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))  // Проверка, была ли нажата одна из кнопок управления
			{
				case IDC_OX_BUTTON:
					rotate = rotateX;
				break;
				case IDC_OY_BUTTON:
					rotate = rotateY;
				break;
				case IDC_OZ_BUTTON:
					rotate = rotateZ;
				break;
				case IDC_EXIT_BUTTON:
					PostQuitMessage(0);
				break;

				case IDC_STOP_BUTTON:
					rotate = NULL;
				break;
			}
		}
		default:
			return DefWindowProc(hWnd,msg,wParam,lParam);
	}
}


// Функция векторного произведения векторов
D3DXVECTOR3 Cross(D3DXVECTOR3 v1,D3DXVECTOR3 v2)
{
	D3DXVECTOR3 v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}

void data_init()  // Инициализация данных
{
	// Инициализация координат вершин и цветов граней правильного тетраэдра
	vertices[0].v.x = 0;
	vertices[0].v.y = 0;
	vertices[0].v.z = 0;
	vertices[0].color = 0xffffffff;
	vertices[1].v.x = 1;
	vertices[1].v.y = 0;
	vertices[1].v.z = 0;
	vertices[1].color = 0xfff33ff33;
	vertices[2].v.x = 0.5;
	vertices[2].v.y = sqrt(3.0f)/2;
	vertices[2].v.z = 0;
	vertices[2].color = 0xffffff00;
	vertices[3].v.x = 0.5;
	vertices[3].v.y = sqrt(3.0f)/6;
	vertices[3].v.z = sqrt(2.0f)/sqrt(3.0f);
	vertices[3].color = 0xff3333ff; 

	// Вычисление нормалей: векторное произведение двух векторов грани и его нормализация
	D3DXVECTOR3 normal;
	normal = Cross(vertices[1].v-vertices[0].v,vertices[2].v-vertices[0].v);
	D3DXVec3Normalize( ( D3DXVECTOR3* )&vertices[0].normal, &normal);
	normal = Cross(vertices[0].v-vertices[1].v,vertices[3].v-vertices[1].v);
	D3DXVec3Normalize( ( D3DXVECTOR3* )&vertices[1].normal, &normal);
	normal = Cross(vertices[1].v-vertices[2].v,vertices[3].v-vertices[2].v);
	D3DXVec3Normalize( ( D3DXVECTOR3* )&vertices[2].normal, &normal);
	normal = Cross(vertices[0].v-vertices[3].v,vertices[2].v-vertices[3].v);
	D3DXVec3Normalize( ( D3DXVECTOR3* )&vertices[3].normal, &normal);

	// Инициализация индексов: порядок вершин в треугольниках
	indexes[0] = 0;	indexes[1] = 1;  indexes[2] = 2;
	indexes[3] = 1;	indexes[4] = 0;  indexes[5] = 3;
	indexes[6] = 2;	indexes[7] = 1;  indexes[8] = 3;
	indexes[9] = 3;	indexes[10] = 0; indexes[11] = 2;

	// Заполнение матриц преобразования: Мира, Вида, Перспективной
	D3DXMatrixIdentity(&matWorld);
	D3DXVECTOR3 EyePt( 0.3f, 0.5f,-2.0f );
    D3DXVECTOR3 LookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 UpVec( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &matCam, &EyePt, &LookatPt, &UpVec );
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/2, 1.0f, 1.0f, 1000.0f );
}

void d3d_init() // Инициализация Direct3D9
{
	// Заполнение структуры D3DPRESENT_PARAMETERS
	ZeroMemory(&pp,sizeof(pp));
	pp.BackBufferWidth = 500;
	pp.BackBufferHeight = 500;
	pp.BackBufferFormat = D3DFMT_UNKNOWN;
	pp.BackBufferCount = 1;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = hWnd;
	pp.Windowed = true;
	pp.EnableAutoDepthStencil = 1; 
	pp.AutoDepthStencilFormat = D3DFMT_D24S8;

	// Создание ресурса D3D
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	// Создание представления видеокарты в программе
	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,hWnd,
	                  D3DCREATE_HARDWARE_VERTEXPROCESSING,
	                  &pp, &dev);

	// Создание вершинного буфера
	dev->CreateVertexBuffer( 4* sizeof(vertex), D3DUSAGE_WRITEONLY,
		D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_NORMAL, D3DPOOL_DEFAULT,
	                         &vb,NULL);

	// Создание индексного буфера
	dev->CreateIndexBuffer( 12*sizeof(unsigned short),
	                        D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
	                        D3DPOOL_DEFAULT, &ib, NULL );

	// Установка матриц преобразования
	dev->SetTransform(D3DTS_WORLD, &matWorld);
	dev->SetTransform(D3DTS_VIEW, &matCam);	
	dev->SetTransform(D3DTS_PROJECTION, &matProj);

	// Заполнение вершинного и индексного буферов
	vb->Lock(0,sizeof(vertices),(void**)&vb_vertices,0);
	memcpy(vb_vertices,vertices,sizeof(vertices));
	vb->Unlock();
	ib->Lock(0,sizeof(indexes),(void**)&ib_indexes,0);
	memcpy(ib_indexes,indexes,sizeof(indexes));
	ib->Unlock();

	// Указание - какой вершинный буфер использовать
	dev->SetStreamSource(0,vb,0,sizeof(vertex));
	// Указание Flexible Vertex Format
	dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_NORMAL);
	// Указание индексного буфера
	dev->SetIndices( ib );

	// Установка состояний рендеринга
	dev->SetRenderState(D3DRS_LIGHTING, true);           // Осведещение
	dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);  // Заливка треугольников цветом
	dev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT); // Затенение треугольников (цвет по первому индексу треугольника)
	dev->SetRenderState(D3DRS_COLORVERTEX,true);         // Цвет вершин
	dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1); // Использовать цвет вершин для освещения
	dev->SetRenderState( D3DRS_ZENABLE, true);           // Буфер глубины

	// Создание направленного источника света
	D3DXVECTOR3 vecDir;
	D3DLIGHT9 light;
    ZeroMemory( &light, sizeof( D3DLIGHT9 ) );
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
	light.Position.x = 1;
	light.Position.y = 2;
	light.Position.z = -5;
    vecDir = D3DXVECTOR3( -1,-2,5);
    D3DXVec3Normalize( ( D3DXVECTOR3* )&light.Direction, &vecDir );
    light.Range = 1000.0f;
    dev->SetLight( 1, &light );
	dev->LightEnable(1, true);
}

void init(HINSTANCE hInstance)  // Функция инициализации
{
	// Заполнение структуры класса окна
	WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(6);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"window";

	RegisterClass(&wc); // Регистрация класса окна
	// Создание главного окна
	hWnd = CreateWindow(L"window",L"Вращение тетраэдра",WS_OVERLAPPEDWINDOW,100,100,500,500,NULL,NULL,hInstance,NULL);
	ShowWindow(hWnd,SW_SHOWNORMAL);
	UpdateWindow(hWnd);
	
	// Создание окна управления и 4х кнопок
	hWnd2 = CreateWindow(L"window",L"Управление",WS_DLGFRAME,610,100,175,175,NULL,NULL,hInstance,NULL);
	HWND ox = CreateWindow(L"BUTTON",L"Вращение вокруг оX",WS_VISIBLE|WS_CHILD,10,10,150,20,hWnd2,(HMENU)IDC_OX_BUTTON,GetModuleHandle(NULL),NULL);
	HWND oy = CreateWindow(L"BUTTON",L"Вращение вокруг оY",WS_VISIBLE|WS_CHILD,10,35,150,20,hWnd2,(HMENU)IDC_OY_BUTTON,GetModuleHandle(NULL),NULL);
	HWND oz = CreateWindow(L"BUTTON",L"Вращение вокруг оZ",WS_VISIBLE|WS_CHILD,10,60,150,20,hWnd2,(HMENU)IDC_OZ_BUTTON,GetModuleHandle(NULL),NULL);
	HWND stop = CreateWindow(L"BUTTON",L"Остановить вращение",WS_VISIBLE|WS_CHILD,10,85,150,20,hWnd2,(HMENU)IDC_STOP_BUTTON,GetModuleHandle(NULL),NULL);
	HWND exit = CreateWindow(L"BUTTON",L"Выход",WS_VISIBLE|WS_CHILD,10,110,150,20,hWnd2,(HMENU)IDC_EXIT_BUTTON,GetModuleHandle(NULL),NULL);
	ShowWindow(hWnd2,SW_SHOWNORMAL);
	UpdateWindow(hWnd2);

	data_init(); // Инициализация данных
	d3d_init();  // Инициализация Direct3D9
}

// Функция вывода на экран
void main()
{
	if (rotate != NULL) // Проверка указателя на функцию - содержит ли значение
		rotate();
	dev->Clear(0,NULL,D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(127,127,127),1.0f,0);
	dev->BeginScene();
	// Вывод тетраэдра
	dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,4);
	dev->EndScene();
	dev->Present(NULL,NULL,NULL,NULL);
}

// Освобождение интерфейсов в конце работы программы
void shutdown()
{
	if (ib  != NULL) ib->Release();
	if (vb  != NULL) vb->Release();
	if (dev != NULL) dev->Release();
	if (d3d != NULL) d3d->Release();
}

// Функция вращения вокруг оси X
void rotateX()
{
	xAngle += 0.05;    // скорость вращения за один кадр
	if (xAngle > 360)
		xAngle = 0;
	// Создание матрицы вращения из matWorld - матрицы мира тетраэдра
	D3DXMatrixRotationX(&matWorld, xAngle);
	dev->SetTransform(D3DTS_WORLD, &matWorld);
}

void rotateY()
{
	yAngle += 0.05;
	if (yAngle > 360)
		yAngle = 0;
	D3DXMatrixRotationY(&matWorld, yAngle);
	dev->SetTransform(D3DTS_WORLD, &matWorld);
}

void rotateZ()
{
	zAngle += 0.05;
	if (zAngle > 360)
		zAngle = 0;
	D3DXMatrixRotationZ(&matWorld, zAngle);
	dev->SetTransform(D3DTS_WORLD, &matWorld);
}