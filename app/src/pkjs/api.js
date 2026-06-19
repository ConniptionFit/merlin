var messageQueue = require('./lib/message_queue').queue;
var timeline = require('./timeline');

var STORAGE_KEY = 'geminiKey';
var BACKEND_KEY = 'backendUrl';
var LAST_PROMPT_KEY = 'lastPrompt';
var LAST_REPLY_KEY = 'lastReply';
var DEFAULT_BACKEND = 'http://localhost:8080';

function getGeminiKey() {
  return localStorage.getItem(STORAGE_KEY);
}

function getBackendUrl() {
  return localStorage.getItem(BACKEND_KEY) || DEFAULT_BACKEND;
}

function storeGeminiKey(key) {
  if (key) {
    localStorage.setItem(STORAGE_KEY, key);
  }
}

function storeBackendUrl(url) {
  if (url) {
    localStorage.setItem(BACKEND_KEY, url);
  }
}

function sendStatus(status) {
  messageQueue.enqueueStatus(status);
}

function handleTimelinePin(pinData) {
  if (!pinData) {
    return;
  }
  timeline.insertUserPin(pinData, function(error) {
    if (error) {
      sendStatus('Timeline pin failed');
    }
  });
}

function dispatchQueryResponse(data, isFeedback) {
  if (!data) {
    sendStatus('Empty response');
    return;
  }

  if (data.reply) {
    localStorage.setItem(LAST_REPLY_KEY, data.reply);
    if (!isFeedback) {
      messageQueue.enqueueResponse(data.reply);
    } else {
      sendStatus('Feedback sent');
    }
  }

  if (data.intent === 'SetTimer' && data.timer && data.timer.duration_seconds) {
    messageQueue.enqueueTimer(data.timer.duration_seconds, data.timer.label || '');
  }

  if (data.intent === 'SetTimelinePin' && data.timeline_pin) {
    handleTimelinePin(data.timeline_pin);
  }

  if (!data.reply && !isFeedback) {
    messageQueue.enqueueResponse('The wizard is silent.');
  }
}

function postJson(path, body, headers, onSuccess, onError) {
  var backend = getBackendUrl().replace(/\/$/, '');
  var xhr = new XMLHttpRequest();
  xhr.open('POST', backend + path);
  xhr.setRequestHeader('Content-Type', 'application/json');

  for (var key in headers) {
    if (headers.hasOwnProperty(key)) {
      xhr.setRequestHeader(key, headers[key]);
    }
  }

  xhr.onload = function() {
    if (xhr.status >= 200 && xhr.status < 300) {
      try {
        onSuccess(JSON.parse(xhr.responseText));
      } catch (err) {
        onError('Invalid JSON response');
      }
      return;
    }
    onError('HTTP ' + xhr.status + ': ' + xhr.responseText);
  };

  xhr.onerror = function() {
    onError('Network error');
  };

  xhr.send(JSON.stringify(body));
}

function queryMerlin(prompt, isFeedback) {
  var key = getGeminiKey();
  if (!key) {
    sendStatus('Set Gemini key in phone config');
    return;
  }

  sendStatus('Thinking...');

  var requestBody = {
    prompt: prompt,
    feedback: !!isFeedback,
    timezone: Intl.DateTimeFormat().resolvedOptions().timeZone
  };

  if (isFeedback) {
    requestBody.feedback_text = prompt;
    requestBody.original_prompt = localStorage.getItem(LAST_PROMPT_KEY) || '';
    requestBody.original_reply = localStorage.getItem(LAST_REPLY_KEY) || '';
  } else {
    localStorage.setItem(LAST_PROMPT_KEY, prompt);
  }

  function runQuery(lat, lon) {
    var headers = { 'X-Gemini-Key': key };
    if (typeof lat === 'number') {
      headers['X-Latitude'] = '' + lat;
    }
    if (typeof lon === 'number') {
      headers['X-Longitude'] = '' + lon;
    }

    var path = isFeedback ? '/feedback' : '/query';
    postJson(path, requestBody, headers, function(data) {
      if (isFeedback) {
        sendStatus('Feedback sent');
        return;
      }
      dispatchQueryResponse(data, false);
    }, function(error) {
      sendStatus(error);
    });
  }

  if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(function(position) {
      runQuery(position.coords.latitude, position.coords.longitude);
    }, function() {
      runQuery(null, null);
    }, { timeout: 10000, maximumAge: 600000 });
  } else {
    runQuery(null, null);
  }
}

exports.queryMerlin = queryMerlin;
exports.storeGeminiKey = storeGeminiKey;
exports.storeBackendUrl = storeBackendUrl;
exports.getBackendUrl = getBackendUrl;
