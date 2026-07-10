// Illustrative example: downtime tracked separately for the line input and output.
// Each device has its own idle timer; one device never resets the other, so a
// jam where the input stops while the output keeps draining is still counted.

const IDLE_THRESHOLD_MS = 20000;

function createDeviceTimer(now = Date.now()) {
  return { lastPulse: now, downtimeMs: 0, stoppedSince: null };
}

function pulse(timer, now) {
  if (timer.stoppedSince !== null) {
    timer.downtimeMs += now - timer.stoppedSince;
    timer.stoppedSince = null;
  }
  timer.lastPulse = now;
}

function tick(timer, now) {
  if (timer.stoppedSince === null && now - timer.lastPulse >= IDLE_THRESHOLD_MS) {
    timer.stoppedSince = timer.lastPulse + IDLE_THRESHOLD_MS;
  }
}

function currentDowntime(timer, now) {
  return timer.downtimeMs + (timer.stoppedSince !== null ? now - timer.stoppedSince : 0);
}

function lineDowntime(input, output, now) {
  return currentDowntime(input, now) + currentDowntime(output, now);
}

module.exports = { IDLE_THRESHOLD_MS, createDeviceTimer, pulse, tick, currentDowntime, lineDowntime };

if (require.main === module) {
  const assert = require('assert');
  const input = createDeviceTimer(0);
  const output = createDeviceTimer(0);
  tick(input, 30000);           // input idle since 20 s
  pulse(output, 25000);         // output still producing
  pulse(input, 50000);          // input resumes: 30 s of downtime
  assert.strictEqual(currentDowntime(input, 50000), 30000);
  assert.strictEqual(currentDowntime(output, 50000), 0); // never stopped
  tick(output, 50000);          // output idle since 45 s
  assert.strictEqual(lineDowntime(input, output, 60000), 30000 + 15000);
  console.log('downtime-timer: ok');
}
