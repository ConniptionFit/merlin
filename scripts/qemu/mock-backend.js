#!/usr/bin/env node
'use strict';

/**
 * Merlin QEMU mock backend for integration testing.
 *
 * Usage:
 *   node scripts/qemu/mock-backend.js
 *   MOCK_MODE=slow MOCK_DELAY_MS=8000 node scripts/qemu/mock-backend.js
 *   MOCK_MODE=504 node scripts/qemu/mock-backend.js
 *   MOCK_MODE=empty node scripts/qemu/mock-backend.js
 *
 * Point Merlin Clay Backend URL to http://127.0.0.1:9090
 * (pebble emu-app-config in the app directory opens Clay in the phone simulator)
 */

var http = require('http');

var PORT = parseInt(process.env.MOCK_PORT || '9090', 10);
var MODE = process.env.MOCK_MODE || 'ok';
var DELAY_MS = parseInt(process.env.MOCK_DELAY_MS || '6000', 10);

function json(res, status, body) {
  var payload = JSON.stringify(body);
  res.writeHead(status, {
    'Content-Type': 'application/json',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Headers': 'Content-Type, X-Gemini-Key, X-Latitude, X-Longitude',
    'Access-Control-Allow-Methods': 'GET, POST, OPTIONS'
  });
  res.end(payload);
}

function handleQuery(req, res, body) {
  console.log('[mock] POST /query mode=%s prompt=%j', MODE, body.prompt);

  if (!req.headers['x-gemini-key']) {
    return json(res, 401, { error: 'missing X-Gemini-Key' });
  }

  if (MODE === '504') {
    return json(res, 504, { error: 'Gateway Timeout' });
  }

  if (MODE === 'empty') {
    return json(res, 200, {});
  }

  var respond = function() {
    if (MODE === 'slow') {
      console.log('[mock] responding after %dms delay', DELAY_MS);
    }
    var reply = 'A swift wizardly stew: sear onions, simmer stock, stir until creamy.';
    if (body.prompt && body.prompt.length > 200) {
      reply = 'Your lengthy incantation was heard, but answers stay under 250 chars, mortal.';
    }
    json(res, 200, {
      reply: reply,
      intent: 'General',
      timer: null,
      timeline_pin: null
    });
  };

  if (MODE === 'slow' || MODE === 'timeout') {
    var delay = MODE === 'timeout' ? DELAY_MS * 3 : DELAY_MS;
    console.log('[mock] delaying %dms (simulate slow network)', delay);
    setTimeout(respond, delay);
    return;
  }

  respond();
}

function handleFeedback(req, res, body) {
  console.log('[mock] POST /feedback text=%j', body.feedback_text || body.prompt);
  json(res, 200, { ok: true });
}

var server = http.createServer(function(req, res) {
  if (req.method === 'OPTIONS') {
    res.writeHead(204, {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Headers': 'Content-Type, X-Gemini-Key, X-Latitude, X-Longitude',
      'Access-Control-Allow-Methods': 'GET, POST, OPTIONS'
    });
    return res.end();
  }

  if (req.method === 'GET' && req.url === '/health') {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    return res.end('merlin-mock');
  }

  var chunks = [];
  req.on('data', function(c) { chunks.push(c); });
  req.on('end', function() {
    var raw = Buffer.concat(chunks).toString('utf8');
    var body = {};
    try {
      body = raw ? JSON.parse(raw) : {};
    } catch (err) {
      return json(res, 400, { error: 'invalid json' });
    }

    if (req.method === 'POST' && req.url === '/query') {
      return handleQuery(req, res, body);
    }
    if (req.method === 'POST' && req.url === '/feedback') {
      return handleFeedback(req, res, body);
    }

    json(res, 404, { error: 'not found' });
  });
});

server.listen(PORT, '0.0.0.0', function() {
  console.log('Merlin mock backend listening on http://127.0.0.1:%d mode=%s', PORT, MODE);
  if (MODE === 'slow' || MODE === 'timeout') {
    console.log('Delay: %dms', DELAY_MS * (MODE === 'timeout' ? 3 : 1));
  }
});
