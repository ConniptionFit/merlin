#!/usr/bin/env node
'use strict';

var assert = require('assert');
var fs = require('fs');
var path = require('path');
var vm = require('vm');

function loadModule(filePath, sandbox) {
  var code = fs.readFileSync(filePath, 'utf8');
  var module = { exports: {} };
  var context = {
    module: module,
    exports: module.exports,
    require: sandbox.require,
    console: console,
    Pebble: sandbox.Pebble,
    localStorage: sandbox.localStorage,
    setTimeout: setTimeout,
    XMLHttpRequest: sandbox.XMLHttpRequest,
    navigator: sandbox.navigator,
    Intl: Intl
  };
  vm.runInNewContext(code, context, { filename: filePath });
  return module.exports;
}

var sent = [];
var Pebble = {
  sendAppMessage: function(message, ok) {
    sent.push(message);
    if (ok) {
      ok();
    }
  }
};

var queue = loadModule(path.join(__dirname, '../app/src/pkjs/lib/message_queue.js'), {
  require: function() { throw new Error('unexpected require'); },
  Pebble: Pebble
}).queue;

sent = [];
queue.enqueueResponse('abcdefghijklmnopqrstuvwxyz');
assert.strictEqual(sent.length, 2);
assert.ok(sent[0].RESPONSE);
assert.ok(sent[1].RESPONSE_DONE);

sent = [];
var longText = new Array(400).join('x');
queue.enqueueResponse(longText);
assert.ok(sent.length >= 2);
assert.ok(sent.every(function(m) { return m.RESPONSE || m.RESPONSE_DONE; }));

console.log('message_queue tests passed');
