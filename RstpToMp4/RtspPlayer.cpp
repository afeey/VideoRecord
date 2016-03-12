#include "stdafx.h"
#include "Player.h"
#include <windows.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <SDL.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

static char* url;
static char* filename;
static Player player(url);
static int Play();
static unsigned __stdcall Record(void * pThis);

int _tmain(int argc, _TCHAR* argv[])
{	
	url = "rtsp://218.204.223.237:554/live/1/66251FC11353191F/e7ooqwcfbqjoo80j.sdp";
	//url = "rtsp://admin:@192.168.1.168:80/ch0_0.264";
	filename = "C:\\192.168.1.168_ch0_0.264.mp4";

	player = Player(url);
	player.SetRecord(true);
	player.SetFileName(filename);

	//HANDLE hth;  
 //   unsigned uiThreadID;  
 // 
 //   hth = (HANDLE)_beginthreadex( NULL,				// security  
 //                                  0,				// stack size  
	//							   Record,
	//							   NULL,			// arg list  
 //                                  0,				// CREATE_SUSPENDED so we can later call ResumeThread()  
 //                                  &uiThreadID);  
 // 
 //   if ( hth == 0 ) 
 //   {
 //       printf("创建线程失败\n"); 
 //       return -1;
 //   }

 //   printf("按任意键停止录像\n");
 //   getchar();

	//player.Stop();

 //   printf("按任意键退出\n");
 //   getchar();
       
 	Play();

    return 0;
}

static int Play()
{
	player.Play();

	return 0;
}

static unsigned __stdcall Record(void * pThis)
{
	player.Record();

	return 0;
}