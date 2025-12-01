
/*
  WebServer.cpp -  web server functions class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../Grbl.h"

#if defined(ENABLE_WIFI) && defined(ENABLE_HTTP)

#    include "WifiServices.h"

#    include "ESPResponse.h"
#    include "Serial2Socket.h"
#    include "WebServer.h"
#    include <WebSocketsServer.h>
#    include <WiFi.h>
#    include <FS.h>
#    include <SPIFFS.h>
#    ifdef ENABLE_SD_CARD
#        include <SD.h>
#        include "../SDCard.h"
#include "dirent.h"
#    endif
#    include <WebServer.h>
#    include <ESP32SSDP.h>
#    include <StreamString.h>
#    include <Update.h>
#    include <esp_wifi_types.h>
#    ifdef ENABLE_MDNS
#        include <ESPmDNS.h>
#    endif
#    ifdef ENABLE_SSDP
#        include <ESP32SSDP.h>
#    endif
#    ifdef ENABLE_CAPTIVE_PORTAL
#        include <DNSServer.h>

namespace WebUI {
    const byte DNS_PORT = 53;
    DNSServer  dnsServer;
}

#    endif
#    include <esp_ota_ops.h>

//embedded response file if no files on SPIFFS
#    include "NoFile.h"

static const char* TAG = "WebServer.cpp";

//namespace WebUI {
    //Default 404
   static const char PAGE_404[] =
        "<HTML>\n<HEAD>\n<title>Redirecting...</title> \n</HEAD>\n<BODY>\n<CENTER>Unknown page : $QUERY$- you will be "
        "redirected...\n<BR><BR>\nif not redirected, <a href='http://$WEB_ADDRESS$'>click here</a>\n<BR><BR>\n<PROGRESS name='prg' "
        "id='prg'></PROGRESS>\n\n<script>\nvar i = 0; \nvar x = document.getElementById(\"prg\"); \nx.max=5; \nvar "
        "interval=setInterval(function(){\ni=i+1; \nvar x = document.getElementById(\"prg\"); \nx.value=i; \nif (i>5) "
        "\n{\nclearInterval(interval);\nwindow.location.href='/';\n}\n},1000);\n</script>\n</CENTER>\n</BODY>\n</HTML>\n\n";
   static const char PAGE_CAPTIVE[] =
        "<HTML>\n<HEAD>\n<title>Captive Portal</title> \n</HEAD>\n<BODY>\n<CENTER>Captive Portal page : $QUERY$- you will be "
        "redirected...\n<BR><BR>\nif not redirected, <a href='http://$WEB_ADDRESS$'>click here</a>\n<BR><BR>\n<PROGRESS name='prg' "
        "id='prg'></PROGRESS>\n\n<script>\nvar i = 0; \nvar x = document.getElementById(\"prg\"); \nx.max=5; \nvar "
        "interval=setInterval(function(){\ni=i+1; \nvar x = document.getElementById(\"prg\"); \nx.value=i; \nif (i>5) "
        "\n{\nclearInterval(interval);\nwindow.location.href='/';\n}\n},1000);\n</script>\n</CENTER>\n</BODY>\n</HTML>\n\n";

    bool process_command(const char* cmd) {
        char scmd[64]; 
        int cmd_len = strlen(cmd);
        int start_index = 0;
        bool silent = false;
        bool hasError = false;
        
        for (int i = 0; i <= cmd_len; i++) {
            if (cmd[i] == '\n' || cmd[i] == '\0') {
                int scmd_len = i - start_index;
                strncpy(scmd, &cmd[start_index], scmd_len);
                scmd[scmd_len] = '\0';  // Null terminate the string

                if (!silent && scmd_len == 2 && scmd[0] == 0xC2) {
                    scmd[0] = scmd[1];
                    scmd[1] = '\0';
                }

                if (scmd_len > 1) {
                    strcat(scmd, "\n");
                } else if (is_realtime_command(scmd[0])) {
                    ESP_LOGW(TAG, "[realtime command]Receive: %s", scmd);
                    execute_realtime_command((Cmd)scmd[0], CLIENT_WEBUI);
                    start_index = i + 1;
                    hasError = true;
                    continue;
                } else {
                    strcat(scmd, "\n");
                }

                ESP_LOGW(TAG, "Receive to Serial2Socket: %s", scmd);
                ESP_LOG_BUFFER_HEX(TAG, scmd, strlen(scmd));

                if (!WebUI::Serial2Socket.push(scmd)) {
                    hasError = true;
                }

                start_index = i + 1;
            }
        }
        return hasError;
    }

    void url_decode(char* str) {
        char *pstr = str;
        char hex[3] = {0}; 
        while (*pstr) {
            if (*pstr == '%' && *(pstr + 1) && *(pstr + 2)) {
                hex[0] = *(pstr + 1);
                hex[1] = *(pstr + 2);
                *pstr = (char)strtol(hex, NULL, 16);  
                memmove(pstr + 1, pstr + 3, strlen(pstr + 3) + 1); 
            } else if (*pstr == '+') {
                *pstr = ' ';
                pstr++;
            } else {
                pstr++;
            }
        }
    }

    void replace_double_slashes(char *str) {
        char *read_ptr = str;  
        char *write_ptr = str; 
        
        while (*read_ptr != '\0') {
            if (*read_ptr == '/' && *(read_ptr + 1) == '/') {
                
                *write_ptr = *read_ptr;  
                write_ptr++;            
                read_ptr++;              
            } else {
                
                *write_ptr = *read_ptr;
                write_ptr++; 
            }
            read_ptr++;     
        }
        
        *write_ptr = '\0';
    }
Error delete_files(const char *path) {
    // Check if file exists
    struct stat st;
    ESP_LOGE(TAG, "delete_files: %d", sizeof(struct stat));
    if (stat(path, &st) == 0) {
        //ESP_LOGI(TAG, "File %s exists", path);
        //return ESP_OK;
    } else {
        ESP_LOGE(TAG, "File %s does not exist", path);
        return Error::FsFileNotFound;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            ESP_LOGE(TAG, "Failed to open directory: %s", path);
            return Error::FsFailedOpenFile;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            size_t path_len = strlen(path) + strlen(entry->d_name) + 2;
            char *entryPath = (char *)malloc(path_len);
            if (entryPath == NULL) {
                ESP_LOGE(TAG, "Memory allocation failed for entryPath");
                closedir(dir);
                return Error::FsFailedDelFile;
            }
            snprintf(entryPath, path_len, "%s/%s", path, entry->d_name);

            if (delete_files(entryPath) != Error::Ok) {
                free(entryPath);
                closedir(dir);
                return Error::FsFailedDelFile;
            }

            free(entryPath);
        }
        closedir(dir);

        if (rmdir(path) != 0) {
            return Error::FsFailedDelDir;
        }
    } else {
        if (SD.remove(path)) {
            ESP_LOGE(TAG, "Failed to delete file: %s", path);
            return Error::FsFailedDelFile;
        }
    }
    return Error::Ok;
}
    static esp_err_t update_upload_handler(httpd_req_t *req) {
        #define UPDATE_UPLOAD_BUFFER_SIZE 2048
        esp_ota_handle_t update_handle = 0;
        const esp_partition_t *update_partition = NULL;
        char*  buf = (char *)calloc(1, UPDATE_UPLOAD_BUFFER_SIZE);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", req->content_len);
            httpd_resp_send_500(req);
            return ESP_ERR_NO_MEM;
        }
        int received;
        int remaining = req->content_len;

        int total_received = 0;
        int last_percent = -1;
        int current_percent = 0;

        if ((received = httpd_req_recv(req, buf, MIN(remaining, UPDATE_UPLOAD_BUFFER_SIZE))) <= 0) {
            free(buf);
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }
        UpdateFirmwareFileInfo updateFileInfo;
        const char *currentPos = buf;
        const char *str = "Content-Disposition: form-data; name=";
        uint8_t strLen = strlen(str);
        uint64_t start_time = esp_timer_get_time();
        if ((currentPos = strcasestr(currentPos, str)) != NULL) {
            currentPos += strLen;
            char temp[32];
            if (sscanf(currentPos,"\"%[^\"]\"\r\n\r\n%[^\r\n]", temp,updateFileInfo.path) == 2) {
                if (strcmp(temp, "path") != 0) {
                    free(buf);
                    ESP_LOGE(TAG, "Invalid Content-Disposition header");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                grbl_send(CLIENT_ALL, "[MSG:Invalid parameter]\r\n");
                ESP_LOGE(TAG, "Failed to parse Content-Disposition header");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            }
        } else {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
            return ESP_ERR_INVALID_ARG;
        }
        if ((currentPos = strcasestr(currentPos, str)) != NULL) {
            currentPos += strLen;
            char temp[32];
            if (sscanf(currentPos, "\"%[^\"]\"\r\n\r\n%u", temp,&updateFileInfo.totalSize) == 2) {
                if (strcmp(temp, "firmware.binS") != 0) {
                    free(buf);
                    ESP_LOGE(TAG, "Invalid Content-Disposition header");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                grbl_send(CLIENT_ALL, "[MSG:Invalid parameter]\r\n");
                ESP_LOGE(TAG, "Failed to parse Content-Disposition header");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            }
        } else {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
            return ESP_ERR_INVALID_ARG;
        }
        if ((currentPos = strcasestr(currentPos, str)) != NULL) {
            currentPos += strLen;
            char temp[32];
            if (sscanf(currentPos, "\"%[^\"]\"; filename=\"%[^\"]\"", temp, updateFileInfo.name) == 2) {
                if (strcmp(temp, "file") != 0) {
                    free(buf);
                    ESP_LOGE(TAG, "Invalid Content-Disposition header");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
                if ((currentPos = strcasestr(currentPos, "\r\n\r\n")) != NULL) {
                    currentPos += strlen("\r\n\r\n");
                    if (OTA_update_init(&update_handle, &update_partition) != ESP_OK)
                     {
                        free(buf);
                        grbl_send(CLIENT_ALL, "[MSG:Upload cancelled]\r\n");
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA init failed");
                        return ESP_ERR_INVALID_ARG;
                    }

                    if (updateFileInfo.totalSize >= update_partition->size) {
                        free(buf);
                        grbl_send(CLIENT_ALL, "[MSG:Upload rejected, not enough space]\r\n");
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload rejected, not enough space");
                        return ESP_ERR_INVALID_SIZE;
                    }
                     int first_chunk_size = received - (currentPos - buf);
                    if (esp_ota_write(update_handle, (const void *)currentPos, received - (currentPos - buf)) != ESP_OK) {
                        free(buf);
                        OTA_update_end(update_handle, update_partition);
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File write failed");
                        return ESP_FAIL;
                    }
                    total_received += first_chunk_size;
                    current_percent = (total_received * 100) / updateFileInfo.totalSize;
                    if (current_percent != last_percent) {
                        grbl_sendf(CLIENT_ALL, "[MSG:Upload progress: %d%%]\r\n", current_percent);
                        last_percent = current_percent;
                    }
                }
            } else {
                free(buf);
                grbl_send(CLIENT_ALL, "[MSG:Invalid parameter]\r\n");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            }
        } else {
            free(buf);
            grbl_send(CLIENT_ALL, "[MSG:Invalid parameter]\r\n");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
            return ESP_ERR_INVALID_ARG;
        }
        remaining -= received;
        while (remaining > 0) {
            if ((received = httpd_req_recv(req, buf, MIN(remaining, UPDATE_UPLOAD_BUFFER_SIZE))) <= 0) {
                if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                    continue;
                }
                free(buf);
                OTA_update_end(update_handle, update_partition);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
                return ESP_FAIL;
            }

            if (esp_ota_write(update_handle, (const void *)buf, received) != ESP_OK) {
                free(buf);
                OTA_update_end(update_handle, update_partition);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File write failed");
                return ESP_FAIL;
            }
            remaining -= received;
            total_received += received;
            current_percent = (total_received * 100) / updateFileInfo.totalSize;
         //   if (current_percent != last_percent && (current_percent % 5 == 0 || current_percent == 100)) {
                grbl_sendf(CLIENT_ALL, "[MSG:Upload progress: %d%%]\r\n", current_percent);
                last_percent = current_percent;
          //  }
        }
        free(buf);
        OTA_update_end(update_handle, update_partition);
        grbl_send(CLIENT_ALL, "Firmware update successful!");
        httpd_resp_send(req, "Upload end, wait reboot", HTTPD_RESP_USE_STRLEN);
        WebUI::COMMANDS::wait(1000);
        WebUI::COMMANDS::restart_ESP();
        return ESP_OK;
    }
    static esp_err_t web_command_handler(httpd_req_t *req) {
        WebUI::AuthenticationLevel auth_level = WebUI::AuthenticationLevel::LEVEL_ADMIN;
        char command_str[255];
        uint64_t start_time = esp_timer_get_time();
        httpd_resp_set_hdr(req, "Connection", "close");
        size_t len = httpd_req_get_url_query_len(req);
        if (len == 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid command");
            return ESP_ERR_INVALID_ARG;
        }
        
        char *parametr = (char *)malloc(len + 1);
        if (parametr == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
            return ESP_ERR_NO_MEM;
        }
        if (httpd_req_get_url_query_str(req, parametr, len + 1) == ESP_OK) {
            url_decode(parametr);

            if (httpd_query_key_value(parametr, "plain", command_str, sizeof(command_str)) == ESP_OK) {
            } else if (httpd_query_key_value(parametr, "commandText", command_str, sizeof(command_str)) == ESP_OK) {
            } else {
                free(parametr);
                const char *err = "Invalid command";
                httpd_resp_send(req, err, strlen(err));
                return ESP_ERR_INVALID_ARG;
            }
        } else {
            free(parametr);
            const char *err = "Invalid command";
            httpd_resp_send(req, err, strlen(err));
            return ESP_ERR_INVALID_ARG;
        }
        if (strcasestr(command_str, "[ESP") != NULL) {
            char line[256];
            strncpy(line, command_str, 255);
            WebUI::ESPResponseStream* espresponse = new WebUI::ESPResponseStream(CLIENT_WEBUI, req);
            if (espresponse == NULL) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
                free(parametr);
                return ESP_ERR_NO_MEM;
            }

            Error              err         = system_execute_line(line, espresponse, auth_level);
            String             answer;
            if (err == Error::Ok) {
                answer = "ok";
            } else {
                const char* msg = errorString(err);
                answer          = "Error: ";
                if (msg) {
                    answer += msg;
                } else {
                    answer += static_cast<int>(err);
                }
            }
            if (!espresponse->anyOutput()) {
                httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
                httpd_resp_send(req, answer.c_str(), HTTPD_RESP_USE_STRLEN);
            } else {
                espresponse->flush();
            }
            delete(espresponse);
        } else {
            ESP_LOG_BUFFER_HEX(TAG, command_str, strlen(command_str));
            bool hasError = process_command(command_str);

            httpd_resp_send(req, hasError? "Error": "", HTTPD_RESP_USE_STRLEN);
        }

        free(parametr);
        uint64_t end_time = esp_timer_get_time();
        return ESP_OK;
    }
    
    static esp_err_t root_handler(httpd_req_t *req)
    {
        extern const unsigned char file_start[] asm("_binary_index_html_gz_start");
        extern const unsigned char file_end[]   asm("_binary_index_html_gz_end");
        const size_t file_size = (file_end - file_start);

        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

        httpd_resp_send(req, (char *)file_start, file_size);
        return ESP_OK;
    }
    static esp_err_t icon_handler(httpd_req_t *req)
    {
        extern const unsigned char icon_start[] asm("_binary_favicon_ico_start");
        extern const unsigned char icon_end[]   asm("_binary_favicon_ico_end");
        const size_t icon_size = (icon_end - icon_start);

        httpd_resp_set_hdr(req, "Content-Type", "image/x-icon");
        httpd_resp_send(req, (char *)icon_start, icon_size);
        return ESP_OK;
    }
    static esp_err_t not_found_handle(httpd_req_t *req, httpd_err_code_t error)
    {
        if (error == HTTPD_404_NOT_FOUND) {
            String contentType = PAGE_404;
            String KEY_IP    = "$WEB_ADDRESS$";
            String KEY_QUERY = "$QUERY$"; 
            String stmp = (WiFi.getMode() == WIFI_STA) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();

            uint16_t port = WebUI::http_port->get();
            if (port != 80) {
                stmp += ":" + String(port);
            }
            
            contentType.replace(KEY_IP, stmp);
            contentType.replace(KEY_QUERY, req->uri);

            httpd_resp_set_hdr(req, "Content-Type", HTTPD_TYPE_TEXT);
            httpd_resp_send(req, contentType.c_str(), HTTPD_RESP_USE_STRLEN);
        }
        return ESP_OK;
    }
            //SD File upload with direct access to SD///////////////////////////////
    static esp_err_t SDFile_direct_upload_handler(httpd_req_t *req) {
        #define UPLOAD_FILE_BUFFER_SIZE 1024
        static File   sdUploadFile;
        if (req->method == HTTP_POST) {
            size_t free_heap_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
            //ESP_LOGI(TAG, "Free heap size: %d bytes", free_heap_size);
           char* buf = (char*)heap_caps_malloc(UPLOAD_FILE_BUFFER_SIZE, MALLOC_CAP_INTERNAL);
            if (!buf) {
                ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", req->content_len);
                httpd_resp_send_500(req);
                return ESP_ERR_NO_MEM;
            }

            int received=0, remaining = 0;
            size_t write_size = 0;

            int last_percent = -1;
            int current_percent = 0;
            int total_received = 0;

            //ESP_LOGI(TAG, "Upload file size : %d", remaining);
            String tempFileName;
            if ((received = httpd_req_recv(req, buf, MIN(req->content_len, UPLOAD_FILE_BUFFER_SIZE))) <= 0) {
                free(buf);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
                return ESP_FAIL;
            }
            UploadFileInfo uploadFileInfo;
            const char *currentPos = buf;
            const char *str = "content-disposition: form-data; name=";
            uint8_t strLen = strlen(str);
            uint64_t start_time = esp_timer_get_time();
            if ((currentPos = strcasestr(currentPos, str)) != NULL) {
                currentPos += strLen;
                char temp[32];
                if (sscanf(currentPos, "\"%[^\"]\"\r\n\r\n%[^\r\n]", temp, uploadFileInfo.path) == 2) {
                    if (strcmp(temp, "path") != 0) {
                        free(buf);
                        ESP_LOGE(TAG, "Invalid Content-Disposition header");
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                        return ESP_ERR_INVALID_ARG;
                    }
                } else {
                    free(buf);
                    ESP_LOGE(TAG, "Failed to parse Content-Disposition header");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            }

            if ((currentPos = strcasestr(currentPos, str)) != NULL) {
                currentPos += strLen;
                char temp[32];
                if (sscanf(currentPos, "\"%[^\"]\"\r\n\r\n%u",temp, &uploadFileInfo.totalSize) == 2) {
                    if (strcmp(temp, "size") != 0) {
                        free(buf);
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                        return ESP_ERR_INVALID_ARG;
                    }
                } else {
                    free(buf);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            } 
            if ((currentPos = strcasestr(currentPos, str)) != NULL) {
                currentPos += strLen;
                char temp[32];
                if (sscanf(currentPos, "\"%[^\"]\"; filename=\"%[^\"]\"", temp, uploadFileInfo.name) == 2) {
                    if (strcmp(temp, "file") != 0) {
                        free(buf);
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                        return ESP_ERR_INVALID_ARG;
                    }
                    if ((currentPos = strcasestr(currentPos, "\r\n\r\n")) != NULL) {
                        currentPos += strlen("\r\n\r\n");
                        if (get_sd_state(true) != SDState::Idle) {
                            grbl_send(CLIENT_ALL, "[MSG:Upload cancelled]\r\n");
                            if (get_sd_state(true) == SDState::NotPresent) {
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No SD card");
                            } else {
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SDCard busy");
                            }
                            free(buf);
                            return ESP_ERR_INVALID_ARG;
                        }
                        set_sd_state(SDState::BusyUploading);
                        String filename = uploadFileInfo.name;
                        if (filename[0] != '/') {
                            filename = "/" + filename;
                        }
                        tempFileName = filename;
                        if (SD.exists(filename)) {
                            SD.remove(filename);
                            if (tempFileName.length() > 0 && tempFileName.startsWith("/")) {
                                tempFileName = tempFileName.substring(1);
                            }
                        }
                        String sizeargname =filename + "S";
                        if (uploadFileInfo.totalSize > 0) {
                            remaining = uploadFileInfo.totalSize;
                            uint64_t freespace = SD.totalBytes() - SD.usedBytes();
                            if (uploadFileInfo.totalSize > freespace) {
                                free(buf);
                                grbl_send(CLIENT_ALL, "[MSG:Upload error]\r\n");
                                set_sd_state(SDState::Idle);
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload rejected, not enough space");
                                return ESP_ERR_INVALID_ARG;
                            }
                        }
                        //Create file for writing
                         size_t bytesWritten = 0;
                        sdUploadFile = SD.open(filename.c_str(), FILE_WRITE);
                        //check if creation succeed
                        if (!sdUploadFile) {
                            free(buf);
                            sdUploadFile.close();
                            grbl_send(CLIENT_ALL, "[MSG:Upload failed]\r\n");
                            set_sd_state(SDState::Idle);
                            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File creation failed");
                            return ESP_ERR_NO_MEM;
                        } else 
                        {
                            if (received - (currentPos - buf) > remaining) {
                                write_size = remaining;
                            } else {
                                write_size = received - (currentPos - buf);
                            }
                           bytesWritten=sdUploadFile.write((const uint8_t *)currentPos,write_size); 
                            if (write_size != bytesWritten) {
                                free(buf);
                                grbl_send(CLIENT_ALL, "[MSG:Upload error]\r\n");
                                sdUploadFile.close();
                                set_sd_state(SDState::Idle);
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload error");
                            }
                            remaining -= bytesWritten;

                            total_received += bytesWritten;
                            current_percent = (total_received * 100) / uploadFileInfo.totalSize;
                            if (current_percent > last_percent) {
                                grbl_sendf(CLIENT_ALL, "[MSG:Progress: %d%%]\r\n", current_percent);
                                last_percent = current_percent;
                            }
                        }  
                    }
                } else {
                    free(buf);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    set_sd_state(SDState::Idle);
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                set_sd_state(SDState::Idle);
                 return ESP_ERR_INVALID_ARG;
            }

            printf("path:%s, file:%s, size:%d", uploadFileInfo.path, uploadFileInfo.name, uploadFileInfo.totalSize);
           
            while (remaining > 0) {
                if ((received = httpd_req_recv(req, buf, MIN(remaining, UPLOAD_FILE_BUFFER_SIZE))) <= 0) {
                    if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                        continue;
                    }
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
                    sdUploadFile.close();
                    set_sd_state(SDState::Idle);
                    free(buf);
                    return ESP_FAIL;
                }
                size_t bytesWritten = 0;
                write_size = (received > remaining)? remaining: received;
                bytesWritten=sdUploadFile.write((const uint8_t *)buf, write_size);
                if (write_size != bytesWritten) {
                    grbl_send(CLIENT_ALL, "[MSG:SDCard write error]\r\n");
                    sdUploadFile.close();
                    set_sd_state(SDState::Idle);
                    free(buf);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SDCard write error");
                     return ESP_FAIL;
                }
                remaining -= bytesWritten;

                total_received += bytesWritten;
                current_percent = (total_received * 100) / uploadFileInfo.totalSize;
               // if ((current_percent > last_percent && (current_percent % 5 == 0 || current_percent == 100)) ) { 
                    grbl_sendf(CLIENT_ALL, "[MSG:Progress: %d%%]\r\n", current_percent);
                    last_percent = current_percent;
               // }
            }  
            free(buf);
            sdUploadFile.close();  
            if (tempFileName.length() > 0 && tempFileName.startsWith("/")) {
                tempFileName = tempFileName.substring(1);
            }
            printf("upload success, use time:%lldus\n", esp_timer_get_time() - start_time);
            set_sd_state(SDState::Idle);  
        } 
        char command_str[64];   
        bool     list_files = true;
        uint64_t totalspace = 0;
        uint64_t freespace  = 0;
        uint64_t usedspace  = 0;

        String sstatus = "Ok";
        httpd_resp_set_hdr(req, "Connection", "close");
        char *path = (char *)heap_caps_calloc(1, 256, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (path == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
            return ESP_ERR_NO_MEM;
        }
        *path = '/';
        size_t len = httpd_req_get_url_query_len(req);
        if (len == 0 && req->method != HTTP_POST) {
            const char *err = "Invalid command";
            httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
            httpd_resp_send(req, err, strlen(err));
            return ESP_ERR_INVALID_ARG;
        }

        char *parametr = (char *)heap_caps_malloc(len + 1, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (parametr == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
            return ESP_ERR_NO_MEM;
        } 
        SDState  state      = get_sd_state(true);
        if (state != SDState::Idle) {
            String status = "{\"status\":\"";
            status += state == SDState::NotPresent ? "No SD Card\"}" : "Busy\"}";
            httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, status.c_str(), status.length());
            return ESP_OK;
        }
        set_sd_state(SDState::BusyParsing);
        if (httpd_req_get_url_query_str(req, parametr, len + 1) == ESP_OK) {
            url_decode(parametr);
            if (httpd_query_key_value(parametr, "path", command_str, sizeof(command_str)) == ESP_OK) {
                strcpy(path + 1, command_str);
                replace_double_slashes(path);
                if (path[strlen(path) - 1] != '/') {
                    strcat(path, "/");
                }
            }
            if (httpd_query_key_value(parametr, "action", command_str, sizeof(command_str)) == ESP_OK) {
                if (strcmp(command_str, "delete") == 0 ) {
                    if (httpd_query_key_value(parametr, "filename", command_str, sizeof(command_str)) == ESP_OK) {
                        String filename;
                        String shortname = command_str;
                        filename         = path + shortname;
                        shortname.replace("/", "");
                        filename.replace("//", "/");
                        if (!SD.exists(filename)) {
                            sstatus = shortname + " does not exist!";
                        } else {
                            if (SD.remove(filename)) {
                                sstatus = shortname + " deleted";
                            } else {
                                sstatus = "Cannot deleted ";
                                sstatus += shortname;
                            }
                        }   
                    }
                } else if (strcmp(command_str, "deletedir") == 0) {
                    if (httpd_query_key_value(parametr, "filename", command_str, sizeof(command_str)) == ESP_OK) {
                        String filename;
                        String shortname = command_str;
                        shortname.replace("/", "");
                        filename = String(path) + "/" + shortname;
                        filename.replace("//", "/");
                        if (filename != "/") {
                            if (!SD.exists(filename)) {
                                sstatus = shortname + " does not exist!";
                            } else {
                                Error err = delete_files(filename.c_str());
                                if (err != Error::Ok) {
                                    sstatus = "Error deleting: ";
                                    sstatus += shortname;
                                } else {
                                    sstatus = shortname;
                                    sstatus += " deleted";
                                }
                            }
                        } else {
                            sstatus = "Cannot delete root";
                        }
                    }
                } else if (strcmp(command_str, "createdir") == 0) {
                    if (httpd_query_key_value(parametr, "filename", command_str, sizeof(command_str)) == ESP_OK) {
                        String filename;
                        String shortname = command_str;
                        filename         = path + shortname;
                        shortname.replace("/", "");
                        filename.replace("//", "/");
                        if (SD.exists(filename)) {
                            sstatus = shortname + " already exists!";
                        } else {
                            if (!SD.mkdir(filename)) {
                            sstatus = "Cannot create ";
                            sstatus += shortname;
                            } else {
                                sstatus = shortname + " created";
                            }
                        }
                    }
                }
            }
            if(httpd_query_key_value(parametr, "dontlist", command_str, sizeof(command_str)) == ESP_OK && strcmp(command_str, "yes")) {
                    list_files = false;
            }  
        }

        // TODO Settings - consider using the JSONEncoder class
        String jsonfile = "{";
        jsonfile += "\"files\":[";

        if (strcmp(path, "/") != 0) {
            size_t len = strlen(path);
            if (path[len - 1] == '/') {
                path[len - 1] = '\0';
            }
        }
        if(strcmp(path, "/") != 0 && !SD.exists(path)) {
            String s = "{\"status\":\" ";
            s += path;
            s += " does not exist on SD Card\"}";
            httpd_resp_set_hdr(req, "Cache-Control", HTTPD_TYPE_JSON);
            httpd_resp_send(req, s.c_str(), s.length());
           // _webserver->send(200, "application/json", s);
           // SD.end();
            set_sd_state(SDState::Idle);
            return ESP_OK;
        }
        String file_path ="/sd";
        if (list_files) {
            DIR *dir = opendir("/sd");
            if (dir) {
                //ESP_LOGI("SD", "Directory opened successfully");
                int i = 0;
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    WebUI::COMMANDS::wait(1);
                    struct stat entry_stat;
                    if (i > 0) {
                        jsonfile += ",";
                    }
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }
                    char entryPath[512];
                    snprintf(entryPath, sizeof(entryPath), "%s/%s", file_path.c_str(), entry->d_name);
                    if (stat(entryPath, &entry_stat) == -1) {
                        ESP_LOGE(TAG, "Failed to get stat for %s", file_path.c_str());
                        continue;
                    }
                    jsonfile += "{\"name\":\"";
                    String tmpname = entry->d_name;
                    int    pos     = tmpname.lastIndexOf("/");
                    tmpname        = tmpname.substring(pos + 1);
                    jsonfile += tmpname;
                    jsonfile += "\",\"shortname\":\"";  //No need here
                    jsonfile += tmpname;
                    jsonfile += "\",\"size\":\"";
                    jsonfile += S_ISDIR(entry_stat.st_mode)? "-1": WebUI::ESPResponseStream::formatBytes(entry_stat.st_size);
                    jsonfile += "\",\"datetime\":\"";
                    //TODO - can be done later
                    jsonfile += "\"}";
                    i++;
                }
                closedir(dir);
            }
        }
        jsonfile += "],\"path\":\"";
        jsonfile +=String(path) + "\",";
        jsonfile += "\"total\":\"";
        String stotalspace, susedspace;
        //SDCard are in GB or MB but no less
        totalspace  = SD.totalBytes();
        usedspace   = SD.usedBytes();
        stotalspace = WebUI::ESPResponseStream::formatBytes(totalspace);
        susedspace  = WebUI::ESPResponseStream::formatBytes(usedspace + 1);

        uint32_t occupedspace = 1;
        uint32_t usedspace2   = usedspace / (1024 * 1024);
        uint32_t totalspace2  = totalspace / (1024 * 1024);
        occupedspace          = (usedspace2 * 100) / totalspace2;
        //minimum if even one byte is used is 1%
        if (occupedspace <= 1) {
            occupedspace = 1;
        }
        if (totalspace) {
            jsonfile += stotalspace;
        } else {
            jsonfile += "-1";
        }
        jsonfile += "\",\"used\":\"";
        jsonfile += susedspace;
        jsonfile += "\",\"occupation\":\"";
        if (totalspace) {
            jsonfile += String(occupedspace);
        } else {
            jsonfile += "-1";
        }
        jsonfile += "\",";
        jsonfile += "\"mode\":\"direct\",";
        jsonfile += "\"status\":\"";
        jsonfile += sstatus + "\"";
        jsonfile += "}";
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
        //  Uart0.printf("JSON Output: %s\r\n", jsonfile.c_str());
        //ESP_LOGI(TAG, "JSON Output: %s", jsonfile.c_str());
        httpd_resp_set_hdr(req, "Content-Type", HTTPD_TYPE_JSON);
        httpd_resp_send(req, jsonfile.c_str(), jsonfile.length());
        set_sd_state(SDState::Idle);
        return ESP_OK;
    }
    
    uint16_t fileNum = 0;
    static esp_err_t handle_getsd_fileAndNum_handler(httpd_req_t *req) {
        #define UPLOAD_FILE_BUFFER_SIZE 2048
        static File   sdUploadFile;
        if (req->method == HTTP_POST) {
            size_t free_heap_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
           char* buf = (char*)heap_caps_malloc(UPLOAD_FILE_BUFFER_SIZE, MALLOC_CAP_INTERNAL);
            if (!buf) {
                ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", req->content_len);
                httpd_resp_send_500(req);
                return ESP_ERR_NO_MEM;
            }

            int received=0, remaining = 0;
            size_t write_size = 0;
            String tempFileName;
            if ((received = httpd_req_recv(req, buf, MIN(remaining, UPLOAD_FILE_BUFFER_SIZE))) <= 0) {
                free(buf);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
                return ESP_FAIL;
            }
            UploadFileInfo uploadFileInfo;
            const char *currentPos = buf;
            const char *str = "content-disposition: form-data; name=";
            uint8_t strLen = strlen(str);
            uint64_t start_time = esp_timer_get_time();
            if ((currentPos = strcasestr(currentPos, str)) != NULL) {
                currentPos += strLen;
                char temp[32];
                if (sscanf(currentPos, "\"%[^\"]\"\r\n\r\n%[^\r\n]", temp, uploadFileInfo.path) == 2) {
                    if (strcmp(temp, "path") != 0) {
                        free(buf);
                        ESP_LOGE(TAG, "Invalid Content-Disposition header");
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                        return ESP_ERR_INVALID_ARG;
                    }
                } else {
                    free(buf);
                    ESP_LOGE(TAG, "Failed to parse Content-Disposition header");
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            }

            if ((currentPos = strcasestr(currentPos, str)) != NULL) {
                currentPos += strLen;
                char temp[32];
                if (sscanf(currentPos, "\"%[^\"]\"\r\n\r\n%u",temp, &uploadFileInfo.totalSize) == 2) {
                    if (strcmp(temp, "size") != 0) {
                        free(buf);
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                        return ESP_ERR_INVALID_ARG;
                    }
                } else {
                    free(buf);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                return ESP_ERR_INVALID_ARG;
            } 
            if ((currentPos = strcasestr(currentPos, str)) != NULL) {
                currentPos += strLen;
                char temp[32];
                if (sscanf(currentPos, "\"%[^\"]\"; filename=\"%[^\"]\"", temp, uploadFileInfo.name) == 2) {
                    if (strcmp(temp, "file") != 0) {
                        free(buf);
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                        return ESP_ERR_INVALID_ARG;
                    }
                    if ((currentPos = strcasestr(currentPos, "\r\n\r\n")) != NULL) {
                        currentPos += strlen("\r\n\r\n");
                        if (get_sd_state(true) != SDState::Idle) {
                            grbl_send(CLIENT_ALL, "[MSG:Upload cancelled]\r\n");
                            if (get_sd_state(true) == SDState::NotPresent) {
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No SD card");
                            } else {
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SDCard busy");
                            }
                            free(buf);
                            return ESP_ERR_INVALID_ARG;
                        }
                        set_sd_state(SDState::BusyUploading);
                        String filename = uploadFileInfo.name;
                        if (filename[0] != '/') {
                            filename = "/" + filename;
                        }
                        if (SD.exists(filename)) {
                            SD.remove(filename);
                            if (filename.length() > 0 && filename.startsWith("/")) {
                                filename = filename.substring(1);
                            }
                        }
                        String sizeargname =filename + "S";
                        if (uploadFileInfo.totalSize > 0) {
                           remaining = uploadFileInfo.totalSize;
                            uint64_t freespace = SD.totalBytes() - SD.usedBytes();
                            if (uploadFileInfo.totalSize > freespace) {
                                free(buf);
                                grbl_send(CLIENT_ALL, "[MSG:Upload error]\r\n");
                                set_sd_state(SDState::Idle);
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload rejected, not enough space");
                                return ESP_ERR_INVALID_ARG;
                            }
                        }
                        //Create file for writing
                         size_t bytesWritten = 0;
                        sdUploadFile = SD.open(filename.c_str(), FILE_WRITE);
                        //check if creation succeed
                        if (!sdUploadFile) {
                            free(buf);
                            sdUploadFile.close();
                            grbl_send(CLIENT_ALL, "[MSG:Upload failed]\r\n");
                            set_sd_state(SDState::Idle);
                            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File creation failed");
                            return ESP_ERR_NO_MEM;
                        } else {
                            if (received - (currentPos - buf) > remaining) {
                                write_size = remaining;
                            } else {
                                write_size = received - (currentPos - buf);
                            }
                           bytesWritten=sdUploadFile.write((const uint8_t *)currentPos, write_size); 
                            if (write_size != bytesWritten) {
                                free(buf);
                                grbl_send(CLIENT_ALL, "[MSG:Upload error]\r\n");
                                sdUploadFile.close();
                                set_sd_state(SDState::Idle);
                                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload error");
                            }
                            remaining -= bytesWritten;
                        }  
                    }
                } else {
                    free(buf);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                    set_sd_state(SDState::Idle);
                    return ESP_ERR_INVALID_ARG;
                }
            } else {
                free(buf);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid parameter");
                set_sd_state(SDState::Idle);
                 return ESP_ERR_INVALID_ARG;
            }

            printf("path:%s, file:%s, size:%d, upload������\n", uploadFileInfo.path, uploadFileInfo.name, uploadFileInfo.totalSize);

            while (remaining > 0) {
                if ((received = httpd_req_recv(req, buf, MIN(remaining, UPLOAD_FILE_BUFFER_SIZE))) <= 0) {
                    if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                        continue;
                    }
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
                    sdUploadFile.close();
                    set_sd_state(SDState::Idle);
                    free(buf);
                    return ESP_FAIL;
                }
                size_t bytesWritten = 0;
                 write_size = (received > remaining)? remaining: received;
                bytesWritten=sdUploadFile.write((const uint8_t *)buf, write_size);
                if (write_size != bytesWritten) {
                    grbl_send(CLIENT_ALL, "[MSG:SDCard write error]\r\n");
                    sdUploadFile.close();
                    set_sd_state(SDState::Idle);
                     free(buf);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SDCard write error");
                      return ESP_FAIL;
                }
                remaining -= bytesWritten;
            }  
            free(buf);
            sdUploadFile.close();  
            printf("upload success, use time:%lldus\n", esp_timer_get_time() - start_time);
            set_sd_state(SDState::Idle);  
        }
       
        char command_str[64];   
        bool     list_files = true;
        uint64_t totalspace = 0;
        uint64_t freespace  = 0;
        uint64_t usedspace  = 0;

        String sstatus = "Ok";
        httpd_resp_set_hdr(req, "Connection", "close");
        char *path = (char *)heap_caps_calloc(1, 256, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (path == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
            return ESP_ERR_NO_MEM;
        }
        *path = '/';
        size_t len = httpd_req_get_url_query_len(req);
        if (len == 0 && req->method != HTTP_POST) {
            const char *err = "Invalid command";
            httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
            httpd_resp_send(req, err, strlen(err));
            return ESP_ERR_INVALID_ARG;
        }

        char *parametr = (char *)heap_caps_malloc(len + 1, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (parametr == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
            return ESP_ERR_NO_MEM;
        } 
        SDState  state      = get_sd_state(true);
        if (state != SDState::Idle) {
            String status = "{\"status\":\"";
            status += state == SDState::NotPresent ? "No SD Card\"}" : "Busy\"}";
            httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, status.c_str(), status.length());
            return ESP_OK;
        }
        set_sd_state(SDState::BusyParsing);
        if (httpd_req_get_url_query_str(req, parametr, len + 1) == ESP_OK) {
            url_decode(parametr);
            if (httpd_query_key_value(parametr, "path", command_str, sizeof(command_str)) == ESP_OK) {
                strcpy(path + 1, command_str);
                replace_double_slashes(path);
                if (path[strlen(path) - 1] != '/') {
                    strcat(path, "/");
                }
            }
            if (httpd_query_key_value(parametr, "action", command_str, sizeof(command_str)) == ESP_OK) {
                if (strcmp(command_str, "delete") == 0 ) {
                    if (httpd_query_key_value(parametr, "filename", command_str, sizeof(command_str)) == ESP_OK) {
                        String filename;
                        String shortname = command_str;
                        filename         = path + shortname;
                        shortname.replace("/", "");
                        filename.replace("//", "/");
                        if (!SD.exists(filename)) {
                            sstatus = shortname + " does not exist!";
                        } else {
                            if (SD.remove(filename)) {
                                sstatus = shortname + " deleted";
                            } else {
                                sstatus = "Cannot deleted ";
                                sstatus += shortname;
                            }
                        }   
                    }
                } else if (strcmp(command_str, "deletedir") == 0) {
                    if (httpd_query_key_value(parametr, "filename", command_str, sizeof(command_str)) == ESP_OK) {
                        String filename;
                        String shortname = command_str;
                        shortname.replace("/", "");
                        filename = String(path) + "/" + shortname;
                        filename.replace("//", "/");
                        if (filename != "/") {
                            if (!SD.exists(filename)) {
                                sstatus = shortname + " does not exist!";
                            } else {
                                Error err = delete_files(filename.c_str());
                                if (err != Error::Ok) {
                                    sstatus = "Error deleting: ";
                                    sstatus += shortname;
                                } else {
                                    sstatus = shortname;
                                    sstatus += " deleted";
                                }
                            }
                        } else {
                            sstatus = "Cannot delete root";
                        }
                    }
                } else if (strcmp(command_str, "createdir") == 0) {
                    if (httpd_query_key_value(parametr, "filename", command_str, sizeof(command_str)) == ESP_OK) {
                        String filename;
                        String shortname = command_str;
                        filename         = path + shortname;
                        shortname.replace("/", "");
                        filename.replace("//", "/");
                        if (SD.exists(filename)) {
                            sstatus = shortname + " already exists!";
                        } else {
                            if (!SD.mkdir(filename)) {
                            sstatus = "Cannot create ";
                            sstatus += shortname;
                            } else {
                                sstatus = shortname + " created";
                            }
                        }
                    }
                }
            }
            if(httpd_query_key_value(parametr, "dontlist", command_str, sizeof(command_str)) == ESP_OK && strcmp(command_str, "yes")) {
                    list_files = false;
            }  
        }

        // TODO Settings - consider using the JSONEncoder class
        String jsonfile = "{";
        jsonfile += "\"files\":[";

        if (strcmp(path, "/") != 0) {
            size_t len = strlen(path);
            if (path[len - 1] == '/') {
                path[len - 1] = '\0';
            }
        }
        if(strcmp(path, "/") != 0 && !SD.exists(path)) {
            String s = "{\"status\":\" ";
            s += path;
            s += " does not exist on SD Card\"}";
            httpd_resp_set_hdr(req, "Cache-Control", HTTPD_TYPE_JSON);
            httpd_resp_send(req, s.c_str(), s.length());
            set_sd_state(SDState::Idle);
            return ESP_OK;
        }

        if (list_files) {
             jsonfile = "{";
            jsonfile += "\"path\":\"";
            jsonfile += String(path) + "\",";
            jsonfile += "\"total\":\"";
            String stotalspace, susedspace;
            //SDCard are in GB or MB but no less
            totalspace  = SD.totalBytes();
            usedspace   = SD.usedBytes();
          //  stotalspace = ESPResponseStream::formatBytes(totalspace);
          //  susedspace  = ESPResponseStream::formatBytes(usedspace + 1);

            uint32_t occupedspace = 1;
          //  uint32_t usedspace2   = usedspace / (1024 * 1024);
          //  uint32_t totalspace2  = totalspace / (1024 * 1024);
          //  occupedspace          = (usedspace2 * 100) / totalspace2;
           occupedspace = (usedspace * 100) / totalspace;
            //minimum if even one byte is used is 1%
            if (occupedspace <= 1) {
                occupedspace = 1;
            }
            if (totalspace) {
                jsonfile += totalspace;
            } else {
                jsonfile += "-1";
            }
            jsonfile += "\",\"used\":\"";
            jsonfile += usedspace;
            jsonfile += "\",\"occupation\":\"";
            if (totalspace) {
                jsonfile += String(occupedspace);
            } else {
                jsonfile += "-1";
            }
            jsonfile += "\",";
            jsonfile += "\"mode\":\"direct\",";
            jsonfile += "\"status\":\"";
            jsonfile += sstatus + "\"";
            jsonfile += "}";
            httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
            httpd_resp_set_hdr(req, "Content-Type", HTTPD_TYPE_JSON);
            httpd_resp_send(req, jsonfile.c_str(), jsonfile.length());
            set_sd_state(SDState::Idle);
        }
        return ESP_OK;
    }
    static const httpd_uri_t basic_handlers[] = {
        { .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_handler,
        .user_ctx = NULL,
        },
        { .uri      = "/favicon.ico",
        .method   = HTTP_GET,
        .handler  = icon_handler,
        .user_ctx = NULL,
        },
        { .uri      = "/upload",
        .method   = HTTP_POST,
        .handler  = SDFile_direct_upload_handler,
        .user_ctx = NULL,
        },
        { .uri      = "/files",
        .method   = HTTP_GET,
        .handler  = SDFile_direct_upload_handler,
        .user_ctx = NULL,
        },
        { .uri      = "/getsd",
        .method   = HTTP_GET,
        .handler  = handle_getsd_fileAndNum_handler,
        .user_ctx = NULL,
        },
        { .uri      = "/command",
        .method   = HTTP_GET,
        .handler  = web_command_handler,
        .user_ctx = NULL,
        },
        { .uri      = "/updatefw",
        .method   = HTTP_POST,
        .handler  = update_upload_handler,
        .user_ctx = NULL,
        },
    };
    static const uint8_t basic_handlers_no = sizeof(basic_handlers)/sizeof(httpd_uri_t);

    static void register_basic_handlers(httpd_handle_t hd)
    {
        //ESP_LOGI(TAG, "Registering basic handlers");
        for (uint8_t i = 0; i < basic_handlers_no; i++) {
            if (httpd_register_uri_handler(hd, &basic_handlers[i]) != ESP_OK) {
                ESP_LOGW(TAG, "register uri failed for %d", i);
                return;
            }
        }
    }
    static httpd_handle_t start_webserver(void)
    {
        httpd_handle_t server = NULL;
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.stack_size = 5120;
        config.max_uri_handlers  = 8;
        config.server_port = 8848;
        // Start the httpd server
        
        if (httpd_start(&server, &config) == ESP_OK) {
            // Registering the ws handler
            register_basic_handlers(server);
            httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, not_found_handle);
            ESP_LOGW(TAG, "Started HTTP server on port: '%d'", config.server_port);
            ESP_LOGW(TAG, "Max URI handlers: '%d'", config.max_uri_handlers);
            ESP_LOGW(TAG, "Max Open Sessions: '%d'", config.max_open_sockets);
            ESP_LOGW(TAG, "Max Header Length: '%d'", HTTPD_MAX_REQ_HDR_LEN);
            ESP_LOGW(TAG, "Max URI Length: '%d'", HTTPD_MAX_URI_LEN);
            ESP_LOGW(TAG, "Max Stack Size: '%d'", config.stack_size);
            return server;
        }

        ESP_LOGE(TAG, "Error starting server!");
        return NULL;
    }
    esp_err_t stop_webserver(httpd_handle_t server)
    {
        if (server) {
            return httpd_stop(server);
        }
        return ESP_OK;
    }
 #    ifdef ENABLE_SSDP
    //http SSDP xml presentation
    static esp_err_t SSDP_handler(httpd_req_t *req) {
    StreamString sschema;
    if (sschema.reserve(1024)) {
        // 使用更安全的字符串构建方式
        String sip = WiFi.localIP().toString();
        uint32_t chipId = (uint16_t)(ESP.getEfuseMac() >> 32);
        
        char uuid[37];
        snprintf(uuid, sizeof(uuid),
                "38323636-4558-4dda-9188-cda0e6%02x%02x%02x",
                (uint16_t)((chipId >> 16) & 0xff),
                (uint16_t)(chipId & 0xff),
                (uint16_t)((chipId >> 8) & 0xff));
        
        String serialNumber = String(chipId);
        String hostname = WebUI::wifi_config.Hostname();
        
        // 构建 XML 响应 - 使用 StreamString 的 print 方法
        sschema.print("<?xml version=\"1.0\"?>");
        sschema.print("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">");
        sschema.print("<specVersion>");
        sschema.print("<major>1</major>");
        sschema.print("<minor>0</minor>");
        sschema.print("</specVersion>");
        sschema.print("<URLBase>http://");
        sschema.print(sip.c_str());
        sschema.print(":");
        sschema.print(WebUI::http_port->get());
        sschema.print("/</URLBase>");
        sschema.print("<device>");
        sschema.print("<deviceType>urn:device:BLGS</deviceType>");
        sschema.print("<Machine>");
        sschema.print(MACHINE_TYPE);
        sschema.print("</Machine>");
        sschema.print("<software>");
        sschema.print(VERSION);
        sschema.print("</software>");
        sschema.print("<Wavelength>");
        sschema.print(WAVELENGTH);
        sschema.print("</Wavelength>");
        sschema.print("<Power>");
        sschema.print(POWER);
        sschema.print("</Power>");
        
        // 工作空间
        char workspace[64];
        snprintf(workspace, sizeof(workspace), "XSIZE=%.2f,YSIZE=%.2f", 
                 DEFAULT_X_MAX_TRAVEL, DEFAULT_Y_MAX_TRAVEL);
        sschema.print("<WorkSpace>");
        sschema.print(workspace);
        sschema.print("</WorkSpace>");
        
        sschema.print("<friendlyName>");
        sschema.print(hostname.c_str());
        sschema.print("</friendlyName>");
        sschema.print("<presentationURL>/</presentationURL>");
        sschema.print("<serialNumber>");
        sschema.print(serialNumber.c_str());
        sschema.print("</serialNumber>");
        sschema.print("<modelName>ESP32</modelName>");
        sschema.print("<modelNumber>Marlin</modelNumber>");
        sschema.print("<modelURL>http://espressif.com/en/products/hardware/esp-wroom-32/overview</modelURL>");
        sschema.print("<manufacturer>Espressif Systems</manufacturer>");
        sschema.print("<manufacturerURL>http://espressif.com</manufacturerURL>");
        sschema.print("<UDN>uuid:");
        sschema.print(uuid);
        sschema.print("</UDN>");
        sschema.print("</device>");
        sschema.print("</root>\r\n\r\n");
        
        httpd_resp_set_hdr(req, "Content-Type", "text/xml");
        httpd_resp_send(req, sschema.c_str(), HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
    }
    return ESP_OK;
}
#    endif
    namespace WebUI {
        static const char *TAG = "WebServer";

        Web_Server        web_server;
        bool              Web_Server::_setupdone     = false;
        uint16_t          Web_Server::_port          = 0;
        long              Web_Server::_id_connection = 0;
        httpd_handle_t    Web_Server::_webserver     = NULL;
        WebSocketsServer* Web_Server::_socket_server = NULL;
        SSDPClass*        Web_Server::_ssdp_server   = NULL;

        Web_Server::Web_Server() {}
        Web_Server::~Web_Server() { end(); }

        long Web_Server::get_client_ID() { return _id_connection; }

        bool Web_Server::begin() {
            bool no_error = true;
            if (http_enable->get() == 0 || _setupdone) {
                ESP_LOGW(TAG, "WebServer already started or disabled");
                return false;
            }

            _port = http_port->get();
            _webserver = start_webserver();

            _socket_server = new WebSocketsServer(_port + 1);
            _socket_server->begin();
            _socket_server->onEvent(handle_Websocket_Event);

            //Websocket output
            Serial2Socket.attachWS(_socket_server);

    #    ifdef ENABLE_SSDP
            //SSDP service presentation
            if (WiFi.getMode() == WIFI_STA) {
                httpd_uri_t ssdp_handler = {
                    .uri      = "/description.xml",
                    .method   = HTTP_GET,
                    .handler  = SSDP_handler,
                    .user_ctx = NULL
                };
                httpd_register_uri_handler(_webserver, &ssdp_handler);
                //Add specific for SSDP
                _ssdp_server =  new SSDPClass();
                _ssdp_server->setSchemaURL("description.xml");
                _ssdp_server->setHTTPPort(_port);
                _ssdp_server->setName(wifi_config.Hostname());
                _ssdp_server->setURL("/");
                _ssdp_server->setDeviceType("urn:device:LOKLIK LEF01");
                //Start SSDP
    #        ifdef DEBUG_INIT
                grbl_send(CLIENT_ALL, "[MSG:SSDP Started]\r\n");
    #        endif
                grbl_send(CLIENT_ALL, "[MSG:SSDP Started]\r\n");
                _ssdp_server->begin();
            }
    #    endif
            _setupdone = true;
            return no_error;
        }

        void Web_Server::end() {
            _setupdone = false;
    #    ifdef ENABLE_SSDP
            if (_ssdp_server) {
                httpd_unregister_uri_handler(_webserver, "/description.xml", HTTP_GET);
                delete _ssdp_server;
                _ssdp_server = NULL;
            }
    #    endif  //ENABLE_SSDP

            ESP_LOGE(TAG, "stop webserver");
            if (_webserver) {
                stop_webserver(_webserver);
                _webserver = NULL;
            }

            Serial2Socket.detachWS();
            if (_socket_server) {
                delete _socket_server;
                _socket_server = NULL;
            }
    }

    void Web_Server::handle() {
        static uint32_t timeout = millis();
        COMMANDS::wait(0);
        if (_socket_server && _setupdone) {
            _socket_server->loop();
        }
        if ((millis() - timeout) > 10000 && _socket_server) {
            String s = "PING:";
            s += String(_id_connection);
            _socket_server->broadcastTXT(s);
            timeout = millis();
        }
    }
   
    void Web_Server::handle_Websocket_Event(uint8_t num, uint8_t type, uint8_t* payload, size_t length) {
        switch (type) {
            case WStype_DISCONNECTED:
                Uart0.printf("[%u] Disconnected!\n", num);
                break;
            case WStype_CONNECTED: {
                IPAddress ip = _socket_server->remoteIP(num);
                Uart0.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
                String s = "CURRENT_ID:" + String(num);
                // send message to client
                _id_connection = num;
                _socket_server->sendTXT(_id_connection, s);
                s = "ACTIVE_ID:" + String(_id_connection);
                _socket_server->broadcastTXT(s);
            } break;
            case WStype_TEXT:
                Uart0.printf("[%u] get Text: %s\n", num, payload);

                // send message to client
                // webSocket.sendTXT(num, "message here");

                // send data to all connected clients
                // webSocket.broadcastTXT("message here");
                break;
            case WStype_BIN:
                Uart0.printf("[%u] get binary length: %u\n", num, length);
                //hexdump(payload, length);

                // send message to client
                // webSocket.sendBIN(num, payload, length);
                break;
            default:
                break;
        }
    }
}
#endif  // Enable HTTP && ENABLE_WIFI

