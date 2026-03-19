#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"
#include <time.h>
#include <sys/time.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include "nvs_flash.h"
#include "esp_eth.h"
#include "esp_vfs.h"

#include "web_server.h"

#define WEB_SERVER_HTTP_QUERY_KEY_MAX_LEN  (64)

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

struct file_server_data
{
  /* Base path of file storage */
  char base_path[ESP_VFS_PATH_MAX + 1];

  /* Scratch buffer for temporary storage during file transfer */
  char scratch[SCRATCH_BUFSIZE];
};


static const char* TAG = "web_server";
static httpd_handle_t _server = NULL;
static struct file_server_data *server_data = NULL;

static esp_err_t
set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
  const char *type = "text/plain";
  if (CHECK_FILE_EXTENSION(filepath, ".html")) {
    type = "text/html";
  } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
    type = "application/javascript";
  } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
    type = "text/css";
  } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
    type = "image/png";
  } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
    type = "image/x-icon";
  } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
    type = "image/svg+xml";
  }
  return httpd_resp_set_type(req, type);
}

static esp_err_t
http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
  if (strcmp("/hello", req->uri) == 0)
  {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
    /* Return ESP_OK to keep underlying socket open */
    return ESP_OK;
  }
  else if (strcmp("/echo", req->uri) == 0)
  {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
    /* Return ESP_FAIL to close underlying socket */
    return ESP_FAIL;
  }
  /* For any other URI send 404 and close socket */
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
  return ESP_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP GET Handler for /hello
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t
hello_get_handler(httpd_req_t *req)
{
  char*  buf;
  size_t buf_len;

  /* Get header value string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
    /* Copy null terminated value string into buffer */
    if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found header => Host: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
    if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
    if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
    }
    free(buf);
  }

  /* Read URL query string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      ESP_LOGI(TAG, "Found URL query => %s", buf);
      char param[WEB_SERVER_HTTP_QUERY_KEY_MAX_LEN], dec_param[WEB_SERVER_HTTP_QUERY_KEY_MAX_LEN] = {0};

      /* Get value of expected key from query string */
      if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
        example_uri_decode(dec_param, param, strnlen(param, WEB_SERVER_HTTP_QUERY_KEY_MAX_LEN));
        ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
      }
      if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
        example_uri_decode(dec_param, param, strnlen(param, WEB_SERVER_HTTP_QUERY_KEY_MAX_LEN));
        ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
      }
      if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK)
      {
        ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
        example_uri_decode(dec_param, param, strnlen(param, WEB_SERVER_HTTP_QUERY_KEY_MAX_LEN));
        ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
      }
    }
    free(buf);
  }

  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
  httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

  /* Send response with custom headers and body set as the
   * string passed in user context*/
  const char* resp_str = (const char*) req->user_ctx;
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  /* After sending the HTTP response the old HTTP request
   * headers are lost. Check if HTTP request headers can be read now. */
  if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    ESP_LOGI(TAG, "Request headers lost");
  }
  return ESP_OK;
}

