//---------------------------------------------------------------------------

#include <tchar.h>
#include <vcl.h>
#include <windows.h>    // Header file for windows
#include <math.h>	// For the Sin() function
#include <stdio.h>      // Header file for standard Input/Output
#include <gl\gl.h>      // Header file for the OpenGL32 library
#include <gl\glu.h>     // Header file for the GLu32 library
#include <gl\glaux.h>   // Header file for the GLaux library
#pragma comment (lib, "glaux.lib")
#pragma hdrstop
//---------------------------------------------------------------------------
#pragma argsused

HGLRC hRC = NULL;               // Permanent rendering context
HDC hDC = NULL;                 // Private GDI device context
HWND hWnd = NULL;               // Holds our window handle
HINSTANCE hInstance = NULL;     // Holds the instance of the application

bool keys[256];                 // Array used for the keyboard routine
bool active = true;             // Window active flag set to TRUE by default
bool fullscreen = false;         // Fullscreen flag set to fullscreen mode by default

bool light;			// Lighting ON / OFF
bool lp;			// L pressed?
bool fp;			// F pressed?
bool sp;			// Spacebar pressed?	( NEW )

int part1;			// Start of disc	( NEW )
int part2;			// End of disc		( NEW )
int p1 = 0;			// Increase 1		( NEW )
int p2 = 1;			// Increase 2		( NEW )

GLfloat	xrot;			// X rotation
GLfloat	yrot;			// Y rotation
GLfloat xspeed;			// X rotation speed
GLfloat yspeed;			// Y rotation speed
GLfloat	z = -5.0f;		// Depth into the screen

GLUquadricObj *quadratic;	// Storage for our quadratic objects ( NEW )

GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };    // Ambient light values
GLfloat LightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };    // Diffuse light values
GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };	// Light position

GLuint filter;			// Which filter to use
GLuint texture[6];		// Storage for 3 textures
GLuint object = 0;		// Which object to draw         ( NEW )

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);   // Declaration for WndProc

AUX_RGBImageRec *LoadBMP(char *Filename)        // Loads a bitmap image
{
	FILE *File = NULL;              // File handle

	if (!Filename)		        // Make sure a filename was given
	{
		return NULL;	        // If not return NULL
	}

	File = fopen(Filename,"r");	// Check to see if the file exists

	if (File)			// Does the file exist?
	{
		fclose(File);		// Close the handle
		return auxDIBImageLoad(Filename);       // Load the bitmap and return a pointer
	}

	return NULL;                    // If load failed return NULL
}

int LoadGLTextures()                                // Load Bitmaps And Convert To Textures
{
    int Status=FALSE;                           // Status Indicator

    AUX_RGBImageRec *TextureImage[2];                   // Create Storage Space For The Texture

    memset(TextureImage,0,sizeof(void *)*2);                    // Set The Pointer To NULL

    // Load The Bitmap, Check For Errors, If Bitmap's Not Found Quit
	if ((TextureImage[0]=LoadBMP("../../Data/BG.bmp")) &&             // Background Texture
		(TextureImage[1]=LoadBMP("../../Data/Reflect.bmp")))          // Reflection Texture (Spheremap)
    {
        Status=TRUE;                            // Set The Status To TRUE

        glGenTextures(6, &texture[0]);                  // Create Three Textures (For Two Images)

        for (int loop=0; loop<2; loop++)
        {
            // Create Nearest Filtered Texture
            glBindTexture(GL_TEXTURE_2D, texture[loop]);        // Gen Tex 0 And 1
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[loop]->sizeX, TextureImage[loop]->sizeY,
                0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[loop]->data);

            // Create Linear Filtered Texture
            glBindTexture(GL_TEXTURE_2D, texture[loop+2]);      // Gen Tex 2, 3 And 4
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[loop]->sizeX, TextureImage[loop]->sizeY,
                0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[loop]->data);

            // Create MipMapped Texture
            glBindTexture(GL_TEXTURE_2D, texture[loop+4]);      // Gen Tex 4 and 5
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
            gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[loop]->sizeX, TextureImage[loop]->sizeY,
                GL_RGB, GL_UNSIGNED_BYTE, TextureImage[loop]->data);
        }
        for (int loop=0; loop<2; loop++)
        {
            if (TextureImage[loop])                     // If Texture Exists
            {
                    if (TextureImage[loop]->data)            // If Texture Image Exists
                    {
                            free(TextureImage[loop]->data);  // Free The Texture Image Memory
                    }
                    free(TextureImage[loop]);       // Free The Image Structure
            }
        }
    }

    return Status;                              // Return The Status
}
GLvoid ReSizeGLScene(GLsizei width, GLsizei height)     // Resize and initialize the GL window
{
		if (height == 0)                        // Prevent A Divide By Zero By
		{
                height = 1;                     // Making height equal One
        }

        glViewport(0, 0, width, height);        // Reset the current viewport

        glMatrixMode(GL_PROJECTION);            // Select the projection matrix
	glLoadIdentity();                       // Reset the projection matrix

	// Calculate the aspect ratio of the window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);             // Select the modelview matrix
	glLoadIdentity();                       // Reset the modelview matrix
}

