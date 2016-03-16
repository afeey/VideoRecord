#include "stdafx.h"
#include <queue>
#include "string.h"
#include "Windows.h"

using namespace std;

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

AVFormatContext	*pInFormatCtx;	//�����ʽ������
AVStream		*pInStream;		//������

AVFormatContext	*pOutFormatCtx; //�����ʽ������
AVStream		*pOutStream;	//�����

AVCodecContext	*pInCodecCtx;
AVCodec			*pCodec;

int				videoIndex = -1;//��Ƶ������

int				i;
queue<AVFrame*> frameQueue;		//��Ƶ֡����
AVFrame			*pFrame;
AVFrame			*pFrameYUV;
uint8_t			*out_buffer;
AVPacket		*pInPacket;
int				ret;
int				got_picture;


int screen_w;
int	screen_h;
SDL_Window *screen; 
SDL_Renderer* sdlRenderer;
SDL_Texture* sdlTexture;
SDL_Rect	sdlRect;
SDL_Thread *receive_tid;	//�����߳�ID
SDL_Thread *refresh_tid;	//�����߳�ID
SDL_mutex *pLock;			//����
SDL_Event event;

struct SwsContext *img_convert_ctx;

//char *url = "rtsp://admin:@192.168.1.168:80/ch0_0.264";
char *url = "";
char *filename = "";

bool stop = false;


// �˳�
void Quit();

//��
int Open();
//����
int Play();
//�ر�
void Close();


// ��ȡ����Ƶ��
int receive_thread(void *opaque){
	while (!stop) {
		while(true){
			// ��ȡ���ݰ�
			try{
				if(av_read_frame(pInFormatCtx, pInPacket)<0){
					printf("��ȡ��������֡�������˳�\n");
					Quit();
				}
			}catch(exception e){
				printf("��ȡ����֡�쳣�������˳�\n");
				Quit();
			}

			if(pInPacket->stream_index == videoIndex){
				printf("�������ݰ�\n");
				ret = avcodec_decode_video2(pInCodecCtx, pFrame, &got_picture, pInPacket);
				if(ret < 0){
					printf("������󣬳����˳�\n");
					Quit();
					break;
				}
				if(got_picture){
					printf("ת��ͼƬ֡\n");
					pFrameYUV = av_frame_alloc();
					avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height);
					sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pInCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
					
					SDL_mutexP(pLock);
					frameQueue.push(pFrameYUV);
					SDL_mutexV(pLock);
				}

				printf("д���ݰ�...\n");
				// д�ļ�
				pInPacket->flags |= AV_PKT_FLAG_KEY;
				pInPacket->stream_index = 0;
				av_interleaved_write_frame(pOutFormatCtx, pInPacket);
				av_free_packet(pInPacket);
				break;
			}
		}
		SDL_Delay(1);
	}
	printf("�����߳��˳�\n");
	return 0;
}

// ��Ƶ��ʾˢ��
int refresh_thread(void *opaque){
	while (!stop) {
		if(!frameQueue.empty()){

			SDL_mutexP(pLock);
			AVFrame *outFrame = frameQueue.front();
			frameQueue.pop();
			SDL_mutexV(pLock);

			printf("ˢ����Ƶ,֡��ַ��%p,frame size %d\n",outFrame,frameQueue.size());
			SDL_UpdateTexture( sdlTexture, NULL, outFrame->data[0], outFrame->linesize[0] );  
			SDL_RenderClear( sdlRenderer );  
			SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);  
			SDL_RenderPresent( sdlRenderer );

			av_frame_free(&outFrame);
		}
		SDL_Delay(20);
	}
	printf("�����߳��˳�\n");
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//system("mode con cols=40 lines=20");
	HWND hwnd = GetConsoleWindow();
	HMENU hMenu=GetSystemMenu(hwnd,false);
	EnableMenuItem(hMenu,SC_CLOSE,MF_GRAYED|MF_BYCOMMAND);

	if(Open() == -1){
		return -1;
	}

	if(Play() == -1){
		return -1;
	}

	while(true){
		SDL_WaitEvent(&event);
		if(event.type==SDL_QUIT){
			printf("�˳�SDL����\n");
			stop = true;
			break;
		}
	}
	
	Close();

	printf("\n--------------��������˳�------------------\n");
	getchar();

	return 0;
}

