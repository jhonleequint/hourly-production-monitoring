// Illustrative example: newline-delimited text over a raw TCP socket.
// Cameras send one result per line; a single 'data' event may carry a partial
// line or several lines, so the reader buffers until it sees '\n'.

const net = require('net');

function createLineSplitter(onLine) {
  let buffer = '';
  return (chunk) => {
    buffer += chunk.toString('utf8');
    let index;
    while ((index = buffer.indexOf('\n')) !== -1) {
      const line = buffer.slice(0, index).replace(/\r$/, '').trim();
      buffer = buffer.slice(index + 1);
      if (line) onLine(line);
    }
  };
}

function connectCamera({ host, port, onLine, retryMs = 3000 }) {
  const socket = net.createConnection({ host, port });
  socket.on('data', createLineSplitter(onLine));
  socket.on('error', () => {}); // 'close' follows and handles the retry
  socket.on('close', () => setTimeout(() => connectCamera({ host, port, onLine, retryMs }), retryMs));
  return socket;
}

module.exports = { createLineSplitter, connectCamera };

if (require.main === module) {
  const assert = require('assert');
  const lines = [];
  const split = createLineSplitter((l) => lines.push(l));
  split('OK\nN');
  split('G\r\nOK');
  split('\n');
  assert.deepStrictEqual(lines, ['OK', 'NG', 'OK']);
  console.log('tcp-line-reader: ok');
}
