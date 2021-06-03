// constants for finger points in prediction.landmarks
const fingers = [0, 5, 9, 13, 17, 21]
const thumb = 5;

//let pHtmlMsg;
let serial;
let video;
let handPose;
let predictions = [];
let gameMode = -1;
let sequenceLength;
let currentSequence = [];
let checkSequence = [];
let lastRaised = -1;

let serialOptions = { baudRate: 115200 };

function setup() {
  createCanvas(400, 400);
  // Setup Web Serial using serial.js
  serial = new Serial();
  serial.on(SerialEvents.CONNECTION_OPENED, onSerialConnectionOpened);
  serial.on(SerialEvents.CONNECTION_CLOSED, onSerialConnectionClosed);
  serial.on(SerialEvents.DATA_RECEIVED, onSerialDataReceived);
  serial.on(SerialEvents.ERROR_OCCURRED, onSerialErrorOccurred);

  // If we have previously approved ports, attempt to connect with them
  serial.autoConnectAndOpenPreviouslyApprovedPort(serialOptions);

  //pHtmlMsg = createP("Click anywhere on this page to open the serial connection dialog");

  video = createCapture(VIDEO);
  video.size(width, height);
  handPose = ml5.handpose(video);
  // This sets up an event that fills the global variable "predictions"
  // with an array every time new hand poses are detected
  handPose.on("predict", results => {
    predictions = results;
  });

  video.hide();
}

function draw() {
  image(video, 0, 0, width, height);

  if (gameMode == 0) {
    // fingers raised
    let raised = 0;
    for (let i = 0; i < predictions.length; i++) {
      const prediction = predictions[i];
      for (let j = 1; j < fingers.length; j++) {
        if (isFingerRaised(fingers[j - 1], fingers[j], prediction)) {
          raised++;
        }
      }
      console.log(raised);
    }

    if (lastRaised != raised && predictions.length != 0) {
      currentSequence.push(raised);
      lastRaised = raised;
    }
    
    if (currentSequence.length == sequenceLength) {
      let win = 1;
      for (let i = 0; i < sequenceLength; i++) {
        if (currentSequence[i] != checkSequence[i]) {
          win = 0;
        }
      }
      serialWriteTextData(win);
      currentSequence = [];
    }
  } else if (gameMode == 1) {
    // match expression
    
  }
}

function onSerialErrorOccurred(eventSender, error) {
  console.log("onSerialErrorOccurred", error);
}

function onSerialConnectionOpened(eventSender) {
  console.log("onSerialConnectionOpened");
}

function onSerialConnectionClosed(eventSender) {
  console.log("onSerialConnectionClosed");
}

// format: gameMode, sequenceLength, sequence[]
function onSerialDataReceived(eventSender, newData) {
  console.log("onSerialDataReceived", newData);

  let splitInput = newData.split(",");
  gameMode = parseInt(splitInput[0]);
  sequenceLength = parseInt(splitInput[1]);
  console.log("Mode: " + gameMode + ", Length: " + sequenceLength);

  for (let i = 0; i < sequenceLength; i++) {
    checkSequence.push(splitInput[i]);
  }
}

async function serialWriteTextData(textData) {
  if (serial.isOpen()) {
    console.log("Writing to serial: ", textData);
    serial.writeLine(textData);
  }
}

function isFingerRaised(prevFinger, thisFinger, prediction) {
  let checkCoord = 1;
  if (thisFinger == thumb) {
    checkCoord = 0;
  }
  
  let rightHand = prediction.landmarks[0][0] < prediction.landmarks[thumb - 1][0];
  let currCoord = prediction.landmarks[prevFinger][1];
  for (let i = prevFinger + 1; i < thisFinger; i++) {
    const keypoint = prediction.landmarks[i];
    const newCoord = keypoint[checkCoord];
    if (thisFinger == thumb && rightHand && newCoord <= currCoord) {
      return false
    } else if (newCoord >= currCoord) {
      return false
    }
    currCoord = newCoord;
  }
  return true;
}

/*
function mouseClicked() {
  if (!serial.isOpen()) {
    serial.connectAndOpen(null, serialOptions);
  }
}
*/