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


#ifdef SDCARD
void handle_storage()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	String dirname = "/";
	char strTime[100];

	if (server.args() > 0)
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "SD_INIT")
			{
				// SD.end();
				// if (!SD.begin(SDCARD_CS, spiSD, SDSPEED)) {
				//	Serial.println("SD CARD Initialization failed!");
				//	//return;
				// }
			}
		}
	}

	//	setHTML(1);
	uint8_t cardType = SD.cardType();

	webString = "<div class=\"dash\">\n";
	webString += "<div class=\"dash-section-title\">SD Card information</div>\n";
	webString += "<div class=\"dash-panel\">\n";
	webString += "<table style=\"width:200px\">\n";
	webString += "<tr>\n";
	webString += "<td align=\"right\"><b>SD CARD TYPE:</b></td><td>";
	if (cardType == CARD_NONE)
	{
		// Close the row/table/panel opened above - the "else" branch below
		// closes these itself after appending its own card-size rows, but
		// with no card there are no more rows to add before closing them.
		webString += "NOT FOUND</td></tr>\n";
		webString += "</table>\n";
		webString += "</div>\n"; // .dash-panel
	}
	else
	{
		if (cardType == CARD_MMC)
		{
			webString += "MMC\n";
		}
		else if (cardType == CARD_SD)
		{
			webString += "SDSC\n";
		}
		else if (cardType == CARD_SDHC)
		{
			webString += "SDHC\n";
		}
		else
		{
			webString += "UNKNOWN\n";
		}
		unsigned long cardSize = SD.cardSize() / (1024 * 1024);
		unsigned long cardTotal = SD.totalBytes() / (1024 * 1024);
		unsigned long cardUsed = SD.usedBytes() / (1024 * 1024);

		webString += "</td></tr>";
		webString += "<tr><td style=\"width: 60px;text-align: right;\"><b>SD Card Size: </b></td><td style=\"text-align: right;\">";
		webString += String((double)cardSize / 1000, 1) + "GB</td></tr>\n";
		webString += "<tr><td style=\"width: 60px;text-align: right;\"><b>Total space: </b></td><td style=\"text-align: right;\">";
		webString += String((unsigned long)cardTotal) + "MB</td></tr>\n";
		webString += "<tr><td style=\"width: 60px;text-align: right;\"><b>Used space: </b></td><td style=\"text-align: right;\">";
		webString += String((unsigned long)cardUsed) + "M</td></tr>\n";

		webString += "</table>\n";
		webString += "</div>\n"; // .dash-panel

		webString += "<div class=\"dash-section-title\">Listing directory: " + dirname + "</div>\n";
		webString += "<div class=\"dash-panel\">\n";

		File root = SD.open(dirname);
		if (!root)
		{
			webString += "Failed to open directory\n";
			// return;
		}
		if (!root.isDirectory())
		{
			webString += "Not a directory";
			// return;
		}

		File file = root.openNextFile();
		webString += "<table border=\"1\"><tr align=\"center\" bgcolor=\"#03DDFC\"><td><b>DIRECTORY</b></td><td width=\"150\"><b>FILE NAME</b></td><td width=\"100\"><b>SIZE(Byte)</b></td><td width=\"170\"><b>DATE TIME</b></td><td><b>DEL</b></td></tr>";
		while (file)
		{
			if (file.isDirectory())
			{
				// webString += "<tr><td>DIR : ");
				webString += "<tr><td>" + String(file.name()) + "</td>";
				time_t t = file.getLastWrite();
				struct tm *tmstruct = localtime(&t);
				sprintf(strTime, "<td></td><td></td><td align=\"right\">%d-%02d-%02d %02d:%02d:%02d</td>", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
				webString += String(strTime);
				// if (levels) {
				//	listDir(fs, file.name(), levels - 1);
				// }
				webString += "<td></td></tr>\n";
			}
			else
			{
				/*Serial.print("  FILE: ");
				Serial.print(file.name());*/
				String fName = String(file.name()).substring(1);
				webString += "<tr><td>/</td><td align=\"right\"><a href=\"/download?FILE=" + fName + "\" target=\"_blank\">" + fName + "</a></td>";
				// Serial.print("  SIZE: ");
				webString += "<td align=\"right\">" + String(file.size()) + "</td>";
				time_t t = file.getLastWrite();
				struct tm *tmstruct = localtime(&t);
				sprintf(strTime, "<td align=\"right\">%d-%02d-%02d %02d:%02d:%02d</td>", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
				webString += String(strTime);
				webString += "<td align=\"center\"><a href=\"/delete?FILE=" + fName + "\">X</a></td></tr>\n";
			}
			file = root.openNextFile();
		}
		webString += "</table>\n";
		webString += "</div>\n"; // .dash-panel
	}

	webString += "</div>\n"; // .dash
	server.send(200, "text/html", webString); // send to someones browser when asked
}

#endif

#ifdef SDCARD
void handle_download()
{
	String dataType = "text/plain";
	String path;
	if (server.args() > 0)
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "FILE")
			{
				path = server.arg(i);
				break;
			}
		}
	}

	if (path.endsWith(".src"))
		path = path.substring(0, path.lastIndexOf("."));
	else if (path.endsWith(".htm"))
		dataType = "text/html";
	else if (path.endsWith(".csv"))
		dataType = "text/csv";
	else if (path.endsWith(".css"))
		dataType = "text/css";
	else if (path.endsWith(".xml"))
		dataType = "text/xml";
	else if (path.endsWith(".png"))
		dataType = "image/png";
	else if (path.endsWith(".gif"))
		dataType = "image/gif";
	else if (path.endsWith(".jpg"))
		dataType = "image/jpeg";
	else if (path.endsWith(".ico"))
		dataType = "image/x-icon";
	else if (path.endsWith(".svg"))
		dataType = "image/svg+xml";
	else if (path.endsWith(".ico"))
		dataType = "image/x-icon";
	else if (path.endsWith(".js"))
		dataType = "application/javascript";
	else if (path.endsWith(".pdf"))
		dataType = "application/pdf";
	else if (path.endsWith(".zip"))
		dataType = "application/zip";
	else if (path.endsWith(".gz"))
	{
		if (path.startsWith("/gz/htm"))
			dataType = "text/html";
		else if (path.startsWith("/gz/css"))
			dataType = "text/css";
		else if (path.startsWith("/gz/csv"))
			dataType = "text/csv";
		else if (path.startsWith("/gz/xml"))
			dataType = "text/xml";
		else if (path.startsWith("/gz/js"))
			dataType = "application/javascript";
		else if (path.startsWith("/gz/svg"))
			dataType = "image/svg+xml";
		else
			dataType = "application/x-gzip";
	}

	File myFile = SD.open("/" + path, "r");
	if (myFile)
	{
		server.sendHeader("Content-Type", dataType);
		server.sendHeader("Content-Disposition", "attachment; filename=" + path);
		server.sendHeader("Connection", "close");
		server.streamFile(myFile, "application/octet-stream");
		myFile.close();
	}
	delay(100);
}

void handle_delete()
{
	String dataType = "text/plain";
	String path;
	if (server.args() > 0)
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "FILE")
			{
				path = server.arg(i);
				Serial.println("Deleting file: " + path);
				if (SD.remove("/" + path))
				{
					Serial.println("File deleted");
				}
				else
				{
					Serial.println("Delete failed");
				}
				break;
			}
		}
	}

	handle_storage();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
	Serial.printf("Listing directory: %s\r\n", dirname);

	File root = fs.open(dirname);
	if (!root)
	{
		Serial.println("Failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println("Not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.print("  DIR : ");
			Serial.print(file.name());
			time_t t = file.getLastWrite();
			struct tm *tmstruct = localtime(&t);
			Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\r\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
			if (levels)
			{
				listDir(fs, file.name(), levels - 1);
			}
		}
		else
		{
			Serial.print("  FILE: ");
			Serial.print(file.name());
			Serial.print("  SIZE: ");
			Serial.print(file.size());
			time_t t = file.getLastWrite();
			struct tm *tmstruct = localtime(&t);
			Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\r\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
		}
		file = root.openNextFile();
	}
}
#endif
