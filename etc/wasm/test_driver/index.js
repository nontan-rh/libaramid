const yargs = require('yargs');
const webdriver = require('selenium-webdriver');

const argv = yargs
  .option('sandbox', {
    description: 'If false, it launches Chrome with --no-sandbox option',
    type: 'boolean',
    default: true,
  })
  .help()
  .alias('help', 'h')
  .argv;

const chromeCapabilities = webdriver.Capabilities.chrome();
chromeCapabilities.set('chromeOptions', { args: ['--headless', '--disable-gpu', argv.sandbox ? null : '--no-sandbox'].filter(x => !!x) });

(async function () {
  const driver = await new webdriver.Builder()
    .forBrowser('chrome')
    .withCapabilities(chromeCapabilities)
    .build();
  let statusCode = 1;
  try {
    await driver.get('http://localhost:8080/');
    const statusElement = await driver.findElement({ id: 'status' });
    const outputElement = await driver.findElement({ id: 'output' });
    await driver.wait(async function () {
      const statusText = await statusElement.getText();
      return statusText !== 'Running';
    }, 10000);

    const finalOutputText = await outputElement.getText();
    const finalStatusText = await statusElement.getText();

    console.log(finalOutputText);
    if (finalStatusText !== 'exit: 0') {
      throw new Error(`Test finished with with status text: ${finalStatusText}`);
    }

    statusCode = 0;
  } catch (e) {
    console.error('Exception in test driver:');
    console.error(e);
  } finally {
    try {
      await driver.quit();
    } finally {
      process.exit(statusCode);
    }
  }
})();
