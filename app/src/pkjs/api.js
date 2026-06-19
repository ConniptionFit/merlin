var messageQueue = require('./lib/message_queue').queue;
var timeline = require('./timeline');

var STORAGE_KEY = 'geminiKey';
var LAST_PROMPT_KEY = 'lastPrompt';
var LAST_REPLY_KEY = 'lastReply';
var GEMINI_MODEL = 'gemini-2.0-flash';
var GEMINI_API_ROOT = 'https://generativelanguage.googleapis.com/v1beta/models/';
var MAX_REPLY_CHARS = 250;

var FUNCTION_DECLARATIONS = [
  {
    name: 'set_timer',
    description: 'Set a countdown timer on the watch that will buzz when complete.',
    parameters: {
      type: 'OBJECT',
      properties: {
        duration_seconds: {
          type: 'INTEGER',
          description: 'Seconds from now until the timer fires.'
        },
        label: {
          type: 'STRING',
          description: 'Short label for the timer.'
        }
      },
      required: ['duration_seconds']
    }
  },
  {
    name: 'set_timeline_pin',
    description: 'Create a timeline reminder pin on the user phone.',
    parameters: {
      type: 'OBJECT',
      properties: {
        time: {
          type: 'STRING',
          description: 'ISO8601 time for the pin.'
        },
        title: {
          type: 'STRING',
          description: 'Pin title.'
        },
        body: {
          type: 'STRING',
          description: 'Pin subtitle or body text.'
        }
      },
      required: ['time', 'title']
    }
  },
  {
    name: 'get_weather',
    description: 'Answer a weather question using the user approximate location context.',
    parameters: {
      type: 'OBJECT',
      properties: {}
    }
  }
];

function getGeminiKey() {
  return localStorage.getItem(STORAGE_KEY);
}

function storeGeminiKey(key) {
  if (key) {
    localStorage.setItem(STORAGE_KEY, key);
  }
}

function sendStatus(status) {
  messageQueue.enqueueStatus(status);
}

function truncateReply(text) {
  if (!text) {
    return '';
  }
  text = text.trim();
  if (text.length <= MAX_REPLY_CHARS) {
    return text;
  }
  return text.substring(0, MAX_REPLY_CHARS);
}

function generatePinId() {
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    var r = Math.random() * 16 | 0;
    var v = c === 'x' ? r : (r & 0x3 | 0x8);
    return v.toString(16);
  });
}

function buildSystemPrompt(city, timezone) {
  return 'You are an eccentric pixelated wizard inside a watch. Answer in <250 characters. ' +
    'User is near ' + city + ', timezone ' + timezone + '. ' +
    'Use set_timer for countdown requests, set_timeline_pin for future reminders, get_weather for weather questions.';
}

function reverseGeocodeCity(lat, lon, callback) {
  if (typeof lat !== 'number' || typeof lon !== 'number') {
    callback('unknown location');
    return;
  }

  var url = 'https://nominatim.openstreetmap.org/reverse?format=jsonv2&lat=' +
    encodeURIComponent(lat) + '&lon=' + encodeURIComponent(lon) + '&zoom=10';
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url);
  xhr.setRequestHeader('Accept', 'application/json');
  xhr.setRequestHeader('User-Agent', 'MerlinPebbleAssistant/1.0');
  xhr.onload = function() {
    if (xhr.status < 200 || xhr.status >= 300) {
      callback('unknown location');
      return;
    }
    try {
      var data = JSON.parse(xhr.responseText);
      var address = data.address || {};
      var city = address.city || address.town || address.village ||
        address.municipality || address.county || address.state || 'unknown location';
      callback(city);
    } catch (err) {
      callback('unknown location');
    }
  };
  xhr.onerror = function() {
    callback('unknown location');
  };
  xhr.send();
}

