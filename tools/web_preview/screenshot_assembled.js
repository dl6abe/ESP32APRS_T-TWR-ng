// Screenshots every assembled/*.html page via file:// - no network access
// needed at all, since fetch_and_assemble.sh already baked each page into a
// fully self-contained file (CSS, JS, and referenced icons inlined
// alongside it). Run after fetch_and_assemble.sh:
//   node screenshot_assembled.js

const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');

const ASSEMBLED_DIR = path.join(__dirname, 'assembled');
const OUT_DIR = path.join(__dirname, 'out');

async function main()
{
	const pages = fs
		.readdirSync(ASSEMBLED_DIR)
		.filter((f) => f.endsWith('.html') && !f.startsWith('_'));

	if (pages.length === 0)
	{
		console.error('No assembled pages found - run fetch_and_assemble.sh first.');
		process.exit(1);
	}

	fs.mkdirSync(OUT_DIR, { recursive: true });

	const browser = await chromium.launch();
	const page = await browser.newPage({ viewport: { width: 1000, height: 800 } });

	for (const file of pages)
	{
		const name = file.replace(/\.html$/, '');
		console.log(`Rendering ${name} ...`);
		await page.goto(`file://${path.join(ASSEMBLED_DIR, file)}`);
		await page.screenshot({ path: path.join(OUT_DIR, `${name}.png`), fullPage: true });
	}

	await browser.close();
	console.log(`\nDone. Screenshots in ${OUT_DIR}/`);
}

main().catch((err) => {
	console.error('FAILED:', err);
	process.exit(1);
});
