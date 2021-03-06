var autoUpdateTimerID;

class webSocketActionObject {
  constructor(motor, direction, seconds) {
    this.data = {};
    this.data.type = "startMotor";
    this.data.attributes = {
      "motor": motor, "direction": direction, "seconds": seconds
    };
  }
}

class webSocketTimerObject {
  constructor(timestamp, seconds, active) {
    this.data = {};
    this.data.type = "timer";
    this.data.id = "1";
    this.data.attributes = { "active": active, "timestamp": timestamp, "seconds": seconds }
  }
}

function webSocketConnect(){
  // return new WebSocket('ws://192.168.178.39/ws');
  // return new WebSocket('ws://192.168.178.72/ws ');
  return new WebSocket('ws://' + location.hostname + '/ws');
}

var connection = webSocketConnect();

connection.onopen = function () {
  var iconElement = document.getElementById("connectionIcon");
  iconElement.classList.remove("filter-orange");
  iconElement.classList.remove("filter-red");
  iconElement.classList.add("filter-green");

  //activateAutoUpdate();
  toast("Connected");
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);

  var iconElement = document.getElementById("connectionIcon");
  iconElement.classList.remove("filter-green");
  iconElement.classList.remove("filter-red");
  iconElement.classList.add("filter-orange");
};
connection.onmessage = function (e) {
  var jsonData = JSON.parse(e.data).data;

  if (jsonData.type == "initial") {
    timerOnConnect(jsonData.attributes.timer1);
    statsOnConnect(jsonData.attributes);
  } else if (jsonData.type == "response") {
    toast(jsonData.attributes.message);
  }
};
connection.onclose = function () {
  var iconElement = document.getElementById("connectionIcon");
  iconElement.classList.remove("filter-orange");
  iconElement.classList.remove("filter-green");
  iconElement.classList.add("filter-red");

  toast('Connection lost')
};

