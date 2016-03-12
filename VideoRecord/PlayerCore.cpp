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

#define REFRESH_EVENT  (SDL_USEREVENT + 1)	//ˢ��	
#define QUIT_EVENT  (SDL_USEREVENT + 2)		//�˳�

AVCodecContext	*pInCodecCtx = NULL;		//�������������
AVCodec			*pInCodec = NULL;			//�������

AVFormatContext	*pInFormatCtx = NULL;	//�����ʽ������
AVStream		*pInStream = NULL;		//������

AVFormatContext *pOutFormatCtx = NULL;	//�����ʽ������
AVStream		*pOutStream = NULL;		//������

int				videoIndex = -1;		//������Ƶ������

int screen_w;				//���ڿ�
int	screen_h;				//���ڸ�
SDL_Window *screen;			//����
SDL_Renderer* sdlRenderer;	//��Ⱦ��
SDL_Texture* sdlTexture;	//����
SDL_Rect sdlRect;			//������
SDL_Thread *receive_tid;	//��Ƶ�����߳�ID
SDL_Thread *refresh_tid;	//��Ƶ��ʾ�߳�ID
SDL_Event event;			//�¼�


AVFrame			*pFrame;		//��Ƶ֡
AVFrame			*pFrameYUV;		//YUV��Ƶ֡
uint8_t			*out_buffer;			
AVPacket		*packet;
int				ret;
int				got_picture;
SwsContext *img_convert_ctx;	//ͼƬת��������

int stop = false;			// ֹͣ��־

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

	// ��ʼ��
	av_register_all();
	avcodec_register_all();
	avformat_network_init();

	// ���ļ�������Ƶ��
	pInFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&pInFormatCtx,url,NULL,NULL)!=0){
		printf("�޷����ļ�\n");
		return -1;
	}

	if(avformat_find_stream_info(pInFormatCtx, NULL)<0)
	{
		printf("δ��������Ϣ\n");
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
		printf("δ������Ƶ��\n");
		return -1;
	}

	// ���ҡ�����Ƶ���������
	pInCodecCtx = pInStream ->codec;
	pInCodec=avcodec_find_decoder(pInCodecCtx->codec_id);

	if(pInCodec == NULL)
	{
		printf("δ���ֱ������\n");
		return -1;
	}

	if(avcodec_open2(pInCodecCtx, pInCodec, NULL)<0)
	{
		printf("�޷��򿪱������\n");
		return -1;
	}

	printf("������============\n");
	printf("�ֱ��ʣ�%d * %d \n",pInCodecCtx ->width,pInCodecCtx ->height);
	printf("��ʽ��%s\n",pInCodec->name);
	printf("���ظ�ʽ��%d\n",pInCodecCtx->pix_fmt);

	
	// ��ʼ�������ʽ������
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

		printf("\n�����============\n");
		printf("time_base.num �� %d \n", pOutCodexCtx->time_base.num);
		printf("time_base.den : %d\n", pOutCodexCtx->time_base.den);
		printf("�����%d * %d , ���ظ�ʽ %d", pOutCodexCtx->width, pOutCodexCtx->height, pOutCodexCtx->pix_fmt);
	}
	// �������ļ���д��ͷ
	avio_open(&pOutFormatCtx->pb, filename, AVIO_FLAG_WRITE);
	avformat_write_header(pOutFormatCtx, NULL);
}

int Player_Play(){

	// ��ʼ��SDL
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

	// ��ʼ��֡��ͼƬת��������
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height);

	img_convert_ctx = sws_getContext(pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt, 
		pInCodecCtx->width, pInCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	
	// �����߳�
	receive_tid = SDL_CreateThread(receive_thread,NULL,NULL);
	refresh_tid = SDL_CreateThread(refresh_thread,NULL,NULL);


	return 0;
}

int Play_Stop(){
	return 0;
}