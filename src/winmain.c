#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iconv.h>
#include "resource.h"
#include "curl/curl.h"

#define	MY_USER_AGENT	"pagekeywordalert"
#define	CONFIG_PATH	"config"
#define	SAFE_FREE(MEM)	{if ((MEM)) free ((MEM)); (MEM) = NULL;}

HWND hDlg;
char g_errmsg[256];
int g_day[32];

struct doc {
	char *data;
	int len;
};

struct page_keyword {
	char *url;
	char *postdata;
	char *keyword;
	int oclock;
};
struct page_keyword g_pk;

int
charset_convert (const char *srcenc, const char *dstenc, char *buf, int len, char **ret)
{
	size_t inbytesleft, outbytesleft;
	char *inbuf, *outbuf, *sbuf;
	iconv_t cd;

	inbuf = buf;
	*ret = sbuf = outbuf = malloc (len * 4);
	inbytesleft = len;
	outbytesleft = (len * 4);

	if ((cd = iconv_open (dstenc, srcenc)) < 0) {
		printf ("iconv_open() error\n");
		goto ERR;
	}

	if (iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) < 0) {
		printf ("iconv() error\n");
		goto ERR;
	}

	if (iconv_close (cd) < 0) {
		printf ("iconv_close() error\n");
		goto ERR;
	}
	int outbuflen = (len * 4) - outbytesleft;
	if (outbuflen > 0) {
		sbuf[outbuflen] = '\0';
	}
	return outbuflen;
ERR:
	free(outbuf);
	outbuf = NULL;
	*ret = buf;
	return 0;
}

size_t
write_cb (void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct doc *doc = (struct doc *)stream;
	if (!doc->data) {
		doc->data = (char *)malloc (size*nmemb+1);
		memcpy (doc->data, ptr, size*nmemb);
		doc->len  = size*nmemb;
	} else {
		doc->data = (char *) realloc (doc->data, doc->len + size*nmemb + 1);
		memcpy (&doc->data[doc->len], ptr, size*nmemb);
		doc->len  += size*nmemb;
	}
	return size*nmemb;
}

void
page_keyword_free (struct page_keyword *pk)
{
	SAFE_FREE(pk->url);
	SAFE_FREE(pk->keyword);
	SAFE_FREE(pk->postdata);
}

void
page_keyword_set (struct page_keyword *pk)
{
	char tmps[1024];
	char *utmps;
	GetDlgItemText (hDlg, IDC_EDIT_URL, tmps, sizeof(tmps));
	if (tmps[0]) {
		charset_convert ("BIG5", "UTF-8", tmps, strlen(tmps), &utmps);
		pk->url = strdup (utmps? utmps: tmps);
		SAFE_FREE(utmps);
	}
	GetDlgItemText (hDlg, IDC_EDIT_KEYWORD, tmps, sizeof(tmps));
	if (tmps[0]) {
		charset_convert ("BIG5", "UTF-8", tmps, strlen(tmps), &utmps);
		pk->keyword = strdup (utmps? utmps: tmps);
		SAFE_FREE(utmps);
	}
	GetDlgItemText (hDlg, IDC_EDIT_POSTDATA, tmps, sizeof(tmps));
	if (tmps[0]) {
		charset_convert ("BIG5", "UTF-8", tmps, strlen(tmps), &utmps);
		pk->postdata = strdup (utmps? utmps: tmps);
		SAFE_FREE(utmps);
	}
	GetDlgItemText (hDlg, IDC_EDIT_OCLOCK, tmps, sizeof(tmps));
	if (tmps[0]) {
		pk->oclock = atoi(tmps);
	}
}

int
page_keyword_alert (char *url, char *postdata, char *keyword)
{
	CURL *curl = NULL;
	CURLcode res;
	struct doc doc = {NULL, 0};

	if (!(curl=curl_easy_init())) {
                sprintf (g_errmsg, "%s:%d curl_easy_init() fail", __FILE__, __LINE__);
                return -1;
        }
        if ((res=curl_easy_setopt(curl, CURLOPT_URL, url)) != CURLE_OK) {
                sprintf (g_errmsg, "%s:%d curl_easy_setopt: %s", __FILE__, __LINE__, curl_easy_strerror(res));
		goto ERR;
        }
	if (postdata) {
		if ((res=curl_easy_setopt(curl, CURLOPT_POST, 1)) != CURLE_OK ||
		    (res=curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata)) != CURLE_OK) {
			sprintf (g_errmsg, "%s:%d curl_easy_setopt: %s", __FILE__, __LINE__, curl_easy_strerror(res));
			goto ERR;
		}
	}
	if ((res=curl_easy_setopt(curl, CURLOPT_USERAGENT, MY_USER_AGENT)) != CURLE_OK){
                sprintf (g_errmsg, "%s:%d curl_easy_setopt: %s", __FILE__, __LINE__, curl_easy_strerror(res));
		goto ERR;
        }
	if ((res=curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb)) != CURLE_OK) {
		sprintf (g_errmsg, "%s:%d curl_easy_setopt: %s", __FILE__, __LINE__, curl_easy_strerror(res));
		goto ERR;
	}
	if ((res=curl_easy_setopt(curl, CURLOPT_WRITEDATA, &doc)) != CURLE_OK) {
		sprintf (g_errmsg, "%s:%d curl_easy_setopt: %s", __FILE__, __LINE__, curl_easy_strerror(res));
		goto ERR;
	}
	if ((res = curl_easy_perform(curl) != CURLE_OK)) {
		sprintf (g_errmsg, "%s:%d curl_easy_perform: %s", __FILE__, __LINE__, curl_easy_strerror(res));
		goto ERR;
	}
	if (doc.data) {
		doc.data[doc.len] = '\0';
		if (strstr(doc.data, keyword)) {
			MessageBox(NULL, "keyword matched", "Alert", MB_OK);
		}
		free(doc.data);
	}
	return 0;