function startMotorsForwards() {
  var obj = new webSocketActionObject(findActiveMotors(), "forwards", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

function startMotorsForwardsSeconds() {
  var seconds = document.getElementById("seconds_forwards").value;
  if (seconds > 0) {
    var obj = new webSocketActionObject(findActiveMotors(), "forwards", seconds * 1000);
    var jsonString = JSON.stringify(obj);
    console.log(jsonString);
    connection.send(jsonString);
  }
}

function startMotorsBackwardsSeconds() {
  var seconds = document.getElementById("seconds_backwards").value;
  if (seconds > 0) {
    var obj = new webSocketActionObject("backwards", seconds * 1000);
    var jsonString = JSON.stringify(obj);
    connection.send(jsonString);
  }
}

function startMotorsBackwards() {
  var obj = new webSocketActionObject("backwards", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

function stopMotors() {
  connection.send('{"data":{"type":"stopMotor"}}');
}

function saveTimer1() {
  var action = "saveTimer";
  var selectedTime = document.getElementById("timer1Time").value;
  var selectedHour = parseInt(selectedTime.split(":")[0]);
  var selectedMinute = parseInt(selectedTime.split(":")[1]);
  var forSeconds = parseInt(document.getElementById("timer1Seconds").value);
  var radioActivated = document.getElementById("timer1Activated").checked;
  var radioDeactivated = document.getElementById("timer1Deactivated").checked;

  var calculatedDate = new Date(1970, 0, 1, selectedHour, selectedMinute);
  var calculatedTimestamp = Math.round(calculatedDate.getTime() / 1000);

  if (selectedTime != null && forSeconds != null && (radioActivated == true || radioDeactivated == true)) {
    if (radioActivated == true) {
      var active = true;
    } else {
      active = false;
    }
    //var timerObject = new webSocketTimerObject(action, selectedHour, selectedMinute, forSeconds, active);
    var timerObject = new webSocketTimerObject(calculatedTimestamp, forSeconds, active);
    var jsonString = JSON.stringify(timerObject);

    connection.send(jsonString);
  }
}

function timerOnConnect(jsonDataTimer) {
  var timestampToDate = new Date(jsonDataTimer.timestamp * 1000);

  document.getElementById("timer1Time").value = leadingZero(timestampToDate.getHours()) + ':' + leadingZero(timestampToDate.getMinutes());

  document.getElementById("timer1Seconds").value = jsonDataTimer.seconds

  if (jsonDataTimer.active == true) {
    document.getElementById("timer1Activated").checked = true;
  } else {
    document.getElementById("timer1Deactivated").checked = true;
  }
}

function statsOnConnect(jsonDataAttributes) {
  var dateBootTime = new Date(jsonDataAttributes.boottime * 1000);
  var dateLastFeedTimeRight = new Date(jsonDataAttributes.rightlastfeedtime * 1000);
  var dateLastFeedTimeLeft = new Date(jsonDataAttributes.leftlastfeedtime * 1000);

  document.getElementById("bootTime").value = dateBootTime.toLocaleString();
  document.getElementById("rebootReasonCpu0").value = jsonDataAttributes.rebootreasoncpu0;
  document.getElementById("rebootReasonCpu1").value = jsonDataAttributes.rebootreasoncpu1;
  document.getElementById("freeHeap").value = jsonDataAttributes.freeheap;
  document.getElementById("rightLastFeedDuration").value = jsonDataAttributes.rightlastfeedduration / 1000;
  document.getElementById("rightLastFeedTime").value = dateLastFeedTimeRight.toLocaleString();
  document.getElementById("leftLastFeedDuration").value = jsonDataAttributes.leftlastfeedduration / 1000;
  document.getElementById("leftLastFeedTime").value = dateLastFeedTimeLeft.toLocaleString();
  document.getElementById("wifiRssi").value = jsonDataAttributes.rssi;

  var dateNow = new Date();
  document.getElementById("lastUpdate").innerText = dateNow.toLocaleString();
}

function leadingZero(number) {
  if (number < 10) {
    return '0' + number;
  } else {
    return number;
  }
}

function rebootESP() {
  connection.send('{"data":{"type":"reboot"}}');
}

function flushPreferencesESP() {
  connection.send('{"action":"flush"}');
}

function leftMotor() {
  setButtonSecondary("rightMotorButton");
  setButtonSecondary("bothMotorsButton");
  setButtonPrimary("leftMotorButton");
}

function rightMotor() {
  setButtonSecondary("leftMotorButton");
  setButtonSecondary("bothMotorsButton");
  setButtonPrimary("rightMotorButton");
}

function bothMotors() {
  setButtonSecondary("rightMotorButton");
  setButtonSecondary("leftMotorButton");
  setButtonPrimary("bothMotorsButton");
}

function setButtonSecondary(id) {
  document.getElementById(id).classList.remove("btn-primary");
  document.getElementById(id).classList.add("btn-secondary");
}

function setButtonPrimary(id) {
  document.getElementById(id).classList.remove("btn-secondary");
  document.getElementById(id).classList.add("btn-primary");
}

function findActiveMotors() {
  var father = document.getElementById("activeMotors");
  var descendents = father.getElementsByTagName('*');

  var i, e, d;
  for (i = 0; i < descendents.length; ++i) {
    e = descendents[i];
    for (j = 0; j < e.classList.length; ++j) {
      if (e.classList[j] == "btn-primary") {
        var activeMotors = e.id;
        break;
      }
      if (activeMotors != undefined) {
        break;
      }
    }
  }

  switch (activeMotors) {
    case 'bothMotorsButton':
      return 3;
      break;
    case 'leftMotorButton':
      return 1;
      break;
    case 'rightMotorButton':
      return 2;
      break;
  }
}

function toggleAutoUpdate() {
  //Find out current state
  var autoUpdateButton = document.getElementById("autoUpdateButton");

  if (autoUpdateButton.classList.contains("btn-primary")) {
    autoUpdateButton.classList.remove("btn-primary");
    autoUpdateButton.classList.add("btn-secondary");

    deactivateAutoUpdate();
  } else {
    autoUpdateButton.classList.remove("btn-secondary");
    autoUpdateButton.classList.add("btn-primary");

    connection.send('{"action":"statsUpdate"}');

    activateAutoUpdate();
  }
}

function activateAutoUpdate() {
  autoUpdateTimerID = window.setInterval(autoUpdate, 10000);
}

function deactivateAutoUpdate() {
  window.clearInterval(autoUpdateTimerID);
}

function autoUpdate() {
  if (document.hidden === false) {
    connection.send('{"action":"statsUpdate"}');
  }
}

function toast(text) {
  $.toast({
    text: text,
    showHideTransition: 'fade',
    allowToastClose: false,
    textAlign: 'center',
    position: 'bottom-center',
    hideAfter: 2000
  })
}