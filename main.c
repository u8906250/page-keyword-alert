#include <gtk/gtk.h>
#include <string.h>
#include "curl/curl.h"

#define	MY_USER_AGENT	"pagekeywordalert"
#define	CONFIG_PATH	"config"
#define	SAFE_FREE(MEM)	{if ((MEM)) free ((MEM)); (MEM) = NULL;}

char g_errmsg[256];
int g_day[32];

GtkEntryBuffer *url_buf;
GtkEntryBuffer *keyword_buf;
GtkEntryBuffer *postdata_buf;
GtkEntryBuffer *oclock_buf;

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
	char *utmps;
	if ((utmps = gtk_entry_buffer_get_text(url_buf))) {
		pk->url = strdup (utmps);
	}
	if ((utmps = gtk_entry_buffer_get_text(keyword_buf))) {
		pk->keyword = strdup (utmps);
	}
	if ((utmps = gtk_entry_buffer_get_text(postdata_buf))) {
		pk->postdata = strdup (utmps);
	}
	if ((utmps = gtk_entry_buffer_get_text(oclock_buf))) {
		pk->oclock = atoi(utmps);
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
			GtkWidget *window = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, "keyword matched");
			gtk_widget_show_all (window);
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

static void
test_cb (GtkWidget *widget, gpointer data)
{
	struct page_keyword pk;
	memset (&pk, 0, sizeof(struct page_keyword));
	page_keyword_set(&pk);
	if (pk.url && pk.keyword)
		page_keyword_alert (pk.url, pk.postdata, pk.keyword);
	page_keyword_free(&pk);
}

static void
save_cb (GtkWidget *widget, gpointer data)
{
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
}

static void
activate (GtkApplication* app, gpointer  user_data)
{
	GtkWidget *window;
	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "page keyword alert");
	gtk_window_set_default_size (GTK_WINDOW (window), 400, 100);

	GtkWidget *vbox;
	vbox = gtk_button_box_new (GTK_ORIENTATION_VERTICAL);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	GtkWidget *hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	GtkWidget *grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (hbox), grid);

	GtkWidget *label = gtk_label_new ("url");
	gtk_grid_attach ((GtkGrid *)grid, label, 0, 0, 10, 10);

	url_buf = gtk_entry_buffer_new("", 256);
	GtkWidget *url_entry = gtk_entry_new_with_buffer(url_buf);
	gtk_grid_attach ((GtkGrid *)grid, url_entry, 10, 0, 10, 10);

	GtkWidget *label2 = gtk_label_new ("keyword");
	gtk_grid_attach_next_to ((GtkGrid *)grid, label2, label, GTK_POS_BOTTOM, 10, 10);
	keyword_buf = gtk_entry_buffer_new("", 256);
	GtkWidget *keyword_entry = gtk_entry_new_with_buffer(keyword_buf);
	gtk_grid_attach_next_to ((GtkGrid *)grid, keyword_entry, label2, GTK_POS_RIGHT, 10, 10);

	GtkWidget *label3= gtk_label_new ("postdata");
	gtk_grid_attach_next_to ((GtkGrid *)grid, label3, label2, GTK_POS_BOTTOM, 10, 10);
	postdata_buf = gtk_entry_buffer_new("", 256);
	GtkWidget *postdata_entry = gtk_entry_new_with_buffer(postdata_buf);
	gtk_grid_attach_next_to ((GtkGrid *)grid, postdata_entry, label3, GTK_POS_RIGHT, 10, 10);


	hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (hbox), grid);

	GtkWidget *button1;
	button1 = gtk_button_new_with_label ("test");
	g_signal_connect (button1, "clicked", G_CALLBACK (test_cb), NULL);
	gtk_grid_attach ((GtkGrid *)grid, button1, 0, 0, 10, 10);

	GtkWidget *button2 = gtk_button_new_with_label ("save");
	g_signal_connect (button2, "clicked", G_CALLBACK (save_cb), NULL);
	gtk_grid_attach_next_to ((GtkGrid *)grid, button2, button1, GTK_POS_RIGHT, 10, 10);

	GtkWidget *label4 = gtk_label_new ("run once every day");
	gtk_grid_attach_next_to ((GtkGrid *)grid, label4, button2, GTK_POS_RIGHT, 10, 10);

	oclock_buf = gtk_entry_buffer_new("", 256);
	GtkWidget *oclock_entry = gtk_entry_new_with_buffer(oclock_buf);
	gtk_grid_attach_next_to ((GtkGrid *)grid, oclock_entry, label4, GTK_POS_RIGHT, 10, 10);

	GtkWidget *label5 = gtk_label_new ("o'clock");
	gtk_grid_attach_next_to ((GtkGrid *)grid, label5, oclock_entry, GTK_POS_RIGHT, 10, 10);

	//init
	memset (g_day, 0, sizeof(g_day));
	memset (&g_pk, 0 , sizeof(struct page_keyword));
	FILE *fp = fopen(CONFIG_PATH,"r");
	if (fp) {
		char utmps[1024];
		if (fgets(utmps, sizeof(utmps), fp)) {
			strtok(utmps, "\r\n");
			g_pk.url = strdup(utmps);
			gtk_entry_buffer_set_text (url_buf, utmps, strlen(utmps));
			if (fgets(utmps, sizeof(utmps), fp)) {
				strtok(utmps, "\r\n");
				g_pk.keyword = strdup(utmps);
				gtk_entry_buffer_set_text (keyword_buf, utmps, strlen(utmps));
				if (fgets(utmps, sizeof(utmps), fp)) {
					strtok(utmps, "\r\n");
					g_pk.oclock = atoi(utmps);
					gtk_entry_buffer_set_text (oclock_buf, utmps, strlen(utmps));
					if (fgets(utmps, sizeof(utmps), fp)) {
						strtok(utmps, "\r\n");
						g_pk.postdata = strdup(utmps);
						gtk_entry_buffer_set_text (postdata_buf, utmps, strlen(utmps));
					}
				}
			}
		}
		fclose (fp);
	}
	SetTimer (NULL, 1000, 10000, page_keyword_timer);

	gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
	GtkApplication *app;
	int status;

	app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	return status;
}
