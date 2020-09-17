/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * video.h: header for video manager
 *
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <v4lutils.h>

typedef struct _normlist
{
	char name[10];
	int type;
} normlist;

extern int video_horizontalFlip;
extern int video_width;
extern int video_height;
extern int video_area;

int video_init(char *file, int channel, int w, int h);
void video_quit(void);
int video_setformat(int palette);
int video_grab_check(int palette);
int video_set_grabformat(int palette);
int video_grabstart(void);
int video_grabstop(void);
int video_setfreq(int v);
int video_syncframe(void);
int video_grabframe(void);
unsigned char *video_getaddress(void);
int video_retry(void);

#endif /* __VIDEO_H__ */
