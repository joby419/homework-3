// ayomide johnson
// cs335
// Duck Hunt
// my github:
// added sound to jasons work from github: https://github.com/Husky3388/duckhunt

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "ppm.h"
#include <stdio.h>
#include <unistd.h> //for sleep function

//800, 600
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define MAX_DUCKS 2

#define rnd() (float)rand() / (float)RAND_MAX

extern "C"
{
#include "fonts.h"
}

#define USE_SOUND

#ifdef USE_SOUND
#include <FMOD/fmod.h>
#include <FMOD/wincompat.h>
#include "fmod.h"
#endif //USE_SOUND

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

struct timespec timeStart, timeCurrent;
struct timespec timePause;
double movementCountdown = 0.0;
double timeSpan = 0.0;
const double oobillion = 1.0 / 1e9;
const double movementRate = 1.0 / 60.0;
double timeDiff(struct timespec *start, struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) * oobillion;
}
void timeCopy(struct timespec *dest, struct timespec *source)
{
    memcpy(dest, source, sizeof(struct timespec));
}

//Structures

struct Vec {
    float x, y, z;
};

struct Shape {
    float width, height, radius;
    Vec center;
    struct timespec time;
};

struct Duck
{
    Shape s;
    Vec velocity;
    struct timespec time;
    struct Duck *prev;
    struct Duck *next;
    Duck()
    {
	prev = NULL;
	next = NULL;
    }
};

struct deadDuck
{
    Shape s;
    Vec velocity;
    struct timespec time;
    struct deadDuck *prev;
    struct deadDuck *next;
    deadDuck()
    {
	prev = NULL;
	next = NULL;
    }
};

struct freeDuck
{
    Shape s;
    Vec velocity;
    //struct timespec time;
    struct freeDuck *prev;
    struct freeDuck *next;
    freeDuck()
    {
	prev = NULL;
	next = NULL;
    }
};

struct Dog
{
    Shape s;
    Vec velocity;
    struct timespec time;
    struct Dog *prev;
    struct Dog *next;
    Dog()
    {
	prev = NULL;
	next = NULL;
    }
};

struct Game {
    int bullets, n, rounds, score, duckCount, duckShot;
    Duck *duck;
    deadDuck *deadD;
    freeDuck *freeD;
    Dog *dog;
    float floor;
    struct timespec duckTimer, dogTimer;
    Shape box[6];
    bool oneDuck, twoDuck, animateDog;
    ~Game()
    {
	delete [] duck;
	delete [] deadD;
	delete [] freeD;
	delete [] dog;
    }
    Game()
    {
	duck = NULL;
	deadD = NULL;
	freeD = NULL;
	dog = NULL;
	bullets = 0;
	n = 0;
	floor = WINDOW_HEIGHT / 5.0;
	rounds = 1;
	score = 0;
	duckCount = 0;
	duckShot = 0;
	oneDuck = false;
	twoDuck = false;
	animateDog = false;

	//bullet
	box[0].width = 45;
	box[0].height = 35;
	box[0].center.x = (WINDOW_WIDTH / 10) - (WINDOW_WIDTH / 20);
	box[0].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.1);
	box[0].center.z = 0;

	//count
	box[1].width = 100;
	box[1].height = 35;
	box[1].center.x = WINDOW_WIDTH / 4;
	box[1].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.1);
	box[1].center.z = 0;

	//score
	box[2].width = 45;
	box[2].height = 35;
	box[2].center.x = (WINDOW_WIDTH / 2) + (WINDOW_WIDTH / 4);
	box[2].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.1);
	box[2].center.z = 0;

	//round
	box[3].width = 45;
	box[3].height = 35;
	box[3].center.x = (WINDOW_WIDTH / 10) - (WINDOW_WIDTH / 70);
	box[3].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.5);
	box[3].center.z = 0;

	//score on shot
	box[4].width = 45;
	box[4].height = 35;
	box[4].center.x = 0;
	box[4].center.y = 0;
	box[4].center.z = 0;
    }
};

//Function prototypes
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e, Game *game);
int check_keys(XEvent *e, Game *game);
void movement(Game *game);
void render(Game *game);
void makeDuck(Game *game);
void makeDeadDuck(Game *game);
void makeFreeDuck(Game *game);
void makeDog(Game *game);
void deleteDuck(Game *game, Duck *duck);
void deleteDeadDuck(Game *game, deadDuck *deadD);
void deleteFreeDuck(Game *game, freeDuck *freeD);
void deleteDog(Game *game, Dog *dog);
void check_resize(XEvent *e);
void init_sounds(void);

