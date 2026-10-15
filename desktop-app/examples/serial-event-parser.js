// Illustrative example: parsing the line protocol sent by the ESP32 gateway.
// Format: MODEL|EVENT|CATEGORY|QTY  (e.g. "CV-AX53(BR)1.6|DEFECT|SOLDER|1")

const EVENTS = new Set(['PRODUCTION', 'DEFECT', 'MODEL_SELECTED']);

function parseEventLine(line) {
  const parts = String(line).trim().split('|');
  if (parts.length !== 4) return null;

  const [model, event, category, qtyText] = parts.map((p) => p.trim());
  const quantity = Number.parseInt(qtyText, 10);

  if (!model || !EVENTS.has(event) || !Number.isInteger(quantity) || quantity < 0) return null;
  if (event === 'DEFECT' && !category) return null;

  return { model, event, category: category || null, quantity };
}

module.exports = { parseEventLine };

if (require.main === module) {
  const assert = require('assert');
  assert.deepStrictEqual(parseEventLine('CV-AX53(BR)1.6|DEFECT|SOLDER|1'),
    { model: 'CV-AX53(BR)1.6', event: 'DEFECT', category: 'SOLDER', quantity: 1 });
  assert.deepStrictEqual(parseEventLine('CV-AX53(BR)1.6|PRODUCTION||1\r\n'),
    { model: 'CV-AX53(BR)1.6', event: 'PRODUCTION', category: null, quantity: 1 });
  assert.strictEqual(parseEventLine('PACOTE RECEBIDO'), null);   // gateway debug output
  assert.strictEqual(parseEventLine('X|DEFECT||1'), null);       // defect without category
  assert.strictEqual(parseEventLine('X|PRODUCTION||abc'), null);
  console.log('serial-event-parser: ok');
}
