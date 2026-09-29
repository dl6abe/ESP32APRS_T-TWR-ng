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


void handle_radio()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	// bool noiseEn=false;
	bool radioEnable = false;
	if (server.hasArg("commitRadio"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "radioEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						radioEnable = true;
					}
				}
			}

			if (server.argName(i) == "nw_band")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.band = server.arg(i).toInt();
					}
				}
			}

			if (server.argName(i) == "volume")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.volume = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rf_power")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						if (server.arg(i).toInt())
							config.rf_power = true;
						else
							config.rf_power = false;
					}
				}
			}

			if (server.argName(i) == "sql_level")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.sql_level = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx_freq")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.freq_tx = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "rx_freq")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.freq_rx = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "tx_offset")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.offset_tx = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "rx_offset")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.offset_rx = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx_ctcss")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tone_tx = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "rx_ctcss")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tone_rx = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "rf_type")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.rf_type = server.arg(i).toInt();
				}
			}
		}
		// config.noise=noiseEn;
		// config.agc=agcEn;
		config.rf_en = radioEnable;
		String html = "OK";
		server.send(200, "text/html", html); // send to someones browser when asked
		saveEEPROM();
		// delay(100);
		RF_MODULE(false);
	}
	else if (server.hasArg("commitTNC"))
	{
		bool hpf = 0;
		bool bpf = 0;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "HPF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						hpf = true;
					}
				}
			}
			if (server.argName(i) == "BPF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						bpf = true;
					}
				}
			}
			if (server.argName(i) == "timeSlot")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.tx_timeslot = server.arg(i).toInt();
					}
				}
			}
			if (server.argName(i) == "preamble")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.preamble = server.arg(i).toInt();
					}
				}
			}
		}
		config.audio_hpf = hpf;
		config.audio_bpf = bpf;
		afskSetHPF(config.audio_hpf);
		afskSetBPF(config.audio_bpf);
		String html = "OK";
		server.send(200, "text/html", html); // send to someones browser when asked
		saveEEPROM();
	}
	else
	{
		String html = "<script type=\"text/javascript\">\n";
		html += "var sliderVol = document.getElementById(\"sliderVolume\");\n";
		html += "var outputVol = document.getElementById(\"volShow\");\n";
		html += "var sliderSql = document.getElementById(\"sliderSql\");\n";
		html += "var outputSql = document.getElementById(\"sqlShow\");\n";
		html += "outputVol.innerHTML = sliderVol.value;\n";
		html += "outputSql.innerHTML = sliderSql.value;\n";
		html += "\n";
		html += "sliderVol.oninput = function () {\n";
		html += "outputVol.innerHTML = this.value;\n";
		html += "}\n";
		html += "sliderSql.oninput = function () {\n";
		html += "outputSql.innerHTML = this.value;\n";
		html += "}\n";
		html += "\n";
		html += "document.querySelectorAll('form').forEach(function (form) {\n";
		html += "form.addEventListener('submit', function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formRadio\") document.getElementById(\"submitRadio\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formTNC\") document.getElementById(\"submitTNC\").disabled=true;\n";
		html += "fetch('/radio', { method: 'POST', body: data })\n";
		html += ".then(function () { alert(\"Submited Successfully\"); })\n";
		html += ".catch(function () { alert(\"An error occurred.\"); });\n";
		html += "});\n";
		html += "});\n";
		html += "function rfType(){\n";
		html += "var type = document.getElementById(\"rf_type\").value;\n";
		html += "if(type==1||type==4||type==7){document.getElementById(\"tx_freq\").setAttribute(\"max\",174);document.getElementById(\"rx_freq\").setAttribute(\"max\",174);};\n";
		html += "if(type==1){document.getElementById(\"tx_freq\").setAttribute(\"min\",134);document.getElementById(\"rx_freq\").setAttribute(\"min\",134);};\n";
		html += "if(type==4||type==7){document.getElementById(\"tx_freq\").setAttribute(\"min\",136);document.getElementById(\"rx_freq\").setAttribute(\"min\",136);};\n";
		html += "if(type==2||type==5||type==8){document.getElementById(\"tx_freq\").setAttribute(\"max\",470);document.getElementById(\"rx_freq\").setAttribute(\"max\",470);};\n";
		html += "if(type==2||type==5||type==8){document.getElementById(\"tx_freq\").setAttribute(\"min\",400);document.getElementById(\"rx_freq\").setAttribute(\"min\",400);};\n";
		html += "if(type==3){document.getElementById(\"tx_freq\").setAttribute(\"min\",320);document.getElementById(\"rx_freq\").setAttribute(\"min\",320);};\n";
		html += "if(type==3){document.getElementById(\"tx_freq\").setAttribute(\"max\",400);document.getElementById(\"rx_freq\").setAttribute(\"max\",400);};\n";
		html += "if(type==6){document.getElementById(\"tx_freq\").setAttribute(\"min\",350);document.getElementById(\"rx_freq\").setAttribute(\"min\",350);};\n";
		html += "if(type==6){document.getElementById(\"tx_freq\").setAttribute(\"max\",390);document.getElementById(\"rx_freq\").setAttribute(\"max\",390);};\n";
		html += "if(type==1||type==4||type==7){document.getElementById(\"tx_freq\").setAttribute(\"value\",144.390);document.getElementById(\"rx_freq\").setAttribute(\"value\",144.390);};\n";
		html += "if(type==2||type==5||type==8){document.getElementById(\"tx_freq\").setAttribute(\"value\",432.5);document.getElementById(\"rx_freq\").setAttribute(\"value\",432.5);};\n";
		html += "\n";
		html += "}\n";
		html += "</script>\n";
		html += "<div class=\"dash\">\n";
		html += "<form id='formRadio' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">RF Analog Module</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String radioEnFlag = "";
		if (config.rf_en)
			radioEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"radioEnable\" value=\"OK\" " + radioEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"rf_type\">Module Type</label><div class=\"dash-field-body\"><select name=\"rf_type\" id=\"rf_type\" onchange=\"rfType()\">\n";
		for (int i = 0; i < 10; i++)
		{
			if (config.rf_type == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(RF_TYPE[i]) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(RF_TYPE[i]) + "</option>\n";
		}
		html += "</select></div></div>\n";
		float freqMin = 0;
		float freqMax = 0;
		switch (config.rf_type)
		{
		case RF_SA868_VHF:
			freqMin = 134.0F;
			freqMax = 174.0F;
			break;
		case RF_SR_1WV:
		case RF_SR_2WVS:
			freqMin = 136.0F;
			freqMax = 174.0F;
			break;
		case RF_SA868_350:
			freqMin = 320.0F;
			freqMax = 400.0F;
			break;
		case RF_SR_1W350:
			freqMin = 350.0F;
			freqMax = 390.0F;
			break;
		case RF_SA868_UHF:
		case RF_SR_1WU:
		case RF_SR_2WUS:
			freqMin = 400.0F;
			freqMax = 470.0F;
			break;
		default:
			freqMin = 134.0F;
			freqMax = 500.0F;
			break;
		}
		html += "<div class=\"dash-field\"><label for=\"tx_freq\">TX Frequency</label><div class=\"dash-field-body\"><input type=\"number\" id=\"tx_freq\" name=\"tx_freq\" min=\"" + String(freqMin, 4) + "\" max=\"" + String(freqMax, 4) + "\" step=\"0.0001\" value=\"" + String(config.freq_tx, 4) + "\" /> MHz</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"rx_freq\">RX Frequency</label><div class=\"dash-field-body\"><input type=\"number\" id=\"rx_freq\" name=\"rx_freq\" min=\"" + String(freqMin, 4) + "\" max=\"" + String(freqMax, 4) + "\" step=\"0.0001\" value=\"" + String(config.freq_rx, 4) + "\" /> Mhz</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"tx_ctcss\">TX CTCSS</label><div class=\"dash-field-body\"><select name=\"tx_ctcss\" id=\"tx_ctcss\">\n";
		for (int i = 0; i < 39; i++)
		{
			if (config.tone_tx == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(ctcss[i], 1) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(ctcss[i], 1) + "</option>\n";
		}
		html += "</select> Hz</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"rx_ctcss\">RX CTCSS</label><div class=\"dash-field-body\"><select name=\"rx_ctcss\" id=\"rx_ctcss\">\n";
		html += "<option value=\"0\" selected>0.0</option>\n";
		for (int i = 0; i < 39; i++)
		{
			if (config.tone_rx == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(ctcss[i], 1) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(ctcss[i], 1) + "</option>\n";
		}
		html += "</select> Hz</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"nw_band\">Narrow/Wide</label><div class=\"dash-field-body\"><select name=\"nw_band\" id=\"nw_band\">\n";
		String cmSelNWT = "";
		String cmSelNWF = "";
		if (config.band)
		{
			cmSelNWT = "selected";
		}
		else
		{
			cmSelNWF = "selected";
		}
		html += "<option value=\"1\" " + cmSelNWT + ">25.0KHz</option>\n";
		html += "<option value=\"0\" " + cmSelNWF + ">12.5KHz</option>\n";
		html += "</select></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"rf_power\">TX Power</label><div class=\"dash-field-body\"><select name=\"rf_power\" id=\"rf_power\">\n";
		String cmRfPwrF = "";
		String cmRfPwrT = "";
		if (config.rf_power)
		{
			cmRfPwrT = "selected";
		}
		else
		{
			cmRfPwrF = "selected";
		}
		html += "<option value=\"1\" " + cmRfPwrT + ">HIGH</option>\n";
		html += "<option value=\"0\" " + cmRfPwrF + ">LOW</option>\n";
		html += "</select></div></div>\n";

		html += "<div class=\"dash-field\"><label>VOLUME</label><div class=\"dash-field-body\"><input id=\"sliderVolume\" name=\"volume\" type=\"range\" min=\"1\" max=\"8\" value=\"" + String(config.volume) + "\" /><b><span style=\"font-size: 14pt;\" id=\"volShow\">" + String(config.volume) + "</span></b></div></div>\n";

		html += "<div class=\"dash-field\"><label>SQL Level</label><div class=\"dash-field-body\"><input id=\"sliderSql\" name=\"sql_level\" type=\"range\" min=\"0\" max=\"8\" value=\"" + String(config.sql_level) + "\" /><b><span style=\"font-size: 14pt;\" id=\"sqlShow\">" + String(config.sql_level) + "</span></b></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitRadio' name=\"commitRadio\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitRadio\"/>\n";
		html += "</form>\n";

		// AFSK,TNC Configuration
		html += "<form id='formTNC' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">AFSK/TNC Configuration</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String strFlag = "";
		if (config.audio_hpf)
			strFlag = "checked";
		html += "<div class=\"dash-field\"><label>Audio HPF</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"HPF\" value=\"OK\" " + strFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Audio high pass filter &gt;1KHz cutoff 10Khz</span></div></div>\n";

		strFlag = "";
		if (config.audio_bpf)
			strFlag = "checked";
		html += "<div class=\"dash-field\"><label>Audio BPF</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"BPF\" value=\"OK\" " + strFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Audio band pass filter 1Khz-2.5KHz</span></div></div>\n";

		html += "<div class=\"dash-field\"><label>TX Time Slot</label><div class=\"dash-field-body\"><input type=\"number\" name=\"timeSlot\" min=\"2000\" max=\"99999\" step=\"1000\" value=\"" + String(config.tx_timeslot) + "\" /> mSec.</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"preamble\">Preamble</label><div class=\"dash-field-body\"><select name=\"preamble\" id=\"preamble\">\n";
		for (int i = 1; i < 10; i++)
		{
			if (config.preamble == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(i * 100) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(i * 100) + "</option>\n";
		}
		html += "</select> mSec.</div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitTNC' name=\"commitTNC\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitTNC\"/>\n";
		html += "</form>\n";
		html += "</div>\n"; // .dash
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_vpn()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("commitVPN"))
	{
		bool vpnEn = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "vpnEnable")
			{
				if (server.arg(i) != "")
				{
					// if (isValidNumber(server.arg(i)))
					if (String(server.arg(i)) == "OK")
						vpnEn = true;
				}
			}

			// if (server.argName(i) == "taretime") {
			//	if (server.arg(i) != "")
			//	{
			//		//if (isValidNumber(server.arg(i)))
			//		if (String(server.arg(i)) == "OK")
			//			taretime = true;
			//	}
			// }
			if (server.argName(i) == "wg_port")
			{
				if (server.arg(i) != "")
				{
					config.wg_port = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "wg_public_key")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_public_key, server.arg(i).c_str());
					config.wg_public_key[44] = 0;
				}
			}

			if (server.argName(i) == "wg_private_key")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_private_key, server.arg(i).c_str());
					config.wg_private_key[44] = 0;
				}
			}

			if (server.argName(i) == "wg_peer_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_peer_address, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "wg_local_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_local_address, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "wg_netmask_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_netmask_address, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "wg_gw_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_gw_address, server.arg(i).c_str());
				}
			}
		}

		config.vpn = vpnEn;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "document.querySelectorAll('form').forEach(function (form) {\n";
		html += "form.addEventListener('submit', function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formVPN\") document.getElementById(\"submitVPN\").disabled=true;\n";
		html += "fetch('/vpn', { method: 'POST', body: data })\n";
		html += ".then(function () { alert(\"Submited Successfully\"); })\n";
		html += ".catch(function () { alert(\"An error occurred.\"); });\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		html += "<div class=\"dash\">\n";
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"formVPN\" method=\"post\">\n";
		html += "<div class=\"dash-section-title\">Wireguard Configuration</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String syncFlage = "";
		if (config.vpn)
			syncFlage = "checked";
		html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"vpnEnable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_peer_address\">Server Address</label><div class=\"dash-field-body\"><input size=\"20\" id=\"wg_peer_address\" name=\"wg_peer_address\" type=\"text\" value=\"" + String(config.wg_peer_address) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_port\">Server Port</label><div class=\"dash-field-body\"><input id=\"wg_port\" size=\"5\" name=\"wg_port\" type=\"number\" value=\"" + String(config.wg_port) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_local_address\">Local Address</label><div class=\"dash-field-body\"><input id=\"wg_local_address\" name=\"wg_local_address\" type=\"text\" value=\"" + String(config.wg_local_address) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_netmask_address\">Netmask</label><div class=\"dash-field-body\"><input id=\"wg_netmask_address\" name=\"wg_netmask_address\" type=\"text\" value=\"" + String(config.wg_netmask_address) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_gw_address\">Gateway</label><div class=\"dash-field-body\"><input id=\"wg_gw_address\" name=\"wg_gw_address\" type=\"text\" value=\"" + String(config.wg_gw_address) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_public_key\">Public Key</label><div class=\"dash-field-body\"><input size=\"50\" maxlength=\"44\" id=\"wg_public_key\" name=\"wg_public_key\" type=\"text\" value=\"" + String(config.wg_public_key) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wg_private_key\">Private Key</label><div class=\"dash-field-body\"><input size=\"50\" maxlength=\"44\" id=\"wg_private_key\" name=\"wg_private_key\" type=\"text\" value=\"" + String(config.wg_private_key) + "\" /></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitVPN' name=\"commitVPN\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitVPN\"/>\n";
		html += "</form>\n";
		html += "</div>\n"; // .dash

		html += "</body></html>\n";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

