#include "stdafx.h"

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

#ifdef __cplusplus
}
#endif

#define REFRESH_EVENT  (SDL_USEREVENT + 1)	//刷新	
#define QUIT_EVENT  (SDL_USEREVENT + 2)		//退出

AVCodecContext	*pInCodecCtx = NULL;		//编解码器上下文
AVCodec			*pInCodec = NULL;			//编解码器

AVFormatContext	*pInFormatCtx = NULL;	//输入格式上下文
AVStream		*pInStream = NULL;		//输入流

AVFormatContext *pOutFormatCtx = NULL;	//输出格式上下文
AVStream		*pOutStream = NULL;		//输入流

int				videoIndex = -1;		//输入视频流索引

int screen_w;				//窗口宽
int	screen_h;				//窗口高
SDL_Window *screen;			//窗口
SDL_Renderer* sdlRenderer;	//渲染器
SDL_Texture* sdlTexture;	//纹理
SDL_Rect sdlRect;			//矩形区
SDL_Thread *receive_tid;	//视频接收线程ID
SDL_Thread *refresh_tid;	//视频显示线程ID
SDL_Event event;			//事件


AVFrame			*pFrame;		//视频帧
AVFrame			*pFrameYUV;		//YUV视频帧
uint8_t			*out_buffer;			
AVPacket		*packet;
int				ret;
int				got_picture;
SwsContext *img_convert_ctx;	//图片转换上下文

int stop = false;			// 停止标志

// 
int receive_thread(void *opaque){
	while (!stop) {
		
		SDL_Delay(1);
	}

	SDL_Event event;
	event.type = QUIT_EVENT;
	SDL_PushEvent(&event);
	return 0;
}

int refresh_thread(void *opaque){
	while (!stop) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(30);
	}
	return 0;
}

int Player_Open(char* url,char* filename){

	// 初始化
	av_register_all();
	avcodec_register_all();
	avformat_network_init();

	// 打开文件查找视频流
	pInFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&pInFormatCtx,url,NULL,NULL)!=0){
		printf("无法打开文件\n");
		return -1;
	}

	if(avformat_find_stream_info(pInFormatCtx, NULL)<0)
	{
		printf("未发现流信息\n");
		return -1;
	}

	for(int i=0; i<pInFormatCtx->nb_streams; i++)
	{ 
		if(pInFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			pInStream = pInFormatCtx->streams[i];
			videoIndex = i;
			break;
		}
	}

	if(videoIndex==-1)
	{
		printf("未发现视频流\n");
		return -1;
	}

	// 查找、打开视频流编解码器
	pInCodecCtx = pInStream ->codec;
	pInCodec=avcodec_find_decoder(pInCodecCtx->codec_id);

	if(pInCodec == NULL)
	{
		printf("未发现编解码器\n");
		return -1;
	}

	if(avcodec_open2(pInCodecCtx, pInCodec, NULL)<0)
	{
		printf("无法打开编解码器\n");
		return -1;
	}

	printf("输入流============\n");
	printf("分辨率：%d * %d \n",pInCodecCtx ->width,pInCodecCtx ->height);
	printf("格式：%s\n",pInCodec->name);
	printf("像素格式：%d\n",pInCodecCtx->pix_fmt);

	
	// 初始化输出格式上下文
	avformat_alloc_output_context2(&pOutFormatCtx, NULL, NULL, filename);
	pOutStream = avformat_new_stream(pOutFormatCtx, NULL);
	{
		AVCodecContext *pOutCodexCtx;
		pOutCodexCtx = pOutStream->codec;
		pOutCodexCtx->bit_rate = 400000;
		pOutCodexCtx->codec_id = pInStream->codec->codec_id;
		pOutCodexCtx->codec_type = pInStream->codec->codec_type;
		pOutCodexCtx->time_base.num = pInStream->time_base.num;
		pOutCodexCtx->time_base.den = pInStream->time_base.den;
		
		pOutCodexCtx->width = pInStream->codec->width;
		pOutCodexCtx->height = pInStream->codec->height;
		pOutCodexCtx->pix_fmt = pInStream->codec->pix_fmt;
		
		pOutCodexCtx->flags = pInStream->codec->flags;
		pOutCodexCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
		pOutCodexCtx->me_range = pInStream->codec->me_range;
		pOutCodexCtx->max_qdiff = pInStream->codec->max_qdiff;

		pOutCodexCtx->qmin = pInStream->codec->qmin;
		pOutCodexCtx->qmax = pInStream->codec->qmax;

		pOutCodexCtx->qcompress = pInStream->codec->qcompress;

		printf("\n输出流============\n");
		printf("time_base.num ： %d \n", pOutCodexCtx->time_base.num);
		printf("time_base.den : %d\n", pOutCodexCtx->time_base.den);
		printf("输出：%d * %d , 像素格式 %d", pOutCodexCtx->width, pOutCodexCtx->height, pOutCodexCtx->pix_fmt);
	}
	// 打开输入文件，写入头
	avio_open(&pOutFormatCtx->pb, filename, AVIO_FLAG_WRITE);
	avformat_write_header(pOutFormatCtx, NULL);
}

int Player_Play(){

	// 初始化SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 

	screen_w = pInCodecCtx->width;
	screen_h = pInCodecCtx->height;
	screen = SDL_CreateWindow("Video", 0, 0,
		screen_w, screen_h,SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED);

	if(!screen) {  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pInCodecCtx->width,pInCodecCtx->height);  

	/*sdlRect.x=0;
	sdlRect.y=0;
	sdlRect.w=screen_w;
	sdlRect.h=screen_h;*/

	// 初始化帧、图片转换上下文
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height);

	img_convert_ctx = sws_getContext(pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt, 
		pInCodecCtx->width, pInCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	
	// 创建线程
	receive_tid = SDL_CreateThread(receive_thread,NULL,NULL);
	refresh_tid = SDL_CreateThread(refresh_thread,NULL,NULL);


	return 0;
}

int Play_Stop(){
	return 0;
}