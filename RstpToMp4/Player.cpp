#include "Player.h"
#include <windows.h>

void Player::SetRecord(bool record){
	this->record = record;
}

void Player::SetFileName(char* filename){
	this->filename = filename;
}

int Player::Play()
{
	av_register_all();
	avcodec_register_all();
	avformat_network_init();

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

	if(pInFormatCtx==NULL)
	{
		printf("未发现视频流\n");
		return -1;
	}

	pCodecCtx=pInStream ->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);

	if(pCodec == NULL)
	{
		printf("未发现编解码器\n");
		return -1;
	}

	printf("输入流============\n");
	printf("分辨率：%d * %d \n",pCodecCtx ->width,pCodecCtx ->height);
	printf("格式：%s\n",pCodec->name);
	printf("像素格式：%d\n",pCodecCtx->pix_fmt);

	if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
	{
		printf("无法打开编解码器\n");
		return -1;
	}

	AVFrame *pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();     //存储解码后AVFrame
	pFrameYUV = av_frame_alloc();  //存储转换后AVFrame

	uint8_t *out_buffer;
	out_buffer=new uint8_t[avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height)];		 //分配AVFrame所需内存
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height); //填充AVFrame
	pFrameYUV->width = pCodecCtx->width;
	pFrameYUV->height =  pCodecCtx->height;
	int outLinesize[4] = {pFrameYUV->width, pFrameYUV->width/2, pFrameYUV->width/2, 0};

	//------------SDL初始化--------
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "无法初始化 SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
	wchar_t* wTitle = L"RTSP播放器";
	char title[1024] = {};
	WideCharToMultiByte(CP_UTF8,0,wTitle,-1,title,sizeof(title),NULL,NULL);
	SDL_Window *screen = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		pCodecCtx->width, pCodecCtx->height,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_OPENGL);
	if(!screen) {  
		printf("SDL: 无法创建视频窗口\n");  
		return -1;
	}
	
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	SDL_Texture* sdlTexture = SDL_CreateTexture(  
		sdlRenderer,  
		SDL_PIXELFORMAT_IYUV,  
		SDL_TEXTUREACCESS_STREAMING,  
		pCodecCtx->width,  
		pCodecCtx->height
		); 

	SDL_Rect rect;
	//-----------------------------
	int ret, got_picture;
	static struct SwsContext *img_convert_ctx;
	int y_size = pCodecCtx->width * pCodecCtx->height;

	SDL_Event event; 
	AVPacket *i_pkt=(AVPacket *)malloc(sizeof(AVPacket));//存储解码前数据包AVPacket
	av_new_packet(i_pkt, y_size);

	//输出一下信息-----------------------------
	//printf("文件信息-----------------------------------------\n");
	//av_dump_format(pInFormatCtx,0,filepath,0);
	//printf("-------------------------------------------------\n");
	//------------------------------

	while(av_read_frame(pInFormatCtx, i_pkt)>=0)	//循环获取压缩数据包AVPacket
	{
		// 解码显示
		if(i_pkt->stream_index==videoIndex)
		{
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, i_pkt);//解码。输入为AVPacket，输出为AVFrame
			if(ret < 0)
			{
				printf("解码错误\n");
				return -1;
			}else if(ret == 0){
				continue;			
			}

			if(got_picture)
			{
				//像素格式转换。pFrame转换为pFrameYUV。
				img_convert_ctx = sws_getContext(pFrame->width, pFrame->height, pCodecCtx->pix_fmt, pFrameYUV->width, pFrameYUV->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pFrame->height, pFrameYUV->data, pFrameYUV->linesize);
				sws_freeContext(img_convert_ctx);
				//------------SDL显示--------
				rect.x = 0;    
				rect.y = 0;    
				rect.w = pFrameYUV->width;    
				rect.h = pFrameYUV->height;    

				SDL_UpdateTexture( sdlTexture, &rect, pFrameYUV->data[0], pFrameYUV->linesize[0] );  
				SDL_RenderClear( sdlRenderer );  
				SDL_RenderCopy( sdlRenderer, sdlTexture, &rect, &rect );  
				SDL_RenderPresent( sdlRenderer );
				//延时20ms
				SDL_Delay(1);
				//------------SDL-----------
			}
		}

		//av_free_packet(i_pkt);
		SDL_PollEvent(&event);  
		switch( event.type ) {  
		case SDL_QUIT:  
			SDL_Quit();  
			exit(0);  
			break;  
		default:  
			break;  
		}  
	}
	av_write_trailer(pOutFormatCtx);
	avio_close(pOutFormatCtx->pb);

	SDL_DestroyTexture(sdlTexture); 

	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pInFormatCtx);

	delete[] out_buffer;
	avcodec_close(pOutFormatCtx->streams[0]->codec);
	av_freep(&pOutFormatCtx->streams[0]->codec);
	av_freep(&pOutFormatCtx->streams[0]);
	av_free(pOutFormatCtx);
}

