var API_URL_ROOT = 'https://timeline-api.rebble.io/';

function timelineRequest(pin, type, callback) {
  var url = API_URL_ROOT + 'v1/user/pins/' + pin.id;
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    console.log('timeline: response ' + this.responseText);
    if (callback) {
      callback(null, this.responseText);
    }
  };
  xhr.onerror = function() {
    if (callback) {
      callback(new Error('Timeline request failed'));
    }
  };
  xhr.open(type, url);
  xhr.setRequestHeader('Content-Type', 'application/json');

  Pebble.getTimelineToken(function(token) {
    xhr.setRequestHeader('X-User-Token', '' + token);
    xhr.send(JSON.stringify(pin));
  }, function(error) {
    console.log('timeline token error: ' + error);
    if (callback) {
      callback(new Error('Timeline token error: ' + error));
    }
  });
}

exports.insertUserPin = function(pin, callback) {
  timelineRequest(pin, 'PUT', callback);
};
