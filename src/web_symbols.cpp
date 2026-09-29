/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/
#include "AFSK.h"
#include "webservice.h"
#include "wireguard_vpn.h"
#include <LibAPRSesp.h>
#include <parse_aprs.h>
#include "wifi_config.h"

#include "glcdfont.c" // classic 5x7 font, for overlay-character glyphs in handle_symbol_icon()
#include "aprs_color_icons.h"

// Serves an APRS symbol icon for the web dashboard. Prefers a color PNG from
// aprs_color_icons.h (see that file for licensing/attribution); falls back
// to a tiny 16x16 1-bit BMP built from the Icon_TableA/Icon_TableB bitmaps
// also used by the OLED display (see gui_lcd.cpp's use of drawYBitmap()) for
// symbols with no color icon (the two excluded trademarked logos) and for
// any symbol requested with an overlay character - compositing the overlay
// glyph onto a decoded color PNG isn't implemented, so overlay icons stay
// monochrome. No network access, no external server involved either way.
// Query args: c=symbol code (33-128), t=table (1='/', 2='\', anything else
// falls back to a generic icon), o=overlay character code (0=none).
// This replaces symbol images previously hotlinked from dprns.com, whose
// TLS certificate expired and whose /symbols/icons/ path no longer exists.
void handle_symbol_icon()
{
	uint8_t code = server.hasArg("c") ? (uint8_t)server.arg("c").toInt() : 0;
	int t = server.hasArg("t") ? server.arg("t").toInt() : 0;
	uint8_t overlay = server.hasArg("o") ? (uint8_t)server.arg("o").toInt() : 0;

	uint8_t symIdx = code - 0x21;

	if (overlay == 0 && symIdx <= 95 && (t == 1 || t == 2))
	{
		const AprsColorIcon &icon = (t == 1) ? aprsColorIcons_primary[symIdx] : aprsColorIcons_secondary[symIdx];
		if (icon.data != nullptr)
		{
			server.sendHeader("Cache-Control", "max-age=86400");
			server.send_P(200, "image/png", (PGM_P)icon.data, icon.len);
			return;
		}
	}

	const uint8_t *bmp;
	if (symIdx <= 95 && t == 1)
		bmp = &Icon_TableA[symIdx][0];
	else if (symIdx <= 95 && t == 2)
		bmp = &Icon_TableB[symIdx][0];
	else
		bmp = &Icon_TableB[5][0]; // '&' - generic fallback, same as gui_lcd.cpp

	const int W = 16, H = 16, rowBytes = 4; // 16px @ 1bpp = 2 bytes, padded to 4
	uint8_t bmpFile[62 + rowBytes * H];
	memset(bmpFile, 0, sizeof(bmpFile));

	uint32_t fileSize = sizeof(bmpFile), pixelOffset = 62, hdrSize = 40;
	int32_t width = W, height = H;
	uint16_t planes = 1, bpp = 1;
	uint32_t imageSize = rowBytes * H;

	bmpFile[0] = 'B';
	bmpFile[1] = 'M';
	memcpy(&bmpFile[2], &fileSize, 4);
	memcpy(&bmpFile[10], &pixelOffset, 4);
	memcpy(&bmpFile[14], &hdrSize, 4);
	memcpy(&bmpFile[18], &width, 4);
	memcpy(&bmpFile[22], &height, 4);
	memcpy(&bmpFile[26], &planes, 2);
	memcpy(&bmpFile[28], &bpp, 2);
	memcpy(&bmpFile[34], &imageSize, 4);
	// Palette: a set bit indexes entry 1, so entry 0 = white (bit=0, background),
	// entry 1 = black (bit=1, the symbol's ink) - entry 1 is left at (0,0,0)
	// from the memset above.
	bmpFile[54] = 255;
	bmpFile[55] = 255;
	bmpFile[56] = 255;

	// bmp[] is packed column-major, 8 vertical pixels per byte, LSB first
	// (see drawYBitmap in lib/Adafruit_GFX). BMP rows are stored bottom-up.
	for (int row = 0; row < H; row++)
	{
		int srcRow = H - 1 - row;
		uint8_t *dest = &bmpFile[62 + row * rowBytes];
		for (int col = 0; col < W; col++)
		{
			uint8_t b = pgm_read_byte(&bmp[col + (srcRow / 8) * W]);
			if ((b >> (srcRow % 8)) & 0x01)
				dest[col / 8] |= (0x80 >> (col % 8));
		}
	}

	// APRS overlay character: an alternate-table symbol whose table id byte
	// is A-Z/0-9 instead of '\' pairs with an overlay char drawn on top of
	// the icon (see parse_aprs.cpp's overlay handling). Draw it as a small
	// glyph in the bottom-right corner instead of dropping it.
	if (overlay >= 0x20 && overlay < 0x80)
	{
		const int glyphW = 5, glyphH = 7, ox = W - glyphW, oy = H - glyphH;
		for (int gr = 0; gr < glyphH; gr++)
		{
			int row = H - 1 - (oy + gr);
			uint8_t *dest = &bmpFile[62 + row * rowBytes];
			for (int gc = 0; gc < glyphW; gc++)
			{
				uint8_t colBits = pgm_read_byte(&font[overlay * 5 + gc]);
				if ((colBits >> gr) & 0x01)
				{
					int c = ox + gc;
					dest[c / 8] |= (0x80 >> (c % 8));
				}
			}
		}
	}

	server.sendHeader("Cache-Control", "max-age=86400");
	server.setContentLength(sizeof(bmpFile));
	server.send(200, "image/bmp", "");
	server.sendContent((const char *)bmpFile, sizeof(bmpFile));
}