static const httpd_uri_t hello = 
{
  .uri       = "/hello",
  .method    = HTTP_GET,
  .handler   = hello_get_handler,
  .user_ctx  = "Hello World!"
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP POST Handler for /echo
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t
echo_post_handler(httpd_req_t *req)
{
  char buf[100];
  int ret, remaining = req->content_len;

  while (remaining > 0)
  {
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
            MIN(remaining, sizeof(buf)))) <= 0) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        /* Retry receiving if timeout occurred */
        continue;
      }
      return ESP_FAIL;
    }

    /* Send back the same data */
    httpd_resp_send_chunk(req, buf, ret);
    remaining -= ret;

    /* Log data received */
    ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG, "%.*s", ret, buf);
    ESP_LOGI(TAG, "====================================");
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t echo =
{
  .uri       = "/echo",
  .method    = HTTP_POST,
  .handler   = echo_post_handler,
  .user_ctx  = NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP PUT Handler for /ctrl
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t
ctrl_put_handler(httpd_req_t *req)
{
  char buf;
  int ret;

  if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
      httpd_resp_send_408(req);
    }
    return ESP_FAIL;
  }

  if (buf == '0')
  {
    /* URI handlers can be unregistered using the uri string */
    ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
    httpd_unregister_uri(req->handle, "/hello");
    httpd_unregister_uri(req->handle, "/echo");
    /* Register the custom error handler */
    httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
  }
  else
  {
    ESP_LOGI(TAG, "Registering /hello and /echo URIs");
    httpd_register_uri_handler(req->handle, &hello);
    httpd_register_uri_handler(req->handle, &echo);
    /* Unregister custom error handler */
    httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
  }

  /* Respond with empty body */
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t ctrl =
{
  .uri       = "/ctrl",
  .method    = HTTP_PUT,
  .handler   = ctrl_put_handler,
  .user_ctx  = NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP ANY Handler for /any
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t
any_handler(httpd_req_t *req)
{
  /* Send response with body set as the
   * string passed in user context*/
  const char* resp_str = (const char*) req->user_ctx;
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  // End response
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t any =
{
  .uri       = "/any",
  .method    = HTTP_ANY,
  .handler   = any_handler,
  /* Let's pass response string in user
   * context to demonstrate it's usage */
  .user_ctx  = "Hello World!"
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP SSE Handler for /sse
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XXX
// THIS implementation is quite problematic and controversial
// because it doesn't relinquish web server task's control
// by not returning to the caller.
// let's see how we will be able to take advantage of THIS
//
static esp_err_t
sse_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/event-stream");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  httpd_resp_set_hdr(req, "Connection", "keep-alive");

  char sse_data[64];
  while (1)
  {
    struct timeval tv;
    gettimeofday(&tv, NULL); // Get the current time
    int64_t time_since_boot = tv.tv_sec; // Time since boot in seconds
    esp_err_t err;
    int len = snprintf(sse_data, sizeof(sse_data), "data: Time since boot: %" PRIi64 " seconds\n\n", time_since_boot);
    if ((err = httpd_resp_send_chunk(req, sse_data, len)) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to send sse data (returned %02X)", err);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Send data every second
  }

  httpd_resp_send_chunk(req, NULL, 0); // End response
  return ESP_OK;
}

static const httpd_uri_t sse = {
  .uri       = "/sse",
  .method    = HTTP_GET,
  .handler   = sse_handler,
  .user_ctx  = NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP GET default Handler for file serving
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t
default_get_handler(httpd_req_t *req)
{
  char filepath[FILE_PATH_MAX];

  struct file_server_data *s_data = (struct file_server_data *)req->user_ctx;

  strlcpy(filepath, s_data->base_path, sizeof(filepath));

  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  } else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  set_content_type_from_file(req, filepath);

  ESP_LOGI(TAG, "static file request: %s", filepath);

  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    //
    // append ".gz" for compressed files
    //
    strlcat(filepath, ".gz", sizeof(filepath));
    fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
      ESP_LOGE(TAG, "Failed to open file : %s", filepath);
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
      return ESP_FAIL;
    }
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  }

  char *chunk = s_data->scratch;
  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
    if (read_bytes == -1) {
      ESP_LOGE(TAG, "Failed to read file : %s", filepath);
    } else if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(TAG, "File sending failed : %s", filepath);
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(TAG, "File sending complete: %s", filepath);
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;

}

static httpd_uri_t common_get_uri = {
  .uri = "/*",
  .method = HTTP_GET,
  .handler = default_get_handler,
  .user_ctx = NULL,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HTTP Web Socket Handler for Realtime IMU data
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t
ws_imu_handler(httpd_req_t *req)
{
  if (req->method == HTTP_GET)
  {
    // This is the handshake; return ESP_OK to switch protocols
    ESP_LOGI("WS", "Handshake done, client connected");

    /*
     if it takes a long time even after setting short keep-alive
     during http server initialization.

     #include <lwip/sockets.h>

     int keep_idle = 5;    // 5s idle
     int keep_intvl = 2;   // 2s interval
     int keep_cnt = 3;     // 3 fails
     int keep_alive = 1;   // Enable

     setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(int));
     setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(int));
     setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_intvl, sizeof(int));
     setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(int));
     */
      
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  uint8_t *buf = NULL;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  // First call to get the frame length
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK)
  {
    return ret;
  }

  if (ws_pkt.len > 0)
  {
    buf = calloc(1, ws_pkt.len + 1);
    ws_pkt.payload = buf;

    // Second call to actually receive data
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret == ESP_OK)
    {
      ESP_LOGI("WS", "Received packet: %s", ws_pkt.payload);
    }
    free(buf);
  }
  return ret;
}

void
ws_broadcast_imu_update(imu_telemetry_pkt_t* pkt)
{
  size_t clients = 8; // Max clients to check
  int fds[8];

  // Get list of all secure/active file descriptors
  if (httpd_get_client_list(_server, &clients, fds) == ESP_OK)
  {
    for (size_t i = 0; i < clients; i++)
    {
      // Only send if the descriptor is actually a WebSocket
      if (httpd_ws_get_fd_info(_server, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
      {
        httpd_ws_frame_t packet = 
        {
          .payload = (uint8_t *)pkt,
          .len = sizeof(imu_telemetry_pkt_t),
          .type = HTTPD_WS_TYPE_BINARY,
          .final = true
        };
        esp_err_t ret = httpd_ws_send_frame_async(_server, fds[i], &packet);
        
        if (ret != ESP_OK)
        {
            // The socket is dead or the buffer is a disaster.
            // HARDCORE MOVE: Kill the socket manually.
            // This stops the 'Error 11' logs immediately.
            ESP_LOGW(TAG, "Killing Ghost Client FD: %d", fds[i]);
            httpd_sess_trigger_close(_server, fds[i]); 
        }
      }
    }
  }
}

static httpd_uri_t ws_imu_uri = {
  .uri = "/ws_imu",
  .method = HTTP_GET,
  .handler = ws_imu_handler,
  .user_ctx = NULL,
  .is_websocket = true,
};

void
web_server_init(void)
{
  /* Allocate memory for server data */
  server_data = calloc(1, sizeof(struct file_server_data));
  if (!server_data)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for server data");
    return;
  }
  strlcpy(server_data->base_path, "/spiffs", sizeof(server_data->base_path));

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.keep_alive_enable = true;
  config.keep_alive_idle = 5;       // How long to wait of total silence before probing (Seconds)
  config.keep_alive_interval = 2;   // Interval between individual probes (Seconds)
  config.keep_alive_count = 3;      // How many failed probes before killing the socket

  config.max_uri_handlers = 32;
  config.max_open_sockets = 20;
  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&_server, &config) == ESP_OK)
  {
    common_get_uri.user_ctx = server_data;

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(_server, &hello);
    httpd_register_uri_handler(_server, &echo);
    httpd_register_uri_handler(_server, &ctrl);
    httpd_register_uri_handler(_server, &any);
    httpd_register_uri_handler(_server, &sse);
    httpd_register_uri_handler(_server, &ws_imu_uri);
    httpd_register_uri_handler(_server, &common_get_uri);
  }
}
