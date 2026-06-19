var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var api = require('./api');

var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) {
    return;
  }

  var settings = clay.getSettings(e.response);
  if (settings.GeminiApiKey) {
    api.storeGeminiKey(settings.GeminiApiKey);
    delete settings.GeminiApiKey;
  }
});

Pebble.addEventListener('ready', function() {
  console.log('Merlin PKJS ready (direct Gemini API)');
  try {
    var saved = JSON.parse(localStorage.getItem('clay-settings'));
    if (saved && saved.GeminiApiKey) {
      api.storeGeminiKey(saved.GeminiApiKey);
    }
  } catch (err) {
    console.log('No saved Clay settings yet');
  }
});

Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload;
  if (payload.DICTATION_TEXT) {
    api.queryMerlin(payload.DICTATION_TEXT, false);
    return;
  }
  if (payload.FEEDBACK_TEXT) {
    api.queryMerlin(payload.FEEDBACK_TEXT, true);
  }
});
