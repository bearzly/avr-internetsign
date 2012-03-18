/*
 * RequestHandler.h
 *
 * Created: 17/03/2012 7:07:31 PM
 *  Author: Benjamin
 */ 


#ifndef REQUESTHANDLER_H_
#define REQUESTHANDLER_H_

#include <avr/pgmspace.h>

const static char HTTP_OK[] PROGMEM  = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
const static char HTTP_AUTH_REQUIRED[] PROGMEM = "HTTP/1.0 401 Authorization Required\r\nWWW-Authenticate: Basic realm=\"Secure Area\"\r\nContent-Type: text/plain\r\n\r\nAuthorization Required";
const static char HTTP_NOT_FOUND[]PROGMEM  = "HTTP/1.0 404 Not Found\r\nContent-Type:text/plain\r\n\r\nThe page you requested does not exist";

const static char HTML_HEADER[] PROGMEM = "<!DOCTYPE html>"
"<html>"
"<head>"
"	<title>Internet Sign</title>"
"   <style type='text/css'>"
"body{font-family:Arial;background:#eee;padding:10px 0}h1{color:#126}#header{background:#777;padding:10px 0;border-bottom:7px solid #8ae;}#header ul{padding:0;margin:0;margin-left:3px}#header li{display:inline;background:#555;padding:7px 5px;border:1px #222}#header a{color:#ddd;text-decoration:none}#header a:hover{text-decoration:underline}#container{font-size: 10pt;background:#fff;padding:10px 0}h1{margin:0}#content{padding:5px 10px}#body{width:65%;margin:auto;border:2px solid #ddd}"
"   </style>"
"</head>";
const static char HTML_BODY[] PROGMEM = "<body>"
"   <div id='body'>"
"	<div id='header'>"
"		<ul>"
"			<li><a href='/'>Home</a></li>"
"			<li><a href='/config'>Configuration</a></li>"
"		</ul>"
"	</div>"
"   <div id='container'>"
"	<div id='content'>";

const static char CONFIG_ROW[] PROGMEM = "<tr><td>%s</td><td>%d.%d.%d.%d</td><td><input type='text' name='%s'></td></tr>";
const static char MAC_ROW[] PROGMEM = "<tr><td>MAC Address</td><td>%.2X:%.2X:%.2X:%.2X:%.2X:%.2X</td><td><input type='text' name='mac'></td></tr>";
const static char OPTION_BOX[] PROGMEM = "<tr><td>%s</td><td><input type='text' name='%s' style='width:20px' maxlength='2' value='%d'></td></tr>";

const static char HTML_FOOTER[] PROGMEM = "</div></div></div>"
"</body>"
"</html>";

int handle_request(const uint8_t* buffer);
void handle_root_param(const char* param, const char* value);
void handle_config_param(const char* param, const char* value);
void create_root_response(char* buffer);
void create_config_response(char* buffer);

int chartoint(char c);
void urldecode(char* dest, const char* src, int size);
char base64digit(uint8_t n);
char* const base64encode(const char* src);

#endif /* REQUESTHANDLER_H_ */