ERR:
	curl_easy_cleanup(curl);
	return -1;
}

void CALLBACK
page_keyword_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	if (lt.wHour == g_pk.oclock) {
		if (!g_day[lt.wDay]) {
			memset (g_day, 0, sizeof(g_day));
			if (g_pk.url && g_pk.keyword) {
				g_day[lt.wDay]++;
				page_keyword_alert (g_pk.url, g_pk.postdata, g_pk.keyword);
			}
		}
	}
}


INT_PTR CALLBACK
DialogProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:{
			memset (g_day, 0, sizeof(g_day));
			memset (&g_pk, 0 , sizeof(struct page_keyword));
			FILE *fp = fopen(CONFIG_PATH,"r");
			if (fp) {
				char utmps[1024];
				char *tmps=NULL;
				if (fgets(utmps, sizeof(utmps), fp)) {
					strtok(utmps, "\r\n");
					g_pk.url = strdup(utmps);
					charset_convert ("UTF-8", "BIG5", utmps, strlen(utmps), &tmps);
					SetDlgItemText (hDlg, IDC_EDIT_URL, tmps? tmps: utmps);
					SAFE_FREE(tmps);
					if (fgets(utmps, sizeof(utmps), fp)) {
						strtok(utmps, "\r\n");
						g_pk.keyword = strdup(utmps);
						charset_convert ("UTF-8", "BIG5", utmps, strlen(utmps), &tmps);
						SetDlgItemText (hDlg, IDC_EDIT_KEYWORD, tmps? tmps: utmps);
						SAFE_FREE(tmps);
						if (fgets(utmps, sizeof(utmps), fp)) {
							strtok(utmps, "\r\n");
							g_pk.oclock = atoi(utmps);
							SetDlgItemText (hDlg, IDC_EDIT_OCLOCK, utmps);
							if (fgets(utmps, sizeof(utmps), fp)) {
								strtok(utmps, "\r\n");
								g_pk.postdata = strdup(utmps);
								charset_convert ("UTF-8", "BIG5", utmps, strlen(utmps), &tmps);
								SetDlgItemText (hDlg, IDC_EDIT_POSTDATA, tmps? tmps: utmps);
								SAFE_FREE(tmps);
							}
						}
					}
				}
				fclose (fp);
			}
			break;}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
				case IDSAVE:{
					struct page_keyword pk;
					memset (&pk, 0, sizeof(struct page_keyword));
					page_keyword_set(&pk);
					if (pk.url && pk.keyword) {
						FILE *fp = fopen(CONFIG_PATH, "w");
						if (fp) {
							fprintf (fp, "%s\r\n", pk.url);
							fprintf (fp, "%s\r\n", pk.keyword);
							fprintf (fp, "%d\r\n", pk.oclock);
							if (pk.postdata) {
								fprintf (fp, "%s\r\n", pk.postdata);
							}
							fclose(fp);

							page_keyword_free(&g_pk);
							g_pk.url = strdup (pk.url);
							g_pk.keyword = strdup (pk.keyword);
							g_pk.oclock = pk.oclock;
							if (pk.postdata)
								g_pk.url = strdup (pk.postdata);
						}
					}
					page_keyword_free(&pk);
					break;}
				case IDTEST:{
					struct page_keyword pk;
					memset (&pk, 0, sizeof(struct page_keyword));
					page_keyword_set(&pk);
					if (pk.url && pk.keyword)
						page_keyword_alert (pk.url, pk.postdata, pk.keyword);
					page_keyword_free(&pk);
					break;}
				default:
					break;
			}
			break;
		case WM_CLOSE:
			page_keyword_free(&g_pk);
			DestroyWindow(hDlg);
			return TRUE;

		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;
		default:
			break;
	}
	return FALSE;
}

int WINAPI
WinMain (HINSTANCE hInst, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow)
{

	MSG msg;
	BOOL ret;
	InitCommonControls();

	hDlg = CreateDialogParam(hInst, "MAINDLG", 0, DialogProc, 0);

	SetTimer (hDlg, IDT_TIMER1, 10000, page_keyword_timer);

	ShowWindow(hDlg, nCmdShow);

	while((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
		if(ret == -1)
			return -1;
		if(!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