int InitGL(void)      // All setup for OpenGL goes here
{
        if (!LoadGLTextures())			// Jump to texture loading routine
	{
		return false;			// If texture didn't load return FALSE
	}

	glEnable(GL_TEXTURE_2D);                // Enable texture mapping

	glShadeModel(GL_SMOOTH);                // Enable smooth shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);   // Black background
	glClearDepth(1.0f);                     // Depth buffer setup
	glEnable(GL_DEPTH_TEST);                // Enables depth testing
	glDepthFunc(GL_LEQUAL);                 // The type of depth testing to do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);      // Really nice perspective calculations

        glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// Setup the ambient light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// Setup the diffuse light
	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);	// Position the light
	glEnable(GL_LIGHT1);                                    // Enable light one

		quadratic = gluNewQuadric();			        // Create a pointer to the quadric object ( NEW )
	gluQuadricNormals(quadratic, GLU_SMOOTH);       	// Create smooth normals ( NEW )
	gluQuadricTexture(quadratic, GL_TRUE);	        	// Create texture coords ( NEW )

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP); // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP); // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

	return true;                            // Initialization went OK
}

GLvoid glDrawCube()     // Draw A Cube
{
	glBegin(GL_QUADS);
        // Front Face
        glNormal3f( 0.0f, 0.0f, 0.5f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
        // Back Face
        glNormal3f( 0.0f, 0.0f,-0.5f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
        // Top Face
        glNormal3f( 0.0f, 0.5f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
        // Bottom Face
        glNormal3f( 0.0f,-0.5f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
        // Right Face
        glNormal3f( 0.5f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		// Left Face
		glNormal3f(-0.5f, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
	glEnd();
}

int DrawGLScene()                             // Here's Where We Do All The Drawing
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         // Clear The Screen And The Depth Buffer
    glLoadIdentity();                           // Reset The View

    glTranslatef(0.0f,0.0f,z);

    glEnable(GL_TEXTURE_GEN_S);                     // Enable Texture Coord Generation For S ( NEW )
    glEnable(GL_TEXTURE_GEN_T);                     // Enable Texture Coord Generation For T ( NEW )

    glBindTexture(GL_TEXTURE_2D, texture[filter+(filter+1)]);       // This Will Select A Sphere Map
    glPushMatrix();
    glRotatef(xrot,1.0f,0.0f,0.0f);
    glRotatef(yrot,0.0f,1.0f,0.0f);
    switch(object)
    {
    case 0:
        glDrawCube();
        break;
    case 1:
        glTranslatef(0.0f,0.0f,-1.5f);                  // Center The Cylinder
        gluCylinder(quadratic,1.0f,1.0f,3.0f,32,32);            // A Cylinder With A Radius Of 0.5 And A Height Of 2
        break;
    case 2:
        gluSphere(quadratic,1.3f,32,32);                // Sphere With A Radius Of 1 And 16 Longitude/Latitude Segments
        break;
    case 3:
        glTranslatef(0.0f,0.0f,-1.5f);                  // Center The Cone
        gluCylinder(quadratic,1.0f,0.0f,3.0f,32,32);            // Cone With A Bottom Radius Of .5 And Height Of 2
        break;
    };

    glPopMatrix();
    glDisable(GL_TEXTURE_GEN_S);                        // Disable Texture Coord Generation ( NEW )
    glDisable(GL_TEXTURE_GEN_T);                        // Disable Texture Coord Generation ( NEW )

    glBindTexture(GL_TEXTURE_2D, texture[filter*2]);            // This Will Select The BG Texture ( NEW )
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, -24.0f);
        glBegin(GL_QUADS);
            glNormal3f( 0.0f, 0.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-13.3f, -10.0f,  10.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 13.3f, -10.0f,  10.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 13.3f,  10.0f,  10.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-13.3f,  10.0f,  10.0f);
        glEnd();

    glPopMatrix();

    xrot+=xspeed;
    yrot+=yspeed;
    return TRUE;                                // Keep Going
}

GLvoid KillGLWindow(void)     // Properly kill the window
{
        gluDeleteQuadric(quadratic);	// Delete Quadratic - Free Resources

	if (fullscreen)         // Are we in fullscreen mode?
	{
		ChangeDisplaySettings(NULL,0);  // If so switch back to the desktop
		ShowCursor(true);               // Show mouse pointer
	}

	if (hRC)        // Do we have a rendering context?
	{
		if (!wglMakeCurrent(NULL,NULL))         // Are we able to release the DC and RC contexts?
		{
			MessageBox(NULL,"Release of DC and RC failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))             // Are we able to delete the RC?
		{
			MessageBox(NULL,"Release rendering context failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL;             // Set RC to NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))        // Are we able to release the DC
	{
		MessageBox(NULL,"Release device context failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC = NULL;             // Set DC to NULL
	}

	if (hWnd && !DestroyWindow(hWnd))       // Are we able to destroy the window?
	{
		MessageBox(NULL,"Could not release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;            // Set hWnd to NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))       // Are we able to unregister class
	{
		MessageBox(NULL,"Could not unregister class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;       // Set hInstance to NULL
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:
 *	title			- Title To Appear At The Top Of The Window
 *	width			- Width Of The GL Window Or Fullscreen Mode
 *	height			- Height Of The GL Window Or Fullscreen Mode
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)*/

BOOL CreateGLWindow(char* title, int width, int height, byte bits, bool fullscreenflag)
{
	GLuint		PixelFormat;		// Holds the results after searching for a match
	WNDCLASS	wc;		        // Windows class structure
	DWORD		dwExStyle;              // Window extended style
	DWORD		dwStyle;                // Window style
	RECT		WindowRect;             // Grabs rctangle upper left / lower right values
	WindowRect.left = (long)0;              // Set left value to 0
	WindowRect.right = (long)width;		// Set right value to requested width
	WindowRect.top = (long)0;               // Set top value to 0
	WindowRect.bottom = (long)height;       // Set bottom value to requested height

	fullscreen = fullscreenflag;              // Set the global fullscreen flag

	hInstance               = GetModuleHandle(NULL);		// Grab an instance for our window
	wc.style                = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;   // Redraw on size, and own DC for window
	wc.lpfnWndProc          = (WNDPROC) WndProc;			// WndProc handles messages
	wc.cbClsExtra           = 0;					// No extra window data
	wc.cbWndExtra           = 0;					// No extra window data
	wc.hInstance            = hInstance;				// Set the Instance
	wc.hIcon                = LoadIcon(NULL, IDI_WINLOGO);		// Load the default icon
	wc.hCursor              = LoadCursor(NULL, IDC_ARROW);		// Load the arrow pointer
	wc.hbrBackground        = NULL;					// No background required for GL
	wc.lpszMenuName		= NULL;					// We don't want a menu
	wc.lpszClassName	= "OpenGL";				// Set the class name

	if (!RegisterClass(&wc))					// Attempt to register the window class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);

		return false;   // Return FALSE
	}

	if (fullscreen)         // Attempt fullscreen mode?
	{
		DEVMODE dmScreenSettings;                                       // Device mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	        // Makes sure memory's cleared
		dmScreenSettings.dmSize         = sizeof(dmScreenSettings);     // Size of the devmode structure
		dmScreenSettings.dmPelsWidth	= width;                        // Selected screen width
		dmScreenSettings.dmPelsHeight	= height;                       // Selected screen height
		dmScreenSettings.dmBitsPerPel	= bits;	                        // Selected bits per pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try to set selected mode and get results. NOTE: CDS_FULLSCREEN gets rid of start bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If the mode fails, offer two options. Quit or use windowed mode.
			if (MessageBox(NULL,"The requested fullscreen mode is not supported by\nyour video card. Use windowed mode instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen = false;       // Windowed mode selected. Fullscreen = FALSE
			}
			else
			{
				// Pop up a message box letting user know the program is closing.
				MessageBox(NULL,"Program will now close.","ERROR",MB_OK|MB_ICONSTOP);
				return false;           // Return FALSE
			}
		}
	}

	if (fullscreen)                         // Are We Still In Fullscreen Mode?
	{
		dwExStyle = WS_EX_APPWINDOW;    // Window extended style
		dwStyle = WS_POPUP;		// Windows style
		ShowCursor(false);		// Hide mouse pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;           // Window extended style
		dwStyle=WS_OVERLAPPEDWINDOW;                            // Windows style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);     // Adjust window to true requested size

	// Create the window
	if (!(hWnd = CreateWindowEx(dwExStyle,          // Extended Style For The Window
                "OpenGL",				// Class name
		title,					// Window title
		dwStyle |				// Defined window style
		WS_CLIPSIBLINGS |			// Required window style
		WS_CLIPCHILDREN,			// Required window style
		0, 0,					// Window position
		WindowRect.right-WindowRect.left,	// Calculate window width
		WindowRect.bottom-WindowRect.top,	// Calculate window height
		NULL,					// No parent window
		NULL,					// No menu
		hInstance,				// Instance
		NULL)))					// Dont pass anything to WM_CREATE
	{
		KillGLWindow();                         // Reset the display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;                           // Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd =             // pfd tells windows how we want things to be
	{
		sizeof(PIXELFORMATDESCRIPTOR),          // Size of this pixel format descriptor
		1,					// Version number
		PFD_DRAW_TO_WINDOW |			// Format must support window
		PFD_SUPPORT_OPENGL |			// Format must support OpenGL
		PFD_DOUBLEBUFFER,			// Must support double buffering
		PFD_TYPE_RGBA,				// Request an RGBA format
		bits,					// Select our color depth
		0, 0, 0, 0, 0, 0,			// Color bits ignored
		0,					// No alpha buffer
		0,					// Shift bit ignored
		0,					// No accumulation buffer
		0, 0, 0, 0,				// Accumulation bits ignored
		16,					// 16Bit Z-Buffer (Depth buffer)
		0,					// No stencil buffer
		0,					// No auxiliary buffer
		PFD_MAIN_PLANE,				// Main drawing layer
		0,					// Reserved
		0, 0, 0					// Layer masks ignored
	};

	if (!(hDC=GetDC(hWnd)))         // Did we get a device context?
	{
		KillGLWindow();         // Reset the display
		MessageBox(NULL,"Can't create a GL device context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;           // Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did windows find a matching pixel format?
	{
		KillGLWindow();         // Reset the display
		MessageBox(NULL,"Can't find a suitable pixelformat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;           // Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))       // Are we able to set the pixel format?
	{
		KillGLWindow();         // Reset the display
		MessageBox(NULL,"Can't set the pixelformat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;           // Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))               // Are we able to get a rendering context?
	{
		KillGLWindow();         // Reset the display
		MessageBox(NULL,"Can't create a GL rendering context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;           // Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))    // Try to activate the rendering context
	{
		KillGLWindow();         // Reset the display
		MessageBox(NULL,"Can't activate the GL rendering context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;           // Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);       // Show the window
	SetForegroundWindow(hWnd);      // Slightly higher priority
	SetFocus(hWnd);                 // Sets keyboard focus to the window
	ReSizeGLScene(width, height);   // Set up our perspective GL screen

	if (!InitGL())                  // Initialize our newly created GL window
	{
		KillGLWindow();         // Reset the display
		MessageBox(NULL,"Initialization failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;           // Return FALSE
	}

	return true;                    // Success
}

LRESULT CALLBACK WndProc(HWND hWnd,     // Handle for this window
                        UINT uMsg,      // Message for this window
			WPARAM wParam,  // Additional message information
			LPARAM lParam)  // Additional message information
{
	switch (uMsg)                           // Check for windows messages
	{
		case WM_ACTIVATE:               // Watch for window activate message
		{
			if (!HIWORD(wParam))    // Check minimization state
			{
				active = true;  // Program is active
			}
			else
			{
				active = false; // Program is no longer active
			}

			return 0;               // Return to the message loop
		}

		case WM_SYSCOMMAND:             // Intercept system commands
		{
			switch (wParam)         // Check system calls
			{
				case SC_SCREENSAVE:     // Screensaver trying to start?
				case SC_MONITORPOWER:	// Monitor trying to enter powersave?
				return 0;       // Prevent from happening
			}
			break;                  // Exit
		}

		case WM_CLOSE:                  // Did we receive a close message?
		{
			PostQuitMessage(0);     // Send a quit message
			return 0;               // Jump back
		}

		case WM_KEYDOWN:                // Is a key being held down?
		{
			keys[wParam] = true;    // If so, mark it as TRUE
			return 0;               // Jump back
		}

		case WM_KEYUP:                  // Has a key been released?
		{
			keys[wParam] = false;   // If so, mark it as FALSE
			return 0;               // Jump back
		}

		case WM_SIZE:                   // Resize the OpenGL window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord = Width, HiWord = Height
			return 0;               // Jump back
		}
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
        MSG msg;                // Windows message structure
	bool done = false;      // Bool variable to exit loop

	// Ask the user which screen mode they prefer
	///if (MessageBox(NULL,"Would you like to run in fullscreen mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
		fullscreen = false;       // Windowed mode
	}

	// Create our OpenGL window
	if (!CreateGLWindow("NeHe & TipTup's Quadratics Tutorial",640,480,16,fullscreen))
	{
		return 0;               // Quit if window was not created
	}

	while(!done)            // Loop that runs while done = FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is there a message waiting?
		{
			if (msg.message == WM_QUIT)             // Have we received a quit message?
			{
				done = true;                    // If so done = TRUE
			}
			else                                    // If not, deal with window messages
			{
				TranslateMessage(&msg);         // Translate the message
				DispatchMessage(&msg);          // Dispatch the message
			}
		}
		else            // If there are no messages
		{
			// Draw the scene. Watch for ESC key and quit messages from DrawGLScene()
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE])	// Active?  Was there a quit received?
			{
				done = true;            // ESC or DrawGLScene signalled a quit
			}
			else                            // Not time to quit, update screen
			{
				SwapBuffers(hDC);       // Swap buffers (double buffering)

				if (keys['L'] && !lp)
				{
					lp = true;
					light = !light;
					if (!light)
					{
						glDisable(GL_LIGHTING);
					}
					else
					{
						glEnable(GL_LIGHTING);
					}
				}
				if (!keys['L'])
				{
					lp=FALSE;
				}
				if (keys['F'] && !fp)
				{
					fp = true;
					filter += 1;
					if (filter > 2)
					{
						filter = 0;
					}
				}
				if (!keys['F'])
				{
					fp = false;
				}
				if (keys[' '] && !sp)
				{
					sp=TRUE;
					object++;
					if(object>3)
						object=0;
				}
				if (!keys[' '])
				{
					sp=FALSE;
				}
				if (keys[VK_PRIOR])
				{
					z -= 0.02f;
				}
				if (keys[VK_NEXT])
				{
					z += 0.02f;
				}
				if (keys[VK_UP])
				{
					xspeed -= 0.01f;
				}
				if (keys[VK_DOWN])
				{
					xspeed += 0.01f;
				}
				if (keys[VK_RIGHT])
				{
					yspeed += 0.01f;
				}
				if (keys[VK_LEFT])
				{
					yspeed -= 0.01f;
				}

                                if (keys[' '] && !sp)		// Is spacebar being pressed?
				{
					sp = true;		// If so, set sp to TRUE
					object++;		// Cycle through the objects
					if (object > 5)		// Is object greater than 5?
						object = 0;	// If so, set to zero
				}
				if (!keys[' '])			// Has the spacebar been released?
				{
					sp = false;		// If so, set sp to FALSE
				}

				if (keys[VK_F1])                // Is F1 being pressed?
				{
					keys[VK_F1] = false;    // If so make key FALSE
					KillGLWindow();	        // Kill our current window
					fullscreen=!fullscreen; // Toggle fullscreen / windowed mode
					// Recreate our OpenGL window
					if (!CreateGLWindow("NeHe & TipTup's Quadratics Tutorial",640,480,16,fullscreen))
					{
						return 0;       // Quit if window was not created
					}
				}
			}
		}
	}

	// Shutdown
    gluDeleteQuadric(quadratic);
    KillGLWindow();                             // Kill The Window
    return (msg.wParam);                            // Exit The Program
}
//---------------------------------------------------------------------------
