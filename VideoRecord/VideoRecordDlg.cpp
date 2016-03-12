
// VideoRecordDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "VideoRecord.h"
#include "VideoRecordDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "PlayerCore.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVideoRecordDlg 对话框



CVideoRecordDlg::CVideoRecordDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVideoRecordDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVideoRecordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVideoRecordDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CVideoRecordDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_CLOSE, &CVideoRecordDlg::OnBnClickedClose)
END_MESSAGE_MAP()


// CVideoRecordDlg 消息处理程序

BOOL CVideoRecordDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//CString filepath = L"rtsp://admin:@192.168.1.168:80/ch0_0.264";
	CString filepath = L"rtsp://218.204.223.237:554/live/1/66251FC11353191F/e7ooqwcfbqjoo80j.sdp";
	
	GetDlgItem(IDC_URL)->SetWindowTextW(filepath);

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CVideoRecordDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CVideoRecordDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CVideoRecordDlg::OnBnClickedStart()
{

	CString text("");
	GetDlgItem(IDC_START)->GetWindowTextW(text);

	if(text=="开始"){
		GetDlgItem(IDC_START)->SetWindowTextW(L"停止");

		CString url_text("");
		GetDlgItem(IDC_URL)->GetWindowTextW(url_text);

		int len =WideCharToMultiByte(CP_ACP,0,url_text,-1,NULL,0,NULL,NULL);  
		char *url =new char[len +1];  
		WideCharToMultiByte(CP_ACP,0,url_text,-1,url,len,NULL,NULL ); 

		CTime tm = CTime::GetCurrentTime();
		CString nameStr = tm.Format("%Y%m%d%H%M%S.mp4");

		len =WideCharToMultiByte(CP_ACP,0,nameStr,-1,NULL,0,NULL,NULL);  
		char *filename =new char[len + 1];  
		WideCharToMultiByte(CP_ACP,0,nameStr,-1,filename,len,NULL,NULL );
		
		Player_Open(url,filename);
		Player_Play();

	}else{
		GetDlgItem(IDC_START)->SetWindowTextW(L"开始");
	}
}

void CVideoRecordDlg::OnBnClickedClose()
{
	CDialog::OnCancel();
}