Ppmimage *backgroundImage = NULL;
GLuint backgroundTexture;
int background = 1;

int main(void)
{
    int done=0;
    srand(time(NULL));
    initXWindows();
    init_opengl();
    init_sounds();

    clock_gettime(CLOCK_REALTIME, &timePause);
    clock_gettime(CLOCK_REALTIME, &timeStart);
    //declare game object
    Game game;

    //start animation
    while(!done) {
	while(XPending(dpy)) {
	    XEvent e;
	    XNextEvent(dpy, &e);
	    check_resize(&e);
	    check_mouse(&e, &game);
	    done = check_keys(&e, &game);
	}
	clock_gettime(CLOCK_REALTIME, &timeCurrent);
	timeSpan = timeDiff(&timeStart, &timeCurrent);
	timeCopy(&timeStart, &timeCurrent);
	movementCountdown += timeSpan;
	while(movementCountdown >= movementRate)
	{
	    movement(&game);
	    movementCountdown -= movementRate;
	}
	render(&game);
	glXSwapBuffers(dpy, win);
    }
    cleanupXWindows();
    cleanup_fonts();
    #ifdef USE_SOUND
    fmod_cleanup();
    #endif //USE_SOUND
    return 0;

}

void set_title(void)
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "Duck Hunt");
}

void cleanupXWindows(void) {
    //do not change
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void initXWindows(void) {
    //do not change
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w=WINDOW_WIDTH, h=WINDOW_HEIGHT;

    XSetWindowAttributes swa;

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
	std::cout << "\n\tcannot connect to X server\n" << std::endl;
	exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if(vi == NULL) {
	std::cout << "\n\tno appropriate visual found\n" << std::endl;
	exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    //XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	ButtonPress | ButtonReleaseMask |
	PointerMotionMask |
	StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
	    InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void reshape_window(int width, int height)
{
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    set_title();
}

unsigned char *buildAlphaData(Ppmimage *img) {
    // add 4th component to RGB stream...
    int a,b,c;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    //newdata = (unsigned char *)malloc(img->width * img->height * 4);
    newdata = new unsigned char[img->width * img->height * 4];
    ptr = newdata;
    for (int i=0; i<img->width * img->height * 3; i+=3) {
	a = *(data+0);
	b = *(data+1);
	c = *(data+2);
	*(ptr+0) = a;
	*(ptr+1) = b;
	*(ptr+2) = c;
	//
	//get the alpha value
	//
	//original code
	//get largest color component...
	//*(ptr+3) = (unsigned char)((
	//      (int)*(ptr+0) +
	//      (int)*(ptr+1) +
	//      (int)*(ptr+2)) / 3);
	//d = a;
	//if (b >= a && b >= c) d = b;
	//if (c >= a && c >= b) d = c;
	//*(ptr+3) = d;
	//
	//new code, suggested by Chris Smith, Fall 2013
	*(ptr+3) = (a|b|c);
	//
	ptr += 4;
	data += 3;
    }
    return newdata;
}

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);

    //added for background
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    //clear the screen
    glClearColor(1.0, 1.0, 1.0, 1.0);
    backgroundImage = ppm6GetImage("./images/background.ppm");
    //
    //create opengl texture elements
    glGenTextures(1, &backgroundTexture);
    //background
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    //
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, backgroundImage->width, backgroundImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, backgroundImage->data); 
    //Set the screen background color
    //glClearColor(0.1, 0.1, 0.1, 1.0);
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
}

void makeDuck(Game *game, float x, float y)
{
    if(game->n >= MAX_DUCKS)
	return;
    struct timespec dt;
    clock_gettime(CLOCK_REALTIME, &dt);
    timeCopy(&game->duckTimer, &dt);
    int directionNum = rand() % 101;
    Duck *d;
    try
    {
	d = new Duck;
    }
    catch(std::bad_alloc)
    {
	return;
    }
    timeCopy(&d->time, &dt);
    d->s.center.x = x;
    d->s.center.y = y;
    d->s.center.z = 0.0;
    if(directionNum >= 50)
	d->velocity.x = 4.0 * (game->rounds * .5);
    else
	d->velocity.x = -4.0 * (game->rounds * .5);
    d->velocity.y = 4.0 * (game->rounds * .5);
    d->velocity.z = 0.0;
    d->s.width = 50.0;
    d->s.height = 50.0;
    d->next = game->duck;
    if(game->duck != NULL)
    {
	game->duck->prev = d;
    }
    game->duck = d;
    game->n++;
}

