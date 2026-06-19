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
  if (settings.BackendUrl) {
    api.storeBackendUrl(settings.BackendUrl);
    delete settings.BackendUrl;
  }
});

Pebble.addEventListener('ready', function() {
  console.log('Merlin PKJS ready');
  try {
    var saved = JSON.parse(localStorage.getItem('clay-settings'));
    if (saved && saved.GeminiApiKey) {
      api.storeGeminiKey(saved.GeminiApiKey);
    }
    if (saved && saved.BackendUrl) {
      api.storeBackendUrl(saved.BackendUrl);
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