function extractGeminiResponse(data) {
  var result = {
    reply: '',
    intent: 'General',
    timer: null,
    timeline_pin: null
  };

  if (!data || !data.candidates || !data.candidates.length) {
    return result;
  }

  var parts = data.candidates[0].content && data.candidates[0].content.parts;
  if (!parts) {
    return result;
  }

  var textParts = [];
  var functionCall = null;

  for (var i = 0; i < parts.length; i++) {
    if (parts[i].text) {
      textParts.push(parts[i].text);
    }
    if (parts[i].functionCall) {
      functionCall = parts[i].functionCall;
    }
  }

  result.reply = truncateReply(textParts.join(' '));

  if (!functionCall) {
    if (!result.reply) {
      result.reply = 'The wizard ponders silently.';
    }
    return result;
  }

  var args = functionCall.args || {};

  if (functionCall.name === 'set_timer' && args.duration_seconds > 0) {
    result.intent = 'SetTimer';
    result.timer = {
      duration_seconds: args.duration_seconds,
      label: args.label || ''
    };
    if (!result.reply) {
      result.reply = 'Timer set, mortal.';
    }
  } else if (functionCall.name === 'set_timeline_pin') {
    result.intent = 'SetTimelinePin';
    result.timeline_pin = {
      id: generatePinId(),
      time: args.time || new Date(Date.now() + 3600000).toISOString(),
      layout: {
        type: 'generic',
        title: args.title || 'Reminder',
        subtitle: args.body || ''
      }
    };
    if (!result.reply) {
      result.reply = 'A reminder spell is cast.';
    }
  } else if (functionCall.name === 'get_weather') {
    result.intent = 'Weather';
    if (!result.reply) {
      result.reply = 'The clouds whisper, but say little.';
    }
  }

  return result;
}

function callGemini(apiKey, prompt, systemPrompt, onSuccess, onError) {
  var url = GEMINI_API_ROOT + GEMINI_MODEL + ':generateContent';
  var xhr = new XMLHttpRequest();
  xhr.open('POST', url);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.setRequestHeader('x-goog-api-key', apiKey);

  var body = {
    systemInstruction: {
      parts: [{ text: systemPrompt }]
    },
    contents: [{
      role: 'user',
      parts: [{ text: prompt }]
    }],
    tools: [{
      functionDeclarations: FUNCTION_DECLARATIONS
    }],
    generationConfig: {
      temperature: 0.5
    }
  };

  xhr.onload = function() {
    if (xhr.status >= 200 && xhr.status < 300) {
      try {
        onSuccess(JSON.parse(xhr.responseText));
      } catch (err) {
        onError('Invalid Gemini JSON response');
      }
      return;
    }
    onError('Gemini HTTP ' + xhr.status);
  };

  xhr.onerror = function() {
    onError('Gemini network error');
  };

  xhr.send(JSON.stringify(body));
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
      sendStatus('Feedback noted');
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

function queryMerlin(prompt, isFeedback) {
  var key = getGeminiKey();
  if (!key) {
    sendStatus('Set Gemini key in phone config');
    return;
  }

  if (isFeedback) {
    console.log('Merlin feedback:', {
      feedback: prompt,
      original_prompt: localStorage.getItem(LAST_PROMPT_KEY) || '',
      original_reply: localStorage.getItem(LAST_REPLY_KEY) || ''
    });
    sendStatus('Feedback noted');
    return;
  }

  sendStatus('Thinking...');
  localStorage.setItem(LAST_PROMPT_KEY, prompt);

  var timezone = Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC';

  function runWithLocation(city) {
    var systemPrompt = buildSystemPrompt(city, timezone);
    callGemini(key, prompt, systemPrompt, function(geminiData) {
      dispatchQueryResponse(extractGeminiResponse(geminiData), false);
    }, function(error) {
      sendStatus(error);
    });
  }

  if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(function(position) {
      reverseGeocodeCity(position.coords.latitude, position.coords.longitude, runWithLocation);
    }, function() {
      runWithLocation('unknown location');
    }, { timeout: 10000, maximumAge: 600000 });
  } else {
    runWithLocation('unknown location');
  }
}

exports.queryMerlin = queryMerlin;
exports.storeGeminiKey = storeGeminiKey;