void makeDeadDuck(Game *game, Duck *duck)
{
    //if(game->n >= MAX_DUCKS)
    //	return;
    struct timespec dt;
    clock_gettime(CLOCK_REALTIME, &dt);
    timeCopy(&game->duckTimer, &dt);
    deadDuck *dd;
    try
    {
	dd = new deadDuck;
    }
    catch(std::bad_alloc)
    {
	return;
    }
    timeCopy(&dd->time, &dt);
    dd->s.center.x = duck->s.center.x;
    dd->s.center.y = duck->s.center.y;
    dd->s.center.z = 0.0;
    dd->velocity.x = 0.0;
    //dd->velocity.y = -3.5;
    dd->velocity.z = 0.0;
    dd->s.width = 50.0;
    dd->s.height = 50.0;
    dd->next = game->deadD;
    if(game->deadD != NULL)
    {
	game->deadD->prev = dd;
    }
    game->deadD = dd;
}

void makeFreeDuck(Game *game, Duck *duck)
{
    //if(game->n >= MAX_DUCKS)
    //	return;
    //struct timespec dt;
    //clock_gettime(CLOCK_REALTIME, &dt);
    //timeCopy(&game->duckTimer, &dt);
    freeDuck *fd;
    try
    {
	fd = new freeDuck;
    }
    catch(std::bad_alloc)
    {
	return;
    }
    //timeCopy(&dd->time, &dt);
    fd->s.center.x = duck->s.center.x;
    fd->s.center.y = duck->s.center.y;
    fd->s.center.z = 0.0;
    fd->velocity.x = duck->velocity.x;
    fd->velocity.y = duck->velocity.y;
    fd->velocity.z = 0.0;
    fd->s.width = 50.0;
    fd->s.height = 50.0;
    fd->next = game->freeD;
    if(game->freeD != NULL)
    {
	game->freeD->prev = fd;
    }
    game->freeD = fd;
}


void makeDog(Game *game, float x, float y)
{
    //if(game->n >= MAX_DUCKS)
    //	return;
    struct timespec dt;
    clock_gettime(CLOCK_REALTIME, &dt);
    timeCopy(&game->dogTimer, &dt);
    //std::cout << "makeDog() " << x << " " << y << std::endl;
    Dog *doge;
    try
    {
	doge = new Dog;
    }
    catch(std::bad_alloc)
    {
	return;
    }
    timeCopy(&doge->time, &dt);
    doge->s.center.x = x;
    doge->s.center.y = y;
    doge->s.center.z = 0.0;
    doge->velocity.x = 1.0;
    doge->velocity.y = 1.0;
    doge->velocity.z = 0.0;
    doge->s.width = 50.0;
    doge->s.height = 50.0;
    doge->next = game->dog;
    if(game->dog != NULL)
    {
	game->dog->prev = doge;
    }
    game->dog = doge;
}

void check_resize(XEvent *e)
{
    if(e->type != ConfigureNotify)
	return;
    XConfigureEvent xce = e->xconfigure;
    if(xce.width != WINDOW_WIDTH || xce.height != WINDOW_HEIGHT)
    {
	reshape_window(xce.width, xce.height);
    }
}

