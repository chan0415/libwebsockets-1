/*
 * lws-minimal-http-server-form-get
 *
 * Copyright (C) 2018 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates a minimal http server that performs a form GET with a couple
 * of parameters.  It dumps the parameters to the console log and redirects
 * to another page.
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

static int interrupted;

static const char * param_names[] = {
	"text1",
	"send"
};

static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	      void *in, size_t len)
{
	uint8_t buf[LWS_PRE + 256], *start = &buf[LWS_PRE], *p = start,
		*end = &buf[sizeof(buf) - 1];
	const char *val;
	int n;

	switch (reason) {
	case LWS_CALLBACK_HTTP:

		if (!lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI))
			/* not a GET */
			break;
		if (strcmp((const char *)in, "/form1"))
			/* not our form URL */
			break;

		/* we just dump the decoded things to the log */

		for (n = 0; n < (int)ARRAY_SIZE(param_names); n++) {
			val = lws_get_urlarg_by_name(wsi, param_names[n],
					(char *)buf, sizeof(buf));
			if (!val)
				lwsl_user("%s: undefined\n", param_names[n]);
			else
				lwsl_user("%s: (len %d) '%s'\n", param_names[n],
					  (int)strlen((const char *)buf),buf);
		}

		/*
		 * Our response is to redirect to a static page.  We could
		 * have generated a dynamic html page here instead.
		 */

		if (lws_http_redirect(wsi, HTTP_STATUS_MOVED_PERMANENTLY,
				      (unsigned char *)"after-form1.html",
				      16, &p, end))
			return -1;

		/* we could add more headers here */

		if (lws_finalize_http_header(wsi, &p, end))
			return -1;

		n = lws_write(wsi, start, lws_ptr_diff(p, start),
			      LWS_WRITE_HTTP_HEADERS |
			      LWS_WRITE_H2_STREAM_END);
		if (n < 0)
			return -1;

		break;

	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static struct lws_protocols protocols[] = {
	{ "http", callback_http, 0, 0 },
	{ NULL, NULL, 0, 0 } /* terminator */
};

/* default mount serves the URL space from ./mount-origin */

static const struct lws_http_mount mount = {
	/* .mount_next */	       NULL,		/* linked-list "next" */
	/* .mountpoint */		"/",		/* mountpoint URL */
	/* .origin */		"./mount-origin",	/* serve from dir */
	/* .def */			"index.html",	/* default filename */
	/* .protocol */			NULL,
	/* .cgienv */			NULL,
	/* .extra_mimetypes */		NULL,
	/* .interpret */		NULL,
	/* .cgi_timeout */		0,
	/* .cache_max_age */		0,
	/* .auth_mask */		0,
	/* .cache_reusable */		0,
	/* .cache_revalidate */		0,
	/* .cache_intermediaries */	0,
	/* .origin_protocol */		LWSMPRO_FILE,	/* files in a dir */
	/* .mountpoint_len */		1,		/* char count */
	/* .basic_auth_login_file */	NULL,
};

void sigint_handler(int sig)
{
	interrupted = 1;
}

int main(int argc, char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	int n = 0;

	signal(SIGINT, sigint_handler);

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	info.port = 7681;
	info.protocols = protocols;
	info.mounts = &mount;

	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */, NULL);

	lwsl_user("LWS minimal http server GET | visit http://localhost:7681\n");

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}

	while (n >= 0 && !interrupted)
		n = lws_service(context, 1000);

	lws_context_destroy(context);

	return 0;
}
