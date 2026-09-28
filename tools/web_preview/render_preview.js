// Renders every tab of the live device's web config UI to a PNG, the same
// way an actual browser session would produce them: load the SPA shell at
// "/", click each tab button, let jQuery's $("#contentmain").load(...) AJAX
// fetch finish, then screenshot. This deliberately does NOT fetch each
// endpoint (e.g. /igate) directly - those return bare HTML fragments with no
// <html>/<head>/stylesheet, meant only to be injected into the shell; a
// screenshot of the fragment alone would not show real layout/CSS.
//
// Needs an actual board reachable over HTTP - unlike tools/aprs_test, there
// is no way to do this without hardware, since the "visual" artifact here is
// what a real browser renders, not something this project's C++ can produce
// standalone (webservice.cpp reads config/WiFi/SD state directly throughout,
// same coupling problem as gui_lcd.cpp - see FORK_NOTES.md).
//
// Usage: BASE_URL=http://<device-ip> WEB_USER=admin WEB_PASS=admin node render_preview.js
//
// If this fails to reach the device at all (ERR_ADDRESS_UNREACHABLE /
// EHOSTUNREACH even though curl reaches it fine - a sandboxed dev
// environment that only allow-lists specific binaries for network access,
// for instance), use fetch_and_assemble.sh + screenshot_assembled.js
// instead: that pair does all network I/O via curl and only ever opens
// file:// URLs in the browser, at the cost of not exercising the real
// $("#contentmain").load(...) AJAX flow live.

const { chromium } = require('playwright');
const path = require('path');

const BASE_URL = process.env.BASE_URL || 'http://192.168.1.100';
const WEB_USER = process.env.WEB_USER || 'admin';
const WEB_PASS = process.env.WEB_PASS || 'admin';
const OUT_DIR = path.join(__dirname, 'out');

// Label text as shown on each tab button, and a filesystem-safe name for it.
const TABS = [
	['DashBoard', 'dashboard'],
	['Radio', 'radio'],
	['IGATE', 'igate'],
	['DIGI', 'digi'],
	['TRACKER', 'tracker'],
	['VPN', 'vpn'],
	['Wireless', 'wireless'],
	['System', 'system'],
	['File', 'file'],
	['About', 'about'],
];

async function main()
{
	const browser = await chromium.launch();
	const context = await browser.newContext({
		httpCredentials: { username: WEB_USER, password: WEB_PASS },
		viewport: { width: 1000, height: 800 },
	});
	const page = await context.newPage();

	const consoleErrors = [];
	page.on('pageerror', (err) => consoleErrors.push(`[${page.url()}] ${err.message}`));
	page.on('console', (msg) => {
		if (msg.type() === 'error')
			consoleErrors.push(`[${page.url()}] console.error: ${msg.text()}`);
	});

	console.log(`Loading ${BASE_URL}/ ...`);
	await page.goto(`${BASE_URL}/`, { waitUntil: 'networkidle' });
	await page.screenshot({ path: path.join(OUT_DIR, '00-shell.png'), fullPage: true });

	for (const [label, slug] of TABS)
	{
		console.log(`Rendering tab: ${label}`);
		await page.click(`button:has-text("${label}")`);
		// jQuery .load() is an async AJAX call - wait for the network to go
		// idle rather than a fixed delay, so this doesn't flake under load.
		await page.waitForLoadState('networkidle');
		const outPath = path.join(OUT_DIR, `${slug}.png`);
		await page.screenshot({ path: outPath, fullPage: true });
	}

	await browser.close();

	if (consoleErrors.length > 0)
	{
		console.log('\nJS errors/console.error seen while rendering:');
		for (const e of consoleErrors)
			console.log(' -', e);
	}
	console.log(`\nDone. Screenshots in ${OUT_DIR}/`);
}

main().catch((err) => {
	console.error('FAILED:', err);
	process.exit(1);
});