void check_mouse(XEvent *e, Game *game)
{
    int y = WINDOW_HEIGHT - e->xbutton.y;

    Duck *d = game->duck;
    Duck *saved;
    struct timespec dt;
    clock_gettime(CLOCK_REALTIME, &dt);
    double ts;

    if (e->type == ButtonRelease) {
	return;
    }
    if (e->type == ButtonPress) {
	if (e->xbutton.button==1) {
        #ifdef USE_SOUND
        fmod_playsound(0);
	    //Left button was pressed
	    while(d)
	    {	
		if(e->xbutton.x >= d->s.center.x - d->s.width &&
			e->xbutton.x <= d->s.center.x + d->s.width &&
			y <= d->s.center.y + d->s.height &&
			y >= d->s.center.y - d->s.height)
		{
		    fmod_playsound(1);
		    ts = timeDiff(&d->time, &dt);
		    if(ts < 1.5)
			game->score += 200;
		    else
			game->score += 100;
		    makeDeadDuck(game, d);
		    saved = d->next;
		    deleteDuck(game, d);
		    d = saved;
		    game->bullets--;	
		    game->duckShot++;
		    if(game->bullets < 1)
		    {
			if(game->n == 1)
			{
			    makeFreeDuck(game, d);
			    saved = d->next;
			    deleteDuck(game, d);
			    d = saved;
			}
			return;
		    }
		    return;
		}
		if(game->n == 2)
		{
		    d = d->next;
		    if(e->xbutton.x >= d->s.center.x - d->s.width &&
			    e->xbutton.x <= d->s.center.x + d->s.width &&
			    y <= d->s.center.y + d->s.height &&
			    y >= d->s.center.y - d->s.height)
		    {   
			fmod_playsound(1);
			ts = timeDiff(&d->time, &dt);
			if(ts < 1.5)
			    game->score += 200;
			else
			    game->score += 100;
			makeDeadDuck(game, d);
			saved = d->prev;
			deleteDuck(game, d);
			d = saved;
			game->bullets--;
			game->duckShot++;
			if(game->bullets < 1)
			{
			    if(game->n == 1)
			    {
				makeFreeDuck(game, d);
				saved = d->next;
				deleteDuck(game, d);
				d = saved;
			    }
			    return;
			}
			return;
		    }
		}
		if(game->bullets <= 1)
		{
		    if(game->n == 2)
		    {
			d = d->prev;
		    }
		    makeFreeDuck(game, d);
		    saved = d->next;
		    deleteDuck(game, d);
		    d = saved;
		    if(game->n == 1)
		    {
			makeFreeDuck(game, d);
			saved = d->next;
			deleteDuck(game, d);
			d = saved;
		    }
		    return;
		}
		game->bullets--;	
		d = d->next;
	    }
	}
    
    #endif
    }
    if (e->xbutton.button==3) {
	//Right button was pressed
	return;
    }
}

int check_keys(XEvent *e, Game *game)
{
    Duck *d = game->duck;
    Dog *doge = game->dog;
    //Was there input from the keyboard?
    if (e->type == KeyPress) {
	int key = XLookupKeysym(&e->xkey, 0);
	if (key == XK_Escape) {
	    return 1;
	}
	//You may check other keys here.
	if(key == XK_1)
	{
	    while(d)
	    {
		deleteDuck(game, d);
		d = d->next;
	    }
	    while(doge)
	    {
		deleteDog(game, doge);
		doge = doge->next;
	    }
	    game->rounds = 1;
	    game->duckCount = 0;
	    game->duckShot = 0;
	    game->bullets = 3;
	    game->score = 0;
	    if(!game->oneDuck)
		game->oneDuck = true;
	    else
		game->oneDuck = false;
	    game->twoDuck = false;
	    if(!game->animateDog)
		game->animateDog = true;
	    else
		game->animateDog = false;
	}
	if(key == XK_2)
	{
	    while(d)
	    {
		deleteDuck(game, d);
		d = d->next;
	    }	
	    while(doge)
	    {
		deleteDog(game, doge);
		doge = doge->next;
	    }
	    game->rounds = 1;
	    game->duckCount = 0;
	    game->duckShot = 0;
	    game->bullets = 3;
	    game->score = 0;
	    if(!game->twoDuck)
		game->twoDuck = true;
	    else
		game->twoDuck = false;
	    game->oneDuck = false;
	    if(!game->animateDog)
		game->animateDog = true;
	    else
		game->animateDog = false;
	}
    }
    return 0;
}