int Player::Record()
{
	av_register_all();
	avcodec_register_all();
	avformat_network_init();

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
		if(pInFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			pInStream = pInFormatCtx->streams[i];
			videoIndex = i;
			break;
		}
	}

	if(pInFormatCtx == NULL)
	{
		printf("未发现视频流\n");
		return -1;
	}

	pCodecCtx = pInStream -> codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if(pCodec == NULL)
	{
		printf("未发现编解码器\n");
		return -1;
	}

	printf("输入流============\n");
	printf("分辨率：%d * %d \n",pCodecCtx ->width,pCodecCtx ->height);
	printf("格式：%s\n",pCodec->name);
	printf("像素格式：%d\n",pCodecCtx->pix_fmt);

	if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
	{
		printf("无法打开编解码器\n");
		return -1;
	}

	avformat_alloc_output_context2(&pOutFormatCtx, NULL, NULL, filename);
	pOutStream = avformat_new_stream(pOutFormatCtx, NULL);
	{
		AVCodecContext *c;
		c = pOutStream->codec;
		c->bit_rate = 400000;
		c->codec_id = pInStream->codec->codec_id;
		c->codec_type = pInStream->codec->codec_type;
		c->time_base.num = pInStream->time_base.num;
		c->time_base.den = pInStream->time_base.den;
		
		c->width = pInStream->codec->width;
		c->height = pInStream->codec->height;
		c->pix_fmt = pInStream->codec->pix_fmt;
		
		c->flags = pInStream->codec->flags;
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		c->me_range = pInStream->codec->me_range;
		c->max_qdiff = pInStream->codec->max_qdiff;

		c->qmin = pInStream->codec->qmin;
		c->qmax = pInStream->codec->qmax;

		c->qcompress = pInStream->codec->qcompress;

		printf("\n输出流============\n");
		printf("time_base.num ： %d \n", c->time_base.num);
		printf("time_base.den : %d\n", c->time_base.den);
		printf("输出：%d * %d , 像素格式 %d", c->width, c->height, c->pix_fmt);
	}

	avio_open(&pOutFormatCtx->pb, filename, AVIO_FLAG_WRITE);
	avformat_write_header(pOutFormatCtx, NULL);

	int last_pts = 0;
    int last_dts = 0;

    int64_t pts, dts;
    while (!stop)
    {
        AVPacket i_pkt;
        av_init_packet(&i_pkt);
        i_pkt.size = 0;
        i_pkt.data = NULL;
        if (av_read_frame(pInFormatCtx, &i_pkt) < 0 )
            break;

        i_pkt.flags |= AV_PKT_FLAG_KEY;
        pts = i_pkt.pts;
        i_pkt.pts += last_pts;
        dts = i_pkt.dts;
        i_pkt.dts += last_dts;
        i_pkt.stream_index = 0;

        printf("frame %d, pts:%lld, dts:%lld\n",frameCount++, i_pkt.pts, i_pkt.dts);

        av_interleaved_write_frame(pOutFormatCtx, &i_pkt);
        //av_free_packet(&i_pkt);
        //av_init_packet(&i_pkt);
         Sleep(1);
    }
    last_dts += dts;
    last_pts += pts;

    avformat_close_input(&pInFormatCtx);

    av_write_trailer(pOutFormatCtx);
    avcodec_close(pOutFormatCtx->streams[0]->codec);
    av_freep(&pOutFormatCtx->streams[0]->codec);
    av_freep(&pOutFormatCtx->streams[0]);

    avio_close(pOutFormatCtx->pb);
    av_free(pOutFormatCtx);

    return 0;
}

int Player::Start(){
	return -1;
}

void Player::Stop(){
	stop = true;
}