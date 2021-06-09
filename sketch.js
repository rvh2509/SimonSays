/*
 * Richard Halbert
 * Made for University of Washington
 * CSE 490F MP3 Spring 2021
 * 
 * Runs a version of the game "Simon Says" where the user has
 * to repeat a sequence of numbers on either the buttons
 * connected to the Arduino or through holding the correct
 * number of fingers up in their browser. See MP3_Sketch/MP3_Sketch.ino
 * for the Arduino side of the code.
 * 
 * All Serial commmuncation is based on lessons from CSE 490F:
 * https://makeabilitylab.github.io/physcomp/communication/p5js-serial-io.html
 */

// constants for finger points in prediction.landmarks
const fingers = [0, 5, 9, 13, 17, 21]
const thumb = 5;

// time for the user to enter a number
const numberInterval = 4000;

// game variables
let serial;
let video;
let handPose;
let predictions = [];
let gameMode = -1;
let sequenceLength;
let currentSequence = [];
let checkSequence = [];
let lastCheckTime = -1;
let lastRaised = -1;
let pointsReached = 0;
let roundStart = true;

// serial options for Serial object
let serialOptions = { baudRate: 115200 };

function setup() {
  createCanvas(640, 480);
  // Setup Web Serial using serial.js
  serial = new Serial();
  serial.on(SerialEvents.CONNECTION_OPENED, onSerialConnectionOpened);
  serial.on(SerialEvents.CONNECTION_CLOSED, onSerialConnectionClosed);
  serial.on(SerialEvents.DATA_RECEIVED, onSerialDataReceived);
  serial.on(SerialEvents.ERROR_OCCURRED, onSerialErrorOccurred);

  // If we have previously approved ports, attempt to connect with them
  serial.autoConnectAndOpenPreviouslyApprovedPort(serialOptions);

  video = createCapture(VIDEO);
  video.size(width, height);
  video.hide();

  // handPose initialization from ml5's reference:
  // https://learn.ml5js.org/#/reference/handpose
  handPose = ml5.handpose(video);
  handPose.on("predict", results => {
    predictions = results;
  });
}

function draw() {
  image(video, 0, 0, width, height);

  // if we've been told to by the Arduino, start the game
  if (gameMode == 0) {
    if (roundStart) {
      lastCheckTime = millis();
      roundStart = false;
    }

    let currTime = millis();
    
    // display which number the user is on
    textSize(20);
    textAlign(CENTER);
    fill(255);
    noStroke();
    text("Enter item " + (currentSequence.length + 1) + " of the sequence:", width / 2, 20);
    
    // display the seconds remaining to enter the correct number
    textSize(100);
    textAlign(CENTER);
    fill(255);
    noStroke();
    text(Math.floor((numberInterval - currTime + lastCheckTime) / 1000), width/2, height / 2);

    // fingers raised
    let raised = 0;
    for (let i = 0; i < predictions.length; i++) {
      const prediction = predictions[i];
      for (let j = 1; j < fingers.length; j++) {
        if (isFingerRaised(fingers[j - 1], fingers[j], prediction)) {
          raised++;
        }
      }
    }

    if (predictions.length != 0) {
      if (currTime - lastCheckTime < numberInterval) {
        if (raised == checkSequence[currentSequence.length]) {
          currentSequence.push(raised);
          lastCheckTime = currTime;
          console.log("Correct!")
        }
      }
    }

    if (currTime - lastCheckTime >= numberInterval) {
      console.log("Ran out of time!");
      serialWriteTextData(0);
      currentSequence = [];
      checkSequence = [];
      gameMode = -1;
    } else if (currentSequence.length == sequenceLength) {
      let win = 1;
      for (let i = 0; i < sequenceLength; i++) {
        if (currentSequence[i] != checkSequence[i]) {
          win = 0;
        }
      }
      serialWriteTextData(win);
      currentSequence = [];
      checkSequence = [];
      gameMode = -1;
    }
  }
}

/*
 * Called when a Serial error occurs
 */
function onSerialErrorOccurred(eventSender, error) {
  console.log("onSerialErrorOccurred", error);
}

/*
 * Called when Serial connection opens
 */
function onSerialConnectionOpened(eventSender) {
  console.log("onSerialConnectionOpened");
}

/*
 * Called when Serial connection closes
 */
function onSerialConnectionClosed(eventSender) {
  console.log("onSerialConnectionClosed");
}

/*
 * Called when Serial data is recieved from the Arduino
 * newData format: gameMode, sequenceLength, sequence[]
 */
function onSerialDataReceived(eventSender, newData) {
  console.log("onSerialDataReceived", newData);

  let splitInput = newData.split(",");
  gameMode = parseInt(splitInput[0]);
  sequenceLength = parseInt(splitInput[1]);

  for (let i = 0; i < sequenceLength; i++) {
    checkSequence.push(splitInput[2 + i]);
  }
  roundStart = true;
}

/*
 * Write textData to the Arduino
 */
async function serialWriteTextData(textData) {
  if (serial.isOpen()) {
    serial.writeLine(textData);
  }
}

/*
 * Returns true iff the current finger is raised, unless the
 * finger is a thumb, in which case it returns false
 */
function isFingerRaised(prevFinger, thisFinger, prediction) {
  // thumbs caused too many issues, so we don't count them
  if (thisFinger == thumb) {
    return false;
  }

  let currCoord = prediction.landmarks[prevFinger][1];
  for (let i = prevFinger + 1; i < thisFinger; i++) {
    const keypoint = prediction.landmarks[i];
    const newCoord = keypoint[1];
    if (newCoord >= currCoord) {
      return false
    }
    currCoord = newCoord;
  }
  return true;
}