void movement(Game *game)
{
    Duck *d = game->duck;
    deadDuck *dd = game->deadD;
    freeDuck *fd = game->freeD;
    Dog *doge = game->dog;
    struct timespec dt;
    clock_gettime(CLOCK_REALTIME, &dt);
    int randDirectionNumX, randDirectionNumY;

    if (game->n <= 0)
	return;

    while(d)
    {
	double ts = timeDiff(&d->time, &dt);
	randDirectionNumX = rand() % 101;
	randDirectionNumY = rand() % 101;
	if(ts > 5.0)
	{
	    Duck *saved = d->next;
	    deleteDuck(game, d);
	    d = saved;
	    continue;
	}
	if(1.0 < ts && ts < 1.01)
	{
	    if(randDirectionNumX >= 50)
		d->velocity.x *= -1;
	    if(randDirectionNumY >= 50)
		d->velocity.y *= -1;
	}
	if(2.0 < ts && ts < 2.01)
	{
	    if(randDirectionNumX >= 50)
		d->velocity.x *= -1;
	    if(randDirectionNumY >= 50)
		d->velocity.y *= -1;
	}
	if(3.0 < ts && ts < 3.01)
	{
	    if(randDirectionNumX >= 50)
		d->velocity.x *= -1;
	    if(randDirectionNumY >= 50)
		d->velocity.y *= -1;
	}
	if(4.0 < ts && ts < 4.01)
	{
	    if(randDirectionNumX >= 50)
		d->velocity.x *= -1;
	    if(randDirectionNumY >= 50)
		d->velocity.y *= -1;
	}

	if(d->s.center.x - d->s.width <= 0.0)
	{
	    d->s.center.x = d->s.width + 0.1;
	    d->velocity.x *= -1.0;
	}
	if(d->s.center.x + d->s.width >= WINDOW_WIDTH)
	{
	    d->s.center.x = WINDOW_WIDTH - d->s.width - 0.1;
	    d->velocity.x *= -1.0;
	}
	if(d->s.center.y - d->s.height <= game->floor)
	{
	    d->s.center.y = game->floor + d->s.height + 0.1;
	    d->velocity.y *= -1.0;
	}
	if(d->s.center.y + d->s.height >= WINDOW_HEIGHT)
	{
	    d->s.center.y = WINDOW_HEIGHT - d->s.height - 0.1;
	    d->velocity.y *= -1.0;
	}

	d->s.center.x += d->velocity.x;
	d->s.center.y += d->velocity.y;

	d = d->next;	
    }

    while(dd)
    {
	double ts = timeDiff(&dd->time, &dt);
	float velocity = -3.5;
	if(ts < 0.3)
	    dd->velocity.y = 0.0;
	if(ts > 0.3)
	    dd->velocity.y = velocity;
	if(dd->s.center.y - dd->s.height <= game->floor)
	    deleteDeadDuck(game, dd);
	dd->s.center.y += dd->velocity.y;
	dd = dd->next;
    }

    while(fd)
    {
	if(fd->s.center.x + fd->s.width <= 0.0)
	{
	    deleteFreeDuck(game, fd);
	}
	if(fd->s.center.x - fd->s.width >= WINDOW_WIDTH)
	{
	    deleteFreeDuck(game, fd);
	}
	if(fd->velocity.y < 0.0)
	    fd->velocity.y *= -1.0;
	if(fd->s.center.y - fd->s.height >= WINDOW_HEIGHT)
	{
	    deleteFreeDuck(game, fd);
	}

	fd->s.center.x += fd->velocity.x;
	fd->s.center.y += fd->velocity.y;

	fd = fd->next;
    }

    while(doge)
    {
	double ts = timeDiff(&doge->time, &dt);
	if(ts > 5.0)
	{
	    Dog *saved = doge->next;
	    deleteDog(game, doge);
	    doge = saved;
	    continue;
	}
	if(1.0 < ts && ts < 1.01)
	    doge->velocity.x = 0.0;
	if(2.0 < ts && ts < 2.01)
	    doge->velocity.x = 1.0;
	if(3.0 < ts && ts < 3.01)
	    doge->velocity.x = 0.0;
	if(4.0 < ts && ts < 4.01)
	    doge->velocity.x = 1.0;
	doge->s.center.x += doge->velocity.x;
	doge->s.center.y += doge->velocity.y;

	doge = doge->next;
    }
}

