/*
 * EffecTV - Realtime Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * main.c: start up module
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <SDL/SDL.h>

#include "EffecTV.h"
#include "effects/effects.h"
#include "effects/utils.h"

int debug = 0;

static effectRegistFunc *effects_register_list[] =
{
	dumbRegister,
	quarkRegister,
	fireRegister,
	burnRegister,
	blurzoomRegister,
	baltanRegister,
	streakRegister,
	onedRegister,
	dotRegister,
	mosaicRegister,
	puzzleRegister,
	predatorRegister,
//	spiralRegister,
	simuraRegister
};

static effect **effectsList;
static int effectMax;
static int currentEffectNum;
static effect *currentEffect;
static int fps = 0;

static void usage()
{
	printf("EffecTV - Realtime Video Effector\n");
	printf("Version: %s\n", VERSION_STRING);
	printf("Usage: effectv [options...]\n");
	printf("Options:\n");
	printf("\tdevice FILE\tuse device FILE for video4linux\n");
	printf("\tchannel NUMBER\tchannel number of video source\n");
	printf("\tnorm {ntsc,pal,secam,ntsc-jp}\tset video norm\n");
	printf("\tfullscreen\tenable fullscreen mode\n");
	printf("\tdouble\t\tdoubling screen size\n");
	printf("\thardware\tuse direct video memory(if possible)\n");
	printf("\tdoublebuffer\tenable double buffering mode(if possible)\n");
	printf("\tfps\t\tshow frames/sec\n");
}

static void drawErrorPattern()
{
	screen_clear(0xff0000);
}

static int registEffects()
{
	int i, n;
	effect *entry;

	n = sizeof(effects_register_list)/sizeof(effectRegistFunc *);
	effectsList = (effect **)malloc(n*sizeof(effect *));
	effectMax = 0;
	for(i=0;i<n;i++) {
		entry = (*effects_register_list[i])();
		if(entry) {
			printf("%.40s OK.\n",entry->name);
			effectsList[effectMax] = entry;
			effectMax++;
		}
	}
	printf("%d effects are available.\n",effectMax);
	return effectMax;
}

static int changeEffect(int num)
{
/* return value:
 *  0: fatal error
 *  1: success
 *  2: not available
 */
	if(currentEffect)
		currentEffect->stop();
	currentEffectNum = num;
	if(currentEffectNum < 0)
		currentEffectNum += effectMax;
	if(currentEffectNum >= effectMax)
		currentEffectNum -= effectMax;
	currentEffect = effectsList[currentEffectNum];
	screen_setcaption(currentEffect->name);
	if(currentEffect->start() < 0)
		return 2;

	return 1;
}

static int startTV()
{
	int flag;
	int frames=0;
	struct timeval tv;
	long lastusec=0, usec=0;
	SDL_Event event;
	char buf[256];

	currentEffectNum = 0;
	currentEffect = NULL;
	flag = changeEffect(currentEffectNum);

	if(fps) {
		gettimeofday(&tv, NULL);
		lastusec = tv.tv_sec*1000000+tv.tv_usec;
		frames = 0;
	}
	while(flag) {
		if(flag == 1) {
			flag = (currentEffect->draw())?2:1;
		}
		if (flag == 2) {
			drawErrorPattern();
			flag = 3;
		}
		if(fps) {
			frames++;
			if(frames == 100) {
				gettimeofday(&tv, NULL);
				usec = tv.tv_sec*1000000+tv.tv_usec;
				sprintf(buf, "%s (%2.2f fps)", currentEffect->name, (float)100000000/(usec - lastusec));
				screen_setcaption(buf);
				lastusec = usec;
				frames = 0;
			}
		}
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_KEYDOWN) {
				switch(event.key.keysym.sym) {
				case SDLK_UP:
					flag = changeEffect(currentEffectNum-1);
					break;
				case SDLK_DOWN:
					flag = changeEffect(currentEffectNum+1);
					break;
				case SDLK_ESCAPE:
					flag = 0;
					break;
				default:
					break;
				}
			}
			if(event.type == SDL_QUIT) flag=0;
			if(currentEffect->event) {
				currentEffect->event(&event);
			}
		}
	}
	currentEffect->stop();
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	char *option;
	int channel = 0;
	int norm = DEFAULT_VIDEO_NORM;
	char *devfile = NULL;

	for(i=1;i<argc;i++) {
		option = argv[i];
		if(*option == '-')
			option++;
		if (strncmp(option, "channel", 2) == 0) {
			i++;
			if(i<argc) {
				channel = atoi(argv[i]);
			} else {
				fprintf(stderr, "missing channel number.\n");
				exit(1);
			}
		} else if(strcmp(option, "norm") == 0) {
			i++;
			if(i<argc) {
				if((norm = videox_getnorm(argv[i])) < 0) {
					fprintf(stderr, "norm %s is not supported.\n", argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "missing norm.\n");
				exit(1);
			}
		} else if(strncmp(option, "device", 6) == 0) {
			i++;
			if(i<argc) {
				devfile = argv[i];
			} else {
				fprintf(stderr, "missing device file.\n");
				exit(1);
			}
		} else if(strncmp(option, "hardware", 8) == 0) {
			hwsurface = 1;
		} else if(strncmp(option, "fullscreen", 4) == 0) {
			fullscreen = 1;
		} else if(strncmp(option, "doublebuffer", 9) == 0) {
			doublebuf = 1;
		} else if(strncmp(option, "double", 6) == 0) {
			scale = 2;
		} else if(strncmp(option, "fps", 3) == 0) {
			fps = 1;
		} else if(strncmp(option, "help", 1) == 0) {
			usage();
			exit(0);
		} else {
			fprintf(stderr, "invalid option %s\n",argv[i]);
			usage();
			exit(1);
		}
	}

	srand(time(NULL));
	fastsrand(time(NULL));

	if(sharedbuffer_init(scale)){
		fprintf(stderr, "Memory allocation failed.\n");
		exit(1);
	}
	if(video_init(devfile, channel, norm)) {
		fprintf(stderr, "Video initialization failed.\n");
		exit(1);
	}
	if(screen_init()) {
		fprintf(stderr, "Screen initialization failed.\n");
		exit(1);
	}
	if(registEffects() == 0) {
		fprintf(stderr, "No available effect.\n");
		exit(1);
	}

//	showTitle();
	startTV();

	return 0;
}
