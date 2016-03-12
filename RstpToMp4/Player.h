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

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 1)

class Player
{
private:
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;

	AVFormatContext	*pInFormatCtx;
    AVStream		*pInStream;

	AVFormatContext *pOutFormatCtx;
	AVStream		*pOutStream;

	char*			url;		//播放视频路径
	int				videoIndex; //视频流索引
	bool			record;		//录制视频标志
	char*			filename;	//保存文件名
	bool			stop;		//停止标志
	int				frameCount; //帧计数

public:
	Player(char* url){
		pCodecCtx = NULL;
		pCodec = NULL;
		pInFormatCtx = NULL;
		pInStream = NULL;
		pOutFormatCtx = NULL;
		pOutStream = NULL;

		videoIndex = -1;
		record = false;
		this-> url = url;
		filename = "";
		stop = false;
		frameCount = 0;
	}

	// 设置录像
	void SetRecord(bool record);

	// 设置录像文件名
	void SetFileName(char* filename);

	// 播放
	int Play();

	// 录像
	int Record();

	// 开始
	int Start();

	// 停止
	void Stop();

	~Player(){
	}
};