void render(Game *game)
{
    float w, h, x, y;
    Duck *d = game->duck;
    deadDuck *dd = game->deadD;
    freeDuck *fd = game->freeD;
    //Dog *doge = game->dog;

    glColor3ub(255, 255, 255);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if(background) {
	glBindTexture(GL_TEXTURE_2D, backgroundTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
	glTexCoord2f(0.0f, 0.0f); glVertex2i(0, WINDOW_HEIGHT);
	glTexCoord2f(1.0f, 0.0f); glVertex2i(WINDOW_WIDTH, WINDOW_HEIGHT);
	glTexCoord2f(1.0f, 1.0f); glVertex2i(WINDOW_WIDTH, 0);
	glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    //Drawing Shapes
    glColor3ub(255, 255, 255);
    glBegin(GL_LINES);
    glVertex2f(0.0, game->floor);
    glVertex2f(WINDOW_WIDTH, game->floor);
    glEnd();

    //GERARDO
    //Printing text in Boxes
    Rect r;
    //  glClear(GL_COLOR_BUFFER_BIT);
    r.bot = WINDOW_HEIGHT - 550;
    r.left = WINDOW_WIDTH - 715;
    r.center = 0;

    //Drawing Boxes
    Shape *s;
    glColor3ub(90, 140, 90);
    s = &game->box[0];
    glPushMatrix();
    glTranslatef(s->center.x, s->center.y, s->center.z);
    w = s->width;
    h = s->height;
    r.bot = s->height;
    r.left = s->width;
    glVertex2i(-w, -h);
    glVertex2i(-w, h);
    glVertex2i(w, h);
    glVertex2i(w, -h);
    glEnd();
    ggprint16(&r , 16, 0x00ffffff, "%i", game->bullets);
    glPopMatrix();

    glColor3ub(90, 140, 90);
    s = &game->box[1];
    glPushMatrix();
    glTranslatef(s->center.x, s->center.y, s->center.z);
    w = s->width;
    h = s->height;
    r.bot = s->height;
    r.left = s->width;
    glVertex2i(-w, -h);
    glVertex2i(-w, h);
    glVertex2i(w, h);
    glVertex2i(w, -h);
    glEnd();
    if(!d && game->duckCount >= 10 && game->duckShot < 6)
    {
	ggprint16(&r , 16, 0x00ffffff, "GAME OVER");
    }
    ggprint16(&r , 16, 0x00ffffff, "%i / 10", game->duckShot);
    glPopMatrix();

    glColor3ub(90, 140, 90);
    s = &game->box[2];
    glPushMatrix();
    glTranslatef(s->center.x, s->center.y, s->center.z);
    w = s->width;
    h = s->height;
    r.bot = s->height;
    r.left = s->width;
    glVertex2i(-w, -h);
    glVertex2i(-w, h);
    glVertex2i(w, h);
    glVertex2i(w, -h);
    glEnd();
    ggprint16(&r , 16, 0x00ffffff, "%i", game->score);
    glPopMatrix();

    glColor3ub(90, 140, 90);
    s = &game->box[3];
    glPushMatrix();
    glTranslatef(s->center.x, s->center.y, s->center.z);
    w = s->width;
    h = s->height;
    r.bot = s->height;
    r.left = s->width;
    glVertex2i(-w, -h);
    glVertex2i(-w, h);
    glVertex2i(w, h);
    glVertex2i(w, -h);
    glEnd();
    ggprint16(&r , 16, 0x00ffffff, "%i", game->rounds);
    glPopMatrix();
    
    if((game->oneDuck || game->twoDuck))
    {
	if(!d && game->duckCount >= 10 && game->duckShot >= 6)
	{
	    game->rounds++;
	    game->duckCount = 0;
	    game->duckShot = 0;
	}
	if(!d && game->duckCount >= 10 && game->duckShot < 6)
	{
	    while(d)
	    {
		deleteDuck(game, d);
		d = d->next;
	    }
	    game->oneDuck = false;
	    game->twoDuck = false;
	    std::cout << "GAME OVER" << std::endl;
	}
	if(!d && game->oneDuck && game->duckCount < 10)
	{
	    game->bullets = 3;
	    makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
	    game->duckCount++;
	}
	if(!d && game->twoDuck && game->duckCount < 9)
	{
	    game->bullets = 3;
	    makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
	    makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
	    game->duckCount += 2;
	}
    }

    glColor3ub(255, 255, 255);
    while(d)
    {
	w = d->s.width;
	h = d->s.height;
	x = d->s.center.x;
	y = d->s.center.y;
	glBegin(GL_QUADS);
	glVertex2f(x-w, y+h);
	glVertex2f(x-w, y-h);
	glVertex2f(x+w, y-h);
	glVertex2f(x+w, y+h);
	glEnd();
	d = d->next;
    }

    glColor3ub(255, 0, 0);
    while(dd)
    {
	w = dd->s.width;
	h = dd->s.height;
	x = dd->s.center.x;
	y = dd->s.center.y;
	glBegin(GL_QUADS);
	glVertex2f(x-w, y+h);
	glVertex2f(x-w, y-h);
	glVertex2f(x+w, y-h);
	glVertex2f(x+w, y+h);
	glEnd();
	dd = dd->next;
    }
    
    glColor3ub(0, 0, 255);
    while(fd)
    {
	w = fd->s.width;
	h = fd->s.height;
	x = fd->s.center.x;
	y = fd->s.center.y;
	glBegin(GL_QUADS);
	glVertex2f(x-w, y+h);
	glVertex2f(x-w, y-h);
	glVertex2f(x+w, y-h);
	glVertex2f(x+w, y+h);
	glEnd();
	fd = fd->next;
    }

    /*if(game->animateDog)
      {
      makeDog(game, 51, game->floor + 51);
      }
      glColor3ub(255, 255, 255);
      while(doge)
      {
      w = doge->s.width;
      h = doge->s.height;
      x = doge->s.center.x;
      y = doge->s.center.y;
      glBegin(GL_QUADS);
      glVertex2f(x-w, y+h);
      glVertex2f(x-w, y-h);
      glVertex2f(x+w, y-h);
      glVertex2f(x+w, y+h);
      glEnd();
      doge = doge->next;
      }*/

}

void deleteDuck(Game *game, Duck *node)
{
    if(node->prev == NULL)
    {
	if(node->next == NULL)
	{
	    game->duck = NULL;
	}
	else
	{
	    node->next->prev = NULL;
	    game->duck = node->next;
	}
    }
    else
    {
	if(node->next == NULL)
	{
	    node->prev->next = NULL;
	}
	else
	{
	    node->prev->next = node->next;
	    node->next->prev = node->prev;
	}
    }
    delete node;
    node = NULL;
    game->n--;
}

void deleteDeadDuck(Game *game, deadDuck *node)
{
    if(node->prev == NULL)
    {
	if(node->next == NULL)
	{
	    game->deadD = NULL;
	}
	else
	{
	    node->next->prev = NULL;
	    game->deadD = node->next;
	}
    }
    else
    {
	if(node->next == NULL)
	{
	    node->prev->next = NULL;
	}
	else
	{
	    node->prev->next = node->next;
	    node->next->prev = node->prev;
	}
    }
    delete node;
    node = NULL;
}

void deleteFreeDuck(Game *game, freeDuck *node)
{
    if(node->prev == NULL)
    {
	if(node->next == NULL)
	{
	    game->freeD = NULL;
	}
	else
	{
	    node->next->prev = NULL;
	    game->freeD = node->next;
	}
    }
    else
    {
	if(node->next == NULL)
	{
	    node->prev->next = NULL;
	}
	else
	{
	    node->prev->next = node->next;
	    node->next->prev = node->prev;
	}
    }
    delete node;
    node = NULL;
}

void deleteDog(Game *game, Dog *node)
{
    if(node->prev == NULL)
    {
	if(node->next == NULL)
	{
	    game->dog = NULL;
	}
	else
	{
	    node->next->prev = NULL;
	    game->dog = node->next;
	}
    }
    else
    {
	if(node->next == NULL)
	{
	    node->prev->next = NULL;
	}
	else
	{
	    node->prev->next = node->next;
	    node->next->prev = node->prev;
	}
    }
    delete node;
    node = NULL;
}

void init_sounds(void)
{
	#ifdef USE_SOUND
	//FMOD_RESULT result;
	if (fmod_init()) {
		std::cout << "ERROR - fmod_init()\n" << std::endl;
		return;
	}
	if (fmod_createsound((char *)"./sounds/gunshot.wav", 0)) {
		std::cout << "ERROR - fmod_createsound()\n" << std::endl;
		return;
	}
	if (fmod_createsound((char *)"./sounds/drip.mp3", 1)) {
		std::cout << "ERROR - fmod_createsound()\n" << std::endl;
		return;
	}
	fmod_setmode(0,FMOD_LOOP_OFF);
	//fmod_playsound(0);
	//fmod_systemupdate();
	#endif //USE_SOUND
}