// ��
int Open(){

	FILE *config = fopen("config.txt","rt");
	if(!config)
	{
		printf("�������ļ�ʧ��\n");
		getchar();
	}
	char config_url[500];
	fgets(config_url,500,config);
	fclose(config);

	string urlStr = config_url;
	urlStr = urlStr.replace(0,4,"");
	url = (char*)urlStr.data();
	printf("��ȡ�ļ�·��:%s\n",url);

	//��ȡ�����ļ���
	time_t t = time(NULL); //��ȡĿǰ��ʱ��  
	tm* local = localtime(&t); //תΪ����ʱ��

	char name[128]= {0};
	strftime(name, 64, "%Y%m%d%H%M%S", local); 
	string nameStr = name;
	nameStr+=".mp4";
	filename = (char*)nameStr.data();

	// ��ʼ��������������ļ���
	av_register_all();
	avformat_network_init();
	pInFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pInFormatCtx,url,NULL,NULL)!=0){
		printf("�޷����ļ�\n");
		getchar();
		return -1;
	}

	printf("---------------- �ļ���Ϣ ---------------\n");
	av_dump_format(pInFormatCtx,0,url,0);
	printf("-------------------------------------------------\n");

	if(avformat_find_stream_info(pInFormatCtx,NULL)<0){
		printf("δ��������Ϣ\n");
		getchar();
		return -1;
	}

	videoIndex=-1;
	for(i=0; i<pInFormatCtx->nb_streams; i++){
		if(pInFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoIndex=i;
			pInStream = pInFormatCtx->streams[i];
			break;
		}
	}

	if(videoIndex==-1){
		printf("δ������Ƶ��\n");
		getchar();
		return -1;
	}
	pInCodecCtx=pInFormatCtx->streams[videoIndex]->codec;
	pCodec=avcodec_find_decoder(pInCodecCtx->codec_id);
	if(pCodec==NULL){
		printf("δ���ֱ������\n");
		getchar();
		return -1;
	}
	if(avcodec_open2(pInCodecCtx, pCodec,NULL)<0){
		printf("�޷��򿪱������\n");
		return -1;
	}

	printf("������============\n");
	printf("time_base.num �� %d \n", pInCodecCtx->time_base.num);
	printf("time_base.den : %d\n", pInCodecCtx->time_base.den);
	printf("�����%d * %d , ���ظ�ʽ %d, ֡�ʣ�%d\n", pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt,pInCodecCtx->framerate);

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

	avio_open(&pOutFormatCtx->pb, filename, AVIO_FLAG_WRITE);
	avformat_write_header(pOutFormatCtx, NULL);
}

// ����
int Play(){
	// ������SDL����
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "�޷���ʼ��SDL - %s\n", SDL_GetError()); 
		getchar();
		return -1;
	} 
	
	screen_w = pInCodecCtx->width;
	screen_h = pInCodecCtx->height;
	screen = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h,SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);

	if(!screen) {  
		printf("SDL:��������ʧ��:%s\n",SDL_GetError()); 
		getchar();
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pInCodecCtx->width,pInCodecCtx->height);  

	sdlRect.x=0;
	sdlRect.y=0;
	sdlRect.w=screen_w;
	sdlRect.h=screen_h;

	pLock = SDL_CreateMutex();

	// �����߳�
	pInPacket=(AVPacket *)av_malloc(sizeof(AVPacket));
	receive_tid = SDL_CreateThread(receive_thread,NULL,NULL);

	pFrame = av_frame_alloc();
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height));
	
	img_convert_ctx = sws_getContext(pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt, 
		pInCodecCtx->width, pInCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	refresh_tid = SDL_CreateThread(refresh_thread,NULL,NULL);
}

// �ر�
void Close(){
	//��ʱ1���ͷ���Դ
	SDL_Delay(1000);
	SDL_Quit();
	SDL_DestroyMutex(pLock);

	av_write_trailer(pOutFormatCtx);
	printf("�����ļ�\n");

	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pInCodecCtx);
	avformat_close_input(&pInFormatCtx);
	avformat_close_input(&pOutFormatCtx);
}

//�˳�
void Quit(){
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
	stop = true;
}