var MAX_BYTES_IN_FLIGHT = 400;
var MAX_MESSAGES_IN_FLIGHT = 6;
var MAX_RETRY_ATTEMPTS = 5;

function MessageQueue() {
  this.queue = [];
  this.messagesInFlight = 0;
  this.bytesInFlight = 0;
}

function countBytes(message) {
  var bytes = 0;
  for (var key in message) {
    if (!message.hasOwnProperty(key)) {
      continue;
    }
    var value = message[key];
    if (typeof value === 'string') {
      bytes += value.length;
    } else if (typeof value === 'number') {
      bytes += 4;
    } else if (typeof value === 'boolean') {
      bytes += 1;
    }
    bytes += 12;
  }
  return bytes;
}

MessageQueue.prototype.enqueue = function(message, attempt) {
  this.queue.push({ message: message, attempt: attempt || 0 });
  this.maybeSend();
};

MessageQueue.prototype.maybeSend = function() {
  if (this.queue.length === 0) {
    return;
  }
  if (this.messagesInFlight >= MAX_MESSAGES_IN_FLIGHT || this.bytesInFlight >= MAX_BYTES_IN_FLIGHT) {
    return;
  }

  var entry = this.queue.shift();
  var message = entry.message;
  var attempt = entry.attempt;
  var messageSize = countBytes(message);

  this.messagesInFlight++;
  this.bytesInFlight += messageSize;

  var self = this;
  Pebble.sendAppMessage(message, function() {
    self.messagesInFlight--;
    self.bytesInFlight -= messageSize;
    self.maybeSend();
  }, function(error) {
    self.messagesInFlight--;
    self.bytesInFlight -= messageSize;
    console.log('AppMessage failed: ' + JSON.stringify(error));

    if (attempt >= MAX_RETRY_ATTEMPTS) {
      console.log('AppMessage retry exhausted');
      self.maybeSend();
      return;
    }

    var delay = Math.min(1000, 10 * Math.pow(2, attempt));
    setTimeout(function() {
      self.queue.unshift({ message: message, attempt: attempt + 1 });
      self.maybeSend();
    }, delay);
  });
};

MessageQueue.prototype.enqueueResponse = function(text) {
  var chunkSize = 350;
  for (var i = 0; i < text.length; i += chunkSize) {
    this.enqueue({ RESPONSE: text.substring(i, i + chunkSize) });
  }
  this.enqueue({ RESPONSE_DONE: 1 });
};

MessageQueue.prototype.enqueueStatus = function(status) {
  this.enqueue({ STATUS: status });
};

MessageQueue.prototype.enqueueTimer = function(durationSeconds, label) {
  var message = { TIMER_DURATION: durationSeconds };
  if (label) {
    message.TIMER_NAME = label;
  }
  this.enqueue(message);
};

exports.queue = new MessageQueue();