void handle_symbol()
{
	int i;
	String html = "<div class=\"dash\">\n";
	html += "<div class=\"dash-panel\">\n";
	html += "<table>\n";
	html += "<tr><th colspan=\"16\">Table '/'</th></tr>\n";
	html += "<tr>\n";
	for (i = 33; i < 129; i++)
	{
		html += "<td><img src=\"/icon.png?c=" + String(i) + "&t=1\"></td>\n";
		if (((i % 16) == 0) && (i < 126))
			html += "</tr>\n<tr>\n";
	}
	html += "</tr>";
	html += "</table>\n";
	html += "<table>\n";
	html += "<tr><th colspan=\"16\">Table '\\'</th></tr>\n";
	html += "<tr>\n";
	for (i = 33; i < 129; i++)
	{
		html += "<td><img src=\"/icon.png?c=" + String(i) + "&t=2\"></td>\n";
		if (((i % 16) == 0) && (i < 126))
			html += "</tr>\n<tr>\n";
	}
	html += "</tr>";
	html += "</table>\n";
	html += "</div>\n"; // .dash-panel
	html += "</div>\n"; // .dash
	server.send(200, "text/html", html); // send to someones browser when asked
}

// Escapes text before it goes into HTML. Needed specifically for anything
// that came from a received RF/APRS-IS packet (callsign, item/object name,
// digipeat path) - that's attacker-controlled by anyone transmitting on the
// configured frequency, no authentication required, and nothing upstream
// validates its character set. Without this, a crafted callsign/path
// containing '<script>' or similar renders as live HTML/JS in the admin's
// browser - a stored-injection vector sourced from radio traffic, not just
// the web form. See FORK_NOTES.md.
String escapeHtml(const String &input)
{
	String out;
	out.reserve(input.length());
	for (unsigned int i = 0; i < input.length(); i++)
	{
		char c = input.charAt(i);
		switch (c)
		{
		case '&':
			out += "&amp;";
			break;
		case '<':
			out += "&lt;";
			break;
		case '>':
			out += "&gt;";
			break;
		case '"':
			out += "&quot;";
			break;
		case '\'':
			out += "&#39;";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}

void handle_lastHeard()
{
	struct pbuf_t aprs;
	ParseAPRS aprsParse;
	struct tm tmstruct;
	String html = "";
	sort(pkgList, PKGLISTSIZE);

	html = "<table>\n";
	// No inline background here (was a hardcoded #070ac2 that clashed with the
	// dashboard's accent blue) - inherits .dash table th's background so this
	// title row and the column-header row below it are the same blue.
	html += "<th colspan=\"8\">LAST HEARD</th>\n";
	html += "<tr>\n";
	html += "<th style=\"min-width:10ch\"><span><b>Time (";
	if (config.timeZone >= 0)
		html += "+";
	// else
	//	html += "-";

	if (config.timeZone == (int)config.timeZone)
		html += String((int)config.timeZone) + ")</b></span></th>\n";
	else
		html += String(config.timeZone, 1) + ")</b></span></th>\n";
	html += "<th style=\"min-width:16px\">ICON</th>\n";
	html += "<th style=\"min-width:10ch\">Callsign</th>\n";
	html += "<th>VIA LAST PATH</th>\n";
	html += "<th style=\"min-width:5ch\">DX</th>\n";
	html += "<th style=\"min-width:5ch\">PACKET</th>\n";
	html += "<th style=\"min-width:5ch\">AUDIO</th>\n";
	html += "<th>COMMENT</th>\n";
	html += "</tr>\n";

	// PKGLISTSIZE (main.h), not a hardcoded 30 - the underlying pkgList
	// already retains this many heard packets (100 with PSRAM, 10 without),
	// the table was just never showing more than the first 30 of them.
	for (int i = 0; i < PKGLISTSIZE; i++)
	{
		pkgListType pkg = getPkgList(i);
		if (pkg.time > 0)
		{
			String line = String(pkg.raw);
			int packet = pkg.pkg;
			int start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
			if (start_val > 3)
			{
				String src_call = line.substring(0, start_val);
				memset(&aprs, 0, sizeof(pbuf_t));
				aprs.buf_len = 300;
				aprs.packet_len = line.length();
				if (aprs.packet_len > (int)sizeof(aprs.data) - 1)
					aprs.packet_len = (int)sizeof(aprs.data) - 1; // clamp: data[] is fixed-size, avoid overrun on oversized packets
				line.toCharArray(&aprs.data[0], aprs.packet_len + 1); // +1: toCharArray's 2nd arg is buffer size, not char count - avoids dropping the last byte
				int start_info = line.indexOf(":", 0);
				int end_ssid = line.indexOf(",", 0);
				int start_dst = line.indexOf(">", 2);
				int start_dstssid = line.indexOf("-", start_dst);
				String path = "";

				if ((end_ssid > start_dst) && (end_ssid < start_info))
				{
					path = line.substring(end_ssid + 1, start_info);
				}
				if (end_ssid < 5)
					end_ssid = start_info;
				if ((start_dstssid > start_dst) && (start_dstssid < start_dst + 10))
				{
					aprs.dstcall_end_or_ssid = &aprs.data[start_dstssid];
				}
				else
				{
					aprs.dstcall_end_or_ssid = &aprs.data[end_ssid];
				}
				aprs.info_start = &aprs.data[start_info + 1];
				aprs.dstname = &aprs.data[start_dst + 1];
				aprs.dstname_len = end_ssid - start_dst;
				aprs.dstcall_end = &aprs.data[end_ssid];
				aprs.srccall_end = &aprs.data[start_dst];

				// Serial.println(aprs.info_start);
				// aprsParse.parse_aprs(&aprs);
				if (aprsParse.parse_aprs(&aprs))
				{
					pkg.calsign[10] = 0;
					localtime_r(&pkg.time, &tmstruct);
					char strTime[10];
					snprintf(strTime, sizeof(strTime), "%02d:%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
					// String str = String(tmstruct.tm_hour, DEC) + ":" + String(tmstruct.tm_min, DEC) + ":" + String(tmstruct.tm_sec, DEC);

					html += "<tr><td>" + String(strTime) + "</td>";
					uint8_t sym = (uint8_t)aprs.symbol[1];
					if (sym > 31 && sym < 127)
					{
						if (aprs.symbol[0] > 64 && aprs.symbol[0] < 91) // overlay char (A-Z), paired with the alternate table
						{
							html += "<td><img src=\"/icon.png?c=" + String(sym) + "&t=2&o=" + String((int)aprs.symbol[0]) + "\"></td>";
						}
						else
						{
							int symTable = (aprs.symbol[0] == 92) ? 2 : (aprs.symbol[0] == 47) ? 1 : 0;
							html += "<td><img src=\"/icon.png?c=" + String(sym) + "&t=" + String(symTable) + "\"></td>";
						}
					}
					else
					{
						html += "<td><img src=\"/icon.png?c=38&t=0\"></td>";
					}
					String src_call_esc = escapeHtml(src_call);
					html += "<td><a href=\"https://aprs.fi/" + src_call_esc + "\" target=\"_blank\">" + src_call_esc + "</a>";
					if (aprs.srcname_len > 0 && aprs.srcname_len < 10) // Get Item/Object
					{
						char itemname[10];
						memset(&itemname, 0, sizeof(itemname));
						memcpy(&itemname, aprs.srcname, aprs.srcname_len);
						html += "(" + escapeHtml(String(itemname)) + ")";
					}
					html += +"</td>";
					if (path == "")
					{
						html += "<td style=\"text-align: left;\">RF: DIRECT</td>";
					}
					else
					{
						String LPath = path.substring(path.lastIndexOf(',') + 1);
						// if(path.indexOf("qAR")>=0 || path.indexOf("qAS")>=0 || path.indexOf("qAC")>=0){ //Via from Internet Server
						if (path.indexOf("qA") >= 0 || path.indexOf("TCPIP") >= 0)
						{
							html += "<td style=\"text-align: left;\">INET: " + escapeHtml(LPath) + "</td>";
						}
						else
						{
							if (LPath.indexOf("*") > 0)
								html += "<td style=\"text-align: left;\">DIGI: " + escapeHtml(path) + "</td>";
							else
								html += "<td style=\"text-align: left;\">RF: " + escapeHtml(path) + "</td>";
						}
					}
					// html += "<td>" + path + "</td>";
					if (aprs.flags & F_HASPOS)
					{
						double lat, lon;
						if (gps.location.isValid())
						{
							lat = gps.location.lat();
							lon = gps.location.lng();
						}
						else
						{
							lat = config.igate_lat;
							lon = config.igate_lon;
						}
						double dtmp = aprsParse.direction(lon, lat, aprs.lng, aprs.lat);
						double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
						html += "<td>" + String(dist, 1) + "km/" + String(dtmp, 0) + "°</td>";
					}
					else
					{
						html += "<td>-</td>\n";
					}
					html += "<td>" + String(packet) + "</td>\n";
					if (pkg.audio_level == 0)
					{
						html += "<td>-</td>\n";
					}
					else
					{
						//Vp-p to dBV at http://earmark.net/gesr/opamp/db_calc.htm
						double Vrms = (double)pkg.audio_level / 1000;
						double audBV = 20.0F * log10(Vrms);
						if(audBV<-15.0F){
							html += "<td style=\"color: #0000f0;\">"; //Low wave amplitude <0.5Vp-p
						}else if(audBV>-4.0F){
							html += "<td style=\"color: #f00000;\">"; //High wave amplitude >1.8Vp-p
						}else{
							html += "<td style=\"color: #008000;\">";
						}
						html += String(audBV, 1) + "dBV</td>\n";
					}
					// aprs.comment points into aprs.data (not its own
					// null-terminated buffer), bounded by comment_len - copy
					// into a fixed buffer the same way srcname/itemname does
					// above, and escape it since it's attacker-controlled
					// (RF/APRS-IS packet content, see escapeHtml()'s comment).
					if (aprs.comment_len > 0 && aprs.comment_len < 200)
					{
						char commentBuf[200];
						memset(commentBuf, 0, sizeof(commentBuf));
						memcpy(commentBuf, aprs.comment, aprs.comment_len);
						html += "<td style=\"text-align: left;white-space:normal;max-width:280px;\">" + escapeHtml(String(commentBuf)) + "</td></tr>\n";
					}
					else
					{
						html += "<td>-</td></tr>\n";
					}
				}
			}
		}
	}
	html += "</table>\n";
	server.send(200, "text/html", html); // send to someones browser when asked
}

